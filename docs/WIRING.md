# Wiring Guide — Solar Hybrid Agri Cool Hub

## ESP32-S3 N16R8 Pin Assignments

### I2C Bus (shared by SHT40, DS3231, INA219)

| Signal | ESP32-S3 GPIO | Notes |
|--------|---------------|-------|
| SDA    | GPIO 8        | Pull-up 4.7 kΩ to 3.3 V recommended |
| SCL    | GPIO 9        | Pull-up 4.7 kΩ to 3.3 V recommended |

All three I2C devices share the same SDA/SCL lines. Addresses:
- SHT40: `0x44`
- DS3231: `0x68`
- INA219: `0x40` (default, A0=A1=GND)

### SPI Bus — SD Card Module

| Signal | ESP32-S3 GPIO |
|--------|---------------|
| MOSI   | GPIO 11       |
| MISO   | GPIO 13       |
| CLK    | GPIO 12       |
| CS     | GPIO 10       |

Use a 5 V to 3.3 V level-shifted SD card module, or a module with onboard
level-shifting (most common breakout boards include this).

### ADC Pins — MQ Gas Sensors

| Sensor | ESP32-S3 GPIO | Notes |
|--------|---------------|-------|
| MQ-3   | GPIO 4        | AOUT pin — see voltage note below |
| MQ-135 | GPIO 5        | AOUT pin |
| MQ-4   | GPIO 6        | AOUT pin |

**⚠️ VOLTAGE WARNING:**

The ESP32-S3 ADC pins are rated for maximum 3.3 V input.

- If MQ sensors are powered at **3.3 V**: a 10 kΩ / 10 kΩ voltage divider
  on AOUT is still recommended to keep the maximum voltage within safe range
  (divides to ≤ 1.65 V, increasing effective resolution in the lower range).

- If MQ sensors are powered at **5 V**: a voltage divider is **required**.
  Use 10 kΩ (upper) / 10 kΩ (lower) to divide 5 V AOUT down to ≤ 2.5 V.
  Do NOT connect 5 V signals directly to ESP32-S3 GPIO pins.

**Voltage divider circuit for 5 V supply:**
```
  MQ AOUT ──── R1 (10 kΩ) ──── ADC PIN
                                 │
                              R2 (10 kΩ)
                                 │
                               GND
```

### INA219 — Battery Monitor

| Signal | ESP32-S3 GPIO / Rail | Notes |
|--------|---------------------|-------|
| SDA    | GPIO 8              | Shared I2C bus |
| SCL    | GPIO 9              | Shared I2C bus |
| VCC    | 3.3 V rail          |       |
| GND    | GND                 |       |
| VIN+   | Battery positive    | Before shunt |
| VIN-   | Battery load path   | After shunt (0.1 Ω) |

The INA219 breakout board (Adafruit) includes a 0.1 Ω shunt resistor.
Calibrated for 32 V / 2 A range in firmware. Adjust `BATTERY_FULL_V` and
`BATTERY_EMPTY_V` in `config.h` if your battery pack is not 4S LiPo.

---

## Power Supply

The ESP32-S3 must be powered from a stable 3.3 V regulated supply.
The SHT40, DS3231, and INA219 are all 3.3 V devices.
MQ sensors require 5 V for the heater coil — supply from the 5 V rail
if your board provides one, then use a voltage divider on AOUT as described above.

The DS3231 RTC requires a CR2032 coin cell backup battery to maintain time
when the main supply is removed.

---

## I2C Scanner Sketch

If a device is not detected, flash this minimal sketch to find addresses:

```cpp
#include <Wire.h>
void setup() {
  Serial.begin(115200);
  Wire.begin(8, 9); // SDA=8, SCL=9
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found device at 0x");
      Serial.println(addr, HEX);
    }
  }
  Serial.println("Scan complete.");
}
void loop() {}
```

Expected output with all three I2C devices connected:
```
Found device at 0x40   (INA219)
Found device at 0x44   (SHT40)
Found device at 0x68   (DS3231)
```
