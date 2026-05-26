#include <Arduino.h>
#include "Display/DisplayManager.h"
#include "EnergyStorage/EnergyStorage.h"
#include "LoadBalancing/LoadBalancer.h"
#include "Modbus/ModbusManager.h"
#include "Scheduler/Scheduler.h"

// Hardware constants
const int RX_PIN                        = 2;
const int TX_PIN                        = 3;
const int RS485_CONTROL_PIN             = 7;
const int MODBUS_SLAVE_ID               = 1;

const int SG_PIN                        = 6;
const int LED_SG_PIN                    = 13;
const int BOILER_PIN                    = 9;
const int LED_BOILER_PIN                = 12;

const int DISPLAY_ADDR                  = 0x27;
const int DISPLAY_COLS                  = 20;
const int DISPLAY_ROWS                  = 4;

const int boilerPowerDelay              = 500;
const int energyStorageWattageBuffer    = 0;

// Managers
ModbusManager   modbusManager(RX_PIN, TX_PIN, RS485_CONTROL_PIN, MODBUS_SLAVE_ID);
LoadBalancer    loadBalancer(SG_PIN, LED_SG_PIN);
EnergyStorage   energyStorage(BOILER_PIN, LED_BOILER_PIN, energyStorageWattageBuffer);
DisplayManager  display(DISPLAY_ADDR, DISPLAY_COLS, DISPLAY_ROWS);
Scheduler       scheduler;

// Timing constants in milliseconds
const unsigned long MODBUS_READ_INTERVAL            = 50;
const unsigned long LOAD_BALANCING_INTERVAL         = 500;
const unsigned long ENERGY_EXPORT_INTERVAL          = 10;
const unsigned long ENERGY_UPDATE_VALUES_INTERVAL   = 1000;
const unsigned long DISPLAY_INTERVAL                = 500;
const unsigned long SERIAL_INTERVAL                 = 200;

void modbusPollTask() {
    modbusManager.readAllRegs();
}

void energyStorageModulateTask() {
    energyStorage.modulateBoiler();
}

void displayTask() {
    display.printStatus(
        energyStorage.isBoilerOn(),
        modbusManager.getTotalCurrent(),
        energyStorage.getEnergyExport(),
        energyStorage.getEnergyStored(),
        energyStorage.getPercentage()
    );
}

void serialTask() {
    Serial.print("Total current: ");
    Serial.print(modbusManager.getTotalCurrent());
    Serial.print(" A, Export: ");
    Serial.print(energyStorage.getEnergyExport());
    Serial.print(" W, Stored: ");
    Serial.print(energyStorage.getEnergyStored());
    Serial.print(" W, Percentage: ");
    Serial.print(energyStorage.getPercentage());
    Serial.println(" %");
}

// Function to measure boiler total power
int measureBoilerPower() {
    int measurements[3] = {0, 0, 0};

    delay(boilerPowerDelay);

    for (int i = 0; i < 3; i++) {
        int baseline = modbusManager.readReg(CURRENT_ADDR);
        digitalWrite(BOILER_PIN, HIGH);
        digitalWrite(LED_BOILER_PIN, HIGH);
        delay(boilerPowerDelay);
        
        int newCurrent = modbusManager.readReg(CURRENT_ADDR);
        digitalWrite(BOILER_PIN, LOW);
        digitalWrite(LED_BOILER_PIN, LOW);
        delay(boilerPowerDelay);
        
        measurements[i] = newCurrent - baseline;
    }

    Serial.print("Boiler Power Measurements: ");
    Serial.print(measurements[0]);
    Serial.print(", ");
    Serial.print(measurements[1]);
    Serial.print(", ");
    Serial.println(measurements[2]);

    int average = (measurements[0] + measurements[1] + measurements[2]) / 3;
    int rounded = ((average + 25) / 50) * 50; // Round to nearest 50

    Serial.print("Average number: ");
    Serial.print(average);
    Serial.print(" W, Rounded number: ");
    Serial.print(rounded);
    Serial.println(" W");

    return rounded;
}

void setup() {
    Serial.begin(9600);
    Serial.println("Initialized serial");

    // Initialize pins
    pinMode(BOILER_PIN, OUTPUT);
    pinMode(LED_BOILER_PIN, OUTPUT);

    digitalWrite(BOILER_PIN, LOW);
    digitalWrite(LED_BOILER_PIN, LOW);
    
    // Initialize Modbus manager
    modbusManager.init();
    Serial.println("Initialized modbus");
    
    // Initialize load balancer
    loadBalancer.init();
    Serial.println("Initialized load balancer");

    // Initialize energy storage
    energyStorage.init(measureBoilerPower());
    Serial.println("Initialized energy storage");

    // Initialize display
    display.init();
    Serial.println("Initialized display");

    // Subscribe to Modbus updates
    modbusManager.addListener(&loadBalancer);
    modbusManager.addListener(&energyStorage);

    // Register scheduled tasks
    scheduler.addTask(modbusPollTask, MODBUS_READ_INTERVAL, true);
    scheduler.addTask(energyStorageModulateTask, ENERGY_EXPORT_INTERVAL, true);
    scheduler.addTask(displayTask, DISPLAY_INTERVAL, true);
    scheduler.addTask(serialTask, SERIAL_INTERVAL, true);
}

void loop() {
    modbusManager.update();
    scheduler.run();
}

