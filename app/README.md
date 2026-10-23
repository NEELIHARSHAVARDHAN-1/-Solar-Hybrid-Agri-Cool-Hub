# Agri Cool Hub — Android App

Native Android application (Java/Kotlin) for the Solar Hybrid Agri Cool Hub IoT system.
Replaces the previous Flutter implementation.

## Tech Stack

| Concern | Library |
|---------|---------|
| UI | Material Components 3 + ViewBinding |
| Navigation | Jetpack Navigation Component |
| Networking | Retrofit 2 + OkHttp 4 + Gson |
| Charts | MPAndroidChart v3.1.0 |
| Splash animation | Lottie 6.x |
| Persistence | SharedPreferences (via `Prefs.java`) |
| Language | Java 8 (activities/fragments) + Kotlin interop |

## Jakob Nielsen's 10 Usability Principles — Compliance Map

| # | Principle | Implementation |
|---|-----------|----------------|
| 1 | Visibility of system status | ONLINE/OFFLINE chip, loading spinners, stale-data warning banner |
| 2 | Match between system and the real world | Plain-English sensor labels (e.g. "Temperature", "Air Quality") with units (°C, %, V) |
| 3 | User control and freedom | Resolve button for alerts; Logout in Settings; pull-to-refresh everywhere |
| 4 | Consistency and standards | Uniform card layout across all screens; standard Material icons |
| 5 | Error prevention | Input validation before login submit; empty-field guards |
| 6 | Recognition rather than recall | Persistent bottom nav with icon+label; always-visible field hints |
| 7 | Flexibility and efficiency of use | Pull-to-refresh; configurable backend URL; session persisted between launches |
| 8 | Aesthetic and minimalist design | Cards show only the current parameter value — no decoration noise |
| 9 | Help users recognize, diagnose, and recover from errors | Red error messages + Retry button on every data screen |
| 10 | Help and documentation | About section in Settings; inline helper text on URL field |

## Package Structure

```
com.agrihub.coolhub
├── data/
│   ├── model/          Java POJOs (TelemetryReading, DeviceStatus, Alert, …)
│   └── network/        Retrofit interface (AgriHubApi) + ApiClient singleton
├── ui/
│   ├── SplashActivity  Welcome screen with Lottie animation
│   ├── LoginActivity   Farmer ID + password login
│   ├── MainActivity    BottomNavigation shell
│   ├── DashboardFragment   Live sensor parameter cards
│   ├── HistoryFragment     24h MPAndroidChart + recent readings table
│   ├── AlertsFragment      Tabbed active/all alerts with resolve action
│   └── SettingsFragment    Backend URL config + logout
└── util/
    ├── Prefs.java      SharedPreferences wrapper
    └── DateUtils.java  ISO-8601 → display string helpers
```

## Demo Credentials

| Farmer ID | Password |
|-----------|----------|
| farmer001 | farm@2024 |
| farmer002 | agri#hub |
| admin     | admin123 |

> In production, replace the hardcoded credential check in `LoginActivity.java`
> with a real auth API call.

## Build Instructions

### Prerequisites
- Android Studio Hedgehog (2023.1.1) or newer
- Android SDK 34
- JDK 17

### Steps

```bash
# 1. Open the 'app/' folder as an Android Studio project
# 2. Let Gradle sync finish
# 3. Set your backend URL in SettingsFragment, or edit Prefs.DEFAULT_BACKEND

# Build debug APK
./gradlew assembleDebug

# Install on connected device / emulator
./gradlew installDebug
```

### Backend URL
- Emulator → local PC: `http://10.0.2.2:8000`
- Physical device → same network: `http://192.168.x.x:8000`
- Change at runtime: Settings → Backend URL → Save

## Splash Animation

The welcome screen uses a Lottie animation (`res/raw/splash_animation.json`) — pulsing
green concentric rings — paired with an `ObjectAnimator` fade-in + slide-up for the
app name and tagline. Replace `splash_animation.json` with any Lottie file from
[LottieFiles.com](https://lottiefiles.com) (search "leaf", "plant", "agriculture") for
a richer effect.
