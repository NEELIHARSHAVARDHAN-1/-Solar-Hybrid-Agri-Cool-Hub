# Frontend (Web Dashboard) — Complete Documentation
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

The **Agri Cool Hub Frontend** is a single-page web application built with **React 19**,
**TypeScript**, and **Vite**, providing a desktop/browser-based monitoring dashboard for
the Solar Hybrid Agri Cool Hub IoT system.

Unlike the Android app (which targets individual farmers in the field), the frontend is
designed for **operators and administrators** who monitor the cold-storage fleet from a
workstation. It provides real-time updates via **Server-Sent Events (SSE)**, interactive
charts, alert management, and live threshold configuration — all without requiring a
page refresh.

The frontend communicates entirely with the FastAPI backend over HTTP REST and SSE. It
requires no separate server-side rendering and can be deployed as a static bundle on any
CDN or web host.

---

## Features

### 1. Dashboard — Real-Time Device Cards with SSE

- A **summary KPI strip** at the top shows:
  - Total Devices, Online count, Offline count, Active Alerts.
  - Active Alerts count updates every 10 seconds via polling.
- Each registered device gets its own **device card**, fed in real time via a
  per-device SSE stream (`/devices/{id}/stream`).
- Each card shows:
  - **Online/Offline dot** indicator with `last seen HH:mm:ss` timestamp.
  - **Alert badge** (`NORMAL`, `WARNING`, `CRITICAL`) with color-coded border.
  - **Radial gauge** for each of the 5 sensor parameters:
    Temperature, Humidity, MQ3 (Ethanol), MQ135 (Air Quality), MQ4 (Methane).
  - Each gauge shows warning (amber) and critical (red) threshold zones visually.
  - **Active alert count** banner below the header when alerts are active.
  - **Battery %** and **GPS coordinates** in the card footer.
  - Critical cards **pulse red** with a CSS animation to draw immediate attention.
- No page refresh needed — SSE pushes updates as new telemetry arrives.

### 2. History — Interactive Recharts Line Chart

- Selectable **device** and **time range** (1h, 6h, 12h, 24h, 7d).
- Multi-series line chart (temperature, humidity, and MQ sensors) rendered by
  **Recharts v3**.
- Interactive tooltip showing exact values on hover.
- Responsive chart that scales to window width.
- A scrollable raw-data table below the chart for precise values.

### 3. Alerts — Unified Alert Management

- **Toggle** between resolved and active alerts.
- Each alert shows: metric name, value vs threshold, alert level badge, device ID,
  and timestamp.
- **Resolve** button calls `PATCH /alerts/{id}/resolve` and immediately removes the
  item from the active list.
- Alert level color coding: `warning` → amber, `critical` → red.

### 4. Settings — Live Threshold Configuration

- Per-device threshold editor for all 5 sensor parameters × 2 tiers (warning / critical).
- Values are fetched via `GET /devices/{id}/thresholds` and submitted via
  `PUT /devices/{id}/thresholds`.
- Changes take effect immediately on the backend — the next telemetry reading will be
  evaluated against the new thresholds.

### 5. Sidebar Navigation

- Fixed left sidebar with icon + label links to Dashboard, History, Alerts, Settings.
- Active route is highlighted.
- Responsive layout: `main-content` fills remaining space.

### 6. Radial Gauge Component

- Custom SVG-based radial gauge (`RadialGauge.tsx`) — no external gauge library needed.
- Renders an arc showing value position relative to min/max.
- Arc colour transitions: green → amber → red as value crosses warning/critical thresholds.
- Displays numeric value and unit label below the arc.

### 7. Alert Badge Component

- `AlertBadge.tsx` renders a compact pill badge: `NORMAL` (green), `WARNING` (amber),
  `CRITICAL` (red).
- Used consistently in device cards and alert lists.

---

## How IBM Bob Was Used in Building

1. **Project Scaffolding** — Bob initialised the Vite + React + TypeScript project with
   the correct `tsconfig.app.json` configuration, ESM module settings in `package.json`,
   and the `.env.local` pattern for `VITE_API_BASE_URL`.

2. **TypeScript Type Definitions** — Bob wrote the entire `api.ts` module with fully
   typed interfaces (`DeviceOut`, `TelemetryOut`, `AlertOut`, `ThresholdOut`, `SSEEvent`)
   that exactly mirror the Pydantic response schemas from the FastAPI backend, ensuring
   compile-time safety across the frontend.

