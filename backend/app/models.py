from datetime import datetime
from typing import Optional

from sqlalchemy import (
    Boolean, DateTime, Float, ForeignKey, Integer,
    String, UniqueConstraint, func,
)
from sqlalchemy.orm import Mapped, mapped_column, relationship

from .database import Base


class Device(Base):
    __tablename__ = "devices"

    id:         Mapped[int]      = mapped_column(Integer, primary_key=True, autoincrement=True)
    device_id:  Mapped[str]      = mapped_column(String(64), unique=True, nullable=False, index=True)
    last_seen:  Mapped[Optional[datetime]] = mapped_column(DateTime(timezone=True), nullable=True)
    is_online:  Mapped[bool]     = mapped_column(Boolean, default=False)
    created_at: Mapped[datetime] = mapped_column(DateTime(timezone=True), server_default=func.now())

    telemetry:  Mapped[list["Telemetry"]]       = relationship(back_populates="device", lazy="noload")
    alerts:     Mapped[list["Alert"]]            = relationship(back_populates="device", lazy="noload")
    thresholds: Mapped[Optional["DeviceThreshold"]] = relationship(back_populates="device", lazy="noload", uselist=False)


class Telemetry(Base):
    __tablename__ = "telemetry"
    __table_args__ = (
        UniqueConstraint("device_id", "timestamp", name="uq_telemetry_device_ts"),
    )

    id:              Mapped[int]   = mapped_column(Integer, primary_key=True, autoincrement=True)
    device_id:       Mapped[str]   = mapped_column(String(64), ForeignKey("devices.device_id"), nullable=False, index=True)
    timestamp:       Mapped[datetime] = mapped_column(DateTime(timezone=True), nullable=False, index=True)
    temperature:     Mapped[Optional[float]] = mapped_column(Float, nullable=True)
    humidity:        Mapped[Optional[float]] = mapped_column(Float, nullable=True)
    mq3:             Mapped[float] = mapped_column(Float, nullable=False)
    mq135:           Mapped[float] = mapped_column(Float, nullable=False)
    mq4:             Mapped[float] = mapped_column(Float, nullable=False)
    battery_percent: Mapped[Optional[int]]   = mapped_column(Integer, nullable=True)
    latitude:        Mapped[Optional[float]] = mapped_column(Float, nullable=True)
    longitude:       Mapped[Optional[float]] = mapped_column(Float, nullable=True)
    received_at:     Mapped[datetime] = mapped_column(DateTime(timezone=True), server_default=func.now())

    device: Mapped["Device"] = relationship(back_populates="telemetry", lazy="noload")


class Alert(Base):
    __tablename__ = "alerts"

    id:          Mapped[int]   = mapped_column(Integer, primary_key=True, autoincrement=True)
    device_id:   Mapped[str]   = mapped_column(String(64), ForeignKey("devices.device_id"), nullable=False, index=True)
    timestamp:   Mapped[datetime] = mapped_column(DateTime(timezone=True), nullable=False)
    alert_level: Mapped[str]  = mapped_column(String(16), nullable=False, default="warning")   # "warning" | "critical"
    alert_type:  Mapped[str]  = mapped_column(String(32), nullable=False)
    metric:      Mapped[str]  = mapped_column(String(64), nullable=False, default="")          # human-readable label
    value:       Mapped[float] = mapped_column(Float, nullable=False)
    threshold:   Mapped[float] = mapped_column(Float, nullable=False)
    resolved_at: Mapped[Optional[datetime]] = mapped_column(DateTime(timezone=True), nullable=True)
    created_at:  Mapped[datetime] = mapped_column(DateTime(timezone=True), server_default=func.now())

    device: Mapped["Device"] = relationship(back_populates="alerts", lazy="noload")


class DeviceThreshold(Base):
    """Per-device configurable warning + critical thresholds."""
    __tablename__ = "device_thresholds"

    id:        Mapped[int] = mapped_column(Integer, primary_key=True, autoincrement=True)
    device_id: Mapped[str] = mapped_column(String(64), ForeignKey("devices.device_id"), unique=True, nullable=False, index=True)

    # Temperature °C
    temp_warning:  Mapped[float] = mapped_column(Float, nullable=False, default=25.0)
    temp_critical: Mapped[float] = mapped_column(Float, nullable=False, default=30.0)

    # Humidity %
    humidity_warning:  Mapped[float] = mapped_column(Float, nullable=False, default=70.0)
    humidity_critical: Mapped[float] = mapped_column(Float, nullable=False, default=85.0)

    # MQ3 (0–100 firmware scale)
    mq3_warning:  Mapped[float] = mapped_column(Float, nullable=False, default=20.0)
    mq3_critical: Mapped[float] = mapped_column(Float, nullable=False, default=30.0)

    # MQ135 (0–100 firmware scale)
    mq135_warning:  Mapped[float] = mapped_column(Float, nullable=False, default=25.0)
    mq135_critical: Mapped[float] = mapped_column(Float, nullable=False, default=40.0)

    # MQ4 (0–100 firmware scale)
    mq4_warning:  Mapped[float] = mapped_column(Float, nullable=False, default=20.0)
    mq4_critical: Mapped[float] = mapped_column(Float, nullable=False, default=35.0)

    updated_at: Mapped[datetime] = mapped_column(DateTime(timezone=True), server_default=func.now(), onupdate=func.now())

    device: Mapped["Device"] = relationship(back_populates="thresholds", lazy="noload")
