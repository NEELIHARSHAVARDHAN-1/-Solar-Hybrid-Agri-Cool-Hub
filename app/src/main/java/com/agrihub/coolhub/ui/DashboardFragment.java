package com.agrihub.coolhub.ui;

import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.animation.ValueAnimator;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AnimationUtils;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import com.agrihub.coolhub.R;
import com.agrihub.coolhub.data.model.DeviceStatus;
import com.agrihub.coolhub.data.model.TelemetryReading;
import com.agrihub.coolhub.data.network.AgriHubApi;
import com.agrihub.coolhub.data.network.ApiClient;
import com.agrihub.coolhub.databinding.FragmentDashboardBinding;
import com.agrihub.coolhub.domain.FruitConfig;
import com.agrihub.coolhub.domain.RipenessEngine;
import com.agrihub.coolhub.util.DateUtils;
import com.agrihub.coolhub.util.Prefs;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;

import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;

/**
 * FactorySense Live Dashboard — real telemetry from FastAPI backend.
 *
 * Key behaviors:
 *  - MQ-3, MQ-135, MQ-4 displayed as normalized % (0–100 firmware scale), NOT voltage
 *  - Missing values show "N/A", never replaced with 0
 *  - Stale/previous data labeled with "Previous data" + timestamp
 *  - Auto-refresh every 15 seconds (lifecycle-aware, paused in background)
 *  - No overlapping requests
 */
public class DashboardFragment extends Fragment {

    /** Refresh interval — 15 s matches typical ESP32 MQTT publish rate. */
    private static final long AUTO_REFRESH_MS = 15_000L;

    /** A reading older than this is considered stale. */
    private static final long STALE_THRESHOLD_MS = 5 * 60 * 1000L;

    private FragmentDashboardBinding binding;
    private Prefs prefs;
    private AgriHubApi api;
    private final Handler refreshHandler = new Handler(Looper.getMainLooper());
    private FruitConfig selectedFruit;

    /** Track whether a network call is in flight to prevent overlapping requests. */
    private boolean loadInProgress = false;

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    @Override
    public View onCreateView(@NonNull LayoutInflater inflater,
                             ViewGroup container, Bundle savedInstanceState) {
        binding = FragmentDashboardBinding.inflate(inflater, container, false);
        return binding.getRoot();
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        prefs = new Prefs(requireContext());
        api   = ApiClient.create(prefs.getBackendUrl());
        selectedFruit = FruitConfig.fromIndex(prefs.getFruitIndex());

        binding.swipeRefresh.setColorSchemeResources(R.color.green_600);
        binding.swipeRefresh.setOnRefreshListener(this::loadData);
        binding.btnRetry.setOnClickListener(v -> loadData());

        binding.cardRipeness.btnSelectFruit.setOnClickListener(v -> showFruitPicker());

        loadData();
    }

    @Override
    public void onResume() {
        super.onResume();
        selectedFruit = FruitConfig.fromIndex(prefs.getFruitIndex());
        scheduleAutoRefresh();
    }

    @Override
    public void onPause() {
        super.onPause();
        refreshHandler.removeCallbacksAndMessages(null);
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        refreshHandler.removeCallbacksAndMessages(null);
        binding = null;
    }

    // ── Auto-refresh ──────────────────────────────────────────────────────────

    private void scheduleAutoRefresh() {
        refreshHandler.removeCallbacksAndMessages(null);
        refreshHandler.postDelayed(() -> {
            if (isAdded() && binding != null) {
                loadData();
                scheduleAutoRefresh();
            }
        }, AUTO_REFRESH_MS);
    }

    // ── Data loading ──────────────────────────────────────────────────────────

    private void loadData() {
        if (binding == null || loadInProgress) return;
        loadInProgress = true;
        showLoading();
        api = ApiClient.create(prefs.getBackendUrl());
        api.getDeviceStatus(Prefs.DEFAULT_DEVICE)
           .enqueue(new Callback<DeviceStatus>() {
               @Override
               public void onResponse(@NonNull Call<DeviceStatus> call,
                                      @NonNull Response<DeviceStatus> response) {
                   if (!isAdded() || binding == null) { loadInProgress = false; return; }
                   binding.swipeRefresh.setRefreshing(false);
                   loadInProgress = false;
                   if (response.isSuccessful() && response.body() != null) {
                       showContent(response.body());
                   } else {
                       showError("Server returned " + response.code());
                   }
               }

               @Override
               public void onFailure(@NonNull Call<DeviceStatus> call, @NonNull Throwable t) {
                   if (!isAdded() || binding == null) { loadInProgress = false; return; }
                   binding.swipeRefresh.setRefreshing(false);
                   loadInProgress = false;
                   showError(t.getMessage());
               }
           });
    }

