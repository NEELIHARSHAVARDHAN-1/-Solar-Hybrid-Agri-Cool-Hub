# AgriCoolHub — Solar Hybrid Smart Crop Preservation & Transport Monitoring

## Overview

AgriCoolHub is an AI-enabled, solar-powered smart crop preservation and transportation monitoring system built on the ESP32-S3 platform.

The firmware continuously monitors environmental and gas-sensor parameters, acquires GPS location and UTC time, measures electrical quantities with an INA219, and publishes telemetry securely to a cloud MQTT broker every 15 seconds.

The firmware is derived from the FactorySense industrial IoT base and inherits its proven reliability architecture:

- WiFi Auto Recovery with Exponential Backoff
- MQTT over TLS (ESP-IDF native client, HiveMQ Cloud, QoS 1)
- Offline FIFO Telemetry Buffer (60 messages, PSRAM-backed)
- Buffered Telemetry Replay After Reconnection
- NTP + DS3231 RTC Fallback Timestamping
- SD Card Event Logging
- Web Diagnostics Dashboard
- Task Watchdog
- RGB Status LED

---

## Quick Start

### 1. Clone Repository

```bash
git clone <repo-url>
cd factorysense-firmware
```

### 2. Install Arduino IDE 2.x

https://www.arduino.cc/en/software

### 3. Install ESP32 Board Package

```
File → Preferences → Additional Board URLs:
https://espressif.github.io/arduino-esp32/package_esp32_index.json

Boards Manager → Install: esp32 by Espressif Systems 3.x
```

### 4. Install Required Libraries (Arduino Library Manager)

| Library | Version | Purpose |
|---|---|---|
| Adafruit SHT4x Library | 1.x | SHT4x temperature/humidity |
| Adafruit BusIO | 1.x | Adafruit I2C/SPI abstraction |
| Adafruit Unified Sensor | 1.x | Sensor abstraction layer |
| Adafruit INA219 | 1.x | INA219 current/voltage sensor |
| TinyGPSPlus | 1.x | GPS NMEA parser |
| ArduinoJson | 7.x | JSON serialization |
| RTClib | 2.x | DS3231 RTC |
| OneWire | 2.x | (legacy, kept in tree) |
| DallasTemperature | 4.x | (legacy, kept in tree) |
| Adafruit MPU6050 | 2.x | (legacy, kept in tree) |

The following are included with the ESP32 core:
- WiFi, SPI, SD, Wire, WebServer, HardwareSerial

### 5. Configure Credentials

Create `secrets.h` in the project folder (this file is excluded from Git):

```cpp
#ifndef SECRETS_H
#define SECRETS_H

#define WIFI_SSID     "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

#define MQTT_USERNAME "YOUR_MQTT_USERNAME"
#define MQTT_PASSWORD "YOUR_MQTT_PASSWORD"

#endif
```

### 6. Configure TLS Certificate

Replace the Root CA certificate in `certs.h` with the certificate from your MQTT broker.

### 7. Board Settings (Arduino IDE)

| Setting | Value |
|---|---|
| Board | ESP32S3 Dev Module |
| Flash Size | 16 MB |
| PSRAM | OPI PSRAM |
| Partition Scheme | 16MB Flash (3MB APP / 9MB FATFS) |
| Upload Speed | 921600 |

### 8. Upload and Open Serial Monitor at 115200 baud

---

## Hardware

### Microcontroller

| Component | Details |
|---|---|
| ESP32-S3 | Main controller, integrated WiFi, OPI PSRAM |

---

### Sensors

