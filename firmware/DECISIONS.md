# FactorySense - Engineering Decisions Log

## Purpose

This document records important technical, hardware, software, testing, and project decisions made during the development of FactorySense.

The objective is to provide a quick reference explaining why decisions were made, what alternatives were considered, and the current project direction.

---

# Decision 001 - ESP32-S3 as Main Controller

**Decision**

Use ESP32-S3 as the primary controller.

**Reason**

* Integrated WiFi
* Sufficient processing capability
* PSRAM support
* Large ecosystem
* Suitable for Industrial IoT prototyping

**Status**

Implemented

---

# Decision 002 - MQTT as Communication Protocol

**Decision**

Use MQTT for telemetry communication.

**Reason**

* Lightweight protocol
* Reliable publish/subscribe model
* Industry standard for IoT
* Easy cloud integration

**Status**

Implemented

---

# Decision 003 - HiveMQ Cloud Broker

**Decision**

Use HiveMQ Cloud as the MQTT broker.

**Reason**

* Easy setup
* TLS support
* Cloud-hosted infrastructure
* MQTT Explorer compatibility

**Status**

Implemented

---

# Decision 004 - QoS 1 Telemetry Publishing

**Decision**

Use MQTT QoS 1 for telemetry delivery.

**Reason**

QoS 0 could result in telemetry loss during network interruptions.

QoS 1 provides acknowledgement-based delivery.

**Status**

Implemented

---

# Decision 005 - Device ID from ESP32 MAC Address

**Decision**

Generate device identifiers using ESP32 MAC addresses.

**Reason**

* Unique per device
* No manual configuration
* Consistent topic generation

**Status**

Implemented

---

# Decision 006 - JSON Telemetry Format

**Decision**

Use JSON for telemetry payloads.

**Reason**

* Human readable
* Easy backend parsing
* Flexible structure

**Status**

Implemented

---

# Decision 007 - Offline Telemetry Buffering

**Decision**

Store telemetry locally during network outages.

**Reason**

Prevent telemetry loss when WiFi or MQTT becomes unavailable.

**Current Configuration**

* FIFO Buffer
* 60 Message Capacity
* Automatic Replay

**Status**

Implemented

---

# Decision 008 - Automatic WiFi Recovery

**Decision**

Implement automatic WiFi reconnection.

**Reason**

Industrial deployments cannot depend on manual intervention.

**Current Configuration**

* Exponential Backoff
* Maximum Delay: 30 Seconds

**Status**

Implemented

---

# Decision 009 - MQTT Recovery Strategy

**Decision**

Implement automatic MQTT reconnection only after WiFi connectivity has been successfully established.

**Reason**

MQTT cannot operate without an active network connection. Attempting to reconnect while WiFi is unavailable wastes CPU time and network resources.

The firmware therefore prioritizes network recovery before initiating MQTT recovery.

**Current Behavior**

* MQTT connection is attempted only when WiFi is connected.
* Automatic reconnection is performed after WiFi recovery.
* QoS 1 ensures reliable telemetry delivery.
* Offline telemetry is buffered while MQTT is unavailable.
* Buffered telemetry is replayed after reconnection.

**Future Enhancement**

Implement MQTT exponential backoff reconnection to reduce repeated connection attempts during prolonged broker outages.

**Status**

Implemented

---

# Decision 010 - MQTT Last Will and Testament (LWT)

**Decision**

Publish device offline status using MQTT LWT.

**Reason**

Backend systems must detect unexpected device failures.

**Status**

Implemented

---

# Decision 011 - UTC Timestamps via NTP

**Decision**

Use UTC timestamps synchronized through NTP.

**Reason**

* Consistent time across deployments
* Easier backend processing
* Avoid timezone issues

**Status**

Implemented

---

# Decision 012 - DS18B20 Temperature Monitoring

**Decision**

Use DS18B20 for temperature sensing.

**Reason**

* Digital communication
* Reliable readings
* Widely used in industrial monitoring

**Additional Improvements**

* Sensor disconnect detection
* Range validation
* Moving average filtering

