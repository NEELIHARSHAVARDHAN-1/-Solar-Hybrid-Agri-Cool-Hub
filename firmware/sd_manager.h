/**
 * @file        sd_manager.h
 * @brief       MicroSD storage and diagnostic logging manager interface.
 * @project     FactorySense Industrial IoT Firmware
 * @platform    ESP32-S3 (Arduino Framework)
 * * @section Description
 * Interface definitions for the industrial non-volatile filesystem and logger module. 
 * Exposes lifecycle handles to mount storage blocks via explicit hardware peripheral lines 
 * and provides persistent streaming utilities to track system uptime context, runtime behaviors, 
 * and recovery sequences.
 * * @section Responsibilities
 * - Exposes hardware mount validations for dedicated board SPI configurations.
 * - Outlines diagnostic tracking entrypoints for processor crash reasons.
 * - Declares string reference writers for historical log captures (`event_log.txt`, `reset_log.txt`).
 * * @note        STRICT NON-FUNCTIONAL CLEANUP PASS. Do not alter execution flow,
 * timing sequences, or state variables.
 */

#ifndef SD_MANAGER_H
#define SD_MANAGER_H

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
 * @brief  Initializes the MicroSD card interface.
 * @return True if initialized successfully, False otherwise.
 */
bool initSD();

/**
 * @brief  Logs the CPU reset reason to the diagnostic file.
 */
void logResetReason();

/**
 * @brief  Appends a timestamped event to the event log file.
 * @param  event The description of the event to log.
 */
void logEvent(const String& event);

/**
 * @brief  Appends a timestamped critical fault to the fault log file.
 * @param  fault The description of the fault to log.
 */
void logFault(const String& fault);

#endif // SD_MANAGER_H