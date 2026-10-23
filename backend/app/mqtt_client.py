"""
MQTT Bridge — IoT device → Backend Database
============================================
Subscribes to the HiveMQ Cloud telemetry topic, validates each incoming
JSON payload against TelemetryPayload, and persists it to the database via
ingest_telemetry().

Connection lifecycle
--------------------
* TLS is enabled unconditionally when mqtt_port == 8883 (HiveMQ Cloud free
  tier).  For a plain-text local broker on port 1883 set MQTT_PORT=1883 and
  TLS will be skipped automatically.
* Exponential back-off on disconnect: 5 s → 10 s → … → 120 s max.
* The module-level `mqtt_connected` flag is updated on every state change so
  the health router can surface it without importing aiomqtt.
"""

import asyncio
import json
import logging
import ssl
from datetime import datetime, timezone

import aiomqtt

from pydantic import ValidationError

from .config import settings
from .database import get_session_factory
from .schemas import TelemetryPayload
from .services import ingest_telemetry, update_device_status

logger = logging.getLogger(__name__)

# ---------------------------------------------------------------------------
# Shared connection state — read by the health router
# ---------------------------------------------------------------------------
mqtt_connected: bool = False


async def mqtt_listener() -> None:
    """
    Long-running async task: subscribes to the telemetry topic and ingests
    each message into the database.

    Reconnects automatically with exponential back-off on broker disconnect.
    Runs for the lifetime of the application (launched from FastAPI lifespan).
    """
    global mqtt_connected
    reconnect_delay = 5  # seconds; doubles on each failure up to 120 s

    # Build a TLS context only when connecting to port 8883.
    tls_context: ssl.SSLContext | None = None
    if settings.mqtt_port == 8883:
        tls_context = ssl.create_default_context()
        # HiveMQ Cloud uses publicly-trusted certificates — no custom CA needed.

    while True:
        try:
            logger.info(
                "Connecting to MQTT broker %s:%d (TLS=%s)",
                settings.mqtt_host,
                settings.mqtt_port,
                tls_context is not None,
            )

            client_kwargs = dict(
                hostname=settings.mqtt_host,
                port=settings.mqtt_port,
                username=settings.mqtt_user,
                password=settings.mqtt_password,
                identifier=settings.mqtt_client_id,
                keepalive=10,  # HiveMQ Cloud free tier max keepalive = 10 s
            )
            if tls_context is not None:
                client_kwargs["tls_context"] = tls_context

            async with aiomqtt.Client(**client_kwargs) as client:
                mqtt_connected = True
                logger.info(
                    "MQTT connected. Subscribing to telemetry=%s  status=%s",
                    settings.mqtt_topic_telemetry,
                    settings.mqtt_topic_status,
                )
                await client.subscribe(settings.mqtt_topic_telemetry, qos=1)
                await client.subscribe(settings.mqtt_topic_status, qos=1)
                reconnect_delay = 5  # reset on successful connect

                async for message in client.messages:
                    topic = str(message.topic)
                    # message.payload is bytes — decode to str before parsing.
                    # str(b"...") produces "b'...'" (Python repr), which is NOT
                    # valid JSON.  Always decode explicitly.
                    raw = (
                        message.payload.decode()
                        if isinstance(message.payload, (bytes, bytearray))
                        else str(message.payload)
                    )
                    # Route to appropriate handler based on topic pattern
                    if _matches_status_topic(topic):
                        await _handle_status_message(topic, raw)
                    else:
                        await _handle_message(raw)

        except aiomqtt.MqttError as exc:
            mqtt_connected = False
            logger.warning(
                "MQTT connection lost: %s. Reconnecting in %ds.",
                exc,
                reconnect_delay,
            )
            await asyncio.sleep(reconnect_delay)
            reconnect_delay = min(reconnect_delay * 2, 120)

        except Exception as exc:  # noqa: BLE001
            mqtt_connected = False
            logger.error(
                "Unexpected error in MQTT listener: %s",
                exc,
                exc_info=True,
            )
            await asyncio.sleep(reconnect_delay)
            reconnect_delay = min(reconnect_delay * 2, 120)

        finally:
            mqtt_connected = False


def _matches_status_topic(topic: str) -> bool:
    """
    Returns True if the topic matches agricool/v1/devices/<id>/status.
    Handles both the wildcard subscription and exact topics.
    """
    parts = topic.split("/")
    # Expected: ['agricool', 'v1', 'devices', '<device_id>', 'status']
    return (
        len(parts) == 5
        and parts[0] == "agricool"
        and parts[1] == "v1"
        and parts[2] == "devices"
        and parts[4] == "status"
    )


async def _handle_status_message(topic: str, raw: str) -> None:
    """
    Handle a device status message from topic:
      agricool/v1/devices/<device_id>/status
    Payload: {"status": "ONLINE"} or {"status": "OFFLINE"}
    """
    # Extract device_id from topic path
    parts = topic.split("/")
    device_id = parts[3]

    try:
        data = json.loads(raw)
    except json.JSONDecodeError as exc:
        logger.warning("Invalid JSON in status message topic=%s: %s | %s", topic, raw, exc)
        return

    status_str = data.get("status", "").upper()
    if status_str not in ("ONLINE", "OFFLINE"):
        logger.warning("Unknown status value '%s' on topic %s", status_str, topic)
        return

    is_online = status_str == "ONLINE"

    async with get_session_factory()() as session:
        try:
            await update_device_status(device_id, is_online, session)
            await session.commit()
        except Exception as exc:  # noqa: BLE001
            await session.rollback()
            logger.error("Failed to update device status for %s: %s", device_id, exc, exc_info=True)


async def _handle_message(raw: str) -> None:
    """Parse and ingest one MQTT telemetry message.  Logs and discards invalid payloads."""
    # --- JSON parse -----------------------------------------------------------
    try:
        data = json.loads(raw)
    except json.JSONDecodeError as exc:
        logger.warning("Invalid JSON in MQTT message: %.200s | error: %s", raw, exc)
        return

    # --- Schema validation ----------------------------------------------------
    try:
        payload = TelemetryPayload(**data)
    except ValidationError as exc:
        logger.warning("Telemetry validation failed: %s", exc)
        return

    # --- Persist to database --------------------------------------------------
    async with get_session_factory()() as session:
        try:
            await ingest_telemetry(payload, session)
            await session.commit()
            logger.debug(
                "Ingested telemetry: device=%s ts=%s temp=%.1f hum=%.1f "
                "mq3=%.2f mq135=%.2f mq4=%.2f batt=%s",
                payload.device_id,
                payload.timestamp.isoformat(),
                payload.temperature if payload.temperature is not None else float("nan"),
                payload.humidity if payload.humidity is not None else float("nan"),
                payload.mq3,
                payload.mq135,
                payload.mq4,
                payload.battery,
            )
        except Exception as exc:  # noqa: BLE001
            await session.rollback()
            logger.error("Failed to ingest telemetry: %s", exc, exc_info=True)
