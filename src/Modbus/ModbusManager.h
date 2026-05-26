#ifndef MODBUSMANAGER_H
#define MODBUSMANAGER_H

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <ModbusRTU.h>

const uint16_t CURRENT_ADDR             = 0x0006;
const uint16_t ACTIVE_POWER_ADDR        = 0x000C;

struct ModbusData {
    float totalCurrent;
    float activePower;
    float wattageExport;
    unsigned long timestamp;
    bool valid;
};

class IModbusListener {
public:
    virtual void onModbusData(const ModbusData& data) = 0;
};

class ModbusManager {
public:
    ModbusManager(int RX_pin, int TX_pin, int rs485_control_pin, int slave_ID);

    void init();
    void update();
    void readAllRegs();
    uint16_t readReg(int regAddr);

    bool addListener(IModbusListener* listener);
    bool removeListener(IModbusListener* listener);

    const ModbusData& getData() const { return _data; }

    // Accessors
    float getTotalCurrent() const {
        if (_data.activePower < 0) {
            return 0;
        }
        return _data.totalCurrent;
    }

    float getActivePower() const {
        return _data.activePower;
    }

    float getWattageExport() const {
        return _data.wattageExport;
    }

    bool isExporting() const {
        return _data.activePower < 0;
    }

    bool isReady() const { return _isReady; }

private:
    enum RequestGroup : int {
        RegCurrent = 0,
        RegActivePower = 1,
        RegNone = -1
    };

    static const uint8_t MAX_LISTENERS = 6;

    SoftwareSerial _rs485;
    ModbusRTU _mb;

    int _rs485ControlPin;
    int _slaveId;

    uint16_t _current[2];
    uint16_t _activePower[2];

    int _state;
    bool _isReady;
    RequestGroup _pendingRequestGroup;
    unsigned long _lastRequestTime;
    const unsigned long _timeout;

    ModbusData _data;
    IModbusListener* _listeners[MAX_LISTENERS];
    uint8_t _listenerCount;

    static ModbusManager* _instance;
    static bool cbRead(Modbus::ResultCode event, uint16_t transactionId, void* data);
    bool handleReadResponse(Modbus::ResultCode event, uint16_t transactionId, void* data);
    void updateDataFromRegs(RequestGroup group);
    void notifyListeners();

    float convertToFloat(const uint16_t* regs) const;
    uint16_t convertToInt(const uint16_t* regs) const;
};

#endif // MODBUSMANAGER_H
