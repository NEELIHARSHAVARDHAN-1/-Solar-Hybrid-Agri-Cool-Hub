"""
Server-Sent Events (SSE) router — real-time telemetry and alert push.

Clients connect to:
  GET /devices/{device_id}/stream   — live telemetry + current alert level

The endpoint polls the database every 2 seconds and pushes a JSON event
whenever new data arrives.  This is compatible with the browser EventSource API.
"""
import asyncio
import json
import logging
from typing import AsyncGenerator

from fastapi import APIRouter, Depends
from fastapi.responses import StreamingResponse
from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession

from ..database import get_db, get_session_factory
from ..models import Telemetry, Alert, Device, DeviceThreshold
from ..services import get_or_create_thresholds

logger = logging.getLogger(__name__)

router = APIRouter()

_POLL_INTERVAL = 2  # seconds


def _compute_alert_level(latest: Telemetry, thresholds: DeviceThreshold) -> str:
    """Derive the current alert level from a telemetry row + threshold config."""
    # Critical checks
    if (
        (latest.temperature is not None and latest.temperature > thresholds.temp_critical)
        or (latest.mq3   > thresholds.mq3_critical)
        or (latest.mq135 > thresholds.mq135_critical)
        or (latest.mq4   > thresholds.mq4_critical)
        or (latest.humidity is not None and latest.humidity > thresholds.humidity_critical)
    ):
        return "critical"
    # Warning checks
    if (
        (latest.temperature is not None and latest.temperature > thresholds.temp_warning)
        or (latest.mq3   > thresholds.mq3_warning)
        or (latest.mq135 > thresholds.mq135_warning)
        or (latest.mq4   > thresholds.mq4_warning)
        or (latest.humidity is not None and latest.humidity > thresholds.humidity_warning)
    ):
        return "warning"
    return "normal"


async def _sse_generator(device_id: str) -> AsyncGenerator[str, None]:
    """Async generator that polls DB and yields SSE-formatted events."""
    last_ts = None

    while True:
        try:
            event_payload: str | None = None

            # ── Open the session, do all DB work, close it, THEN sleep ──
            factory = get_session_factory()
            async with factory() as session:
                # Latest telemetry
                result = await session.execute(
                    select(Telemetry)
                    .where(Telemetry.device_id == device_id)
                    .order_by(Telemetry.timestamp.desc())
                    .limit(1)
                )
                latest = result.scalars().first()

                if latest is None or latest.timestamp == last_ts:
                    # Nothing new — release session and sleep
                    pass
                else:
                    last_ts = latest.timestamp

                    # Device online status — authoritative value set by MQTT status topic
                    device_result = await session.execute(
                        select(Device).where(Device.device_id == device_id)
                    )
                    device = device_result.scalars().first()
                    is_online = device.is_online if device else False

                    # Active alert count
                    active_count_result = await session.execute(
                        select(Alert).where(
                            Alert.device_id   == device_id,
                            Alert.resolved_at.is_(None),
                        )
                    )
                    active_alerts = active_count_result.scalars().all()
                    active_count  = len(active_alerts)

                    # Thresholds for level computation
                    thresholds = await get_or_create_thresholds(device_id, session)
                    alert_level = _compute_alert_level(latest, thresholds)

                    event_payload = json.dumps({
                        "device_id":   device_id,
                        "is_online":   is_online,
                        "alert_level": alert_level,
                        "active_count": active_count,
                        "telemetry": {
                            "timestamp":       latest.timestamp.isoformat(),
                            "temperature":     latest.temperature,
                            "humidity":        latest.humidity,
                            "mq3":             latest.mq3,
                            "mq135":           latest.mq135,
                            "mq4":             latest.mq4,
                            "battery_percent": latest.battery_percent,
                            "latitude":        latest.latitude,
                            "longitude":       latest.longitude,
                        },
                        "thresholds": {
                            "temp_warning":      thresholds.temp_warning,
                            "temp_critical":     thresholds.temp_critical,
                            "humidity_warning":  thresholds.humidity_warning,
                            "humidity_critical": thresholds.humidity_critical,
                            "mq3_warning":       thresholds.mq3_warning,
                            "mq3_critical":      thresholds.mq3_critical,
                            "mq135_warning":     thresholds.mq135_warning,
                            "mq135_critical":    thresholds.mq135_critical,
                            "mq4_warning":       thresholds.mq4_warning,
                            "mq4_critical":      thresholds.mq4_critical,
                        },
                    })
            # Session is now closed — safe to yield and sleep without holding a connection
            if event_payload:
                yield f"data: {event_payload}\n\n"
            await asyncio.sleep(_POLL_INTERVAL)

        except asyncio.CancelledError:
            break
        except Exception as exc:  # noqa: BLE001
            logger.error("SSE generator error for %s: %s", device_id, exc)
            await asyncio.sleep(_POLL_INTERVAL)


@router.get("/devices/{device_id}/stream")
async def device_stream(device_id: str):
    """
    SSE endpoint: streams live telemetry + alert level for a device.
    Connect with: EventSource('/devices/agrihub-001/stream')
    """
    return StreamingResponse(
        _sse_generator(device_id),
        media_type="text/event-stream",
        headers={
            "Cache-Control": "no-cache",
            "X-Accel-Buffering": "no",
        },
    )
