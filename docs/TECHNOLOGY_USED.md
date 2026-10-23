# Technology Used
## Solar Hybrid Agri Cool Hub: An AI-Enabled, Solar-Powered Smart Crop Preservation and Transportation Monitoring System

---

## Technology Stack Overview

The Solar Hybrid Agri Cool Hub is built across four interconnected technology layers — IoT firmware, a cloud backend, a web dashboard, and a mobile application — each selected to serve the specific demands of agricultural cold-storage monitoring in resource-constrained, connectivity-variable environments.

---

## 0. The Hybrid Power Architecture — What "Solar Hybrid" Means

Understanding the power design is essential before examining the technology stack,
because it explains both the system's name and why the software monitors what it does.

The system uses **two independent power sources** assigned to two distinct functions:

| Function | Actuator | Power Source | Reason |
|----------|----------|-------------|--------|
| **Active Cooling** | Peltier thermoelectric module (TEC) | Mains / DC power supply | Peltier modules draw high, stable current; solar alone would be unreliable on overcast days — unacceptable for food safety |
| **Humidity Management** | Humidifier module | Solar panel | Humidifiers are low-power and intermittent; solar is sufficient and reduces running costs |

This deliberate split is what makes the system **genuinely hybrid**: reliable mains/DC
power for the safety-critical cooling function, and free renewable solar energy for the
lower-priority humidity function. Neither function is compromised by the other.

**How this shapes the monitoring software:**
- The **INA219** power monitor measures the solar-fed humidifier circuit — flagging
  whether the panel is delivering adequate voltage and current.
- The **SHT4x** sensor measures the combined outcome: if the Peltier fails or the solar
  panel cannot drive the humidifier, temperature rises and humidity drops are captured
  immediately and routed as alerts through the full software pipeline.
- The **MQ gas sensors** detect fermentation gases that begin to accumulate when
  cooling is insufficient, providing a second early-warning layer independent of
  temperature readings.

---

## 1. Hardware and Embedded Platform

**Microcontroller — ESP32-S3 N16R8**
The ESP32-S3 is the edge intelligence of the system. Its dual-core Xtensa LX7 processor, built-in Wi-Fi, 16 MB flash, and 8 MB OPI PSRAM make it suitable for sensor aggregation, JSON serialization, and TLS-encrypted MQTT publishing simultaneously. The PSRAM is used to back a 60-message offline telemetry buffer, ensuring no readings are lost during connectivity outages.

**Actuators**

| Actuator | Power Source | Purpose |
|----------|-------------|---------|
| Peltier thermoelectric cooler (TEC) | Mains DC power supply | Active cooling of the storage compartment |
| Humidifier module | Solar panel | Maintaining produce-appropriate humidity levels |

**Sensors**

| Sensor | Purpose | Interface |
|--------|---------|-----------|
| SHT4x | Temperature (°C) and relative humidity (%) | I2C — SDA=GPIO8, SCL=GPIO9 |
| MQ-3 | Ethanol/alcohol gas — fermentation indicator | ADC — GPIO4 |
| MQ-135 | Air quality / CO₂ / NH₃ | ADC — GPIO5 |
| MQ-4 | Methane / LPG | ADC — GPIO6 |
| INA219 | Solar circuit voltage, current, and power monitoring | I2C — GPIO8/9 |
| NEO-6M GPS | Real-time latitude, longitude, UTC time | UART1 — RX=GPIO17, TX=GPIO18 |
| DS3231 RTC | Offline timestamp fallback (coin-cell backed) | I2C — GPIO8/9 |
| MicroSD | Event and telemetry log storage | SPI — CS=GPIO10 |

MQ sensor analog outputs are conditioned through a 10 kΩ / 20 kΩ voltage divider before the ADC pin to protect the ESP32-S3's 3.3 V-tolerant inputs. MQ values are published as normalized ADC percentages (0–100 %) and are not calibrated gas concentrations.

**Firmware — Arduino Framework + ESP-IDF Core 3.x (C++)**
The firmware is written in C++ under the Arduino Framework on top of Espressif's ESP-IDF core. It uses a fully non-blocking main loop with `millis()`-based timers, exponential-backoff Wi-Fi reconnection, and a hardware watchdog for autonomous recovery.

---

## 2. Communication and Connectivity

**MQTT over TLS — HiveMQ Cloud**
All telemetry is published using MQTT v3.1.1 at QoS 1 (at-least-once delivery) over TLS port 8883 to HiveMQ Cloud. The firmware uses the ESP-IDF native MQTT client (`esp_mqtt`) with a Root CA certificate stored in `certs.h`. A Last Will and Testament (LWT) message (`{"status":"OFFLINE"}`) is registered at connection time so the broker automatically notifies the backend of unclean disconnects.

**Topics**

| Topic | Direction | Purpose |
|-------|-----------|---------|
| `agricool/v1/devices/{id}/telemetry` | ESP32 → Backend | 10-field sensor telemetry every 15 s |
| `agricool/v1/devices/{id}/status` | ESP32 → Backend | Online / Offline via LWT |

**Telemetry Payload (JSON)**
```json
{
  "device_id": "agrihub-001",  "timestamp": "2026-10-22T11:00:00Z",
  "temperature": 25.6,          "humidity": 72.4,
  "mq3": 10.0,                  "mq135": 30.0,   "mq4": 28.0,
  "latitude": 12.9716,          "longitude": 77.5946,
  "battery": null
}
```

---

## 3. Backend (Cloud API Server)

