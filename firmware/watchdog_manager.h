/**
 * @file        watchdog_manager.h
 * @brief       ESP32-S3 Task Watchdog Timer (TWDT) management interface.
 * @project     FactorySense Industrial IoT Firmware
 * @platform    ESP32-S3 (Arduino Framework)
 * * @section Description
 * Interface definitions for the hardware task watchdog management system. Exposes
 * hooks to register execution tracking contexts with the ESP-IDF subsystem layer, 
 * providing safe clearing mechanisms to prevent automated chip resets during normal 
 * operational processing routines.
 * * @section Responsibilities
 * - Exposes initialization hooks to bind the primary execution task loop to the TWDT.
 * - Outlines safe, periodic clearing routines (`feedWatchdog`) to reset hardware fallback counters.
 * * @note        STRICT NON-FUNCTIONAL CLEANUP PASS. Do not alter execution flow,
 * timing sequences, or state variables.
 */

#ifndef WATCHDOG_MANAGER_H
#define WATCHDOG_MANAGER_H

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
 * @brief  Attaches the current task execution thread to the task watchdog tracking engine.
 */
void initWatchdog();

/**
 * @brief  Resets (feeds) the hardware watchdog timer counter register.
 * * Must be called periodically within the configured timeout window to prevent
 * hardware-triggered system recovery reset loops.
 */
void feedWatchdog();

#endif // WATCHDOG_MANAGER_H