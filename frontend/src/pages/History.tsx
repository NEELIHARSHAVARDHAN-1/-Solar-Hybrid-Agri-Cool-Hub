import { useEffect, useState, useCallback } from "react";
import { fetchDevices, fetchTelemetry } from "../api";
import type { DeviceOut, TelemetryOut } from "../api";
import {
  LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer, Legend,
} from "recharts";
import { format, subHours, parseISO } from "date-fns";

type Window = "1h" | "6h" | "24h";

const WINDOWS: { label: string; val: Window; hours: number }[] = [
  { label: "Past 1 Hour",  val: "1h",  hours: 1 },
  { label: "Past 6 Hours", val: "6h",  hours: 6 },
  { label: "Past 24 Hours",val: "24h", hours: 24 },
];

const SERIES = [
  { key: "temperature", color: "#3b82f6", label: "Temp (°C)",    yMax: 50 },
  { key: "humidity",    color: "#14b8a6", label: "Humidity (%)", yMax: 100 },
  { key: "mq3",         color: "#f97316", label: "MQ3 (%)",      yMax: 100 },
  { key: "mq135",       color: "#a855f7", label: "MQ135 (%)",    yMax: 100 },
  { key: "mq4",         color: "#ec4899", label: "MQ4 (%)",      yMax: 100 },
  { key: "battery_percent", color: "#22c55e", label: "Battery (%)", yMax: 100 },
];

function Chart({
  data,
  series,
  title,
}: {
  data: { time: string; [k: string]: any }[];
  series: typeof SERIES[0][];
  title: string;
}) {
  return (
    <div className="card" style={{ marginBottom: 20 }}>
      <div className="card-header">
        <span className="card-title">{title}</span>
      </div>
      <ResponsiveContainer width="100%" height={220}>
        <LineChart data={data} margin={{ top: 5, right: 10, left: -10, bottom: 5 }}>
          <CartesianGrid strokeDasharray="3 3" stroke="#2e3348" />
          <XAxis dataKey="time" tick={{ fill: "#8a91a8", fontSize: 11 }} tickLine={false} />
          <YAxis tick={{ fill: "#8a91a8", fontSize: 11 }} tickLine={false} axisLine={false} />
          <Tooltip
            contentStyle={{ background: "#22263a", border: "1px solid #2e3348", borderRadius: 8, fontSize: 12 }}
            labelStyle={{ color: "#e8eaf0" }}
          />
          <Legend wrapperStyle={{ fontSize: 12, color: "#8a91a8" }} />
          {series.map(s => (
            <Line
              key={s.key}
              type="monotone"
              dataKey={s.key}
              name={s.label}
              stroke={s.color}
              dot={false}
              strokeWidth={2}
              connectNulls
            />
          ))}
        </LineChart>
      </ResponsiveContainer>
    </div>
  );
}

export function History() {
  const [devices, setDevices]   = useState<DeviceOut[]>([]);
  const [device,  setDevice]    = useState<string>("");
  const [window,  setWindow]    = useState<Window>("1h");
  const [items,   setItems]     = useState<TelemetryOut[]>([]);
  const [loading, setLoading]   = useState(false);

  useEffect(() => {
    fetchDevices().then(d => {
      setDevices(d);
      if (d.length > 0) setDevice(d[0].device_id);
    });
  }, []);

  const load = useCallback(() => {
    if (!device) return;
    const hours = WINDOWS.find(w => w.val === window)!.hours;
    const from  = subHours(new Date(), hours).toISOString();
    setLoading(true);
    fetchTelemetry(device, from, undefined, 500)
      .then(r => setItems([...r.items].reverse()))
      .catch(console.error)
      .finally(() => setLoading(false));
  }, [device, window]);

  useEffect(() => { load(); }, [load]);
  useEffect(() => {
    const id = setInterval(load, 15_000);
    return () => clearInterval(id);
  }, [load]);

  const chartData = items.map(t => ({
    time:            format(parseISO(t.timestamp), "HH:mm"),
    temperature:     t.temperature,
    humidity:        t.humidity,
    mq3:             +t.mq3.toFixed(2),
    mq135:           +t.mq135.toFixed(2),
    mq4:             +t.mq4.toFixed(2),
    battery_percent: t.battery_percent,
  }));

  return (
    <div>
      <div className="page-header">
        <h1>📈 History</h1>
        <p>Time-series logs for any device and time window</p>
      </div>

      {/* Controls */}
      <div className="form-row" style={{ marginBottom: 24 }}>
        <div className="form-group">
          <label className="form-label">Device</label>
          <select
            className="form-input"
            value={device}
            onChange={e => setDevice(e.target.value)}
            style={{ width: 200 }}
          >
            {devices.map(d => <option key={d.device_id} value={d.device_id}>{d.device_id}</option>)}
          </select>
        </div>
        <div className="form-group">
          <label className="form-label">Time Window</label>
          <div className="tab-bar" style={{ marginBottom: 0, width: 360 }}>
            {WINDOWS.map(w => (
              <div
                key={w.val}
                className={`tab${window === w.val ? " active" : ""}`}
                onClick={() => setWindow(w.val)}
              >
                {w.label}
              </div>
            ))}
          </div>
        </div>
        <div className="form-group" style={{ justifyContent: "flex-end" }}>
          <label className="form-label">&nbsp;</label>
          <button className="btn btn-ghost" onClick={load}>↻ Refresh</button>
        </div>
      </div>

      {loading && <div style={{ color: "var(--muted)", marginBottom: 16 }}>Loading…</div>}

      {!loading && chartData.length === 0 && (
        <div className="card empty-state">No data for this window.</div>
      )}

      {chartData.length > 0 && (
        <>
          <Chart data={chartData} series={[SERIES[0], SERIES[1]]} title="Temperature & Humidity" />
          <Chart data={chartData} series={[SERIES[2], SERIES[3], SERIES[4]]} title="Gas Sensors (MQ3 / MQ135 / MQ4)" />
          <Chart data={chartData} series={[SERIES[5]]} title="Battery Level" />

          {/* Raw table */}
          <div className="card">
            <div className="card-header">
              <span className="card-title">Raw Readings ({items.length} rows)</span>
            </div>
            <div className="table-wrap">
              <table>
                <thead>
                  <tr>
                    <th>Timestamp</th>
                    <th>Temp (°C)</th>
                    <th>Humidity (%)</th>
                    <th>MQ3</th>
                    <th>MQ135</th>
                    <th>MQ4</th>
                    <th>Battery (%)</th>
                  </tr>
                </thead>
                <tbody>
                  {[...items].reverse().slice(0, 100).map(t => (
                    <tr key={t.id}>
                      <td>{format(parseISO(t.timestamp), "yyyy-MM-dd HH:mm:ss")}</td>
                      <td>{t.temperature?.toFixed(2) ?? "--"}</td>
                      <td>{t.humidity?.toFixed(2) ?? "--"}</td>
                      <td>{t.mq3.toFixed(2)}</td>
                      <td>{t.mq135.toFixed(2)}</td>
                      <td>{t.mq4.toFixed(2)}</td>
                      <td>{t.battery_percent ?? "--"}</td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          </div>
        </>
      )}
    </div>
  );
}
