/**
 * @file        wifi_manager.cpp
 * @brief       Industrial-grade WiFi Connection Manager with Liveness Verification
 * @project     FactorySense Industrial IoT Firmware
 * @platform    ESP32-S3 (Arduino Framework)
 * * @section Description
 * Manages low-level Wi-Fi connection states, network status callbacks, manual reconnection 
 * procedures with binary exponential backoff throttling, and cascading peripheral-to-MCU 
 * hardware fault recovery routines.
 * * @section Responsibilities
 * - Decodes async platform network stack event triggers to maintain status variables.
 * - Handles non-blocking retry loops without blocking main thread loops.
 * - Performs peripheral driver recovery asynchronously during persistent connection issues.
 * - Monitors WAN connectivity health to mitigate route-blackholing (Hotspot with no data).
 * - Triggers an emergency hardware reset when connection faults hit critical thresholds.
 * * @note        STRICT NON-FUNCTIONAL CLEANUP PASS. Do not alter execution flow,
 * timing sequences, or state variables.
 */

// 1. Standard C/C++ Libraries
// (None required natively in global scope)

// 2. ESP-IDF / Core Arduino Libraries
#include <WiFi.h>
#include <esp_wifi.h>
#include <lwip/dns.h>

// 3. Third-Party Libraries
// (Managed inside subsystem implementation layers)

// 4. Project Local Headers
#include "wifi_manager.h"
#include "config.h"
#include "watchdog_manager.h"
#include "led_manager.h"

// =====================================================
// Configuration Constants
// =====================================================

static constexpr uint32_t INITIAL_RETRY_MS = 1000;
static constexpr uint32_t MAX_RETRY_MS     = 30000;

static constexpr uint32_t WIFI_RESET_AFTER_FAILURES = 50;
static constexpr uint32_t MCU_RESET_AFTER_FAILURES  = 500;

// Non-blocking recovery state step delays
static constexpr uint32_t DISCONNECT_STEP_MS = 100;
static constexpr uint32_t OFF_STEP_MS         = 1000;
static constexpr uint32_t ON_STEP_MS          = 1000;

// Internet Liveness Thresholds (Hotspot without Internet mitigation)
static constexpr uint32_t LIVENESS_CHECK_INTERVAL_MS = 15000; // Check every 15s
static constexpr uint32_t MAX_LIVENESS_FAILURES       = 3;     // Reset after 3 failed internet checks

enum RecoveryState {
    RECOVERY_IDLE,
    RECOVERY_DISCONNECT,
    RECOVERY_OFF,
    RECOVERY_ON
};

// =====================================================
// State Variables
// =====================================================

static volatile bool wifiConnected = false;
static volatile bool reconnectRequired = false;
static bool reconnectEnabled = false;

static uint32_t reconnectDelayMs = INITIAL_RETRY_MS;
static uint32_t reconnectFailures = 0;
static unsigned long lastReconnectAttempt = 0;

// Non-blocking recovery state trackers
static RecoveryState currentRecoveryState = RECOVERY_IDLE;
static unsigned long lastRecoveryStepTime = 0;

// Liveness tracking
static unsigned long lastLivenessCheck = 0;
static uint32_t livenessFailures = 0;

// =====================================================
// WiFi Event Handler
// =====================================================

/**
 * @brief  Asynchronous callback handler managed by the core network task engine.
 * @param  event The internal event ID raised by the underlying framework.
 * @param  info Unified structure array carrying specific diagnostic details.
 */
static void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info)
{
    switch (event)
    {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            wifiConnected = true;
            reconnectRequired = false;
            reconnectFailures = 0;
            livenessFailures = 0; // Reset liveness metrics on valid lease assignment
            reconnectDelayMs = INITIAL_RETRY_MS;
            setLEDState(LED_WIFI_CONNECTED);
            break;

        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            wifiConnected = false;
            if (reconnectEnabled)
            {
                reconnectRequired = true;
            }
            break;

        default:
            break;
    }
}

// =====================================================
// Initialization
// =====================================================

/**
 * @brief  Configures the internal Wi-Fi parameters and registers the service event loops.
 */
void initWiFi()
{
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false); 
    WiFi.persistent(false);

    WiFi.onEvent(WiFiEvent);

    wifiConnected = false;
    reconnectRequired = false;
    reconnectEnabled = false;
    reconnectDelayMs = INITIAL_RETRY_MS;
    reconnectFailures = 0;
    livenessFailures = 0;
    lastReconnectAttempt = millis();
    lastLivenessCheck = millis();
    currentRecoveryState = RECOVERY_IDLE;
    lastRecoveryStepTime = 0;

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

