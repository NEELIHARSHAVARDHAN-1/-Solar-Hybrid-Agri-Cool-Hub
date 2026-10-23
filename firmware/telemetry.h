/**
 * @file        telemetry.h
 * @brief       AgriCoolHub telemetry data model — strict 10-field contract.
 * @project     AgriCoolHub Solar Hybrid Agri Cool Hub
 * @platform    ESP32-S3 (Arduino Framework)
 *
 * @section JSONContract
 * Every MQTT publish must serialize exactly these ten fields:
 *   device_id, timestamp, temperature, humidity,
 *   mq3, mq135, mq4, latitude, longitude, battery
 *
 * Rules:
 *  - Use NaN sentinel (isnan()) to signal "no valid reading".
 *  - json_builder serializes NaN fields as JSON null.
 *  - Do NOT write zero when a sensor is absent or failed.
 *  - mq3/mq135/mq4 are normalized ADC output percentages (0–100),
 *    NOT calibrated gas concentrations or safety-certified measurements.
 *  - battery reports null unless a defensible estimate is available.
 */

#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <Arduino.h>

// =====================================================
// AgriCoolHub Telemetry Data Model
// =====================================================

struct TelemetryData
{
    // --- Identity & Time ---
    String device_id;   // stable device identifier
    String timestamp;   // ISO 8601 UTC string, or "" when unavailable

    // --- Environmental (SHT4x) ---
    float temperature;  // degrees Celsius, or NaN
    float humidity;     // relative humidity %, or NaN

    // --- MQ Analog Sensors (normalized %, NOT ppm / concentration) ---
    float mq3;          // MQ-3  normalized output %, or NaN
    float mq135;        // MQ-135 normalized output %, or NaN
    float mq4;          // MQ-4  normalized output %, or NaN

    // --- GPS ---
    float latitude;     // decimal degrees, or NaN when fix unavailable
    float longitude;    // decimal degrees, or NaN when fix unavailable

    // --- Power (INA219) ---
    // battery: null unless a validated estimate is available.
    // INA219 measures electrical quantities; it does NOT directly measure
    // state-of-charge. Populate only if a documented conversion model is used.
    float battery;      // estimated charge %, or NaN

    // --- Network diagnostics (not published in 10-field contract) ---
    bool wifi_status;
    int  wifi_rssi_dbm;
};

// =====================================================
// Global Instance
// =====================================================

extern TelemetryData telemetry;

#endif // TELEMETRY_H
