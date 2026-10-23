/**
 * @file        MQSensor.cpp
 * @brief       Reusable analog scaling implementation for MQ-series gas sensors.
 * @project     AgriCoolHub Solar Hybrid Agri Cool Hub
 * @platform    ESP32-S3 (Arduino Framework)
 *
 * @section MathSummary
 *   pinVoltage    = (rawADC / ADC_MAX_READING) * ESP32_REF_VOLTAGE
 *   sensorVoltage = pinVoltage * VOLTAGE_DIVIDER_RATIO
 *   percentage    = (sensorVoltage / SENSOR_FULL_SCALE_V) * 100
 *   percentage    = constrain(percentage, 0, 100)
 *
 * Maximum achievable: (3.3 * 1.5 / 5.0) * 100 = 99 %
 *
 * @section DebugOutput
 * When MQ_DEBUG_SERIAL == 1 in config.h each call to readMQSensor() prints:
 *   [MQ<pin>] Raw: 1240 | ADC: 1.00 V | Sensor: 1.50 V | Output: 30.0%
 */

#include "MQSensor.h"
#include "config.h"

// =====================================================
// Initialization
// =====================================================

/**
 * @brief  Sets ADC resolution to 12 bits. Call once in setup().
 */
void initMQSensors()
{
    analogReadResolution(12);
    Serial.println("[MQ] ADC resolution set to 12-bit");
    Serial.print("[MQ] Warm-up period: ");
    Serial.print(MQ_WARMUP_MS / 1000UL);
    Serial.println(" s");
}

// =====================================================
// Warm-Up Check
// =====================================================

/**
 * @brief  Returns true once the warm-up period has elapsed since boot.
 *         Note: warm-up does not guarantee calibration; it only satisfies
 *         the minimum heater run time specified in the sensor datasheet.
 */
bool isMQWarmedUp()
{
    return (millis() >= MQ_WARMUP_MS);
}

// =====================================================
// Core Scaling Function
// =====================================================

/**
 * @brief  Reads one analog pin and computes the scaled MQ output.
 *
 * This function is the single reusable implementation used for MQ-3,
 * MQ-135, and MQ-4. Each sensor is read independently; one sensor's
 * ADC reading cannot overwrite another's because each call uses a
 * separate stack-allocated MQReading return value.
 *
 * @param  pin  ADC-capable GPIO (MQ3_ANALOG_PIN / MQ135_ANALOG_PIN / MQ4_ANALOG_PIN)
 * @return MQReading with rawADC, pinVoltage, sensorVoltage, percentage
 */
MQReading readMQSensor(uint8_t pin)
{
    MQReading result;

    // 1. Read raw ADC (12-bit, 0–4095)
    result.rawADC = analogRead(pin);

    // 2. Voltage at the ADC node (after the voltage divider)
    result.pinVoltage =
        ((float)result.rawADC / ADC_MAX_READING) * ESP32_REF_VOLTAGE;

    // 3. Estimated voltage at the sensor AO output (before the divider)
    result.sensorVoltage =
        result.pinVoltage * VOLTAGE_DIVIDER_RATIO;

    // 4. Normalized percentage relative to nominal 5 V full-scale
    result.percentage =
        (result.sensorVoltage / SENSOR_FULL_SCALE_V) * 100.0f;

    // 5. Clamp to valid range
    if (result.percentage < 0.0f)  result.percentage = 0.0f;
    if (result.percentage > 100.0f) result.percentage = 100.0f;

    // 6. Configurable serial diagnostics
#if MQ_DEBUG_SERIAL
    Serial.print("[MQ GPIO");
    Serial.print(pin);
    Serial.print("] Raw: ");
    Serial.print(result.rawADC);
    Serial.print(" | ADC: ");
    Serial.print(result.pinVoltage, 2);
    Serial.print(" V | Sensor: ");
    Serial.print(result.sensorVoltage, 2);
    Serial.print(" V | Output: ");
    Serial.print(result.percentage, 1);
    Serial.println("%");
#endif

    return result;
}
