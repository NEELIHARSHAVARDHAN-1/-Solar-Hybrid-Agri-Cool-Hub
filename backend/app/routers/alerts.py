"""
Updated alerts router with alert_level filter support and all-devices endpoint.
"""
from datetime import datetime, timezone

from fastapi import APIRouter, Depends, HTTPException, Query
from sqlalchemy import select, func
from sqlalchemy.ext.asyncio import AsyncSession
from typing import Optional

from ..database import get_db
from ..models import Alert
from ..schemas import AlertOut, PaginatedAlerts

router = APIRouter()


@router.get("/devices/{device_id}/alerts", response_model=PaginatedAlerts)
async def list_alerts(
    device_id: str,
    alert_level: Optional[str] = Query(default=None, description="Filter: 'warning' or 'critical'"),
    resolved: Optional[bool]   = Query(default=None, description="Filter: true=resolved, false=active, null=all"),
    limit:  int = Query(default=50, ge=1, le=500),
    offset: int = Query(default=0,  ge=0),
    db: AsyncSession = Depends(get_db),
):
    q = select(Alert).where(Alert.device_id == device_id)
    if alert_level:
        q = q.where(Alert.alert_level == alert_level)
    if resolved is True:
        q = q.where(Alert.resolved_at.isnot(None))
    elif resolved is False:
        q = q.where(Alert.resolved_at.is_(None))

    total = (await db.execute(select(func.count()).select_from(q.subquery()))).scalar_one()
    result = await db.execute(q.order_by(Alert.created_at.desc()).offset(offset).limit(limit))
    return PaginatedAlerts(total=total, items=result.scalars().all())


@router.get("/devices/{device_id}/alerts/active", response_model=PaginatedAlerts)
async def active_alerts(
    device_id: str,
    limit:  int = Query(default=50, ge=1, le=500),
    offset: int = Query(default=0,  ge=0),
    db: AsyncSession = Depends(get_db),
):
    q = select(Alert).where(
        Alert.device_id == device_id,
        Alert.resolved_at.is_(None),
    )
    total = (await db.execute(select(func.count()).select_from(q.subquery()))).scalar_one()
    result = await db.execute(q.order_by(Alert.created_at.desc()).offset(offset).limit(limit))
    return PaginatedAlerts(total=total, items=result.scalars().all())


@router.get("/alerts", response_model=PaginatedAlerts)
async def list_all_alerts(
    alert_level: Optional[str] = Query(default=None),
    resolved:    Optional[bool] = Query(default=None),
    limit:  int = Query(default=100, ge=1, le=500),
    offset: int = Query(default=0,   ge=0),
    db: AsyncSession = Depends(get_db),
):
    """All alerts across all devices (for the alerts overview page)."""
    q = select(Alert)
    if alert_level:
        q = q.where(Alert.alert_level == alert_level)
    if resolved is True:
        q = q.where(Alert.resolved_at.isnot(None))
    elif resolved is False:
        q = q.where(Alert.resolved_at.is_(None))

    total = (await db.execute(select(func.count()).select_from(q.subquery()))).scalar_one()
    result = await db.execute(q.order_by(Alert.created_at.desc()).offset(offset).limit(limit))
    return PaginatedAlerts(total=total, items=result.scalars().all())


@router.patch("/alerts/{alert_id}/resolve", response_model=AlertOut)
async def resolve_alert(alert_id: int, db: AsyncSession = Depends(get_db)):
    result = await db.execute(select(Alert).where(Alert.id == alert_id))
    alert = result.scalars().first()
    if alert is None:
        raise HTTPException(status_code=404, detail="Alert not found")
    if alert.resolved_at is not None:
        raise HTTPException(status_code=400, detail="Alert already resolved")
    alert.resolved_at = datetime.now(timezone.utc)
    await db.flush()
    return alert
