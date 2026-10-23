# Solar Hybrid Agri Cool Hub — Implementation Plan

## Top-Level Overview

Build a complete end-to-end IoT system for agricultural cold-storage monitoring.

**Data flow:**
```
ESP32-S3 N16R8 → HiveMQ Cloud (MQTT) → FastAPI Backend → PostgreSQL → Flutter Android App
```

The ESP32 reads sensors every 30 seconds, timestamps every reading with the DS3231 RTC, stores readings to SD card when offline, and publishes telemetry via MQTT when connected. The FastAPI backend ingests telemetry, detects alert conditions against configurable thresholds, and serves REST APIs. The Flutter app displays live status, history, and alerts.

**Scope boundaries:**
- Humidifier relay is NOT installed — humidifier code is excluded entirely.
- INA219 battery monitor IS installed — battery percentage is enabled.
- Coordinates are static config values, not GPS.
- MQ sensor values are raw ADC integers — no gas calibration claimed.

**Verified hardware constants:**
| Component | Interface | Pins |
|-----------|-----------|------|
| SHT40 | I2C | SDA=GPIO8, SCL=GPIO9 |
| DS3231 | I2C (shared) | SDA=GPIO8, SCL=GPIO9 |
| INA219 | I2C (shared) | SDA=GPIO8, SCL=GPIO9 |
| SD Card | SPI | MOSI=11, MISO=13, CLK=12, CS=10 |
| MQ-3 | ADC | GPIO4 |
| MQ-135 | ADC | GPIO5 |
| MQ-4 | ADC | GPIO6 |
| Humidifier | — | NOT INSTALLED |

**MQ sensor voltage safety:** The ESP32-S3 ADC inputs are 3.3 V tolerant. The MQ sensor AOUT pin must be connected through a voltage divider (10 kΩ / 10 kΩ recommended) if the sensor is powered at 5 V, to bring AOUT from ~5 V max to ~2.5 V max. If powered at 3.3 V, the divider is still recommended to stay within the safe ADC range.

---

## Sub-Task 1 — Project Scaffold and Directory Structure

**Status:** `[ ] pending`

**Intent:** Create the full directory layout so every subsequent sub-task writes files into the correct location with no ambiguity.

**Expected Outcomes:**
- Root directory contains `firmware/`, `backend/`, `app/`, `docs/`, `tests/` folders.
- Each folder contains a placeholder `README.md` explaining its purpose.
- Root `README.md` exists with project title and folder map.

**Todo List:**
1. Create `firmware/` with sub-folders: `firmware/src/`, `firmware/lib/`.
2. Create `backend/` with sub-folders: `backend/app/`, `backend/tests/`.
3. Create `app/` (Flutter project root — populated in Sub-Task 6).
4. Create `docs/` for wiring guide, API docs, and deployment guide.
5. Create `tests/` for backend integration tests.
6. Write root `README.md` with title, architecture diagram reference, and folder map.

**Relevant Context:**
- All paths are relative to the workspace root `p:\IBM PROJECT\Finals`.
- Firmware is Arduino IDE 2.x — the `firmware/` folder IS the sketch folder; the main `.ino` file must match the folder name.

---

## Sub-Task 2 — Firmware: Configuration Module

**Status:** `[ ] pending`

**Intent:** Centralise all credentials, pin assignments, thresholds, and tuning constants in one file so no secret appears in source code and hardware constants are documented in one place.

**Expected Outcomes:**
- `firmware/src/config.h` contains all configurable values.
- `firmware/src/config_secrets.h.example` contains placeholder credentials.
- The main sketch includes `config_secrets.h` which is git-ignored.

**Todo List:**
1. Write `firmware/src/config.h`:
   - Pin definitions (all GPIO constants from verified wiring table above).
   - I2C address constants for SHT40 (0x44), DS3231 (0x68), INA219 (0x40).
   - MQTT topic strings: `agrihub/telemetry`, `agrihub/command`.
   - Telemetry interval: 30 000 ms.
   - Exponential backoff constants: `WIFI_BACKOFF_BASE_MS=2000`, `WIFI_BACKOFF_MAX_MS=120000`.
   - SD queue filename: `/queue.jsonl`.
   - Device ID: `"agrihub-001"`.
   - Timezone offset seconds: `19800` (UTC+05:30).
   - Static coordinates: `LAT=15.8281`, `LON=78.0373`.
   - INA219 shunt resistor: `0.1 Ω`, max current: `3.2 A`, battery full voltage: `16.8 V`, battery empty voltage: `12.0 V`.
   - Feature flags: `ENABLE_BATTERY true`, `ENABLE_HUMIDIFIER false`.
