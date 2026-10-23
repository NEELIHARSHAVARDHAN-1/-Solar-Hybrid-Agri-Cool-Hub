/**
 * @file        json_builder.h
 * @brief       Builds MQTT telemetry JSON payloads interface.
 * @project     FactorySense Industrial IoT Firmware
 * @platform    ESP32-S3 (Arduino Framework)
 * * @section Description
 * Interface definitions for the data serialization subsystem. Provides lightweight
 * methods to aggregate runtime data layers and format them into structured, 
 * standard-compliant JSON strings suitable for cloud ingestion over MQTT.
 * * @section Responsibilities
 * - Exposes the public serialization handle (`buildTelemetryJSON`).
 * - Formats float arrays, integers, and state tags into minified string buffers.
 * * @note        STRICT NON-FUNCTIONAL CLEANUP PASS. Do not alter execution flow,
 * timing sequences, or state variables.
 */

#ifndef JSON_BUILDER_H
#define JSON_BUILDER_H

// 1. Standard C/C++ Libraries
// (None required natively in global scope)

// 2. ESP-IDF / Core Arduino Libraries
#include <Arduino.h>

// 3. Third-Party Libraries
// (Managed inside subsystem implementation layers)

// 4. Project Local Headers
// (None required natively in global scope)

// =====================================================
// Public Function Prototypes
// =====================================================

/**
 * @brief  Builds and serializes the global telemetry structure into a minified JSON string.
 * @return String object carrying the fully formatted JSON text payload.
 */
String buildTelemetryJSON();

#endif // JSON_BUILDER_H