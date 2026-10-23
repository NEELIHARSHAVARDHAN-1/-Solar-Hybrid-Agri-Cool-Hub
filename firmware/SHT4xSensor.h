/**
 * @file        SHT4xSensor.h
 * @brief       SHT4x temperature and humidity sensor interface.
 * @project     AgriCoolHub Solar Hybrid Agri Cool Hub
 * @platform    ESP32-S3 (Arduino Framework)
 *
 * @section Hardware
 * SHT4x communicates over I2C (default address 0x44).
 * Shares the bus on GPIO8 (SDA) / GPIO9 (SCL) with INA219 and DS3231.
 *
 * @section Library
 * Requires: Adafruit SHT4x Library (install via Arduino Library Manager)
 * Depends on: Adafruit BusIO
 */

#ifndef SHT4XSENSOR_H
#define SHT4XSENSOR_H

#include <Arduino.h>

// =====================================================
// Public API
// =====================================================

/**
 * @brief  Initializes the SHT4x sensor over I2C.
 *         Prints a fault message and returns false if sensor is not detected.
 * @return true if sensor initialized successfully, false otherwise.
 */
bool initSHT4x();

/**
 * @brief  Reads temperature and humidity from the SHT4x sensor.
 *         Writes results to telemetry.temperature and telemetry.humidity.
 *         On failure, writes NaN to both fields (never zero).
 */
void readSHT4x();

#endif // SHT4XSENSOR_H
