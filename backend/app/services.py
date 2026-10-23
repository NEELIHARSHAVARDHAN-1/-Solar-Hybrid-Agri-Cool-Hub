"""
Core business logic: telemetry ingestion + two-tier alert evaluation.

Alert tiers (spec-compliant):
  CRITICAL — any of:  mq3 > critical_mq3  |  mq135 > critical_mq135  |  temp > critical_temp
  WARNING  — any of:  mq3 > warning_mq3   |  mq135 > warning_mq135   |  temp > warning_temp
  NORMAL   — all values below WARNING thresholds (no alert created)

Thresholds are loaded from the `device_thresholds` table. If no row exists for a
device, the default ThresholdSet values are used and a row is created automatically.
"""
import logging
from datetime import datetime, timezone
from typing import Optional

from sqlalchemy import select, func
from sqlalchemy.ext.asyncio import AsyncSession
from sqlalchemy.dialects.postgresql import insert as pg_insert

from .database import get_engine
from .models import Device, Telemetry, Alert, DeviceThreshold
from .schemas import TelemetryPayload, ThresholdSet

logger = logging.getLogger(__name__)

# ---- Alert type constants ---------------------------------------------------
ALERT_TEMP_HIGH     = "TEMP_HIGH"
ALERT_HUMIDITY_HIGH = "HUMIDITY_HIGH"
ALERT_MQ3_HIGH      = "MQ3_HIGH"
ALERT_MQ135_HIGH    = "MQ135_HIGH"
ALERT_MQ4_HIGH      = "MQ4_HIGH"

LEVEL_CRITICAL = "critical"
LEVEL_WARNING  = "warning"

# Human-readable metric labels
_METRIC_LABELS = {
    ALERT_TEMP_HIGH:     "Temperature (°C)",
    ALERT_HUMIDITY_HIGH: "Humidity (%)",
    ALERT_MQ3_HIGH:      "MQ3 Gas Sensor (Ethanol/Alcohol)",
    ALERT_MQ135_HIGH:    "MQ135 Air Quality (CO₂/NH₃)",
    ALERT_MQ4_HIGH:      "MQ4 Gas Sensor (Methane/LPG)",
}


def _is_postgres() -> bool:
    return get_engine().dialect.name == "postgresql"


async def get_or_create_thresholds(device_id: str, session: AsyncSession) -> DeviceThreshold:
    """Load threshold row for a device, creating defaults if absent."""
    result = await session.execute(
        select(DeviceThreshold).where(DeviceThreshold.device_id == device_id)
    )
    row = result.scalars().first()
    if row is not None:
        return row

    defaults = ThresholdSet()
    row = DeviceThreshold(
        device_id        = device_id,
        temp_warning     = defaults.temp_warning,
        temp_critical    = defaults.temp_critical,
        humidity_warning = defaults.humidity_warning,
        humidity_critical= defaults.humidity_critical,
        mq3_warning      = defaults.mq3_warning,
        mq3_critical     = defaults.mq3_critical,
        mq135_warning    = defaults.mq135_warning,
        mq135_critical   = defaults.mq135_critical,
        mq4_warning      = defaults.mq4_warning,
        mq4_critical     = defaults.mq4_critical,
    )
    session.add(row)
    await session.flush()
    return row


async def ingest_telemetry(payload: TelemetryPayload, session: AsyncSession) -> None:
    """
    Upsert the device record, insert telemetry (ignoring duplicates),
    and evaluate two-tier alert conditions.
    """
    now = datetime.now(timezone.utc)

    # --- Upsert device -------------------------------------------------------
    if _is_postgres():
        stmt = pg_insert(Device).values(
            device_id=payload.device_id,
            last_seen=now,
            is_online=True,
        ).on_conflict_do_update(
            index_elements=["device_id"],
            set_={"last_seen": now, "is_online": True},
        )
        await session.execute(stmt)
    else:
        device = (await session.execute(
            select(Device).where(Device.device_id == payload.device_id)
        )).scalars().first()
        if device is None:
            session.add(Device(device_id=payload.device_id, last_seen=now, is_online=True))
        else:
            device.last_seen = now
            device.is_online = True
        await session.flush()

    # --- Insert telemetry (ignore duplicate device_id+timestamp) -------------
    battery = payload.battery if payload.battery is not None else payload.battery_percent

    if _is_postgres():
        stmt = pg_insert(Telemetry).values(
            device_id       = payload.device_id,
            timestamp       = payload.timestamp,
            temperature     = payload.temperature,
            humidity        = payload.humidity,
            mq3             = payload.mq3,
            mq135           = payload.mq135,
            mq4             = payload.mq4,
            battery_percent = battery,
            latitude        = payload.latitude,
            longitude       = payload.longitude,
        ).on_conflict_do_nothing(constraint="uq_telemetry_device_ts")
        result = await session.execute(stmt)
        inserted = result.rowcount > 0
    else:
        existing = (await session.execute(
            select(Telemetry).where(
                Telemetry.device_id == payload.device_id,
                Telemetry.timestamp == payload.timestamp,
            )
        )).scalars().first()
        if existing is not None:
            logger.debug("Duplicate telemetry ignored: %s @ %s", payload.device_id, payload.timestamp)
            return
        session.add(Telemetry(
            device_id       = payload.device_id,
            timestamp       = payload.timestamp,
            temperature     = payload.temperature,
            humidity        = payload.humidity,
            mq3             = payload.mq3,
            mq135           = payload.mq135,
            mq4             = payload.mq4,
            battery_percent = battery,
            latitude        = payload.latitude,
            longitude       = payload.longitude,
        ))
        await session.flush()
        inserted = True

    if not inserted:
        logger.debug("Duplicate telemetry ignored: %s @ %s", payload.device_id, payload.timestamp)
        return

    # --- Evaluate alerts -----------------------------------------------------
    await evaluate_alerts(payload, session)


