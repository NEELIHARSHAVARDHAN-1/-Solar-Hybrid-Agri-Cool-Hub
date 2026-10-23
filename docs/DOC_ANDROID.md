# Android App — Complete Documentation
## Solar Hybrid Agri Cool Hub

---

## Table of Contents

1. [Overview](#overview)
2. [Features](#features)
3. [How IBM Bob Was Used in Building](#how-ibm-bob-was-used-in-building)
4. [APIs Used](#apis-used)
5. [Protocols](#protocols)
6. [Challenges and How Bob Overcame Them](#challenges-and-how-bob-overcame-them)
7. [Feature Enhancements](#feature-enhancements)
8. [Conclusion](#conclusion)
9. [Setup Guide](#setup-guide)
10. [Run Procedures](#run-procedures)

---

## Overview

The **Agri Cool Hub Android App** is a native Android application written in Java (with Kotlin
interop) that provides farmers and agri-storage operators with a real-time monitoring interface
for IoT-connected cold-storage facilities powered by the Solar Hybrid Agri Cool Hub system.

The application connects to the FastAPI backend over HTTP REST, displays live sensor telemetry,
historical data visualizations, active alerts, and allows configuration of the backend endpoint.
It was designed to be used on any Android device running API 21+ (Android 5.0 Lollipop or higher),
ensuring compatibility with low-cost agricultural-sector devices.

---

## Features

### 1. Splash Screen with Lottie Animation
- A full-screen branded splash with a pulsing green Lottie animation (`splash_animation.json`).
- Accompanied by an `ObjectAnimator` fade-in and slide-up transition for the app name and tagline.
- Introduces the app brand before navigating to login.

### 2. Farmer Authentication (Login Screen)
- Farmer ID + password login with client-side input validation.
- Empty-field guard prevents submission with blank credentials.
- Demo credentials supported (`farmer001 / farm@2024`, `farmer002 / agri#hub`, `admin / admin123`).
- Session is persisted across launches via `SharedPreferences` — no re-login after close.
- Logout is available from the Settings screen.

### 3. Dashboard — Live Sensor Cards
- Shows real-time telemetry for every registered device.
- Per-device card shows:
  - **Online / Offline** status chip with green/red dot indicator.
  - **Temperature** (°C), **Humidity** (%), **Battery** (%).
  - **MQ-3** (Ethanol/Alcohol raw ADC), **MQ-135** (Air Quality raw ADC), **MQ-4** (Methane/LPG raw ADC).
  - **Last seen** timestamp.
  - **Stale data warning banner** when last seen > 5 minutes.
- Pull-to-refresh on every data screen.
- Loading spinner while fetching; error message + Retry button on failure.

### 4. History — 24-Hour Chart and Reading Table
- Line chart powered by **MPAndroidChart v3.1.0** showing temperature and humidity over the
  last 24 hours.
- A scrollable table of recent raw sensor readings below the chart.
- Time-axis formatted using `DateUtils.java`.
- Pull-to-refresh refreshes both the chart and the table.

### 5. Alerts — Tabbed Active and History View
- Two tabs: **Active** (unresolved) and **All** (complete history).
- Each alert card shows: alert type, sensor value, threshold that was exceeded,
  timestamp, and alert level (WARNING / CRITICAL).
- **Resolve** button on active alerts calls `PATCH /alerts/{id}/resolve` API.
- Badge count on the bottom navigation tab for unresolved alerts.

### 6. Settings — Backend URL and Account
- Editable **Backend URL** field (e.g., `http://10.0.2.2:8000` or `https://your-cloud-url`).
- Value persisted in `SharedPreferences` via `Prefs.java`.
- **Save** button with inline helper text validating URL format.
- **Logout** action clears session and returns to the Login screen.
- **About** section displaying app version and project credits.

### 7. Jakob Nielsen Usability Compliance
The app implements all 10 usability heuristics:

| # | Principle | Implementation |
|---|-----------|----------------|
| 1 | Visibility of system status | ONLINE/OFFLINE chip, loading spinners, stale-data warning banner |
| 2 | Match between system and the real world | Plain-English labels (e.g. "Temperature", "Air Quality") with units |
| 3 | User control and freedom | Resolve button for alerts; Logout in Settings; pull-to-refresh |
| 4 | Consistency and standards | Uniform card layout across all screens; standard Material icons |
| 5 | Error prevention | Input validation before login submit; empty-field guards |
| 6 | Recognition rather than recall | Persistent bottom nav with icon+label; always-visible field hints |
| 7 | Flexibility and efficiency of use | Pull-to-refresh; configurable backend URL; persisted session |
| 8 | Aesthetic and minimalist design | Cards show only current parameter value |
| 9 | Help users recover from errors | Red error messages + Retry button on every data screen |
| 10 | Help and documentation | About section in Settings; inline helper text on URL field |

---

## How IBM Bob Was Used in Building

IBM Bob (AI coding assistant) was integral to building the Android application end-to-end:

1. **Architecture Design** — Bob proposed the `data/`, `ui/`, `util/` package structure
   and the Retrofit + OkHttp + Gson networking stack appropriate for a Java/Android project.
   This kept the app modular and testable.

2. **Java Boilerplate Generation** — Bob generated all Java POJO model classes
   (`TelemetryReading.java`, `DeviceStatus.java`, `Alert.java`, `AlertResponse.java`) with
   `@SerializedName` annotations matching the backend JSON schema exactly.

3. **Retrofit API Interface** — Bob wrote the entire `AgriHubApi.java` Retrofit interface
   with correct return types (`Call<>` wrappers), matching all 9 backend endpoints including
   paginated responses.

4. **Fragment and Navigation Wiring** — Bob scaffolded `DashboardFragment`, `HistoryFragment`,
   `AlertsFragment`, and `SettingsFragment` with correct `ViewBinding` patterns, null-safety
   guards, and `onDestroyView` binding cleanup.

5. **MPAndroidChart Integration** — Bob handled the complex `LineDataSet` and `LineData`
   configuration for the 24-hour history chart, including axis formatting, legend, and the
   `DateUtils.java` helper for ISO-8601 → display string conversion.

6. **Lottie Animation Wiring** — Bob added the Lottie dependency, wrote the splash timing
   logic (`postDelayed` navigation), and provided the `ObjectAnimator` chain for the
   fade-in/slide-up combination.

7. **Gradle Configuration** — Bob resolved dependency version conflicts in `build.gradle`,
   pinned compatible versions of Retrofit, OkHttp, MPAndroidChart, and Lottie, and
   configured `compileOptions` for Java 8.

8. **Debugging Assistance** — When `ViewBinding` null-pointer exceptions occurred on fragment
   back-stack return, Bob identified the standard `onDestroyView` pattern and applied it
   consistently across all fragments.

9. **Usability Review** — Bob cross-checked the implementation against Nielsen's 10 heuristics
   and flagged missing states (empty list, loading, error with retry) that were then added.

---

## APIs Used

All communication is over **HTTP REST** to the FastAPI backend.

### Base URL
Configurable at runtime via Settings screen. Default for emulator: `http://10.0.2.2:8000`

### Endpoints Consumed

| Method | Endpoint | Purpose |
|--------|----------|---------|
| `GET` | `/devices` | List all registered devices with online status |
| `GET` | `/devices/{device_id}/status` | Latest telemetry reading + online flag |
| `GET` | `/devices/{device_id}/telemetry` | Paginated history (`?limit=&offset=&from=&to=`) |
| `GET` | `/devices/{device_id}/telemetry/latest` | Most recent single reading |
| `GET` | `/devices/{device_id}/alerts` | Paginated alert history |
| `GET` | `/devices/{device_id}/alerts/active` | Unresolved alerts only |
| `PATCH` | `/alerts/{alert_id}/resolve` | Resolve an alert |

### Retrofit Client (`ApiClient.java`)

```java
// Singleton with configurable base URL from SharedPreferences
Retrofit retrofit = new Retrofit.Builder()
    .baseUrl(Prefs.getBackendUrl(context))
    .addConverterFactory(GsonConverterFactory.create())
    .client(new OkHttpClient.Builder()
        .connectTimeout(10, TimeUnit.SECONDS)
        .readTimeout(15, TimeUnit.SECONDS)
        .build())
    .build();
```

### Third-Party Libraries

| Library | Version | Purpose |
|---------|---------|---------|
| Retrofit 2 | 2.9.0 | HTTP client abstraction |
| OkHttp 4 | 4.11.0 | Underlying HTTP transport |
| Gson | 2.10.1 | JSON ↔ Java object serialization |
| MPAndroidChart | 3.1.0 | History line charts |
| Lottie | 6.x | Splash animation |
| Jetpack Navigation | 2.7.x | Fragment navigation |
| Material Components 3 | 1.10.x | UI components |

---

## Protocols

### HTTP/1.1 REST (Primary)
- All API calls use standard HTTP/1.1 over TCP.
- JSON request/response bodies.
- Status codes: `200 OK`, `404 Not Found`, `400 Bad Request`.
- Timeout: 10 s connect / 15 s read (configurable in `ApiClient.java`).

### Data Binding (UI Layer)
- Android **ViewBinding** (not Data Binding) is used — type-safe view references
  without reflection overhead.
- Binding objects are inflated in `onCreateView` and nullified in `onDestroyView`
  to avoid memory leaks on back-stack returns.

### SharedPreferences (Local Persistence)
- Used via `Prefs.java` wrapper for:
  - Backend URL (`pref_backend_url`)
  - Login session token / farmer ID (`pref_logged_in`, `pref_farmer_id`)
- No encrypted storage (acceptable for non-sensitive config values).

---

## Challenges and How Bob Overcame Them

### Challenge 1 — ViewBinding Null Crashes on Fragment Back-Stack
**Problem:** `_binding` was accessed in `onResume()` after `onDestroyView()` had nullified it,
causing `NullPointerException` when the user navigated back to a fragment.

**Bob's Solution:** Applied the standard Android fragment binding pattern — `_binding` is
nullable, access only through a non-null `binding` property inside `onCreateView` /
`onViewCreated`, and always nullify in `onDestroyView`. Bob applied this pattern consistently
across all four fragments in a single pass.

### Challenge 2 — Retrofit `Call<Void>` for PATCH Resolve Endpoint
**Problem:** The PATCH `/alerts/{id}/resolve` endpoint returns the updated alert object,
but an incorrect `Call<Void>` declaration caused a Gson deserialization error.

**Bob's Solution:** Corrected the Retrofit interface to `Call<AlertItem>` and verified the
response schema against the backend `AlertOut` Pydantic model.

### Challenge 3 — MPAndroidChart Axis Formatting with ISO-8601 Timestamps
**Problem:** The history endpoint returns ISO-8601 timestamp strings; the chart X-axis
required numeric epoch values and a custom `ValueFormatter`.

**Bob's Solution:** Bob wrote `DateUtils.isoToEpoch()` converting timestamps to Unix epoch
milliseconds, and a `TimeAxisFormatter` extending `ValueFormatter` that formats epoch values
back to `HH:mm` for axis labels.

### Challenge 4 — Backend URL Injection at Runtime
**Problem:** Hardcoding `localhost:8000` breaks on physical devices; the URL must be
user-configurable per-deployment.

**Bob's Solution:** Bob introduced `Prefs.DEFAULT_BACKEND` and wired `ApiClient` to read from
SharedPreferences on every call, with Settings screen providing the input UI and
`Prefs.setBackendUrl()` persisting the change immediately.

### Challenge 5 — Gradle Dependency Conflicts
**Problem:** OkHttp 4.x pulled in a different version of Kotlin stdlib than Retrofit 2.9
expected, causing `DuplicateClassException` at build time.

**Bob's Solution:** Bob added `configurations.all { resolutionStrategy.force ... }` to pin
OkHttp and identified the exact exclusion needed for the conflicting `kotlin-stdlib-jdk8`
transitive dependency.

---

## Feature Enhancements

The following enhancements are planned or recommended for future development:

1. **Push Notifications (FCM)** — Integrate Firebase Cloud Messaging so alerts fire a system
   notification even when the app is backgrounded. The backend's SSE stream can be used as the
   trigger source.

2. **Real Authentication API** — Replace the hardcoded credential check in `LoginActivity.java`
   with a proper `POST /auth/login` call returning a JWT, with token storage in
   `EncryptedSharedPreferences`.

3. **Offline Mode / Room Database** — Cache the last known telemetry in a local Room database
   so the dashboard loads instantly on app open and shows a "cached" badge when the backend
   is unreachable.

4. **Multi-Device Support** — The backend already supports multiple `device_id` values; the
   app should show a device selector or a scrollable multi-card layout on the dashboard
   (partially implemented).

5. **Dark Mode** — Add a `night` resource qualifier and toggle in Settings using
   `AppCompatDelegate.setDefaultNightMode()`.

6. **Threshold Configuration** — Expose the backend's `PATCH /devices/{id}/thresholds` endpoint
   in the app so farmers can adjust alert thresholds without SSH-ing into the server.

7. **Export to CSV** — Allow the History screen to export the displayed readings as a CSV
   file shared via the Android `Sharesheet`.

8. **Biometric Login** — Add `BiometricPrompt` for quick re-authentication after the session
   has been established.

---

## Conclusion

The Agri Cool Hub Android App delivers a complete, production-quality monitoring interface for
agricultural cold-storage IoT deployments. Built with native Android (Java/Kotlin), it achieves
full compliance with Jakob Nielsen's 10 usability heuristics, maintains a clean MVVM-style
separation between networking and UI, and integrates with the FastAPI backend via a typed Retrofit
client.

IBM Bob was fundamental throughout the build: from initial architecture decisions and Java
boilerplate generation, through complex library integrations (MPAndroidChart, Lottie), to
debugging subtle Android lifecycle issues. The result is a maintainable, extensible codebase
that can be handed to any Android developer and immediately understood.

The app is ready for real-device deployment and APK distribution. With the enhancements outlined
above — push notifications, JWT authentication, and offline caching — it is also ready for
production agri-tech deployment.

---

## Setup Guide

### Prerequisites

| Requirement | Version |
|-------------|---------|
| Android Studio | Hedgehog (2023.1.1) or newer |
| Android SDK | 34 |
| JDK | 17 |
| Gradle | 8.x (managed by wrapper) |
| Android device / emulator | API 21+ |

### Steps

1. **Open the project in Android Studio**
   ```
   File → Open → select the  app/  folder (not the root)
   ```

2. **Let Gradle sync complete** — Android Studio will download all dependencies automatically.

3. **Configure the backend URL**
   - If using a local backend on the same machine as the emulator:
     `http://10.0.2.2:8000`
   - If using a physical device on the same Wi-Fi as the backend PC:
     `http://192.168.x.x:8000`
   - If using a cloud-deployed backend:
     `https://your-backend-host.railway.app`
   - The URL can also be set at runtime: open the app → Settings → Backend URL → Save.

4. **Ensure the backend is running** — See [Backend Documentation](DOC_BACKEND.md).

---

## Run Procedures

### Build Debug APK (Command Line)

```bash
cd app
./gradlew assembleDebug
# APK output: app/build/outputs/apk/debug/AgriCoolHub-debug.apk
```

### Install on Connected Device / Emulator

```bash
cd app
./gradlew installDebug
# Requires: adb in PATH, device connected with USB debugging enabled
```

### Build Release APK

```bash
cd app
./gradlew assembleRelease
# Requires signing config in build.gradle
```

### Run in Android Studio

1. Select a target device (emulator or physical) from the device dropdown.
2. Click **Run ▶** (Shift+F10).
3. The app launches on the selected device.

### Demo Login Credentials

| Farmer ID | Password |
|-----------|----------|
| `farmer001` | `farm@2024` |
| `farmer002` | `agri#hub` |
| `admin` | `admin123` |

> **Note:** These are hardcoded demo credentials for development. Replace with a real
> authentication API before production deployment.

### Troubleshooting

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| `Connection refused` on Dashboard | Backend not running or wrong URL | Start backend; check Settings URL |
| Dashboard shows "Device offline" | ESP32 not sending data | Check firmware is flashed and MQTT connected |
| Lottie animation not playing | Missing `splash_animation.json` | Ensure `res/raw/splash_animation.json` exists |
| Gradle sync fails | Missing Android SDK component | SDK Manager → install missing SDK |
| `NullPointerException` on fragment | ViewBinding lifecycle issue | Ensure binding is nullified in `onDestroyView` |
