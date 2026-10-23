/**
 * @file        sd_manager.cpp
 * @brief       SD Card Logging Manager
 * @project     FactorySense Industrial IoT Firmware
 * @platform    ESP32-S3 (Arduino Framework)
 * * @section Description
 * Implementation of an industrial-grade non-volatile storage logging subsystem 
 * utilizing the SPI bus interface with the SD library. Coordinates concurrent 
 * file writing vectors to preserve historical diagnostic records across soft/hard resets,
 * firmware panic events, operational events, and system structural faults.
 * * @section Responsibilities
 * - Sets custom SPI line allocations matching physical board schemas.
 * - Samples the ESP-IDF reset reason register mapping down to a text log file.
 * - Wraps string parsing vectors to bundle asynchronous event and fault records.
 * - Evaluates secondary real-time clock availability profiles to append safe tracking headers.
 * * @note        STRICT NON-FUNCTIONAL CLEANUP PASS. Do not alter execution flow,
 * timing sequences, or state variables.
 */

// 1. Standard C/C++ Libraries
// (None required natively in global scope)

// 2. ESP-IDF / Core Arduino Libraries
#include <SPI.h>
#include <SD.h>
#include <esp_system.h>

// 3. Third-Party Libraries
// (Managed inside subsystem implementation layers)

// 4. Project Local Headers
#include "sd_manager.h"
#include "pins.h"
#include "rtc_manager.h"

// =====================================================
// Internal State
// =====================================================

static bool sdReady = false;

// =====================================================
// Timestamp Helper
// =====================================================

/**
 * @brief  Retrieves the current chronological label to prepend onto new log entries.
 * @return String representing an ISO8601 clock format, or "RTC_INVALID" on battery depletion.
 */
static String getLogTimestamp()
{
    if (isRTCValid())
    {
        return getRTCTimestampISO8601();
    }

    return "RTC_INVALID";
}

// =====================================================
// SD Initialization
// =====================================================

/**
 * @brief  Configures shared hardware SPI channels and mounts the physical SD filesystem layer.
 * @return true if the media sector structure is verified and attached, false otherwise.
 */
bool initSD()
{
    SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);

    if (!SD.begin(SD_CS_PIN))
    {
        Serial.println("[SD] Init Failed");
        sdReady = false;
        return false;
    }

    sdReady = true;
    Serial.println("[SD] Card Mounted");
    return true;
}

// =====================================================
// Reset Reason Logging
// =====================================================

/**
 * @brief  Queries hardware register flags and appends the translated boot rationale string to flash.
 */
void logResetReason()
{
    if (!sdReady)
    {
        return;
    }

    File file = SD.open("/reset_log.txt", FILE_APPEND);

    if (!file)
    {
        Serial.println("[SD] Failed To Open reset_log.txt");
        return;
    }

    esp_reset_reason_t reason = esp_reset_reason();

    file.print(getLogTimestamp());
    file.print(" : ");

    switch (reason)
    {
        case ESP_RST_POWERON:
            file.println("POWER_RESET");
            break;

        case ESP_RST_TASK_WDT:
            file.println("WATCHDOG_RESET");
            break;

        case ESP_RST_INT_WDT:
            file.println("INTERRUPT_WATCHDOG_RESET");
            break;

        case ESP_RST_BROWNOUT:
            file.println("BROWNOUT_RESET");
            break;

        case ESP_RST_SW:
            file.println("SOFTWARE_RESET");
            break;

        case ESP_RST_PANIC:
            file.println("PANIC_RESET");
            break;

        default:
            file.println("OTHER_RESET");
            break;
    }

    file.flush();
    file.close();
}

// =====================================================
// Event Logging
// =====================================================

/**
 * @brief  Commits a generic runtime operational tracking statement directly into the event text file.
 * @param  event reference string describing the logged system event milestone.
 */
void logEvent(const String& event)
{
    if (!sdReady)
    {
        return;
    }

    File file = SD.open("/event_log.txt", FILE_APPEND);

    if (!file)
    {
        Serial.println("[SD] Failed To Open event_log.txt");
        return;
    }

    file.print(getLogTimestamp());
    file.print(" : ");
    file.println(event);

    file.flush();
    file.close();
}

// =====================================================
// Fault Logging
// =====================================================

/**
 * @brief  Forwards hardware subsystem or transactional faults into the central file infrastructure.
 * @param  fault reference string identifying the anomalous execution code.
 */
void logFault(const String& fault)
{
    logEvent(fault);
}