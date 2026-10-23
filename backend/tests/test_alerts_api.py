"""
Focused tests for the alerts API endpoints.

Covers:
  - GET /alerts (all-devices) with resolved filter and pagination
  - GET /devices/{id}/alerts with alert_level filter
  - PATCH /alerts/{id}/resolve
  - alert_level and metric fields present in response
  - Existing alert records preserved (no accidental truncation)
"""
import pytest
from datetime import datetime, timezone
from httpx import AsyncClient
from sqlalchemy.ext.asyncio import AsyncSession

from app.models import Alert, Device
from app.schemas import AlertOut


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

async def _create_device(session: AsyncSession, device_id: str) -> None:
    session.add(Device(
        device_id=device_id,
        is_online=True,
        last_seen=datetime.now(timezone.utc),
    ))
    await session.flush()


async def _create_alert(
    session: AsyncSession,
    device_id: str,
    alert_type: str = "TEMP_HIGH",
    alert_level: str = "warning",
    value: float = 30.0,
    threshold: float = 25.0,
    resolved: bool = False,
) -> Alert:
    alert = Alert(
        device_id=device_id,
        timestamp=datetime.now(timezone.utc),
        alert_level=alert_level,
        alert_type=alert_type,
        metric=f"Test metric for {alert_type}",
        value=value,
        threshold=threshold,
        resolved_at=datetime.now(timezone.utc) if resolved else None,
    )
    session.add(alert)
    await session.flush()
    return alert


# ---------------------------------------------------------------------------
# GET /alerts (all-devices)
# ---------------------------------------------------------------------------

@pytest.mark.asyncio
async def test_get_all_alerts_empty(client: AsyncClient):
    r = await client.get("/alerts")
    assert r.status_code == 200
    data = r.json()
    assert data["total"] == 0
    assert data["items"] == []


@pytest.mark.asyncio
async def test_get_all_alerts_returns_alert_level_and_metric(
    client: AsyncClient, db_session: AsyncSession
):
    """alert_level and metric fields must be present in every AlertOut response."""
    await _create_device(db_session, "dev-001")
    await _create_alert(db_session, "dev-001", alert_level="critical", alert_type="MQ3_HIGH")
    await db_session.commit()

    r = await client.get("/alerts")
    assert r.status_code == 200
    item = r.json()["items"][0]
    assert item["alert_level"] == "critical"
    assert item["alert_type"] == "MQ3_HIGH"
    assert "metric" in item
    assert item["metric"] != ""


@pytest.mark.asyncio
async def test_get_all_alerts_resolved_false_filter(
    client: AsyncClient, db_session: AsyncSession
):
    await _create_device(db_session, "dev-002")
    await _create_alert(db_session, "dev-002", alert_type="TEMP_HIGH", resolved=False)
    await _create_alert(db_session, "dev-002", alert_type="MQ135_HIGH", resolved=True)
    await db_session.commit()

    r = await client.get("/alerts?resolved=false")
    assert r.status_code == 200
    data = r.json()
    assert data["total"] == 1
    assert data["items"][0]["alert_type"] == "TEMP_HIGH"
    assert data["items"][0]["resolved_at"] is None


@pytest.mark.asyncio
async def test_get_all_alerts_resolved_true_filter(
    client: AsyncClient, db_session: AsyncSession
):
    await _create_device(db_session, "dev-003")
    await _create_alert(db_session, "dev-003", alert_type="HUMIDITY_HIGH", resolved=True)
    await _create_alert(db_session, "dev-003", alert_type="MQ4_HIGH",     resolved=False)
    await db_session.commit()

    r = await client.get("/alerts?resolved=true")
    assert r.status_code == 200
    data = r.json()
    assert data["total"] == 1
    assert data["items"][0]["resolved_at"] is not None


@pytest.mark.asyncio
async def test_get_all_alerts_pagination(
    client: AsyncClient, db_session: AsyncSession
):
    await _create_device(db_session, "dev-004")
    for _ in range(5):
        await _create_alert(db_session, "dev-004")
    await db_session.commit()

    r = await client.get("/alerts?limit=2&offset=0")
    assert r.status_code == 200
    data = r.json()
    assert data["total"] == 5
    assert len(data["items"]) == 2

    r2 = await client.get("/alerts?limit=2&offset=4")
    assert r2.status_code == 200
    assert len(r2.json()["items"]) == 1


# ---------------------------------------------------------------------------
# GET /devices/{id}/alerts with alert_level filter
# ---------------------------------------------------------------------------

@pytest.mark.asyncio
async def test_device_alerts_alert_level_filter(
    client: AsyncClient, db_session: AsyncSession
):
    await _create_device(db_session, "dev-005")
    await _create_alert(db_session, "dev-005", alert_level="warning",  alert_type="TEMP_HIGH")
    await _create_alert(db_session, "dev-005", alert_level="critical", alert_type="MQ3_HIGH")
    await db_session.commit()

    r = await client.get("/devices/dev-005/alerts?alert_level=critical")
    assert r.status_code == 200
    data = r.json()
    assert data["total"] == 1
    assert data["items"][0]["alert_level"] == "critical"


# ---------------------------------------------------------------------------
# PATCH /alerts/{id}/resolve
# ---------------------------------------------------------------------------

@pytest.mark.asyncio
async def test_resolve_alert(client: AsyncClient, db_session: AsyncSession):
    await _create_device(db_session, "dev-006")
    alert = await _create_alert(db_session, "dev-006", resolved=False)
    await db_session.commit()

    r = await client.patch(f"/alerts/{alert.id}/resolve")
    assert r.status_code == 200
    assert r.json()["resolved_at"] is not None


@pytest.mark.asyncio
async def test_resolve_already_resolved_returns_400(
    client: AsyncClient, db_session: AsyncSession
):
    await _create_device(db_session, "dev-007")
    alert = await _create_alert(db_session, "dev-007", resolved=True)
    await db_session.commit()

    r = await client.patch(f"/alerts/{alert.id}/resolve")
    assert r.status_code == 400


@pytest.mark.asyncio
async def test_existing_alerts_not_deleted_after_fixture(
    client: AsyncClient, db_session: AsyncSession
):
    """Regression: ensure alerts created before test run remain intact."""
    await _create_device(db_session, "dev-008")
    await _create_alert(db_session, "dev-008", alert_level="warning")
    await _create_alert(db_session, "dev-008", alert_level="critical")
    await db_session.commit()

    r = await client.get("/alerts")
    assert r.status_code == 200
    assert r.json()["total"] >= 2