/**
 * @brief  Arms the non-blocking exponential backoff state machine background worker for network recovery.
 */
void enableWiFiReconnect()
{
    reconnectEnabled = true;
    if (!wifiConnected)
    {
        reconnectRequired = true;
    }
    lastReconnectAttempt = millis();
    lastLivenessCheck = millis();
}

// =====================================================
// Maintenance Tasks
// =====================================================

/**
 * @brief  Cyclic tracking loop verifying network state and applying connection rules.
 */
void handleWiFi()
{
    feedWatchdog();
    unsigned long now = millis();

    // -------------------------------------------------
    // Asynchronous Stack Recovery State Machine
    // -------------------------------------------------
    if (currentRecoveryState != RECOVERY_IDLE)
    {
        switch (currentRecoveryState)
        {
            case RECOVERY_DISCONNECT:
                if (now - lastRecoveryStepTime >= DISCONNECT_STEP_MS)
                {
                    WiFi.mode(WIFI_OFF);
                    currentRecoveryState = RECOVERY_OFF;
                    lastRecoveryStepTime = now;
                }
                return;

            case RECOVERY_OFF:
                if (now - lastRecoveryStepTime >= OFF_STEP_MS)
                {
                    WiFi.mode(WIFI_STA);
                    currentRecoveryState = RECOVERY_ON;
                    lastRecoveryStepTime = now;
                }
                return;

            case RECOVERY_ON:
                if (now - lastRecoveryStepTime >= ON_STEP_MS)
                {
                    reconnectDelayMs = INITIAL_RETRY_MS;
                    currentRecoveryState = RECOVERY_IDLE;
                    reconnectRequired = true; 
                    lastReconnectAttempt = 0; 
                }
                return;
                
            default:
                break;
        }
    }

    // -------------------------------------------------
    // Hotspot Liveness Verification Engine
    // -------------------------------------------------
    if (wifiConnected && (now - lastLivenessCheck >= LIVENESS_CHECK_INTERVAL_MS))
    {
        lastLivenessCheck = now;
        
        const ip_addr_t* dnsServerPtr = dns_getserver(0);
        bool invalidDns = true;

        if (dnsServerPtr != nullptr)
        {
            if (dnsServerPtr->u_addr.ip4.addr != 0)
            {
                invalidDns = false;
            }
        }
        
        if (invalidDns)
        {
            livenessFailures++;
        }
        else
        {
            livenessFailures = 0; 
        }

        if (livenessFailures >= MAX_LIVENESS_FAILURES)
        {
            wifiConnected = false;
            livenessFailures = 0;
            WiFi.disconnect(false, false);
            currentRecoveryState = RECOVERY_DISCONNECT;
            lastRecoveryStepTime = now;
            return;
        }
    }

    if (!reconnectEnabled || !reconnectRequired)
    {
        return;
    }

    // Check if backoff time has elapsed
    if ((now - lastReconnectAttempt) < reconnectDelayMs)
    {
        return;
    }

    lastReconnectAttempt = now;
    reconnectFailures++;

    setLEDState(LED_WIFI_RETRY_FLASH);

    // Trigger Non-Blocking Recovery Chain if failures persist
    if (reconnectFailures % WIFI_RESET_AFTER_FAILURES == 0)
    {
        WiFi.disconnect(false, false);
        currentRecoveryState = RECOVERY_DISCONNECT;
        lastRecoveryStepTime = now;
        return;
    }

    // Emergency MCU Recovery
    if (reconnectFailures >= MCU_RESET_AFTER_FAILURES)
    {
        delay(1000);
        ESP.restart();
    }

    // Connection Attempt (Non-blocking)
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // Apply exponential backoff
    reconnectDelayMs = min(reconnectDelayMs * 2, MAX_RETRY_MS);
}

// =====================================================
// Status Getters
// =====================================================

/**
 * @brief  Checks the current active connection state.
 * @return true if the station is connected and has been assigned an IP address, false otherwise.
 */
bool isWiFiConnected()
{
    return wifiConnected;
}

/**
 * @brief  Retrieves the local IP address assigned to the interface.
 * @return String representation of the interface IP address.
 */
String getWiFiIP()
{
    return wifiConnected ? WiFi.localIP().toString() : "0.0.0.0";
}

/**
 * @brief  Reads the Received Signal Strength Indicator (RSSI) value.
 * @return integer signal strength in dBm, or -999 if interface is offline.
 */
int getWiFiRSSI()
{
    return wifiConnected ? WiFi.RSSI() : -999;
}