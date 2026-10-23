package com.agrihub.coolhub.ui;

import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.fragment.app.Fragment;

import com.agrihub.coolhub.R;
import com.agrihub.coolhub.data.model.DeviceStatus;
import com.agrihub.coolhub.data.model.TelemetryReading;
import com.agrihub.coolhub.data.network.AgriHubApi;
import com.agrihub.coolhub.data.network.ApiClient;
import com.agrihub.coolhub.databinding.FragmentMapBinding;
import com.agrihub.coolhub.util.DateUtils;
import com.agrihub.coolhub.util.Prefs;

import org.osmdroid.config.Configuration;
import org.osmdroid.tileprovider.tilesource.TileSourceFactory;
import org.osmdroid.util.GeoPoint;
import org.osmdroid.views.MapView;
import org.osmdroid.views.overlay.Marker;

import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;

/**
 * MapFragment — displays the backend device GPS location on a real OpenStreetMap.
 *
 * Implementation notes:
 *  - Uses osmdroid 6.1.x with standard OpenStreetMap HTTPS tile server.
 *  - User-Agent set to "FactorySense/2.0" to comply with OSM tile usage policy.
 *  - The map shows the DEVICE'S coordinates from the backend (not the phone's GPS).
 *  - Phone location permission is NOT requested; this fragment does not use it.
 *  - Coordinates (0,0) are rejected as invalid — they represent the Gulf of Guinea.
 *  - Invalid coordinates (|lat| > 90 or |lon| > 180) are rejected.
 *  - Attribution © OpenStreetMap contributors is displayed in the info panel.
 *  - Tile fetching is delegated entirely to osmdroid; no tile scraping or prefetching.
 */
public class MapFragment extends Fragment {

    private FragmentMapBinding binding;
    private MapView mapView;
    private Marker deviceMarker;
    private Prefs prefs;

    @Override
    public View onCreateView(@NonNull LayoutInflater inflater,
                             ViewGroup container, Bundle savedInstanceState) {
        // Set a descriptive User-Agent per OSM tile usage policy before map init
        Configuration.getInstance().setUserAgentValue("FactorySense/2.0 AgriHub Android");

        binding = FragmentMapBinding.inflate(inflater, container, false);
        return binding.getRoot();
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        prefs = new Prefs(requireContext());

        // Configure osmdroid map
        mapView = binding.mapView;
        mapView.setTileSource(TileSourceFactory.MAPNIK);
        mapView.setMultiTouchControls(true);
        mapView.getController().setZoom(15.0);

        binding.btnRetry.setOnClickListener(v -> loadDeviceLocation());
        binding.btnRecenter.setOnClickListener(v -> recenterMap());

        loadDeviceLocation();
    }

    // ── Data loading ──────────────────────────────────────────────────────────

    private void loadDeviceLocation() {
        showLoading();
        AgriHubApi api = ApiClient.create(prefs.getBackendUrl());
        api.getDeviceStatus(Prefs.DEFAULT_DEVICE).enqueue(new Callback<DeviceStatus>() {
            @Override
            public void onResponse(@NonNull Call<DeviceStatus> call,
                                   @NonNull Response<DeviceStatus> response) {
                if (!isAdded() || binding == null) return;
                if (response.isSuccessful() && response.body() != null) {
                    showContent(response.body());
                } else {
                    showError(getString(R.string.error_load_failed));
                }
            }

            @Override
            public void onFailure(@NonNull Call<DeviceStatus> call, @NonNull Throwable t) {
                if (!isAdded() || binding == null) return;
                showError(t.getMessage() != null ? t.getMessage()
                        : getString(R.string.error_network));
            }
        });
    }

