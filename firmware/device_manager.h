/**
 * @file        device_manager.h
 * @brief       Device identification utilities interface.
 * @project     FactorySense Industrial IoT Firmware
 * @platform    ESP32-S3 (Arduino Framework)
 * * @section Description
 * Interface definitions for the physical unit identification subsystem. Exposes 
 * helper methods to generate stable, immutable hardware device identifiers by extracting 
 * and transforming the silicon-backed ESP32 chip MAC layer.
 * * @section Responsibilities
 * - Exposes the public getter interface (`getDeviceID`) to obtain unique physical handles.
 * - Outlines character-array serialization models used to index individual edge nodes.
 * * @note        STRICT NON-FUNCTIONAL CLEANUP PASS. Do not alter execution flow,
 * timing sequences, or state variables.
 */

#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

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
 * @brief  Returns a unique, immutable hardware device identifier string.
 * * Device ID is generated from the hardware-fused ESP32 WiFi Station MAC address.
 * * Example Output: "14C19F2DDFF4"
 * @return String object carrying the uppercase alphanumeric signature.
 */
String getDeviceID();

#endif // DEVICE_MANAGER_H