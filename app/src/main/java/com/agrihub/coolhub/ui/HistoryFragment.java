package com.agrihub.coolhub.ui;

import android.graphics.Color;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.agrihub.coolhub.R;
import com.agrihub.coolhub.data.model.PaginatedTelemetry;
import com.agrihub.coolhub.data.model.TelemetryReading;
import com.agrihub.coolhub.data.network.AgriHubApi;
import com.agrihub.coolhub.data.network.ApiClient;
import com.agrihub.coolhub.databinding.FragmentHistoryBinding;
import com.agrihub.coolhub.databinding.ItemReadingRowBinding;
import com.agrihub.coolhub.util.DateUtils;
import com.agrihub.coolhub.util.Prefs;
import com.github.mikephil.charting.charts.LineChart;
import com.github.mikephil.charting.components.XAxis;
import com.github.mikephil.charting.data.Entry;
import com.github.mikephil.charting.data.LineData;
import com.github.mikephil.charting.data.LineDataSet;
import com.github.mikephil.charting.formatter.IndexAxisValueFormatter;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.List;
import java.util.Locale;

import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;

/**
 * History fragment — time-filtered charts and recent readings table.
 *
 * Time filters: 1h / 6h / 24h (via backend time-range query)
 * Charts: Temperature · Humidity · MQ-3 output % · MQ-135 output % · MQ-4 output %
 *
 * MQ values are displayed as normalized % (0–100 firmware scale).
 * Missing values are skipped; gaps are NOT interpolated.
 */
public class HistoryFragment extends Fragment {

    private FragmentHistoryBinding binding;
    private Prefs prefs;
    private List<TelemetryReading> readings = new ArrayList<>();
    private ReadingsAdapter adapter;
    /** Hours to fetch: 1, 6, or 24 */
    private int hoursFilter = 6;

    @Override
    public View onCreateView(@NonNull LayoutInflater inflater,
                             ViewGroup container, Bundle savedInstanceState) {
        binding = FragmentHistoryBinding.inflate(inflater, container, false);
        return binding.getRoot();
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        prefs = new Prefs(requireContext());
        hoursFilter = indexToHours(prefs.getHistoryFilter());

        // RecyclerView
        adapter = new ReadingsAdapter();
        binding.rvReadings.setLayoutManager(new LinearLayoutManager(requireContext()));
        binding.rvReadings.setAdapter(adapter);

        binding.swipeRefresh.setColorSchemeResources(R.color.green_600);
        binding.swipeRefresh.setOnRefreshListener(this::loadData);
        binding.btnRetry.setOnClickListener(v -> loadData());

        // Time filter chips
        binding.chip1h.setOnCheckedChangeListener((b, checked) -> {
            if (checked) { prefs.setHistoryFilter(0); hoursFilter = 1; loadData(); }
        });
        binding.chip6h.setOnCheckedChangeListener((b, checked) -> {
            if (checked) { prefs.setHistoryFilter(1); hoursFilter = 6; loadData(); }
        });
        binding.chip24h.setOnCheckedChangeListener((b, checked) -> {
            if (checked) { prefs.setHistoryFilter(2); hoursFilter = 24; loadData(); }
        });

        // Restore saved filter
        switch (prefs.getHistoryFilter()) {
            case 0: binding.chip1h.setChecked(true);  break;
            case 1: binding.chip6h.setChecked(true);  break;
            default: binding.chip24h.setChecked(true); break;
        }

        loadData();
    }

    private int indexToHours(int index) {
        switch (index) { case 0: return 1; case 1: return 6; default: return 24; }
    }

    // ── Data loading ──────────────────────────────────────────────────────────

