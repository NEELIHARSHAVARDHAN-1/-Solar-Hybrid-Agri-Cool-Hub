"""
Thresholds router — GET and PUT per-device warning/critical thresholds.
Changes take effect on the next ingested telemetry message.
"""
from datetime import datetime, timezone

from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession

from ..database import get_db
from ..models import Device, DeviceThreshold
from ..schemas import ThresholdSet, ThresholdOut
from ..services import get_or_create_thresholds

router = APIRouter()


@router.get("/devices/{device_id}/thresholds", response_model=ThresholdOut)
async def get_thresholds(device_id: str, db: AsyncSession = Depends(get_db)):
    """Return current thresholds for a device. Creates defaults if none exist."""
    device_result = await db.execute(select(Device).where(Device.device_id == device_id))
    if device_result.scalars().first() is None:
        raise HTTPException(status_code=404, detail="Device not found")

    row = await get_or_create_thresholds(device_id, db)
    return row


@router.put("/devices/{device_id}/thresholds", response_model=ThresholdOut)
async def update_thresholds(
    device_id: str,
    body: ThresholdSet,
    db: AsyncSession = Depends(get_db),
):
    """
    Update warning + critical thresholds for a device.
    Alert evaluation on the NEXT incoming payload will use the new values.
    """
    device_result = await db.execute(select(Device).where(Device.device_id == device_id))
    if device_result.scalars().first() is None:
        raise HTTPException(status_code=404, detail="Device not found")

    row = await get_or_create_thresholds(device_id, db)

    row.temp_warning      = body.temp_warning
    row.temp_critical     = body.temp_critical
    row.humidity_warning  = body.humidity_warning
    row.humidity_critical = body.humidity_critical
    row.mq3_warning       = body.mq3_warning
    row.mq3_critical      = body.mq3_critical
    row.mq135_warning     = body.mq135_warning
    row.mq135_critical    = body.mq135_critical
    row.mq4_warning       = body.mq4_warning
    row.mq4_critical      = body.mq4_critical
    row.updated_at        = datetime.now(timezone.utc)

    await db.flush()
    return row
