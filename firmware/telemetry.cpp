/**
 * @file        telemetry.cpp
 * @brief       Global telemetry data instance — AgriCoolHub.
 * @project     AgriCoolHub Solar Hybrid Agri Cool Hub
 * @platform    ESP32-S3 (Arduino Framework)
 */

#include "telemetry.h"
#include <math.h>

// =====================================================
// Global Telemetry Instance
// All float fields initialized to NaN so json_builder
// correctly serializes absent readings as JSON null.
// =====================================================

TelemetryData telemetry = {
    /* device_id  */ "",
    /* timestamp  */ "",
    /* temperature*/ NAN,
    /* humidity   */ NAN,
    /* mq3        */ NAN,
    /* mq135      */ NAN,
    /* mq4        */ NAN,
    /* latitude   */ NAN,
    /* longitude  */ NAN,
    /* battery    */ NAN,
    /* wifi_status*/ false,
    /* wifi_rssi  */ 0
};
