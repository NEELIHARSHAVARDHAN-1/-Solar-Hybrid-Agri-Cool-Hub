import { NavLink } from "react-router-dom";
import { LayoutDashboard, History, Bell, Settings, Leaf } from "lucide-react";

const links = [
  { to: "/",         label: "Dashboard", icon: LayoutDashboard },
  { to: "/history",  label: "History",   icon: History },
  { to: "/alerts",   label: "Alerts",    icon: Bell },
  { to: "/settings", label: "Settings",  icon: Settings },
];

export function Sidebar() {
  return (
    <aside className="sidebar">
      <div className="sidebar-logo">
        <Leaf size={22} />
        <span>AgriHub</span>
      </div>
      <nav className="sidebar-nav">
        {links.map(({ to, label, icon: Icon }) => (
          <NavLink
            key={to}
            to={to}
            end={to === "/"}
            className={({ isActive }) => `nav-item${isActive ? " active" : ""}`}
          >
            <Icon size={16} />
            {label}
          </NavLink>
        ))}
      </nav>
      <div style={{ padding: "0 20px 12px", color: "var(--muted)", fontSize: 11 }}>
        AgriHub v2.0 &nbsp;·&nbsp; IoT Monitor
      </div>
    </aside>
  );
}
