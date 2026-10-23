package com.agrihub.coolhub.data.network;

import com.agrihub.coolhub.data.model.Alert;
import com.agrihub.coolhub.data.model.DeviceStatus;
import com.agrihub.coolhub.data.model.PaginatedAlerts;
import com.agrihub.coolhub.data.model.PaginatedTelemetry;
import com.agrihub.coolhub.data.model.TelemetryReading;

import java.util.List;
import java.util.Map;

import retrofit2.Call;
import retrofit2.http.GET;
import retrofit2.http.PATCH;
import retrofit2.http.Path;
import retrofit2.http.Query;

/**
 * Retrofit interface for the FactorySense backend REST API.
 *
 * All endpoints mirror backend/app/routers/* routes exactly.
 * No endpoint paths are invented — all are verified against the backend source.
 *
 * Base URL configuration:
 *   - Android emulator → use http://10.0.2.2:8000
 *   - Physical device (same LAN) → use http://<host-IP>:8000
 *   - Production → use https://<your-domain>
 */
public interface AgriHubApi {

    // ── Health ──────────────────────────────────────────────────────────────

    /** GET /health */
    @GET("health")
    Call<Map<String, Object>> checkHealth();

    // ── Devices ─────────────────────────────────────────────────────────────

    /** GET /devices */
    @GET("devices")
    Call<List<Map<String, Object>>> listDevices();

    /** GET /devices/{device_id} */
    @GET("devices/{device_id}")
    Call<Map<String, Object>> getDevice(@Path("device_id") String deviceId);

    /** GET /devices/{device_id}/status */
    @GET("devices/{device_id}/status")
    Call<DeviceStatus> getDeviceStatus(@Path("device_id") String deviceId);

    // ── Telemetry ────────────────────────────────────────────────────────────

    /** GET /devices/{device_id}/telemetry */
    @GET("devices/{device_id}/telemetry")
    Call<PaginatedTelemetry> getTelemetry(
            @Path("device_id") String deviceId,
            @Query("limit") int limit,
            @Query("offset") int offset,
            @Query("from") String from,
            @Query("to") String to
    );

    /** GET /devices/{device_id}/telemetry/latest */
    @GET("devices/{device_id}/telemetry/latest")
    Call<TelemetryReading> getLatestTelemetry(@Path("device_id") String deviceId);

    // ── Alerts ───────────────────────────────────────────────────────────────

    /**
     * GET /devices/{device_id}/alerts
     * Optional filters: alert_level ("warning"|"critical"), resolved (true|false)
     */
    @GET("devices/{device_id}/alerts")
    Call<PaginatedAlerts> getAlerts(
            @Path("device_id") String deviceId,
            @Query("limit") int limit,
            @Query("offset") int offset,
            @Query("alert_level") String alertLevel,
            @Query("resolved") Boolean resolved
    );

    /** GET /devices/{device_id}/alerts/active */
    @GET("devices/{device_id}/alerts/active")
    Call<PaginatedAlerts> getActiveAlerts(@Path("device_id") String deviceId);

    /** GET /alerts — all alerts across all devices */
    @GET("alerts")
    Call<PaginatedAlerts> getAllAlerts(
            @Query("alert_level") String alertLevel,
            @Query("resolved") Boolean resolved,
            @Query("limit") int limit,
            @Query("offset") int offset
    );

    /** PATCH /alerts/{alert_id}/resolve */
    @PATCH("alerts/{alert_id}/resolve")
    Call<Alert> resolveAlert(@Path("alert_id") int alertId);

    // ── Thresholds ───────────────────────────────────────────────────────────

    /** GET /devices/{device_id}/thresholds */
    @GET("devices/{device_id}/thresholds")
    Call<Map<String, Object>> getThresholds(@Path("device_id") String deviceId);
}
