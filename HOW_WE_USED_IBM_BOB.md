# How We Used IBM Bob to Build the Solar Hybrid Agri Cool Hub

> A complete walkthrough — from the first idea to working code —
> written so that anyone, including judges with no prior knowledge
> of the project, can understand exactly what IBM Bob did and why it mattered.

---

## Before We Start: What Is IBM Bob?

IBM Bob is an AI software-engineering assistant that lives inside the
developer's IDE (code editor). Unlike a search engine or a chatbot you
open in a browser, Bob can **read every file in the project**, understand
how they connect to each other, write new code, edit existing code, run
commands, spot bugs, and explain decisions — all without the developer
having to copy-paste anything.

Think of Bob as a senior engineer who is always available, has read every
line of code in the repository, and never loses context between sessions.

---

## The Problem We Were Trying to Solve

Farmers in India lose a significant portion of their harvest every year
not because of bad crops, but because of bad storage conditions during
transportation. A truckload of tomatoes or bananas can deteriorate in
hours if the temperature rises unexpectedly, humidity drops, or gases
released by the produce build up inside a closed container.

The core challenge is information: farmers cannot monitor what is happening
inside a sealed storage unit while it is on the road. By the time they
open it at the destination and discover the problem, it is too late.

Our goal was to build a system that:

1. Measures temperature, humidity, and gas levels continuously.
2. Sends that data to the cloud in real time.
3. Alerts the farmer immediately when something goes wrong.
4. Works even when the internet connection is temporarily lost.
5. Keeps the storage cool and humid using a practical hybrid power approach.

The system we built has four parts: a hardware sensor device (ESP32-S3),
a cloud server (Python/FastAPI), a web dashboard (React), and a mobile
app (Android). These four parts needed to work together perfectly.

This is where IBM Bob became essential.

---

## Understanding the "Solar Hybrid" Architecture

Before going into the software, it is important to understand what makes
this system a **Solar Hybrid** — because this shapes what the software
must monitor.

The system uses **two separate power sources** for two separate jobs:

```
COOLING          — Peltier module (thermoelectric cooler)
                   Powered by: mains / DC power supply
                   Keeps the interior cold reliably and continuously.

HUMIDIFICATION   — Humidifier module
                   Powered by: Solar panel
                   Uses free renewable energy for this lower-power task.
```

**Why this split matters:**
A Peltier cooling module draws significant current and needs stable voltage
to maintain a consistent cold temperature. Powering it entirely from solar
would require a large, expensive battery bank and would be unreliable on
cloudy days — unacceptable for food storage.

The humidifier, by contrast, is a low-power device that runs intermittently.
Solar power is perfectly adequate for it, and using solar here reduces
running costs and dependence on the grid.

This is what makes the system genuinely **hybrid**: it combines the
reliability of a conventional power supply for the critical cooling function
with renewable solar energy for the humidity management function.

**What this means for the monitoring software:**
The INA219 power monitor in the circuit measures the solar-powered
humidifier circuit — tracking whether the solar supply is delivering
adequate voltage and current. The SHT4x sensor monitors whether the
combined effect of Peltier cooling and humidifier operation is actually
keeping temperature and humidity within the correct range for the stored
produce. If either system fails — the Peltier loses power or the solar
panel cannot drive the humidifier — the sensor readings will immediately
reflect deteriorating conditions and trigger alerts through the software
pipeline.

---

## Phase 1 — Turning the Idea into a Plan

### The challenge at this stage
We had a concept but not a concrete plan. Four technology layers — embedded
C++ firmware, Python cloud server, TypeScript web app, and Java Android app
— need to agree on data formats, protocols, and API contracts before any
code is written. Getting this wrong means rewriting everything later.

### What Bob did
We described the project to Bob in plain English. Bob responded by producing
a complete implementation plan stored as `agri-cool-hub-plan.md` in the
repository. This plan included:

- The exact data flow from sensor to screen:
  ESP32 → HiveMQ Cloud (MQTT) → FastAPI → PostgreSQL → Android / React

