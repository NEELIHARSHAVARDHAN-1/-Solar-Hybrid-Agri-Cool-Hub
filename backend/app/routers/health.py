from fastapi import APIRouter
from ..mqtt_client import mqtt_connected
from ..schemas import HealthOut

router = APIRouter()


@router.get("/health", response_model=HealthOut)
async def health_check():
    return HealthOut(
        status="ok",
        mqtt_connected=mqtt_connected,
    )