2. Write `firmware/src/config_secrets.h.example` with empty-string placeholders for `WIFI_SSID`, `WIFI_PASSWORD`, `MQTT_HOST`, `MQTT_PORT`, `MQTT_USER`, `MQTT_PASSWORD`.
3. Add `.gitignore` at `firmware/` level that excludes `config_secrets.h`.

**Relevant Context:**
- Arduino IDE 2.x uses `#include` relative paths; `config.h` and `config_secrets.h` live in `firmware/src/` alongside all other `.h` files.

---

## Sub-Task 3 — Firmware: Core Modules (Sensors, RTC, Battery)

**Status:** `[ ] pending`

**Intent:** Implement the sensor reading, RTC timestamping, and INA219 battery modules as separate `.h`/`.cpp` pairs so the main loop stays clean.

**Expected Outcomes:**
- `SensorManager` reads SHT40 and MQ-3/135/4; returns a struct with all values and a validity flag.
- `RTCManager` reads DS3231 and returns an ISO-8601 timestamp string with IST offset.
- `BatteryMonitor` reads INA219 bus voltage and maps it to a 0–100% integer.
- All modules initialise safely and set an error flag if hardware is absent.

**Todo List:**
1. Write `firmware/src/SensorManager.h` and `SensorManager.cpp`:
   - Include `Adafruit_SHT4x.h`.
   - Read SHT40 via I2C; store temperature (°C) and humidity (%).
   - Read MQ-3, MQ-135, MQ-4 via `analogRead()` on GPIO4/5/6.
   - Return a `SensorData` struct: `{ float temperature, float humidity, int mq3, int mq135, int mq4, bool valid }`.
   - Mark `valid=false` if SHT40 read fails; MQ reads always succeed (hardware permitting).
2. Write `firmware/src/RTCManager.h` and `RTCManager.cpp`:
   - Include `RTClib.h`.
   - On init, if RTC lost power, log a warning to Serial but do not halt.
   - `getTimestamp()` returns a `String` in format `"YYYY-MM-DDTHH:MM:SS+05:30"`.
   - Provide `setFromCompile()` to sync RTC to build time (used once on first flash).
3. Write `firmware/src/BatteryMonitor.h` and `BatteryMonitor.cpp`:
   - Include `Adafruit_INA219.h`.
   - Read bus voltage; compute percentage using linear map between `BATTERY_EMPTY_V` and `BATTERY_FULL_V`.
   - Clamp result to 0–100.
   - Return `int` (percent); return `-1` if INA219 not found.

**Relevant Context:**
- Required Arduino libraries: `Adafruit SHT4x`, `RTClib` (Adafruit), `Adafruit INA219`.
- SHT40, DS3231, and INA219 all share the I2C bus on SDA=GPIO8, SCL=GPIO9.
- `Wire.begin(8, 9)` must be called in `setup()` before initialising any I2C device.

---

## Sub-Task 4 — Firmware: Wi-Fi and MQTT with Exponential Backoff

**Status:** `[ ] pending`

**Intent:** Implement non-blocking Wi-Fi and MQTT connectivity using a state machine so the main loop never stalls on connection attempts.

**Expected Outcomes:**
- `NetworkManager` has a `tick()` method called from `loop()`.
- Wi-Fi reconnection uses exponential backoff with jitter.
- MQTT reconnects independently once Wi-Fi is up.
- `publish(payload)` returns `bool`; caller decides what to do on failure.
- Credentials never appear in source; read from `config_secrets.h`.

**Todo List:**
1. Write `firmware/src/NetworkManager.h` and `NetworkManager.cpp`:
   - States: `WIFI_DISCONNECTED`, `WIFI_CONNECTING`, `WIFI_CONNECTED`, `MQTT_CONNECTING`, `MQTT_CONNECTED`.
   - Backoff algorithm:
     - Start: `delay = WIFI_BACKOFF_BASE_MS`.
     - On failure: `delay = min(delay * 2, WIFI_BACKOFF_MAX_MS)`.
     - Jitter: add `random(0, delay / 4)` to every delay.
     - On success: reset `delay = WIFI_BACKOFF_BASE_MS`.
   - Use `WiFi.status()` polling — no blocking `WiFi.begin()` loop.
   - Use `PubSubClient` for MQTT over TLS (HiveMQ Cloud requires port 8883 with CA cert).
   - `isConnected()` returns `true` only when MQTT state is `MQTT_CONNECTED`.
   - `publish(const char* topic, const String& payload)` returns `bool`.
   - Include a `setMessageCallback(cb)` for future command subscription.