**Language and Framework — Python 3.11, FastAPI 0.111**
The backend is an asynchronous Python service using FastAPI on a Uvicorn ASGI server. The entire stack — MQTT ingestion, REST API, SSE streaming — runs concurrently inside a single `asyncio` event loop with no blocking I/O.

**Database — PostgreSQL 15 with asyncpg**
Telemetry, device registry, alerts, and per-device threshold configurations are stored in PostgreSQL. SQLAlchemy 2.x async ORM provides type-safe queries. A unique constraint on `(device_id, timestamp)` with `INSERT ... ON CONFLICT DO NOTHING` ensures duplicate-free replay of SD-queued messages.

**MQTT Bridge — aiomqtt 2.1.0**
The backend subscribes to HiveMQ Cloud over TLS as a background `asyncio` task. Each incoming message is validated with a Pydantic v2 schema before persistence, and malformed messages are logged and discarded without crashing the service.

**Alert Engine**
A two-tier (WARNING / CRITICAL) configurable alert engine evaluates every ingested reading against per-device thresholds stored in the `device_thresholds` table. Alerts auto-escalate and auto-resolve as values move across threshold boundaries.

**Server-Sent Events (SSE)**
`GET /devices/{id}/stream` provides a live push stream to web browser clients, polling the database every 2 seconds and emitting a JSON event containing telemetry, alert level, active alert count, and current thresholds — all without WebSocket overhead.

**Key Libraries**

| Library | Version | Purpose |
|---------|---------|---------|
| FastAPI | 0.111 | REST API framework |
| SQLAlchemy | 2.0.30 | Async ORM |
| asyncpg | 0.29 | PostgreSQL async driver |
| aiomqtt | 2.1.0 | Async MQTT client |
| Pydantic | 2.7 | Schema validation |
| Uvicorn | 0.29 | ASGI server |
| pytest + httpx | 8.2 / 0.27 | Test suite (24 tests) |

---

## 4. Web Frontend (React Dashboard)

**React 19 + TypeScript + Vite**
The web dashboard is a single-page application providing real-time monitoring for operators. It connects to the backend's SSE stream via a custom `useSSE` hook built on React 19's `useSyncExternalStore`, updating device cards live as new telemetry arrives — no polling required.

**Key Libraries**

| Library | Purpose |
|---------|---------|
| React 19 + React Router 7 | UI framework and client-side routing |
| Recharts 3 | Interactive history line charts |
| Axios 1.20 | REST API HTTP client |
| Lucide React | Icon set |
| date-fns 4 | Timestamp formatting |
| TypeScript 6 | Compile-time type safety |
| Vite 8 | Build tool and HMR dev server |
| Oxlint | Fast Rust-based linter |

---

## 5. Mobile Application (Android)

**Native Android — Java 8 + Kotlin, Material Components 3**
The Android application provides field-level monitoring for farmers. It targets API 21+ (Android 5.0) for broad device compatibility and uses standard Jetpack architecture patterns.

**Key Libraries**

| Library | Purpose |
|---------|---------|
| Retrofit 2 + OkHttp 4 | Typed HTTP REST client |
| Gson | JSON serialization |
| MPAndroidChart 3.1 | 24-hour sensor history line chart |
| Lottie 6 | Splash screen animation |
| Jetpack Navigation | Fragment navigation |
| SharedPreferences | Local settings persistence |

---

## 6. Development Infrastructure

| Tool | Purpose |
|------|---------|
| Arduino IDE 2.x | ESP32-S3 firmware compilation and flashing |
| Docker Compose | Local PostgreSQL development instance |
| HiveMQ Cloud | Managed MQTT broker (free tier, TLS) |
| MQTT Explorer | MQTT topic debugging and payload inspection |
| Postman / FastAPI /docs | REST API testing (Swagger UI auto-generated) |
| GitHub | Version control and CI |
| IBM Bob | AI-assisted development across all four layers |

---

## 7. IBM Bob — AI-Assisted Development

IBM Bob was the primary development accelerator across the entire project. Its contributions spanned architecture, implementation, debugging, and documentation across all four technology layers:

- **Architecture Planning**: Bob produced the complete multi-layer system design, MQTT topic specification, database schema, and API contract before implementation began — ensuring all components shared a consistent data model from day one.
- **Firmware**: Bob designed the modular C++ manager architecture, derived the MQ voltage-divider safety formula, implemented the non-blocking GPS feed, designed the 3-tier timestamp fallback (GPS → NTP → RTC), and specified the PSRAM offline buffer strategy.
- **Backend**: Bob identified and fixed the critical `str(bytes)` MQTT decoding bug (which would have silently dropped all telemetry), designed the two-tier alert escalation logic, made the SSE generator connection-pool safe, and wrote idempotent startup schema migrations.
- **Frontend**: Bob designed the `useSyncExternalStore`-based SSE hook eliminating stale closure issues, generated all TypeScript interface types mirroring the Pydantic schemas, and computed the SVG arc math for the radial gauge component.
- **Android**: Bob applied the `onDestroyView` ViewBinding lifecycle pattern across all fragments, corrected the Retrofit return type for the PATCH endpoint, and resolved Gradle dependency version conflicts.
- **Documentation**: Bob authored all eight project documentation files — wiring guides, API references, deployment procedures, configuration guides, and this technology document — grounded in the actual codebase rather than generic templates.

IBM Bob made a four-layer IoT system achievable within the project timeline by reasoning simultaneously across embedded C++, Python async, TypeScript, and Java — enforcing consistency at every API and protocol boundary.
