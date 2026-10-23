# API Reference — Solar Hybrid Agri Cool Hub Backend

Base URL: `http://YOUR_BACKEND_HOST:8000`

All responses are JSON. All timestamps are ISO-8601 with timezone.

---

## Health

### GET /health

Returns service health status.

**Response 200:**
```json
{"status": "ok"}
```

---

## Devices

### GET /devices

List all known devices with online status.

**Response 200:**
```json
[
  {
    "device_id": "agrihub-001",
    "last_seen": "2026-10-21T10:30:00+00:00",
    "is_online": true,
    "created_at": "2026-10-01T08:00:00+00:00"
  }
]
```

---

### GET /devices/{device_id}

Get a single device by ID.

**Response 200:**
```json
{
  "device_id": "agrihub-001",
  "last_seen": "2026-10-21T10:30:00+00:00",
  "is_online": true,
  "created_at": "2026-10-01T08:00:00+00:00"
}
```

**Response 404:** Device not found.

---

### GET /devices/{device_id}/status

Get device status including the latest telemetry reading.

**Response 200:**
```json
{
  "device_id": "agrihub-001",
  "is_online": true,
  "last_seen": "2026-10-21T10:30:00+00:00",
  "latest": {
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
}
```

`is_online` is `true` when `last_seen` is within the last 5 minutes (configurable).

---

## Telemetry

### GET /devices/{device_id}/telemetry

Paginated telemetry history.

**Query Parameters:**
| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `limit` | int | 50 | Max rows to return (1–500) |
| `offset` | int | 0 | Rows to skip |
| `from` | datetime | — | Filter from timestamp (ISO-8601) |
| `to` | datetime | — | Filter to timestamp (ISO-8601) |

**Example:** `GET /devices/agrihub-001/telemetry?limit=10&from=2026-10-21T00:00:00Z`

**Response 200:**
```json
{
  "total": 288,
  "items": [
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
  ]
}
```

---

### GET /devices/{device_id}/telemetry/latest

Most recent telemetry row.

**Response 200:** Single `TelemetryOut` object (same shape as items above).

**Response 404:** No data found.

---

## Alerts

### GET /devices/{device_id}/alerts

Paginated alert history (all alerts, resolved and active).

**Query Parameters:** `limit`, `offset` (same as telemetry)

**Response 200:**
```json
{
  "total": 3,
  "items": [
    {
      "id": 7,
      "device_id": "agrihub-001",
      "timestamp": "2026-10-21T10:30:00+05:30",
      "alert_type": "TEMP_HIGH",
      "value": 37.2,
      "threshold": 35.0,
      "resolved_at": null,
      "created_at": "2026-10-21T05:00:10+00:00"
    }
  ]
}
```

**Alert Types:**
| Type | Triggered By |
|------|-------------|
| `TEMP_HIGH` | temperature > ALERT_TEMP_MAX |
| `HUMIDITY_HIGH` | humidity > ALERT_HUMIDITY_MAX |
| `MQ3_HIGH` | mq3 > ALERT_MQ3_MAX |
| `MQ135_HIGH` | mq135 > ALERT_MQ135_MAX |
| `MQ4_HIGH` | mq4 > ALERT_MQ4_MAX |

---

### GET /devices/{device_id}/alerts/active

Unresolved alerts only (`resolved_at` is null).

**Response 200:** Same shape as `/alerts` but only active items.

---

### PATCH /alerts/{alert_id}/resolve

Mark an alert as resolved.

**Response 200:** Updated alert object with `resolved_at` set to current UTC time.

**Response 404:** Alert not found.

**Response 400:** Alert already resolved.

---

## MQTT Topics

| Topic | Direction | Publisher | Payload |
|-------|-----------|-----------|---------|
| `agrihub/telemetry` | ESP32 → Backend | Firmware | Telemetry JSON (see below) |
| `agrihub/command` | Backend → ESP32 | Backend (reserved) | TBD |

QoS: 1 (at-least-once). Retained: false.

### Telemetry Payload Schema

```json
{
  "device_id": "agrihub-001",
  "timestamp": "2026-10-21T10:30:00+05:30",
  "temperature": 25.6,
  "humidity": 72.4,
  "mq3": 412,
  "mq135": 530,
  "mq4": 287,
  "battery_percent": 78,
  "latitude": 15.8281,
  "longitude": 78.0373
}
```

**Field Rules:**
- `temperature` / `humidity`: `null` if SHT40 read failed.
- `battery_percent`: integer 0–100 when INA219 available, `null` otherwise.
- `latitude` / `longitude`: static configured values; `null` if not configured.
- `humidifier_enabled`: omitted (actuator not installed in this deployment).
- `mq3` / `mq135` / `mq4`: raw 12-bit ADC integers (0–4095). Not calibrated.
