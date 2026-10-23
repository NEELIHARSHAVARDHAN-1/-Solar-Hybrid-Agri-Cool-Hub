# Hardware Test Checklist — Solar Hybrid Agri Cool Hub

This checklist distinguishes between tests that can be executed on a PC (automated)
and tests that require the physical ESP32-S3 hardware.

Legend:
- ✅ AUTOMATED — runs with `pytest` in the `backend/` directory.
- 🔧 HARDWARE — requires physical ESP32-S3 board and connected components.

---

## 1. Backend Automated Tests (No Hardware Required)

| # | Test | Command | Expected Result | Status |
|---|------|---------|-----------------|--------|
| 1.1 | Valid payload accepted | `pytest tests/test_schemas.py` | PASS | ✅ PASS |
| 1.2 | Null battery accepted | `pytest tests/test_schemas.py` | PASS | ✅ PASS |
| 1.3 | Missing optional fields default to None | `pytest tests/test_schemas.py` | PASS | ✅ PASS |
| 1.4 | Invalid temperature rejected | `pytest tests/test_schemas.py` | PASS | ✅ PASS |
| 1.5 | ADC value out of range rejected | `pytest tests/test_schemas.py` | PASS | ✅ PASS |
| 1.6 | Device + telemetry created on ingest | `pytest tests/test_services.py` | PASS | ✅ PASS |
| 1.7 | Duplicate insert silently ignored | `pytest tests/test_services.py` | PASS | ✅ PASS |
| 1.8 | Alert created on threshold breach | `pytest tests/test_services.py` | PASS | ✅ PASS |
| 1.9 | Alert not duplicated on repeated breach | `pytest tests/test_services.py` | PASS | ✅ PASS |
| 1.10 | No alert below threshold | `pytest tests/test_services.py` | PASS | ✅ PASS |
| 1.11 | All API endpoints respond correctly | `pytest tests/test_api.py` | PASS | ✅ PASS |

---

## 2. Firmware Compilation Test (Arduino IDE)

| # | Test | Procedure | Expected Result | Status |
|---|------|-----------|-----------------|--------|
| 2.1 | Sketch compiles without errors | Open `firmware/firmware.ino`, set board to "ESP32S3 Dev Module", click Verify (✓) | 0 errors | 🔧 REQUIRES HARDWARE |

**Note:** Compilation requires the Arduino IDE with all libraries from `library_list.txt`
and a valid `src/config_secrets.h` file copied from `src/config_secrets.h.example`.

---

## 3. Sensor and RTC Reading Tests

| # | Test | Procedure | Expected Result | Status |
|---|------|-----------|-----------------|--------|
| 3.1 | SHT40 reads temperature and humidity | Flash firmware, open Serial Monitor 115200 baud, observe telemetry JSON | `temperature` is float 10–40°C; `humidity` is 0–100% | 🔧 REQUIRES HARDWARE |
| 3.2 | DS3231 returns correct timestamp | Check `timestamp` field in telemetry JSON | ISO-8601 string with `+05:30` offset; time matches real time | 🔧 REQUIRES HARDWARE |
| 3.3 | MQ-3/135/4 return ADC values | Check `mq3`, `mq135`, `mq4` in JSON | Integer values in range 0–4095 | 🔧 REQUIRES HARDWARE |
| 3.4 | INA219 returns battery percent | Check `battery_percent` in JSON | Integer 0–100 or `null` if INA219 not present | 🔧 REQUIRES HARDWARE |
| 3.5 | RTC recovery after power loss | Remove coin cell, power cycle, reflash | `[RTCManager] WARNING: DS3231 lost power` in Serial, RTC set to compile time | 🔧 REQUIRES HARDWARE |

---

## 4. Wi-Fi Connectivity and Backoff Tests

| # | Test | Procedure | Expected Result | Status |
|---|------|-----------|-----------------|--------|
| 4.1 | Wi-Fi connects successfully | Flash with correct SSID/password | `[Network] Wi-Fi connected. IP: x.x.x.x` in Serial | 🔧 REQUIRES HARDWARE |
| 4.2 | Wrong SSID triggers backoff | Set wrong SSID, observe Serial | `[Network] Backoff: waiting XXXX ms` logged, delays double each cycle up to 120 s | 🔧 REQUIRES HARDWARE |
| 4.3 | Wi-Fi recovery after dropout | Disconnect router, wait 30 s, reconnect | Firmware automatically reconnects without reset | 🔧 REQUIRES HARDWARE |
| 4.4 | Telemetry continues offline | Disconnect Wi-Fi, observe Serial | `Offline — queued. Depth=N` printed each 30 s cycle | 🔧 REQUIRES HARDWARE |

---

## 5. MQTT Connectivity and Backoff Tests

