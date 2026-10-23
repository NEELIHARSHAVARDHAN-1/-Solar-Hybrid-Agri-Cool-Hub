/**
 * @file        topic_manager.cpp
 * @brief       MQTT topic generation for AgriCoolHub.
 * @project     AgriCoolHub Solar Hybrid Agri Cool Hub
 * @platform    ESP32-S3 (Arduino Framework)
 *
 * @section TopicStructure
 *   Telemetry : agricool/v1/devices/<device_id>/telemetry
 *   Status    : agricool/v1/devices/<device_id>/status
 */

#include "topic_manager.h"

// =====================================================
// Topic Generation
// =====================================================

/**
 * @brief  Constructs the telemetry publication topic.
 * @param  deviceId  unique hardware identifier
 * @return String    e.g. "agricool/v1/devices/agrihub-001/telemetry"
 */
String getTelemetryTopic(const String& deviceId)
{
    return "agricool/v1/devices/" + deviceId + "/telemetry";
}

/**
 * @brief  Constructs the device status (LWT / birth) topic.
 * @param  deviceId  unique hardware identifier
 * @return String    e.g. "agricool/v1/devices/agrihub-001/status"
 */
String getStatusTopic(const String& deviceId)
{
    return "agricool/v1/devices/" + deviceId + "/status";
}