| Sensor | Interface | Purpose |
|---|---|---|
| SHT4x | I2C (0x44) | Temperature (°C) and relative humidity (%) |
| INA219 | I2C (0x40) | Bus voltage, shunt voltage, current, power |
| MQ-3 | ADC GPIO4 | Alcohol-sensitive analog output |
| MQ-135 | ADC GPIO5 | Air-quality analog output |
| MQ-4 | ADC GPIO6 | Methane/natural-gas analog output |
| GY-GPS6MV2 / NEO-6M | UART1 | GPS latitude, longitude, UTC time |
| DS3231 RTC | I2C | Offline timestamp fallback |
| MicroSD | SPI | Event and reset logging |

---

## Wiring

### I2C Bus (Shared — SHT4x, INA219, DS3231)

| Pin | ESP32-S3 GPIO |
|---|---|
| SDA | GPIO 8 |
| SCL | GPIO 9 |

### SHT4x

| SHT4x Pin | ESP32-S3 |
|---|---|
| VCC | 3.3 V |
| GND | GND |
| SDA | GPIO 8 |
| SCL | GPIO 9 |

### INA219

| INA219 Pin | ESP32-S3 |
|---|---|
| VCC | 3.3 V |
| GND | GND |
| SDA | GPIO 8 |
| SCL | GPIO 9 |

### MQ Sensor Voltage Divider

**IMPORTANT:** ESP32-S3 ADC pins are limited to 3.3 V maximum. MQ modules may output up to 5 V on their AO pin. A voltage divider is required.

Expected circuit per sensor:

```
MQ AO ── 10 kΩ ──┬── ESP32-S3 ADC pin
                  │
                20 kΩ
                  │
                 GND
```

`V_ADC = V_sensor × (20 / 30) = V_sensor × 0.667`
`V_sensor = V_ADC × 1.5`

**Verify actual resistor values with a multimeter before powering on.**

| MQ Sensor | ADC Pin | GPIO |
|---|---|---|
| MQ-3 | MQ3_ANALOG_PIN | GPIO 4 |
| MQ-135 | MQ135_ANALOG_PIN | GPIO 5 |
| MQ-4 | MQ4_ANALOG_PIN | GPIO 6 |

> **Note:** GPIO 4 was previously used for DS18B20. DS18B20 is excluded from this build. Do not connect both simultaneously.

### GPS (NEO-6M / GY-GPS6MV2)

| GPS Module Pin | ESP32-S3 |
|---|---|
| VCC | 3.3 V or 5 V (check module) |
| GND | GND |
| TX (GPS) | GPIO 17 (ESP RX) |
| RX (GPS) | GPIO 18 (ESP TX) |

### RGB Status LED

| Pin | GPIO |
|---|---|
| RGB Data | GPIO 48 (onboard NeoPixel) |

### MicroSD (SPI)

| SD Pin | ESP32-S3 |
|---|---|
| CS | GPIO 10 |
| MOSI | GPIO 11 |
| SCK | GPIO 12 |
| MISO | GPIO 13 |

---

## Telemetry Payload (JSON Contract)

Every MQTT publish produces exactly these ten fields:

```json
{
  "device_id": "agrihub-001",
  "timestamp": "2026-10-22T11:00:00Z",
  "temperature": 25.6,
  "humidity": 72.4,
  "mq3": 10.0,
  "mq135": 30.0,
  "mq4": 28.0,
  "latitude": 12.9716,
  "longitude": 77.5946,
  "battery": null
}
```

### Field Reference

| Field | Type | Notes |
|---|---|---|
| `device_id` | string | Configured in `config.h` as `DEVICE_ID` |
| `timestamp` | string or null | ISO 8601 UTC. GPS time preferred; NTP/RTC fallback; null if unavailable |
| `temperature` | number or null | Degrees Celsius (SHT4x) |
| `humidity` | number or null | Relative humidity % (SHT4x) |
| `mq3` | number or null | **Normalized ADC output % (0–100). NOT alcohol concentration.** |
| `mq135` | number or null | **Normalized ADC output % (0–100). NOT air-pollution concentration.** |
| `mq4` | number or null | **Normalized ADC output % (0–100). NOT methane concentration.** |
| `latitude` | number or null | Decimal degrees. null when GPS fix unavailable or stale |
| `longitude` | number or null | Decimal degrees. null when GPS fix unavailable or stale |
| `battery` | number or null | **Always null** until a validated SoC model is implemented |

