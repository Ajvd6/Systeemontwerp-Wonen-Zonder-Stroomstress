#include <Arduino.h>
#include "DisplayManager.h"

DisplayManager::DisplayManager(uint8_t address, uint8_t cols, uint8_t rows) : 
    _lcd(address, cols, rows) {}

void DisplayManager::init() {
    _lcd.init();
    _lcd.backlight();
    _lcd.clear();

    _lcd.setCursor(0, 0);
    _lcd.print("Initialisatie...");
}

void DisplayManager::setup() {
    delay(1000);
    _lcd.clear();

    _lcd.setCursor(0, 0);
    _lcd.print("Verbruik:");
    _lcd.setCursor(19, 0);
    _lcd.print("A");

    _lcd.setCursor(0, 1);
    _lcd.print("Export:");
    _lcd.setCursor(19, 1);
    _lcd.print("W");

    _lcd.setCursor(0, 2);
    _lcd.print("Opslag:");
    _lcd.setCursor(19, 2);
    _lcd.print("W");

    _lcd.setCursor(0, 3);
    _lcd.print("Boiler:");
    _lcd.setCursor(13, 3);
    _lcd.print("On/Off");
}

void DisplayManager::boilerUpdate(int boilerMeasurement, int index) {
    _lcd.setCursor(0, 0);
    _lcd.print("Boiler vermogen:");

    _lcd.setCursor(0, index+1);
    _lcd.print("Meting:");
    _lcd.setCursor(13, index+1);
    _lcd.print(boilerMeasurement);
    _lcd.setCursor(19, index+1);
    _lcd.print("W");
}

void DisplayManager::printStatus(float currentImport, int energyExport, int energyStored, bool isBoilerOn) {

    _lcd.setCursor(VALUE_ROW_START, 0);
    _lcd.print("      ");
    _lcd.setCursor(VALUE_ROW_START, 0);
    _lcd.print(currentImport);

    _lcd.setCursor(VALUE_ROW_START, 1);
    _lcd.print("      ");
    _lcd.setCursor(VALUE_ROW_START, 1);
    _lcd.print(energyExport);

    _lcd.setCursor(VALUE_ROW_START, 2);
    _lcd.print("      ");
    _lcd.setCursor(VALUE_ROW_START, 2);
    _lcd.print(energyStored);

    _lcd.setCursor(VALUE_ROW_START, 3);
    _lcd.print("      ");
    _lcd.setCursor(VALUE_ROW_START, 3);
    _lcd.print(isBoilerOn ? "On" : "Off");
}

