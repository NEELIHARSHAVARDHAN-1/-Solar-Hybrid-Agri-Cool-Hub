from datetime import datetime, timezone
from typing import Optional

from fastapi import APIRouter, Depends, HTTPException, Query
from sqlalchemy import select, func
from sqlalchemy.ext.asyncio import AsyncSession

from ..database import get_db
from ..models import Telemetry
from ..schemas import TelemetryOut, PaginatedTelemetry

router = APIRouter()


def _ensure_utc(dt: datetime | None) -> datetime | None:
    """Attach UTC timezone to a naive datetime so DB comparisons work correctly."""
    if dt is None:
        return None
    if dt.tzinfo is None:
        return dt.replace(tzinfo=timezone.utc)
    return dt


@router.get("/{device_id}/telemetry", response_model=PaginatedTelemetry)
async def list_telemetry(
    device_id: str,
    limit:  int = Query(default=50,  ge=1, le=500),
    offset: int = Query(default=0,   ge=0),
    from_ts: Optional[datetime] = Query(default=None, alias="from"),
    to_ts:   Optional[datetime] = Query(default=None, alias="to"),
    db: AsyncSession = Depends(get_db),
):
    from_ts = _ensure_utc(from_ts)
    to_ts   = _ensure_utc(to_ts)

    q = select(Telemetry).where(Telemetry.device_id == device_id)
    if from_ts:
        q = q.where(Telemetry.timestamp >= from_ts)
    if to_ts:
        q = q.where(Telemetry.timestamp <= to_ts)

    total_result = await db.execute(select(func.count()).select_from(q.subquery()))
    total = total_result.scalar_one()

    q = q.order_by(Telemetry.timestamp.desc()).offset(offset).limit(limit)
    result = await db.execute(q)
    items = result.scalars().all()

    return PaginatedTelemetry(total=total, items=items)


@router.get("/{device_id}/telemetry/latest", response_model=TelemetryOut)
async def latest_telemetry(device_id: str, db: AsyncSession = Depends(get_db)):
    result = await db.execute(
        select(Telemetry)
        .where(Telemetry.device_id == device_id)
        .order_by(Telemetry.timestamp.desc())
        .limit(1)
    )
    row = result.scalars().first()
    if row is None:
        raise HTTPException(status_code=404, detail="No telemetry found for device")
    return row
