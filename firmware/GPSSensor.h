/**
 * @file        GPSSensor.h
 * @brief       GPS UART parsing + IP geolocation fallback interface.
 * @project     AgriCoolHub Solar Hybrid Agri Cool Hub
 * @platform    ESP32-S3 (Arduino Framework)
 *
 * @section LocationPriority
 *   1. Hardware GPS (TinyGPSPlus, UART1) — most accurate, used when fix is valid
 *   2. IP geolocation (ip-api.com HTTP) — city-level fallback when GPS has no fix
 *
 * @section IPGeoNote
 * IP geolocation gives approximate location (city level, ~1–10 km accuracy).
 * It is fetched once on demand and cached. It is NOT updated continuously.
 * GPS fix always overrides the IP location when available.
 */

#ifndef GPSSENSOR_H
#define GPSSENSOR_H

#include <Arduino.h>

// =====================================================
// Public API
// =====================================================

/** @brief  Initializes UART1 and the TinyGPSPlus parser. */
void initGPS();

/**
 * @brief  Feeds available UART bytes to the GPS parser (non-blocking).
 *         Call every loop() iteration.
 */
void handleGPS();

/** @brief  Returns true if a valid, fresh hardware GPS fix is held. */
bool isGPSFixValid();

/**
 * @brief  Returns latitude in decimal degrees.
 *         Source priority: hardware GPS fix → cached IP geolocation → NaN.
 */
float getGPSLatitude();

/**
 * @brief  Returns longitude in decimal degrees.
 *         Source priority: hardware GPS fix → cached IP geolocation → NaN.
 */
float getGPSLongitude();

/**
 * @brief  Returns ISO 8601 UTC timestamp from GPS hardware when valid,
 *         or empty string otherwise. Never falls back to IP or device time.
 */
String getGPSTimestamp();

/**
 * @brief  Fetches approximate location from ip-api.com using the device's
 *         public IP address. Stores result in internal cache.
 *         Only called when WiFi is connected and GPS has no fix.
 *         Blocks for up to 5 seconds (HTTP request) — call from setup()
 *         or a timed background slot, NOT inside the tight loop.
 */
void fetchIPGeolocation();

/**
 * @brief  Returns true if a cached IP geolocation result is available.
 */
bool isIPGeoAvailable();

#endif // GPSSENSOR_H
