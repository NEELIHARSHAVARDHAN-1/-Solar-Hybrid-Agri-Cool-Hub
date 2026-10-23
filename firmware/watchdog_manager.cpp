/**
 * @file        watchdog_manager.cpp
 * @brief       Task Watchdog Timer (TWDT) management for ESP32-S3.
 * @project     FactorySense Industrial IoT Firmware
 * @platform    ESP32-S3 (Arduino / ESP-IDF Hybrid Framework)
 * * @section Description
 * Implementation of a foundational system stability monitoring subsystem leveraging the 
 * native ESP-IDF Task Watchdog Timer (TWDT) driver API. Provides hardware-backed panic 
 * fallback sequences to automatically recover from critical lockups, endless loops, or deadlocks.
 * * @section Responsibilities
 * - Provisions system-wide multi-core idle task watching thresholds (`esp_task_wdt_config_t`).
 * - Attaches the primary execution thread context directly to the active monitoring subsystem.
 * - Exposes low-latency, non-blocking check-in hooks (`feedWatchdog`) to service active tickers.
 * * @note        STRICT NON-FUNCTIONAL CLEANUP PASS. Do not alter execution flow,
 * timing sequences, or state variables.
 */

// 1. Standard C/C++ Libraries
// (None required natively in global scope)

// 2. ESP-IDF / Core Arduino Libraries
#include <esp_task_wdt.h>

// 3. Third-Party Libraries
// (Managed inside subsystem implementation layers)

// 4. Project Local Headers
#include "watchdog_manager.h"

// =====================================================
// Watchdog Optimization Logic
// =====================================================

/**
 * @brief  Configures the native Task Watchdog infrastructure and hooks the active task handle.
 */
void initWatchdog()
{
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = 30000,
        .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
        .trigger_panic = true
    };

    esp_err_t result = esp_task_wdt_init(&wdt_config);
    Serial.printf("[WDT] Timeout = %d ms\n", wdt_config.timeout_ms);

    if (result != ESP_OK && result != ESP_ERR_INVALID_STATE)
    {
        Serial.printf("[WDT] Init Failed: %d\n", result);
        return;
    }

    result = esp_task_wdt_add(NULL);

    Serial.printf("[WDT] Attach Result = %d\n", result);
}

/**
 * @brief  Resets the active Task Watchdog hardware counter to prevent system panic sequences.
 */
void feedWatchdog()
{
    esp_err_t result = esp_task_wdt_reset();

    if (result != ESP_OK && result != ESP_ERR_NOT_FOUND)
    {
        Serial.printf("[WDT] Reset Failed: %d\n", result);
    }
}