// Radial gauge using SVG arc
interface GaugeProps {
  value: number;
  min?: number;
  max: number;
  warning: number;
  critical: number;
  label: string;
  unit: string;
  size?: number;
}

function getColor(value: number, warning: number, critical: number) {
  if (value >= critical) return "#ef4444";
  if (value >= warning)  return "#eab308";
  return "#22c55e";
}

export function RadialGauge({ value, min = 0, max, warning, critical, label, unit, size = 120 }: GaugeProps) {
  const pct   = Math.min(1, Math.max(0, (value - min) / (max - min)));
  const color = getColor(value, warning, critical);
  const stroke = 10;
  const r     = (size - stroke * 2) / 2;
  const cx    = size / 2;
  const cy    = size / 2;
  const startAngle = -210; // degrees
  const sweepAngle = 240;
  const angle  = startAngle + pct * sweepAngle;

  function polar(angleDeg: number, radius: number) {
    const rad = (angleDeg * Math.PI) / 180;
    return [cx + radius * Math.cos(rad), cy + radius * Math.sin(rad)];
  }

  function arc(fromDeg: number, toDeg: number, radius: number, color: string, strokeW: number) {
    const steps = 60;
    const delta = (toDeg - fromDeg) / steps;
    const pts = Array.from({ length: steps + 1 }, (_, i) => polar(fromDeg + i * delta, radius));
    const d = pts.map((p, i) => `${i === 0 ? "M" : "L"} ${p[0].toFixed(2)} ${p[1].toFixed(2)}`).join(" ");
    return <path d={d} stroke={color} strokeWidth={strokeW} fill="none" strokeLinecap="round" />;
  }

  const displayVal = Number.isFinite(value) ? (value % 1 === 0 ? value : value.toFixed(1)) : "--";

  return (
    <div className="gauge-wrap">
      <svg width={size} height={size} viewBox={`0 0 ${size} ${size}`}>
        {/* Background track */}
        {arc(startAngle, startAngle + sweepAngle, r, "#2e3348", stroke)}
        {/* Value arc */}
        {pct > 0 && arc(startAngle, angle, r, color, stroke)}
        {/* Warning marker */}
        {(() => {
          const wPct = Math.min(1, Math.max(0, (warning - min) / (max - min)));
          const wa = startAngle + wPct * sweepAngle;
          const [x1, y1] = polar(wa, r - stroke);
          const [x2, y2] = polar(wa, r + stroke / 2);
          return <line x1={x1} y1={y1} x2={x2} y2={y2} stroke="#eab308" strokeWidth={2} />;
        })()}
        {/* Critical marker */}
        {(() => {
          const cPct = Math.min(1, Math.max(0, (critical - min) / (max - min)));
          const ca = startAngle + cPct * sweepAngle;
          const [x1, y1] = polar(ca, r - stroke);
          const [x2, y2] = polar(ca, r + stroke / 2);
          return <line x1={x1} y1={y1} x2={x2} y2={y2} stroke="#ef4444" strokeWidth={2} />;
        })()}
        {/* Center value */}
        <text x={cx} y={cy + 6} textAnchor="middle" fill={color} fontSize={size * 0.18} fontWeight="800">
          {displayVal}
        </text>
        <text x={cx} y={cy + 20} textAnchor="middle" fill="#8a91a8" fontSize={size * 0.1}>
          {unit}
        </text>
      </svg>
      <div className="gauge-label">{label}</div>
    </div>
  );
}
