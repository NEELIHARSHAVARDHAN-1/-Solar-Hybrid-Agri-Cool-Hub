import { useEffect, useState, useCallback } from "react";
import { fetchDevices, fetchThresholds, updateThresholds } from "../api";
import type { DeviceOut, ThresholdOut } from "../api";
import { Save, Info } from "lucide-react";

type ThresholdForm = Omit<ThresholdOut, "device_id" | "updated_at">;

const GROUPED: { metric: string; warn: keyof ThresholdForm; crit: keyof ThresholdForm; unit: string; min: number; max: number; step: number }[] = [
  { metric: "Temperature",   warn: "temp_warning",     crit: "temp_critical",     unit: "°C",  min: -10, max: 85,  step: 0.5 },
  { metric: "Humidity",      warn: "humidity_warning", crit: "humidity_critical", unit: "%",   min: 0,   max: 100, step: 1 },
  { metric: "MQ3 Ethanol",   warn: "mq3_warning",      crit: "mq3_critical",      unit: "%",   min: 0,   max: 100, step: 1 },
  { metric: "MQ135 Air",     warn: "mq135_warning",    crit: "mq135_critical",    unit: "%",   min: 0,   max: 100, step: 1 },
  { metric: "MQ4 Methane",   warn: "mq4_warning",      crit: "mq4_critical",      unit: "%",   min: 0,   max: 100, step: 1 },
];

function ThresholdPanel({ device }: { device: string }) {
  const [form,    setForm]    = useState<ThresholdForm | null>(null);
  const [loading, setLoading] = useState(true);
  const [saving,  setSaving]  = useState(false);
  const [saved,   setSaved]   = useState(false);

  const load = useCallback(() => {
    setLoading(true);
    fetchThresholds(device)
      .then(t => {
        const { device_id, updated_at, ...rest } = t;
        setForm(rest);
      })
      .catch(console.error)
      .finally(() => setLoading(false));
  }, [device]);

  useEffect(() => { load(); }, [load]);

  const handleChange = (key: keyof ThresholdForm, val: string) => {
    setForm(f => f ? { ...f, [key]: parseFloat(val) } : null);
  };

  const handleSave = async () => {
    if (!form) return;
    setSaving(true);
    try {
      await updateThresholds(device, form);
      setSaved(true);
      setTimeout(() => setSaved(false), 3000);
    } catch (e) {
      console.error(e);
    } finally {
      setSaving(false);
    }
  };

  if (loading) return <div style={{ color: "var(--muted)", padding: 16 }}>Loading thresholds…</div>;
  if (!form)   return <div style={{ color: "var(--red)", padding: 16 }}>Failed to load thresholds.</div>;

  return (
    <div>
      {/* Info banner */}
      <div style={{ display: "flex", alignItems: "flex-start", gap: 10, padding: "12px 16px",
                    background: "rgba(59,130,246,0.08)", borderRadius: 10, marginBottom: 24,
                    border: "1px solid rgba(59,130,246,0.2)" }}>
        <Info size={16} color="#3b82f6" style={{ flexShrink: 0, marginTop: 2 }} />
        <p style={{ fontSize: 13, color: "var(--muted)", lineHeight: 1.6 }}>
          Changes take effect on the <strong style={{ color: "var(--text)" }}>next incoming MQTT message</strong>.
          Warning thresholds trigger yellow alerts; Critical thresholds trigger red alerts + max-ventilation protocol.
        </p>
      </div>

      {/* Threshold grid header */}
      <div className="threshold-row" style={{ borderBottom: "2px solid var(--border)", paddingBottom: 8 }}>
        <span style={{ fontSize: 12, fontWeight: 700, color: "var(--muted)", textTransform: "uppercase" }}>Parameter</span>
        <span style={{ fontSize: 12, fontWeight: 700, color: "#eab308", textTransform: "uppercase" }}>⚠ Warning</span>
        <span style={{ fontSize: 12, fontWeight: 700, color: "#ef4444", textTransform: "uppercase" }}>🔴 Critical</span>
      </div>

      {GROUPED.map(g => (
        <div key={g.metric} className="threshold-row">
          <div style={{ fontWeight: 600 }}>{g.metric} <span style={{ color: "var(--muted)", fontSize: 11 }}>({g.unit})</span></div>
          {/* Warning input */}
          <div style={{ display: "flex", alignItems: "center", gap: 8 }}>
            <input
              type="number"
              className="form-input"
              style={{ width: 90 }}
              value={form[g.warn]}
              min={g.min} max={g.max} step={g.step}
              onChange={e => handleChange(g.warn, e.target.value)}
            />
            <input
              type="range"
              min={g.min} max={g.max} step={g.step}
              value={form[g.warn]}
              onChange={e => handleChange(g.warn, e.target.value)}
              style={{ flex: 1, accentColor: "#eab308" }}
            />
          </div>
          {/* Critical input */}
          <div style={{ display: "flex", alignItems: "center", gap: 8 }}>
            <input
              type="number"
              className="form-input"
              style={{ width: 90 }}
              value={form[g.crit]}
              min={g.min} max={g.max} step={g.step}
              onChange={e => handleChange(g.crit, e.target.value)}
            />
            <input
              type="range"
              min={g.min} max={g.max} step={g.step}
              value={form[g.crit]}
              onChange={e => handleChange(g.crit, e.target.value)}
              style={{ flex: 1, accentColor: "#ef4444" }}
            />
          </div>
        </div>
      ))}

      <div style={{ display: "flex", justifyContent: "flex-end", gap: 12, marginTop: 24 }}>
        {saved && (
          <span style={{ color: "var(--green)", fontSize: 13, alignSelf: "center", display: "flex", alignItems: "center", gap: 4 }}>
            ✓ Saved successfully
          </span>
        )}
        <button className="btn btn-ghost" onClick={load} disabled={saving}>↩ Reset</button>
        <button className="btn btn-primary" onClick={handleSave} disabled={saving}>
          <Save size={14} />
          {saving ? "Saving…" : "Save Thresholds"}
        </button>
      </div>
    </div>
  );
}

