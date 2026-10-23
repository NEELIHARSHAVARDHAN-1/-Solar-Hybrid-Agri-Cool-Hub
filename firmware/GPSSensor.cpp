/**
 * @file        GPSSensor.cpp
 * @brief       GPS UART parsing (TinyGPSPlus) + IP geolocation fallback.
 * @project     AgriCoolHub Solar Hybrid Agri Cool Hub
 * @platform    ESP32-S3 (Arduino Framework)
 *
 * @section LocationPriority
 *   1. Hardware GPS fix (isGPSFixValid()) — always preferred
 *   2. IP geolocation cache (fetchIPGeolocation()) — city-level fallback
 *   3. NaN — when neither source has data
 *
 * @section IPGeoAPI
 * Uses http://ip-api.com/json/?fields=lat,lon (free, no API key, plain HTTP).
 * Returns city-level accuracy (~1–10 km). Do NOT use for precision navigation.
 * Rate limit: 45 requests/minute on free tier — fetchIPGeolocation() is called
 * at most once every 10 minutes (IP_GEO_REFRESH_MS).
 */

#include <TinyGPSPlus.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <math.h>

#include "GPSSensor.h"
#include "config.h"
#include "pins.h"
#include "wifi_manager.h"

// =====================================================
// Internal State
// =====================================================

static TinyGPSPlus gps;

// IP geolocation cache
static float  ipLat          = NAN;
static float  ipLon          = NAN;
static bool   ipGeoAvailable = false;

// Refresh IP geolocation at most once every 10 minutes
static const unsigned long IP_GEO_REFRESH_MS = 600000UL;
static unsigned long lastIPGeoFetch = 0;

// =====================================================
// Initialization
// =====================================================

void initGPS()
{
    Serial1.begin(GPS_BAUD_RATE, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    Serial.print("[GPS] UART1 opened — RX=");
    Serial.print(GPS_RX_PIN);
    Serial.print(", TX=");
    Serial.print(GPS_TX_PIN);
    Serial.print(", Baud=");
    Serial.println(GPS_BAUD_RATE);
}

// =====================================================
// Non-Blocking Parser Feed
// =====================================================

void handleGPS()
{
    while (Serial1.available())
    {
        gps.encode(Serial1.read());
    }
}

// =====================================================
// Hardware GPS Fix Check
// =====================================================

bool isGPSFixValid()
{
    return gps.location.isValid() &&
           (gps.location.age() < GPS_MAX_FIX_AGE_MS);
}

// =====================================================
// IP Geolocation Fetch
// =====================================================

/**
 * @brief  Calls ip-api.com to get lat/lon from public IP.
 *         Blocking HTTP call — call from setup() or a low-frequency slot.
 *         Skipped if WiFi is not connected, or refresh window has not elapsed.
 */
void fetchIPGeolocation()
{
    if (!isWiFiConnected())
    {
        return;
    }

    // Rate-limit: do not re-fetch within the refresh window
    unsigned long now = millis();
    if (ipGeoAvailable && (now - lastIPGeoFetch) < IP_GEO_REFRESH_MS)
    {
        return;
    }

    Serial.println("[IPGeo] Fetching location from ip-api.com...");

    HTTPClient http;
    // Request only lat and lon fields — minimal response
    http.begin("http://ip-api.com/json/?fields=lat,lon");
    http.setTimeout(5000);

    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK)
    {
        String body = http.getString();
        http.end();

        // Parse JSON response: {"lat":12.9716,"lon":77.5946}
        StaticJsonDocument<128> doc;
        DeserializationError err = deserializeJson(doc, body);

        if (!err)
        {
            float lat = doc["lat"] | NAN;
            float lon = doc["lon"] | NAN;

            if (!isnan(lat) && !isnan(lon))
            {
                ipLat          = lat;
                ipLon          = lon;
                ipGeoAvailable = true;
                lastIPGeoFetch = now;

                Serial.print("[IPGeo] Location: ");
                Serial.print(ipLat, 6);
                Serial.print(", ");
                Serial.println(ipLon, 6);
                Serial.println("[IPGeo] WARNING: city-level accuracy only (~1-10 km)");
            }
            else
            {
                Serial.println("[IPGeo] Invalid lat/lon in response");
            }
        }
        else
        {
            Serial.print("[IPGeo] JSON parse error: ");
            Serial.println(err.c_str());
        }
    }
    else
    {
        http.end();
        Serial.print("[IPGeo] HTTP error: ");
        Serial.println(httpCode);
    }
}

bool isIPGeoAvailable()
{
    return ipGeoAvailable;
}

// =====================================================
// Location Accessors (GPS priority → IP fallback → NaN)
// =====================================================

float getGPSLatitude()
{
    // 1. Hardware GPS fix — most accurate
    if (isGPSFixValid())
    {
        return (float)gps.location.lat();
    }

    // 2. IP geolocation cache — city-level fallback
    if (ipGeoAvailable)
    {
        return ipLat;
    }

    return NAN;
}

float getGPSLongitude()
{
    // 1. Hardware GPS fix — most accurate
    if (isGPSFixValid())
    {
        return (float)gps.location.lng();
    }

    // 2. IP geolocation cache — city-level fallback
    if (ipGeoAvailable)
    {
        return ipLon;
    }

    return NAN;
}

// =====================================================
// GPS Timestamp (hardware only — never IP)
// =====================================================

String getGPSTimestamp()
{
    if (!gps.date.isValid() || !gps.time.isValid())
    {
        return "";
    }

    char buf[25];
    snprintf(buf, sizeof(buf),
             "%04d-%02d-%02dT%02d:%02d:%02dZ",
             (int)gps.date.year(),
             (int)gps.date.month(),
             (int)gps.date.day(),
             (int)gps.time.hour(),
             (int)gps.time.minute(),
             (int)gps.time.second());

    return String(buf);
}
