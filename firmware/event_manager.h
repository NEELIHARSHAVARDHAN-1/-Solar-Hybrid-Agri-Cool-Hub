/**
 * @file        event_manager.h
 * @brief       Header for telemetry event evaluation and alarm management.
 * @project     FactorySense Industrial IoT Firmware
 * @platform    ESP32-S3 (Arduino Framework)
 * * @section Description
 * Interface definitions for the industrial analytics, change-detection, and hysteresis
 * processing system. Exposes hooks for event scheduling, conditional reporting thresholds, 
 * network heartbeat tracking, and multi-sensor alarm states.
 * * @section Responsibilities
 * - Exposes evaluation checks for delta limits and state changes.
 * - Outlines heartbeat chronological boundary verification hooks.
 * - Provides modular accessor interfaces to query sensor status anomalies.
 * * @note        STRICT NON-FUNCTIONAL CLEANUP PASS. Do not alter execution flow,
 * timing sequences, or state variables.
 */

#ifndef EVENT_MANAGER_H
#define EVENT_MANAGER_H

// 1. Standard C/C++ Libraries
// (None required natively in global scope)

// 2. ESP-IDF / Core Arduino Libraries
// (None required natively in global scope)

// 3. Third-Party Libraries
// (Managed inside subsystem implementation layers)

// 4. Project Local Headers
// (None required natively in global scope)

// =====================================================
// Public Function Prototypes
// =====================================================

/**
 * @brief  Evaluates whether telemetry should be published based on sensor threshold changes or active alarm states.
 * @return True if publication is required, false otherwise.
 */
bool shouldPublishTelemetry();

/**
 * @brief  Checks if the periodic heartbeat interval has elapsed.
 * @return True if a heartbeat transmission is due.
 */
bool shouldSendHeartbeat();

/**
 * @brief  Updates the internal snapshot of sensor values and resets the heartbeat timer after a successful publish.
 */
void updateStoredSnapshot();

/**
 * @brief  Checks the active status of the temperature alarm.
 * @return True if the temperature is currently in an alarm state.
 */
bool isTemperatureAlarmActive();

/**
 * @brief  Checks the active status of the current alarm.
 * @return True if the current is currently in an alarm state.
 */
bool isCurrentAlarmActive();

/**
 * @brief  Checks the active status of the vibration alarm.
 * @return True if the vibration magnitude is currently in an alarm state.
 */
bool isVibrationAlarmActive();

/**
 * @brief  Checks if any system alarm is currently active.
 * @return True if at least one sensor alarm is active.
 */
bool isAnyAlarmActive();

#endif // EVENT_MANAGER_H