**Status**

Implemented

---

# Decision 013 - MPU6050 Vibration Monitoring

**Decision**

Use MPU6050 for vibration monitoring.

**Reason**

* Cost effective
* Easily available
* Suitable for machine vibration analysis

**Additional Improvements**

* Startup calibration
* Offset compensation
* Gravity compensation
* Moving average filtering
* DLPF set to 94 Hz

**Status**

Implemented

---

# Decision 014 - ACS712 Current Monitoring

**Decision**

Use ACS712 current sensor.

**Reason**

* Simple integration
* Non-invasive current measurement
* Suitable for industrial loads

**Additional Improvements**

* 20-second calibration
* Startup stabilization
* Deadband suppression
* Moving average filtering
* Empirical calibration factor tuning

**Status**

Implemented

---

# Decision 015 - RGB LED Diagnostic System

**Decision**

Use onboard RGB LED for diagnostics.

**Reason**

Field diagnostics should not require a laptop or serial monitor.

**Supported Indications**

* Boot
* Calibration
* WiFi Connecting
* MQTT Connecting
* MQTT Connected
* Publish Success
* Buffer Activity
* WiFi Lost
* MQTT Lost
* Sensor Fault
* Critical Fault

**Status**

Implemented

---

# Decision 016 - Watchdog Protection

**Decision**

Implement ESP32 Task Watchdog.

**Reason**

Recover from unexpected firmware lockups.

**Status**

Implemented

**Note**

Long-duration validation is ongoing.

---

# Decision 017 - Separate Power Sources

**Decision**

Use separate power sources for ESP32 and ACS712.

**Configuration**

ESP32:

* Powered from Protonics 10000 mAh Power Bank

ACS712:

* Powered from Samsung Adapter

Ground:

* Common Ground Connection

**Reason**

Improves measurement stability and reduces interference.

**Status**

Implemented

---

# Decision 018 - Common Ground Architecture

**Decision**

Maintain common ground across all supplies.

**Reason**

Required for stable analog measurements.

**Status**

Implemented

---

# Decision 019 - Relay Control Deferred

**Decision**

Remove relay from active deployment.

**Reason**

Testing showed:

* Voltage drops
* ADC instability
* Calibration drift
* Current measurement errors

**Status**

Deferred

**Future Plan**

Reintroduce after implementing proper power isolation.

---

# Decision 020 - Capacitor Across Motor Load

**Decision**

Add capacitor across motor terminals.

**Reason**

Reduce switching noise and voltage spikes.

**Benefits**

* Cleaner sensor readings
* Improved stability
* Reduced electrical noise

**Status**

Implemented During Testing

---

# Decision 021 - Noise Reduction Strategy

**Decision**

Treat noise reduction as a system-level requirement.

**Methods Used**

Hardware:

* Common Ground
* Stable Power Sources
* Motor Noise Suppression

Firmware:

* Moving Average Filtering
* Calibration Offsets
* Deadband Suppression

**Status**

Implemented

---

# Decision 022 - External WiFi Antenna

**Decision**

Use external ESP32 antenna configuration.

**Reason**

Improve signal strength and MQTT reliability.

**Benefits**

* Better RSSI
* More reliable communication
* Improved reconnection behavior

**Status**

Implemented

---

# Decision 023 - Startup Initialization Sequence

**Decision**

Complete all hardware initialization and sensor calibration before establishing cloud connectivity and beginning telemetry transmission.

**Reason**

FactorySense must always start in a known, validated state before transmitting operational data. Initializing and calibrating sensors first ensures that telemetry contains valid measurements and prevents unstable startup values from being published.

The startup sequence also supports autonomous operation during network outages by allowing the firmware to continue monitoring and buffering telemetry until connectivity is restored.

**Current Sequence**

1. Initialize core hardware peripherals.
2. Initialize RGB status LED.
3. Initialize SD card logging subsystem.
4. Initialize DS3231 RTC.
5. Initialize DS18B20 temperature sensor.
6. Initialize MPU6050 vibration sensor.
7. Calibrate MPU6050.
8. Initialize ACS712 current sensor.
9. Allow analog circuitry to stabilize.
10. Calibrate ACS712 (20-second zero-current calibration).

