/**
 * @file        INA219Sensor.h
 * @brief       INA219 voltage, current, and power measurement interface.
 * @project     AgriCoolHub Solar Hybrid Agri Cool Hub
 * @platform    ESP32-S3 (Arduino Framework)
 *
 * @section Hardware
 * INA219 communicates over I2C (default address 0x40).
 * Shares the bus on GPIO8 (SDA) / GPIO9 (SCL).
 *
 * @section BatteryEstimation
 * INA219 measures bus voltage, shunt voltage, current, and power.
 * It does NOT measure battery state-of-charge directly.
 * telemetry.battery is set to NaN until a validated estimation model
 * (based on battery chemistry and voltage-to-SoC curve) is implemented.
 *
 * @section Library
 * Requires: Adafruit INA219 Library (install via Arduino Library Manager)
 * Depends on: Adafruit BusIO
 */

#ifndef INA219SENSOR_H
#define INA219SENSOR_H

#include <Arduino.h>

// =====================================================
// INA219 Diagnostic Reading
// =====================================================

/**
 * @brief  Snapshot of all INA219 electrical measurements.
 *         Not part of the 10-field MQTT contract, but available for
 *         diagnostics and future battery-estimation logic.
 */
struct INA219Reading
{
    float busVoltage_V;    // load-side bus voltage (V)
    float shuntVoltage_mV; // voltage across shunt resistor (mV)
    float current_mA;      // calculated current through shunt (mA)
    float power_mW;        // power (mW)
    bool  valid;           // false if sensor is absent or read failed
};

// =====================================================
// Public API
// =====================================================

/**
 * @brief  Initializes the INA219 sensor over I2C.
 * @return true if sensor detected and configured, false otherwise.
 */
bool initINA219();

/**
 * @brief  Reads all INA219 electrical measurements.
 *         telemetry.battery remains NaN (no validated SoC model present).
 * @return INA219Reading struct with current electrical snapshot.
 */
INA219Reading readINA219();

#endif // INA219SENSOR_H