2. Document the backoff algorithm and configuration constants in `firmware/src/config.h` comments.

**Relevant Context:**
- HiveMQ Cloud free tier uses TLS on port 8883; `WiFiClientSecure` with the ISRG Root X1 CA certificate is required.
- Library: `PubSubClient` (knolleary), `WiFiClientSecure` (bundled with ESP32 Arduino core).
- TLS CA cert string goes in `config.h` (not a secret); credentials go in `config_secrets.h`.

---

## Sub-Task 5 — Firmware: SD Card Queue and Replay

**Status:** `[ ] pending`

**Intent:** Implement store-and-forward: every telemetry reading is appended to `/queue.jsonl` on the SD card; after MQTT reconnects, the queue is replayed line-by-line and delivered readings are removed.

**Expected Outcomes:**
- `SDQueue` appends one JSON line per reading to `/queue.jsonl`.
- After MQTT connects, `SDQueue::replay()` reads the file line by line, publishes each, and on success rewrites the file without the delivered line.
- Corrupted (non-parseable) JSON lines are skipped with a Serial warning; they do not halt replay.
- SD initialisation failure is logged; the firmware continues without SD.
- File rotation: if file exceeds `SD_MAX_FILE_BYTES` (default 500 KB), oldest lines are dropped.

**Todo List:**
1. Write `firmware/src/SDQueue.h` and `SDQueue.cpp`:
   - `begin()`: call `SD.begin(SD_CS_PIN)` with SPI pins; set `available` flag.
   - `enqueue(const String& json)`: append line to `/queue.jsonl`; check file size before writing.
   - `replay(NetworkManager& net)`: read file, attempt publish for each line, collect successfully-delivered lines, rewrite file without them.
   - Deduplication note: the backend uses `(device_id, timestamp)` as a unique key and ignores duplicate inserts; the firmware does NOT add extra dedup fields to the payload.
   - `getQueueDepth()`: returns line count of `/queue.jsonl`.
2. Add `SD_CS_PIN`, `SD_MAX_FILE_BYTES` to `config.h`.

**Relevant Context:**
- SPI pins for SD: MOSI=11, MISO=13, CLK=12, CS=10.
- `SPI.begin(12, 13, 11, 10)` must be called before `SD.begin(10)`.
- Library: `SD` (built into ESP32 Arduino core).

---

## Sub-Task 6 — Firmware: Main Sketch

**Status:** `[ ] pending`

**Intent:** Wire all modules together in `firmware/firmware.ino` with a clean, readable main loop.

**Expected Outcomes:**
- `setup()` initialises Serial, Wire, SPI, all modules in dependency order.
- `loop()` calls `networkManager.tick()`, reads sensors every 30 s, builds JSON payload, enqueues to SD, publishes if connected, and triggers replay when newly connected.
- Payload builder follows the exact JSON spec: omits `humidifier_enabled` (not installed), sends `battery_percent` as int or `null`.
- Serial output shows state transitions and publish results without flooding at 115200 baud.

**Todo List:**
1. Write `firmware/firmware.ino` as the Arduino sketch entry point.
2. Include all module headers.
3. Implement `buildPayload()` returning `String` with ArduinoJson.
4. Implement the 30-second non-blocking timer using `millis()`.
5. Call `sdQueue.replay()` once immediately after MQTT connects (detect transition).
6. Add `firmware/library_list.txt` listing all required libraries with versions.
7. Add `firmware/FLASH_INSTRUCTIONS.md` with board selection, port, and upload steps.

**Relevant Context:**
- Library: `ArduinoJson` (≥6.x) for payload building and SD queue parsing.
- Board: "ESP32S3 Dev Module", flash size 16 MB, PSRAM: OPI PSRAM.
- The sketch folder name must match the `.ino` filename: both must be `firmware`.

---

## Sub-Task 7 — Backend: Project Scaffold and Configuration

**Status:** `[ ] pending`

**Intent:** Create the FastAPI project structure, environment configuration, and database connection setup.

