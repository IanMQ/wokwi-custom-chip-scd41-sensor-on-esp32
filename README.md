# wokwi-custom-chip-scd41-sensor-on-esp32

[![Wokwi](https://img.shields.io/badge/Wokwi-Simulate-blue)](https://wokwi.com/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-red)](https://www.espressif.com/en/products/socs/esp32)

<div align="center">

![SCD41 Sensor](https://github.com/DFRobot/DFRobot_SCD4X/raw/main/resources/images/SCD41.png)

# 🌿 SCD41 CO₂, Temperature & Humidity Sensor Simulator

**A complete Wokwi custom chip implementation of the Sensirion SCD41 sensor for ESP32**

[![Run on Wokwi](https://img.shields.io/badge/Demo-wokwi.com-blue)](https://wokwi.com/projects/464075553530924033)

</div>

## Summary

IoT Embedded Application illustrating the creation and use of a custom chip in Wokwi. The chip is a **Sensirion SCD41 CO₂, Temperature and Humidity Sensor**. The C++ Sketch Application reads values from the sensor and displays information about:

- 🌫️ **CO₂ concentration** (ppm)
- 🌡️ **Temperature** (°C)
- 💧 **Relative Humidity** (%)

The custom chip simulates the complete I2C communication protocol of the SCD41, including CRC checksum validation, periodic measurements, single-shot mode, and configuration registers. The simulation includes **interactive controls** that allow users to adjust sensor readings in real-time.

## Table of Contents

- [Components](#components)
- [Libraries](#libraries)
- [Features](#features)
- [Controls](#controls)
- [Pin Connections](#pin-connections)
- [Additional References](#additional-references)
- [Credits](#credits)

## Components

- **ESP32 DevKit V4** - Main microcontroller
- **SCD41 CO2/Temperature/Humidity Sensor** - Custom chip implementation

## Libraries

- **DFRobot_SCD4X** - Official SCD4X library for sensor communication
- **Wire** - I2C communication protocol
- **stdio** - Standard input/output
- **stdlib** - Standard library functions
- **string** - String manipulation
- **wokwi-api** - Wokwi custom chip API

## Features

- **Complete I2C protocol simulation** including address 0x62
- **CRC8 checksum validation** for all data transfers
- **Interactive controls** for CO2, Temperature, and Humidity
- **Periodic measurement mode** (5-second intervals)
- **Single-shot measurement mode**
- **Low power measurement mode** (30-second intervals)
- **Configuration registers** (temperature offset, altitude, pressure)
- **Automatic self-calibration (ASC)** support
- **Forced recalibration (FRC)** capability
- **Factory reset functionality**
- **Serial number reading** (simulated unique ID: BE02-7F07-3BFB)
- **Sleep/Wake modes** for power saving
- **Data ready status checking**

## Controls

The custom chip includes three interactive sliders in the Wokwi UI:

| Control | Range | Default | Step | Description |
|---------|-------|---------|------|-------------|
| CO2 (ppm) | 0 - 40000 | 420 | 10 | Carbon dioxide concentration |
| Temperature (°C) | -10 - 60 | 25 | 0.1 | Ambient temperature |
| Humidity (%) | 0 - 100 | 50 | 1 | Relative humidity |

## Pin Connections

| ESP32 Pin | SCD41 Pin | Description |
|-----------|-----------|-------------|
| 3V3 | VCC | Power supply (3.3V) |
| GND | GND | Ground |
| GPIO21 | SDA | I2C Data line |
| GPIO22 | SCL | I2C Clock line |

## Additional References

### Technical Documentation
- Sensirion SCD41 Datasheet: https://sensirion.com/products/catalog/SCD41
- DFRobot_SCD4X Library: https://github.com/DFRobot/DFRobot_SCD4X

### Protocol References
- I2C Communication Reference: https://i2c.info
- Wokwi Custom Chips API: https://docs.wokwi.com/chips-api/getting-started

### Tutorials
- SCD41 Getting Started: https://sensirion.com/products/co2-sensors/scd4x/getting-started
- ESP32 I2C Tutorial: https://randomnerdtutorials.com/esp32-i2c-communication-arduino/
- Wokwi Custom Chip Examples: https://docs.wokwi.com/chips-api/example

## Credits

| Role | Contributor | Description |
|------|-------------|-------------|
| **Base Library** | DFRobot Co.Ltd | Original DFRobot_SCD4X library implementation |
| **Concept & Wokwi Adaptation** | IanMQ | Idea to port SCD41 to Wokwi custom chip |
| **Code Assistance & Debugging** | DeepSeek | AI assistance for CRC, I2C implementation |
| **Platform** | Uri Shaked | Wokwi simulation platform |