**Network Initialization**

After all sensors have been successfully initialized:

### If WiFi is Available

1. Connect to the configured WiFi network.
2. Synchronize the DS3231 RTC using NTP.
3. Initialize and connect the MQTT client.
4. Publish device ONLINE status.
5. Begin periodic telemetry acquisition.
6. Start event-driven telemetry publishing.

### If WiFi is Unavailable

1. Continue normal sensor monitoring.
2. Timestamp measurements using the DS3231 RTC.
3. Store telemetry in the offline FIFO buffer.
4. Begin WiFi recovery using exponential backoff.
5. Synchronize RTC using NTP once WiFi reconnects.
6. Connect MQTT only after WiFi has been successfully restored.
7. Replay buffered telemetry automatically.
8. Resume normal cloud publishing.

**Benefits**

* Sensors are fully calibrated before data transmission.
* Invalid startup measurements are avoided.
* Reliable operation without Internet connectivity.
* No telemetry loss during startup network failures.
* Automatic recovery without requiring a device reboot.
* Consistent startup behavior across deployments.

**Status**

Implemented

---

# Decision 024 - Industrial Monitoring First

**Decision**

Prioritize monitoring before machine control.

**Reason**

Reliable monitoring must be established before introducing actuation.

**Current Focus**

* Telemetry
* Reliability
* Recovery
* Diagnostics

**Status**

Active Project Strategy

---

# Decision 025 - Purchase of Digital Multimeter

**Decision**

Purchase a dedicated digital multimeter.

**Reason**

Sensor readings must be verified against a trusted reference.

**Used For**

* Voltage verification
* Current verification
* Calibration validation
* Circuit debugging

**Status**

Implemented

---

# Decision 026 - Physical Measurement Validation

**Decision**

Validate sensor values against real measurements.

**Reason**

Simulation results alone are insufficient.

**Examples**

* ACS712 calibration verification
* Power rail verification
* Sensor accuracy checks

**Status**

Implemented

---

# Decision 027 - Wokwi for Early Development

**Decision**

Use Wokwi for rapid development and physical hardware for validation.

**Reason**

Combines development speed with real-world testing.

**Status**

Implemented

---

# Decision 028 - MQTT Explorer as Primary Debug Tool

**Decision**

Use MQTT Explorer for telemetry validation.

**Reason**

Provides visibility into:

* Topics
* Payloads
* Device status
* Telemetry delivery

**Status**

Implemented

---

# Decision 029 - Detailed Serial Diagnostics

**Decision**

Maintain detailed serial logs during development.

**Reason**

Reduced debugging time significantly.

**Used For**

* Calibration
* MQTT Debugging
* WiFi Debugging
* Watchdog Testing
* Current Sensor Tuning

**Status**

Implemented

---

# Decision 030 - Calibration Before Feature Expansion

**Decision**

Calibration accuracy takes priority over adding new features.

**Reason**

Incorrect measurements reduce system value.

**Status**

Implemented

---

# Decision 031 - Reliability Before New Features

**Decision**

Prioritize stability improvements before feature expansion.

**Implemented First**

* Buffering
* Recovery Logic
* Calibration
* Diagnostics
* Watchdog

**Deferred**

* OTA
* Relay Control
* Additional Sensors

**Status**

Active Engineering Guideline

---

# Decision 032 - Flat Arduino Project Structure

**Decision**

Keep current repository structure.

**Reason**

Folder reorganization introduced build and include-path issues.

**Status**

Retained

**Future Plan**

Revisit after firmware stabilization.

---

# Decision 033 - TLS Certificate Verification

**Decision**

Enable TLS certificate verification using the MQTT broker's Root CA certificate.

**Reason**

Encrypted communication alone is insufficient. Certificate verification ensures the ESP32 communicates only with the intended MQTT broker and protects against man-in-the-middle attacks.

**Implementation**

* Root CA stored in `certs.h`
* TLS verification enabled
* Username/Password authentication retained

