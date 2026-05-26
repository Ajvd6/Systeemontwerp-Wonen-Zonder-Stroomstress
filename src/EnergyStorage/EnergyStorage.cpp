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
        _energyExport = 0;
        _energyStored = 0;
        _percentage = 0;
        _onTime = 0;
        Serial.println("Boiler is on, not storing energy");
        return; // If boiler is on, we are not storing energy
    }

    // Treat only positive export as available energy to store
    _energyExport = energyExport < 0 ? 0 : energyExport;

    // Available energy after applying buffer
    int availableToStore = _energyExport - _energyExportBuffer;
    if (availableToStore < 0) availableToStore = 0;

    // Update stored energy and clamp to [0, _maxBoilerPower]
    long newStored = (long)_energyStored + (long)availableToStore;
    if (newStored < 0) newStored = 0;
    if (newStored > (long)_maxBoilerPower) newStored = _maxBoilerPower;
    _energyStored = (int)newStored;

    // Compute percentage (avoid double mapping which quantized values oddly)
    if (_maxBoilerPower > 0) {
        _percentage = (int)((100L * _energyStored) / _maxBoilerPower);
    } else {
        _percentage = 0;
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