- Every hardware pin assignment for the ESP32-S3 (which GPIO connects to
  which sensor, verified against the physical board schematic).

- The exact JSON format the firmware would publish, down to every field name
  and its data type:

```
{
  "device_id":   "agrihub-001",
  "timestamp":   "2026-10-22T11:00:00Z",
  "temperature": 25.6,
  "humidity":    72.4,
  "mq3":         10.0,
  "mq135":       30.0,
  "mq4":         28.0,
  "latitude":    12.9716,
  "longitude":   77.5946,
  "battery":     null
}
```

- The database table structure (three tables: devices, telemetry, alerts).

- All nine REST API endpoints the mobile app and web dashboard would call.

- The strategy for handling duplicate data (when the SD card replays stored
  readings after the internet reconnects, the database should silently ignore
  duplicates using a unique constraint on device_id + timestamp).

- The two-tier alert system (WARNING for early signs, CRITICAL for urgent
  action needed).

This plan meant every team member and every technology layer had the same
shared blueprint from day one. Nothing was left to assumption.

---

## Phase 2 — Building the Firmware (The Hardware Brain)

### The challenge at this stage
Firmware for the ESP32-S3 is written in C++. It must read six sensors,
manage Wi-Fi, connect to an encrypted MQTT broker, store data when offline,
parse GPS signals in real time without freezing, and recover automatically
from any failure — all without human intervention, running 24/7 in a
moving truck.

This is the most technically demanding part of the project. A single
mistake — like applying too much voltage to an ADC pin — can permanently
destroy the microcontroller.

### What Bob did

**Designed the module structure**
Bob organised the firmware into 18 separate modules, each handling exactly
one responsibility: SHT4xSensor for temperature/humidity, MQSensor for gas
readings, GPSSensor for location, mqtt_manager for cloud communication,
wifi_manager for connectivity, buffer_manager for offline storage, and so on.
The main sketch file (FactorySense.ino) calls these modules but contains no
hardware logic itself. This made the code readable, testable, and easy to
update one part without breaking another.

**Prevented a hardware-destroying voltage mistake**
MQ gas sensors are powered at 5 V and output up to 5 V on their signal pin.
The ESP32-S3's ADC input pins can only handle 3.3 V. Connecting them directly
would have permanently damaged the microcontroller.

Bob identified this risk when reading the pin definitions and specified the
exact voltage-divider circuit required (a 10 kΩ resistor and a 20 kΩ resistor
in series), calculated the resulting safe voltage, and documented it in the
wiring guide with the explicit warning: "Verify with a multimeter before
powering on." Bob also calculated the exact scaling formula used in the code:

  Sensor output % = (ADC reading / 4095 × 3.3 × 1.5 / 5.0) × 100

**Solved the "freezing GPS" problem**
GPS modules send continuous data streams over a serial port. An early version
of the firmware called a blocking GPS read function, which froze the entire
microcontroller for up to one second waiting for data. During that freeze,
the Wi-Fi stack could drop, the hardware watchdog could trigger a reset, and
MQTT publishes could fail.

Bob restructured the GPS reading into a non-blocking function (handleGPS())
that reads one byte at a time on each loop iteration. The main loop never
waits. GPS data accumulates gradually and is processed when a complete
sentence arrives — all without pausing anything else.

**Designed the three-tier timestamp system**
The telemetry data needs a timestamp on every reading. The most accurate
source is GPS time (UTC directly from satellites). But if the GPS has no fix
(common indoors or at startup), the firmware needs a fallback.

Bob designed a three-level priority system:
1. GPS time — used when the GPS module has a valid fix.
2. NTP time — used when the device is connected to Wi-Fi.
3. DS3231 hardware clock — used when offline (this chip keeps time from a
   small coin cell battery even when the main power is off).

Without this system, indoor deployments would have null timestamps on every
reading, which would cause the database to reject most of the data.

