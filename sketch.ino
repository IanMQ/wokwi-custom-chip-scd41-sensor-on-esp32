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

// Crear una clase derivada para acceder a métodos protected
class MySCD41 : public DFRobot_SCD4X {
public:
  MySCD41(TwoWire *pWire = &Wire, uint8_t i2cAddr = 0x62) 
    : DFRobot_SCD4X(pWire, i2cAddr) {}
  
  // Exponer métodos protected como públicos
  bool getSerialNumber(uint16_t *wordBuf) {
    return DFRobot_SCD4X::getSerialNumber(wordBuf);
  }
  
  int16_t performForcedRecalibration(uint16_t CO2ppm) {
    return DFRobot_SCD4X::performForcedRecalibration(CO2ppm);
  }
};

MySCD41 scd41(&Wire, 0x62);

// Variables para control de tiempo
unsigned long lastReadTime = 0;
const unsigned long readInterval = 2000; // Leer cada 2 segundos

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=================================");
  Serial.println("SCD41 Sensor Test with ESP32");
  Serial.println("=================================");
  
  Wire.begin(21, 22); // SDA=GPIO21, SCL=GPIO22
  Wire.setClock(100000); // 100kHz I2C clock
  
  Serial.println("Inicializando sensor SCD41...");
  
  if (!scd41.begin()) {
    Serial.println("Error: No se pudo inicializar el sensor SCD41");
    Serial.println("Verifique las conexiones I2C");
    while (1) {
      delay(1000);
      Serial.println("Reintentando...");
      if (scd41.begin()) break;
    }
  }
  
  Serial.println("Sensor SCD41 inicializado correctamente!");
  
  // Obtener y mostrar el número de serie (ahora es accesible)
  uint16_t serialNumber[3];
  if (scd41.getSerialNumber(serialNumber)) {
    Serial.print("Número de serie: ");
    Serial.print(serialNumber[0], HEX);
    Serial.print(" ");
    Serial.print(serialNumber[1], HEX);
    Serial.print(" ");
    Serial.println(serialNumber[2], HEX);
  } else {
    Serial.println("Error al leer número de serie");
  }
  
  // Configurar el sensor
  scd41.setAutoCalibMode(true);
  Serial.print("Modo calibración automática: ");
  Serial.println(scd41.getAutoCalibMode() ? "Activado" : "Desactivado");
  
  // Iniciar mediciones periódicas
  scd41.enablePeriodMeasure(SCD4X_START_PERIODIC_MEASURE);
  Serial.println("Mediciones periódicas iniciadas (cada 5 segundos)");
  
  Serial.println("\nEsperando primeras lecturas...\n");
  delay(6000); // Esperar la primera medición
}

void loop() {
  // Verificar si hay datos disponibles
  if (scd41.getDataReadyStatus()) {
    DFRobot_SCD4X::sSensorMeasurement_t measurement;
    scd41.readMeasurement(&measurement);
    
    // Mostrar resultados
    Serial.println("=================================");
    Serial.print("CO2: ");
    Serial.print(measurement.CO2ppm);
    Serial.println(" ppm");
    
    Serial.print("Temperatura: ");
    Serial.print(measurement.temp, 1);
    Serial.println(" °C");
    
    Serial.print("Humedad: ");
    Serial.print(measurement.humidity, 1);
    Serial.println(" %");
    Serial.println("=================================\n");
    
    delay(readInterval);
  } else {
    // Esperar a que haya datos
    delay(100);
  }
}