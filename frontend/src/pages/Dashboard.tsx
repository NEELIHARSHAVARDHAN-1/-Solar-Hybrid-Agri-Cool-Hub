import { useEffect, useState } from "react";
import { fetchDevices, fetchAllAlerts } from "../api";
import type { DeviceOut } from "../api";
import { useSSE } from "../useSSE";
import { RadialGauge } from "../components/RadialGauge";
import { AlertBadge } from "../components/AlertBadge";
import { Battery, MapPin, AlertTriangle, CheckCircle } from "lucide-react";
import { format, parseISO } from "date-fns";

// Per-device card fed by SSE
function DeviceCard({ device }: { device: DeviceOut }) {
  const live = useSSE(device.device_id);

  const thr  = live?.thresholds;
  const tel  = live?.telemetry;
  const alertLevel = live ? live.alert_level : (device.is_online ? "normal" : "offline") as any;
  const isOnline   = live ? live.is_online   : device.is_online;

  const cardClass = `card device-card-${alertLevel === "offline" ? "normal" : alertLevel}${alertLevel === "critical" ? " pulse-red" : ""}`;

  return (
    <div className={cardClass} style={{ borderWidth: 2 }}>
      {/* Header */}
      <div className="card-header" style={{ marginBottom: 12 }}>
        <div>
          <div style={{ display: "flex", alignItems: "center", gap: 8, marginBottom: 4 }}>
            <span className={`online-dot ${isOnline ? "online" : "offline"}`} />
            <span style={{ fontWeight: 700, fontSize: 15 }}>{device.device_id}</span>
          </div>
          <div style={{ fontSize: 11, color: "var(--muted)" }}>
            {device.last_seen ? `Last seen ${format(parseISO(device.last_seen), "HH:mm:ss")}` : "Never seen"}
          </div>
        </div>
        <AlertBadge level={alertLevel} />
      </div>

      {/* Active alert count */}
      {live && live.active_count > 0 && (
        <div style={{ display: "flex", alignItems: "center", gap: 6, marginBottom: 14,
                      background: alertLevel === "critical" ? "rgba(239,68,68,0.1)" : "rgba(234,179,8,0.1)",
                      borderRadius: 8, padding: "6px 10px" }}>
          <AlertTriangle size={14} color={alertLevel === "critical" ? "#ef4444" : "#eab308"} />
          <span style={{ fontSize: 12, color: alertLevel === "critical" ? "#ef4444" : "#eab308", fontWeight: 600 }}>
            {live.active_count} active alert{live.active_count > 1 ? "s" : ""}
          </span>
        </div>
      )}

      {/* Gauges */}
      {tel && thr ? (
        <div className="grid-5" style={{ marginBottom: 12 }}>
          <RadialGauge
            value={tel.temperature ?? 0}
            max={50} min={-10}
            warning={thr.temp_warning}
            critical={thr.temp_critical}
            label="Temperature" unit="°C"
            size={100}
          />
          <RadialGauge
            value={tel.humidity ?? 0}
            max={100}
            warning={thr.humidity_warning}
            critical={thr.humidity_critical}
            label="Humidity" unit="%"
            size={100}
          />
          <RadialGauge
            value={tel.mq3}
            max={100}
            warning={thr.mq3_warning}
            critical={thr.mq3_critical}
            label="MQ3 Ethanol" unit="%"
            size={100}
          />
          <RadialGauge
            value={tel.mq135}
            max={100}
            warning={thr.mq135_warning}
            critical={thr.mq135_critical}
            label="MQ135 Air" unit="%"
            size={100}
          />
          <RadialGauge
            value={tel.mq4}
            max={100}
            warning={thr.mq4_warning}
            critical={thr.mq4_critical}
            label="MQ4 Methane" unit="%"
            size={100}
          />
        </div>
      ) : (
        <div style={{ textAlign: "center", padding: "24px 0", color: "var(--muted)", fontSize: 13 }}>
          {isOnline ? "Waiting for telemetry…" : "Device offline — no live data"}
        </div>
      )}

      {/* Footer: battery + location */}
      {tel && (
        <div style={{ display: "flex", justifyContent: "space-between", paddingTop: 10,
                      borderTop: "1px solid var(--border)", fontSize: 12, color: "var(--muted)" }}>
          <div style={{ display: "flex", alignItems: "center", gap: 4 }}>
            <Battery size={13} />
            <span>{tel.battery_percent ?? "--"}%</span>
          </div>
          {tel.latitude != null && (
            <div style={{ display: "flex", alignItems: "center", gap: 4 }}>
              <MapPin size={13} />
              <span>{tel.latitude.toFixed(4)}, {tel.longitude?.toFixed(4)}</span>
            </div>
          )}
          <div style={{ color: "var(--muted)" }}>
            {format(parseISO(tel.timestamp), "HH:mm:ss")}
          </div>
        </div>
      )}
    </div>
  );
}

// Summary KPI strip at top
function SummaryStrip({ devices }: { devices: DeviceOut[] }) {
  const [activeAlerts, setActiveAlerts] = useState(0);
  const onlineCount   = devices.filter(d => d.is_online).length;

  useEffect(() => {
    fetchAllAlerts(false, 500).then(r => setActiveAlerts(r.total)).catch(() => {});
    const id = setInterval(() => {
      fetchAllAlerts(false, 500).then(r => setActiveAlerts(r.total)).catch(() => {});
    }, 10_000);
    return () => clearInterval(id);
  }, []);

  const items = [
    { label: "Total Devices",   value: devices.length,   color: "var(--blue)" },
    { label: "Online",          value: onlineCount,      color: "var(--green)" },
    { label: "Offline",         value: devices.length - onlineCount, color: "var(--muted)" },
    { label: "Active Alerts",   value: activeAlerts,     color: activeAlerts > 0 ? "var(--red)" : "var(--green)" },
  ];

  return (
    <div className="grid-4" style={{ marginBottom: 28 }}>
      {items.map(({ label, value, color }) => (
        <div key={label} className="card" style={{ textAlign: "center", padding: "16px 20px" }}>
          <div className="stat-number" style={{ color }}>{value}</div>
          <div style={{ fontSize: 12, color: "var(--muted)", marginTop: 2 }}>{label}</div>
        </div>
      ))}
    </div>
  );
}

export function Dashboard() {
  const [devices, setDevices] = useState<DeviceOut[]>([]);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    fetchDevices()
      .then(setDevices)
      .catch(console.error)
      .finally(() => setLoading(false));
    const id = setInterval(() => {
      fetchDevices().then(setDevices).catch(() => {});
    }, 30_000);
    return () => clearInterval(id);
  }, []);

  if (loading) return <div style={{ padding: 40, color: "var(--muted)" }}>Loading devices…</div>;

  return (
    <div>
      <div className="page-header">
        <h1>🌿 Dashboard</h1>
        <p>Real-time monitoring of all AgriHub cool storage devices</p>
      </div>

      <SummaryStrip devices={devices} />

      {devices.length === 0 ? (
        <div className="card empty-state">
          <CheckCircle size={40} />
          <p>No devices registered yet.</p>
          <p style={{ fontSize: 12, marginTop: 6 }}>Connect a device via MQTT to get started.</p>
        </div>
      ) : (
        <div style={{ display: "grid", gridTemplateColumns: "repeat(auto-fill, minmax(520px, 1fr))", gap: 20 }}>
          {devices.map(d => <DeviceCard key={d.device_id} device={d} />)}
        </div>
      )}
    </div>
  );
}
