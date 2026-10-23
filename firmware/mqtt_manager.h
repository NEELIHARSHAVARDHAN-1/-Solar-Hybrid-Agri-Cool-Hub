/**
 * @file        mqtt_manager.h
 * @brief       MQTT client management and messaging interface.
 * @project     FactorySense Industrial IoT Firmware
 * @platform    ESP32-S3 (Arduino Framework)
 * * @section Description
 * Interface definitions for the secure PubSub messaging client wrapper subsystem. 
 * Exposes hooks for driver initialization, non-blocking background connection 
 * state-machine tracking, connection verification, and atomic publishing channels 
 * for transactional telemetry and system health status frames.
 * * @section Responsibilities
 * - Registers event loops and secures credentials for protected TLS socket sessions.
 * - Outlines network maintenance routines (`handleMQTT`) to manage retry delays.
 * - Declares string buffer publication methods for both serialized telemetry and status logs.
 * * @note        STRICT NON-FUNCTIONAL CLEANUP PASS. Do not alter execution flow,
 * timing sequences, or state variables.
 */

#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

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
 * @brief  Initializes the MQTT client configuration, certificate attachments, and event registration.
 */
void initMQTT();

/**
 * @brief  Maintains the background MQTT connection state-machine and handles periodic keep-alive tasks.
 */
void handleMQTT();

/**
 * @brief  Checks the current network connectivity status to the remote MQTT broker.
 * @return True if connected, False otherwise.
 */
bool isMQTTConnected();

/**
 * @brief  Publishes real-time telemetry data to the specified MQTT topic destination.
 * @param  topic   The target MQTT topic string pathway.
 * @param  payload The JSON-formatted telemetry string data.
 * @return True if publication was successful, False otherwise.
 */
bool publishTelemetry(const String& topic, const String& payload);

/**
 * @brief  Publishes system operational status messages to the specified MQTT topic destination.
 * @param  topic   The target MQTT topic string pathway.
 * @param  payload The status tracking message payload string.
 * @return True if publication was successful, False otherwise.
 */
bool publishStatus(const String& topic, const String& payload);

#endif // MQTT_MANAGER_H