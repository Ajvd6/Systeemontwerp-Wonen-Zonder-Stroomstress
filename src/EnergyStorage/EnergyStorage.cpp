#include <Arduino.h>
#include "EnergyStorage.h"

EnergyStorage::EnergyStorage(int boilerPin, int ledBoilerPin, int energyExportBuffer) : 
    _boilerPin(boilerPin),
    _ledBoilerPin(ledBoilerPin),
    _energyExport(0),
    _energyStored(0),
    _maxBoilerPower(0),
    _energyExportBuffer(energyExportBuffer),
    _cycleStart(0),
    _cycleLength(1000),
    _onTime(0),
    _percentage(0),
    _isBoilerOn(false) {}

void EnergyStorage::init(int maxBoilerPower) {
    _maxBoilerPower = maxBoilerPower;
}

void EnergyStorage::onModbusData(const ModbusData& data) {
    updateValues(static_cast<int>(data.wattageExport));
}

void EnergyStorage::updateValues(int energyExport) {
    if (_isBoilerOn) {
        _energyStored = 0;
        _percentage = 0;
        return; // If boiler is on, we are not storing energy, so skip calculations
    }
    _energyExport = energyExport - _energyStored;
    _energyStored += _energyExport - _energyExportBuffer;
    
    if (_energyStored < 0) {
        _energyStored = 0; // Prevent negative storage
    } else if (_energyStored > _maxBoilerPower) {
        _energyStored = _maxBoilerPower; // Cap storage at max boiler power
    }

    _percentage = map(_energyStored, 0, _maxBoilerPower, 0, 100);
    _energyStored = map(_percentage, 0, 100, 0, _maxBoilerPower);

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

