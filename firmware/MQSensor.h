/**
 * @file        MQSensor.h
 * @brief       Reusable analog scaling for MQ-series gas sensors.
 * @project     AgriCoolHub Solar Hybrid Agri Cool Hub
 * @platform    ESP32-S3 (Arduino Framework)
 *
 * @section CircuitAssumption
 * Expected voltage-divider topology per sensor:
 *
 *   MQ AO ──┬── 10 kΩ (top) ──┬── ESP32-S3 ADC pin
 *            └─────────────────┘
 *                              └── 20 kΩ equiv. (bottom) ── GND
 *
 *   V_ADC    = V_sensor × (20 / (10 + 20))
 *   V_sensor = V_ADC × 1.5
 *
 * VERIFY actual resistor values with a multimeter before powering on.
 * All ADC inputs must remain ≤ 3.3 V; ESP32-S3 GPIOs are NOT 5 V tolerant.
 *
 * @section InterpretationRestriction
 * The percentage values produced by readMQSensor() are NORMALIZED ANALOG
 * OUTPUT PERCENTAGES, not calibrated gas concentrations, not PPM, and not
 * safety-certified measurements. Do not use them as gas-leak alarms or
 * concentration indicators without proper calibration and a validated model.
 *
 * @section MathematicalMaximum
 * Under the stated assumptions the maximum achievable percentage is:
 *   (3.3 × 1.5 / 5.0) × 100 = 99 %
 * Do not alter the divider ratio to force 100 %.
 */

#ifndef MQSENSOR_H
#define MQSENSOR_H

#include <Arduino.h>

// =====================================================
// MQ Reading Structure
// =====================================================

/**
 * @brief  Holds all intermediate values for one MQ sensor acquisition.
 *
 * Fields:
 *  rawADC       — raw 12-bit ADC reading (0–4095)
 *  pinVoltage   — voltage at the ADC pin (V), after the divider
 *  sensorVoltage— estimated voltage at the sensor AO output (V)
 *  percentage   — normalised output percentage (0–100), clamped
 */
struct MQReading
{
    int   rawADC;
    float pinVoltage;
    float sensorVoltage;
    float percentage;
};

// =====================================================
// Public API
// =====================================================

/**
 * @brief  Initializes ADC resolution. Call once during setup().
 */
void initMQSensors();

/**
 * @brief  Reads one MQ sensor and returns all intermediate scaling values.
 *
 * Algorithm:
 *  1. Read analogRead(pin) → rawADC
 *  2. pinVoltage    = (rawADC / 4095.0) × 3.3
 *  3. sensorVoltage = pinVoltage × VOLTAGE_DIVIDER_RATIO  (default 1.5)
 *  4. percentage    = (sensorVoltage / 5.0) × 100, clamped 0–100
 *
 * @param  pin  GPIO number to read (e.g. MQ3_ANALOG_PIN).
 * @return MQReading struct with raw and computed values.
 */
MQReading readMQSensor(uint8_t pin);

/**
 * @brief  Returns true if the MQ warm-up period (MQ_WARMUP_MS) has elapsed.
 *         Readings before warm-up are technically valid ADC numbers but may
 *         not represent stable sensor resistance.
 */
bool isMQWarmedUp();

#endif // MQSENSOR_H
