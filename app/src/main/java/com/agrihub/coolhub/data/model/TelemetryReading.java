package com.agrihub.coolhub.data.model;

import com.google.gson.annotations.SerializedName;

/**
 * Mirrors TelemetryReading from the backend API.
 * Fields match the JSON payload from /devices/{id}/telemetry.
 */
public class TelemetryReading {

    @SerializedName("id")
    public int id;

    @SerializedName("device_id")
    public String deviceId;

    @SerializedName("timestamp")
    public String timestamp;

    @SerializedName("temperature")
    public Double temperature;

    @SerializedName("humidity")
    public Double humidity;

    @SerializedName("mq3")
    public Double mq3;

    @SerializedName("mq135")
    public Double mq135;

    @SerializedName("mq4")
    public Double mq4;

    @SerializedName("battery_percent")
    public Integer batteryPercent;

    @SerializedName("latitude")
    public Double latitude;

    @SerializedName("longitude")
    public Double longitude;

    @SerializedName("received_at")
    public String receivedAt;
}
