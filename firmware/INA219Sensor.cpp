/**
 * @file        INA219Sensor.cpp
 * @brief       INA219 voltage, current, and power measurement implementation.
 * @project     AgriCoolHub Solar Hybrid Agri Cool Hub
 * @platform    ESP32-S3 (Arduino Framework)
 *
 * @section BatteryEstimationModel
 * The INA219 measures the USB power bank output rail (nominally 5 V).
 *
 * Model: USB power bank output voltage → battery percentage
 *
 * A USB power bank regulates its output to ~5 V and holds that level
 * until internal cells are nearly exhausted, then the output voltage
 * drops. The observable range on the output rail is:
 *
 *   >= 5.10 V  → 100 %  (full / charging)
 *   ~  4.90 V  → ~50 %  (mid charge)
 *   <= 4.60 V  →   0 %  (cutoff / nearly dead)
 *
 * Linear interpolation between 4.60 V (0%) and 5.20 V (100%).
 * Clamped to 0–100.
 *
 * LIMITATION: This is an approximate model for a regulated USB output.
 * It does not represent the internal Li-ion cell voltage or true SoC.
 * Replace with a calibrated curve if measuring a bare LiPo directly.
 *
 * Readings below 4.0 V or above 6.0 V are treated as invalid (NaN).
 *
 * @section Library
 * Adafruit INA219 Library — install via Arduino Library Manager.
 */

#include <Wire.h>
#include <Adafruit_INA219.h>
#include <math.h>

#include "INA219Sensor.h"
#include "telemetry.h"
#include "sd_manager.h"

// =====================================================
// Battery Voltage Model Constants (USB power bank)
// =====================================================

static constexpr float BATT_V_MIN   = 4.60f;   // 0 %  — output sagging, nearly dead
static constexpr float BATT_V_MAX   = 5.20f;   // 100 % — healthy full output
static constexpr float BATT_V_LOW   = 4.00f;   // below this → invalid reading
static constexpr float BATT_V_HIGH  = 6.00f;   // above this → invalid / not a power bank

// =====================================================
// Internal State
// =====================================================

static Adafruit_INA219 ina219;
static bool ina219Available = false;

// =====================================================
// Initialization
// =====================================================

bool initINA219()
{
    if (!ina219.begin())
    {
        Serial.println("[INA219] NOT DETECTED — check I2C wiring (SDA=8, SCL=9)");
        logFault("INA219_INIT_FAIL");
        ina219Available = false;
        return false;
    }

    ina219Available = true;
    Serial.println("[INA219] Initialized");
    return true;
}

// =====================================================
// Battery Percentage Calculation
// =====================================================

/**
 * @brief  Converts USB power bank output voltage to an estimated charge %.
 *
 * Linear map: BATT_V_MIN (0%) → BATT_V_MAX (100%), clamped.
 * Returns NaN for voltages outside the plausible power bank range.
 *
 * @param  busV  INA219 bus voltage in Volts
 * @return float  0–100 %, or NaN if voltage is out of valid range
 */
static float voltageToPercent(float busV)
{
    if (isnan(busV) || busV < BATT_V_LOW || busV > BATT_V_HIGH)
    {
        return NAN;
    }

    float pct = ((busV - BATT_V_MIN) / (BATT_V_MAX - BATT_V_MIN)) * 100.0f;

    // Clamp to 0–100
    if (pct < 0.0f)   pct = 0.0f;
    if (pct > 100.0f) pct = 100.0f;

    return pct;
}

// =====================================================
// Read
// =====================================================

/**
 * @brief  Samples all INA219 channels and updates telemetry.battery.
 */
INA219Reading readINA219()
{
    INA219Reading r;

    if (!ina219Available)
    {
        r.busVoltage_V    = NAN;
        r.shuntVoltage_mV = NAN;
        r.current_mA      = NAN;
        r.power_mW        = NAN;
        r.valid           = false;
        telemetry.battery = NAN;
        return r;
    }

    r.shuntVoltage_mV = ina219.getShuntVoltage_mV();
    r.busVoltage_V    = ina219.getBusVoltage_V();
    r.current_mA      = ina219.getCurrent_mA();
    r.power_mW        = ina219.getPower_mW();
    r.valid           = true;

    // Estimate battery % from output rail voltage
    telemetry.battery = voltageToPercent(r.busVoltage_V);

    Serial.print("[INA219] Bus: ");
    Serial.print(r.busVoltage_V, 3);
    Serial.print(" V | Shunt: ");
    Serial.print(r.shuntVoltage_mV, 2);
    Serial.print(" mV | I: ");
    Serial.print(r.current_mA, 2);
    Serial.print(" mA | P: ");
    Serial.print(r.power_mW, 2);
    Serial.print(" mW | Battery: ");
    if (isnan(telemetry.battery))
        Serial.println("null");
    else
    {
        Serial.print(telemetry.battery, 1);
        Serial.println("%");
    }

    return r;
}
