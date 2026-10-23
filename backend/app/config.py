import logging
from pathlib import Path
from pydantic_settings import BaseSettings, SettingsConfigDict
from pydantic import Field, field_validator

# Resolve .env relative to this file so the path is correct regardless of
# the working directory uvicorn is launched from.
_ENV_FILE = Path(__file__).resolve().parent.parent / ".env"

logger = logging.getLogger(__name__)

# Placeholder values that indicate the user has not filled in their .env
_PLACEHOLDER_HOSTS = {"YOUR_HOST.hivemq.cloud", "your_host.hivemq.cloud", ""}


class Settings(BaseSettings):
    model_config = SettingsConfigDict(env_file=str(_ENV_FILE), extra="ignore")

    # ---- Database -----------------------------------------------------------
    database_url: str = Field(
        default="postgresql+asyncpg://agrihub:agrihub@localhost:5432/agrihub",
        description="Async SQLAlchemy PostgreSQL URL (use asyncpg driver)",
    )

    # ---- MQTT (HiveMQ Cloud) ------------------------------------------------
    mqtt_host: str     = Field(default="YOUR_HOST.hivemq.cloud")
    mqtt_port: int     = Field(default=8883)
    mqtt_user: str     = Field(default="YOUR_MQTT_USER")
    mqtt_password: str = Field(default="YOUR_MQTT_PASSWORD")
    mqtt_topic_telemetry: str = Field(default="agricool/v1/devices/+/telemetry")
    mqtt_topic_status:    str = Field(default="agricool/v1/devices/+/status")
    mqtt_client_id: str       = Field(default="agrihub-backend")

    # ---- Alert Thresholds (configurable per deployment) --------------------
    # MQ values are scaled sensor output voltage (V), range 0.0–5.0.
    # Defaults below correspond to roughly 700/4095 ADC counts at 5V supply.
    alert_temp_max:     float = Field(default=35.0,  description="°C")
    alert_humidity_max: float = Field(default=90.0,  description="%")
    alert_mq3_max:      float = Field(default=2.82,  description="V (sensor output)")
    alert_mq135_max:    float = Field(default=3.23,  description="V (sensor output)")
    alert_mq4_max:      float = Field(default=2.82,  description="V (sensor output)")

    # ---- Device Staleness ---------------------------------------------------
    device_offline_minutes: int = Field(
        default=5,
        description="Minutes since last_seen after which device is considered offline",
    )

    @field_validator("mqtt_host")
    @classmethod
    def validate_mqtt_host(cls, v: str) -> str:
        """
        Reject placeholder hostnames and hostnames that embed a URL scheme or
        port, which would cause socket.getaddrinfo to fail with NXDOMAIN /
        [Errno 11001].

        Examples of malformed values that are caught:
          - "YOUR_HOST.hivemq.cloud"    ← placeholder (never edited .env)
          - "mqtts://abc.hivemq.cloud"  ← scheme embedded in host
          - "abc.hivemq.cloud:8883"     ← port embedded in host
        """
        stripped = v.strip()
        if stripped.lower() in _PLACEHOLDER_HOSTS:
            logger.error(
                "MQTT_HOST is not configured (value=%r). "
                "Set MQTT_HOST to your HiveMQ Cloud cluster hostname in .env. "
                "MQTT connection will not be attempted.",
                stripped,
            )
            # Return the value unchanged — the MQTT listener will handle the
            # DNS failure gracefully rather than crashing the whole app.
            return stripped

        if "://" in stripped:
            raise ValueError(
                f"MQTT_HOST must not contain a URL scheme (got {stripped!r}). "
                "Use only the bare hostname, e.g. abc123.s1.eu.hivemq.cloud"
            )

        if ":" in stripped:
            raise ValueError(
                f"MQTT_HOST must not contain a port (got {stripped!r}). "
                "Set the port separately via MQTT_PORT."
            )

        return stripped


settings = Settings()
