#ifndef ENERGYSTORAGE_H
#define ENERGYSTORAGE_H

#include <Arduino.h>
#include "Modbus/ModbusManager.h"

class EnergyStorage : public IModbusListener {
public:
    EnergyStorage(int boilerPin, int ledBoilerPin, int energyExportBuffer);

    void init(int maxBoilerPower);

    void onModbusData(const ModbusData& data) override;
    void updateValues(int energyExport, float totalCurrent, float activePower);
    void modulateBoiler();

    // Accessors
    int getEnergyExport() const { return static_cast<int>(_energyExport + 0.5f); }
    int getEffectiveExport() const {
        int effectiveExport = static_cast<int>(_energyExport + 0.5f);
        if (_boilerHealthy) {
            effectiveExport -= getEnergyStored();
            if (effectiveExport < 0) {
                effectiveExport = 0;
            }
        }
        return effectiveExport;
    }
    int getEnergyStored() const { return _boilerHealthy ? static_cast<int>(_percentage * _maxBoilerPower / 100.0f + 0.5f) : 0; }
    int getPercentage() const { return _boilerHealthy ? _percentage : 0; }
    bool isBoilerOn() const { return _isBoilerOn; }
    bool isBoilerHealthy() const { return _boilerHealthy; }
    
private:
    int _boilerPin;
    int _ledBoilerPin;

    float _energyExport;
    float _energyStored;
    float _storedFraction;
    float _storageCapacity;

    unsigned int _maxBoilerPower;
    int _energyExportBuffer;

    unsigned long _lastUpdateTime;
    unsigned long _cycleStart;
    const unsigned long _cycleLength;

    unsigned long _onTime;

    int _percentage;

    bool _isBoilerOn;
    bool _boilerHealthy;

    float _totalCurrent;
    float _activePower;
    unsigned long _lastBoilerCheckTime;
    unsigned long _boilerCheckStartTime;
    const unsigned long _boilerCheckInterval;
    const unsigned long _boilerCheckDuration;
    float _boilerCheckBaselineActivePower;
    float _boilerCheckSumActivePower;
    int _boilerCheckSamples;
    bool _boilerCheckRunning;

    void evaluateBoilerCheck();
};

#endif // ENERGYSTORAGE_H
