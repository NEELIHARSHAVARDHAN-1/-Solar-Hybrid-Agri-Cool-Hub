import { BrowserRouter, Routes, Route } from "react-router-dom";
import { Sidebar } from "./components/Sidebar";
import { Dashboard } from "./pages/Dashboard";
import { History } from "./pages/History";
import { Alerts } from "./pages/Alerts";
import { Settings } from "./pages/Settings";
import "./index.css";

export default function App() {
  return (
    <BrowserRouter>
      <div className="shell">
        <Sidebar />
        <main className="main-content">
          <Routes>
            <Route path="/"         element={<Dashboard />} />
            <Route path="/history"  element={<History />} />
            <Route path="/alerts"   element={<Alerts />} />
            <Route path="/settings" element={<Settings />} />
          </Routes>
        </main>
      </div>
    </BrowserRouter>
  );
}