**Status**

Implemented

---

# Decision 034 - Documentation as Part of Development

**Decision**

Maintain documentation alongside firmware development.

**Documents**

* README.md
* DECISIONS.md
* Working Reports
* PR Documentation
* Source Code Documentation

**Status**

Implemented

---

# Decision 035 - Event-Based Telemetry Publishing

**Decision**

Publish telemetry only when meaningful state changes occur instead of transmitting every fixed interval.

**Reason**

Continuous telemetry publishing generates unnecessary network traffic, cloud storage usage, and MQTT broker load.

**Implementation**

Telemetry is published when:

* Temperature changes by ≥ 0.5°C
* Current changes by ≥ 0.1A
* Vibration changes by ≥ 0.05G
* Alarm state changes
* Heartbeat interval expires

**Benefits**

* Reduced bandwidth usage
* Reduced cloud storage
* Faster anomaly detection
* Lower MQTT traffic

**Status**

Implemented

---

# Decision 036 - Heartbeat-Based Device Presence

**Decision**

Transmit periodic heartbeat telemetry even when no process values change.

**Reason**

A silent device may be interpreted as offline by backend systems.

**Current Configuration**

* Heartbeat Interval: 5 Minutes

**Benefits**

* Device health monitoring
* Connectivity verification
* Backend visibility

**Status**

Implemented

---

# Decision 037 - RTC Fallback During NTP Unavailability

**Decision**

Use DS3231 RTC as the primary time source whenever NTP synchronization is unavailable.

**Reason**

Factory deployments cannot assume continuous internet connectivity.

**Benefits**

* Continuous timestamps
* Offline operation
* Reliable event ordering

**Status**

Implemented

---

# Decision 038 - Periodic NTP Re-Synchronization

**Decision**

Periodically re-synchronize RTC from NTP after initial synchronization.

**Reason**

RTC drift accumulates over long deployments.

**Current Configuration**

* Re-sync every 6 hours

**Benefits**

* Improved timestamp accuracy
* Long-term stability

**Status**

Implemented

---

# Decision 039 - Web-Based Diagnostics Dashboard

**Decision**

Host a lightweight diagnostics dashboard directly on the ESP32-S3.

**Reason**

Field diagnostics should not require serial monitor access.

**Features**

* System status
* Event logs
* Reset logs
* Log download functionality

**Status**

Implemented

---

# Decision 040 - Downloadable SD Card Logs

**Decision**

Allow browser-based download of diagnostic logs.

**Reason**

Simplifies field support and failure analysis.

**Files**

* event_log.txt
* reset_log.txt

**Benefits**

* Easier troubleshooting
* Faster issue reporting

**Status**

Implemented

---

# Decision 041 - WiFi Event-Driven Recovery

**Decision**

WiFi event-driven recovery with exponential backoff and controlled reconnection logic instead of aggressive reconnect loops.

**Reason**

Repeated connection attempts caused ESP32 WiFi stack instability.

**Benefits**

* Improved stability
* Lower CPU load
* Reduced connection failures

**Status**

Implemented

---

# Decision 042 - MQTT Recovery Independent of WiFi Recovery

**Decision**

Allow MQTT recovery only after WiFi connectivity has been fully restored.

**Reason**

MQTT connection attempts without valid network connectivity generated unnecessary TLS and DNS failures.

**Benefits**

* Cleaner recovery process
* Reduced broker connection spam
* Lower resource consumption

**Status**

Implemented

---

# Decision 043 - Telemetry Buffer Stores Only Meaningful Changes

**Decision**

Store telemetry in the offline buffer only when telemetry qualifies for publication under event-based rules.

**Reason**

Buffering every sample during outages can fill memory with duplicate information.

**Benefits**

* Efficient memory usage
* More valuable historical data
* Reduced replay duration

**Status**

Implemented

---


# Decision 044 - Sensor Disconnection Detection

**Decision**

Validate sensor connectivity before publishing telemetry.

**Reason**

Disconnected sensors should be reported as invalid rather than publishing misleading values.

