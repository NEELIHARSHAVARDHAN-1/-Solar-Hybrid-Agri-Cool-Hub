/**
 * @file        led_manager.cpp
 * @brief       Onboard RGB status LED management state machine.
 * @project     FactorySense Industrial IoT Firmware
 * @platform    ESP32-S3 (Arduino Framework)
 * * @section Description
 * Implementation of an asynchronous status signaling subsystem managing an onboard 
 * addressable NeoPixel RGB LED. Operates as a non-blocking cyclical state machine, 
 * rendering system milestones, networking contexts, and error fault codes.
 * * @section Responsibilities
 * - Abstracts addressable peripheral driver writes (`neopixelWrite`).
 * - Manages explicit asynchronous timing flags for periodic flasher sequences.
 * - Displays telemetry transmission success vectors via brief execution loops.
 * * @note        STRICT NON-FUNCTIONAL CLEANUP PASS. Do not alter execution flow,
 * timing sequences, or state variables.
 */

// 1. Standard C/C++ Libraries
// (None required natively in global scope)

// 2. ESP-IDF / Core Arduino Libraries
#include <Arduino.h>

// 3. Third-Party Libraries
// (Managed inside subsystem implementation layers)

// 4. Project Local Headers
#include "led_manager.h"
#include "pins.h"

// =====================================================
// Internal Variables & State Trackers
// =====================================================

static LedState currentState = LED_OFF;
static unsigned long lastBlink = 0;
static bool ledToggle = false;

// =====================================================
// RGB Helper
// =====================================================

/**
 * @brief  Writes raw color intensity components out to the targeted RGB bus line.
 * @param  r Red channel illumination scale (0-255).
 * @param  g Green channel illumination scale (0-255).
 * @param  b Blue channel illumination scale (0-255).
 */
static void setRGB(uint8_t r, uint8_t g, uint8_t b)
{
    neopixelWrite(RGB_PIN, r, g, b);
}

// =====================================================
// Initialization
// =====================================================

/**
 * @brief  Blanks out RGB output lines to ensure an unlit initialization checkpoint.
 */
void initLED()
{
    setRGB(0, 0, 0);
}

// =====================================================
// State Setter
// =====================================================

/**
 * @brief  Updates the global internal state pointer to switch active flash profiles.
 * @param  state The requested enumeration code representing new hardware health indexes.
 */
void setLEDState(LedState state)
{
    currentState = state;
}

// =====================================================
// LED State Machine
// =====================================================

/**
 * @brief  Asynchronously evaluates delta timing parameters to drive the active status color mask.
 */
void updateLED()
{
    unsigned long now = millis();

    switch (currentState)
    {
        case LED_OFF:
        case LED_WIFI_CONNECTED:
        case LED_MQTT_CONNECTED:
            setRGB(0, 0, 0);
            break;

        case LED_BOOTING:
            if (now - lastBlink > 500)
            {
                lastBlink = now;
                ledToggle = !ledToggle;
                setRGB(0, 0, ledToggle ? 255 : 0); // Blinking Blue
            }
            break;

        case LED_CALIBRATING:
            if (now - lastBlink > 500)
            {
                lastBlink = now;
                ledToggle = !ledToggle;
                setRGB(0, ledToggle ? 255 : 0, ledToggle ? 255 : 0); // Blinking Cyan
            }
            break;

        case LED_WIFI_CONNECTING:
            if (now - lastBlink > 1000)
            {
                lastBlink = now;
                ledToggle = !ledToggle;
                setRGB(ledToggle ? 255 : 0, ledToggle ? 255 : 0, 0); // Blinking Yellow
            }
            break;

        case LED_WIFI_RETRY_FLASH:
            if (!ledToggle)
            {
                ledToggle = true;
                lastBlink = now;
                setRGB(255, 255, 0);
            }

            if (now - lastBlink >= 100)
            {
                setRGB(0, 0, 0);
                ledToggle = false;
                currentState = LED_OFF;
            }
            break;

        case LED_MQTT_CONNECTING:
            if (now - lastBlink > 500)
            {
                lastBlink = now;
                ledToggle = !ledToggle;
                setRGB(ledToggle ? 255 : 0, 0, ledToggle ? 255 : 0); // Blinking Magenta
            }
            break;

        case LED_WIFI_LOST:
            setRGB(0, 0, 0);
            break;

        case LED_MQTT_LOST:
            if (now - lastBlink > 2000)
            {
                lastBlink = now;
                ledToggle = !ledToggle;
                setRGB(ledToggle ? 255 : 0, 0, 0); // Slow Blinking Red
            }
            break;

        case LED_PUBLISH_SUCCESS:
            if (!ledToggle)
            {
                ledToggle = true;
                lastBlink = now;
                setRGB(0, 255, 0); // Solid Green for 500ms
            }

            if (now - lastBlink >= 500)
            {
                setRGB(0, 0, 0);
                ledToggle = false;
                currentState = LED_OFF;
            }
            break;

        case LED_BUFFERED:
            if (now - lastBlink > 150)
            {
                lastBlink = now;
                ledToggle = !ledToggle;
                setRGB(ledToggle ? 255 : 0, 0, ledToggle ? 255 : 0); // Fast Blinking Magenta
            }
            break;

        case LED_SENSOR_FAULT:
            if (now - lastBlink > 150)
            {
                lastBlink = now;
                ledToggle = !ledToggle;
                setRGB(ledToggle ? 255 : 0, 0, 0); // Fast Blinking Red
            }
            break;

        case LED_CRITICAL_FAULT:
            setRGB(255, 0, 0); // Solid Red
            break;

        default:
            break;
    }
}