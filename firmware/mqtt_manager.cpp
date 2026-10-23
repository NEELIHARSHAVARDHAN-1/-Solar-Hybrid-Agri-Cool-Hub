/**
 * @file        mqtt_manager.cpp
 * @brief       MQTT client management and messaging logic.
 * @project     FactorySense Industrial IoT Firmware
 * @platform    ESP32-S3 (ESP-IDF / Arduino Hybrid)
 * * @section Description
 * Implementation of an asynchronous MQTT client abstraction around the ESP-IDF MQTT core. 
 * Manages connection lifecycle events, handles state-based automatic reconnect sequences 
 * without blocking system execution, registers Last Will and Testament (LWT) payloads, 
 * and routes persistent QoS 1 messages.
 * * @section Responsibilities
 * - Manages low-level configurations (credentials, keep-alive frames, root certificates).
 * - Tracks runtime state changes via an event dispatcher (`mqttEventHandler`).
 * - Handles timed background reconnect polling using non-blocking interval offsets.
 * - Publishes telemetry payloads with strict QoS 1 delivery constraints.
 * * @note        STRICT NON-FUNCTIONAL CLEANUP PASS. Do not alter execution flow,
 * timing sequences, or state variables.
 */

// 1. Standard C/C++ Libraries
// (None required natively in global scope)

// 2. ESP-IDF / Core Arduino Libraries
#include <WiFi.h>
#include <mqtt_client.h>
#include <esp_event.h>
#include <esp_crt_bundle.h>   // ESP32 built-in trusted CA bundle (includes ISRG Root X1)

// 3. Third-Party Libraries
// (Managed inside subsystem implementation layers)

// 4. Project Local Headers
#include "mqtt_manager.h"
// certs.h not included — certificate verification is skipped (prototype mode)
#include "wifi_manager.h"
#include "config.h"
#include "device_manager.h"
#include "topic_manager.h"
#include "watchdog_manager.h"
#include "led_manager.h"
#include "sd_manager.h"

// =====================================================
// MQTT Globals
// =====================================================

static esp_mqtt_client_handle_t mqttClient = nullptr;

static bool mqttConnected  = false;
static bool mqttStarted    = false;
static bool pendingOnlineStatus = false;

static unsigned long lastMQTTReconnect = 0;
static const unsigned long MQTT_RECONNECT_INTERVAL_MS = 10000; // 10 s — give DNS time to settle

// Track whether the client needs a stop before re-start
static bool mqttNeedsStop = false;

// Persistent storage for LWT topic
static String gStatusTopic;

// =====================================================
// Forward Declarations
// =====================================================

static void mqttEventHandler(
    void *handler_args,
    esp_event_base_t base,
    int32_t event_id,
    void *event_data);

static void reconnectMQTT();

bool publishStatus(
    const String& topic,
    const String& payload);

bool publishTelemetry(
    const String& topic,
    const String& payload);

// =====================================================
// MQTT Initialization
// =====================================================

/**
 * @brief  Initializes the native MQTT subsystem and hooks the execution event handler loop.
 */
void initMQTT()
{
    esp_mqtt_client_config_t mqtt_cfg = {};

    // ---------------------------------
    // Broker Configuration — TLS with built-in CA bundle
    // esp_crt_bundle_attach provides the ESP32's compiled-in Mozilla CA
    // root store, which includes ISRG Root X1 (Let's Encrypt / HiveMQ).
    // No external certs.h needed. Full certificate chain verified.
    // ---------------------------------
    mqtt_cfg.broker.address.uri                              = MQTT_BROKER_URI;
    mqtt_cfg.credentials.username                            = MQTT_USERNAME;
    mqtt_cfg.credentials.authentication.password             = MQTT_PASSWORD;
    mqtt_cfg.broker.verification.crt_bundle_attach           = esp_crt_bundle_attach;
    mqtt_cfg.broker.verification.skip_cert_common_name_check = false;

    // ---------------------------------
    // Status Topic
    // ---------------------------------
    //gStatusTopic = getStatusTopic(getDeviceID());
    gStatusTopic = getStatusTopic(String(DEVICE_ID));
    // ---------------------------------
    // Keep Alive
    // ---------------------------------
    mqtt_cfg.session.keepalive = 15;

    // ---------------------------------
    // Last Will & Testament (LWT)
    // ---------------------------------
    mqtt_cfg.session.last_will.topic = gStatusTopic.c_str();
    mqtt_cfg.session.last_will.msg = "{\"status\":\"OFFLINE\"}";
    mqtt_cfg.session.last_will.msg_len = strlen("{\"status\":\"OFFLINE\"}");
    mqtt_cfg.session.last_will.qos = 1;
    mqtt_cfg.session.last_will.retain = true;

    // ---------------------------------
    // Client Initialization
    // ---------------------------------
    mqttClient = esp_mqtt_client_init(&mqtt_cfg);

    esp_mqtt_client_register_event(
        mqttClient,
        MQTT_EVENT_ANY,
        mqttEventHandler,
        nullptr);

    Serial.println("[MQTT] Initialized");
    Serial.print("[MQTT] Status Topic: ");
    Serial.println(gStatusTopic);
}

// =====================================================
// MQTT Event Handler
// =====================================================

