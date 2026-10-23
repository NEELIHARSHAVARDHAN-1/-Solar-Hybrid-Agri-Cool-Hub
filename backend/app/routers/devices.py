from typing import List

from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession

from ..database import get_db
from ..models import Device, Telemetry
from ..schemas import DeviceOut, DeviceStatusOut, TelemetryOut

router = APIRouter()


@router.get("", response_model=List[DeviceOut])
async def list_devices(db: AsyncSession = Depends(get_db)):
    result = await db.execute(select(Device).order_by(Device.device_id))
    devices = result.scalars().all()
    return devices


@router.get("/{device_id}", response_model=DeviceOut)
async def get_device(device_id: str, db: AsyncSession = Depends(get_db)):
    result = await db.execute(select(Device).where(Device.device_id == device_id))
    device = result.scalars().first()
    if device is None:
        raise HTTPException(status_code=404, detail="Device not found")
    return device


@router.get("/{device_id}/status", response_model=DeviceStatusOut)
async def get_device_status(device_id: str, db: AsyncSession = Depends(get_db)):
    result = await db.execute(select(Device).where(Device.device_id == device_id))
    device = result.scalars().first()
    if device is None:
        raise HTTPException(status_code=404, detail="Device not found")

    latest_result = await db.execute(
        select(Telemetry)
        .where(Telemetry.device_id == device_id)
        .order_by(Telemetry.timestamp.desc())
        .limit(1)
    )
    latest = latest_result.scalars().first()

    return DeviceStatusOut(
        device_id=device.device_id,
        is_online=device.is_online,
        last_seen=device.last_seen,
        latest=TelemetryOut.model_validate(latest) if latest else None,
    )
