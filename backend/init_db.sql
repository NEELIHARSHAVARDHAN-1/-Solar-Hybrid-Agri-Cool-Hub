-- =============================================================================
-- AgriHub Cool Storage Monitor — PostgreSQL initialization script
-- Run once to create tables. Idempotent (IF NOT EXISTS / ADD COLUMN IF NOT EXISTS).
-- For local dev: docker-compose up automatically runs this via
--   /docker-entrypoint-initdb.d/init_db.sql
-- For cloud: run manually: psql $DATABASE_URL -f init_db.sql
-- =============================================================================

CREATE TABLE IF NOT EXISTS devices (
    id         SERIAL PRIMARY KEY,
    device_id  VARCHAR(64) UNIQUE NOT NULL,
    last_seen  TIMESTAMPTZ,
    is_online  BOOLEAN NOT NULL DEFAULT FALSE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_devices_device_id ON devices(device_id);

CREATE TABLE IF NOT EXISTS telemetry (
    id              SERIAL PRIMARY KEY,
    device_id       VARCHAR(64) NOT NULL REFERENCES devices(device_id),
    timestamp       TIMESTAMPTZ NOT NULL,
    temperature     REAL,
    humidity        REAL,
    -- MQ values stored as 0-100 custom voltage scale (float)
    mq3             REAL NOT NULL,
    mq135           REAL NOT NULL,
    mq4             REAL NOT NULL,
    battery_percent INTEGER,
    latitude        REAL,
    longitude       REAL,
    received_at     TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    CONSTRAINT uq_telemetry_device_ts UNIQUE (device_id, timestamp)
);

CREATE INDEX IF NOT EXISTS idx_telemetry_device_ts ON telemetry(device_id, timestamp DESC);

CREATE TABLE IF NOT EXISTS alerts (
    id          SERIAL PRIMARY KEY,
    device_id   VARCHAR(64) NOT NULL REFERENCES devices(device_id),
    timestamp   TIMESTAMPTZ NOT NULL,
    -- alert_level: 'warning' | 'critical'
    alert_level VARCHAR(16) NOT NULL DEFAULT 'warning',
    alert_type  VARCHAR(32) NOT NULL,
    -- metric: human-readable label e.g. "Temperature (°C)"
    metric      VARCHAR(64) NOT NULL DEFAULT '',
    value       REAL NOT NULL,
    threshold   REAL NOT NULL,
    resolved_at TIMESTAMPTZ,
    created_at  TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_alerts_device_id ON alerts(device_id);
CREATE INDEX IF NOT EXISTS idx_alerts_resolved  ON alerts(device_id, resolved_at) WHERE resolved_at IS NULL;
CREATE INDEX IF NOT EXISTS idx_alerts_level     ON alerts(device_id, alert_level);

-- =============================================================================
-- Per-device configurable alert thresholds (warning + critical for each metric)
-- =============================================================================
CREATE TABLE IF NOT EXISTS device_thresholds (
    id                SERIAL PRIMARY KEY,
    device_id         VARCHAR(64) UNIQUE NOT NULL REFERENCES devices(device_id),
    -- Temperature °C
    temp_warning      REAL NOT NULL DEFAULT 25.0,
    temp_critical     REAL NOT NULL DEFAULT 30.0,
    -- Humidity %
    humidity_warning  REAL NOT NULL DEFAULT 70.0,
    humidity_critical REAL NOT NULL DEFAULT 85.0,
    -- MQ3 (0-100 scale)
    mq3_warning       REAL NOT NULL DEFAULT 20.0,
    mq3_critical      REAL NOT NULL DEFAULT 30.0,
    -- MQ135 (0-100 scale)
    mq135_warning     REAL NOT NULL DEFAULT 25.0,
    mq135_critical    REAL NOT NULL DEFAULT 40.0,
    -- MQ4 (0-100 scale)
    mq4_warning       REAL NOT NULL DEFAULT 20.0,
    mq4_critical      REAL NOT NULL DEFAULT 35.0,
    updated_at        TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_device_thresholds_device_id ON device_thresholds(device_id);

-- =============================================================================
-- Migration helpers for existing databases (safe to run multiple times)
-- =============================================================================
DO $$
BEGIN
    -- Add alert_level column if upgrading from v1
    IF NOT EXISTS (
        SELECT 1 FROM information_schema.columns
        WHERE table_name='alerts' AND column_name='alert_level'
    ) THEN
        ALTER TABLE alerts ADD COLUMN alert_level VARCHAR(16) NOT NULL DEFAULT 'warning';
    END IF;

    -- Add metric column if upgrading from v1
    IF NOT EXISTS (
        SELECT 1 FROM information_schema.columns
        WHERE table_name='alerts' AND column_name='metric'
    ) THEN
        ALTER TABLE alerts ADD COLUMN metric VARCHAR(64) NOT NULL DEFAULT '';
    END IF;

    -- Change mq3/mq135/mq4 from INTEGER to REAL if upgrading from v1
    IF EXISTS (
        SELECT 1 FROM information_schema.columns
        WHERE table_name='telemetry' AND column_name='mq3'
          AND data_type='integer'
    ) THEN
        ALTER TABLE telemetry
            ALTER COLUMN mq3   TYPE REAL USING mq3::real,
            ALTER COLUMN mq135 TYPE REAL USING mq135::real,
            ALTER COLUMN mq4   TYPE REAL USING mq4::real;
    END IF;
END $$;
