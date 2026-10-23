/**
 * @file        ntp_manager.cpp
 * @brief       NTP synchronization and timestamp generation.
 * @project     FactorySense Industrial IoT Firmware
 * @platform    ESP32-S3 (Arduino Framework)
 * * @section Description
 * Manages periodic synchronization with network time servers, updates local 
 * hardware clock chips during validation milestones, and transforms standard POSIX structures 
 * into transactional ISO8601 UTC timestamp character formats.
 * * @section Responsibilities
 * - Sets core POSIX time zone environments without tracking local offsets.
 * - Handles non-blocking time queries with safe microsecond hardware limits.
 * - Forwards synchronized time frames to persistent internal real-time clock chips.
 * - Acts as an automatic fallback tracker when external network routes are broken.
 * * @note        STRICT NON-FUNCTIONAL CLEANUP PASS. Do not alter execution flow,
 * timing sequences, or state variables.
 */

// 1. Standard C/C++ Libraries
#include <time.h>

// 2. ESP-IDF / Core Arduino Libraries
#include <WiFi.h>

// 3. Third-Party Libraries
// (Managed inside subsystem implementation layers)

// 4. Project Local Headers
#include "ntp_manager.h"
#include "rtc_manager.h"
#include "watchdog_manager.h"

// =====================================================
// NTP Configuration
// =====================================================

static const char* NTP_SERVER = "pool.ntp.org";

static bool firstSyncDone = false;
static unsigned long lastNTPSync = 0;

// Resync interval: 6 hours (in milliseconds)
static const unsigned long NTP_RESYNC_INTERVAL_MS = 21600000UL;

// =====================================================
// Initialize NTP
// =====================================================

/**
 * @brief  Configures network time structures with zero offset (UTC) and no daylight saving adjustments.
 */
void initNTP()
{
    configTime(0, 0, NTP_SERVER);

    Serial.println("[NTP] Initialized");
}

// =====================================================
// ISO8601 Timestamp Generation
// =====================================================

/**
 * @brief  Generates a formatted text representation of the current system epoch time.
 * @return String representing time in ISO8601 format ("YYYY-MM-DDTHH:MM:SSZ"). Falls back to hardware RTC on failures.
 */
String getTimestampISO8601()
{
    struct tm timeinfo;

    // Attempt to get time from system (NTP)
    if (getLocalTime(&timeinfo, 100))
    {
        char buffer[30];
        strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
        return String(buffer);
    }

    // Fallback to RTC if NTP time is unavailable
    return getRTCTimestampISO8601();
}

// =====================================================
// NTP Maintenance Tasks
// =====================================================

/**
 * @brief  Cyclic scheduling block managing network availability verification, sync tracking, and local clock updates.
 */
void handleNTP()
{
    // Verify Network Connectivity
    if (WiFi.status() != WL_CONNECTED)
    {
        return;
    }

    unsigned long now = millis();

    // Periodic Re-Synchronization Check
    if (firstSyncDone)
    {
        if ((now - lastNTPSync) < NTP_RESYNC_INTERVAL_MS)
        {
            return;
        }
    }

    feedWatchdog();

    struct tm timeinfo;

    // Attempt to synchronize system time via NTP
    if (getLocalTime(&timeinfo, 100))
    {
        Serial.println("[NTP] Time Synchronized");

        // Update the external RTC module with the new time
        syncRTCFromNTP();

        Serial.println("[RTC] Synced From NTP");

        firstSyncDone = true;
        lastNTPSync = now;
    }
}