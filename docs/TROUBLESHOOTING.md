# Troubleshooting Guide — Solar Hybrid Agri Cool Hub

---

## Firmware Issues

### SHT40 / DS3231 / INA219 not found on I2C bus

**Symptom:** Serial shows `[SensorManager] SHT40 not found on I2C bus.`

**Causes and fixes:**
1. Loose or swapped SDA/SCL wires — verify GPIO8=SDA, GPIO9=SCL.
2. Missing pull-up resistors — add 4.7 kΩ pull-ups to 3.3 V on SDA and SCL.
3. Wrong I2C address — run the I2C scanner sketch in `docs/WIRING.md`.
4. Device not powered — verify 3.3 V supply to VCC of each module.
5. Two devices sharing the same address — INA219 A0/A1 pins must be grounded (address 0x40).

---

### MQ sensor reads always 0 or always 4095

**Symptom:** `mq3`, `mq135`, or `mq4` always shows 0 or 4095.

**Causes and fixes:**
1. Sensor heater not warmed up — wait 30–60 seconds after power-on for stable readings.
2. Pin not connected to AOUT — confirm the ADC wire goes to the analog output pin, not DOUT.
3. Voltage too high — if powered at 5 V without a divider, the ADC pin may be clamped at maximum.
4. Wrong GPIO — verify GPIO4, GPIO5, GPIO6 respectively.

---

### SD card not found

**Symptom:** `[SDQueue] SD card not found or failed to mount.`

**Causes and fixes:**
1. Loose SPI wires — check GPIO10/11/12/13.
2. SD card not formatted as FAT32 — reformat using a PC.
3. CS pin mismatch — verify `SD_CS_PIN = 10` in `config.h`.
4. SPI not initialised before `SD.begin()` — the main sketch calls `SPI.begin()` first; confirm the order in `setup()`.

---

### MQTT connection fails with backoff loop

**Symptom:** `[Network] MQTT connect returned false. RC=-2` or similar.

**Causes and fixes:**
- RC -2 = connection lost / refused by broker.
  - Check `MQTT_HOST`, `MQTT_USER`, `MQTT_PASSWORD` in `config_secrets.h`.
  - Confirm port is 8883 (TLS), not 1883.
  - In HiveMQ Cloud, confirm the user has permission to publish to `agrihub/telemetry`.
- RC -4 = connection timeout — check Wi-Fi connectivity first.
- RC 5 = not authorized — wrong credentials.

---

### Telemetry timestamp is wrong (year 1970 or 2000)

**Symptom:** Timestamps show epoch or incorrect date.

**Causes and fixes:**
1. DS3231 coin cell dead or never set — serial shows `WARNING: DS3231 lost power`.
   - Call `rtc.setFromCompile()` by ensuring the first-flash warning message is printed
     and the RTC is set to compile time automatically.
   - If the coin cell is dead, replace it (CR2032) and reflash to resync.
2. RTC set to local time instead of UTC — the firmware adds `TZ_OFFSET_SECONDS` to
   whatever the RTC stores. The RTC must store UTC; the offset produces IST output.

---

## Backend Issues

### `ModuleNotFoundError: No module named 'asyncpg'`

**Fix:** Install on Python ≤ 3.12 where wheels are available:
```bash
pip install asyncpg
```
On Windows, you need [Microsoft C++ Build Tools](https://visualstudio.microsoft.com/visual-cpp-build-tools/) 
if no pre-built wheel exists for your Python version.
For development and testing, the aiosqlite SQLite backend is used instead.

---

### Backend starts but MQTT never receives messages

**Check:**
1. `MQTT_HOST`, `MQTT_USER`, `MQTT_PASSWORD` in `backend/.env` match HiveMQ Cloud.
2. Topic `agrihub/telemetry` matches firmware `MQTT_TOPIC_TELEMETRY`.
3. Backend logs show `MQTT connected. Subscribing to agrihub/telemetry`.
4. HiveMQ Cloud dashboard shows a connected client named `agrihub-backend`.

---

### Database connection error on startup

**Symptom:** `sqlalchemy.exc.OperationalError: could not connect to server`

**Fix:**
1. For local dev: `docker-compose up -d` in `backend/`.
2. Check `DATABASE_URL` in `.env` — must use `postgresql+asyncpg://` scheme.
3. Verify PostgreSQL host and port are reachable.

---

### Alert not created despite reading exceeding threshold

**Check:**
1. Threshold values in `.env` (e.g. `ALERT_TEMP_MAX=35.0`).
2. An existing unresolved alert of the same type may be suppressing new ones.
   Resolve the existing alert via `PATCH /alerts/{id}/resolve` or via the app.

---

## Flutter App Issues

### App shows "Failed to load data"

**Fix:**
1. Check the backend URL in **Settings** — include scheme and port, no trailing slash.
   Example: `http://192.168.1.100:8000`
2. On Android emulator, use `http://10.0.2.2:8000` to reach `localhost`.
3. Confirm the backend is running and `/health` responds.
4. On a physical device on a different network, use a public URL (Railway/Render).

---

### History chart shows no data

**Cause:** No telemetry data for the last 24 hours in the database.

**Fix:** Confirm the device is publishing data. Check `/devices/agrihub-001/telemetry` API directly.

---

### Battery percentage shows "N/A"

**Expected behavior:** `battery_percent` is `null` if INA219 is not detected or `ENABLE_BATTERY` is false in firmware. The app shows "N/A" in this case. This is not an error.