    // ── UI states ─────────────────────────────────────────────────────────────

    private void showLoading() {
        if (binding.swipeRefresh.getVisibility() != View.VISIBLE) {
            binding.progressBar.setVisibility(View.VISIBLE);
        }
        binding.layoutError.setVisibility(View.GONE);
    }

    private void showError(String message) {
        binding.progressBar.setVisibility(View.GONE);
        binding.swipeRefresh.setVisibility(View.GONE);
        binding.layoutError.setVisibility(View.VISIBLE);
        binding.tvErrorMessage.setText(message != null ? message
                : getString(R.string.error_network));
        setConnectivity(ConnectivityState.OFFLINE);
    }

    private void showContent(DeviceStatus status) {
        binding.progressBar.setVisibility(View.GONE);
        binding.layoutError.setVisibility(View.GONE);
        binding.swipeRefresh.setVisibility(View.VISIBLE);

        // ── Device status banner ──────────────────────────────────────
        binding.tvDeviceId.setText(status.deviceId != null ? status.deviceId : Prefs.DEFAULT_DEVICE);

        boolean stale = DateUtils.isStale(status.lastSeen, STALE_THRESHOLD_MS);
        ConnectivityState conn;
        if (status.isOnline && !stale) {
            conn = ConnectivityState.LIVE;
        } else if (!status.isOnline && status.lastSeen != null) {
            conn = ConnectivityState.STALE;
        } else {
            conn = ConnectivityState.OFFLINE;
        }
        setConnectivity(conn);

        if (status.isOnline) {
            binding.chipOnlineStatus.setText(getString(R.string.label_online));
            binding.chipOnlineStatus.setChipBackgroundColorResource(R.color.green_600);
            binding.tvDeviceLastSeen.setText("Online");
        } else {
            binding.chipOnlineStatus.setText(getString(R.string.label_offline));
            binding.chipOnlineStatus.setChipBackgroundColorResource(R.color.red_700);
            binding.tvDeviceLastSeen.setText(
                    status.lastSeen != null
                    ? getString(R.string.label_last_seen, DateUtils.formatShort(status.lastSeen))
                    : getString(R.string.label_never_connected));
        }

        // Stale banner
        binding.cardStaleWarning.setVisibility((stale && !status.isOnline) ? View.VISIBLE : View.GONE);

        TelemetryReading r = status.latest;

        // ── Ripeness card ─────────────────────────────────────────────
        updateRipenessCard(r);

        // ── Sensor cards ──────────────────────────────────────────────
        if (r == null) {
            binding.cardTemperature.getRoot().setVisibility(View.GONE);
            binding.cardHumidity.getRoot().setVisibility(View.GONE);
            binding.cardMq3.getRoot().setVisibility(View.GONE);
            binding.cardMq135.getRoot().setVisibility(View.GONE);
            binding.cardMq4.getRoot().setVisibility(View.GONE);
            binding.cardBattery.getRoot().setVisibility(View.GONE);
            binding.tvLastReading.setText(getString(R.string.no_readings));
            updateInsights(null);
            return;
        }

        // Determine data freshness label
        boolean isPreviousData = DateUtils.isStale(r.timestamp, STALE_THRESHOLD_MS);
        String freshnessLabel = isPreviousData
                ? getString(R.string.previous_data_at, DateUtils.formatShort(r.timestamp))
                : null;

        // Temperature — use °C scale correctly (not %)
        configureSensorCard(binding.cardTemperature.getRoot(),
                R.color.sensor_temp, getString(R.string.label_temperature), null,
                r.temperature != null ? String.format("%.1f °C", r.temperature) : getString(R.string.na),
                freshnessLabel);

        configureSensorCard(binding.cardHumidity.getRoot(),
                R.color.sensor_hum, getString(R.string.label_humidity), null,
                r.humidity != null ? String.format("%.1f %%", r.humidity) : getString(R.string.na),
                freshnessLabel);

        // MQ sensors — normalized analog output %, NOT voltage
        configureSensorCard(binding.cardMq3.getRoot(),
                R.color.sensor_mq3, getString(R.string.label_mq3),
                getString(R.string.label_mq3_sub),
                r.mq3 != null ? String.format("%.1f %%", r.mq3) : getString(R.string.na),
                freshnessLabel);

        configureSensorCard(binding.cardMq135.getRoot(),
                R.color.sensor_mq135, getString(R.string.label_mq135),
                getString(R.string.label_mq135_sub),
                r.mq135 != null ? String.format("%.1f %%", r.mq135) : getString(R.string.na),
                freshnessLabel);

        configureSensorCard(binding.cardMq4.getRoot(),
                R.color.sensor_mq4, getString(R.string.label_mq4),
                getString(R.string.label_mq4_sub),
                r.mq4 != null ? String.format("%.1f %%", r.mq4) : getString(R.string.na),
                freshnessLabel);

        if (r.batteryPercent != null) {
            int battColor = r.batteryPercent > 50 ? R.color.sensor_battery_ok
                    : r.batteryPercent > 20 ? R.color.sensor_battery_mid
                    : R.color.sensor_battery_low;
            configureSensorCard(binding.cardBattery.getRoot(),
                    battColor, getString(R.string.label_battery), null,
                    r.batteryPercent + " %",
                    freshnessLabel);
            binding.cardBattery.getRoot().setVisibility(View.VISIBLE);
        } else {
            binding.cardBattery.getRoot().setVisibility(View.GONE);
        }

        String lastReadingText = isPreviousData
                ? getString(R.string.previous_data_at, DateUtils.formatFull(r.timestamp))
                : getString(R.string.label_last_reading, DateUtils.formatFull(r.timestamp));
        binding.tvLastReading.setText(lastReadingText);

        updateInsights(r);
    }

