/**
 * @file        FactorySense.ino
 * @brief       AgriCoolHub main application entry point.
 * @project     Solar Hybrid Agri Cool Hub — AI-Enabled Smart Crop Preservation
 * @platform    ESP32-S3 (Arduino Framework)
 *
 * @section SensorMapping
 *   SHT4x    — I2C (SDA=8, SCL=9)  temperature + humidity
 *   INA219   — I2C (SDA=8, SCL=9)  bus voltage / current / power
 *   MQ-3     — ADC GPIO4            alcohol-sensitive analog output
 *   MQ-135   — ADC GPIO5            air-quality analog output
 *   MQ-4     — ADC GPIO6            methane/natural-gas analog output
 *   NEO-6M   — UART1 RX=17, TX=18  GPS latitude / longitude / UTC time
 *
 * @section JSONContract
 *   Ten-field payload published every 15 seconds:
 *   device_id, timestamp, temperature, humidity,
 *   mq3, mq135, mq4, latitude, longitude, battery
 *
 * @section MQInterpretationRestriction
 *   mq3 / mq135 / mq4 are normalized ADC output percentages (0–100 %).
 *   They are NOT calibrated gas concentrations, NOT PPM values, and must
 *   NOT be used as gas-leak or safety alarms without a validated model.
 *
 * @section ElectricalSafety
 *   All ADC pins (GPIO4/5/6) must remain ≤ 3.3 V.
 *   Verify voltage-divider resistors with a multimeter before powering on.
 *   ESP32-S3 GPIOs are NOT 5 V tolerant.
 */

// ── Standard / Core ────────────────────────────────────────────────────────
#include <Arduino.h>
#include <Wire.h>
#include "esp_system.h"

// ── Project Headers ─────────────────────────────────────────────────────────
#include "config.h"
#include "pins.h"
#include "telemetry.h"
#include "device_manager.h"
#include "topic_manager.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"
#include "ntp_manager.h"
#include "watchdog_manager.h"
#include "json_builder.h"
#include "buffer_manager.h"
#include "led_manager.h"
#include "sd_manager.h"
#include "rtc_manager.h"
#include "web_log_manager.h"
#include "event_manager.h"

// ── New AgriCoolHub Sensor Modules ──────────────────────────────────────────
#include "SHT4xSensor.h"
#include "INA219Sensor.h"
#include "MQSensor.h"
#include "GPSSensor.h"

// ============================================================
// Timing Variables
// ============================================================

static unsigned long lastSensorRead    = 0;
static unsigned long lastWiFiRead      = 0;
static unsigned long lastTimestampRead = 0;
static unsigned long lastJSONPublish   = 0;
static unsigned long lastIPGeoCheck    = 0;

// ============================================================
// State Flags
// ============================================================

static bool systemReady  = false;
static bool mqttStarted  = false;

// ============================================================
// setup()
// ============================================================

void setup()
{
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    Serial.println();
    Serial.println("===========================================");
    Serial.println("     AgriCoolHub Boot");
    Serial.println("===========================================");
    Serial.println();
    Serial.println("===== MEMORY INFO =====");
    Serial.print("PSRAM Size : "); Serial.println(ESP.getPsramSize());
    Serial.print("Free PSRAM : "); Serial.println(ESP.getFreePsram());
    Serial.println("=======================");
    Serial.println();

    // ── Core Infrastructure ────────────────────────────────────────────────
    initLED();
    setLEDState(LED_BOOTING);
    updateLED();

    // Set stable device ID from config (override MAC-based ID for this project)
    telemetry.device_id = DEVICE_ID;
    Serial.print("Device ID : ");
    Serial.println(telemetry.device_id);

    bool rtcAvailable = initRTC();
    initBuffer();
    initWiFi();
    initNTP();

    if (!initSD())
    {
        Serial.println("[SD] Logging Disabled");
    }

    logResetReason();

    if (rtcAvailable)
    {
        if (!isRTCValid())
        {
            Serial.println("[RTC] Waiting For First Sync");
            logEvent("RTC_INVALID");
        }
        else
        {
            Serial.println("[RTC] Valid");
        }
    }

    initWebLogServer();

    // ── Sensor Initialization ──────────────────────────────────────────────

    // I2C bus — initialize once; SHT4x and INA219 share it
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Serial.println("[I2C] Bus initialized (SDA=8, SCL=9)");

    initSHT4x();
    initINA219();
    initMQSensors();   // sets 12-bit ADC resolution
    initGPS();

    // ── Watchdog ───────────────────────────────────────────────────────────
    initWatchdog();
    Serial.println("[Watchdog] Initialized");

    // ── MQ Sensor Warm-Up Notice ───────────────────────────────────────────
    // MQ heaters need time before readings are meaningful.
    // We do NOT block here — the isMQWarmedUp() guard in the loop prevents
    // publishing MQ values before the warm-up period has elapsed.
    Serial.print("[MQ] Warm-up period: ");
    Serial.print(MQ_WARMUP_MS / 1000UL);
    Serial.println(" s (non-blocking)");

    // ── Startup Complete ───────────────────────────────────────────────────
    Serial.println();
    Serial.println("===========================================");
    Serial.println("     AgriCoolHub Started");
    Serial.println("===========================================");
    Serial.println();

    systemReady = true;
    enableWiFiReconnect();

    if (isWiFiConnected())
    {
        Serial.println("[BOOT] WiFi detected. Initializing MQTT...");
        initMQTT();
        mqttStarted = true;

        // Fetch IP geolocation immediately if GPS has no fix
        if (!isGPSFixValid())
        {
            fetchIPGeolocation();
        }
    }
    else
    {
        Serial.println("[BOOT] WiFi unavailable. Reconnect tracking active...");
    }
}