**Expected Outcomes:**
- `backend/` contains `app/main.py`, `app/config.py`, `app/database.py`, `app/models.py`, `app/schemas.py`.
- `backend/.env.example` documents all required environment variables.
- `backend/requirements.txt` lists all dependencies with pinned versions.
- `backend/docker-compose.yml` provides a local PostgreSQL instance for development.

**Todo List:**
1. Write `backend/requirements.txt`: `fastapi`, `uvicorn[standard]`, `asyncpg`, `sqlalchemy[asyncio]`, `aiomqtt`, `pydantic`, `pydantic-settings`, `python-dotenv`, `alembic`, `pytest`, `httpx`.
2. Write `backend/app/config.py` using `pydantic-settings`; reads from environment: `DATABASE_URL`, `MQTT_HOST`, `MQTT_PORT`, `MQTT_USER`, `MQTT_PASSWORD`, `MQTT_TOPIC_TELEMETRY`, alert thresholds (`TEMP_MAX`, `HUMIDITY_MAX`, `MQ3_MAX`, `MQ135_MAX`, `MQ4_MAX`).
3. Write `backend/app/database.py`: async SQLAlchemy engine + session factory.
4. Write `backend/.env.example` with all keys and placeholder values.
5. Write `backend/docker-compose.yml` for local Postgres (image: postgres:15).

**Relevant Context:**
- Use async SQLAlchemy 2.x with `asyncpg` driver for non-blocking DB operations.
- HiveMQ Cloud MQTT uses TLS; `aiomqtt` supports TLS via `tls_params`.

---

## Sub-Task 8 — Backend: Database Schema and Models

**Status:** `[ ] pending`

**Intent:** Define the PostgreSQL tables and SQLAlchemy ORM models for devices, telemetry, and alerts.

**Expected Outcomes:**
- Three tables: `devices`, `telemetry`, `alerts`.
- `telemetry` has a unique constraint on `(device_id, timestamp)` to prevent duplicate replay inserts.
- `alerts` are generated by backend logic, not by the firmware.
- An `init_db.sql` script and Alembic migration are both provided.

**Todo List:**
1. Write `backend/app/models.py` with SQLAlchemy ORM classes:
   - `Device`: `id`, `device_id` (unique), `last_seen` (timestamp), `is_online` (bool), `created_at`.
   - `Telemetry`: `id`, `device_id` (FK), `timestamp` (with timezone), `temperature`, `humidity`, `mq3`, `mq135`, `mq4`, `battery_percent` (nullable), `latitude` (nullable), `longitude` (nullable). Unique constraint on `(device_id, timestamp)`.
   - `Alert`: `id`, `device_id` (FK), `timestamp`, `alert_type` (string), `value` (float), `threshold` (float), `resolved_at` (nullable).
2. Write `backend/init_db.sql` as a standalone reproducible DDL script.
3. Write `backend/app/schemas.py` with Pydantic schemas for request validation and API responses.

**Relevant Context:**
- `(device_id, timestamp)` uniqueness is the dedup key; the backend uses `INSERT ... ON CONFLICT DO NOTHING` for replay tolerance.
- Alert types: `"TEMP_HIGH"`, `"HUMIDITY_HIGH"`, `"MQ3_HIGH"`, `"MQ135_HIGH"`, `"MQ4_HIGH"`.

---

## Sub-Task 9 — Backend: MQTT Subscriber and Telemetry Ingestion

**Status:** `[ ] pending`

**Intent:** Implement the MQTT subscriber that listens to `agrihub/telemetry`, validates incoming payloads, upserts device records, inserts telemetry, and triggers alert evaluation.

**Expected Outcomes:**
- MQTT subscriber runs as an `asyncio` background task alongside FastAPI.
- Payload validation uses Pydantic; invalid messages are logged and discarded.
- `battery_percent`, `latitude`, `longitude` are accepted as `null` or missing.
- Duplicate inserts (from SD replay) are silently ignored via `ON CONFLICT DO NOTHING`.
- Alert evaluation runs after every successful insert.

**Todo List:**
1. Write `backend/app/mqtt_client.py`:
   - Connect to HiveMQ Cloud with TLS using `aiomqtt`.
   - Subscribe to `MQTT_TOPIC_TELEMETRY` from config.
   - Parse and validate each message with the `TelemetryPayload` Pydantic schema.
   - Call `ingest_telemetry(payload, db)` service function.
   - Log malformed messages at WARNING level without crashing.
