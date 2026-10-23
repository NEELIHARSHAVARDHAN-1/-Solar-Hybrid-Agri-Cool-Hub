# Backend — Complete Documentation
## Solar Hybrid Agri Cool Hub

---

## Table of Contents

1. [Overview](#overview)
2. [Features](#features)
3. [How IBM Bob Was Used in Building](#how-ibm-bob-was-used-in-building)
4. [APIs](#apis)
5. [Protocols](#protocols)
6. [Challenges and How Bob Overcame Them](#challenges-and-how-bob-overcame-them)
7. [Feature Enhancements](#feature-enhancements)
8. [Conclusion](#conclusion)
9. [Setup Guide](#setup-guide)
10. [Run Procedures](#run-procedures)

---

## Overview

The **Agri Cool Hub Backend** is an asynchronous Python API server built with
**FastAPI** and **SQLAlchemy 2.x async** (asyncpg driver on PostgreSQL). It serves as
the central hub of the entire IoT system:

- **MQTT Bridge** — subscribes to HiveMQ Cloud, ingests ESP32 sensor telemetry.
- **Alert Engine** — two-tier (WARNING / CRITICAL) configurable threshold evaluation.
- **REST API** — serves all data to the Android app and web frontend.
- **SSE Stream** — pushes real-time telemetry updates to connected web clients.
- **Threshold Management** — per-device configurable warning and critical levels via API.

The backend runs as a single `uvicorn` process, launching the MQTT listener as a
background `asyncio` task alongside the FastAPI HTTP server. It uses PostgreSQL as its
primary database, with an in-memory SQLite database available for test runs.

---

## Features

### 1. MQTT Telemetry Ingestion

- Subscribes to `agrihub/telemetry` (QoS 1) on HiveMQ Cloud over TLS (port 8883).
- Subscribes to `agricool/v1/devices/+/status` for device online/offline status messages.
- Each incoming message is:
  1. Decoded from bytes.
  2. JSON-parsed (`json.loads`).
  3. Validated with the `TelemetryPayload` Pydantic schema.
  4. Persisted to PostgreSQL via `ingest_telemetry()`.
- Invalid JSON or schema violations are logged at WARNING level and discarded — they
  never crash the listener.
- **Duplicate ingestion protection:** `INSERT ... ON CONFLICT (device_id, timestamp) DO NOTHING`
  silently drops replayed readings from the SD card queue.

### 2. Two-Tier Alert Engine

- Evaluates every successfully ingested telemetry reading against per-device thresholds.
- **Tier 1 — WARNING:** value crosses the warning threshold.
- **Tier 2 — CRITICAL:** value crosses the critical threshold (subset of warning breaches).
- Alert types: `TEMP_HIGH`, `HUMIDITY_HIGH`, `MQ3_HIGH`, `MQ135_HIGH`, `MQ4_HIGH`.
- **No duplicate open alerts:** if an open alert of the same type exists for a device,
  the engine updates its level (escalation) rather than inserting a new row.
- **Auto-resolution:** when a value drops below both thresholds, any open alert for that
  type is automatically resolved with `resolved_at = now()`.
- **Per-device thresholds:** stored in the `device_thresholds` table; defaults are
  auto-created on first telemetry from a new device.

### 3. REST API (FastAPI)

Full REST API with automatic interactive documentation at `/docs` (Swagger UI) and
`/redoc` (ReDoc). See the [APIs](#apis) section for the complete endpoint reference.

### 4. Server-Sent Events (SSE) Stream

- `GET /devices/{device_id}/stream` provides a live push stream for the web frontend.
- Polls the database every 2 seconds and emits a JSON event when new telemetry arrives.
- Each event contains: telemetry values, alert level, active alert count, and current
  threshold configuration.
- Uses FastAPI `StreamingResponse` with `media_type="text/event-stream"`.
- `X-Accel-Buffering: no` header disables nginx buffering for live delivery.

### 5. Device Registry

- Devices are auto-registered on first telemetry receipt — no manual provisioning needed.
- `is_online` status is set to `True` on each telemetry ingest and updated by the
  status MQTT topic (`ONLINE`/`OFFLINE` payloads).
- A background staleness check marks devices offline when `last_seen` exceeds
  `DEVICE_OFFLINE_MINUTES` (default 5 minutes).

### 6. Per-Device Threshold Configuration

- `GET /devices/{id}/thresholds` — fetch current thresholds.
- `PUT /devices/{id}/thresholds` — update all 10 threshold values (5 sensors × 2 tiers)
  in a single call.
- Thresholds are validated (warning must be ≤ critical) by the Pydantic schema.
- Defaults: Temp 25°C warning / 30°C critical; Humidity 70%/85%; MQ sensors 20/30,
  25/40, 20/35 (firmware 0–100 scale).

### 7. Database Schema

Four tables:

| Table | Purpose |
|-------|---------|
| `devices` | Device registry with `last_seen`, `is_online` |
| `telemetry` | All sensor readings, unique on `(device_id, timestamp)` |
| `alerts` | Alert history with `alert_level`, `alert_type`, `resolved_at` |
| `device_thresholds` | Per-device configurable warning + critical thresholds |

### 8. Idempotent Schema Migrations

- On every startup, the backend runs `DO $$ ... $$` PostgreSQL migrations that are
  safe to run repeatedly (check-before-alter).
- Adds `alert_level` and `metric` columns if missing (pre-v2 database upgrade).
- Fixes MQ threshold rows accidentally set to voltage scale.
- No separate Alembic migration command needed — migrations are automatic.

### 9. Health Check

- `GET /health` returns `{"status": "ok", "mqtt_connected": true/false}`.
- Reports MQTT connection state so load balancers and monitoring tools can check
  the backend's IoT bridge status.

---

## How IBM Bob Was Used in Building

1. **FastAPI Project Structure** — Bob established the `app/` layout with `main.py`,
   `config.py`, `database.py`, `models.py`, `schemas.py`, `services.py`, `mqtt_client.py`,
   and the `routers/` sub-package, following FastAPI best practices for project layout.

2. **Async SQLAlchemy 2.x Setup** — Bob wrote `database.py` with the async engine,
   session factory, and the `get_db` dependency, correctly using `asyncpg` as the
   driver and handling engine disposal on shutdown.

3. **Pydantic v2 Schemas** — Bob wrote all Pydantic v2 schemas (`TelemetryPayload`,
   `TelemetryOut`, `AlertOut`, `ThresholdSet`, etc.) with correct field aliases,
   optional nullable fields, and validator patterns for the `battery` / `battery_percent`
   dual-field compatibility.

4. **MQTT Listener with Exponential Backoff** — Bob designed the `mqtt_listener()`
   coroutine with the `while True` reconnect loop, doubling `reconnect_delay` from 5 s
   up to 120 s, and resetting it on successful connect. Bob also handled the critical
   bug where `str(bytes_payload)` produces Python repr (`b'...'`) instead of decoded
   text — fixing it to `payload.decode()`.

5. **Two-Tier Alert Engine** — Bob specified and implemented the escalation logic in
   `evaluate_alerts()`: checking critical before warning, fetching the existing open
   alert by `(device_id, alert_type)`, and updating `alert_level` on escalation rather
   than duplicating the alert row. Bob also implemented the auto-resolve path in
   `_auto_resolve()`.

6. **SSE Generator** — Bob wrote the `_sse_generator()` async generator, including the
   session-per-poll pattern (open session → query → close session → sleep) to avoid
   holding a database connection during the 2-second idle period between polls. This
   is critical for connection pool health under many concurrent SSE clients.

7. **PostgreSQL Upsert Pattern** — Bob used `sqlalchemy.dialects.postgresql.insert`
   with `.on_conflict_do_update()` for device upserts and `.on_conflict_do_nothing()`
   for telemetry deduplication — the correct PostgreSQL-native approach vs the
   read-then-write anti-pattern.

8. **Idempotent Migrations** — When the database schema needed the `alert_level` and
   `metric` columns added post-deployment without data loss, Bob wrote the `DO $$ ... $$`
   PL/pgSQL blocks with `IF NOT EXISTS` checks, integrated them into the FastAPI lifespan
   startup handler.

9. **Test Suite** — Bob scaffolded the pytest test suite with an async SQLite in-memory
   database fixture, `pytest-asyncio` configuration, and `httpx.AsyncClient` for API
   endpoint testing — all 24 tests pass without any real PostgreSQL or MQTT broker.

10. **CORS and Router Registration** — Bob registered all routers with correct prefix
    and tag assignments in `main.py`, added `CORSMiddleware` with `allow_origins=["*"]`
    for development, and included the SSE router at the top level (no prefix) since its
    path already starts with `/devices/`.

---

## APIs

Base URL: `http://YOUR_BACKEND_HOST:8000`

Interactive docs: `http://YOUR_BACKEND_HOST:8000/docs`

### Health

| Method | Path | Description | Response |
|--------|------|-------------|---------|
| `GET` | `/health` | Service + MQTT health check | `{"status":"ok","mqtt_connected":true}` |

### Devices

| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/devices` | List all devices with online status |
| `GET` | `/devices/{device_id}` | Single device detail |
| `GET` | `/devices/{device_id}/status` | Latest reading + online flag |

**Device Object:**
```json
{
  "device_id": "agrihub-001",
  "last_seen": "2026-10-21T10:30:00+00:00",
  "is_online": true,
  "created_at": "2026-10-01T08:00:00+00:00"
}
```

### Telemetry

| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/devices/{device_id}/telemetry` | Paginated history (`?limit&offset&from&to`) |
| `GET` | `/devices/{device_id}/telemetry/latest` | Most recent single reading |

**Telemetry Object:**
```json
{
  "id": 142,
  "device_id": "agrihub-001",
  "timestamp": "2026-10-21T10:30:00+05:30",
  "temperature": 25.6,
  "humidity": 72.4,
  "mq3": 412,
  "mq135": 530,
  "mq4": 287,
  "battery_percent": 78,
  "latitude": 15.8281,
  "longitude": 78.0373,
  "received_at": "2026-10-21T05:00:10+00:00"
}
```

### Alerts

| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/devices/{device_id}/alerts` | Paginated alert history |
| `GET` | `/devices/{device_id}/alerts/active` | Unresolved alerts only |
| `GET` | `/alerts` | Cross-device alert list (all devices) |
| `PATCH` | `/alerts/{alert_id}/resolve` | Resolve an alert |

**Alert Object:**
```json
{
  "id": 7,
  "device_id": "agrihub-001",
  "timestamp": "2026-10-21T10:30:00+05:30",
  "alert_level": "critical",
  "alert_type": "TEMP_HIGH",
  "metric": "Temperature (°C)",
  "value": 37.2,
  "threshold": 30.0,
  "resolved_at": null,
  "created_at": "2026-10-21T05:00:10+00:00"
}
```

**Alert Types and Triggers:**

| Type | Triggered When |
|------|---------------|
| `TEMP_HIGH` | temperature > warning/critical temp threshold |
| `HUMIDITY_HIGH` | humidity > warning/critical humidity threshold |
| `MQ3_HIGH` | mq3 > warning/critical mq3 threshold |
| `MQ135_HIGH` | mq135 > warning/critical mq135 threshold |
| `MQ4_HIGH` | mq4 > warning/critical mq4 threshold |

### Thresholds

| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/devices/{device_id}/thresholds` | Get per-device threshold config |
| `PUT` | `/devices/{device_id}/thresholds` | Update threshold config |

**Threshold Object:**
```json
{
  "device_id": "agrihub-001",
  "temp_warning": 25.0,
  "temp_critical": 30.0,
  "humidity_warning": 70.0,
  "humidity_critical": 85.0,
  "mq3_warning": 20.0,
  "mq3_critical": 30.0,
  "mq135_warning": 25.0,
  "mq135_critical": 40.0,
  "mq4_warning": 20.0,
  "mq4_critical": 35.0,
  "updated_at": "2026-10-21T10:00:00+00:00"
}
```

### SSE Stream

| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/devices/{device_id}/stream` | Live SSE push stream |

Connect with `EventSource('/devices/agrihub-001/stream')`. Emits a JSON event every
~2 seconds when new telemetry is available, containing telemetry, alert level, active
alert count, and current thresholds.

---

## Protocols

### MQTT over TLS (IoT Ingestion)

- **Protocol:** MQTT v3.1.1 over TLS 1.2/1.3
- **Broker:** HiveMQ Cloud (free tier)
- **Port:** 8883 (TLS); 1883 (plain, local only)
- **QoS:** Level 1 (at-least-once delivery)
- **Topics subscribed:**
  - `agrihub/telemetry` — sensor telemetry payloads from ESP32
  - `agricool/v1/devices/+/status` — device online/offline status
- **Authentication:** username + password (configured in `.env`)
- **TLS:** `ssl.create_default_context()` — uses the system's trust store; HiveMQ Cloud
  uses publicly trusted Let's Encrypt certificates.
- **Reconnect:** exponential backoff (5 s → 10 s → … → 120 s max).
- **Library:** `aiomqtt` 2.1.0 (async Python MQTT client built on `paho-mqtt`).

### HTTP/1.1 + HTTP/2 (REST API)

- **Framework:** FastAPI 0.111 on Uvicorn ASGI server.
- **Content-Type:** `application/json` for all endpoints.
- **CORS:** enabled for all origins in development (`allow_origins=["*"]`).
  Restrict to specific origins in production.
- **Auto-generated OpenAPI spec** at `/openapi.json`.

### Server-Sent Events (SSE) (Real-Time Push)

- **Protocol:** HTTP/1.1 persistent connection with chunked transfer encoding.
- **Content-Type:** `text/event-stream`.
- **Event format:** `data: {...JSON...}\n\n`
- **Poll interval:** 2 seconds.
- **Client reconnection:** handled automatically by the browser `EventSource` API.
- **Nginx note:** `X-Accel-Buffering: no` header disables proxy buffering.

### PostgreSQL (Database)

- **Driver:** asyncpg (async, non-blocking).
- **ORM:** SQLAlchemy 2.x with mapped class syntax.
- **Connection pooling:** SQLAlchemy's built-in async pool.
- **Upserts:** `INSERT ... ON CONFLICT DO UPDATE/NOTHING` (PostgreSQL dialect).
- **Migrations:** idempotent `DO $$ ... $$` PL/pgSQL blocks run at startup.

---

## Challenges and How Bob Overcame Them

### Challenge 1 — `bytes.decode()` vs `str(bytes)` in MQTT Payload
**Problem:** `aiomqtt` delivers `message.payload` as `bytes`. Using Python's `str()`
on bytes produces `"b'...'"` (Python repr notation), which is invalid JSON and causes
`json.JSONDecodeError` for every incoming message.

**Bob's Solution:** Bob identified the root cause and applied the explicit decode:
```python
raw = (
    message.payload.decode()
    if isinstance(message.payload, (bytes, bytearray))
    else str(message.payload)
)
```
This was applied consistently in both the telemetry and status message handlers.

### Challenge 2 — Database Connection Leak in SSE Generator
**Problem:** An early version of `_sse_generator()` held an open SQLAlchemy session
across the `asyncio.sleep(2)` between polls. Under many concurrent SSE clients, this
exhausted the connection pool, causing new requests to stall.

**Bob's Solution:** Bob restructured the generator to open a session, perform all queries,
commit the session context manager, and only then yield the event and sleep. This ensures
connections are returned to the pool during the idle period.

### Challenge 3 — Alert Escalation vs Duplication
**Problem:** When a temperature reading escalated from WARNING to CRITICAL, the naive
implementation inserted a second alert row, leaving two open alerts for the same device
and alert type.

**Bob's Solution:** Bob wrote the "fetch-then-update-or-insert" pattern in
`evaluate_alerts()`: fetch the existing open alert for `(device_id, alert_type)`, and if
it exists, update `alert_level` and `threshold` in place rather than creating a new row.
This keeps exactly one open alert per type per device at all times.

### Challenge 4 — Dual `battery` / `battery_percent` Field Names
**Problem:** The firmware payload used `battery_percent` but some versions of the
firmware sent `battery`. The Pydantic schema needed to accept both field names without
breaking existing deployments.

**Bob's Solution:** Bob added both fields as optional to `TelemetryPayload` and resolved
them in `ingest_telemetry()`:
```python
battery = payload.battery if payload.battery is not None else payload.battery_percent
```
This ensures backwards compatibility without schema versioning.

### Challenge 5 — SQLite Compatibility for Tests
**Problem:** The PostgreSQL-specific `INSERT ... ON CONFLICT DO NOTHING` (using
`sqlalchemy.dialects.postgresql.insert`) does not work with SQLite, which is used in
the test suite.

**Bob's Solution:** Bob added a `_is_postgres()` helper and provided separate code paths
for Postgres (native upsert) and SQLite (read-then-write fallback). Test runs use SQLite
transparently; production uses the optimised PostgreSQL path.

### Challenge 6 — Schema Migrations Without Downtime
**Problem:** After the `alert_level` and `metric` columns were added to the alerts table
in v2, existing production databases failed to start because the ORM expected the columns.

**Bob's Solution:** Bob wrote `_COLUMN_MIGRATIONS` — a list of `DO $$ ... $$` PL/pgSQL
blocks with `IF NOT EXISTS` guards — executed automatically on each startup after
`create_all`. This brought pre-v2 databases up to date on first restart, with no data
loss and no manual SQL required.

---

## Feature Enhancements

1. **JWT Authentication** — Add `POST /auth/login` returning a JWT, and protect all
   write endpoints with `Depends(get_current_user)`. The mobile app and frontend would
   include `Authorization: Bearer <token>` in every request.

2. **WebSocket Upgrade for SSE** — Replace the SSE polling-based generator with a
   broadcast channel (e.g., `asyncio.Queue` per device) so events are pushed immediately
   on MQTT ingestion rather than on a 2-second poll cycle.

3. **Celery / Background Task Queue** — Move alert evaluation to a Celery worker for
   high-throughput deployments (many devices sending frequent telemetry).

4. **Alembic Migrations** — Replace the startup migration blocks with proper Alembic
   revision files for full migration history, rollback support, and team-friendly diff
   management.

5. **Multi-Tenant Device Isolation** — Add a `farm_id` foreign key to the `devices`
   table and enforce row-level security on all queries so different farm operators see
   only their own devices.

6. **Metrics Endpoint (Prometheus)** — Expose `/metrics` with counters for telemetry
   ingestion rate, alert creation rate, MQTT reconnect count, and API request latency.

7. **MQTT Command Publishing** — Implement `POST /devices/{id}/command` that publishes
   to `agrihub/command`, enabling remote firmware commands (e.g., force sensor read,
   SD flush, reboot).

8. **Rate Limiting** — Add `slowapi` rate limiting on the REST API to prevent
   inadvertent flooding from misconfigured firmware or clients.

---

## Conclusion

The Agri Cool Hub Backend is a production-quality async Python service that handles the
full IoT data pipeline: from MQTT ingestion through alert evaluation to REST API and SSE
streaming. Built on FastAPI + SQLAlchemy 2.x + aiomqtt, it achieves high throughput with
low latency using Python's `asyncio` throughout.

IBM Bob's contributions span the entire backend: async SQLAlchemy patterns, MQTT
reconnect logic, two-tier alert escalation, SSE connection pool management, idempotent
migrations, and the PostgreSQL/SQLite dual-path for testing. The result is a 24-test
suite that passes fully on SQLite and a production backend that runs reliably on
PostgreSQL in the cloud.

---

## Setup Guide

### Prerequisites

| Requirement | Version |
|-------------|---------|
| Python | 3.10 or 3.11 |
| PostgreSQL | 14+ (or Docker for local dev) |
| Docker + Docker Compose | Any recent version |
| HiveMQ Cloud account | Free tier |

### Steps

1. **Clone and navigate to the backend**
   ```bash
   cd backend
   ```

2. **Create and activate a virtual environment**
   ```bash
   python -m venv venv
   # Windows:
   venv\Scripts\activate
   # Linux/macOS:
   source venv/bin/activate
   ```

3. **Install dependencies**
   ```bash
   pip install -r requirements.txt
   ```

4. **Configure environment variables**
   ```bash
   cp .env.example .env
   # Edit .env with your real credentials
   ```

   Required `.env` values:
   ```
   DATABASE_URL=postgresql+asyncpg://agrihub:agrihub@localhost:5432/agrihub
   MQTT_HOST=your-cluster.hivemq.cloud
   MQTT_PORT=8883
   MQTT_USER=your_mqtt_user
   MQTT_PASSWORD=your_mqtt_password
   MQTT_TOPIC_TELEMETRY=agrihub/telemetry
   MQTT_CLIENT_ID=agrihub-backend
   ```

5. **Start PostgreSQL (local development)**
   ```bash
   docker-compose up -d
   # Starts postgres:15 on localhost:5432
   # Database: agrihub, user: agrihub, password: agrihub
   ```

6. **Start the backend**
   ```bash
   uvicorn app.main:app --reload --host 0.0.0.0 --port 8000
   ```
   Tables are created automatically on first start. Migrations run automatically.

---

## Run Procedures

### Development Server (with auto-reload)

```bash
cd backend
uvicorn app.main:app --reload --host 0.0.0.0 --port 8000
```

### Production Server

```bash
cd backend
uvicorn app.main:app --host 0.0.0.0 --port 8000 --workers 1
# Use 1 worker; the MQTT listener is a single asyncio task — do not use multiple workers
# unless you move the MQTT listener to a separate process.
```

### Run Tests

```bash
cd backend
pytest tests/ -v
# Expected: 24 passed
# Uses in-memory SQLite — no PostgreSQL or MQTT broker required
```

### Run Tests with Coverage

```bash
cd backend
pytest tests/ -v --tb=short --cov=app --cov-report=term-missing
```

### Verify Backend Health

```bash
curl http://localhost:8000/health
# Expected: {"status":"ok","mqtt_connected":true}

curl http://localhost:8000/devices
# Expected: [] (empty until a device connects)
```

### Interactive API Documentation

Open in browser: `http://localhost:8000/docs`

### Stop PostgreSQL

```bash
cd backend
docker-compose down
```

### Troubleshooting

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| `asyncpg.exceptions.ConnectionDoesNotExistError` | PostgreSQL not running | `docker-compose up -d` |
| `MQTT connection lost: Connection refused` | Wrong MQTT host/port | Check `MQTT_HOST` and `MQTT_PORT` in `.env` |
| `MQTT connection lost: Not authorised` | Wrong credentials | Check `MQTT_USER` and `MQTT_PASSWORD` |
| Tables not created | `create_all` failed | Check `DATABASE_URL` format (must use `+asyncpg` scheme) |
| Tests fail with `ModuleNotFoundError` | venv not activated | Run `venv\Scripts\activate` |
| Port 8000 already in use | Another process running | Use `--port 8001` or kill the other process |
