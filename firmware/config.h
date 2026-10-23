/**
 * @file        config.h
 * @brief       Global system configuration parameters and endpoint variables.
 * @project     AgriCoolHub Solar Hybrid Agri Cool Hub
 * @platform    ESP32-S3 (Arduino Framework)
 *
 * @section Description
 * Master static configuration file housing operational timing windows,
 * device identity, and encrypted broker endpoint configurations.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include "secrets.h"

// =====================================================
// Device Identity
// =====================================================

// Stable device identifier published in every telemetry packet.
// Override this per-device deployment if needed.
#define DEVICE_ID "agrihub-001"

// =====================================================
// Telemetry Timing
// =====================================================

// Main telemetry publish interval: 15 seconds
#define TELEMETRY_INTERVAL_MS    15000

// Sensor read interval: 1 second (independent of publish interval)
#define SENSOR_READ_INTERVAL_MS  1000

// =====================================================
// MQ Sensor ADC Scaling Constants
// Circuit assumption: 10 kΩ top resistor, 20 kΩ bottom resistor.
// V_ADC = V_sensor * (20 / (10 + 20)) → V_sensor = V_ADC * 1.5
// VERIFY actual resistor values before use.
// =====================================================
#define ADC_MAX_READING       4095.0f
#define ESP32_REF_VOLTAGE     3.3f
#define VOLTAGE_DIVIDER_RATIO 1.5f
#define SENSOR_FULL_SCALE_V   5.0f

// =====================================================
// GPS
// =====================================================
#define GPS_BAUD_RATE    9600
// Maximum age (ms) of a GPS fix before it is considered stale
#define GPS_MAX_FIX_AGE_MS 5000

// =====================================================
// MQ Sensor Warm-Up
// A brief initial delay before readings are used.
// This is NOT a calibration guarantee; follow sensor datasheet
// and characterize warm-up experimentally.
// =====================================================
#define MQ_WARMUP_MS 30000UL

// =====================================================
// Serial Diagnostics
// Set to 1 to enable verbose MQ sensor debug output.
// Set to 0 to silence in production.
// =====================================================
#define MQ_DEBUG_SERIAL 1

// =====================================================
// MQTT Broker — Private HiveMQ Cloud
// TLS encrypted on port 8883, certificate verification SKIPPED.
// Traffic is encrypted; broker identity is not verified.
// Acceptable for prototype. Enable cert verification for production.
// =====================================================
#define MQTT_HOST       "998559fd5537417097b01fa008399933.s1.eu.hivemq.cloud"
#define MQTT_PORT       8883
#define MQTT_BROKER_URI "mqtts://998559fd5537417097b01fa008399933.s1.eu.hivemq.cloud:8883"

// Plain fallback (no TLS) — uncomment if needed:
// #define MQTT_HOST       "broker.hivemq.com"
// #define MQTT_PORT       1883
// #define MQTT_BROKER_URI "mqtt://broker.hivemq.com:1883"

#endif // CONFIG_H