3. **SSE Hook (`useSSE.ts`)** — Bob designed and implemented the `useSSE` custom hook
   using React 19's `useSyncExternalStore` — a pattern that allows SSE data to flow into
   React's concurrent rendering model without stale closure issues. Bob also implemented
   the external store map (`stores` dictionary) with automatic `EventSource` creation and
   cleanup when the last subscriber unmounts.

4. **Radial Gauge Component** — Bob designed the SVG arc math for `RadialGauge.tsx`:
   calculating `strokeDasharray` / `strokeDashoffset` for the arc fill, and the
   threshold colour interpolation logic.

5. **Dashboard Layout** — Bob wrote the `DeviceCard` and `SummaryStrip` components,
   wiring SSE data into the card's visual state (alert level → border colour → pulse
   animation) and handling the loading/offline/no-data states.

6. **Axios API Module** — Bob generated the complete `api.ts` with typed `axios`
   wrappers for all 9 backend endpoints, including optional query parameters (`from`,
   `to`, `resolved`, `limit`).

7. **Recharts Integration** — Bob configured the `LineChart` component for the History
   page with correct `dataKey` mappings, a custom tooltip formatter using `date-fns`
   for timestamp display, and a responsive wrapper.

8. **Oxlint Configuration** — Bob set up `.oxlintrc.json` with React and TypeScript
   plugins, enabling `react/rules-of-hooks` enforcement and `only-export-components`
   warning as part of the lint pipeline.

9. **Vite Config** — Bob configured `vite.config.ts` with the `@vitejs/plugin-react`
   plugin and the `server.proxy` setting to forward `/api` calls to `localhost:8000`
   during development, eliminating CORS friction.

10. **CSS Design System** — Bob wrote `index.css` with a CSS custom-property design
    token system (`--blue`, `--green`, `--red`, `--muted`, `--border`, etc.), the
    `.card`, `.grid-4`, `.grid-5`, `.pulse-red` keyframe animation, and the shell
    layout grid.

---

## APIs Used

### Base URL Configuration

Set in `.env.local`:
```
VITE_API_BASE_URL=http://localhost:8000
```
Falls back to `http://localhost:8000` if the variable is not set.

### REST Endpoints Consumed

| Method | Endpoint | TypeScript Function | Description |
|--------|----------|--------------------|-----------  |
| `GET` | `/devices` | `fetchDevices()` | List all devices |
| `GET` | `/devices/{id}/status` | `fetchDeviceStatus(id)` | Latest reading + online flag |
| `GET` | `/devices/{id}/telemetry` | `fetchTelemetry(id, from, to, limit)` | Paginated history |
| `GET` | `/devices/{id}/alerts` | `fetchAlerts(id, resolved, limit)` | Device alerts |
| `GET` | `/alerts` | `fetchAllAlerts(resolved, limit)` | Cross-device alert list |
| `PATCH` | `/alerts/{id}/resolve` | `resolveAlert(id)` | Resolve single alert |
| `GET` | `/devices/{id}/thresholds` | `fetchThresholds(id)` | Get threshold config |
| `PUT` | `/devices/{id}/thresholds` | `updateThresholds(id, body)` | Save threshold config |

### SSE Endpoint

| Endpoint | Hook | Description |
|----------|------|-------------|
| `GET /devices/{id}/stream` | `useSSE(deviceId)` | Real-time device push stream |

The SSE stream emits `SSEEvent` JSON objects on every new telemetry ingestion:

```typescript
interface SSEEvent {
  device_id:    string;
  is_online:    boolean;
  alert_level:  "normal" | "warning" | "critical";
  active_count: number;
  telemetry:    { timestamp, temperature, humidity, mq3, mq135, mq4, battery_percent, ... };
  thresholds:   { temp_warning, temp_critical, humidity_warning, ... };
}
```

### Axios Instance

```typescript
const api = axios.create({ baseURL: BASE_URL });
```
All functions use this single Axios instance. No request interceptors or auth headers
are required (the backend does not enforce authentication in v1).

### Third-Party Libraries

| Library | Version | Purpose |
|---------|---------|---------|
| React | 19.3.x | UI framework |
| React DOM | 19.3.x | DOM rendering |
| React Router DOM | 7.x | Client-side routing |
| Axios | 1.20.x | HTTP client |
| Recharts | 3.10.x | History line charts |
| Lucide React | 1.61.x | Icon set (Battery, MapPin, AlertTriangle, etc.) |
| date-fns | 4.4.x | Timestamp formatting |
| TypeScript | 6.0.x | Static typing |
| Vite | 8.x | Build tool and dev server |
| Oxlint | 1.88.x | Fast linter (Rust-based) |

