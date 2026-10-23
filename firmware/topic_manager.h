/**
 * @file        topic_manager.h
 * @brief       MQTT topic generation utilities interface.
 * @project     FactorySense Industrial IoT Firmware
 * @platform    ESP32-S3 (Arduino Framework)
 * * @section Description
 * Interface definitions for uniform MQTT namespace topic structuring. Dynamically
 * builds transactional topic address string structures by binding distinct hardware unique 
 * identification keys onto standard hierarchy templates.
 * * @section Responsibilities
 * - Constructs atomic, string-serialized telemetry publication paths.
 * - Compiles persistent state topic string layouts for connection and availability heartbeats.
 * * @note        STRICT NON-FUNCTIONAL CLEANUP PASS. Do not alter execution flow,
 * timing sequences, or state variables.
 */

#ifndef TOPIC_MANAGER_H
#define TOPIC_MANAGER_H

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
 * @brief  Constructs the active real-time data streaming topic path.
 * * Format: "factorysense/<device_id>/telemetry"
 * @param  deviceId Reference string containing the hardware unique identifier.
 * @return String object carrying the fully qualified MQTT telemetry topic.
 */
String getTelemetryTopic(const String& deviceId);

/**
 * @brief  Constructs the node connectivity heartbeat topic path.
 * * Format: "factorysense/<device_id>/status"
 * @param  deviceId Reference string containing the hardware unique identifier.
 * @return String object carrying the fully qualified MQTT status topic.
 */
String getStatusTopic(const String& deviceId);

#endif // TOPIC_MANAGER_H