> **MQ Interpretation Warning:** `mq3`, `mq135`, and `mq4` are normalized analog output percentages derived from a voltage-divider scaled ADC reading. They are **not** calibrated gas concentrations, not PPM values, and must **not** be used as gas-leak or safety alarms without a proper sensor characterization and validated conversion model.

> **Battery Warning:** INA219 measures electrical quantities in the circuit (voltage, current, power). It does **not** directly measure battery state-of-charge. `battery` remains `null` until a documented and validated SoC estimation method is implemented.

---

## MQ Sensor Scaling

### Circuit Assumption

```
V_ADC    = (rawADC / 4095) × 3.3
V_sensor = V_ADC × 1.5          (10 kΩ / 20 kΩ divider)
output % = (V_sensor / 5.0) × 100, clamped 0–100
```

### Mathematical Maximum

Under these assumptions the maximum achievable percentage is:

```
(3.3 × 1.5 / 5.0) × 100 = 99 %
```

This is correct and expected. Do not change the divider ratio to force 100%.

### Warm-Up

MQ sensors require heater warm-up time before their resistance stabilizes. The firmware applies a 30-second non-blocking guard (`MQ_WARMUP_MS`). During this period, MQ fields are published as `null`. This is the **minimum** warm-up — follow the actual sensor datasheet and characterize warm-up experimentally before relying on readings.

---

## MQTT Topics

| Topic | Purpose |
|---|---|
| `agricool/v1/devices/{device_id}/telemetry` | Telemetry data (15-second interval) |
| `agricool/v1/devices/{device_id}/status` | Device online/offline status (LWT) |

---

## GPS Timestamp Policy

- GPS UTC time is used as the telemetry timestamp when `gps.date.isValid()` AND `gps.time.isValid()` are both true.
- Falls back to NTP-synchronized system clock (or DS3231 RTC) when GPS time is unavailable.
- Publishes `null` only if **all** time sources are unavailable.
- Does NOT label unsynchronized local device time as UTC.

---

## Boot Sequence

1. Initialize RGB LED
2. Set Device ID (`agrihub-001`)
3. Initialize RTC (DS3231)
4. Initialize FIFO Buffer
5. Initialize WiFi (non-blocking)
6. Initialize NTP
7. Initialize SD Card
8. Initialize Web Log Server
9. Initialize I2C Bus (GPIO8/9)
10. Initialize SHT4x
11. Initialize INA219
12. Initialize MQ Sensors (set 12-bit ADC)
13. Initialize GPS UART
14. Initialize Watchdog
15. If WiFi connected → Initialize MQTT
16. Enter main loop

---

## Normal Operation

- `handleGPS()` called every loop iteration — drains UART FIFO, non-blocking
- Sensors read every 1 second
- MQ values only written to telemetry after 30-second warm-up
- GPS lat/lon and timestamp updated every 1 second
- NTP/RTC timestamp used when GPS fix is stale or unavailable
- Telemetry published every 15 seconds (or on significant change)
- Offline buffer fills during MQTT outages; replays on reconnection

---

## Troubleshooting

### SHT4x Not Detected
- Check SDA/SCL wiring (GPIO8/GPIO9)
- Verify 3.3 V supply
- Confirm I2C address (default 0x44)

### INA219 Not Detected
- Check SDA/SCL wiring (shared I2C bus)
- Verify I2C address (default 0x40)
- Ensure shunt resistor is connected

### MQ Sensor Reads Zero or Unstable
- Wait for 30-second warm-up
- Verify voltage-divider resistors with a multimeter
- Confirm ADC pin voltage does not exceed 3.3 V
- Check sensor power supply (MQ modules typically need 5 V VCC)

