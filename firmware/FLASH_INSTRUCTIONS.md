# Firmware — Build and Flash Instructions

## Prerequisites

1. Install **Arduino IDE 2.x** from https://www.arduino.cc/en/software
2. Install the **ESP32 Arduino core** (see `library_list.txt` — Board Package section).
3. Install all libraries listed in `library_list.txt`.
4. Copy `src/config_secrets.h.example` → `src/config_secrets.h` and fill in credentials.

## Board Settings (Tools menu)

| Setting | Value |
|---------|-------|
| Board | ESP32S3 Dev Module |
| Flash Size | 16MB (128Mb) |
| PSRAM | OPI PSRAM |
| Flash Mode | QIO 80MHz |
| Upload Speed | 921600 |
| USB CDC On Boot | Enabled |
| Partition Scheme | Default 4MB with spiffs (adjust if needed) |
| Port | (select your ESP32 COM port) |

## First Flash Procedure

1. Open `firmware/firmware.ino` in Arduino IDE.
2. Verify board settings above.
3. Click **Verify** (✓) to check for compilation errors.
4. Hold the **BOOT** button on the ESP32-S3, click **Upload**, then release BOOT.
5. Wait for "Done uploading."
6. Open **Serial Monitor** at **115200 baud**.

## First-Boot RTC Sync

On first boot (or if the RTC coin cell died), the sketch automatically calls
`rtc.setFromCompile()` to set the DS3231 to the build timestamp.

**Important:** The compile time is baked in at build time. If you want accurate
time, build and flash immediately, or set the RTC time manually via a separate
sketch using `RTClib`'s `rtc.adjust(DateTime(YYYY, MM, DD, HH, MM, SS))`.

After the RTC is set, it runs from its own coin cell and is never overwritten
by subsequent firmware uploads.

## Serial Monitor — Expected Output

```
[Main] Solar Hybrid Agri Cool Hub — booting...
[RTCManager] DS3231 initialised.
[SensorManager] SHT40 initialised.
[BatteryMonitor] INA219 initialised.
[SDQueue] SD card mounted.
[Network] NetworkManager ready.
[Main] Boot complete. Entering loop.
[Network] Connecting to Wi-Fi: YOUR_SSID
[Network] Wi-Fi connected. IP: 192.168.x.x
[Network] Connecting to MQTT broker: xxx.hivemq.cloud
[Network] MQTT connected.
[Main] MQTT connected — replaying SD queue.
[SDQueue] Replay done. Published=0 Retained=0 Corrupted=0
[Main] Payload: {"device_id":"agrihub-001","timestamp":"2026-...","temperature":25.6,...}
[Main] Published OK.
```

## Troubleshooting

| Symptom | Likely Cause | Action |
|---------|-------------|--------|
| SHT40 not found | Wrong SDA/SCL or loose wire | Check GPIO8/9, run I2C scanner |
| DS3231 not found | Same I2C bus issue | Check GPIO8/9 |
| INA219 not found | Address conflict or wiring | Verify 0x40 address, check A0/A1 pins |
| SD card not found | SPI wiring or CS pin | Verify GPIO10/11/12/13 |
| MQTT TLS fails | Wrong CA cert or broker host | Check MQTT_HOST in config_secrets.h |
| Backoff looping | Bad credentials | Check WIFI_SSID, WIFI_PASSWORD, MQTT_USER |
