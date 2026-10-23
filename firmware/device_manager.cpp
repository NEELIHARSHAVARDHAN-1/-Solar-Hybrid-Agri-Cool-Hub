/**
 * @file        device_manager.cpp
 * @brief       Device identification implementation.
 * @project     FactorySense Industrial IoT Firmware
 * @platform    ESP32-S3 (Arduino Framework)
 * * @section Description
 * Implementation of the core system identification module. Extracts the fused factory 
 * hardware MAC layer from the ESP32-S3 eFuse blocks and packages it into a standard 
 * null-terminated alphanumeric hex sequence to act as an unalterable global device key.
 * * @section Responsibilities
 * - Queries the lower networking abstraction layer to fetch hardware-fused MAC arrays.
 * - Formats system byte blocks into consistent uppercase alphanumeric string IDs.
 * * @note        STRICT NON-FUNCTIONAL CLEANUP PASS. Do not alter execution flow,
 * timing sequences, or state variables.
 */

// 1. Standard C/C++ Libraries
#include <stdio.h>

// 2. ESP-IDF / Core Arduino Libraries
#include <esp_mac.h>

// 3. Third-Party Libraries
// (None required natively in global scope)

// 4. Project Local Headers
#include "device_manager.h"

// =====================================================
// Device Identification Utilities
// =====================================================

/**
 * @brief  Generates a unique hardware deployment tracking string key using regional eFuse arrays.
 * @return String representing the unique, fixed 12-character uppercase hexadecimal device identifier.
 */
String getDeviceID()
{
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    char deviceId[13];
    snprintf(deviceId, sizeof(deviceId), "%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    return String(deviceId);
}