    private void showContent(DeviceStatus status) {
        binding.progressBar.setVisibility(View.GONE);
        binding.layoutError.setVisibility(View.GONE);
        binding.layoutContent.setVisibility(View.VISIBLE);

        // Device ID
        String deviceId = status.deviceId != null ? status.deviceId : Prefs.DEFAULT_DEVICE;
        binding.tvDeviceId.setText(deviceId);

        TelemetryReading latest = status.latest;

        if (latest == null || !hasValidCoordinates(latest.latitude, latest.longitude)) {
            // No valid coordinates — clear any existing marker, show message
            clearMarker();
            binding.tvNoLocation.setVisibility(View.VISIBLE);
            binding.tvCoordinates.setVisibility(View.GONE);
            binding.tvLocationTimestamp.setVisibility(View.GONE);
            binding.btnRecenter.setEnabled(false);
            // Default map center — India approximate center
            mapView.getController().setCenter(new GeoPoint(20.5937, 78.9629));
            mapView.getController().setZoom(5.0);
            return;
        }

        double lat = latest.latitude;
        double lon = latest.longitude;

        binding.tvNoLocation.setVisibility(View.GONE);
        binding.tvCoordinates.setVisibility(View.VISIBLE);
        binding.tvCoordinates.setText(getString(R.string.map_coordinates, lat, lon));
        binding.btnRecenter.setEnabled(true);

        if (latest.timestamp != null) {
            binding.tvLocationTimestamp.setVisibility(View.VISIBLE);
            binding.tvLocationTimestamp.setText(
                    getString(R.string.map_location_updated,
                            DateUtils.formatFull(latest.timestamp)));
        } else {
            binding.tvLocationTimestamp.setVisibility(View.GONE);
        }

        placeMarker(deviceId, lat, lon);
        mapView.getController().animateTo(new GeoPoint(lat, lon));
        mapView.getController().setZoom(15.0);
    }

    // ── Map helpers ───────────────────────────────────────────────────────────

    /**
     * Validates lat/lon before placing a marker.
     * Rejects (0,0) — Gulf of Guinea — as a missing-coordinate sentinel.
     * Rejects any value outside the valid geographic range.
     */
    private boolean hasValidCoordinates(@Nullable Double lat, @Nullable Double lon) {
        if (lat == null || lon == null) return false;
        if (lat == 0.0 && lon == 0.0) return false;   // (0,0) = no location
        if (lat < -90 || lat > 90)    return false;
        if (lon < -180 || lon > 180)  return false;
        return true;
    }

    private void placeMarker(String deviceId, double lat, double lon) {
        GeoPoint point = new GeoPoint(lat, lon);

        if (deviceMarker == null) {
            deviceMarker = new Marker(mapView);
            mapView.getOverlays().add(deviceMarker);
        }

        deviceMarker.setPosition(point);
        deviceMarker.setAnchor(Marker.ANCHOR_CENTER, Marker.ANCHOR_BOTTOM);
        deviceMarker.setTitle(deviceId);
        deviceMarker.setSnippet(getString(R.string.map_coordinates, lat, lon));

        // Use the material location pin icon, tinted green
        Drawable pinIcon = ContextCompat.getDrawable(requireContext(), R.drawable.ic_map);
        if (pinIcon != null) {
            pinIcon.setTint(requireContext().getColor(R.color.green_700));
            deviceMarker.setIcon(pinIcon);
        }

        mapView.invalidate();
    }

    private void clearMarker() {
        if (deviceMarker != null) {
            mapView.getOverlays().remove(deviceMarker);
            deviceMarker = null;
            mapView.invalidate();
        }
    }

    private void recenterMap() {
        if (deviceMarker != null && deviceMarker.getPosition() != null) {
            mapView.getController().animateTo(deviceMarker.getPosition());
            mapView.getController().setZoom(15.0);
        }
    }

    // ── Loading / error states ────────────────────────────────────────────────

    private void showLoading() {
        binding.progressBar.setVisibility(View.VISIBLE);
        binding.layoutError.setVisibility(View.GONE);
        binding.layoutContent.setVisibility(View.GONE);
    }

    private void showError(String msg) {
        binding.progressBar.setVisibility(View.GONE);
        binding.layoutError.setVisibility(View.VISIBLE);
        binding.layoutContent.setVisibility(View.GONE);
        binding.tvErrorMessage.setText(msg);
    }

    // ── Lifecycle — osmdroid requires these ──────────────────────────────────

    @Override
    public void onResume() {
        super.onResume();
        if (mapView != null) mapView.onResume();
    }

    @Override
    public void onPause() {
        super.onPause();
        if (mapView != null) mapView.onPause();
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        if (mapView != null) mapView.onDetach();
        binding = null;
    }
}