---

## Protocols

### HTTP/1.1 REST (Data Fetching)
- Standard JSON over HTTP used for all state-changing operations and paginated history.
- CORS is enabled on the backend (`allow_origins=["*"]`).
- No authentication tokens in v1.

### Server-Sent Events (SSE) — Real-Time Push
- The browser opens a persistent HTTP connection to `/devices/{id}/stream`.
- The backend streams `data: {...}\n\n` events on every new telemetry ingestion (every
  ~30 seconds from the ESP32).
- The browser's native `EventSource` API handles automatic reconnection on disconnect.
- The `useSSE.ts` hook wraps `EventSource` in a `useSyncExternalStore` for React
  concurrent-mode compatibility.
- **One `EventSource` per device** is created globally (shared across all mounted
  components that call `useSSE` with the same device ID), and is closed when the last
  subscriber unmounts.

**SSE vs WebSocket choice:** SSE was chosen because:
- The data flow is unidirectional (server → client only).
- Native browser reconnection is built in.
- No additional WebSocket upgrade handshake required.
- Works through standard HTTP/2 proxies and CDNs.

### VITE Dev Proxy
During development, Vite proxies `/api/*` to `http://localhost:8000` to avoid CORS issues
with the React dev server running on port 5173.

---

## Challenges and How Bob Overcame Them

### Challenge 1 — SSE Stale Closures in React Concurrent Mode
**Problem:** Naively using `useState` inside `useEffect` with `EventSource` caused stale
closure bugs where the component would not re-render when new SSE data arrived, because
the `EventSource` callback captured a stale version of the state setter.

**Bob's Solution:** Bob rewrote the SSE subscription using React 19's
`useSyncExternalStore`, which is specifically designed for subscribing to external
mutable stores. The `stores` dictionary holds data and listeners outside React's render
cycle, eliminating all stale closure issues.

### Challenge 2 — Multiple Components Subscribing to the Same Device Stream
**Problem:** If `DeviceCard` was mounted multiple times (e.g., in a grid), each instance
opened its own `EventSource` connection, causing redundant network connections to the
backend.

**Bob's Solution:** Bob introduced a global `stores` map keyed by `deviceId`. The first
subscriber for a given device ID creates the `EventSource`; subsequent subscribers join
the same store's listener set. The connection is only closed when the last listener
unsubscribes.

### Challenge 3 — TypeScript Type Safety Across the API Boundary
**Problem:** The FastAPI backend returns snake_case JSON; TypeScript interfaces needed
to match exactly, and optional fields (`battery_percent`, `latitude`, `longitude`) are
nullable. Without strict typing, runtime errors would appear only in production.

**Bob's Solution:** Bob defined the complete TypeScript interface set in `api.ts` with
`number | null` for all nullable fields, `string | null` for nullable timestamps, and
discriminated union types for `alert_level: "normal" | "warning" | "critical"`. This
gave compile-time errors for any mismatched API calls.

### Challenge 4 — Radial Gauge SVG Arc Mathematics
**Problem:** Rendering a value arc on an SVG circle requires converting a value
percentage to `strokeDasharray` / `strokeDashoffset` values — non-trivial math involving
the circumference of the arc path.

**Bob's Solution:** Bob implemented the formula:
```
circumference = 2 * π * radius * (arcAngle / 360)
offset = circumference * (1 - normalizedValue)
```
where `normalizedValue = (value - min) / (max - min)`, clamped to [0, 1]. The arc
colour is determined by comparing `value` to `warning` and `critical` thresholds.

### Challenge 5 — Vite Environment Variable Configuration
**Problem:** Hardcoding `localhost:8000` meant the built bundle could not be deployed
against a cloud backend without a code change.

**Bob's Solution:** Bob introduced `VITE_API_BASE_URL` in `.env.local` and used
`import.meta.env.VITE_API_BASE_URL` in `api.ts` with a fallback. The same pattern works
for deployment: set the environment variable in the CI/CD environment at build time.

### Challenge 6 — CSS Layout for Responsive Card Grid
**Problem:** The card grid needed to show 1 column on narrow screens and 2+ on wide
screens, with consistent card sizing.

**Bob's Solution:** Bob used `grid-template-columns: repeat(auto-fill, minmax(520px, 1fr))`
which auto-sizes columns based on available space, giving 1 column on small viewports and
2+ on wide desktops without any media queries.

---

## Feature Enhancements

