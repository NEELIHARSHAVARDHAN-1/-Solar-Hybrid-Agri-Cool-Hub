/**
 * @file        rtc_manager.cpp
 * @brief       DS3231 RTC Manager implementation.
 * @project     FactorySense Industrial IoT Firmware
 * @platform    ESP32-S3 (Arduino Framework)
 * * @section Description
 * Implementation of an independent local time tracking subsystem utilizing a DS3231 
 * hardware Real-Time Clock over I2C. Acts as a persistent timekeeping fallback, matching 
 * POSIX time structures to hardware registers and rendering transactional ISO8601 
 * UTC timestamp character string streams.
 * * @section Responsibilities
 * - Verifies component link integrity on the local I2C control bus.
 * - Monitors oscillator health and power loss conditions (`lostPower`).
 * - Commits valid network epoch frames down into non-volatile hardware clock registers.
 * - Constructs atomic, string-serialized tracking timestamps.
 * * @note        STRICT NON-FUNCTIONAL CLEANUP PASS. Do not alter execution flow,
 * timing sequences, or state variables.
 */

// 1. Standard C/C++ Libraries
#include <time.h>

// 2. ESP-IDF / Core Arduino Libraries
#include <Wire.h>

// 3. Third-Party Libraries
#include <RTClib.h>

// 4. Project Local Headers
#include "rtc_manager.h"

// =====================================================
// Subsystem Instantiation
// =====================================================

RTC_DS3231 rtc;

// =====================================================
// RTC Initialization
// =====================================================

/**
 * @brief  Probes the control bus to mount the hardware real-time clock chip.
 * @return true if the component responds to address initialization, false otherwise.
 */
bool initRTC()
{
    if (!rtc.begin())
    {
        Serial.println("[RTC] DS3231 Not Found");
        return false;
    }

    Serial.println("[RTC] DS3231 Detected");
    return true;
}

// =====================================================
// RTC Health Check
// =====================================================

/**
 * @brief  Evaluates internal register flags to detect power loss or oscillator failures.
 * @return true if register contents remain valid, false if an underlying reset occurred.
 */
bool isRTCValid()
{
    if (rtc.lostPower())
    {
        Serial.println("[RTC] Lost Power");
        return false;
    }

    return true;
}

// =====================================================
// Sync RTC From NTP
// =====================================================

/**
 * @brief  Binds a fresh POSIX layout structure derived from network time to internal hardware registers.
 */
void syncRTCFromNTP()
{
    struct tm timeinfo;

    if (!getLocalTime(&timeinfo))
    {
        Serial.println("[RTC] NTP Time Not Available");
        return;
    }

    rtc.adjust(
        DateTime(
            timeinfo.tm_year + 1900,
            timeinfo.tm_mon + 1,
            timeinfo.tm_mday,
            timeinfo.tm_hour,
            timeinfo.tm_min,
            timeinfo.tm_sec
        )
    );

    Serial.println("[RTC] Synced From NTP");
}

// =====================================================
// RTC ISO8601 Timestamp
// =====================================================

/**
 * @brief  Queries active hardware date-time layers to compile a standard string payload.
 * @return String object carrying the formatted ISO8601 UTC timestamp ("YYYY-MM-DDTHH:MM:SSZ").
 */
String getRTCTimestampISO8601()
{
    DateTime now = rtc.now();

    char buffer[30];

    snprintf(
        buffer,
        sizeof(buffer),
        "%04d-%02d-%02dT%02d:%02d:%02dZ",
        now.year(),
        now.month(),
        now.day(),
        now.hour(),
        now.minute(),
        now.second()
    );

    return String(buffer);
}