    // ── Connectivity indicator ─────────────────────────────────────────────────

    private enum ConnectivityState { LIVE, STALE, OFFLINE }

    private void setConnectivity(ConnectivityState state) {
        if (binding == null) return;
        switch (state) {
            case LIVE:
                binding.tvConnectivity.setText(getString(R.string.conn_live));
                binding.tvConnectivity.setTextColor(requireContext().getColor(R.color.conn_live));
                binding.tvConnectivity.startAnimation(
                        AnimationUtils.loadAnimation(requireContext(), R.anim.pulse));
                break;
            case STALE:
                binding.tvConnectivity.setText(getString(R.string.conn_stale));
                binding.tvConnectivity.setTextColor(requireContext().getColor(R.color.conn_stale));
                binding.tvConnectivity.clearAnimation();
                break;
            case OFFLINE:
                binding.tvConnectivity.setText(getString(R.string.conn_offline));
                binding.tvConnectivity.setTextColor(requireContext().getColor(R.color.conn_offline));
                binding.tvConnectivity.clearAnimation();
                break;
        }
    }

    // ── Sensor card configuration ─────────────────────────────────────────────

    private void configureSensorCard(View card, int colorRes,
                                     String label, @Nullable String subLabel,
                                     String value, @Nullable String previousDataLabel) {
        card.setVisibility(View.VISIBLE);

        View accentBar   = card.findViewById(R.id.viewAccentBar);
        ImageView icon   = card.findViewById(R.id.ivSensorIcon);
        TextView lbl     = card.findViewById(R.id.tvSensorLabel);
        TextView subLbl  = card.findViewById(R.id.tvSensorSubLabel);
        TextView val     = card.findViewById(R.id.tvSensorValue);
        TextView prevLbl = card.findViewById(R.id.tvPreviousDataLabel);

        int color = requireContext().getColor(colorRes);
        accentBar.setBackgroundColor(color);
        icon.setImageResource(R.drawable.ic_leaf);
        icon.setColorFilter(color);
        lbl.setText(label);
        val.setTextColor(color);

        if (subLabel != null && !subLabel.isEmpty()) {
            subLbl.setText(subLabel);
            subLbl.setVisibility(View.VISIBLE);
        } else {
            subLbl.setVisibility(View.GONE);
        }

        // Previous data label — only visible when reading is stale
        if (prevLbl != null) {
            if (previousDataLabel != null) {
                prevLbl.setText(previousDataLabel);
                prevLbl.setVisibility(View.VISIBLE);
            } else {
                prevLbl.setVisibility(View.GONE);
            }
        }

        String current = val.getText().toString();
        if (!current.equals(value) && !current.isEmpty()) {
            animateTextChange(val, value, color);
        } else {
            val.setText(value);
        }
    }