**Implementation**

* DS18B20 disconnect detection
* MPU6050 availability detection
* ACS712 ADC validation

Invalid measurements are reported as `NaN`.

**Status**

Implemented

---

# Decision 045 - Secure Credential Management

**Decision**

Separate deployment-specific credentials from firmware source code by storing them in a local `secrets.h` file.

**Reason**

WiFi credentials and MQTT authentication information should never be committed to version control.

Separating secrets improves repository security while allowing each deployment to use its own credentials.

**Implementation**

* Created `secrets.h`
* Added `secrets.h` to `.gitignore`
* Added `secrets.h.example` as a template
* Updated README setup instructions

**Benefits**

* Prevents accidental credential leakage
* Simplifies deployment
* Supports multiple deployment environments
* Follows secure software development practices

**Status**

Implemented

---

# Decision 046 - Root CA Authentication

**Decision**

Authenticate the MQTT broker using its Root CA certificate.

**Reason**

Prevent connections to untrusted brokers.

**Benefits**

* Broker authentication
* Secure TLS communication
* Improved deployment security

**Status**

Implemented

---

# Decision 047 - Memory-Efficient Web Log Access

**Decision**

Use file streaming for web log downloads and viewing.

**Reason**

Avoid heap fragmentation caused by loading entire log files into `String` objects.

**Benefits**

* Improved reliability
* Supports larger log files

**Status**

Implemented

---

# Decision 048 - Watchdog Servicing During Long Calibration

**Decision**

Feed the ESP32 Task Watchdog during long-running calibration procedures.

**Reason**

ACS712 zero-current calibration requires approximately 20 seconds.

Without periodically servicing the watchdog, the ESP32 would reset before calibration completed.

**Implementation**

During calibration:

* Call `feedWatchdog()`
* Call `yield()`
* Continue calibration until the required sampling period has elapsed

**Benefits**

* Prevents watchdog resets
* Allows accurate calibration
* Improves startup reliability
* Enables long-duration initialization routines

**Status**

Implemented

---

# Key Lessons Learned

1. Stable power architecture is critical for accurate measurements.
2. Common grounding is mandatory when using multiple supplies.
3. Sensor calibration directly affects telemetry quality.
4. MQTT recovery mechanisms are essential.
5. Telemetry buffering prevents data loss.
6. Visual diagnostics simplify field testing.
7. Real hardware testing reveals issues that simulations cannot.
8. Hardware noise reduction and software filtering must work together.
9. Monitoring reliability must be established before machine control.
10. Documentation is part of engineering, not an afterthought.

---

## Current Project Status

### Completed

- ESP32-S3 Firmware
- WiFi Auto Recovery with Exponential Backoff
- MQTT over TLS
- QoS 1 Telemetry Publishing
- DS18B20 Temperature Monitoring
- MPU6050 Vibration Monitoring
- ACS712 Current Monitoring
- Sensor Calibration Framework
- RTC with NTP Synchronization
- RTC Offline Timestamp Fallback
- Event-Based Telemetry
- Offline FIFO Telemetry Buffer
- SD Card Logging
- Web Diagnostics Dashboard
- TLS Certificate Verification
- Sensor Fault Detection
- Watchdog Recovery
- Device Status Publishing

## Planned Enhancements

- MQTT Exponential Backoff
- OTA Firmware Updates
- Persistent Flash Telemetry Queue
- Remote Device Configuration
- Additional Industrial Sensors
- Enhanced Diagnostics Dashboard

---

# Final Architecture Summary

FactorySense follows an edge-first architecture where sensing, filtering, calibration, fault detection, timestamping, and temporary storage occur locally on the ESP32-S3. Cloud connectivity enhances monitoring but is not required for continued operation.

Core architectural principles include:

- Sensor-first startup
- Offline-first operation
- Event-driven telemetry
- Secure MQTT communication
- Automatic fault recovery
- Persistent local diagnostics
- Modular firmware architecture
- Industrial reliability over feature complexity

---

Last Updated: July 2026

Project: FactorySense Industrial Monitoring System
