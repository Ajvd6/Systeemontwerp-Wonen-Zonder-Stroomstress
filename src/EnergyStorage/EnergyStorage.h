#ifndef ENERGYSTORAGE_H
#define ENERGYSTORAGE_H

#include <Arduino.h>
#include "Modbus/ModbusManager.h"

class EnergyStorage : public IModbusListener {
public:
    EnergyStorage(int boilerPin, int ledBoilerPin, int energyExportBuffer);

    void init(int maxBoilerPower);

    void onModbusData(const ModbusData& data) override;
    void updateValues(int energyExport);
    void modulateBoiler();

    // Accessors
    int getEnergyExport() const { return _energyExport; }
    int getEnergyStored() const { return _energyStored; }
    int getPercentage() const { return _percentage; }
    bool isBoilerOn() const { return _isBoilerOn; }
private:
    int _boilerPin;
    int _ledBoilerPin;

    int _energyExport; // int to prevent integeter wrap around
    int _energyStored; // int to prevent integeter wrap around

    unsigned int _maxBoilerPower;
    int _energyExportBuffer;

    unsigned long _cycleStart;
    const unsigned long _cycleLength;

    unsigned long _onTime;

    int _percentage;

    bool _isBoilerOn;
};

#endif // ENERGYSTORAGE_H