async def evaluate_alerts(payload: TelemetryPayload, session: AsyncSession) -> None:
    """
    Two-tier evaluation per spec:
      • CRITICAL: mq3 > critical | mq135 > critical | temp > critical
      • WARNING:  mq3 > warning  | mq135 > warning  | temp > warning
      • Also monitors: humidity and mq4 at both tiers.

    For each breached (alert_type, alert_level) pair, creates one Alert row
    only if no unresolved alert of the same type already exists for the device.
    When a value drops below both thresholds, any open alert for that type is
    auto-resolved.
    """
    thresholds = await get_or_create_thresholds(payload.device_id, session)

    # Each entry: (alert_type, value, warning_threshold, critical_threshold)
    checks = []
    if payload.temperature is not None:
        checks.append((ALERT_TEMP_HIGH, payload.temperature,
                        thresholds.temp_warning, thresholds.temp_critical))
    if payload.humidity is not None:
        checks.append((ALERT_HUMIDITY_HIGH, payload.humidity,
                        thresholds.humidity_warning, thresholds.humidity_critical))
    checks.append((ALERT_MQ3_HIGH,   payload.mq3,   thresholds.mq3_warning,   thresholds.mq3_critical))
    checks.append((ALERT_MQ135_HIGH, payload.mq135, thresholds.mq135_warning, thresholds.mq135_critical))
    checks.append((ALERT_MQ4_HIGH,   payload.mq4,   thresholds.mq4_warning,   thresholds.mq4_critical))

    for alert_type, value, warn_thresh, crit_thresh in checks:
        # Determine tier
        if value > crit_thresh:
            level     = LEVEL_CRITICAL
            threshold = crit_thresh
        elif value > warn_thresh:
            level     = LEVEL_WARNING
            threshold = warn_thresh
        else:
            # Value is normal — auto-resolve any open alert of this type
            await _auto_resolve(payload.device_id, alert_type, session)
            continue

        # Fetch any open alert for this type (regardless of level)
        existing_result = await session.execute(
            select(Alert).where(
                Alert.device_id  == payload.device_id,
                Alert.alert_type == alert_type,
                Alert.resolved_at.is_(None),
            ).limit(1)
        )
        existing = existing_result.scalars().first()

        if existing is not None:
            # If severity escalated, update level and threshold; otherwise leave it
            if existing.alert_level != level:
                existing.alert_level = level
                existing.threshold   = threshold
                existing.value       = value
                logger.info(
                    "Alert escalated: %s → %s for %s (value=%.2f)",
                    existing.alert_level, level, payload.device_id, value,
                )
        else:
            alert = Alert(
                device_id   = payload.device_id,
                timestamp   = payload.timestamp,
                alert_level = level,
                alert_type  = alert_type,
                metric      = _METRIC_LABELS.get(alert_type, alert_type),
                value       = value,
                threshold   = threshold,
            )
            session.add(alert)
            logger.info(
                "Alert created: [%s] %s for %s (value=%.2f threshold=%.2f)",
                level.upper(), alert_type, payload.device_id, value, threshold,
            )


async def _auto_resolve(device_id: str, alert_type: str, session: AsyncSession) -> None:
    """Resolve any open alert of the given type when value returns to normal."""
    result = await session.execute(
        select(Alert).where(
            Alert.device_id  == device_id,
            Alert.alert_type == alert_type,
            Alert.resolved_at.is_(None),
        ).limit(1)
    )
    alert = result.scalars().first()
    if alert is not None:
        alert.resolved_at = datetime.now(timezone.utc)
        logger.info("Alert auto-resolved: %s for %s", alert_type, device_id)


async def update_device_status(device_id: str, is_online: bool, session: AsyncSession) -> None:
    """
    Upsert the device record with the online/offline status received from
    the MQTT status topic (agricool/v1/devices/<id>/status).
    Sets last_seen to now when the device comes online.
    """
    now = datetime.now(timezone.utc)

    if _is_postgres():
        values: dict = {"device_id": device_id, "is_online": is_online}
        if is_online:
            values["last_seen"] = now
        set_: dict = {"is_online": is_online}
        if is_online:
            set_["last_seen"] = now
        stmt = pg_insert(Device).values(**values).on_conflict_do_update(
            index_elements=["device_id"],
            set_=set_,
        )
        await session.execute(stmt)
    else:
        device = (await session.execute(
            select(Device).where(Device.device_id == device_id)
        )).scalars().first()
        if device is None:
            session.add(Device(
                device_id=device_id,
                is_online=is_online,
                last_seen=now if is_online else None,
            ))
        else:
            device.is_online = is_online
            if is_online:
                device.last_seen = now
        await session.flush()

    logger.info("Device %s status → %s", device_id, "ONLINE" if is_online else "OFFLINE")
