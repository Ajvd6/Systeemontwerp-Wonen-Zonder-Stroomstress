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
    _isBoilerOn(false) {}

void EnergyStorage::init(int maxBoilerPower) {
    const float STORAGE_SECONDS = 10.0f;

    _maxBoilerPower = maxBoilerPower;
    _storageCapacity = _maxBoilerPower * STORAGE_SECONDS;
    _energyStored = 0.0f;
    _energyExport = 0.0f;
    _storedFraction = 0.0f;
    _lastUpdateTime = millis();
}

void EnergyStorage::onModbusData(const ModbusData& data) {
    updateValues(static_cast<int>(data.wattageExport));
}

void EnergyStorage::updateValues(int energyExport) {
    unsigned long now = millis();
    float deltaSeconds = (now - _lastUpdateTime) / 1000.0f;
    if (deltaSeconds <= 0.0f) {
        return;
    }
    _lastUpdateTime = now;

    _energyExport = energyExport < 0 ? 0.0f : static_cast<float>(energyExport);
    float availablePower = _energyExport - _energyExportBuffer;
    if (availablePower < 0.0f) {
        availablePower = 0.0f;
    }

    float boilerAveragePower = (_maxBoilerPower * _percentage) / 100.0f;
    float netPower = availablePower - boilerAveragePower;
    _energyStored += netPower * deltaSeconds;

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

    const float SMOOTHING_ALPHA = 0.2f;
    _storedFraction = _storedFraction * (1.0f - SMOOTHING_ALPHA) + targetFraction * SMOOTHING_ALPHA;

    _percentage = static_cast<int>(_storedFraction * 100.0f + 0.5f);

    if (_percentage < 0) {
        _percentage = 0;
    } else if (_percentage > 100) {
        _percentage = 100;
    }

    _onTime = (_percentage * _cycleLength) / 100;
}

void EnergyStorage::modulateBoiler() {
    if(millis() - _cycleStart >= _cycleLength) {
        _cycleStart = millis(); // Start new cycle
    }

    if(millis() - _cycleStart < _onTime) {
        _isBoilerOn = true;
        digitalWrite(_boilerPin, HIGH); // Boiler ON
        digitalWrite(_ledBoilerPin, HIGH); // LED ON
    } else {
        _isBoilerOn = false;
        digitalWrite(_boilerPin, LOW); // Boiler OFF
        digitalWrite(_ledBoilerPin, LOW); // LED OFF
    }
}

