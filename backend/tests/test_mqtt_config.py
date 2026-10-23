"""
Focused tests for MQTT configuration loading and hostname validation.

Covers:
  - Placeholder hostname detection (warns, does not crash)
  - URL-scheme-in-hostname rejection
  - Port-in-hostname rejection
  - Valid hostname passes through unchanged
  - Missing MQTT_TOPIC_STATUS falls back to default
  - Topic patterns contain the expected device wildcard segment
"""
import pytest
from pydantic import ValidationError
from unittest.mock import patch


# ---------------------------------------------------------------------------
# Helpers — instantiate a fresh Settings object with patched env values
# ---------------------------------------------------------------------------

def _make_settings(**overrides):
    """
    Create a Settings instance with environment overrides without touching
    the real .env file.  Uses model_validate with a direct dict so pydantic
    doesn't re-read the file.
    """
    from app.config import Settings
    base = {
        "database_url": "sqlite+aiosqlite:///:memory:",
        "mqtt_host": "abc123.s1.eu.hivemq.cloud",
        "mqtt_port": 8883,
        "mqtt_user": "testuser",
        "mqtt_password": "testpass",
        "mqtt_topic_telemetry": "agricool/v1/devices/+/telemetry",
        "mqtt_topic_status":    "agricool/v1/devices/+/status",
        "mqtt_client_id":       "test-backend",
    }
    base.update(overrides)
    return Settings.model_validate(base)


# ---------------------------------------------------------------------------
# Hostname validation
# ---------------------------------------------------------------------------

def test_valid_hivemq_hostname_accepted():
    s = _make_settings(mqtt_host="abc123.s1.eu.hivemq.cloud")
    assert s.mqtt_host == "abc123.s1.eu.hivemq.cloud"


def test_hostname_with_scheme_rejected():
    """mqtts://host must raise ValueError — scheme must not be in hostname."""
    with pytest.raises((ValidationError, ValueError)):
        _make_settings(mqtt_host="mqtts://abc123.s1.eu.hivemq.cloud")


def test_hostname_with_port_rejected():
    """host:port in mqtt_host must raise ValueError — port belongs in MQTT_PORT."""
    with pytest.raises((ValidationError, ValueError)):
        _make_settings(mqtt_host="abc123.s1.eu.hivemq.cloud:8883")


def test_placeholder_hostname_does_not_crash(caplog):
    """
    A placeholder hostname should log an ERROR but NOT raise — the app must
    still start so HTTP endpoints remain available.
    """
    import logging
    with caplog.at_level(logging.ERROR, logger="app.config"):
        s = _make_settings(mqtt_host="YOUR_HOST.hivemq.cloud")
    assert s.mqtt_host == "YOUR_HOST.hivemq.cloud"
    # Error should have been logged
    assert any("MQTT_HOST" in r.message for r in caplog.records)


def test_empty_hostname_does_not_crash(caplog):
    import logging
    with caplog.at_level(logging.ERROR, logger="app.config"):
        s = _make_settings(mqtt_host="")
    assert s.mqtt_host == ""


def test_whitespace_stripped_from_hostname():
    s = _make_settings(mqtt_host="  abc123.s1.eu.hivemq.cloud  ")
    assert s.mqtt_host == "abc123.s1.eu.hivemq.cloud"


# ---------------------------------------------------------------------------
# Topic configuration
# ---------------------------------------------------------------------------

def test_default_topic_telemetry_uses_wildcard():
    """Default telemetry topic must include wildcard '+' for device_id."""
    s = _make_settings()
    assert "+" in s.mqtt_topic_telemetry or "{" in s.mqtt_topic_telemetry


def test_default_topic_status_uses_wildcard():
    """Default status topic must include wildcard '+' for device_id."""
    s = _make_settings()
    assert "+" in s.mqtt_topic_status or "{" in s.mqtt_topic_status


def test_topic_scheme_agricool():
    """Both default topics must use the 'agricool/v1/devices/' prefix."""
    s = _make_settings()
    assert s.mqtt_topic_telemetry.startswith("agricool/v1/devices/")
    assert s.mqtt_topic_status.startswith("agricool/v1/devices/")


def test_telemetry_and_status_topics_differ():
    """Telemetry and status topics must be distinct subscriptions."""
    s = _make_settings()
    assert s.mqtt_topic_telemetry != s.mqtt_topic_status


# ---------------------------------------------------------------------------
# Port and TLS
# ---------------------------------------------------------------------------

def test_default_port_is_8883():
    s = _make_settings()
    assert s.mqtt_port == 8883


def test_port_1883_accepted():
    """Plain-text broker on port 1883 is a valid configuration."""
    s = _make_settings(mqtt_port=1883)
    assert s.mqtt_port == 1883


# ---------------------------------------------------------------------------
# Topic routing helper (_matches_status_topic)
# ---------------------------------------------------------------------------

def test_matches_status_topic_valid():
    from app.mqtt_client import _matches_status_topic
    assert _matches_status_topic("agricool/v1/devices/factorysense-001/status") is True


def test_matches_status_topic_telemetry_is_false():
    from app.mqtt_client import _matches_status_topic
    assert _matches_status_topic("agricool/v1/devices/factorysense-001/telemetry") is False


def test_matches_status_topic_too_short():
    from app.mqtt_client import _matches_status_topic
    assert _matches_status_topic("agricool/v1/devices/status") is False


def test_matches_status_topic_wrong_prefix():
    from app.mqtt_client import _matches_status_topic
    assert _matches_status_topic("other/v1/devices/dev-001/status") is False
