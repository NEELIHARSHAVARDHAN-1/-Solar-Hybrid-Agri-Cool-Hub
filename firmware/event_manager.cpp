/**
 * @file        event_manager.cpp
 * @brief       Telemetry event evaluation — AgriCoolHub version.
 * @project     AgriCoolHub Solar Hybrid Agri Cool Hub
 * @platform    ESP32-S3 (Arduino Framework)
 *
 * @section Description
 * Evaluates incoming telemetry against threshold maps to decide when to
 * publish. Uses software hysteresis and delta-change thresholds for
 * temperature and humidity. Heartbeat interval ensures periodic publishing
 * even when values are stable.
 *
 * Alarm evaluation on MQ percentages is intentionally NOT implemented here.
 * MQ values are normalized output percentages, not calibrated concentrations,
 * and must not be used to trigger safety alarms without a validated model.
 */

#include <math.h>
#include <Arduino.h>

#include "event_manager.h"
#include "telemetry.h"

// =====================================================
// Change Detection Thresholds
// =====================================================

static const float TEMP_THRESHOLD     = 0.5f;    // degrees C
static const float HUMIDITY_THRESHOLD = 2.0f;    // %RH

// =====================================================
// Heartbeat Configuration
// =====================================================

// 5-minute heartbeat interval (always publish alive even when stable)
static const unsigned long HEARTBEAT_MS = 300000UL;

// =====================================================
// Stored State
// =====================================================

static float lastStoredTemp    = NAN;
static float lastStoredHumidity = NAN;
static unsigned long lastHeartbeat = 0;

// NaN recovery tracking
static bool nanPendingPublish  = false;
static bool nanRecoveryPending = false;
static unsigned long lastNaNDetected = 0;

// Alarm states (temperature only — humidity shown but no hysteresis alarm)
static const float TEMP_ALARM_ON  = 40.0f;   // cold-chain threshold example
static const float TEMP_ALARM_OFF = 38.0f;
static bool tempAlarmActive = false;

// =====================================================
// Alarm State Machine (Internal)
// =====================================================

static void updateAlarmStates()
{
    if (isnan(telemetry.temperature)) return;

    if (!tempAlarmActive)
    {
        if (telemetry.temperature >= TEMP_ALARM_ON)
        {
            tempAlarmActive = true;
            Serial.println("[ALARM] Temperature Alarm ON");
        }
    }
    else
    {
        if (telemetry.temperature <= TEMP_ALARM_OFF)
        {
            tempAlarmActive = false;
            Serial.println("[ALARM] Temperature Alarm OFF");
        }
    }
}

// =====================================================
// Publishing Decision
// =====================================================

bool shouldPublishTelemetry()
{
    updateAlarmStates();

    // Always publish first packet after boot
    if (isnan(lastStoredTemp))
    {
        return true;
    }

    // NaN timeout: force publish if any primary sensor is NaN for 10 s
    if (isnan(telemetry.temperature) || isnan(telemetry.humidity))
    {
        if (!nanPendingPublish)
        {
            nanPendingPublish = true;
            lastNaNDetected   = millis();
        }
        else if ((millis() - lastNaNDetected) >= 10000UL)
        {
            nanPendingPublish = false;
            Serial.println("[EVENT] Forced publish — NaN sensor value");
            return true;
        }
    }
    else
    {
        if (nanPendingPublish)
        {
            nanPendingPublish  = false;
            nanRecoveryPending = true;
        }
    }

    if (nanRecoveryPending)
    {
        nanRecoveryPending = false;
        Serial.println("[EVENT] Publish on sensor recovery");
        return true;
    }

    // Alarm active
    if (tempAlarmActive) return true;

    // Delta thresholds
    if (!isnan(telemetry.temperature) &&
        fabs(telemetry.temperature - lastStoredTemp) >= TEMP_THRESHOLD)
    {
        return true;
    }

    if (!isnan(telemetry.humidity) &&
        fabs(telemetry.humidity - lastStoredHumidity) >= HUMIDITY_THRESHOLD)
    {
        return true;
    }

    return false;
}

// =====================================================
// Heartbeat
// =====================================================

bool shouldSendHeartbeat()
{
    return ((millis() - lastHeartbeat) >= HEARTBEAT_MS);
}

// =====================================================
// Snapshot Update
// =====================================================

void updateStoredSnapshot()
{
    lastStoredTemp     = telemetry.temperature;
    lastStoredHumidity = telemetry.humidity;
    lastHeartbeat      = millis();
}

// =====================================================
// Alarm Status APIs (kept for interface compatibility)
// =====================================================

bool isTemperatureAlarmActive() { return tempAlarmActive; }
bool isCurrentAlarmActive()     { return false; }   // not used in AgriCoolHub
bool isVibrationAlarmActive()   { return false; }   // not used in AgriCoolHub
bool isAnyAlarmActive()         { return tempAlarmActive; }
