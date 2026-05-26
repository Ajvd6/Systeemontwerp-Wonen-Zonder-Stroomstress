#ifndef DISPLAYMANAGER_H
#define DISPLAYMANAGER_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

const uint8_t VALUE_ROW_START = 13;

class DisplayManager{
public:
    DisplayManager(uint8_t address, uint8_t cols, uint8_t rows);

    void init();

    void printStatus(bool isBoilerOn, float currentImport, int energyExport, int energyStored, int percentage);

private:
    LiquidCrystal_I2C _lcd;
};

#endif // DISPLAYMANAGER_H
