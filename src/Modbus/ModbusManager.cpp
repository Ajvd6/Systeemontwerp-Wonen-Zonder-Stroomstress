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
    _state(0),
    _isReady(false),
    _pendingRequestGroup(RegNone),
    _lastRequestTime(0),
    _timeout(250),
    _data{0.0f, 0.0f, 0.0f, 0, false},
    _stableTotalCurrent(0.0f),
    _stableWattageExport(0.0f),
    _zeroCurrentCount(0),
    _zeroExportCount(0),
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
        default:
            Serial.println("[ERROR] Unexpected state trying to read Modbus registers");
            return;
    }

    _pendingRequestGroup = group;
    _mb.readIreg(_slaveId, address, buffer, 2, cbRead);
    _state = (_state + 1) % 2;
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

    if (_pendingRequestGroup == RegActivePower) {
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
        default:
            return;
    }

    float value = convertToFloat(regs);

    switch (group) {
        case RegCurrent: {
            float currentValue = value < 0.0f ? 0.0f : value;
            if (currentValue <= 0.0f) {
                if (_zeroCurrentCount < 255) {
                    _zeroCurrentCount++;
                }
            } else {
                _zeroCurrentCount = 0;
                _stableTotalCurrent = currentValue;
            }

            if (_zeroCurrentCount >= 5) {
                _data.totalCurrent = 0.0f;
                _stableTotalCurrent = 0.0f;
            } else {
                _data.totalCurrent = _stableTotalCurrent;
            }
            break;
        }
        case RegActivePower: {
            _data.activePower = value;
            float exportValue = value < 0.0f ? -value : 0.0f;
            if (exportValue <= 0.0f) {
                if (_zeroExportCount < 255) {
                    _zeroExportCount++;
                }
            } else {
                _zeroExportCount = 0;
                _stableWattageExport = exportValue;
            }

            if (_zeroExportCount >= 5) {
                _data.wattageExport = 0.0f;
                _stableWattageExport = 0.0f;
            } else {
                _data.wattageExport = _stableWattageExport;
            }
            break;
        }
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