1. **Authentication / Multi-Tenant** — Add a login page and JWT-based auth so different
   farm operators see only their devices. The backend currently has no auth; this would
   require a `POST /auth/login` endpoint and `Authorization: Bearer` header injection
   in the Axios instance.

2. **WebSocket Upgrade** — For sub-second updates (e.g., if sensor interval is reduced),
   replace SSE with WebSocket for lower-latency bidirectional communication.

3. **PWA / Offline Support** — Add a Vite PWA plugin (`vite-plugin-pwa`) so the
   dashboard caches the last known state and is installable as a home screen web app on
   tablets.

4. **Alert Sound / Browser Notification** — When a CRITICAL alert arrives via SSE, play
   an audio alert and/or trigger a `Notification` API push (requires user permission).

5. **Device Map View** — Add a Leaflet.js map page showing device locations as markers
   (using the `latitude`/`longitude` fields in telemetry), with colour-coded pins for
   alert status.

6. **Dark Mode** — Add a CSS class toggle on `<html>` and a settings toggle, redefining
   the CSS custom properties for dark backgrounds.

7. **Export to CSV** — Add a "Download CSV" button on the History page that converts the
   fetched telemetry array to a CSV string and triggers a browser download.

8. **React Query / TanStack Query** — Replace manual `useEffect` polling with TanStack
   Query for smarter caching, background refetching, and loading/error state management.

---

## Conclusion

The Agri Cool Hub Frontend delivers a professional-grade real-time monitoring dashboard
built on modern React 19 + TypeScript + Vite tooling. Its defining feature is the SSE
integration via a custom `useSSE` hook that provides live device telemetry updates to
all dashboard cards without polling, using a minimal connection footprint.

IBM Bob was central to the entire frontend build: from the SSE architecture design, the
TypeScript type system, the SVG gauge component, to the Vite project configuration. The
result is a fully typed, lint-clean, and deployable web dashboard that serves as the
operator-facing companion to the Android app.

The frontend is ready for static deployment (CDN, Netlify, Vercel, or GitHub Pages) and
can be configured for any backend URL at build time via environment variables.

---

## Setup Guide

### Prerequisites

| Requirement | Minimum Version |
|-------------|----------------|
| Node.js | 18.x LTS |
| npm | 9.x |
| A running Agri Cool Hub backend | See `DOC_BACKEND.md` |

### Steps

1. **Navigate to the frontend directory**
   ```bash
   cd frontend
   ```

2. **Install dependencies**
   ```bash
   npm install
   ```

3. **Configure the backend URL**

   Create or edit `frontend/.env.local`:
   ```
   VITE_API_BASE_URL=http://localhost:8000
   ```
   For a cloud backend:
   ```
   VITE_API_BASE_URL=https://your-backend.railway.app
   ```

4. **Ensure the backend is running** — See [Backend Documentation](DOC_BACKEND.md).

---

## Run Procedures

### Start the Development Server

```bash
cd frontend
npm run dev
# Development server starts at http://localhost:5173
# Hot Module Replacement (HMR) is enabled
```

### Build for Production

```bash
cd frontend
npm run build
# Output: frontend/dist/
# Static files can be served by any web server or CDN
```

### Preview the Production Build Locally

```bash
cd frontend
npm run preview
# Serves the built dist/ at http://localhost:4173
```

### Lint the Codebase

```bash
cd frontend
npm run lint
# Uses Oxlint (fast Rust-based linter)
```

### Deploy to a Static Host

**Netlify:**
```bash
# Build command: npm run build
# Publish directory: dist
# Environment variable: VITE_API_BASE_URL = https://your-backend-url
```

**Vercel:**
```bash
# Framework preset: Vite
# Build command: npm run build
# Output directory: dist
# Add VITE_API_BASE_URL in Environment Variables
```

**Serve locally with nginx:**
```bash
npm run build
# Copy dist/ to your nginx root
# Ensure nginx forwards /devices/* to the backend if co-hosting
```

### Troubleshooting

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| Dashboard shows "Loading devices…" forever | Backend not running or wrong URL | Check `VITE_API_BASE_URL` in `.env.local` |
| CORS errors in browser console | Backend URL mismatch | Ensure `VITE_API_BASE_URL` matches the running backend |
| SSE not updating cards | `EventSource` blocked by proxy | Use direct backend URL; avoid /api prefix confusion |
| Charts not rendering | `recharts` not installed | Run `npm install` |
| `tsc` errors on build | TypeScript version mismatch | Use Node 18+; run `npm install` to get correct `typescript@6` |
