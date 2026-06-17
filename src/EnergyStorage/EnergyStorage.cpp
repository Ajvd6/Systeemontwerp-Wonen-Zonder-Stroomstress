#include <Arduino.h>
#include "EnergyStorage.h"

EnergyStorage::EnergyStorage(int boilerPin, int ledBoilerPin, int energyExportBuffer) : 
    _boilerPin(boilerPin),
    _ledBoilerPin(ledBoilerPin),
    _energyExport(0),
    _energyStored(0),
    _storedFraction(0),
    _storageCapacity(0),
    _maxBoilerPower(0),
    _energyExportBuffer(energyExportBuffer),
    _lastUpdateTime(0),
    _cycleStart(0),
    _cycleLength(1000),
    _onTime(0),
    _percentage(0),
    _isBoilerOn(false),
    _boilerHealthy(true),
    _totalCurrent(0.0f),
    _lastBoilerCheckTime(0),
    _boilerCheckStartTime(0),
    _boilerCheckInterval(1UL * 10UL * 1000UL),
    _boilerCheckDuration(500UL),
    _boilerCheckBaselineActivePower(0.0f),
    _boilerCheckSumActivePower(0.0f),
    _boilerCheckSamples(0),
    _boilerCheckRunning(false) {}

void EnergyStorage::init(int maxBoilerPower) {
    const float STORAGE_SECONDS = 10.0f;

    _maxBoilerPower = maxBoilerPower;
    _storageCapacity = _maxBoilerPower * STORAGE_SECONDS;
    _energyStored = 0.0f;
    _energyExport = 0.0f;
    _storedFraction = 0.0f;
    _totalCurrent = 0.0f;
    _lastUpdateTime = millis();
    _lastBoilerCheckTime = millis();
    _boilerCheckRunning = false;
    _boilerHealthy = true;
}

void EnergyStorage::onModbusData(const ModbusData& data) {
    updateValues(static_cast<int>(data.wattageExport), data.totalCurrent, data.activePower);
}

void EnergyStorage::updateValues(int energyExport, float totalCurrent, float activePower) {
    unsigned long now = millis();
    float deltaSeconds = (now - _lastUpdateTime) / 1000.0f;
    if (deltaSeconds <= 0.0f) {
        return;
    }
    _lastUpdateTime = now;

    _energyExport = energyExport < 0 ? 0.0f : static_cast<float>(energyExport);
    _totalCurrent = totalCurrent < 0.0f ? 0.0f : totalCurrent;
    _activePower = activePower;

    float availablePower = _energyExport - _energyExportBuffer;
    if (availablePower < 0.0f) {
        availablePower = 0.0f;
    }

    float boilerAveragePower = (_maxBoilerPower * _percentage) / 100.0f;
    float netPower = availablePower - boilerAveragePower;

    if (_boilerHealthy) {
        _energyStored += netPower * deltaSeconds;
    }

    if (_energyStored < 0.0f) {
        _energyStored = 0.0f;
    } else if (_energyStored > _storageCapacity) {
        _energyStored = _storageCapacity;
    }

    float targetFraction = 0.0f;
    if (_storageCapacity > 0.0f) {
        targetFraction = _energyStored / _storageCapacity;
    }
    if (targetFraction < 0.0f) {
        targetFraction = 0.0f;
    } else if (targetFraction > 1.0f) {
        targetFraction = 1.0f;
    }

    _storedFraction = targetFraction;
    _percentage = static_cast<int>(_storedFraction * 100.0f + 0.5f);

    if (!_boilerHealthy) {
        _energyStored = 0.0f;
        _storedFraction = 0.0f;
        _percentage = 0;
    }

    _onTime = (_percentage * _cycleLength) / 100;
}

void EnergyStorage::evaluateBoilerCheck() {
    // Health determined by the average active power seen while the boiler is forced on
    const float MIN_ACTIVE_POWER_DELTA = 20.0f;   // watts: require noticeable boiler power draw to consider it working
    const float MIN_SAMPLE_COUNT = 2;             // require at least a few samples

    float averageActivePower = (_boilerCheckSamples > 0) ? (_boilerCheckSumActivePower / _boilerCheckSamples) : _activePower;
    float activePowerDelta = averageActivePower - _boilerCheckBaselineActivePower;
    bool passed = (_boilerCheckSamples >= MIN_SAMPLE_COUNT) && (activePowerDelta >= MIN_ACTIVE_POWER_DELTA);

    if (passed) {
        // Boiler draws current on average -> mark healthy
        _boilerHealthy = true;
    } else {
        // No significant draw -> mark unhealthy and ensure it's off
        _isBoilerOn = false;
        _boilerHealthy = false;
        _energyStored = 0.0f;
        _storedFraction = 0.0f;
        _percentage = 0;
        _onTime = 0;
        digitalWrite(_boilerPin, LOW);
        digitalWrite(_ledBoilerPin, LOW);
    }

    _boilerCheckRunning = false;
    _lastBoilerCheckTime = millis();
}

void EnergyStorage::modulateBoiler() {
    unsigned long now = millis();

    // Only start a health check if the boiler is currently off and enough time has passed
    if (!_boilerCheckRunning && !_isBoilerOn && (now - _lastBoilerCheckTime >= _boilerCheckInterval)) {
        _boilerCheckBaselineActivePower = _activePower;
        _boilerCheckSumActivePower = 0.0f;
        _boilerCheckSamples = 0;
        _boilerCheckStartTime = millis();
        _boilerCheckRunning = true;
    }

    // If boiler check is running
    if (_boilerCheckRunning) {
        // If the check duration has passed, evaluate the results
        if (now - _boilerCheckStartTime >= _boilerCheckDuration) {
            evaluateBoilerCheck();
        } else { // If the check is still running, turn on the boiler for the check
            _boilerCheckSumActivePower += _activePower;
            _boilerCheckSamples += 1;
            _isBoilerOn = true;
            digitalWrite(_boilerPin, HIGH);
            digitalWrite(_ledBoilerPin, HIGH);
            return;
        }
    }

    // If the boiler is not healthy, ensure it's turned off
    if (!_boilerHealthy) {
        _isBoilerOn = false;
        digitalWrite(_boilerPin, LOW);
        digitalWrite(_ledBoilerPin, LOW);
        return;
    }

    // Determine if we are in the ON phase of the cycle
    if(now - _cycleStart >= _cycleLength) {
        _cycleStart = now; // Start new cycle
    }

    // Determine if the boiler should be ON or OFF based on the current cycle
    if(now - _cycleStart < _onTime) {
        _isBoilerOn = true;
        digitalWrite(_boilerPin, HIGH); // Boiler ON
        digitalWrite(_ledBoilerPin, HIGH); // LED ON
    } else {
        _isBoilerOn = false;
        digitalWrite(_boilerPin, LOW); // Boiler OFF
        digitalWrite(_ledBoilerPin, LOW); // LED OFF
    }
}

