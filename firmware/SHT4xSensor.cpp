/**
 * @file        SHT4xSensor.cpp
 * @brief       SHT4x temperature and humidity sensor implementation.
 * @project     AgriCoolHub Solar Hybrid Agri Cool Hub
 * @platform    ESP32-S3 (Arduino Framework)
 *
 * @section Library
 * Adafruit SHT4x Library — install via Arduino Library Manager.
 * Board package: esp32 by Espressif Systems 3.x
 */

#include <Wire.h>
#include <Adafruit_SHT4x.h>
#include <math.h>

#include "SHT4xSensor.h"
#include "telemetry.h"
#include "sd_manager.h"

// =====================================================
// Internal State
// =====================================================

static Adafruit_SHT4x sht4x;
static bool sht4xAvailable = false;

// =====================================================
// Initialization
// =====================================================

/**
 * @brief  Probes the I2C bus for the SHT4x and configures precision / heater.
 */
bool initSHT4x()
{
    if (!sht4x.begin())
    {
        Serial.println("[SHT4x] NOT DETECTED — check I2C wiring (SDA=8, SCL=9)");
        logFault("SHT4X_INIT_FAIL");
        sht4xAvailable = false;
        return false;
    }

    // High-precision measurement, no heater
    sht4x.setPrecision(SHT4X_HIGH_PRECISION);
    sht4x.setHeater(SHT4X_NO_HEATER);

    sht4xAvailable = true;
    Serial.println("[SHT4x] Initialized — High Precision, No Heater");
    return true;
}

// =====================================================
// Read
// =====================================================

/**
 * @brief  Acquires a temperature + humidity sample.
 *         Writes NaN on any failure so json_builder emits null rather than 0.
 */
void readSHT4x()
{
    if (!sht4xAvailable)
    {
        telemetry.temperature = NAN;
        telemetry.humidity    = NAN;
        return;
    }

    sensors_event_t hum, temp;
    bool ok = sht4x.getEvent(&hum, &temp);

    if (!ok)
    {
        Serial.println("[SHT4x] Read failed");
        telemetry.temperature = NAN;
        telemetry.humidity    = NAN;
        return;
    }

    float t = temp.temperature;
    float h = hum.relative_humidity;

    // Basic sanity bounds (SHT4x datasheet: -40 to 125 °C, 0–100 %RH)
    if (t < -40.0f || t > 125.0f)
    {
        Serial.println("[SHT4x] Temperature out of range");
        telemetry.temperature = NAN;
    }
    else
    {
        telemetry.temperature = t;
    }

    if (h < 0.0f || h > 100.0f)
    {
        Serial.println("[SHT4x] Humidity out of range");
        telemetry.humidity = NAN;
    }
    else
    {
        telemetry.humidity = h;
    }
}
