#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Comandos SCD4X
#define SCD4X_START_PERIODIC_MEASURE    0x21b1
#define SCD4X_READ_MEASUREMENT          0xec05
#define SCD4X_STOP_PERIODIC_MEASURE     0x3f86
#define SCD4X_GET_SERIAL_NUMBER         0x3682
#define SCD4X_GET_DATA_READY_STATUS     0xe4b8
#define SCD4X_SET_TEMPERATURE_OFFSET    0x241d
#define SCD4X_GET_TEMPERATURE_OFFSET    0x2318
#define SCD4X_SET_SENSOR_ALTITUDE       0x2427
#define SCD4X_GET_SENSOR_ALTITUDE       0x2322
#define SCD4X_SET_AMBIENT_PRESSURE      0xe000
#define SCD4X_PERFORM_FORCED_RECALIB    0x362f
#define SCD4X_SET_AUTOMATIC_CALIB       0x2416
#define SCD4X_GET_AUTOMATIC_CALIB       0x2313
#define SCD4X_PERSIST_SETTINGS          0x3615
#define SCD4X_PERFORM_SELF_TEST         0x3639
#define SCD4X_PERFORM_FACTORY_RESET     0x3632
#define SCD4X_REINIT                    0x3646
#define SCD4X_MEASURE_SINGLE_SHOT       0x219d
#define SCD4X_MEASURE_SINGLE_SHOT_RHT_ONLY 0x2196
#define SCD4X_POWER_DOWN                0x36e0
#define SCD4X_WAKE_UP                   0x36f6
#define SCD4X_START_LOW_POWER_MEASURE   0x21ac

// Dirección I2C del SCD41
#define SCD41_I2C_ADDR 0x62

// Valores simulados
#define DEFAULT_CO2 420.0
#define DEFAULT_TEMP 25.0
#define DEFAULT_HUMIDITY 50.0

// CRC calculation
#define CRC8_POLYNOMIAL 0x31
#define CRC8_INIT 0xFF

typedef struct {
  float co2;
  float temp;
  float humidity;
  uint16_t serial_number[3];
  uint16_t temperature_offset;
  uint16_t sensor_altitude;
  uint32_t ambient_pressure;
  bool auto_calib;
  bool periodic_measurement;
  bool low_power_mode;
  bool sleeping;
  uint16_t last_cmd;
  uint8_t cmd_buffer[16];
  uint8_t cmd_len;
  bool reading_cmd;
  timer_t measurement_timer;
  pin_t sda_pin;
  pin_t scl_pin;
  // Atributos para los controles (como en LM75A)
  uint32_t co2_attr;
  uint32_t temp_attr;
  uint32_t humidity_attr;
} chip_state_t;

// Función para calcular CRC8
static uint8_t calc_crc(uint16_t data) {
  uint8_t crc = CRC8_INIT;
  uint8_t buf[2] = {(data >> 8) & 0xFF, data & 0xFF};
  
  for (int i = 0; i < 2; i++) {
    crc ^= buf[i];
    for (int bit = 0; bit < 8; bit++) {
      if (crc & 0x80)
        crc = (crc << 1) ^ CRC8_POLYNOMIAL;
      else
        crc = (crc << 1);
    }
  }
  return crc;
}

// Función para empaquetar datos con CRC
static void pack_data(uint16_t data, uint8_t *buf) {
  buf[0] = (data >> 8) & 0xFF;
  buf[1] = data & 0xFF;
  buf[2] = calc_crc(data);
}

// Actualizar lecturas desde los controles
static void update_measurements(chip_state_t *chip) {
  // Leer valores actuales de los atributos
  chip->co2 = attr_read_float(chip->co2_attr);
  chip->temp = attr_read_float(chip->temp_attr);
  chip->humidity = attr_read_float(chip->humidity_attr);
  
  printf("SCD41: Medición - CO2: %.0f ppm, Temp: %.1f°C, Hum: %.1f%%\n", 
         chip->co2, chip->temp, chip->humidity);
}

static void on_measurement_timer(void *user_data) {
  chip_state_t *chip = (chip_state_t*)user_data;
  if (chip->periodic_measurement) {
    update_measurements(chip);
  }
}

// Callback cuando el chip es direccionado en el bus I2C
static bool on_i2c_connect(void *user_data, uint32_t address, bool read) {
  chip_state_t *chip = (chip_state_t*)user_data;
  
  if (!read) {
    chip->cmd_len = 0;
    chip->reading_cmd = true;
  }
  return true;
}