**Built the offline storage buffer**
When the internet goes down, telemetry must not be lost. Bob implemented a
circular buffer stored in the ESP32's PSRAM (8 MB of extra memory on the
chip). Up to 60 complete JSON payloads are held in memory. When the
connection restores, the buffer replays automatically. The backend silently
ignores any duplicates using the database constraint Bob designed in Phase 1.

---

## Phase 3 — Building the Cloud Backend (The Processing Engine)

### The challenge at this stage
The backend receives data from the hardware, stores it in a database,
evaluates whether any readings are dangerous, and serves data to the apps.
It must handle MQTT messages and HTTP API requests simultaneously, in an
asynchronous Python environment where one mistake causes silent data loss.

### What Bob did

**Fixed a critical silent data-loss bug**
When the backend received an MQTT message, the payload arrived as a Python
bytes object. An early version of the code converted it to a string using
Python's built-in str() function. This produces output like b'{"temp":25}'
instead of {"temp":25} — the b' prefix makes it invalid JSON. The JSON
parser rejected every single message silently. No error appeared; data
simply disappeared.

Bob spotted this immediately, identified the root cause, and applied the
correct fix: always use .decode() to convert bytes to a proper string before
parsing. This single fix is what made the entire IoT pipeline actually work.

**Built the two-tier alert engine**
Bob implemented the alert logic so that for each incoming sensor reading,
the system checks two sets of thresholds per sensor:

- WARNING: the value is elevated and the farmer should be aware.
- CRITICAL: the value is dangerous and immediate action is required.

If a WARNING alert is already open and the value worsens to CRITICAL, the
existing alert is upgraded rather than creating a duplicate. When the value
returns to normal, the alert is automatically closed. This gives farmers
a clean, accurate view of what is wrong right now.

**Prevented database connection exhaustion**
The web dashboard uses Server-Sent Events (SSE) — a technology where the
browser keeps a permanent open connection to the server to receive live
updates. An early version of the SSE code held a database connection open
during the two-second pause between checks. With 20 browser tabs open, that
is 20 permanently held connections, exhausting the database connection pool
and making the API unusable for everyone else.

Bob restructured the SSE code so the database connection is opened, used,
and closed before the two-second pause begins. During the pause, no
connection is held. This made the system scale correctly.

**Wrote automatic database migration**
After the system was already deployed and storing real data, new columns were
needed in the alerts table. Deleting and recreating the table would lose all
historical data. Writing a manual migration script risks mistakes.

Bob wrote self-checking migration code that runs automatically every time
the server starts. It checks whether each column already exists before trying
to add it. If it exists, nothing happens. If it is missing, it is added. The
data is never touched. This ran successfully on the live database with zero
downtime.

---

## Phase 4 — Building the Web Dashboard (The Operator View)

### The challenge at this stage
The web dashboard needed to show live sensor readings that update
automatically every 15 seconds without the operator refreshing the page.
Doing this correctly in React requires advanced patterns that are easy to
get wrong, resulting in missed updates or stale data on screen.

### What Bob did

**Built the live update system correctly**
Bob implemented the real-time dashboard using Server-Sent Events and React's
useSyncExternalStore hook — a modern React feature specifically designed for
subscribing to data that updates outside React's normal flow. An earlier
simpler approach caused "stale closure" bugs where the screen would stop
updating after a few minutes. Bob's implementation never has this problem.

Bob also ensured that if 10 dashboard cards are showing for 10 different
devices, there is still only one connection to the server per device — not
10 connections. All cards sharing a device automatically share the same
live data stream.

**Generated TypeScript type definitions from the backend schemas**
Bob read the Python backend's data schemas and generated matching TypeScript
interface definitions for the frontend. This means if the backend ever
changes what it returns, the TypeScript compiler immediately flags every
place in the frontend code that breaks — before the code is even run.

---

## Phase 5 — Building the Android App (The Farmer's Tool)