2. Write `backend/app/services.py`:
   - `ingest_telemetry`: upsert device `last_seen`, insert telemetry with conflict ignore, call `evaluate_alerts`.
   - `evaluate_alerts`: compare reading against thresholds from config; insert `Alert` rows for any threshold exceeded; do not create duplicate open alerts for the same device and type.
3. Wire the MQTT background task into `backend/app/main.py` using FastAPI lifespan events.

**Relevant Context:**
- `aiomqtt` async context manager pattern is used; reconnect on broker disconnect.
- Threshold constants live in `app/config.py` and are read from environment.

---

## Sub-Task 10 — Backend: REST API Endpoints

**Status:** `[ ] pending`

**Intent:** Implement all REST endpoints the Flutter app needs.

**Expected Outcomes:**
- All endpoints respond with JSON.
- Pagination is available on list endpoints.
- `GET /health` returns `{"status": "ok"}` with 200.

**Todo List:**
1. Write `backend/app/routers/devices.py`:
   - `GET /devices` — list all known devices with `last_seen` and `is_online` status.
   - `GET /devices/{device_id}` — single device detail.
   - `GET /devices/{device_id}/status` — latest reading + online flag.
2. Write `backend/app/routers/telemetry.py`:
   - `GET /devices/{device_id}/telemetry` — paginated history, supports `?limit=&offset=&from=&to=` filters.
   - `GET /devices/{device_id}/telemetry/latest` — most recent reading.
3. Write `backend/app/routers/alerts.py`:
   - `GET /devices/{device_id}/alerts` — paginated alert history.
   - `GET /devices/{device_id}/alerts/active` — unresolved alerts only.
   - `PATCH /alerts/{alert_id}/resolve` — mark alert resolved.
4. Write `backend/app/routers/health.py`: `GET /health`.
5. Register all routers in `backend/app/main.py`.
6. Write `docs/API.md` with each endpoint, parameters, example request, and example response.

**Relevant Context:**
- Device `is_online` logic: `is_online = (now - last_seen) < 5 minutes`.
- Use FastAPI `Depends` for DB session injection.

---

## Sub-Task 11 — Flutter Android Application

**Status:** `[ ] pending`

**Intent:** Build a minimal, functional Flutter Android app that connects to the real backend API, displaying live status, history, and alerts.

**Expected Outcomes:**
- App has 4 screens: Dashboard, History, Alerts, Settings.
- Dashboard shows: device online/offline badge, temperature, humidity, MQ-3/135/4 raw values, battery %.
- History shows a line chart (fl_chart) of temperature and humidity over time.
- Alerts screen shows active and past alerts.
- Settings screen has a text field for backend base URL (persisted with `shared_preferences`).
- Loading, empty, and error states are handled on every screen.
- Stale data (last seen > 5 min) shows a warning banner.

**Todo List:**
1. Run `flutter create app` or write the Flutter project structure manually under `app/`.
2. Write `app/pubspec.yaml` with dependencies: `http`, `fl_chart`, `shared_preferences`, `intl`, `provider`.
3. Write `app/lib/services/api_service.dart`: typed methods for each backend API call.
4. Write `app/lib/models/`: `DeviceStatus`, `TelemetryReading`, `Alert` data classes with `fromJson`.
5. Write `app/lib/screens/dashboard_screen.dart`.
6. Write `app/lib/screens/history_screen.dart` with fl_chart line chart.
7. Write `app/lib/screens/alerts_screen.dart`.
8. Write `app/lib/screens/settings_screen.dart`.
9. Write `app/lib/main.dart` with route setup and provider initialisation.
10. Write `app/README.md` with build and run instructions.

**Relevant Context:**
- Target: Android only (as specified). Minimum SDK 21.
- Backend URL is configurable in Settings; no hardcoded IP.

---

## Sub-Task 12 — Tests

**Status:** `[ ] pending`

**Intent:** Provide automated backend tests and a structured hardware test checklist.

**Expected Outcomes:**
- `backend/tests/` contains pytest tests for payload validation, duplicate insert handling, alert generation, and all API endpoints.
- `docs/HARDWARE_TEST_CHECKLIST.md` lists every hardware-dependent test with procedure and expected result.
- Tests that do not require hardware can be run with `pytest` and pass.

