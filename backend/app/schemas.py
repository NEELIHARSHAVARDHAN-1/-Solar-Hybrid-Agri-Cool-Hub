from datetime import datetime
from typing import Optional, List
from pydantic import BaseModel, Field, model_validator


# =============================================================================
# Inbound — telemetry payload from ESP32 via MQTT
# Payload format (current firmware):
# {
#   "device_id": "agrihub-001",
#   "timestamp": "2026-10-23T01:32:59Z",
#   "temperature": 30.69,        # float °C
#   "humidity":    69.57,        # float %
#   "mq3":         23.84,        # float 0–100 custom scale (firmware % of sensor range)
#   "mq135":       16.08,        # float 0–100 custom scale
#   "mq4":         14.53,        # float 0–100 custom scale
#   "latitude":    17.3843,      # optional
#   "longitude":   78.4583,      # optional
#   "battery":     99            # int %
# }
# MQ scale: firmware maps raw ADC → 0–100 percentage of configured sensor range.
# Values above 100 are rejected; negative values are rejected.
# =============================================================================

class TelemetryPayload(BaseModel):
    device_id:  str
    timestamp:  datetime
    temperature: Optional[float] = None
    humidity:    Optional[float] = None
    mq3:         float
    mq135:       float
    mq4:         float
    # Accept both "battery" (new firmware) and "battery_percent" (legacy)
    battery:         Optional[int] = None
    battery_percent: Optional[int] = None
    latitude:        Optional[float] = None
    longitude:       Optional[float] = None
    # Legacy / ignored fields
    humidifier_enabled: Optional[bool] = None

    @model_validator(mode="after")
    def validate_and_normalise(self):
        if self.temperature is not None and not (-40 <= self.temperature <= 85):
            raise ValueError(f"temperature out of range: {self.temperature}")
        if self.humidity is not None and not (0 <= self.humidity <= 100):
            raise ValueError(f"humidity out of range: {self.humidity}")

        # MQ values are on the firmware's 0–100 custom scale.
        for name, val in (("mq3", self.mq3), ("mq135", self.mq135), ("mq4", self.mq4)):
            if not (0.0 <= val <= 100.0):
                raise ValueError(f"{name} out of range (expected 0–100): {val}")

        # Normalise battery: prefer "battery" field, fall back to "battery_percent"
        if self.battery is None and self.battery_percent is not None:
            self.battery = self.battery_percent
        if self.battery is not None and not (0 <= self.battery <= 100):
            raise ValueError(f"battery out of range: {self.battery}")

        return self

    @property
    def battery_value(self) -> Optional[int]:
        return self.battery


# =============================================================================
# Outbound — API response schemas
# =============================================================================

class DeviceOut(BaseModel):
    device_id:  str
    last_seen:  Optional[datetime]
    is_online:  bool
    created_at: datetime

    model_config = {"from_attributes": True}


class TelemetryOut(BaseModel):
    id:              int
    device_id:       str
    timestamp:       datetime
    temperature:     Optional[float]
    humidity:        Optional[float]
    mq3:             float
    mq135:           float
    mq4:             float
    battery_percent: Optional[int]
    latitude:        Optional[float]
    longitude:       Optional[float]
    received_at:     datetime

    model_config = {"from_attributes": True}


class DeviceStatusOut(BaseModel):
    device_id: str
    is_online:  bool
    last_seen:  Optional[datetime]
    latest:     Optional[TelemetryOut]

    model_config = {"from_attributes": True}


class AlertOut(BaseModel):
    id:          int
    device_id:   str
    timestamp:   datetime
    alert_level: str   # "critical" | "warning"
    alert_type:  str   # "MQ3_HIGH" | "MQ135_HIGH" | "TEMP_HIGH" | "HUMIDITY_HIGH" | "MQ4_HIGH"
    metric:      str   # human label
    value:       float
    threshold:   float
    resolved_at: Optional[datetime]
    created_at:  datetime

    model_config = {"from_attributes": True}


class PaginatedTelemetry(BaseModel):
    total:  int
    items:  List[TelemetryOut]


class PaginatedAlerts(BaseModel):
    total:  int
    items:  List[AlertOut]


class HealthOut(BaseModel):
    status: str = "ok"
    mqtt_connected: bool = False


# =============================================================================
# Thresholds — per-device configurable alert levels
# =============================================================================

class ThresholdSet(BaseModel):
    """Warning and Critical thresholds for each parameter."""
    # Temperature °C
    temp_warning:  float = Field(default=25.0, description="Warning threshold °C")
    temp_critical: float = Field(default=30.0, description="Critical threshold °C")
    # Humidity %
    humidity_warning:  float = Field(default=70.0, description="Warning threshold %")
    humidity_critical: float = Field(default=85.0, description="Critical threshold %")
    # MQ3 (0–100 firmware scale)
    mq3_warning:  float = Field(default=20.0, description="Warning threshold (0–100 scale)")
    mq3_critical: float = Field(default=30.0, description="Critical threshold (0–100 scale)")
    # MQ135 (0–100 firmware scale)
    mq135_warning:  float = Field(default=25.0, description="Warning threshold (0–100 scale)")
    mq135_critical: float = Field(default=40.0, description="Critical threshold (0–100 scale)")
    # MQ4 (0–100 firmware scale)
    mq4_warning:  float = Field(default=20.0, description="Warning threshold (0–100 scale)")
    mq4_critical: float = Field(default=35.0, description="Critical threshold (0–100 scale)")


class ThresholdOut(BaseModel):
    device_id:         str
    temp_warning:      float
    temp_critical:     float
    humidity_warning:  float
    humidity_critical: float
    mq3_warning:       float
    mq3_critical:      float
    mq135_warning:     float
    mq135_critical:    float
    mq4_warning:       float
    mq4_critical:      float
    updated_at:        datetime

    model_config = {"from_attributes": True}


class DeviceAlertStatus(BaseModel):
    """Current overall alert level for a device based on latest telemetry."""
    device_id:    str
    alert_level:  str   # "normal" | "warning" | "critical"
    is_online:    bool
    last_seen:    Optional[datetime]
    latest:       Optional[TelemetryOut]
    active_count: int   # number of unresolved alerts