// Callback cuando el microcontrolador escribe un byte
static bool on_i2c_write(void *user_data, uint8_t data) {
  chip_state_t *chip = (chip_state_t*)user_data;
  
  if (chip->cmd_len < sizeof(chip->cmd_buffer)) {
    chip->cmd_buffer[chip->cmd_len++] = data;
  }
  
  if (chip->cmd_len >= 2) {
    uint16_t cmd = (chip->cmd_buffer[0] << 8) | chip->cmd_buffer[1];
    chip->last_cmd = cmd;
    
    if (chip->cmd_len == 2) {
      switch (cmd) {
        case SCD4X_STOP_PERIODIC_MEASURE:
          chip->periodic_measurement = false;
          chip->low_power_mode = false;
          printf("SCD41: Medición periódica detenida\n");
          break;
          
        case SCD4X_START_PERIODIC_MEASURE:
          chip->periodic_measurement = true;
          chip->low_power_mode = false;
          update_measurements(chip);
          printf("SCD41: Iniciando medición periódica\n");
          break;
          
        case SCD4X_START_LOW_POWER_MEASURE:
          chip->periodic_measurement = true;
          chip->low_power_mode = true;
          update_measurements(chip);
          printf("SCD41: Iniciando medición de baja potencia\n");
          break;
          
        case SCD4X_POWER_DOWN:
          chip->sleeping = true;
          printf("SCD41: Entrando en modo sleep\n");
          break;
          
        case SCD4X_WAKE_UP:
          chip->sleeping = false;
          printf("SCD41: Despertando\n");
          break;
          
        case SCD4X_PERFORM_SELF_TEST:
          printf("SCD41: Realizando autodiagnóstico\n");
          break;
          
        case SCD4X_REINIT:
          printf("SCD41: Reinicializando\n");
          break;
          
        case SCD4X_PERFORM_FACTORY_RESET:
          chip->temperature_offset = 0;
          chip->sensor_altitude = 0;
          chip->auto_calib = true;
          printf("SCD41: Reset de fábrica completado\n");
          break;
          
        case SCD4X_PERSIST_SETTINGS:
          printf("SCD41: Configuración guardada en EEPROM (simulado)\n");
          break;
          
        case SCD4X_MEASURE_SINGLE_SHOT:
          update_measurements(chip);
          printf("SCD41: Medición única completada\n");
          break;
          
        case SCD4X_MEASURE_SINGLE_SHOT_RHT_ONLY:
          update_measurements(chip);
          printf("SCD41: Medición única (solo RH/Temp) completada\n");
          break;
      }
    }
    else if (chip->cmd_len >= 5) {
      uint16_t data_val = (chip->cmd_buffer[2] << 8) | chip->cmd_buffer[3];
      uint8_t crc = chip->cmd_buffer[4];
      
      if (crc == calc_crc(data_val)) {
        switch (cmd) {
          case SCD4X_SET_TEMPERATURE_OFFSET:
            chip->temperature_offset = data_val;
            printf("SCD41: Offset temperatura establecido: %u\n", data_val);
            break;
            
          case SCD4X_SET_SENSOR_ALTITUDE:
            chip->sensor_altitude = data_val;
            printf("SCD41: Altitud establecida: %u m\n", data_val);
            break;
            
          case SCD4X_SET_AMBIENT_PRESSURE:
            chip->ambient_pressure = data_val * 100;
            printf("SCD41: Presión ambiental establecida: %u Pa\n", chip->ambient_pressure);
            break;
            
          case SCD4X_PERFORM_FORCED_RECALIB:
            printf("SCD41: Calibración forzada con %u ppm\n", data_val);
            break;
            
          case SCD4X_SET_AUTOMATIC_CALIB:
            chip->auto_calib = (data_val == 1);
            printf("SCD41: Calibración automática: %s\n", chip->auto_calib ? "ON" : "OFF");
            break;
        }
      } else {
        printf("SCD41: Error CRC en comando 0x%04X\n", cmd);
      }
      chip->cmd_len = 0;
    }
  }
  
  return true;
}

