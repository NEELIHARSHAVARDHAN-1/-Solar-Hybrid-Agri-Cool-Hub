import asyncio
import logging
from contextlib import asynccontextmanager

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from sqlalchemy import text

from .database import Base, get_engine
from .mqtt_client import mqtt_listener
from .routers import devices, telemetry, alerts, health, thresholds, sse

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s %(levelname)s %(name)s — %(message)s",
)
logger = logging.getLogger(__name__)

# ---------------------------------------------------------------------------
# Idempotent column migrations — run after create_all so that existing
# databases created before these columns were added are brought up to date
# without dropping or truncating any data.
# ---------------------------------------------------------------------------
_COLUMN_MIGRATIONS = [
    # Adds alert_level to alerts table (missing from pre-v2 databases)
    """
    DO $$
    BEGIN
        IF NOT EXISTS (
            SELECT 1 FROM information_schema.columns
            WHERE table_name = 'alerts' AND column_name = 'alert_level'
        ) THEN
            ALTER TABLE alerts ADD COLUMN alert_level VARCHAR(16) NOT NULL DEFAULT 'warning';
            RAISE NOTICE 'Migration applied: alerts.alert_level added';
        END IF;
    END $$;
    """,
    # Adds metric to alerts table (missing from pre-v2 databases)
    """
    DO $$
    BEGIN
        IF NOT EXISTS (
            SELECT 1 FROM information_schema.columns
            WHERE table_name = 'alerts' AND column_name = 'metric'
        ) THEN
            ALTER TABLE alerts ADD COLUMN metric VARCHAR(64) NOT NULL DEFAULT '';
            RAISE NOTICE 'Migration applied: alerts.metric added';
        END IF;
    END $$;
    """,
    # Reset device_thresholds MQ columns that were accidentally set to voltage
    # scale (< 5.0) back to the correct 0–100 firmware scale defaults.
    # This fixes rows written during the brief period when defaults were wrong.
    """
    DO $$
    BEGIN
        IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_name = 'device_thresholds') THEN
            UPDATE device_thresholds
            SET
                mq3_warning   = 20.0,
                mq3_critical  = 30.0,
                mq135_warning = 25.0,
                mq135_critical = 40.0,
                mq4_warning   = 20.0,
                mq4_critical  = 35.0
            WHERE mq3_warning < 5.0 OR mq135_warning < 5.0 OR mq4_warning < 5.0;
        END IF;
    END $$;
    """,
]


async def _run_migrations(engine) -> None:
    """
    Apply idempotent schema migrations for PostgreSQL only.
    SQLite (used in tests) does not support DO $$ blocks; skip silently.
    """
    if engine.dialect.name != "postgresql":
        return
    async with engine.begin() as conn:
        for stmt in _COLUMN_MIGRATIONS:
            await conn.execute(text(stmt))
    logger.info("Schema migration check complete.")


@asynccontextmanager
async def lifespan(app: FastAPI):
    # ---- Startup ----
    logger.info("Creating database tables (if not exist)...")
    engine = get_engine()
    async with engine.begin() as conn:
        await conn.run_sync(Base.metadata.create_all)

    # Apply any missing column migrations (safe for existing data)
    await _run_migrations(engine)

    logger.info("Starting MQTT listener task...")
    mqtt_task = asyncio.create_task(mqtt_listener())

    yield

    # ---- Shutdown ----
    logger.info("Stopping MQTT listener...")
    mqtt_task.cancel()
    try:
        await mqtt_task
    except asyncio.CancelledError:
        pass
    await engine.dispose()
    logger.info("Shutdown complete.")


app = FastAPI(
    title="AgriHub — Cool Storage Monitor Backend",
    version="2.0.0",
    description=(
        "IoT telemetry ingestion, two-tier alert evaluation, and real-time SSE streaming "
        "for agricultural cold-storage monitoring."
    ),
    lifespan=lifespan,
)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],   # restrict in production
    allow_methods=["*"],
    allow_headers=["*"],
)

app.include_router(health.router,      tags=["Health"])
app.include_router(devices.router,     prefix="/devices",  tags=["Devices"])
app.include_router(telemetry.router,   prefix="/devices",  tags=["Telemetry"])
app.include_router(alerts.router,                          tags=["Alerts"])
app.include_router(thresholds.router,                      tags=["Thresholds"])
app.include_router(sse.router,                             tags=["Stream"])
