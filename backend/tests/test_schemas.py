import pytest
from datetime import datetime, timezone
from pydantic import ValidationError
from app.schemas import TelemetryPayload


# MQ values are on the firmware's 0–100 custom scale (% of sensor range).
# These are the real values observed from the ESP32 via MQTT Explorer.
VALID_PAYLOAD = {
    "device_id": "agrihub-001",
    "timestamp": "2026-10-21T10:30:00+05:30",
    "temperature": 25.6,
    "humidity": 72.4,
    "mq3": 23.84,
    "mq135": 16.08,
    "mq4": 14.53,
    "battery_percent": 78,
    "latitude": 15.8281,
    "longitude": 78.0373,
}


def test_valid_payload():
    p = TelemetryPayload(**VALID_PAYLOAD)
    assert p.device_id == "agrihub-001"
    assert p.temperature == 25.6
    assert p.battery_percent == 78


def test_null_battery():
    data = {**VALID_PAYLOAD, "battery_percent": None}
    p = TelemetryPayload(**data)
    assert p.battery_percent is None


def test_missing_battery_defaults_none():
    data = {k: v for k, v in VALID_PAYLOAD.items() if k != "battery_percent"}
    p = TelemetryPayload(**data)
    assert p.battery_percent is None


def test_null_temperature():
    data = {**VALID_PAYLOAD, "temperature": None, "humidity": None}
    p = TelemetryPayload(**data)
    assert p.temperature is None
    assert p.humidity is None


def test_null_coordinates():
    data = {**VALID_PAYLOAD, "latitude": None, "longitude": None}
    p = TelemetryPayload(**data)
    assert p.latitude is None


def test_humidifier_field_accepted_and_ignored():
    data = {**VALID_PAYLOAD, "humidifier_enabled": True}
    p = TelemetryPayload(**data)
    assert p.humidifier_enabled is True


def test_invalid_temperature_too_high():
    data = {**VALID_PAYLOAD, "temperature": 200.0}
    with pytest.raises(ValidationError):
        TelemetryPayload(**data)


def test_invalid_mq_negative():
    data = {**VALID_PAYLOAD, "mq3": -0.01}
    with pytest.raises(ValidationError):
        TelemetryPayload(**data)


def test_invalid_mq_over_range():
    # Firmware scale is 0–100; anything above 100 must be rejected
    data = {**VALID_PAYLOAD, "mq135": 100.01}
    with pytest.raises(ValidationError):
        TelemetryPayload(**data)


def test_mq_at_full_scale_accepted():
    # Values at exactly 100 (full scale) must be accepted
    data = {**VALID_PAYLOAD, "mq3": 100.0, "mq135": 99.99, "mq4": 0.0}
    p = TelemetryPayload(**data)
    assert p.mq3 == 100.0
    assert p.mq135 == 99.99


def test_mq_real_firmware_values_accepted():
    # Real payload from MQTT Explorer (2026-10-23): must pass validation
    data = {**VALID_PAYLOAD, "mq3": 23.83736, "mq135": 16.07692, "mq4": 14.52967}
    p = TelemetryPayload(**data)
    assert round(p.mq3, 2) == 23.84


def test_invalid_battery_over_100():
    data = {**VALID_PAYLOAD, "battery_percent": 150}
    with pytest.raises(ValidationError):
        TelemetryPayload(**data)


def test_missing_required_field():
    data = {k: v for k, v in VALID_PAYLOAD.items() if k != "mq3"}
    with pytest.raises(ValidationError):
        TelemetryPayload(**data)
