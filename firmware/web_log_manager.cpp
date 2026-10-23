/**
 * @file        web_log_manager.cpp
 * @brief       FactorySense Web Dashboard and log server implementation.
 * @project     FactorySense Industrial IoT Firmware
 * @platform    ESP32-S3 (Arduino Framework)
 * * @section Description
 * Implementation of an embedded HTTP server diagnostic layer utilizing the WebServer library.
 * Exposes real-time system diagnostic states, volatile memory availability footprints (Heap, PSRAM),
 * and structures file streaming paths to fetch or download archival log resources from the SD filesystem.
 * * @section Responsibilities
 * - Binds descriptive web resource paths directly to dedicated request handlers.
 * - Manages block-by-block context streaming for file attachments over standard socket sockets.
 * - Renders dynamic HTML content containing networking and hardware telemetry variables.
 * - Implements non-blocking client polling hooks via central system loops.
 * * @note        STRICT NON-FUNCTIONAL CLEANUP PASS. Do not alter execution flow,
 * timing sequences, or state variables.
 */

// 1. Standard C/C++ Libraries
// (None required natively in global scope)

// 2. ESP-IDF / Core Arduino Libraries
#include <WiFi.h>
#include <WebServer.h>
#include <SD.h>

// 3. Third-Party Libraries
// (Managed inside subsystem implementation layers)

// 4. Project Local Headers
#include "web_log_manager.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"
#include "buffer_manager.h"

// =====================================================
// Subsystem Instantiation
// =====================================================

WebServer server(80);

// =====================================================
// Event Logs Request Handler
// =====================================================

/**
 * @brief  Streams raw operational event text entries directly over the active network socket window.
 */
void handleEventLogs()
{
    File file = SD.open("/event_log.txt", FILE_READ);

    if (!file)
    {
        server.send(404, "text/plain", "event_log.txt not found");
        return;
    }

    server.streamFile(file, "text/plain");
    file.close();
}

// =====================================================
// Reset Logs Request Handler
// =====================================================

/**
 * @brief  Streams physical reset cause logs out to the requesting network client.
 */
void handleResetLogs()
{
    File file = SD.open("/reset_log.txt", FILE_READ);

    if (!file)
    {
        server.send(404, "text/plain", "reset_log.txt not found");
        return;
    }

    server.streamFile(file, "text/plain");
    file.close();
}

// =====================================================
// Download Event Logs
// =====================================================

/**
 * @brief  Appends file attachment metadata headers to prompt local saving of the event log file.
 */
void downloadEventLogs()
{
    File file = SD.open("/event_log.txt", FILE_READ);

    if (!file)
    {
        server.send(404, "text/plain", "event_log.txt not found");
        return;
    }

    server.sendHeader("Content-Disposition", "attachment; filename=event_log.txt");
    server.streamFile(file, "text/plain");
    file.close();
}

// =====================================================
// Download Reset Logs
// =====================================================

/**
 * @brief  Appends file attachment metadata headers to prompt local saving of the reset log file.
 */
void downloadResetLogs()
{
    File file = SD.open("/reset_log.txt", FILE_READ);

    if (!file)
    {
        server.send(404, "text/plain", "reset_log.txt not found");
        return;
    }

    server.sendHeader("Content-Disposition", "attachment; filename=reset_log.txt");
    server.streamFile(file, "text/plain");
    file.close();
}

// =====================================================
// System Status Page
// =====================================================

/**
 * @brief  Compiles network statistics and internal memory allocations into a dynamic HTML monitoring table.
 */
void handleStatus()
{
    String html;

    html += "<html><body>";
    html += "<h2>FactorySense Status</h2><hr>";

    html += "<b>WiFi:</b> ";
    html += isWiFiConnected() ? "Connected" : "Disconnected";
    html += "<br>";

    html += "<b>MQTT:</b> ";
    html += isMQTTConnected() ? "Connected" : "Disconnected";
    html += "<br>";

    html += "<b>RSSI:</b> ";
    html += String(getWiFiRSSI());
    html += " dBm<br>";

    html += "<b>Buffered Messages:</b> ";
    html += String(getBufferedCount());
    html += "<br>";

    html += "<b>Free Heap:</b> ";
    html += String(ESP.getFreeHeap());
    html += "<br>";

    html += "<b>Free PSRAM:</b> ";
    html += String(ESP.getFreePsram());
    html += "<br>";

    html += "<b>IP Address:</b> ";
    html += getWiFiIP();
    html += "<br>";

    html += "<br><a href='/'>Back</a>";
    html += "</body></html>";

    server.send(200, "text/html", html);
}

// =====================================================
// Dashboard Home Page
// =====================================================

/**
 * @brief  Serves the root web page displaying hyperlinks to administrative and diagnostic pathways.
 */
void handleRoot()
{
    String html;

    html += "<html><body>";
    html += "<h1>FactorySense Dashboard</h1>";
    html += "<hr>";

    html += "<a href='/status'>System Status</a>";
    html += "<br><br>";

    html += "<a href='/logs'>View Event Logs</a>";
    html += "<br>";

    html += "<a href='/download/events'>Download Event Logs</a>";
    html += "<br><br>";

    html += "<a href='/resets'>View Reset Logs</a>";
    html += "<br>";

    html += "<a href='/download/resets'>Download Reset Logs</a>";

    html += "</body></html>";

    server.send(200, "text/html", html);
}

// =====================================================
// Initialize Server
// =====================================================

/**
 * @brief  Maps resource strings to corresponding function endpoints and boots up local socket ears.
 */
void initWebLogServer()
{
    server.on("/", handleRoot);
    server.on("/status", handleStatus);
    server.on("/logs", handleEventLogs);
    server.on("/resets", handleResetLogs);
    server.on("/download/events", downloadEventLogs);
    server.on("/download/resets", downloadResetLogs);

    server.begin();

    Serial.println("[WEB] Log Server Started");
    Serial.print("[WEB] Browse: http://");
    Serial.println(WiFi.localIP());
}

// =====================================================
// Handle Client Requests
// =====================================================

/**
 * @brief  Polled worker handler checking server network pipelines for pending client transactions.
 */
void handleWebLogServer()
{
    server.handleClient();
}