    private void animateTextChange(TextView tv, String newValue, int color) {
        ObjectAnimator fadeOut = ObjectAnimator.ofFloat(tv, "alpha", 1f, 0f);
        fadeOut.setDuration(150);
        fadeOut.addListener(new android.animation.AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(android.animation.Animator animation) {
                tv.setText(newValue);
                tv.setTextColor(color);
            }
        });
        ObjectAnimator fadeIn = ObjectAnimator.ofFloat(tv, "alpha", 0f, 1f);
        fadeIn.setDuration(200);
        AnimatorSet set = new AnimatorSet();
        set.playSequentially(fadeOut, fadeIn);
        set.start();
    }

    // ── Ripeness card ─────────────────────────────────────────────────────────

    private void updateRipenessCard(TelemetryReading r) {
        if (binding == null) return;

        binding.cardRipeness.tvFruitEmoji.setText(selectedFruit.emoji);
        binding.cardRipeness.tvFruitName.setText(selectedFruit.displayName);

        RipenessEngine.Result result = RipenessEngine.estimate(r, selectedFruit);

        int stageColor = stageColor(result.stage);
        binding.cardRipeness.tvRipenessStage.setText(result.stageLabel);
        binding.cardRipeness.tvRipenessStage.setTextColor(requireContext().getColor(stageColor));

        binding.cardRipeness.progressRipeness.setIndicatorColor(
                requireContext().getColor(stageColor));
        ValueAnimator progressAnim = ValueAnimator.ofInt(
                binding.cardRipeness.progressRipeness.getProgress(),
                result.progressPercent);
        progressAnim.setDuration(600);
        progressAnim.addUpdateListener(a ->
                binding.cardRipeness.progressRipeness.setProgress((int) a.getAnimatedValue()));
        progressAnim.start();
        binding.cardRipeness.tvRipenessPercent.setText(result.progressPercent + "%");

        binding.cardRipeness.tvRipenessDisclaimer.setVisibility(
                result.isExperimental ? View.VISIBLE : View.GONE);
        if (r != null && r.timestamp != null) {
            binding.cardRipeness.tvRipenessTimestamp.setText(
                    getString(R.string.ripeness_as_of, DateUtils.formatShort(r.timestamp)));
        }
    }

    private int stageColor(RipenessEngine.Stage stage) {
        switch (stage) {
            case UNRIPE:   return R.color.ripeness_unripe;
            case RIPENING: return R.color.ripeness_ripening;
            case RIPE:     return R.color.ripeness_ripe;
            case OVERRIPE: return R.color.ripeness_overripe;
            default:       return R.color.ripeness_unknown;
        }
    }

    // ── Insights panel ────────────────────────────────────────────────────────

    private void updateInsights(TelemetryReading r) {
        if (binding == null) return;

        LinearLayout container = binding.cardInsights.layoutInsights;
        container.removeAllViews();

        RipenessEngine.Result result = RipenessEngine.estimate(r, selectedFruit);

        for (String insight : result.insights) {
            View row = LayoutInflater.from(requireContext())
                    .inflate(R.layout.item_insight, container, false);
            TextView tv = row.findViewById(R.id.tvInsightText);
            tv.setText(insight);
            row.startAnimation(AnimationUtils.loadAnimation(
                    requireContext(), R.anim.slide_up_fade_in));
            container.addView(row);
        }
    }

    // ── Fruit picker dialog ───────────────────────────────────────────────────

    private void showFruitPicker() {
        FruitConfig[] fruits = FruitConfig.values();
        String[] labels = new String[fruits.length];
        for (int i = 0; i < fruits.length; i++) {
            labels[i] = fruits[i].getLabel();
        }
        int currentIndex = prefs.getFruitIndex();

        new MaterialAlertDialogBuilder(requireContext())
                .setTitle("Select Crop")
                .setSingleChoiceItems(labels, currentIndex, (dialog, which) -> {
                    prefs.setFruitIndex(which);
                    selectedFruit = FruitConfig.fromIndex(which);
                    dialog.dismiss();
                    updateRipenessCard(null);
                    loadData();
                })
                .show();
    }
}
