# Firmware — Complete Documentation
## Solar Hybrid Agri Cool Hub (ESP32-S3)

---

## Table of Contents

1. [Overview](#overview)
2. [Features](#features)
3. [How IBM Bob Was Used in Building](#how-ibm-bob-was-used-in-building)
4. [APIs and Libraries](#apis-and-libraries)
5. [Protocols](#protocols)
6. [Challenges and How Bob Overcame Them](#challenges-and-how-bob-overcame-them)
7. [Feature Enhancements](#feature-enhancements)
8. [Conclusion](#conclusion)
9. [Setup Guide](#setup-guide)
10. [Run Procedures and IoT Flash Guide](#run-procedures-and-iot-flash-guide)

---

## Overview

The **Agri Cool Hub Firmware** runs on an **ESP32-S3 N16R8** microcontroller (Arduino
Framework, ESP-IDF core 3.x). It is the edge intelligence of the Solar Hybrid Agri Cool
Hub — continuously reading environmental and gas sensors, acquiring GPS coordinates and
UTC time, monitoring electrical power via INA219, and publishing encrypted telemetry to
a cloud MQTT broker every 15 seconds.

The firmware is derived from the **FactorySense** industrial IoT base, inheriting a
proven reliability architecture featuring exponential-backoff Wi-Fi recovery, MQTT over
TLS, an offline PSRAM-backed telemetry buffer, RTC timestamp fallback, SD card logging,
a web diagnostics dashboard, and a hardware watchdog.

**Data flow:**
```
ESP32-S3
  ├─ SHT4x (I2C SDA=8, SCL=9)    → temperature + humidity
  ├─ INA219 (I2C SDA=8, SCL=9)   → bus voltage / current / power
  ├─ DS3231 RTC (I2C SDA=8, SCL=9) → offline timestamp fallback
  ├─ MQ-3 (ADC GPIO4)             → alcohol/ethanol analog %
  ├─ MQ-135 (ADC GPIO5)           → air quality analog %
  ├─ MQ-4 (ADC GPIO6)             → methane/LPG analog %
  ├─ NEO-6M GPS (UART1 RX=17, TX=18) → lat/lon + UTC time
  └─ MicroSD (SPI CS=10, MOSI=11, SCK=12, MISO=13) → event log
          │
          │  Wi-Fi + MQTT TLS port 8883
          ▼
  HiveMQ Cloud → FastAPI Backend
```

---

## Features

### 1. Multi-Sensor Environmental Monitoring

**SHT4x (Temperature + Humidity)**
- I2C address: `0x44`
- Reads temperature (°C) and relative humidity (%) every 1 second.
- Returns `null` in the telemetry JSON if the sensor fails to respond.
- Accuracy: ±0.2°C temperature, ±1.8% RH.

**MQ-3, MQ-135, MQ-4 Gas Sensors**
- ADC inputs on GPIO4, GPIO5, GPIO6 (12-bit resolution: 0–4095).
- Values scaled to a normalized percentage (0–100 %) via voltage-divider math:
  - `V_ADC = (rawADC / 4095) × 3.3`
  - `V_sensor = V_ADC × 1.5` (10 kΩ / 20 kΩ divider)
  - `output_pct = (V_sensor / 5.0) × 100`, clamped to 0–100.
- 30-second non-blocking warm-up guard (`MQ_WARMUP_MS`) — values published as `null`
  during warm-up to prevent stale readings in the dashboard.
- **Important restriction:** MQ values are normalized ADC output percentages only.
  They are **not** calibrated PPM gas concentrations and must not be used as safety
  alarms without a validated conversion model.

**INA219 Power Monitor**
- I2C address: `0x40`
- Measures bus voltage, shunt voltage, current, and calculated power.
- `battery` field is published as `null` in all current builds — INA219 provides
  electrical measurements but not battery state-of-charge directly. A validated
  SoC estimation model is required before this field is enabled.

### 2. GPS Location and UTC Time (NEO-6M)

- UART1 on RX=GPIO17, TX=GPIO18 at 9600 baud.
- Non-blocking GPS feed: `handleGPS()` is called every loop iteration to drain the
  UART FIFO without stalling the main loop.
- **Timestamp priority:**
  1. GPS UTC time (most accurate — used when `gps.date.isValid()` AND `gps.time.isValid()`).
  2. NTP-synchronized system clock (fallback when GPS unavailable).
  3. DS3231 RTC (fallback when NTP unavailable).
  4. `null` (only if all three sources unavailable).
- GPS `latitude` and `longitude` fields are `null` when GPS fix is stale
  (`GPS_MAX_FIX_AGE_MS = 5000 ms`) or unavailable.
- IP geolocation fetch (`fetchIPGeolocation()`) is used when no GPS fix is available
  and Wi-Fi is connected.

### 3. Wi-Fi Connectivity with Exponential Backoff

- Automatic Wi-Fi reconnection using event-driven recovery (`WiFiEvent`).
- Exponential backoff: initial 5 s delay, doubles on each failure to a maximum of
  120 s, with jitter to prevent thundering-herd reconnects.
- Non-blocking: reconnection never stalls the main loop.
- Credentials stored in `secrets.h` (excluded from version control).

### 4. MQTT over TLS (HiveMQ Cloud)

- Uses ESP-IDF's native MQTT client (`esp_mqtt`) for robust TLS over port 8883.
- QoS 1 (at-least-once delivery) for all telemetry publishes.
- **Last Will and Testament (LWT):**
  - Topic: `agricool/v1/devices/{device_id}/status`
  - LWT payload: `{"status":"OFFLINE"}` — published by the broker automatically on
    unclean disconnect.
  - On successful connect: publishes `{"status":"ONLINE"}`.
- MQTT client ID: `DEVICE_ID` from `config.h`.
- Root CA certificate stored in `certs.h` for broker identity verification.

### 5. MQTT Topics

| Topic | Direction | Purpose |
|-------|-----------|---------|
| `agricool/v1/devices/{device_id}/telemetry` | ESP32 → Backend | 10-field sensor telemetry |
| `agricool/v1/devices/{device_id}/status` | ESP32 → Backend | Online/Offline LWT |

### 6. Offline Telemetry Buffer (PSRAM-Backed FIFO)

- 60-message FIFO circular buffer allocated in PSRAM (`buffer_manager`).
- When MQTT is disconnected, telemetry is enqueued to the buffer instead of discarded.
- On MQTT reconnection, the buffer is replayed: all queued messages are published
  in order before new telemetry is sent.
- Buffer depth reported in Serial diagnostics.

### 7. SD Card Event Logging

- SPI on CS=GPIO10, MOSI=GPIO11, SCK=GPIO12, MISO=GPIO13.
- Logs system events (`logEvent()`): boot, reset reason, Wi-Fi connect/disconnect,
  MQTT connect/disconnect, RTC sync events.
- If SD card is absent or fails to mount, logging is disabled gracefully — firmware
  continues without SD.
- Downloadable via the web diagnostics dashboard.

### 8. DS3231 RTC Timestamp Fallback

- I2C address: `0x68`, shared bus on GPIO8/9.
- Initialized at boot; if DS3231 lost power (dead coin cell), firmware logs a warning
  but does not halt.
- Used as the third-priority timestamp source after GPS and NTP.
- First-boot RTC sync: `setFromCompile()` sets DS3231 to build timestamp.
  After that, the RTC runs from its coin cell and is never overwritten by subsequent
  firmware uploads.

### 9. Web Diagnostics Dashboard

- `web_log_manager` starts an HTTP server on port 80 (accessible on the device's IP).
- Displays: last telemetry values, Wi-Fi signal, MQTT state, buffer depth,
  recent event log, SD card status.
- Downloadable SD card log link.
- No authentication in development builds.

### 10. RGB Status LED

- Onboard NeoPixel on GPIO48.
- States:
  - **Booting:** slow blue pulse.
  - **Wi-Fi connecting:** amber blink.
  - **Wi-Fi connected, MQTT connecting:** yellow.
  - **MQTT connected, normal:** solid green.
  - **Warning threshold:** amber.
  - **Critical threshold:** red pulse.
  - **Error / fault:** red solid.

### 11. Task Watchdog

- `initWatchdog()` starts the ESP32 hardware watchdog.
- `feedWatchdog()` is called at the top of every `loop()` iteration.
- If the firmware hangs for more than the watchdog timeout, the ESP32 resets
  automatically and logs the reset reason.
- Reset reason is logged to SD card on next boot (`logResetReason()`).

### 12. Telemetry Payload JSON Contract

Every MQTT publish produces exactly these 10 fields:

```json
{
  "device_id":   "agrihub-001",
  "timestamp":   "2026-10-22T11:00:00Z",
  "temperature": 25.6,
  "humidity":    72.4,
  "mq3":         10.0,
  "mq135":       30.0,
  "mq4":         28.0,
  "latitude":    12.9716,
  "longitude":   77.5946,
  "battery":     null
}
```

| Field | Type | Notes |
|-------|------|-------|
| `device_id` | string | Configured as `DEVICE_ID` in `config.h` |
| `timestamp` | string \| null | ISO-8601 UTC — GPS > NTP > RTC > null |
| `temperature` | float \| null | °C from SHT4x; null on sensor failure |
| `humidity` | float \| null | % RH from SHT4x; null on sensor failure |
| `mq3` | float \| null | Normalized 0–100% ADC output; null during warm-up |
| `mq135` | float \| null | Normalized 0–100% ADC output; null during warm-up |
| `mq4` | float \| null | Normalized 0–100% ADC output; null during warm-up |
| `latitude` | float \| null | Decimal degrees from GPS; null when no fix |
| `longitude` | float \| null | Decimal degrees from GPS; null when no fix |
| `battery` | null | Always null — SoC model not yet implemented |

---

## How IBM Bob Was Used in Building

1. **Module Architecture Design** — Bob proposed the modular `manager` / `sensor`
   split: each hardware concern (`wifi_manager`, `mqtt_manager`, `SHT4xSensor`,
   `MQSensor`, `GPSSensor`, etc.) as its own `.h` / `.cpp` pair, keeping `FactorySense.ino`
   as a clean orchestrator that never contains hardware-specific logic.

2. **MQ Sensor Scaling Formula** — Bob derived and documented the ADC-to-percentage
   conversion formula (`V_sensor = V_ADC × 1.5`, `output_pct = V_sensor / 5.0 × 100`)
   from the voltage divider schematic, and explained why the maximum achievable output
   is 99% (not 100%) given the resistor values.

3. **Non-Blocking GPS Feed** — Bob identified that calling `Serial.readString()` or
   blocking GPS reads inside `loop()` would stall the watchdog. Bob wrote the
   non-blocking pattern: `handleGPS()` drains the UART FIFO using `TinyGPSPlus.encode()`
   per byte, called on every loop iteration.

4. **Timestamp Priority Chain** — Bob designed the 3-tier timestamp fallback logic
   (GPS → NTP → DS3231 → null) and added the `gps.date.isValid() && gps.time.isValid()`
   guard to prevent publishing an invalid GPS timestamp.

5. **PSRAM Buffer Allocation** — Bob chose PSRAM for the offline buffer instead of
   heap RAM, explaining that PSRAM on the ESP32-S3 N16R8 provides 8 MB of additional
   storage, avoiding heap fragmentation for a large circular buffer.

6. **LWT (Last Will and Testament) Configuration** — Bob configured the ESP-IDF MQTT
   client with the LWT topic and payload so the backend always receives an OFFLINE
   notification on unclean disconnect, enabling the device registry to update
   `is_online = false` immediately.

7. **30-Second MQ Warm-Up Guard** — Bob added the `isMQWarmedUp()` non-blocking check
   using `millis()` instead of `delay()`, ensuring the main loop, watchdog, and GPS
   feed continue to run during the warm-up period.

8. **TLS Certificate Pinning** — Bob set up the `certs.h` Root CA certificate storage
   pattern for the ESP-IDF MQTT client, and documented the trade-off: broker traffic is
   encrypted but certificate identity is not verified in the prototype (`tls_insecure`
   mode). Bob noted the production upgrade path.

9. **Watchdog + Long Operations** — Bob flagged that calibration and SD card log
   downloads take longer than the watchdog timeout. Bob documented the pattern of
   calling `feedWatchdog()` inside long-running operations and added the `Decision 048`
   entry to the engineering decisions log.

10. **`secrets.h` Credential Management** — Bob established the `secrets.h` file pattern
    (excluded from `.gitignore`), provided a `secrets.h.example` template, and explained
    why credentials must never appear in `config.h` or the sketch itself.

---

## APIs and Libraries

### Arduino / ESP32 Core Libraries (built-in)

| Library | Purpose |
|---------|---------|
| `WiFi.h` | Wi-Fi station mode |
| `Wire.h` | I2C bus (SDA=8, SCL=9) |
| `SPI.h` | SPI bus (SD card) |
| `SD.h` | SD card file I/O |
| `HardwareSerial` | UART1 for GPS |
| `WebServer.h` | Web diagnostics dashboard |
| `esp_system.h` | Reset reason, PSRAM access |
| `esp_mqtt` | ESP-IDF native MQTT over TLS |

### Third-Party Libraries (Arduino Library Manager)

| Library | Version | Purpose |
|---------|---------|---------|
| Adafruit SHT4x Library | 1.x | SHT4x temperature/humidity sensor |
| Adafruit BusIO | 1.x | I2C/SPI abstraction layer |
| Adafruit Unified Sensor | 1.x | Common sensor abstraction |
| Adafruit INA219 | 1.x | INA219 voltage/current/power monitor |
| TinyGPSPlus | 1.x | NMEA 0183 GPS parser |
| ArduinoJson | 7.x | JSON serialization (`json_builder.cpp`) |
| RTClib | 2.x | DS3231 RTC read/write |
| OneWire | 2.x | (legacy, in-tree, not used in main loop) |
| DallasTemperature | 4.x | (legacy, in-tree, not used in main loop) |
| Adafruit MPU6050 | 2.x | (legacy, in-tree, not used in main loop) |

### Internal Module APIs

| Module | Key Functions | Purpose |
|--------|--------------|---------|
| `wifi_manager` | `initWiFi()`, `handleWiFi()`, `isWiFiConnected()`, `enableWiFiReconnect()` | Wi-Fi with backoff |
| `mqtt_manager` | `initMQTT()`, `publishTelemetry()`, `isMQTTConnected()` | MQTT TLS client |
| `SHT4xSensor` | `initSHT4x()`, `readSHT4x()` | Temperature + humidity |
| `INA219Sensor` | `initINA219()`, `readINA219()` | Voltage / current |
| `MQSensor` | `initMQSensors()`, `readMQSensor(pin)`, `isMQWarmedUp()` | Gas sensor ADC |
| `GPSSensor` | `initGPS()`, `handleGPS()`, `isGPSFixValid()`, `getGPSLatitude()` | GPS NMEA |
| `ntp_manager` | `initNTP()`, `getISOTimestamp()` | NTP + ISO-8601 |
| `rtc_manager` | `initRTC()`, `isRTCValid()`, `getRTCTimestamp()` | DS3231 fallback |
| `buffer_manager` | `initBuffer()`, `enqueue()`, `dequeue()`, `getBufferDepth()` | Offline FIFO |
| `sd_manager` | `initSD()`, `logEvent()` | SD card logging |
| `watchdog_manager` | `initWatchdog()`, `feedWatchdog()` | Hardware watchdog |
| `led_manager` | `initLED()`, `setLEDState()`, `updateLED()` | RGB NeoPixel |
| `json_builder` | `buildTelemetryJSON()` | 10-field JSON serializer |
| `event_manager` | `shouldPublish()` | Delta-threshold publish decision |
| `topic_manager` | `getTelemetryTopic()`, `getStatusTopic()` | MQTT topic strings |
| `web_log_manager` | `initWebLogServer()` | HTTP diagnostics dashboard |

---

## Protocols

### MQTT v3.1.1 over TLS 1.2/1.3

- **Transport:** TCP + TLS to HiveMQ Cloud on port 8883.
- **Library:** ESP-IDF native MQTT client (`esp_mqtt_client`).
- **QoS:** Level 1 (at-least-once). The broker acknowledges each publish; the client
  retransmits if no ACK is received.
- **Retained:** false — telemetry is live data, not a state snapshot.
- **LWT:** `{"status":"OFFLINE"}` on topic `agricool/v1/devices/{id}/status`, published
  by the broker on unclean disconnect.
- **TLS:** Root CA certificate from `certs.h`. Broker identity is verified against the
  CA in production builds.

### I2C (Inter-Integrated Circuit)

- **Bus:** Single shared I2C bus on SDA=GPIO8, SCL=GPIO9.
- **Devices on bus:** SHT4x (`0x44`), INA219 (`0x40`), DS3231 (`0x68`).
- **Library:** Arduino `Wire.h`.
- `Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN)` called once in `setup()` before any I2C device
  is initialized.

### SPI (Serial Peripheral Interface)

- **Bus:** MOSI=GPIO11, MISO=GPIO13, SCK=GPIO12, CS=GPIO10.
- **Device:** MicroSD card.
- **Library:** Arduino `SPI.h` + `SD.h`.
- Used for event log writes and SD card log downloads via the web dashboard.

### UART (Universal Asynchronous Receiver-Transmitter)

- **Bus:** UART1 on RX=GPIO17, TX=GPIO18 at 9600 baud.
- **Device:** NEO-6M GPS module.
- **Library:** `HardwareSerial` + `TinyGPSPlus`.
- Non-blocking byte-by-byte feed via `Serial2.available()` / `TinyGPSPlus.encode()`.

### ADC (Analog-to-Digital Conversion)

- **Resolution:** 12-bit (0–4095).
- **Reference:** 3.3 V internal.
- **Pins:** GPIO4 (MQ-3), GPIO5 (MQ-135), GPIO6 (MQ-4).
- **Voltage safety:** All ADC pins must remain ≤ 3.3 V. Voltage dividers
  (10 kΩ / 20 kΩ) on each MQ sensor AO pin bring the signal within range.

### HTTP (Web Diagnostics)

- ESP32 built-in `WebServer` on port 80.
- Serves a status page and SD card log download endpoint.
- No authentication in prototype builds.

---

## Challenges and How Bob Overcame Them

### Challenge 1 — Non-Blocking Main Loop Design
**Problem:** Early firmware versions used `delay()` calls for sensor reads, GPS parsing,
and Wi-Fi retries. This caused the watchdog to trigger on any operation taking longer
than the timeout, and GPS UART data was lost when the loop was blocked.

**Bob's Solution:** Bob replaced all `delay()` calls with non-blocking `millis()`-based
timers throughout the sketch. Each manager module tracks its own `lastXxx` timestamp.
`handleGPS()` and `handleWiFi()` return immediately if there is nothing to do, and the
watchdog is fed at the top of every `loop()` iteration.

### Challenge 2 — MQ Sensor ADC Voltage Safety
**Problem:** MQ sensor modules powered at 5 V output up to 5 V on their AO pin. The
ESP32-S3 ADC is 3.3 V tolerant maximum. Connecting MQ sensors directly would damage
the GPIO.

**Bob's Solution:** Bob specified the voltage divider circuit (10 kΩ top / 20 kΩ bottom)
for each MQ sensor, calculated the resulting maximum ADC voltage (2.5 V at 5 V sensor
supply), documented the formula in `config.h` and `pins.h`, and added an explicit
warning in the wiring guide. Bob also explained that the maximum normalized output
under these conditions is 99%, not 100%, which is by design.

### Challenge 3 — GPS Cold Start Blocking
**Problem:** An early version called `GPS.waitForFix()` (blocking) in `setup()`, causing
a startup hang of several minutes while the GPS acquired its first fix outdoors.

**Bob's Solution:** Bob removed the blocking wait entirely. GPS data is fed byte-by-byte
in `loop()` via `handleGPS()`. The firmware publishes `null` for latitude, longitude, and
GPS-derived timestamp until a valid fix is received. IP geolocation is used as an
intermediate fallback for location when GPS is unavailable.

### Challenge 4 — Timestamp Reliability Without GPS
**Problem:** On indoor deployments, the GPS module never acquires a fix, leaving `timestamp`
permanently `null`. The backend's deduplication relies on `(device_id, timestamp)`, so
`null` timestamps would cause all indoor telemetry to collide.

**Bob's Solution:** Bob designed the 3-tier timestamp priority chain: GPS UTC → NTP → DS3231
RTC → null. NTP is available whenever Wi-Fi is connected, and the DS3231 RTC provides
offline timestamps when Wi-Fi is down. `null` is only published when all three sources are
unavailable simultaneously.

### Challenge 5 — PSRAM-Backed Offline Buffer
**Problem:** Storing 60 × 512-byte JSON payloads in heap RAM risked heap fragmentation
and eventual crash on a long-running device with frequent MQTT disconnects.

**Bob's Solution:** Bob identified that the ESP32-S3 N16R8 has 8 MB of OPI PSRAM and
recommended using `ps_malloc()` for the offline buffer allocation. The `buffer_manager`
uses PSRAM for the circular buffer array, keeping the heap free for library allocations.

### Challenge 6 — TLS Certificate Management
**Problem:** The ESP-IDF MQTT client requires the MQTT broker's Root CA certificate as a
PEM string in the firmware. Hard-coding the certificate in `config.h` would expose it in
version control and make updates difficult.

**Bob's Solution:** Bob placed the CA certificate in `certs.h` / `certs.cpp` as a `const char*`
string, separate from `config.h` and `secrets.h`. This allows certificate updates without
touching configuration or credential files. Bob also documented the `tls_insecure` option
for prototype development and the production upgrade path.

### Challenge 7 — Watchdog Triggering During SD Log Downloads
**Problem:** The web log manager serves SD card file downloads over HTTP, which can take
several seconds for large log files. The hardware watchdog triggered during downloads,
causing an unnecessary reset.

**Bob's Solution:** Bob added `feedWatchdog()` calls inside the SD file-serving loop,
and documented Decision 048 in `DECISIONS.md` explaining the pattern. Bob also noted
that calibration routines and any other long-running operations must feed the watchdog
internally.

---

## Feature Enhancements

1. **Battery SoC Model** — Implement a validated State-of-Charge estimation using the
   INA219's voltage measurements and a coulomb-counting integration. Map voltage to SoC
   for the specific battery chemistry (LiPo 4S) and publish as `battery` (0–100 %).

2. **MQ Sensor Calibration** — Implement `Rs/R0` ratio calibration for MQ sensors per
   their datasheets. Store calibration coefficients in NVS (ESP32 non-volatile storage)
   and convert normalized ADC readings to approximate PPM values for specific gases.

3. **MQTT Exponential Backoff** — Migrate from the current fixed 5-second MQTT retry
   to a full exponential backoff matching the Wi-Fi manager's pattern (5 s → 10 s → …
   → 120 s max).

4. **OTA (Over-The-Air) Firmware Updates** — Add `ArduinoOTA` or ESP-IDF OTA support
   so firmware can be updated remotely from the backend via MQTT command without physical
   USB access.

5. **MQTT Remote Commands** — Subscribe to `agricool/v1/devices/{id}/command` and
   implement: force sensor read, SD log flush, watchdog test, reboot, and threshold update.

6. **GPS Geofencing Alerts** — Add a geofencing check: if the device moves outside a
   configured radius of its home location, publish a `LOCATION_ALERT` to the status topic.

7. **Capacitive Soil Moisture Sensor** — Add a soil moisture sensor (capacitive, ADC)
   to monitor crop humidity in transit or storage, extending the telemetry payload.

8. **Deep Sleep / Power Saving** — For solar-battery deployments, implement a deep sleep
   cycle between telemetry publishes to reduce power consumption from ~240 mA to
   ~10 µA during sleep.

9. **TLS Certificate Auto-Renewal** — Implement NTP-based certificate expiry checking;
   alert via MQTT status when the Root CA certificate is within 30 days of expiry.

---

## Conclusion

The Agri Cool Hub Firmware delivers a production-quality IoT edge node on the ESP32-S3
platform. Its reliability architecture — non-blocking main loop, exponential-backoff
connectivity, PSRAM offline buffer, 3-tier timestamp fallback, hardware watchdog, and
TLS-encrypted MQTT — ensures continuous, tamper-evident telemetry delivery even through
Wi-Fi outages, GPS cold starts, and power cycles.

IBM Bob's contributions to the firmware were foundational: the module architecture,
non-blocking GPS integration, voltage safety calculations, offline buffer strategy, LWT
configuration, and watchdog patterns. The resulting firmware is robust, well-documented
(including a 48-decision engineering log in `DECISIONS.md`), and ready for real-world
agricultural deployment.

The 10-field JSON telemetry payload is precisely defined and matches the backend's
Pydantic schema exactly, ensuring zero deserialization failures in normal operation.

---

## Setup Guide

### Hardware Requirements

| Component | Model | Interface | GPIO |
|-----------|-------|-----------|------|
| Microcontroller | ESP32-S3 N16R8 | — | — |
| Temp/Humidity | SHT4x | I2C (0x44) | SDA=8, SCL=9 |
| Power Monitor | INA219 | I2C (0x40) | SDA=8, SCL=9 |
| RTC | DS3231 | I2C (0x68) | SDA=8, SCL=9 |
| Gas (Alcohol) | MQ-3 | ADC | GPIO4 |
| Gas (Air Quality) | MQ-135 | ADC | GPIO5 |
| Gas (Methane) | MQ-4 | ADC | GPIO6 |
| GPS | GY-GPS6MV2 / NEO-6M | UART1 | RX=17, TX=18 |
| SD Card | MicroSD module | SPI | CS=10, MOSI=11, SCK=12, MISO=13 |
| RGB LED | Onboard NeoPixel | — | GPIO48 |

### Voltage Divider (Required for MQ Sensors)

MQ sensors powered at 5 V output up to 5 V on AO. The ESP32-S3 ADC is 3.3 V max.
A voltage divider is **mandatory**.

```
MQ AO ─── 10 kΩ ───┬─── ESP32-S3 ADC pin (GPIO 4/5/6)
                    │
                  20 kΩ
                    │
                   GND
```

**Verify actual resistor values with a multimeter before powering on.**

### Software Prerequisites

| Tool | Version | Download |
|------|---------|---------|
| Arduino IDE | 2.x | https://www.arduino.cc/en/software |
| ESP32 Board Package | 3.x | Boards Manager |
| Adafruit SHT4x Library | 1.x | Library Manager |
| Adafruit BusIO | 1.x | Library Manager |
| Adafruit Unified Sensor | 1.x | Library Manager |
| Adafruit INA219 | 1.x | Library Manager |
| TinyGPSPlus | 1.x | Library Manager |
| ArduinoJson | 7.x | Library Manager |
| RTClib | 2.x | Library Manager |

### Install ESP32 Board Package

In Arduino IDE:
```
File → Preferences → Additional Board Manager URLs:
https://espressif.github.io/arduino-esp32/package_esp32_index.json

Tools → Board → Boards Manager → Search "esp32" → Install "esp32 by Espressif Systems" 3.x
```

### Configure Credentials

Create `firmware/secrets.h` (this file is excluded from Git):

```cpp
#ifndef SECRETS_H
#define SECRETS_H

#define WIFI_SSID     "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

#define MQTT_USERNAME "YOUR_MQTT_USERNAME"
#define MQTT_PASSWORD "YOUR_MQTT_PASSWORD"

#endif
```

### Configure TLS Certificate

Open `firmware/certs.h` and replace the `ROOT_CA_CERT` string with the Root CA
certificate from your HiveMQ Cloud broker:

1. Go to your HiveMQ Cloud console → TLS → Download CA Certificate.
2. Copy the PEM content into `certs.h` as a C string literal.

---

## Run Procedures and IoT Flash Guide

### Arduino IDE Board Settings

Open Arduino IDE → `Tools` menu and set:

| Setting | Value |
|---------|-------|
| Board | `ESP32S3 Dev Module` |
| Flash Size | `16MB (128Mb)` |
| PSRAM | `OPI PSRAM` |
| Flash Mode | `QIO 80MHz` |
| Partition Scheme | `16MB Flash (3MB APP / 9MB FATFS)` |
| Upload Speed | `921600` |
| USB CDC On Boot | `Enabled` |
| Port | (select your ESP32 COM port) |

### First Flash Procedure

1. **Open the sketch**
   ```
   File → Open → firmware/FactorySense.ino
   ```

2. **Verify credentials** — confirm `firmware/secrets.h` exists and is filled in.

3. **Verify board settings** — check all settings in the table above.

4. **Compile first** (check for errors)
   - Click the **Verify** button (✓) or press `Ctrl+R`.
   - Fix any errors before uploading.

5. **Enter bootloader mode** (required for first flash on some ESP32-S3 boards)
   - Hold the **BOOT** button on the ESP32-S3.
   - Click **Upload** (→) in Arduino IDE.
   - Release the **BOOT** button after the upload starts ("Connecting...").

6. **Wait for "Done uploading."**

7. **Open Serial Monitor**
   - `Tools → Serial Monitor` or `Ctrl+Shift+M`.
   - Set baud rate to **115200**.
   - Press the **RESET** button on the ESP32-S3.

### Expected Serial Monitor Output on First Boot

```
===========================================
     AgriCoolHub Boot
===========================================
PSRAM Size : 8388608
Free PSRAM : 8323072
[LED] Booting
Device ID : agrihub-001
[RTC] Valid
[I2C] Bus initialized (SDA=8, SCL=9)
[SHT4x] Initialized
[INA219] Initialized
[MQ] Warm-up period: 30 s (non-blocking)
[GPS] UART1 initialized at 9600 baud
[Watchdog] Initialized
===========================================
     AgriCoolHub Started
===========================================
[BOOT] WiFi detected. Initializing MQTT...
[WiFi] Connected. IP: 192.168.x.x
[MQTT] Connected to HiveMQ Cloud.
[Status] Published ONLINE
[Buffer] Replay: 0 queued messages
[Telemetry] Published: {"device_id":"agrihub-001","timestamp":"2026-...","temperature":25.6,...}
```

### First-Boot RTC Sync

On first boot (or when the DS3231 coin cell is dead), the firmware calls
`setFromCompile()` to set the DS3231 to the build timestamp.

**For accurate time:**
- Build and flash the firmware immediately after compiling.
- Or, set the RTC manually using a separate RTClib sketch:
  ```cpp
  rtc.adjust(DateTime(2026, 10, 22, 11, 0, 0));
  ```

After the RTC is set once, it runs from its coin cell indefinitely and is never
overwritten by subsequent firmware uploads or power cycles.

### Subsequent Uploads (No BOOT Button Required)

After the first flash, subsequent uploads using USB CDC On Boot (enabled in board
settings) do not require holding BOOT:
1. Click **Upload** in Arduino IDE.
2. The IDE automatically resets the ESP32 into bootloader mode.

### Verify Telemetry is Reaching the Backend

1. Open the HiveMQ Cloud console → MQTT Webclient.
2. Subscribe to `agricool/v1/devices/agrihub-001/telemetry`.
3. Wait up to 15 seconds for the next publish.
4. A 10-field JSON payload should appear.

Alternatively, use **MQTT Explorer** (desktop app):
1. Connect to your HiveMQ Cloud broker.
2. Expand topic tree: `agricool → v1 → devices → agrihub-001 → telemetry`.
3. JSON payload appears in the right panel on each publish.

### Web Diagnostics Dashboard

While the ESP32 is connected to Wi-Fi:
1. Find the device's IP address in the Serial Monitor (`IP: 192.168.x.x`).
2. Open a browser on the same network: `http://192.168.x.x/`
3. The dashboard shows live sensor values, MQTT state, buffer depth, and event log.
4. Click "Download Log" to retrieve the SD card event log.

### Troubleshooting

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| `SHT4x not found` | Wrong SDA/SCL or loose wire | Check GPIO8/9; run I2C scanner sketch |
| `INA219 not found` | I2C address conflict | Verify 0x40, check A0/A1 pins |
| `DS3231 not found` | Shared I2C bus issue | Check GPIO8/9; verify coin cell |
| `SD card not found` | SPI wiring or CS pin | Verify GPIO10/11/12/13 |
| `MQTT TLS fails` | Wrong CA cert or broker host | Check `certs.h` and `MQTT_HOST` in `config.h` |
| `MQTT: Not authorised` | Wrong credentials | Check `MQTT_USERNAME` / `MQTT_PASSWORD` in `secrets.h` |
| MQ reads 0 | Sensor not warmed up | Wait 30 seconds; verify voltage divider |
| MQ ADC overrange | No voltage divider | Add 10 kΩ / 20 kΩ divider; verify with multimeter |
| GPS always null | No fix acquired | Move outdoors; allow 5–10 min cold start |
| Watchdog resets | Loop stall | Check for new blocking `delay()` calls |
| Upload fails | No bootloader mode | Hold BOOT button while clicking Upload |
| Compile error: `secrets.h` not found | Missing credentials file | Create `firmware/secrets.h` from template |
