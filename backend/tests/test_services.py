import pytest
import pytest_asyncio
from datetime import datetime, timezone

from app.schemas import TelemetryPayload
from app.services import ingest_telemetry
from app.models import Telemetry, Alert, Device
from sqlalchemy import select


# MQ values are on the firmware's 0–100 custom scale.
# Default warning thresholds: mq3=20.0, mq135=25.0, mq4=20.0
PAYLOAD_DATA = {
    "device_id": "agrihub-test",
    "timestamp": "2026-10-21T10:30:00+05:30",
    "temperature": 25.0,
    "humidity": 60.0,
    "mq3": 12.0,    # below mq3_warning default of 20.0
    "mq135": 10.0,  # below mq135_warning default of 25.0
    "mq4": 8.0,     # below mq4_warning default of 20.0
    "battery_percent": 80,
    "latitude": 15.8281,
    "longitude": 78.0373,
}

HIGH_TEMP_PAYLOAD_DATA = {
    **PAYLOAD_DATA,
    "timestamp": "2026-10-21T11:00:00+05:30",
    "temperature": 40.0,  # above temp_critical default of 30.0 → CRITICAL alert
}


@pytest.mark.asyncio
async def test_ingest_creates_device_and_telemetry(db_session):
    payload = TelemetryPayload(**PAYLOAD_DATA)
    await ingest_telemetry(payload, db_session)
    await db_session.commit()

    device = (await db_session.execute(
        select(Device).where(Device.device_id == "agrihub-test")
    )).scalars().first()
    assert device is not None
    assert device.is_online is True

    row = (await db_session.execute(
        select(Telemetry).where(Telemetry.device_id == "agrihub-test")
    )).scalars().first()
    assert row is not None
    assert row.temperature == 25.0


@pytest.mark.asyncio
async def test_duplicate_insert_is_ignored(db_session):
    payload = TelemetryPayload(**PAYLOAD_DATA)
    await ingest_telemetry(payload, db_session)
    await db_session.commit()
    # Second insert with identical device_id+timestamp should not raise
    await ingest_telemetry(payload, db_session)
    await db_session.commit()

    count = (await db_session.execute(
        select(Telemetry).where(Telemetry.device_id == "agrihub-test")
    )).scalars().all()
    assert len(count) == 1  # only one row despite two inserts


@pytest.mark.asyncio
async def test_alert_created_on_threshold_breach(db_session):
    payload = TelemetryPayload(**HIGH_TEMP_PAYLOAD_DATA)
    await ingest_telemetry(payload, db_session)
    await db_session.commit()

    alerts = (await db_session.execute(
        select(Alert).where(Alert.device_id == "agrihub-test")
    )).scalars().all()
    alert_types = [a.alert_type for a in alerts]
    assert "TEMP_HIGH" in alert_types


@pytest.mark.asyncio
async def test_alert_not_duplicated_on_repeated_breach(db_session):
    p1 = TelemetryPayload(**HIGH_TEMP_PAYLOAD_DATA)
    await ingest_telemetry(p1, db_session)
    await db_session.commit()

    p2 = TelemetryPayload(**{**HIGH_TEMP_PAYLOAD_DATA, "timestamp": "2026-10-21T11:01:00+05:30"})
    await ingest_telemetry(p2, db_session)
    await db_session.commit()

    alerts = (await db_session.execute(
        select(Alert).where(
            Alert.device_id == "agrihub-test",
            Alert.alert_type == "TEMP_HIGH",
        )
    )).scalars().all()
    assert len(alerts) == 1  # only one open alert, not two


@pytest.mark.asyncio
async def test_no_alert_below_threshold(db_session):
    payload = TelemetryPayload(**PAYLOAD_DATA)  # temp=25, well below 35
    await ingest_telemetry(payload, db_session)
    await db_session.commit()

    alerts = (await db_session.execute(
        select(Alert).where(Alert.device_id == "agrihub-test")
    )).scalars().all()
    assert len(alerts) == 0
