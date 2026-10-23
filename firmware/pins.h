/**
 * @file        pins.h
 * @brief       Hardware pin definitions mapping schematic lines to MCU registers.
 * @project     AgriCoolHub Solar Hybrid Agri Cool Hub
 * @platform    ESP32-S3 (Arduino Framework)
 *
 * @section Description
 * Master hardware pin definition file mapping the physical board schematic lines
 * directly to the internal GPIO registers of the ESP32-S3 SoC.
 *
 * @section PinConflictNotes
 * - GPIO4 is assigned to MQ-3 analog input. DS18B20 previously used GPIO4 and is
 *   excluded from this build; do not reconnect DS18B20 to GPIO4 while MQ-3 is present.
 * - GPIO8 and GPIO9 are the shared I2C bus (SHT4x, INA219, and any other I2C devices).
 * - GPIO1 (ACS712) is retained in-tree but excluded from the AgriCoolHub main loop.
 * - GPS UART uses UART1 on GPIO17 (RX) / GPIO18 (TX). Verify these are not used by
 *   other peripherals on your specific ESP32-S3 module before wiring.
 *
 * @warning     All ADC inputs (GPIO4, GPIO5, GPIO6) must remain within 0–3.3 V.
 *              Verify voltage-divider resistor values with a multimeter before powering on.
 */

#ifndef PINS_H
#define PINS_H

#include <Arduino.h>

// =====================================================
// I2C Bus (Shared: SHT4x, INA219, DS3231)
// =====================================================
#define I2C_SDA_PIN  8
#define I2C_SCL_PIN  9

// =====================================================
// SHT4x Temperature & Humidity — I2C (no extra pin)
// =====================================================
// SHT4x default I2C address: 0x44

// =====================================================
// INA219 Voltage / Current Monitor — I2C (no extra pin)
// =====================================================
// INA219 default I2C address: 0x40

// =====================================================
// MQ Analog Gas Sensors
// ASSUMPTION: Voltage divider (10 kΩ top / 20 kΩ bottom equivalent)
// reduces sensor AO output so V_ADC <= 3.3 V.
// VERIFY actual resistor values with a multimeter before use.
// =====================================================
#define MQ3_ANALOG_PIN   4   // MQ-3  (alcohol-sensitive)
#define MQ135_ANALOG_PIN 5   // MQ-135 (air quality)
#define MQ4_ANALOG_PIN   6   // MQ-4  (methane/natural gas)

// =====================================================
// GPS — UART1
// =====================================================
#define GPS_RX_PIN  17   // ESP32-S3 RX ← GPS TX
#define GPS_TX_PIN  18   // ESP32-S3 TX → GPS RX

// =====================================================
// Status Indicators (Onboard Smart RGB — NeoPixel)
// =====================================================
#define RGB_PIN      48

// =====================================================
// MicroSD SPI
// =====================================================
#define SD_CS_PIN    10
#define SD_MOSI_PIN  11
#define SD_SCK_PIN   12
#define SD_MISO_PIN  13

// =====================================================
// Legacy / Excluded Pins (kept for reference)
// =====================================================
// #define DS18B20_PIN  4   // Conflicts with MQ3_ANALOG_PIN — excluded
// #define ACS712_PIN   1   // ACS712 excluded from AgriCoolHub build
// #define MPU6050 shares I2C bus (GPIO8/9) — excluded from main loop

#endif // PINS_H