| # | Test | Procedure | Expected Result | Status |
|---|------|-----------|-----------------|--------|
| 5.1 | MQTT connects to HiveMQ Cloud | Flash with correct broker credentials | `[Network] MQTT connected.` in Serial | 🔧 REQUIRES HARDWARE |
| 5.2 | MQTT reconnects after broker disconnect | Stop HiveMQ listener or change credentials temporarily | `[Network] MQTT disconnected. Reconnecting...` then reconnects | 🔧 REQUIRES HARDWARE |
| 5.3 | Wrong MQTT credentials trigger backoff | Set wrong MQTT_USER, observe Serial | Backoff doubles each attempt; does not crash | 🔧 REQUIRES HARDWARE |
| 5.4 | Telemetry published after MQTT connects | Connect, observe backend logs | Backend receives and stores telemetry | 🔧 REQUIRES HARDWARE |

---

## 6. SD Card Queue and Replay Tests

| # | Test | Procedure | Expected Result | Status |
|---|------|-----------|-----------------|--------|
| 6.1 | SD card mounts successfully | Flash firmware, observe Serial | `[SDQueue] SD card mounted.` | 🔧 REQUIRES HARDWARE |
| 6.2 | Readings queued when offline | Disconnect Wi-Fi for 5 min | `queue.jsonl` on SD card contains telemetry JSON lines | 🔧 REQUIRES HARDWARE |
| 6.3 | Queue persists across power cycle | Power off device while offline, power on | `queue.jsonl` still contains queued readings | 🔧 REQUIRES HARDWARE |
| 6.4 | Queue replays after reconnect | Restore Wi-Fi, observe Serial and backend DB | `[SDQueue] Replay done. Published=N` — readings appear in `/devices/agrihub-001/telemetry` API | 🔧 REQUIRES HARDWARE |
| 6.5 | Duplicate records prevented | Replay same readings twice | Backend DB contains no duplicate `(device_id, timestamp)` rows | 🔧 REQUIRES HARDWARE |
| 6.6 | Corrupted line skipped | Manually corrupt one line in `queue.jsonl` | `[SDQueue] Skipping corrupted line:...` in Serial; other lines replay normally | 🔧 REQUIRES HARDWARE |

---

## 7. Backend Integration Tests

| # | Test | Procedure | Expected Result | Status |
|---|------|-----------|-----------------|--------|
| 7.1 | GET /health returns ok | `curl http://localhost:8000/health` | `{"status":"ok"}` | 🔧 REQUIRES BACKEND |
| 7.2 | Telemetry appears in API | Send MQTT message → check `/devices/agrihub-001/telemetry/latest` | JSON row with correct values | 🔧 REQUIRES BACKEND |
| 7.3 | Stale device shows offline | No MQTT for 6+ min → check `/devices/agrihub-001/status` | `is_online: false` | 🔧 REQUIRES BACKEND |
| 7.4 | Alert created on breach | Send temperature > 35°C → check `/devices/agrihub-001/alerts/active` | Alert with type `TEMP_HIGH` present | 🔧 REQUIRES BACKEND |
| 7.5 | Alert resolved via API | `PATCH /alerts/{id}/resolve` | Alert has non-null `resolved_at`; no longer in active list | 🔧 REQUIRES BACKEND |

---

## 8. Flutter App Integration Tests

| # | Test | Procedure | Expected Result | Status |
|---|------|-----------|-----------------|--------|
| 8.1 | App builds and runs | `cd app && flutter run` | No build errors; app launches on emulator | 🔧 REQUIRES FLUTTER |
| 8.2 | Dashboard shows live data | Connect app to running backend | Temperature, humidity, MQ values, battery % displayed | 🔧 REQUIRES BACKEND + FLUTTER |
| 8.3 | Offline device shows OFFLINE badge | Backend reports `is_online: false` | Red OFFLINE badge visible on dashboard | 🔧 REQUIRES BACKEND + FLUTTER |
| 8.4 | History chart renders | Navigate to History screen | Line chart with temperature and humidity data | 🔧 REQUIRES BACKEND + FLUTTER |
| 8.5 | Alerts screen shows active alerts | Active alerts exist in backend | Alert list populated; resolve button works | 🔧 REQUIRES BACKEND + FLUTTER |
| 8.6 | Settings URL change persists | Change URL in Settings, restart app | New URL is loaded on restart | 🔧 REQUIRES FLUTTER |

---

## 9. Power Interruption and Recovery Test

| # | Test | Procedure | Expected Result | Status |
|---|------|-----------|-----------------|--------|
| 9.1 | Power cycle mid-queue | Queue readings, power off, power on | Queued readings present in `queue.jsonl`; replay proceeds | 🔧 REQUIRES HARDWARE |
| 9.2 | Power cycle during SD write | Unlikely to test safely; described here for awareness | Possible partial line at end of file; firmware skips corrupted line via `_isValidJsonLine()` check | 🔧 REQUIRES HARDWARE |

---

## Summary of Automated Test Results

```
Platform: Windows 10, Python 3.10.0
Date: 2026
Command: pytest tests/ -v

24 passed, 0 failed
- test_api.py:      8 passed
- test_schemas.py: 11 passed
- test_services.py: 5 passed
```
