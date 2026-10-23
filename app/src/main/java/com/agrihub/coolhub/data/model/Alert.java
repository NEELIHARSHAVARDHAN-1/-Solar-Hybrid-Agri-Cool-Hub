package com.agrihub.coolhub.data.model;

import com.google.gson.annotations.SerializedName;

/**
 * Mirrors AlertOut from backend /devices/{id}/alerts.
 * Backend fields: id, device_id, timestamp, alert_level, alert_type, metric,
 *                 value, threshold, resolved_at, created_at
 */
public class Alert {

    @SerializedName("id")
    public int id;

    @SerializedName("device_id")
    public String deviceId;

    @SerializedName("timestamp")
    public String timestamp;

    /** "warning" or "critical" — set by backend threshold evaluation. */
    @SerializedName("alert_level")
    public String alertLevel;

    /** "MQ3_HIGH" | "MQ135_HIGH" | "MQ4_HIGH" | "TEMP_HIGH" | "HUMIDITY_HIGH" */
    @SerializedName("alert_type")
    public String alertType;

    /** Human-readable metric label from backend, e.g. "MQ-3 Sensor" */
    @SerializedName("metric")
    public String metric;

    @SerializedName("value")
    public double value;

    @SerializedName("threshold")
    public double threshold;

    @SerializedName("resolved_at")
    public String resolvedAt;

    @SerializedName("created_at")
    public String createdAt;

    /** Active if resolved_at is null or empty. */
    public boolean isActive() {
        return resolvedAt == null || resolvedAt.isEmpty();
    }

    /** Returns true if this is a critical-level alert. */
    public boolean isCritical() {
        return "critical".equalsIgnoreCase(alertLevel);
    }

    /** Human-readable label: prefer metric field, fall back to formatted alert_type. */
    public String getDisplayLabel() {
        if (metric != null && !metric.isEmpty()) return metric;
        if (alertType == null) return "Unknown Alert";
        return alertType.replace("_", " ").toUpperCase();
    }
}
