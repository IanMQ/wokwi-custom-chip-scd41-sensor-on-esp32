// Wokwi Custom Chip - Sensirion SCD41 CO2, Temperature and Humidity Sensor
// 
// Based on DFRobot_SCD4X library: https://github.com/DFRobot/DFRobot_SCD4X
// 
// Chip code adapted from: DFRobot_SCD4X by DFRobot Co.Ltd
// Concept and Wokwi adaptation: IanMQ
// Code assistance and debugging: DeepSeek
// 
// SPDX-License-Identifier: MIT
// Copyright (C) 2026 Uri Shaked / wokwi.com

#include <Wire.h>
#include "DFRobot_SCD4X.h"

// Create a derived class to access protected methods
class MySCD41 : public DFRobot_SCD4X {
public:
  MySCD41(TwoWire *pWire = &Wire, uint8_t i2cAddr = 0x62) 
    : DFRobot_SCD4X(pWire, i2cAddr) {}
  
  // Expose protected methods as public
  bool getSerialNumber(uint16_t *wordBuf) {
    return DFRobot_SCD4X::getSerialNumber(wordBuf);
  }
  
  int16_t performForcedRecalibration(uint16_t CO2ppm) {
    return DFRobot_SCD4X::performForcedRecalibration(CO2ppm);
  }
};

MySCD41 scd41(&Wire, 0x62);

// Timing variables
unsigned long lastReadTime = 0;
const unsigned long readInterval = 2000; // Read every 2 seconds

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=================================");
  Serial.println("SCD41 Sensor Test with ESP32");
  Serial.println("=================================");
  
  Wire.begin(21, 22); // SDA=GPIO21, SCL=GPIO22
  Wire.setClock(100000); // 100kHz I2C clock
  
  Serial.println("Initializing SCD41 sensor...");
  
  if (!scd41.begin()) {
    Serial.println("Error: Failed to initialize SCD41 sensor");
    Serial.println("Check I2C connections");
    while (1) {
      delay(1000);
      Serial.println("Retrying...");
      if (scd41.begin()) break;
    }
  }
  
  Serial.println("SCD41 sensor initialized successfully!");
  
  // Get and display serial number (now accessible)
  uint16_t serialNumber[3];
  if (scd41.getSerialNumber(serialNumber)) {
    Serial.print("Serial number: ");
    Serial.print(serialNumber[0], HEX);
    Serial.print(" ");
    Serial.print(serialNumber[1], HEX);
    Serial.print(" ");
    Serial.println(serialNumber[2], HEX);
  } else {
    Serial.println("Error reading serial number");
  }
  
  // Configure the sensor
  scd41.setAutoCalibMode(true);
  Serial.print("Auto calibration mode: ");
  Serial.println(scd41.getAutoCalibMode() ? "Enabled" : "Disabled");
  
  // Start periodic measurements
  scd41.enablePeriodMeasure(SCD4X_START_PERIODIC_MEASURE);
  Serial.println("Periodic measurements started (every 5 seconds)");
  
  Serial.println("\nWaiting for first readings...\n");
  delay(6000); // Wait for first measurement
}

void loop() {
  // Check if data is available
  if (scd41.getDataReadyStatus()) {
    DFRobot_SCD4X::sSensorMeasurement_t measurement;
    scd41.readMeasurement(&measurement);
    
    // Display results
    Serial.println("=================================");
    Serial.print("CO2: ");
    Serial.print(measurement.CO2ppm);
    Serial.println(" ppm");
    
    Serial.print("Temperature: ");
    Serial.print(measurement.temp, 1);
    Serial.println(" °C");
    
    Serial.print("Humidity: ");
    Serial.print(measurement.humidity, 1);
    Serial.println(" %");
    Serial.println("=================================\n");
    
    delay(readInterval);
  } else {
    // Wait for data to be ready
    delay(100);
  }
}