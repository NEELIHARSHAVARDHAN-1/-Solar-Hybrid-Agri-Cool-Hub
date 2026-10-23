/**
 * @file        buffer_manager.h
 * @brief       Header for the circular telemetry buffer management system.
 * @project     FactorySense Industrial IoT Firmware
 * @platform    ESP32-S3 (Arduino Framework)
 * * @section Description
 * Interface definitions for a local volatile circular queue tracking system. Exposes
 * hooks to temporarily capture serialized JSON payloads during periods of network/MQTT 
 * dropouts, implementing FIFO (First-In, First-Out) eviction mechanics to preserve system stability.
 * * @section Responsibilities
 * - Exposes ring-index tracking state management controllers.
 * - Declares memory-isolated push (`addToBuffer`) and pull (`getBufferedData`) string handles.
 * - Provides non-destructive peeking tools and size checkers to monitor network recovery state.
 * * @note        STRICT NON-FUNCTIONAL CLEANUP PASS. Do not alter execution flow,
 * timing sequences, or state variables.
 */

#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

// 1. Standard C/C++ Libraries
// (None required natively in global scope)

// 2. ESP-IDF / Core Arduino Libraries
#include <Arduino.h>

// 3. Third-Party Libraries
// (Managed inside subsystem implementation layers)

// 4. Project Local Headers
// (None required natively in global scope)

// =====================================================
// Public Subsystem Constants
// =====================================================

/**
 * @brief Maximum number of telemetry payloads the buffer can store.
 */
static const int BUFFER_SIZE = 60;

// =====================================================
// Public Function Prototypes
// =====================================================

/**
 * @brief  Initializes the telemetry buffer ring indices, cursors, and internal counters.
 */
void initBuffer();

/**
 * @brief  Adds a telemetry JSON string to the circular buffer layout.
 * * If the buffer reaches capacity, the oldest entry is evicted to prevent overwrite errors.
 * @param  payload The telemetry data string to store.
 */
void addToBuffer(const String& payload);

/**
 * @brief  Checks if there are any pending unsent payloads resting inside the buffer queue.
 * @return True if data is available for extraction, false otherwise.
 */
bool hasBufferedData();

/**
 * @brief  Retrieves the oldest telemetry payload string frame without popping it from the queue.
 * @return The oldest payload string, or an empty string object if the queue is exhausted.
 */
String getBufferedData();

/**
 * @brief  Removes the oldest telemetry payload frame from the circular buffer allocation.
 */
void removeBufferedData();

/**
 * @brief  Gets the number of payloads currently stored in the ring buffer.
 * @return Current active buffer occupancy count elements.
 */
int getBufferedCount();

#endif // BUFFER_MANAGER_H