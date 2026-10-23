# Configuration Guide — Solar Hybrid Agri Cool Hub

## Firmware Configuration

### `firmware/src/config.h` — Hardware Constants and Timing

| Constant | Default | Description |
|----------|---------|-------------|
| `I2C_SDA_PIN` | 8 | I2C SDA GPIO |
| `I2C_SCL_PIN` | 9 | I2C SCL GPIO |
| `SD_MOSI_PIN` | 11 | SD SPI MOSI |
| `SD_MISO_PIN` | 13 | SD SPI MISO |
| `SD_CLK_PIN` | 12 | SD SPI CLK |
| `SD_CS_PIN` | 10 | SD chip-select |
| `MQ3_ADC_PIN` | 4 | MQ-3 ADC input |
| `MQ135_ADC_PIN` | 5 | MQ-135 ADC input |
| `MQ4_ADC_PIN` | 6 | MQ-4 ADC input |
| `ENABLE_BATTERY` | true | Enable INA219 monitoring |
| `ENABLE_HUMIDIFIER` | false | Enable relay (not installed) |
| `BATTERY_FULL_V` | 16.8 | 4S LiPo fully charged voltage |
| `BATTERY_EMPTY_V` | 12.0 | 4S LiPo depleted voltage |
| `DEVICE_ID` | "agrihub-001" | Unique device identifier |
| `DEVICE_LATITUDE` | 15.8281 | Static GPS coordinate (configured, not derived) |
| `DEVICE_LONGITUDE` | 78.0373 | Static GPS coordinate |
| `TZ_OFFSET_SECONDS` | 19800 | IST = UTC+05:30 |
| `TELEMETRY_INTERVAL_MS` | 30000 | Sensor reading interval (ms) |
| `WIFI_BACKOFF_BASE_MS` | 2000 | Initial Wi-Fi retry delay (ms) |
| `WIFI_BACKOFF_MAX_MS` | 120000 | Maximum Wi-Fi retry delay (ms) |
| `MQTT_TOPIC_TELEMETRY` | "agrihub/telemetry" | MQTT publish topic |
| `MQTT_TOPIC_COMMAND` | "agrihub/command" | MQTT subscribe topic (reserved) |
| `MQTT_BUFFER_SIZE` | 512 | PubSubClient payload buffer (bytes) |
| `SD_QUEUE_FILE` | "/queue.jsonl" | SD card queue file path |
| `SD_MAX_FILE_BYTES` | 524288 | Max queue file size (512 KB) |

### `firmware/src/config_secrets.h` — Credentials (never commit)

Copy `config_secrets.h.example` → `config_secrets.h` and fill in real values:

| Constant | Description |
|----------|-------------|
| `WIFI_SSID` | Your Wi-Fi network name |
| `WIFI_PASSWORD` | Your Wi-Fi password |
| `MQTT_HOST` | HiveMQ Cloud hostname (e.g. `abc.hivemq.cloud`) |
| `MQTT_PORT` | 8883 (TLS) |
| `MQTT_USER` | HiveMQ Cloud username |
| `MQTT_PASSWORD` | HiveMQ Cloud password |

---

## Backend Configuration

### `backend/.env` — Environment Variables

Copy `backend/.env.example` → `backend/.env` and fill in:

| Variable | Default | Description |
|----------|---------|-------------|
| `DATABASE_URL` | (see below) | Async PostgreSQL URL with asyncpg driver |
| `MQTT_HOST` | — | HiveMQ Cloud hostname |
| `MQTT_PORT` | 8883 | MQTT TLS port |
| `MQTT_USER` | — | MQTT username |
| `MQTT_PASSWORD` | — | MQTT password |
| `MQTT_TOPIC_TELEMETRY` | agrihub/telemetry | Must match firmware `MQTT_TOPIC_TELEMETRY` |
| `MQTT_CLIENT_ID` | agrihub-backend | MQTT client ID |
| `ALERT_TEMP_MAX` | 35.0 | Temperature alert threshold (°C) |
| `ALERT_HUMIDITY_MAX` | 90.0 | Humidity alert threshold (%) |
| `ALERT_MQ3_MAX` | 700 | MQ-3 raw ADC alert threshold |
| `ALERT_MQ135_MAX` | 800 | MQ-135 raw ADC alert threshold |
| `ALERT_MQ4_MAX` | 700 | MQ-4 raw ADC alert threshold |
| `DEVICE_OFFLINE_MINUTES` | 5 | Minutes without data before device is offline |

**DATABASE_URL formats:**

Local (Docker Compose):
```
DATABASE_URL=postgresql+asyncpg://agrihub:agrihub@localhost:5432/agrihub
```

Cloud (Railway, Render, Supabase — replace placeholders):
```
DATABASE_URL=postgresql+asyncpg://USER:PASSWORD@HOST:PORT/DATABASE
```

---

## Alert Thresholds

MQ sensor values are **raw 12-bit ADC integers** (0–4095). They do not represent
calibrated gas concentrations. The default thresholds are indicative starting points.
Adjust them based on observed baseline values in your deployment environment.

| Alert Type | Triggered When | Default Threshold |
|------------|---------------|-------------------|
| TEMP_HIGH | temperature > threshold | 35.0 °C |
| HUMIDITY_HIGH | humidity > threshold | 90.0 % |
| MQ3_HIGH | mq3 > threshold | 700 (raw ADC) |
| MQ135_HIGH | mq135 > threshold | 800 (raw ADC) |
| MQ4_HIGH | mq4 > threshold | 700 (raw ADC) |