export function Settings() {
  const [devices, setDevices] = useState<DeviceOut[]>([]);
  const [device,  setDevice]  = useState<string>("");

  useEffect(() => {
    fetchDevices().then(d => {
      setDevices(d);
      if (d.length > 0) setDevice(d[0].device_id);
    });
  }, []);

  return (
    <div>
      <div className="page-header">
        <h1>⚙️ Settings</h1>
        <p>Configure alert thresholds per device — changes apply dynamically</p>
      </div>

      {/* Device selector */}
      {devices.length > 1 && (
        <div className="tab-bar" style={{ maxWidth: 500, marginBottom: 24 }}>
          {devices.map(d => (
            <div
              key={d.device_id}
              className={`tab${device === d.device_id ? " active" : ""}`}
              onClick={() => setDevice(d.device_id)}
            >
              {d.device_id}
            </div>
          ))}
        </div>
      )}

      {device ? (
        <div className="card">
          <div className="card-header">
            <span className="card-title">Thresholds — {device}</span>
          </div>
          <ThresholdPanel key={device} device={device} />
        </div>
      ) : (
        <div className="card empty-state">
          No devices found. Connect a device via MQTT first.
        </div>
      )}

      {/* Alert spec reference */}
      <div className="card" style={{ marginTop: 24 }}>
        <div className="card-header">
          <span className="card-title">Alert Evaluation Logic Reference</span>
        </div>
        <table>
          <thead>
            <tr><th>Level</th><th>Condition</th><th>Action</th></tr>
          </thead>
          <tbody>
            <tr>
              <td><span className="badge badge-red">🔴 Critical</span></td>
              <td style={{ fontSize: 12 }}>MQ3 &gt; critical_mq3 <em>OR</em> MQ135 &gt; critical_mq135 <em>OR</em> Temp &gt; critical_temp</td>
              <td style={{ fontSize: 12, color: "var(--muted)" }}>Max ventilation, emergency alert, buzzer</td>
            </tr>
            <tr>
              <td><span className="badge badge-yellow">⚠ Warning</span></td>
              <td style={{ fontSize: 12 }}>MQ3 &gt; warn_mq3 <em>OR</em> MQ135 &gt; warn_mq135 <em>OR</em> Temp &gt; warn_temp</td>
              <td style={{ fontSize: 12, color: "var(--muted)" }}>Preventive cooling, high telemetry frequency, incident log</td>
            </tr>
            <tr>
              <td><span className="badge badge-green">✓ Normal</span></td>
              <td style={{ fontSize: 12 }}>All values below warning thresholds</td>
              <td style={{ fontSize: 12, color: "var(--muted)" }}>Log to historical timeseries only</td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>
  );
}
