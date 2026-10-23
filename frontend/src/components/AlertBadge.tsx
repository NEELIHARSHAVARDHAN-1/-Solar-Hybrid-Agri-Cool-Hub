type Level = "normal" | "warning" | "critical" | "offline";

interface Props {
  level: Level;
  text?: string;
}

const cfg: Record<Level, { cls: string; dot: string; label: string }> = {
  normal:   { cls: "badge-green",  dot: "#22c55e", label: "NORMAL" },
  warning:  { cls: "badge-yellow", dot: "#eab308", label: "WARNING" },
  critical: { cls: "badge-red",    dot: "#ef4444", label: "CRITICAL" },
  offline:  { cls: "badge-grey",   dot: "#8a91a8", label: "OFFLINE" },
};

export function AlertBadge({ level, text }: Props) {
  const { cls, dot, label } = cfg[level] ?? cfg.normal;
  return (
    <span className={`badge ${cls}`}>
      <span style={{ width: 7, height: 7, borderRadius: "50%", background: dot, display: "inline-block" }} />
      {text ?? label}
    </span>
  );
}
