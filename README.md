# ZegarDuzyV25

ZegarDuzyV25 is a firmware project for the STM32F401 Blackpill microcontroller. The project implements several functionalities including menu handling, SHT30 sensor integration, GPS signal reception with clock synchronization, and support for a rotary encoder with an integrated button.

## Features
- **Menu Handling:** Interactive menu for navigating system options.
- **SHT30 Sensor Integration:** Reads environmental data.
- **GPS Signal Reception & Clock Synchronization:** Automatically synchronizes the clock when a GPS signal is available.
- **Encoder with Button Support:** Supports user input via a rotary encoder with a built-in button.
- **SCT2004 Drivers:** Includes drivers for SCT2004 to extend hardware functionalities.

## Hardware Requirements
- **Main Board:** STM32F401 Blackpill
- **Sensors & Modules:**
  - SHT30 sensor
  - GPS module
  - SCT2004 drivers
  - Rotary encoder with integrated button
 
 ## Usage
  - Menu Navigation: Use the rotary encoder to scroll through menu options.
  - Sensor Data: Monitor environmental data from the SHT30 sensor.
  - GPS Sync: The system will synchronize the clock automatically upon receiving a valid GPS signal.
