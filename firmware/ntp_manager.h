/**
 * @file        ntp_manager.h
 * @brief       NTP time synchronization manager interface.
 * @project     FactorySense Industrial IoT Firmware
 * @platform    ESP32-S3 (Arduino Framework)
 * * @section Description
 * Interface definitions for the Network Time Protocol (NTP) synchronization module.
 * Manages background clock adjustments, internal POSIX time structure updates, and 
 * thread-safe generation of standardized UTC ISO8601 transaction tracking timestamps.
 * * @section Responsibilities
 * - Configures UDP sockets to pull chronological telemetry metrics from network time servers.
 * - Outlines state-machine worker loops to evaluate synchronization status and handle retries.
 * - Declares string formatting tools to construct uppercase ISO8601 string sequences.
 * * @note        STRICT NON-FUNCTIONAL CLEANUP PASS. Do not alter execution flow,
 * timing sequences, or state variables.
 */

#ifndef NTP_MANAGER_H
#define NTP_MANAGER_H

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
 * @brief  Configures the network connection parameters and synchronizes the local system clock using NTP.
 */
void initNTP();

/**
 * @brief  Maintains the internal NTP state machine, sync tracking, and background reconnection workflows.
 */
void handleNTP();

/**
 * @brief  Returns the current system UTC timestamp.
 * * Format: YYYY-MM-DDTHH:MM:SSZ
 * @return String object containing the structured ISO8601 timestamp string.
 */
String getTimestampISO8601();

#endif // NTP_MANAGER_H