/**
 * @file        web_log_manager.h
 * @brief       Web-based diagnostics and log viewing interface.
 * @project     FactorySense Industrial IoT Firmware
 * @platform    ESP32-S3 (Arduino Framework)
 * * @section Description
 * Interface definition for an embedded HTTP server diagnostic layer. Provides API 
 * endpoints to expose real-time application logs, active system metrics, and fault footprints
 * asynchronously over a local network socket connection.
 * * @section Responsibilities
 * - Declares the lifecycle hook to spin up socket binders on standard HTTP port channels.
 * - Exposes poll-driven request dispatcher hooks for asynchronous web server routines.
 * * @note        STRICT NON-FUNCTIONAL CLEANUP PASS. Do not alter execution flow,
 * timing sequences, or state variables.
 */

#ifndef WEB_LOG_MANAGER_H
#define WEB_LOG_MANAGER_H

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
 * @brief  Initializes the internal HTTP server for log access.
 */
void initWebLogServer();

/**
 * @brief  Handles incoming HTTP requests for log retrieval.
 */
void handleWebLogServer();

#endif // WEB_LOG_MANAGER_H