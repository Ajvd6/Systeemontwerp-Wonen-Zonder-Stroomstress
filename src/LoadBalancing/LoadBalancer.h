#ifndef LOADBALANCER_H
#define LOADBALANCER_H

#include <Arduino.h>
#include "Modbus/ModbusManager.h"

const float MAX_CURRENT_THRESHOLD   = 30.0; // 30 Amps
const float SAFE_CURRENT_THRESHOLD  = 25.0; // 25 Amps

const unsigned long OVERCURRENT_DURATION    = 5000;     // 5000 ms = 5 seconds
const unsigned long RESTART_DELAY           = 900000;   // 900 000 ms = 15 minutes

class LoadBalancer : public IModbusListener {
public:
    LoadBalancer(int SG_pin, int LED_SG_pin);

    void init();

    void onModbusData(const ModbusData& data) override;
    void updateValues(float totalCurrentImport);
    void checkLoadBalancing();

    // Accessors
    float getTotalCurrentImport() const { return _totalCurrentImport; }
    bool isHeatpumpEnabled() const { return _heatpumpEnabled; }

private:
    enum class State {
        Normal,
        OverloadPending,
        Tripped,
        Cooldown
    };

    void setHeatpumpEnabled(bool enabled);

    int _SG_pin;
    int _LED_SG_pin;
    float _totalCurrentImport;
    bool _heatpumpEnabled;
    State _state;
    unsigned long _stateStart;
};

#endif // LOADBALANCER_H