**Todo List:**
1. Write `backend/tests/test_schemas.py`: valid payload, missing optional fields, null battery, invalid types.
2. Write `backend/tests/test_services.py`: duplicate insert returns no error, alert is created on threshold breach, alert is not duplicated.
3. Write `backend/tests/test_api.py`: mock DB, test every endpoint response shape.
4. Write `backend/tests/conftest.py` with async test client and DB fixtures.
5. Write `docs/HARDWARE_TEST_CHECKLIST.md` covering: sensor read verification, RTC read, Wi-Fi connect/disconnect recovery, MQTT connect/disconnect recovery, SD write/read/replay, INA219 battery reading, power-cycle recovery.

---

## Sub-Task 13 — Documentation

**Status:** `[ ] pending`

**Intent:** Produce the complete documentation set so the project is reproducible by another student.

**Expected Outcomes:**
- Root `README.md` is complete with architecture overview, quick-start, and folder map.
- `docs/WIRING.md` contains the full pin table, voltage divider guidance for MQ sensors, and I2C bus wiring.
- `docs/DEPLOYMENT.md` covers backend and database deployment to Railway/Render + HiveMQ Cloud setup.
- `docs/CONFIGURATION.md` explains every config variable in firmware and backend.
- `docs/API.md` is complete (written in Sub-Task 10).
- `docs/TROUBLESHOOTING.md` covers common failure modes.

**Todo List:**
1. Write final `README.md` with mermaid-free architecture description, component list, and quick-start steps.
2. Write `docs/WIRING.md`.
3. Write `docs/DEPLOYMENT.md`.
4. Write `docs/CONFIGURATION.md`.
5. Write `docs/TROUBLESHOOTING.md`.

---

## MQTT Topic Specification

| Topic | Direction | Publisher | Subscriber | Description |
|-------|-----------|-----------|------------|-------------|
| `agrihub/telemetry` | ESP32 → Backend | ESP32 firmware | FastAPI MQTT client | Sensor telemetry payload |
| `agrihub/command` | Backend → ESP32 | Backend (future) | ESP32 firmware | Reserved for remote commands |

QoS level: **1** (at-least-once) for telemetry. Retained: **false**.

---

## Database Tables Summary

| Table | Key Columns | Notes |
|-------|-------------|-------|
| `devices` | `device_id` (unique), `last_seen`, `is_online` | Upserted on every telemetry arrival |
| `telemetry` | `device_id`, `timestamp` (unique pair) | `ON CONFLICT DO NOTHING` for replay dedup |
| `alerts` | `device_id`, `alert_type`, `resolved_at` | Null `resolved_at` = active alert |

---

## API Endpoints Summary

| Method | Path | Description |
|--------|------|-------------|
| GET | `/health` | Service health check |
| GET | `/devices` | List all devices |
| GET | `/devices/{id}` | Device detail |
| GET | `/devices/{id}/status` | Latest reading + online flag |
| GET | `/devices/{id}/telemetry` | Paginated telemetry history |
| GET | `/devices/{id}/telemetry/latest` | Most recent telemetry row |
| GET | `/devices/{id}/alerts` | Paginated alert history |
| GET | `/devices/{id}/alerts/active` | Unresolved alerts |
| PATCH | `/alerts/{id}/resolve` | Resolve an alert |

---

## Deduplication Strategy

The firmware sends `device_id` and `timestamp` (from DS3231 RTC). The backend uses a PostgreSQL unique constraint on `(device_id, timestamp)` and executes `INSERT ... ON CONFLICT (device_id, timestamp) DO NOTHING`. During SD card replay, every queued message is republished; duplicates are silently dropped by the database. No extra dedup fields are added to the JSON payload.

---

## Assumptions and Open Items

1. **MQ sensor voltage:** Assumed to be powered at 3.3 V from the ESP32 3.3 V rail. A voltage divider is recommended regardless. If powered at 5 V, a 10 kΩ / 10 kΩ divider MUST be used before the ADC pin.
2. **INA219 battery config:** Shunt = 0.1 Ω, full voltage = 16.8 V (4S LiPo), empty = 12.0 V. Adjust `BATTERY_FULL_V` / `BATTERY_EMPTY_V` in `config.h` if battery chemistry differs.
3. **HiveMQ Cloud TLS:** Uses ISRG Root X1 CA certificate (Let's Encrypt root). This is embedded as a string constant in firmware.
4. **RTC sync:** First-flash procedure sets RTC to compile time. After that, RTC runs from its coin cell and is never overwritten by firmware.
5. **Flutter target:** Android only. iOS not in scope.
6. **Humidifier:** Fully excluded. No relay code, no `humidifier_enabled` field in payload.
