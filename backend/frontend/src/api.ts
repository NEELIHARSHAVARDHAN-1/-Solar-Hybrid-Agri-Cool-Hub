import axios from "axios";

export const BASE_URL = import.meta.env.VITE_API_BASE_URL || "http://localhost:8000";

const api = axios.create({ baseURL: BASE_URL });

export interface DeviceOut {
  device_id: string;
  last_seen: string | null;
  is_online: boolean;
  created_at: string;
}

export interface TelemetryOut {
  id: number;
  device_id: string;
  timestamp: string;
  temperature: number | null;
  humidity: number | null;
  mq3: number;
  mq135: number;
  mq4: number;
  battery_percent: number | null;
  latitude: number | null;
  longitude: number | null;
  received_at: string;
}

export interface AlertOut {
  id: number;
  device_id: string;
  timestamp: string;
  alert_level: "warning" | "critical";
  alert_type: string;
  metric: string;
  value: number;
  threshold: number;
  resolved_at: string | null;
  created_at: string;
}

export interface PaginatedAlerts {
  total: number;
  items: AlertOut[];
}

export interface PaginatedTelemetry {
  total: number;
  items: TelemetryOut[];
}

export interface ThresholdOut {
  device_id: string;
  temp_warning: number;
  temp_critical: number;
  humidity_warning: number;
  humidity_critical: number;
  mq3_warning: number;
  mq3_critical: number;
  mq135_warning: number;
  mq135_critical: number;
  mq4_warning: number;
  mq4_critical: number;
  updated_at: string;
}

export interface SSEEvent {
  device_id: string;
  is_online: boolean;
  alert_level: "normal" | "warning" | "critical";
  active_count: number;
  telemetry: Omit<TelemetryOut, "id" | "device_id" | "received_at">;
  thresholds: Omit<ThresholdOut, "device_id" | "updated_at">;
}

// ---- API calls ----
export const fetchDevices = () =>
  api.get<DeviceOut[]>("/devices").then((r) => r.data);

export const fetchDeviceStatus = (id: string) =>
  api
    .get<{ device_id: string; is_online: boolean; last_seen: string | null; latest: TelemetryOut | null }>(
      `/devices/${id}/status`
    )
    .then((r) => r.data);

export const fetchTelemetry = (id: string, from?: string, to?: string, limit = 200) =>
  api
    .get<PaginatedTelemetry>(`/devices/${id}/telemetry`, { params: { from, to, limit } })
    .then((r) => r.data);

export const fetchAlerts = (id: string, resolved?: boolean, limit = 100) =>
  api
    .get<PaginatedAlerts>(`/devices/${id}/alerts`, { params: { resolved, limit } })
    .then((r) => r.data);

export const fetchAllAlerts = (resolved?: boolean, limit = 200) =>
  api
    .get<PaginatedAlerts>("/alerts", { params: { resolved, limit } })
    .then((r) => r.data);

export const resolveAlert = (alertId: number) =>
  api.patch<AlertOut>(`/alerts/${alertId}/resolve`).then((r) => r.data);

export const fetchThresholds = (id: string) =>
  api.get<ThresholdOut>(`/devices/${id}/thresholds`).then((r) => r.data);

export const updateThresholds = (
  id: string,
  body: Omit<ThresholdOut, "device_id" | "updated_at">
) =>
  api.put<ThresholdOut>(`/devices/${id}/thresholds`, body).then((r) => r.data);
