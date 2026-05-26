#include <Arduino.h>
#include "LoadBalancer.h"

LoadBalancer::LoadBalancer(int SG_pin, int LED_SG_pin) : 
    _SG_pin(SG_pin), 
    _LED_SG_pin(LED_SG_pin),
    _totalCurrentImport(0),
    _heatpumpEnabled(true),
    _state(State::Normal),
    _stateStart(0) {}

void LoadBalancer::init() {
    pinMode(_SG_pin, OUTPUT);
    pinMode(_LED_SG_pin, OUTPUT);

    setHeatpumpEnabled(true);
    _state = State::Normal;
    _stateStart = millis();
}

void LoadBalancer::onModbusData(const ModbusData& data) {
    updateValues(data.totalCurrent);
    checkLoadBalancing();
}

void LoadBalancer::updateValues(float totalCurrentImport) {
    if (totalCurrentImport < 0) {
        totalCurrentImport = 0; // Ensure we don't have negative current
    }
    _totalCurrentImport = totalCurrentImport;
}

void LoadBalancer::setHeatpumpEnabled(bool enabled) {
    _heatpumpEnabled = enabled;
    digitalWrite(_SG_pin, enabled ? LOW : HIGH);
    digitalWrite(_LED_SG_pin, enabled ? LOW : HIGH);
}

void LoadBalancer::checkLoadBalancing() {
    unsigned long now = millis();

    switch (_state) {
        case State::Normal:
            if (_totalCurrentImport >= MAX_CURRENT_THRESHOLD) {
                _state = State::OverloadPending;
                _stateStart = now;
            }
            break;

        case State::OverloadPending:
            if (_totalCurrentImport < MAX_CURRENT_THRESHOLD) {
                _state = State::Normal;
                _stateStart = now;
            } else if (now - _stateStart >= OVERCURRENT_DURATION) {
                _state = State::Tripped;
                _stateStart = now;
                setHeatpumpEnabled(false);
            }
            break;

        case State::Tripped:
            if (_totalCurrentImport < SAFE_CURRENT_THRESHOLD) {
                _state = State::Cooldown;
                _stateStart = now;
            }
            break;

        case State::Cooldown:
            if (_totalCurrentImport >= MAX_CURRENT_THRESHOLD) {
                _state = State::Tripped;
                _stateStart = now;
            } else if (now - _stateStart >= RESTART_DELAY) {
                _state = State::Normal;
                _stateStart = now;
                setHeatpumpEnabled(true);
            }
            break;
    }
}