// ============================================================
// loop()
// ============================================================

void loop()
{
    feedWatchdog();

    // ── Non-Blocking GPS Feed ──────────────────────────────────────────────
    // Must be called every iteration to drain UART FIFO without blocking.
    handleGPS();

    // ── Connectivity Tasks ─────────────────────────────────────────────────
    handleWiFi();

    if (systemReady && !mqttStarted && isWiFiConnected())
    {
        Serial.println("[LOOP] WiFi recovered. Launching MQTT...");
        initMQTT();
        mqttStarted = true;

        // Fetch IP geolocation on late WiFi connect if GPS has no fix
        if (!isGPSFixValid())
        {
            fetchIPGeolocation();
        }
    }

    // ── IP Geolocation Refresh (every 10 min, only when GPS has no fix) ────
    if (isWiFiConnected() && !isGPSFixValid() &&
        (millis() - lastIPGeoCheck >= 600000UL))
    {
        lastIPGeoCheck = millis();
        fetchIPGeolocation();
    }

    if (mqttStarted)
    {
        handleMQTT();
    }

    handleNTP();
    handleWebLogServer();
    updateLED();

    // ── Buffered Telemetry Flush ───────────────────────────────────────────
    if (mqttStarted && isMQTTConnected())
    {
        String topic = getTelemetryTopic(telemetry.device_id);
        uint8_t messagesSent = 0;

        while (hasBufferedData() && messagesSent < 2)
        {
            feedWatchdog();
            yield();

            String payload = getBufferedData();

            if (publishTelemetry(topic, payload))
            {
                removeBufferedData();
                messagesSent++;
                Serial.println("[BUFFER] Replayed offline packet.");
                feedWatchdog();
                yield();
            }
            else
            {
                break;
            }
        }
    }

    // ── Periodic Memory Log ────────────────────────────────────────────────
    static unsigned long lastHeapLog = 0;
    if (millis() - lastHeapLog >= 60000)
    {
        lastHeapLog = millis();
        Serial.print("Free Heap: ");  Serial.println(ESP.getFreeHeap());
        Serial.print("Free PSRAM: "); Serial.println(ESP.getFreePsram());
    }

    // ── 1-Second Tasks ─────────────────────────────────────────────────────

    if (millis() - lastSensorRead >= SENSOR_READ_INTERVAL_MS)
    {
        lastSensorRead = millis();

        // SHT4x — temperature and humidity
        readSHT4x();

        // INA219 — electrical measurements (battery stays NaN; no SoC model)
        readINA219();

        // MQ sensors — only after warm-up period has elapsed
        if (isMQWarmedUp())
        {
            MQReading mq3   = readMQSensor(MQ3_ANALOG_PIN);
            MQReading mq135 = readMQSensor(MQ135_ANALOG_PIN);
            MQReading mq4   = readMQSensor(MQ4_ANALOG_PIN);

            telemetry.mq3   = mq3.percentage;
            telemetry.mq135 = mq135.percentage;
            telemetry.mq4   = mq4.percentage;
        }
        // Before warm-up: leave mq3/mq135/mq4 as NaN → published as null

        // GPS — update lat/lon and timestamp from fix
        telemetry.latitude  = getGPSLatitude();
        telemetry.longitude = getGPSLongitude();

        // Timestamp: prefer GPS UTC; fall back to NTP/RTC
        String gpsTs = getGPSTimestamp();
        if (gpsTs.length() > 0)
        {
            telemetry.timestamp = gpsTs;
        }
        // If GPS time is unavailable the timestamp will be updated in the
        // next block below from NTP/RTC.
    }

    // Network status
    if (millis() - lastWiFiRead >= SENSOR_READ_INTERVAL_MS)
    {
        lastWiFiRead = millis();
        telemetry.wifi_status   = isWiFiConnected();
        telemetry.wifi_rssi_dbm = getWiFiRSSI();
    }

    // Timestamp from NTP/RTC when GPS time is not available
    if (millis() - lastTimestampRead >= SENSOR_READ_INTERVAL_MS)
    {
        lastTimestampRead = millis();
        if (!isGPSFixValid() || telemetry.timestamp.length() == 0)
        {
            telemetry.timestamp = getTimestampISO8601();
        }
    }

    // ── 15-Second Telemetry Publish ────────────────────────────────────────
    if (millis() - lastJSONPublish >= TELEMETRY_INTERVAL_MS)
    {
        lastJSONPublish = millis();

        bool shouldPublish = shouldPublishTelemetry() || shouldSendHeartbeat();

        if (shouldPublish)
        {
            String payload = buildTelemetryJSON();
            String topic   = getTelemetryTopic(telemetry.device_id);
            bool published = false;

            if (mqttStarted && isMQTTConnected())
            {
                published = publishTelemetry(topic, payload);
            }
            else
            {
                Serial.println("[LOOP] MQTT pending — buffering telemetry.");
            }

            if (!published)
            {
                addToBuffer(payload);
                Serial.println("[LOOP] Telemetry buffered.");
            }

            updateStoredSnapshot();

            Serial.println();
            Serial.println("===== TELEMETRY =====");
            Serial.println(topic);
            Serial.println(payload);
            Serial.println(published ? "MQTT Publish OK" : "MQTT Buffered (Pending Link)");
            Serial.println("=====================");
            Serial.println();
        }
    }

    feedWatchdog();
    yield();
}
