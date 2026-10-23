/**
 * @file        led_manager.h
 * @brief       Status LED indicators and color states interface.
 * @project     FactorySense Industrial IoT Firmware
 * @platform    ESP32-S3 (Arduino Framework)
 * * @section Description
 * Interface definitions for the asynchronous onboard addressable NeoPixel RGB LED 
 * signaling subsystem. Exposes system state enumerations and driver management hooks 
 * to render non-blocking status indications, network transit flashes, and fault profiles.
 * * @section Responsibilities
 * - Defines the master system health index enumeration (`LedState`).
 * - Exposes lifecycle handles to clear or initialize peripheral lines.
 * - Outlines poll-driven worker hooks to drive cyclical flasher timing tracks.
 * - Declares the runtime entrypoint to safely jump between signaling states.
 * * @note        STRICT NON-FUNCTIONAL CLEANUP PASS. Do not alter execution flow,
 * timing sequences, or state variables.
 */

#ifndef LED_MANAGER_H
#define LED_MANAGER_H

// 1. Standard C/C++ Libraries
// (None required natively in global scope)

// 2. ESP-IDF / Core Arduino Libraries
#include <Arduino.h>

// 3. Third-Party Libraries
// (Managed inside subsystem implementation layers)

// 4. Project Local Headers
// (None required natively in global scope)

// =====================================================
// Public System State Enumerations
// =====================================================

/**
 * @brief System state enumeration for visual signaling.
 */
enum LedState
{
    LED_OFF,
    LED_BOOTING,
    LED_CALIBRATING,
    LED_WIFI_CONNECTING,
    LED_WIFI_CONNECTED,
    LED_MQTT_CONNECTING,
    LED_MQTT_CONNECTED,
    LED_PUBLISH_SUCCESS,
    LED_BUFFERED,
    LED_WIFI_LOST,
    LED_WIFI_RETRY_FLASH,
    LED_MQTT_LOST,
    LED_SENSOR_FAULT,
    LED_WATCHDOG_RESET,
    LED_CRITICAL_FAULT
};

// =====================================================
// Public Function Prototypes
// =====================================================

/**
 * @brief  Initializes the driver configuration for the onboard RGB LED.
 */
void initLED();

/**
 * @brief  Refreshes the LED animation frames or blinking states.
 */
void updateLED();

/**
 * @brief  Transitions the status display to a new operating state color pattern.
 * @param  state The target system state to indicate.
 */
void setLEDState(LedState state);

#endif // LED_MANAGER_H