### The challenge at this stage
The Android app is what farmers actually use in the field. It must work on
inexpensive Android phones, load quickly, show clear information, and recover
gracefully from network errors.

### What Bob did

**Fixed crashes on navigation**
The app had a recurring crash when a farmer navigated from the Dashboard
back to a previous screen and then returned to the Dashboard again. The
crash was a NullPointerException — the screen was trying to update a UI
element that had already been destroyed.

Bob diagnosed this as an Android fragment lifecycle issue: the ViewBinding
reference was not being cleared when the screen was destroyed, so it held a
stale reference to a dead view. Bob applied the correct three-line fix across
all four screens simultaneously and the crashes stopped.

**Built the 24-hour history chart**
The history screen shows a line chart of temperature and humidity over the
past 24 hours. The backend returns timestamps in ISO-8601 format (a text
string like "2026-10-22T11:00:00Z"). The chart library expects numeric Unix
timestamps (a number like 1761138000). Bob wrote the conversion function and
the custom axis formatter that displays them as readable times (e.g., 11:00).

**Designed the Nielsen usability compliance**
Bob cross-checked every screen against Jakob Nielsen's 10 Usability
Heuristics — the industry standard for measuring whether an app is easy to
use — and identified missing states: loading spinners, empty-list messages,
error messages with Retry buttons, and a stale-data warning when the device
has not been heard from in over 5 minutes. Bob added all of these.

---

## Phase 6 — Testing and Documentation

Bob wrote a 24-test automated test suite for the backend that runs entirely
without requiring a real database or MQTT broker (using an in-memory
SQLite database instead). All 24 tests pass.

Bob also authored all eight project documentation files: the wiring guide,
API reference, deployment guide, configuration guide, troubleshooting guide,
firmware flash guide, and the four component documentation files — totalling
over 100 kilobytes of precise, codebase-grounded technical documentation.

---

## The Overall Picture — Bob's Role Across the Whole Project

The table below shows every major technical problem encountered and how
Bob resolved it:

Problem                               | Bob's Resolution
--------------------------------------|--------------------------------------------------
Hardware voltage risk (MQ sensors)    | Specified voltage divider; derived safety formula
GPS freezing the main loop            | Rewrote as non-blocking byte-by-byte feed
No timestamp when GPS unavailable     | Designed GPS → NTP → RTC → null priority chain
Data lost during connectivity outage  | PSRAM-backed 60-message circular buffer with replay
Silent JSON parse failure (MQTT)      | Identified str() vs .decode() bug; applied fix
Duplicate alert rows on escalation    | Fetch-then-update pattern; one row per sensor type
Database connection pool exhaustion   | Restructured SSE generator; release before sleep
Live schema migration without data loss| DO IF NOT EXISTS PL/pgSQL blocks at startup
SSE stale closure in React            | useSyncExternalStore pattern; global store map
Android crash on back-navigation      | ViewBinding nullified in onDestroyView
Chart axis showing raw timestamps     | ISO-8601 → epoch converter + custom axis formatter
Gradle dependency version conflicts   | Pinned compatible versions; added resolution strategy

---

## Why IBM Bob Mattered for This Project

This project required four completely different specialisations working in
complete harmony: embedded hardware programming, cloud backend development,
web frontend engineering, and mobile app development. Normally, this would
require a team of four specialists. IBM Bob made it possible for a small
student team to deliver all four layers as a single coherent system.

More importantly, Bob did not just write code — it reasoned about what could
go wrong before it went wrong. The voltage safety calculation, the timestamp
fallback chain, the connection pool fix, and the bytes decoding bug were all
caught and resolved proactively. Without Bob, these would have been
discovered the hard way: with a burned microcontroller, missing data in the
database, or a crashing live system.

The Solar Hybrid Agri Cool Hub works end-to-end, from sensor to screen,
because IBM Bob held the full picture of the system in mind at every step.

================================================================================
Document prepared for: IBM TechXchange — Project Submission Review
Project: Solar Hybrid Agri Cool Hub
================================================================================
