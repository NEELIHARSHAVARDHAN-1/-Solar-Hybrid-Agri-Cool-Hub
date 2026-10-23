import { useEffect, useState, useCallback } from "react";
import { fetchAllAlerts, fetchDevices, resolveAlert } from "../api";
import type { AlertOut, DeviceOut } from "../api";
import { AlertBadge } from "../components/AlertBadge";
import { format, parseISO } from "date-fns";
import { CheckCircle, AlertTriangle } from "lucide-react";

type Tab = "active" | "resolved" | "all";

function alertTypeLabel(t: string) {
  const map: Record<string, string> = {
    TEMP_HIGH:     "🌡️ High Temperature",
    HUMIDITY_HIGH: "💧 High Humidity",
    MQ3_HIGH:      "🧪 MQ3 Ethanol/Alcohol",
    MQ135_HIGH:    "☁️ MQ135 Air Quality",
    MQ4_HIGH:      "⚗️ MQ4 Methane/LPG",
  };
  return map[t] ?? t;
}

function AlertRow({ alert, onResolve }: { alert: AlertOut; onResolve: () => void }) {
  const [resolving, setResolving] = useState(false);

  const handleResolve = async () => {
    setResolving(true);
    try {
      await resolveAlert(alert.id);
      onResolve();
    } catch (e) {
      console.error(e);
    } finally {
      setResolving(false);
    }
  };

  return (
    <tr>
      <td>
        <span style={{ fontWeight: 600, fontSize: 13 }}>{alert.device_id}</span>
      </td>
      <td>
        <AlertBadge level={alert.alert_level as any} />
      </td>
      <td>{alertTypeLabel(alert.alert_type)}</td>
      <td style={{ fontFamily: "monospace" }}>
        {alert.value.toFixed(2)} <span style={{ color: "var(--muted)", fontSize: 11 }}>/ {alert.threshold.toFixed(2)}</span>
      </td>
      <td style={{ color: "var(--muted)", fontSize: 12 }}>
        {format(parseISO(alert.created_at), "yyyy-MM-dd HH:mm:ss")}
      </td>
      <td>
        {alert.resolved_at ? (
          <span style={{ color: "var(--green)", fontSize: 12, display: "flex", alignItems: "center", gap: 4 }}>
            <CheckCircle size={13} /> {format(parseISO(alert.resolved_at), "HH:mm:ss")}
          </span>
        ) : (
          <button
            className="btn btn-ghost"
            style={{ padding: "4px 10px", fontSize: 12 }}
            onClick={handleResolve}
            disabled={resolving}
          >
            {resolving ? "…" : "✓ Resolve"}
          </button>
        )}
      </td>
    </tr>
  );
}

export function Alerts() {
  const [tab,     setTab]     = useState<Tab>("active");
  const [alerts,  setAlerts]  = useState<AlertOut[]>([]);
  const [loading, setLoading] = useState(true);
  const [devices, setDevices] = useState<DeviceOut[]>([]);
  const [device,  setDevice]  = useState<string>("all");

  const load = useCallback(() => {
    const resolved = tab === "active" ? false : tab === "resolved" ? true : undefined;
    setLoading(true);
    fetchAllAlerts(resolved, 200)
      .then(r => setAlerts(r.items))
      .catch(console.error)
      .finally(() => setLoading(false));
  }, [tab]);

  useEffect(() => { load(); }, [load]);
  useEffect(() => {
    fetchDevices().then(setDevices).catch(() => {});
  }, []);
  // Auto-refresh active alerts
  useEffect(() => {
    if (tab !== "active") return;
    const id = setInterval(load, 8_000);
    return () => clearInterval(id);
  }, [tab, load]);

  const filtered = device === "all" ? alerts : alerts.filter(a => a.device_id === device);

  const critCount = filtered.filter(a => a.alert_level === "critical" && !a.resolved_at).length;
  const warnCount = filtered.filter(a => a.alert_level === "warning"  && !a.resolved_at).length;

  return (
    <div>
      <div className="page-header">
        <h1>🔔 Alerts</h1>
        <p>Monitor active and historical alert events across all devices</p>
      </div>

      {/* Summary badges */}
      <div style={{ display: "flex", gap: 12, marginBottom: 24, flexWrap: "wrap" }}>
        <div className="card" style={{ padding: "12px 20px", display: "flex", alignItems: "center", gap: 10 }}>
          <AlertTriangle size={18} color="#ef4444" />
          <div>
            <div className="stat-number" style={{ color: "#ef4444", fontSize: 22 }}>{critCount}</div>
            <div style={{ fontSize: 11, color: "var(--muted)" }}>Critical</div>
          </div>
        </div>
        <div className="card" style={{ padding: "12px 20px", display: "flex", alignItems: "center", gap: 10 }}>
          <AlertTriangle size={18} color="#eab308" />
          <div>
            <div className="stat-number" style={{ color: "#eab308", fontSize: 22 }}>{warnCount}</div>
            <div style={{ fontSize: 11, color: "var(--muted)" }}>Warning</div>
          </div>
        </div>
      </div>

      {/* Controls */}
      <div className="form-row" style={{ marginBottom: 20 }}>
        <div className="tab-bar" style={{ marginBottom: 0, width: 320 }}>
          {(["active", "resolved", "all"] as Tab[]).map(t => (
            <div key={t} className={`tab${tab === t ? " active" : ""}`} onClick={() => setTab(t)}>
              {t.charAt(0).toUpperCase() + t.slice(1)}
            </div>
          ))}
        </div>
        <div className="form-group">
          <select
            className="form-input"
            value={device}
            onChange={e => setDevice(e.target.value)}
            style={{ width: 200 }}
          >
            <option value="all">All Devices</option>
            {devices.map(d => <option key={d.device_id} value={d.device_id}>{d.device_id}</option>)}
          </select>
        </div>
        <button className="btn btn-ghost" onClick={load}>↻ Refresh</button>
      </div>

      {loading && <div style={{ color: "var(--muted)", marginBottom: 12 }}>Loading…</div>}

      <div className="card">
        {filtered.length === 0 && !loading ? (
          <div className="empty-state">
            <CheckCircle size={36} />
            <p style={{ marginTop: 10 }}>No alerts in this view.</p>
          </div>
        ) : (
          <div className="table-wrap">
            <table>
              <thead>
                <tr>
                  <th>Device</th>
                  <th>Level</th>
                  <th>Metric</th>
                  <th>Value / Threshold</th>
                  <th>Created</th>
                  <th>Status</th>
                </tr>
              </thead>
              <tbody>
                {filtered.map(a => (
                  <AlertRow key={a.id} alert={a} onResolve={load} />
                ))}
              </tbody>
            </table>
          </div>
        )}
      </div>
    </div>
  );
}