### GPS Not Acquiring Fix
- Allow several minutes outdoors for cold start
- Verify TX/RX wiring (GPS TX → ESP GPIO17, GPS RX → ESP GPIO18)
- Confirm baud rate (9600)
- Check antenna connection on NEO-6M module

### GPS Timestamp Shows null
- GPS date and time must both be valid — requires a valid fix
- Check antenna orientation and obstructions
- Indoor testing will not produce a GPS fix

### WiFi Not Connecting
- Verify SSID and password in `secrets.h`
- Use a 2.4 GHz network

### MQTT Connection Failed
- Verify MQTT credentials in `secrets.h`
- Verify Root CA certificate in `certs.h`
- Verify broker hostname and port 8883

---

## Known Limitations and Next Steps

| Item | Status |
|---|---|
| MQ sensor values are NOT calibrated concentrations | By design — do not change without a validated model |
| `battery` always null | INA219 does not measure SoC — implement model when battery chemistry is confirmed |
| GPS cold-start can take several minutes | Expected behavior for NEO-6M |
| MQ warm-up (30 s) does not guarantee sensor stability | Follow datasheet; characterize experimentally |
| DS18B20 / MPU6050 / ACS712 code retained in tree but excluded from main loop | Can be re-enabled if needed for parallel monitoring |
| TLS Root CA certificate must be updated per broker | See `certs.h` |
| MQTT exponential backoff reconnection planned | Currently uses fixed 5-second retry interval |

---

## File Structure

```
AgriCoolHub/
│
├── FactorySense.ino        ← Main entry point (AgriCoolHub build)
├── config.h                ← Device ID, timing, MQ scaling constants
├── pins.h                  ← All GPIO assignments
├── telemetry.h / .cpp      ← 10-field data model
├── json_builder.h / .cpp   ← Strict 10-field JSON serializer
├── topic_manager.h / .cpp  ← agricool/v1/devices/… topic paths
│
├── SHT4xSensor.h / .cpp    ← Temperature + humidity (SHT4x)
├── INA219Sensor.h / .cpp   ← Voltage / current / power (INA219)
├── MQSensor.h / .cpp       ← Reusable ADC scaling (MQ-3, MQ-135, MQ-4)
├── GPSSensor.h / .cpp      ← Non-blocking GPS parser (TinyGPSPlus)
│
├── mqtt_manager.h / .cpp   ← ESP-IDF MQTT, TLS, QoS 1, LWT
├── wifi_manager.h / .cpp   ← WiFi station + exponential backoff
├── ntp_manager.h / .cpp    ← NTP sync + ISO 8601 timestamp
├── rtc_manager.h / .cpp    ← DS3231 RTC fallback
├── buffer_manager.h / .cpp ← 60-message FIFO offline buffer
├── event_manager.h / .cpp  ← Delta-threshold publish decisions
├── device_manager.h / .cpp ← MAC-based device ID
├── watchdog_manager.h/.cpp ← Task watchdog
├── led_manager.h / .cpp    ← RGB status LED
├── sd_manager.h / .cpp     ← SD card event logging
├── web_log_manager.h/.cpp  ← Web diagnostics dashboard
├── certs.h / .cpp          ← TLS Root CA certificate
├── secrets.h               ← Credentials (excluded from Git)
│
├── README.md
├── DECISIONS.md
└── docs/
```

---

## Library Installation Summary

```
Arduino Library Manager → Install:
  Adafruit SHT4x Library
  Adafruit BusIO
  Adafruit Unified Sensor
  Adafruit INA219
  TinyGPSPlus
  ArduinoJson (7.x)
  RTClib
  OneWire
  DallasTemperature
  Adafruit MPU6050
```

---

*Project: AgriCoolHub — Solar Hybrid Agri Cool Hub*
*Platform: ESP32-S3, Arduino Framework, esp32 core 3.x*
