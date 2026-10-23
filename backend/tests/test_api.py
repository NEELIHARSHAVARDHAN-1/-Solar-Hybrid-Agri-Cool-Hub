import pytest
from httpx import AsyncClient


@pytest.mark.asyncio
async def test_health(client: AsyncClient):
    r = await client.get("/health")
    assert r.status_code == 200
    assert r.json()["status"] == "ok"


@pytest.mark.asyncio
async def test_list_devices_empty(client: AsyncClient):
    r = await client.get("/devices")
    assert r.status_code == 200
    assert r.json() == []


@pytest.mark.asyncio
async def test_get_device_not_found(client: AsyncClient):
    r = await client.get("/devices/nonexistent")
    assert r.status_code == 404


@pytest.mark.asyncio
async def test_device_status_not_found(client: AsyncClient):
    r = await client.get("/devices/nonexistent/status")
    assert r.status_code == 404


@pytest.mark.asyncio
async def test_telemetry_not_found(client: AsyncClient):
    r = await client.get("/devices/nonexistent/telemetry")
    assert r.status_code == 200
    data = r.json()
    assert data["total"] == 0
    assert data["items"] == []


@pytest.mark.asyncio
async def test_alerts_empty(client: AsyncClient):
    r = await client.get("/devices/nonexistent/alerts")
    assert r.status_code == 200
    data = r.json()
    assert data["total"] == 0


@pytest.mark.asyncio
async def test_active_alerts_empty(client: AsyncClient):
    r = await client.get("/devices/nonexistent/alerts/active")
    assert r.status_code == 200
    assert r.json()["total"] == 0


@pytest.mark.asyncio
async def test_resolve_alert_not_found(client: AsyncClient):
    r = await client.patch("/alerts/9999/resolve")
    assert r.status_code == 404
