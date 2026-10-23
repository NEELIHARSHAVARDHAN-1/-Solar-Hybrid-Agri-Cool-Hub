/**
 * @file        json_builder.cpp
 * @brief       Builds the strict 10-field AgriCoolHub MQTT telemetry payload.
 * @project     AgriCoolHub Solar Hybrid Agri Cool Hub
 * @platform    ESP32-S3 (Arduino Framework)
 *
 * @section JSONContract
 * Produces exactly ten keys in this order:
 *   device_id, timestamp, temperature, humidity,
 *   mq3, mq135, mq4, latitude, longitude, battery
 *
 * Rules enforced here:
 *  - NaN float fields → JSON null   (never 0, never a garbage number)
 *  - Empty timestamp string  → JSON null
 *  - MQ values labeled as percentage (0–100), NOT ppm or concentration
 *  - No string concatenation — ArduinoJson used exclusively
 *  - StaticJsonDocument sized for the fixed 10-field payload
 *
 * @section DocSize
 * Conservative sizing: 10 scalar fields × ~30 bytes + overhead = ~400 bytes.
 * 512 bytes provides comfortable margin.
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include <math.h>

#include "json_builder.h"
#include "telemetry.h"

// =====================================================
// Helper: serialize a float as number or null
// =====================================================

/**
 * @brief  Adds a float key to a JsonObject, writing null for NaN values.
 *
 * This helper prevents invalid JSON caused by NaN or Infinity.
 * It is used for every float field in the telemetry contract.
 */
static void addFloatOrNull(JsonObject& obj, const char* key, float value)
{
    if (isnan(value) || isinf(value))
    {
        obj[key].set(nullptr);
    }
    else
    {
        obj[key] = value;
    }
}

// =====================================================
// Telemetry JSON Builder
// =====================================================

/**
 * @brief  Serializes the global telemetry struct into the 10-field JSON contract.
 *
 * @return Minified JSON string. Example (GPS unavailable):
 * {
 *   "device_id":"agrihub-001",
 *   "timestamp":"2026-10-22T11:00:00Z",
 *   "temperature":25.6,
 *   "humidity":72.4,
 *   "mq3":10.0,
 *   "mq135":30.0,
 *   "mq4":28.0,
 *   "latitude":null,
 *   "longitude":null,
 *   "battery":null
 * }
 */
String buildTelemetryJSON()
{
    StaticJsonDocument<512> doc;

    // --- 1. device_id ---
    doc["device_id"] = telemetry.device_id;

    // --- 2. timestamp (null when GPS/NTP time is not available) ---
    if (telemetry.timestamp.length() > 0)
    {
        doc["timestamp"] = telemetry.timestamp;
    }
    else
    {
        doc["timestamp"].set(nullptr);
    }

    // --- 3-4. Environmental ---
    JsonObject root = doc.as<JsonObject>();
    addFloatOrNull(root, "temperature", telemetry.temperature);
    addFloatOrNull(root, "humidity",    telemetry.humidity);

    // --- 5-7. MQ normalized percentages (NOT gas concentrations) ---
    addFloatOrNull(root, "mq3",   telemetry.mq3);
    addFloatOrNull(root, "mq135", telemetry.mq135);
    addFloatOrNull(root, "mq4",   telemetry.mq4);

    // --- 8-9. GPS ---
    addFloatOrNull(root, "latitude",  telemetry.latitude);
    addFloatOrNull(root, "longitude", telemetry.longitude);

    // --- 10. Battery (null until a validated SoC model is implemented) ---
    addFloatOrNull(root, "battery", telemetry.battery);

    String payload;
    serializeJson(doc, payload);
    return payload;
}
