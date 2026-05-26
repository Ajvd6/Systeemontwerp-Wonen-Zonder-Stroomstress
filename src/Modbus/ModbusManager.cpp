#include <Arduino.h>
#include "ModbusManager.h"

ModbusManager* ModbusManager::_instance = nullptr;

ModbusManager::ModbusManager(int RX_pin, int TX_pin, int rs485_control_pin, int slave_ID) : 
    _rs485(RX_pin, TX_pin),
    _mb(),
    _rs485ControlPin(rs485_control_pin),
    _slaveId(slave_ID),
    _current{0, 0},
    _activePower{0, 0},
    _importEnergy{0, 0},
    _exportEnergy{0, 0},
    _state(0),
    _isReady(false),
    _pendingRequestGroup(RegNone),
    _lastRequestTime(0),
    _timeout(250),
    _data{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, false},
    _listenerCount(0) {
    _instance = this;
}

void ModbusManager::init() {
    pinMode(_rs485ControlPin, OUTPUT);
    digitalWrite(_rs485ControlPin, LOW); // Receive mode

    _rs485.begin(9600);
    _mb.begin(&_rs485, _rs485ControlPin);
    _mb.master();
    delay(50);

    _isReady = true;
    _mb.task();
}

void ModbusManager::update() {
    _mb.task();

    if (!_isReady && millis() - _lastRequestTime > _timeout) {
        Serial.println("Modbus timeout!");

        _current[0] = 0;
        _current[1] = 0;
        _activePower[0] = 0;
        _activePower[1] = 0;
        _importEnergy[0] = 0;
        _importEnergy[1] = 0;
        _exportEnergy[0] = 0;
        _exportEnergy[1] = 0;
        _data.valid = false;
        _isReady = true;
        _pendingRequestGroup = RegNone;
    }
}

void ModbusManager::readAllRegs() {
    if (!_isReady) {
        return;
    }

    uint16_t* buffer = nullptr;
    uint16_t address = 0;
    RequestGroup group = static_cast<RequestGroup>(_state);

    switch (group) {
        case RegCurrent:
            buffer = _current;
            address = CURRENT_ADDR;
            break;
        case RegActivePower:
            buffer = _activePower;
            address = ACTIVE_POWER_ADDR;
            break;
        case RegImportEnergy:
            buffer = _importEnergy;
            address = IMPORT_ENERGY_ADDR;
            break;
        case RegExportEnergy:
            buffer = _exportEnergy;
            address = EXPORT_ENERGY_ADDR;
            break;
        default:
            Serial.println("[ERROR] Unexpected state trying to read Modbus registers");
            return;
    }

    _pendingRequestGroup = group;
    _mb.readIreg(_slaveId, address, buffer, 2, cbRead);
    _state = (_state + 1) % 4;
    _isReady = false;
    _lastRequestTime = millis();
    _mb.task();
}

uint16_t ModbusManager::readReg(int regAddr) {
    if (!_isReady) {
        return 0;
    }

    uint16_t value[2];
    _mb.readIreg(_slaveId, regAddr, value, 2, cbRead);

    for (size_t i = 0; i < 100; i++) {
        _mb.task();
        delay(10);
    }

    return convertToInt(value);
}

bool ModbusManager::addListener(IModbusListener* listener) {
    if (listener == nullptr || _listenerCount >= MAX_LISTENERS) {
        return false;
    }

    for (uint8_t i = 0; i < _listenerCount; ++i) {
        if (_listeners[i] == listener) {
            return false;
        }
    }

    _listeners[_listenerCount++] = listener;
    return true;
}

bool ModbusManager::removeListener(IModbusListener* listener) {
    if (listener == nullptr) {
        return false;
    }

    for (uint8_t i = 0; i < _listenerCount; ++i) {
        if (_listeners[i] == listener) {
            for (uint8_t j = i; j + 1 < _listenerCount; ++j) {
                _listeners[j] = _listeners[j + 1];
            }
            --_listenerCount;
            return true;
        }
    }
    return false;
}

bool ModbusManager::cbRead(Modbus::ResultCode event, uint16_t transactionId, void* data) {
    if (_instance == nullptr) {
        return false;
    }
    return _instance->handleReadResponse(event, transactionId, data);
}

bool ModbusManager::handleReadResponse(Modbus::ResultCode event, uint16_t transactionId, void* data) {
    _isReady = true;

    if (event != Modbus::EX_SUCCESS) {
        Serial.print("Read error: ");
        Serial.println(event);
        _data.valid = false;
        _pendingRequestGroup = RegNone;
        return true;
    }

    if (_pendingRequestGroup == RegNone) {
        return true;
    }

    updateDataFromRegs(_pendingRequestGroup);

    if (_pendingRequestGroup == RegExportEnergy) {
        _data.timestamp = millis();
        _data.valid = true;
        notifyListeners();
    }

    _pendingRequestGroup = RegNone;
    return true;
}

void ModbusManager::updateDataFromRegs(RequestGroup group) {
    const uint16_t* regs = nullptr;

    switch (group) {
        case RegCurrent:
            regs = _current;
            break;
        case RegActivePower:
            regs = _activePower;
            break;
        case RegImportEnergy:
            regs = _importEnergy;
            break;
        case RegExportEnergy:
            regs = _exportEnergy;
            break;
        default:
            return;
    }

    float value = convertToFloat(regs);

    switch (group) {
        case RegCurrent:
            _data.totalCurrent = value;
            break;
        case RegActivePower:
            _data.activePower = value;
            _data.wattageExport = value < 0 ? -value : 0;
            break;
        case RegImportEnergy:
            _data.importEnergy = value;
            break;
        case RegExportEnergy:
            _data.exportEnergy = value;
            break;
        default:
            break;
    }
}

void ModbusManager::notifyListeners() {
    for (uint8_t i = 0; i < _listenerCount; ++i) {
        if (_listeners[i] != nullptr) {
            _listeners[i]->onModbusData(_data);
        }
    }
}

float ModbusManager::convertToFloat(const uint16_t* regs) const {
    uint32_t combined = ((uint32_t)(uint16_t)regs[0] << 16) | (uint16_t)regs[1];
    float value;
    memcpy(&value, &combined, sizeof(value));
    return value;
}

uint16_t ModbusManager::convertToInt(const uint16_t* regs) const {
    float f = convertToFloat(regs);
    return (uint16_t)f;
}

