# Class Diagram for `Systeemontwerp_WZS`

This project has the following core classes:

- `ModbusManager`
- `IModbusListener`
- `ModbusData`
- `LoadBalancer`
- `EnergyStorage`
- `DisplayManager`
- `Scheduler`
- `SchedulerTask`

## Key relationships

- `ModbusManager` manages Modbus I/O and notifies listeners.
- `IModbusListener` is an interface implemented by `LoadBalancer` and `EnergyStorage`.
- `LoadBalancer` and `EnergyStorage` both receive `ModbusData` updates.
- `Scheduler` keeps periodic tasks running, as configured from `main.cpp`.
- `DisplayManager` presents current values on the LCD, driven from `main.cpp`.

## Diagram file

The PlantUML source diagram is in `CLASS_DIAGRAM.puml`. You can render it with any PlantUML tool.

## Notes

- `main.cpp` creates and wires the objects together.
- `ModbusManager` also provides accessor methods used by `main.cpp` and the display task.
- `Scheduler` schedules free functions rather than class methods.
