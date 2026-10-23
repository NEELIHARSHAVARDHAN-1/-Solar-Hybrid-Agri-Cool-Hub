package com.agrihub.coolhub.data.model;

import com.google.gson.annotations.SerializedName;

/**
 * Mirrors DeviceStatus from backend /devices/{id}/status.
 */
public class DeviceStatus {

    @SerializedName("device_id")
    public String deviceId;

    @SerializedName("is_online")
    public boolean isOnline;

    @SerializedName("last_seen")
    public String lastSeen;

    @SerializedName("latest")
    public TelemetryReading latest;
}
