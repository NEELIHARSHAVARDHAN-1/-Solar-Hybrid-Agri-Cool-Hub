/**
 * @file        rtc_manager.h
 * @brief       Real-Time Clock (RTC) management for offline timekeeping interface.
 * @project     FactorySense Industrial IoT Firmware
 * @platform    ESP32-S3 (Arduino Framework)
 * * @section Description
 * Interface definitions for the DS3231 hardware real-time clock tracking module.
 * Exposes driver initialization lifecycles, validation states for checking oscillator
 * power health, time-sync functions mapping NTP epochs down to internal registers,
 * and string formatting handlers to export UTC ISO8601 transactional timestamps.
 * * @section Responsibilities
 * - Exposes hardware I2C initialization validations for persistent timekeeping blocks.
 * - Outlines register diagnostic methods to detect backup battery depleted flags.
 * - Declares atomic synchronization methods converting network tm structs to hardware clocks.
 * - Defines transactional string generators exporting chronological metadata labels.
 * * @note        STRICT NON-FUNCTIONAL CLEANUP PASS. Do not alter execution flow,
 * timing sequences, or state variables.
 */

#ifndef RTC_MANAGER_H
#define RTC_MANAGER_H

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
 * @brief  Initializes the RTC hardware module.
 * @return True if initialization succeeds, False otherwise.
 */
bool initRTC();

/**
 * @brief  Checks if the current RTC time is valid.
 * @return True if time is set and valid, False otherwise.
 */
bool isRTCValid();

/**
 * @brief  Synchronizes the local RTC time with data from NTP.
 */
void syncRTCFromNTP();

/**
 * @brief  Returns an ISO8601 formatted timestamp string from the RTC.
 * @return ISO8601 timestamp string.
 */
String getRTCTimestampISO8601();

#endif // RTC_MANAGER_H