// Callback cuando el microcontrolador quiere leer un byte
static uint8_t on_i2c_read(void *user_data) {
  chip_state_t *chip = (chip_state_t*)user_data;
  static uint8_t response[9];
  static int response_index = 0;
  static int response_len = 0;
  static bool first_call = true;
  
  if (first_call) {
    response_index = 0;
    first_call = false;
    
    memset(response, 0, sizeof(response));
    
    switch (chip->last_cmd) {
      case SCD4X_READ_MEASUREMENT: {
        uint16_t co2_value = (uint16_t)chip->co2;
        uint16_t temp_value = (uint16_t)((chip->temp + 45.0) * ((uint32_t)1 << 16) / 175.0);
        uint16_t hum_value = (uint16_t)(chip->humidity * ((uint32_t)1 << 16) / 100.0);
        
        response[0] = (co2_value >> 8) & 0xFF;
        response[1] = co2_value & 0xFF;
        response[2] = calc_crc(co2_value);
        
        response[3] = (temp_value >> 8) & 0xFF;
        response[4] = temp_value & 0xFF;
        response[5] = calc_crc(temp_value);
        
        response[6] = (hum_value >> 8) & 0xFF;
        response[7] = hum_value & 0xFF;
        response[8] = calc_crc(hum_value);
        
        response_len = 9;
        break;
      }
      
      case SCD4X_GET_SERIAL_NUMBER: {
        pack_data(chip->serial_number[0], &response[0]);
        pack_data(chip->serial_number[1], &response[3]);
        pack_data(chip->serial_number[2], &response[6]);
        response_len = 9;
        break;
      }
      
      case SCD4X_GET_DATA_READY_STATUS: {
        uint16_t status = chip->periodic_measurement ? 0x0001 : 0x0000;
        pack_data(status, response);
        response_len = 3;
        break;
      }
      
      case SCD4X_GET_TEMPERATURE_OFFSET: {
        pack_data(chip->temperature_offset, response);
        response_len = 3;
        break;
      }
      
      case SCD4X_GET_SENSOR_ALTITUDE: {
        pack_data(chip->sensor_altitude, response);
        response_len = 3;
        break;
      }
      
      case SCD4X_GET_AUTOMATIC_CALIB: {
        pack_data(chip->auto_calib ? 0x0001 : 0x0000, response);
        response_len = 3;
        break;
      }
      
      case SCD4X_PERFORM_SELF_TEST: {
        pack_data(0x0000, response);
        response_len = 3;
        break;
      }
      
      default:
        response_len = 0;
        break;
    }
  }
  
  uint8_t byte_to_send = 0;
  if (response_index < response_len) {
    byte_to_send = response[response_index++];
  }
  
  if (response_index >= response_len) {
    first_call = true;
    chip->cmd_len = 0;
  }
  
  return byte_to_send;
}

// Callback cuando se desconecta
static void on_i2c_disconnect(void *user_data) {
  chip_state_t *chip = (chip_state_t*)user_data;
  chip->reading_cmd = false;
}

void chip_init() {
  chip_state_t *chip = calloc(1, sizeof(chip_state_t));
  
  // Inicializar valores por defecto
  chip->serial_number[0] = 0xbe02;
  chip->serial_number[1] = 0x7f07;
  chip->serial_number[2] = 0x3bfb;
  chip->temperature_offset = 0;
  chip->sensor_altitude = 0;
  chip->ambient_pressure = 101325;
  chip->auto_calib = true;
  chip->periodic_measurement = false;
  chip->low_power_mode = false;
  chip->sleeping = false;
  chip->last_cmd = 0;
  chip->cmd_len = 0;
  chip->reading_cmd = false;
  
  // Inicializar atributos (controles) - como en LM75A
  chip->co2_attr = attr_init_float("co2", DEFAULT_CO2);
  chip->temp_attr = attr_init_float("temperature", DEFAULT_TEMP);
  chip->humidity_attr = attr_init_float("humidity", DEFAULT_HUMIDITY);
  
  // Leer valores iniciales
  chip->co2 = attr_read_float(chip->co2_attr);
  chip->temp = attr_read_float(chip->temp_attr);
  chip->humidity = attr_read_float(chip->humidity_attr);
  
  // Configurar pines I2C
  chip->sda_pin = pin_init("SDA", INPUT_PULLUP);
  chip->scl_pin = pin_init("SCL", INPUT_PULLUP);
  
  // Configurar I2C
  const i2c_config_t i2c_config = {
    .address = SCD41_I2C_ADDR,
    .sda = chip->sda_pin,
    .scl = chip->scl_pin,
    .connect = on_i2c_connect,
    .read = on_i2c_read,
    .write = on_i2c_write,
    .disconnect = on_i2c_disconnect,
    .user_data = chip,
  };
  
  i2c_init((i2c_config_t*)&i2c_config);
  
  // Crear timer para mediciones periódicas (cada 5 segundos)
  const timer_config_t timer_config = {
    .callback = on_measurement_timer,
    .user_data = chip
  };
  chip->measurement_timer = timer_init(&timer_config);
  timer_start(chip->measurement_timer, 5000000, true);
  
  printf("SCD41: Chip simulator initialized at I2C address 0x%02X\n", SCD41_I2C_ADDR);
  printf("SCD41: Serial number: %04X %04X %04X\n", 
         chip->serial_number[0], chip->serial_number[1], chip->serial_number[2]);
  printf("SCD41: Controles disponibles - CO2, Temperatura, Humedad\n");
}