    private void loadData() {
        showLoading();
        AgriHubApi api = ApiClient.create(prefs.getBackendUrl());

        long nowMs  = System.currentTimeMillis();
        long fromMs = nowMs - (long) hoursFilter * 60 * 60 * 1000L;
        // Format in UTC for backend
        SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss'Z'", Locale.US);
        sdf.setTimeZone(java.util.TimeZone.getTimeZone("UTC"));
        String fromStr = sdf.format(new Date(fromMs));

        api.getTelemetry(Prefs.DEFAULT_DEVICE, 500, 0, fromStr, null)
           .enqueue(new Callback<PaginatedTelemetry>() {
               @Override
               public void onResponse(@NonNull Call<PaginatedTelemetry> call,
                                      @NonNull Response<PaginatedTelemetry> response) {
                   if (!isAdded() || binding == null) return;
                   binding.swipeRefresh.setRefreshing(false);
                   if (response.isSuccessful() && response.body() != null) {
                       List<TelemetryReading> items = response.body().items;
                       Collections.reverse(items); // oldest first for chart
                       showContent(items);
                   } else {
                       showError("Server returned " + response.code());
                   }
               }

               @Override
               public void onFailure(@NonNull Call<PaginatedTelemetry> call, @NonNull Throwable t) {
                   if (!isAdded() || binding == null) return;
                   binding.swipeRefresh.setRefreshing(false);
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

    private void showError(String msg) {
        binding.progressBar.setVisibility(View.GONE);
        binding.swipeRefresh.setVisibility(View.GONE);
        binding.layoutError.setVisibility(View.VISIBLE);
        binding.tvErrorMessage.setText(msg != null ? msg : getString(R.string.error_network));
    }

    private void showContent(List<TelemetryReading> items) {
        binding.progressBar.setVisibility(View.GONE);
        binding.layoutError.setVisibility(View.GONE);
        binding.swipeRefresh.setVisibility(View.VISIBLE);
        readings = items;

        if (!items.isEmpty()) {
            TelemetryReading latest = items.get(items.size() - 1);
            binding.tvLatestTimestamp.setText(
                    getString(R.string.latest_reading_at, DateUtils.formatFull(latest.timestamp)));
        } else {
            binding.tvLatestTimestamp.setText(getString(R.string.no_history));
        }

        String[] timeLabels = buildTimeLabels(items);

        // Temperature (°C) — keep meaningful scale
        setupChart(binding.chartTemperature,
                buildEntries(items, "temp"), "°C",
                Color.parseColor("#FB8C00"), -10f, 60f, timeLabels);

        // Humidity (%)
        setupChart(binding.chartHumidity,
                buildEntries(items, "hum"), "%",
                Color.parseColor("#1E88E5"), 0f, 100f, timeLabels);

        // MQ sensors — normalized % (0–100 firmware scale), NOT voltage
        setupChart(binding.chartMq3,
                buildEntries(items, "mq3"), "%",
                Color.parseColor("#7B1FA2"), 0f, 100f, timeLabels);

        setupChart(binding.chartMq135,
                buildEntries(items, "mq135"), "%",
                Color.parseColor("#00897B"), 0f, 100f, timeLabels);

        setupChart(binding.chartMq4,
                buildEntries(items, "mq4"), "%",
                Color.parseColor("#2E7D32"), 0f, 100f, timeLabels);

        // Recent readings table (last 25, newest first)
        List<TelemetryReading> recent = new ArrayList<>();
        int start = Math.max(0, items.size() - 25);
        for (int i = items.size() - 1; i >= start; i--) {
            recent.add(items.get(i));
        }
        adapter.setReadings(recent);
    }

    // ── Chart helpers ─────────────────────────────────────────────────────────

    private List<Entry> buildEntries(List<TelemetryReading> items, String type) {
        List<Entry> entries = new ArrayList<>();
        for (int i = 0; i < items.size(); i++) {
            TelemetryReading r = items.get(i);
            Float val = null;
            switch (type) {
                case "temp":  if (r.temperature != null) val = r.temperature.floatValue(); break;
                case "hum":   if (r.humidity    != null) val = r.humidity.floatValue();    break;
                case "mq3":   if (r.mq3         != null) val = r.mq3.floatValue();         break;
                case "mq135": if (r.mq135       != null) val = r.mq135.floatValue();       break;
                case "mq4":   if (r.mq4         != null) val = r.mq4.floatValue();         break;
            }
            // Only add entry if value is present — avoids false zero interpolation
            if (val != null) entries.add(new Entry(i, val));
        }
        return entries;
    }

    private String[] buildTimeLabels(List<TelemetryReading> items) {
        String[] labels = new String[items.size()];
        for (int i = 0; i < items.size(); i++) {
            labels[i] = DateUtils.formatTime(items.get(i).timestamp);
        }
        return labels;
    }

    private void setupChart(LineChart chart, List<Entry> entries, String unit,
                            int color, float minY, float maxY, String[] labels) {
        chart.setNoDataText(getString(R.string.no_history));
        chart.setDrawGridBackground(false);
        chart.getDescription().setEnabled(false);
        chart.getLegend().setEnabled(false);
        chart.setTouchEnabled(true);
        chart.setDragEnabled(true);
        chart.setScaleEnabled(true);
        chart.setHighlightPerTapEnabled(true);

        XAxis xAxis = chart.getXAxis();
        xAxis.setPosition(XAxis.XAxisPosition.BOTTOM);
        xAxis.setTextSize(9f);
        xAxis.setLabelCount(Math.min(6, labels.length), true);
        xAxis.setDrawGridLines(false);
        xAxis.setTextColor(Color.parseColor("#4A6741"));
        if (labels.length > 0) {
            xAxis.setValueFormatter(new IndexAxisValueFormatter(labels));
        }

        chart.getAxisLeft().setAxisMinimum(minY);
        chart.getAxisLeft().setAxisMaximum(maxY);
        chart.getAxisLeft().setTextSize(9f);
        chart.getAxisLeft().setTextColor(Color.parseColor("#4A6741"));
        chart.getAxisLeft().setGridColor(Color.parseColor("#E8EDE8"));
        chart.getAxisRight().setEnabled(false);

        if (entries.isEmpty()) {
            chart.clear();
            return;
        }

        LineDataSet dataSet = new LineDataSet(entries, unit);
        dataSet.setColor(color);
        dataSet.setLineWidth(2.5f);
        dataSet.setDrawCircles(false);
        dataSet.setMode(LineDataSet.Mode.CUBIC_BEZIER);
        dataSet.setDrawValues(false);
        dataSet.setDrawFilled(true);
        dataSet.setFillAlpha(25);
        dataSet.setFillColor(color);
        dataSet.setHighLightColor(color);
        dataSet.setDrawHighlightIndicators(true);

        chart.setData(new LineData(dataSet));
        chart.animateX(500);
        chart.invalidate();
    }

    // ── RecyclerView adapter ──────────────────────────────────────────────────

    private static class ReadingsAdapter extends RecyclerView.Adapter<ReadingsAdapter.VH> {

        private List<TelemetryReading> data = new ArrayList<>();

        void setReadings(List<TelemetryReading> list) {
            data = list;
            notifyDataSetChanged();
        }

        @NonNull
        @Override
        public VH onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
            ItemReadingRowBinding b = ItemReadingRowBinding.inflate(
                    LayoutInflater.from(parent.getContext()), parent, false);
            return new VH(b);
        }

        @Override
        public void onBindViewHolder(@NonNull VH holder, int position) {
            holder.bind(data.get(position));
        }

        @Override
        public int getItemCount() { return data.size(); }

        static class VH extends RecyclerView.ViewHolder {
            private final ItemReadingRowBinding b;
            VH(ItemReadingRowBinding b) { super(b.getRoot()); this.b = b; }

            void bind(TelemetryReading r) {
                b.tvTimestamp.setText(DateUtils.formatShort(r.timestamp));
                b.tvTemp.setText(r.temperature != null
                        ? String.format("%.1f°C", r.temperature) : "--");
                b.tvHumidity.setText(r.humidity != null
                        ? String.format("%.1f%%", r.humidity) : "--");
            }
        }
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        binding = null;
    }
}
