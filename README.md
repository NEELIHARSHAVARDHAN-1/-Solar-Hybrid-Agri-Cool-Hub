# Solar Hybrid Agri Cool Hub

End-to-end IoT system for agricultural cold-storage monitoring.

## Architecture

```
ESP32-S3 N16R8
  ├─ SHT40 (I2C)       — temperature + humidity
  ├─ DS3231 (I2C)      — RTC timestamps
  ├─ INA219 (I2C)      — battery monitoring
  ├─ MQ-3/135/4 (ADC)  — raw gas sensor readings
  └─ SD card (SPI)     — offline queue
         │
         │  Wi-Fi + MQTT TLS
         ▼
  HiveMQ Cloud (MQTT broker)
  topic: agrihub/telemetry
         │
         ▼
  FastAPI Backend (Python 3.10+)
  ├─ MQTT subscriber + telemetry ingestion
  ├─ Alert engine (configurable thresholds)
  └─ REST API
         │
         ▼
  PostgreSQL Database
         │
         ▼
  Native Android App (Java/Kotlin)
  ├─ Splash (Lottie animation + branding)
  ├─ Login (Farmer ID + password)
  ├─ Dashboard (live sensor parameter cards)
  ├─ History (24-hour MPAndroidChart line charts)
  ├─ Alerts (active + resolved, tabbed)
  └─ Settings (backend URL + logout)
```

## Folder Map

| Folder | Contents |
|--------|----------|
| `firmware/` | ESP32-S3 Arduino IDE sketch (`firmware.ino`) |
| `firmware/src/` | All `.h` / `.cpp` modules |
| `backend/` | FastAPI Python backend |
| `backend/app/` | Application source code |
| `backend/tests/` | Pytest test suite (24 tests) |
| `app/` | Native Android application (Java/Kotlin) |
| `docs/` | Wiring, API, deployment, configuration, troubleshooting |

## Quick Start

### 1. Hardware Setup

See **[`docs/WIRING.md`](docs/WIRING.md)** for full pin assignments and MQ sensor voltage guidance.

| Component | Interface | GPIO |
|-----------|-----------|------|
| SHT40 (temp/humidity) | I2C | SDA=8, SCL=9 |
| DS3231 (RTC) | I2C | SDA=8, SCL=9 |
| INA219 (battery) | I2C | SDA=8, SCL=9 |
| SD card | SPI | MOSI=11, MISO=13, CLK=12, CS=10 |
| MQ-3 | ADC | GPIO 4 |
| MQ-135 | ADC | GPIO 5 |
| MQ-4 | ADC | GPIO 6 |

### 2. MQTT Broker (HiveMQ Cloud)

1. Register at https://www.hivemq.com/mqtt-cloud-broker/ (free tier).
2. Note your cluster hostname, username, and password.

### 3. Firmware

```
1. Install Arduino IDE 2.x and ESP32 board package (see firmware/library_list.txt)
2. Copy firmware/src/config_secrets.h.example → firmware/src/config_secrets.h
3. Fill in WIFI_SSID, WIFI_PASSWORD, MQTT_HOST, MQTT_USER, MQTT_PASSWORD
4. Open firmware/firmware.ino in Arduino IDE
5. Board: ESP32S3 Dev Module | Flash: 16MB | Upload Speed: 921600
6. Click Upload
```

See **[`firmware/FLASH_INSTRUCTIONS.md`](firmware/FLASH_INSTRUCTIONS.md)** for detailed steps.

### 4. Backend

```bash
cd backend
cp .env.example .env            # fill in DATABASE_URL and MQTT credentials
docker-compose up -d            # start local PostgreSQL
pip install -r requirements.txt # use Python 3.10 or 3.11
uvicorn app.main:app --reload --host 0.0.0.0 --port 8000
```

API docs: http://localhost:8000/docs

See **[`docs/DEPLOYMENT.md`](docs/DEPLOYMENT.md)** for cloud deployment.

### 5. Run Tests

```bash
cd backend
pytest tests/ -v
# Expected: 24 passed
```

### 6. Android App

```bash
# Open the 'app/' folder in Android Studio (Hedgehog or newer)
# Let Gradle sync → Run on emulator or device

# OR build from command line:
cd app
./gradlew assembleDebug
./gradlew installDebug
```

Demo credentials: `farmer001` / `farm@2024` (see [`app/README.md`](app/README.md) for full list).

See **[`app/README.md`](app/README.md)** for build instructions, package layout, and Nielsen usability compliance map.

## Documentation

| Document | Description |
|----------|-------------|
| [`docs/WIRING.md`](docs/WIRING.md) | GPIO pin table, voltage divider guidance |
| [`docs/API.md`](docs/API.md) | Full REST API reference with examples |
| [`docs/CONFIGURATION.md`](docs/CONFIGURATION.md) | All config variables explained |
| [`docs/DEPLOYMENT.md`](docs/DEPLOYMENT.md) | Backend and MQTT cloud setup |
| [`docs/HARDWARE_TEST_CHECKLIST.md`](docs/HARDWARE_TEST_CHECKLIST.md) | Automated + hardware test procedures |
| [`docs/TROUBLESHOOTING.md`](docs/TROUBLESHOOTING.md) | Common failure modes and fixes |
| [`firmware/FLASH_INSTRUCTIONS.md`](firmware/FLASH_INSTRUCTIONS.md) | Flash guide |
| [`firmware/library_list.txt`](firmware/library_list.txt) | Required Arduino libraries |

## Hardware Notes

- **MQ sensor ADC voltage:** MQ sensors powered at 5 V **require** a 10 kΩ/10 kΩ voltage
  divider on AOUT before the ESP32 GPIO pin. See `docs/WIRING.md`.
- **MQ readings are raw ADC integers** (0–4095). No calibrated gas concentrations are implied.
- **Battery** uses INA219 with linear voltage mapping (4S LiPo: 12.0 V = 0%, 16.8 V = 100%).
  Adjust `BATTERY_FULL_V` / `BATTERY_EMPTY_V` in `firmware/src/config.h` for other battery chemistries.
- **Humidifier relay** is not installed in this deployment. `humidifier_enabled` field is omitted.
- **Location** coordinates (15.8281, 78.0373) are static configured values, not GPS-derived.

## Telemetry Payload Specification

```json
{
  "device_id": "agrihub-001",
  "timestamp": "2026-10-21T10:30:00+05:30",
  "temperature": 25.6,
  "humidity": 72.4,
  "mq3": 412,
  "mq135": 530,
  "mq4": 287,
  "battery_percent": 78,
  "latitude": 15.8281,
  "longitude": 78.0373
}
```

Optional fields (`temperature`, `humidity`, `battery_percent`, `latitude`, `longitude`) may be `null`.
`humidifier_enabled` is omitted when the actuator is not installed.