/**
 * @brief  Asynchronous network event thread callback managed by the ESP-IDF event loop framework.
 * @param  handler_args User data context pointer mapped to registration.
 * @param  base Event base identifier object.
 * @param  event_id Internal status state constant mapped out by the driver.
 * @param  event_data System context variables or structures bound to this event instance.
 */
static void mqttEventHandler(
    void *handler_args,
    esp_event_base_t base,
    int32_t event_id,
    void *event_data)
{
    switch ((esp_mqtt_event_id_t)event_id)
    {
        case MQTT_EVENT_CONNECTED:
            mqttConnected   = true;
            mqttNeedsStop   = false;
            Serial.println("[MQTT] Connected");
            logEvent("MQTT_CONNECTED");
            pendingOnlineStatus = true;
            setLEDState(LED_OFF);
            break;

        case MQTT_EVENT_DISCONNECTED:
            mqttConnected = false;
            mqttStarted   = false;
            mqttNeedsStop = true;   // must stop before re-starting
            Serial.println("[MQTT] Disconnected");
            logEvent("MQTT_DISCONNECTED");

            if (isWiFiConnected())
            {
                setLEDState(LED_MQTT_LOST);
            }
            break;

        default:
            break;
    }
}

// =====================================================
// MQTT Reconnection Logic
// =====================================================

/**
 * @brief  Evaluates network connection contexts and initiates asynchronous reconnect requests if needed.
 */
static void reconnectMQTT()
{
    if (!isWiFiConnected() || mqttConnected)
    {
        return;
    }

    if ((millis() - lastMQTTReconnect) < MQTT_RECONNECT_INTERVAL_MS)
    {
        return;
    }

    lastMQTTReconnect = millis();

    // Stop the previous client session before attempting a new one.
    // This is required when the client disconnected abnormally (DNS fail,
    // TLS error, etc.) — calling esp_mqtt_client_start() on an already-started
    // handle without a prior stop causes repeated DNS/transport errors.
    if (mqttClient && mqttNeedsStop)
    {
        esp_mqtt_client_stop(mqttClient);
        mqttNeedsStop = false;
        Serial.println("[MQTT] Client stopped (preparing reconnect)");
        // Return here — let the next interval trigger the actual start,
        // giving the stack a moment to clean up.
        return;
    }

    // Verify DNS is ready before attempting TLS connect.
    // getaddrinfo error 202 means the DNS server is not yet reachable —
    // this happens in the first few seconds after WiFi gets an IP.
    IPAddress resolvedIP;
    if (WiFi.hostByName(MQTT_HOST, resolvedIP) != 1)
    {
        Serial.println("[MQTT] DNS not ready — deferring connect");
        return;
    }

    Serial.println("[MQTT] Attempting reconnect...");
    setLEDState(LED_MQTT_CONNECTING);

    if (mqttClient && !mqttStarted)
    {
        esp_mqtt_client_start(mqttClient);
        mqttStarted = true;
        Serial.println("[MQTT] Client start called");
    }
}

// =====================================================
// MQTT Maintenance Handler
// =====================================================

/**
 * @brief  Cyclic scheduling loop managing state machine checks, connection retries, and vital heartbeat assertions.
 */
void handleMQTT()
{
    reconnectMQTT();

    // Publish online status upon successful connection
    if (pendingOnlineStatus && mqttConnected)
    {
        publishStatus(gStatusTopic, "{\"status\":\"ONLINE\"}");
        pendingOnlineStatus = false;
    }

    feedWatchdog();
}

// =====================================================
// Connection Status
// =====================================================

/**
 * @brief  Checks the current connection state of the MQTT engine.
 * @return true if the broker connection is verified active, false otherwise.
 */
bool isMQTTConnected()
{
    return mqttConnected;
}

// =====================================================
// Publish System Status
// =====================================================

/**
 * @brief  Publishes a retained LWT status notification payload to the network broker.
 * @param  topic Reference destination endpoint path string.
 * @param  payload Data structure contents mapped to outbound arrays.
 * @return true if the packet transmission successfully completed, false on network faults.
 */
bool publishStatus(const String& topic, const String& payload)
{
    if (!mqttConnected)
    {
        return false;
    }

    int msgId = esp_mqtt_client_publish(
        mqttClient,
        topic.c_str(),
        payload.c_str(),
        0,
        1,  // QoS 1
        1   // Retained
    );

    return (msgId >= 0);
}

// =====================================================
// Publish Telemetry Data
// =====================================================

/**
 * @brief  Transmits an un-retained standard QoS 1 transactional data frame out to network brokers.
 * @param  topic Reference destination endpoint path string.
 * @param  payload JSON formatted telemetry packet contents.
 * @return true if delivery receipt successfully completed, false otherwise.
 */
bool publishTelemetry(const String& topic, const String& payload)
{
    if (!mqttConnected)
    {
        return false;
    }

    int msgId = esp_mqtt_client_publish(
        mqttClient,
        topic.c_str(),
        payload.c_str(),
        0,
        1, // QoS 1
        0  // Not retained
    );

    if (msgId >= 0)
    {
        setLEDState(LED_PUBLISH_SUCCESS);
    }
    else
    {
        setLEDState(LED_BUFFERED);
        logEvent("MQTT_PUBLISH_FAILED");
    }

    return (msgId >= 0);
}