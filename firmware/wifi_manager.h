/**
 * @file        wifi_manager.h
 * @brief       WiFi connectivity and state management interface.
 * @project     FactorySense Industrial IoT Firmware
 * @platform    ESP32-S3 (Arduino Framework)
 * * @section Description
 * Interface definitions for the network subsystem layer. Establishes an asynchronous, 
 * non-blocking state machine governing Station mode (STA) connectivity, automated 
 * exponential-backoff reconnect cycles, and continuous RF signal quality (RSSI) telemetry acquisition.
 * * @section Responsibilities
 * - Configures local radio peripheral parameters and registers system event callbacks.
 * - Outlines background execution worker hooks (`handleWiFi`) to process connection drops.
 * - Exposes hardware network parameter abstraction getters (IP string formatting, RSSI tracking).
 * * @note        STRICT NON-FUNCTIONAL CLEANUP PASS. Do not alter execution flow,
 * timing sequences, or state variables.
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

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
 * @brief  Configures WiFi radio parameters, clears persistence, and fires an immediate single connection attempt.
 */
void initWiFi();

/**
 * @brief  Arms the non-blocking exponential backoff state machine background worker for network recovery.
 */
void enableWiFiReconnect();

/**
 * @brief  Cyclic tracking loop verifying network state and applying background connection rules.
 */
void handleWiFi();

/**
 * @brief  Checks the real-time Link-Layer connectivity status of the station interface.
 * @return True if connected with a valid IP lease, False otherwise.
 */
bool isWiFiConnected();

/**
 * @brief  Retrieves the local IPv4 network address assigned to the physical station interface.
 * @return String representation of the local IP handle.
 */
String getWiFiIP();

/**
 * @brief  Retrieves the latest received signal strength indicator of the network connection.
 * @return RSSI integer value measured in dBm, or -999 if interface is offline.
 */
int getWiFiRSSI();

#endif // WIFI_MANAGER_H