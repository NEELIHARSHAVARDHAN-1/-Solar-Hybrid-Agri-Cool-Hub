package com.agrihub.coolhub.ui;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.viewpager2.adapter.FragmentStateAdapter;

import com.agrihub.coolhub.R;
import com.agrihub.coolhub.data.model.Alert;
import com.agrihub.coolhub.data.model.PaginatedAlerts;
import com.agrihub.coolhub.data.network.AgriHubApi;
import com.agrihub.coolhub.data.network.ApiClient;
import com.agrihub.coolhub.databinding.FragmentAlertsBinding;
import com.agrihub.coolhub.databinding.FragmentAlertsListBinding;
import com.agrihub.coolhub.databinding.ItemAlertBinding;
import com.agrihub.coolhub.util.DateUtils;
import com.agrihub.coolhub.util.Prefs;
import com.google.android.material.tabs.TabLayoutMediator;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;

/**
 * Alerts fragment — tabbed active/all view with severity colors and deduplication.
 *
 * Deduplication strategy:
 *  - Backend provides stable integer IDs for each alert.
 *  - Alerts are stored in a LinkedHashMap keyed by ID so re-fetching
 *    replaces existing entries instead of appending duplicates.
 *  - Tab counts are recalculated after each upsert.
 *
 * Severity visual treatment:
 *  - Critical → red (#C62828)
 *  - Warning  → amber (#F57F17)
 *  - Resolved → green (#388E3C)
 */
public class AlertsFragment extends Fragment {

    private FragmentAlertsBinding binding;

    /** Dedup maps: key = alert ID, value = Alert */
    private final LinkedHashMap<Integer, Alert> activeMap = new LinkedHashMap<>();
    private final LinkedHashMap<Integer, Alert> allMap    = new LinkedHashMap<>();

    @Override
    public View onCreateView(@NonNull LayoutInflater inflater,
                             ViewGroup container, Bundle savedInstanceState) {
        binding = FragmentAlertsBinding.inflate(inflater, container, false);
        return binding.getRoot();
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        binding.btnRetry.setOnClickListener(v -> loadData());
        loadData();
    }

    // ── Data loading ─────────────────────────────────────────────────────────

    private void loadData() {
        showLoading();
        Prefs prefs = new Prefs(requireContext());
        AgriHubApi api = ApiClient.create(prefs.getBackendUrl());

        // Load active alerts
        api.getActiveAlerts(Prefs.DEFAULT_DEVICE).enqueue(new Callback<PaginatedAlerts>() {
            @Override
            public void onResponse(@NonNull Call<PaginatedAlerts> call,
                                   @NonNull Response<PaginatedAlerts> response) {
                if (!isAdded()) return;
                if (response.isSuccessful() && response.body() != null) {
                    upsertAlerts(activeMap, response.body().items);
                    loadAllAlerts(api);
                } else {
                    showError("Server returned " + response.code());
                }
            }

            @Override
            public void onFailure(@NonNull Call<PaginatedAlerts> call, @NonNull Throwable t) {
                if (!isAdded()) return;
                showError(t.getMessage());
            }
        });
    }

    private void loadAllAlerts(AgriHubApi api) {
        // null = no filter → all levels; null resolved = all statuses
        api.getAlerts(Prefs.DEFAULT_DEVICE, 100, 0, null, null)
           .enqueue(new Callback<PaginatedAlerts>() {
               @Override
               public void onResponse(@NonNull Call<PaginatedAlerts> call,
                                      @NonNull Response<PaginatedAlerts> response) {
                   if (!isAdded()) return;
                   if (response.isSuccessful() && response.body() != null) {
                       upsertAlerts(allMap, response.body().items);
                       showContent();
                   } else {
                       showError("Server returned " + response.code());
                   }
               }

               @Override
               public void onFailure(@NonNull Call<PaginatedAlerts> call, @NonNull Throwable t) {
                   if (!isAdded()) return;
                   showError(t.getMessage());
               }
           });
    }

    /**
     * Upsert alerts into a dedup map by ID.
     * Existing entries are replaced; new entries are added.
     * This prevents duplicate list rows when the same alert is returned by re-polling.
     */
    private void upsertAlerts(LinkedHashMap<Integer, Alert> map, List<Alert> incoming) {
        if (incoming == null) return;
        for (Alert a : incoming) {
            map.put(a.id, a);
        }
    }

    // ── UI state management ───────────────────────────────────────────────────

    private void showLoading() {
        binding.progressBar.setVisibility(View.VISIBLE);
        binding.viewPager.setVisibility(View.GONE);
        binding.layoutError.setVisibility(View.GONE);
    }

    private void showError(String msg) {
        binding.progressBar.setVisibility(View.GONE);
        binding.viewPager.setVisibility(View.GONE);
        binding.layoutError.setVisibility(View.VISIBLE);
        binding.tvErrorMessage.setText(msg != null ? msg
                : getString(R.string.error_network));
    }

    private void showContent() {
        binding.progressBar.setVisibility(View.GONE);
        binding.layoutError.setVisibility(View.GONE);
        binding.viewPager.setVisibility(View.VISIBLE);

        binding.viewPager.setAdapter(new AlertPagerAdapter(this));
        new TabLayoutMediator(binding.tabLayout, binding.viewPager,
                (tab, position) -> {
                    if (position == 0) {
                        tab.setText("Active (" + activeMap.size() + ")");
                    } else {
                        tab.setText("All (" + allMap.size() + ")");
                    }
                }).attach();
    }

    // ── ViewPager2 adapter ────────────────────────────────────────────────────

    private class AlertPagerAdapter extends FragmentStateAdapter {
        AlertPagerAdapter(Fragment f) { super(f); }

        @Override
        public int getItemCount() { return 2; }

        @NonNull
        @Override
        public Fragment createFragment(int position) {
            AlertListFragment frag = new AlertListFragment();
            Bundle args = new Bundle();
            args.putInt("position", position);
            frag.setArguments(args);
            List<Alert> list = position == 0
                    ? new ArrayList<>(activeMap.values())
                    : new ArrayList<>(allMap.values());
            frag.setAlerts(list, position == 0);
            frag.setParent(AlertsFragment.this);
            return frag;
        }
    }

    void reloadAfterResolve() {
        activeMap.clear();
        allMap.clear();
        loadData();
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        binding = null;
    }

    // ── Inner Fragment for each tab's list ────────────────────────────────────

    public static class AlertListFragment extends Fragment {

        private List<Alert> alerts = new ArrayList<>();
        private boolean showResolve = false;
        private AlertsFragment parent;

        void setAlerts(List<Alert> list, boolean showResolve) {
            this.alerts = list;
            this.showResolve = showResolve;
        }

        void setParent(AlertsFragment p) { parent = p; }

        @Override
        public View onCreateView(@NonNull LayoutInflater inflater,
                                 ViewGroup container, Bundle savedInstanceState) {
            FragmentAlertsListBinding b =
                    FragmentAlertsListBinding.inflate(inflater, container, false);

            if (alerts.isEmpty()) {
                b.tvEmpty.setVisibility(View.VISIBLE);
                b.rvAlerts.setVisibility(View.GONE);
            } else {
                b.tvEmpty.setVisibility(View.GONE);
                b.rvAlerts.setVisibility(View.VISIBLE);
                AlertAdapter adapter = new AlertAdapter(alerts, showResolve, alertId -> {
                    if (!isAdded()) return;
                    Prefs prefs = new Prefs(requireContext());
                    ApiClient.create(prefs.getBackendUrl())
                             .resolveAlert(alertId)
                             .enqueue(new Callback<Alert>() {
                                 @Override
                                 public void onResponse(@NonNull Call<Alert> call,
                                                        @NonNull Response<Alert> resp) {
                                     if (parent != null) parent.reloadAfterResolve();
                                 }
                                 @Override
                                 public void onFailure(@NonNull Call<Alert> c, @NonNull Throwable t) { }
                             });
                });
                b.rvAlerts.setLayoutManager(new LinearLayoutManager(requireContext()));
                b.rvAlerts.setAdapter(adapter);
            }
            return b.getRoot();
        }
    }

    // ── Alert RecyclerView adapter ────────────────────────────────────────────

    interface OnResolveListener { void resolve(int alertId); }

    private static class AlertAdapter extends RecyclerView.Adapter<AlertAdapter.VH> {

        private final List<Alert> data;
        private final boolean showResolve;
        private final OnResolveListener listener;

        AlertAdapter(List<Alert> data, boolean showResolve, OnResolveListener listener) {
            this.data        = data;
            this.showResolve = showResolve;
            this.listener    = listener;
        }

        @NonNull
        @Override
        public VH onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
            ItemAlertBinding b = ItemAlertBinding.inflate(
                    LayoutInflater.from(parent.getContext()), parent, false);
            return new VH(b);
        }

        @Override
        public void onBindViewHolder(@NonNull VH h, int position) {
            h.bind(data.get(position), showResolve, listener);
        }

        @Override
        public int getItemCount() { return data.size(); }

        static class VH extends RecyclerView.ViewHolder {
            private final ItemAlertBinding b;

            VH(ItemAlertBinding b) {
                super(b.getRoot());
                this.b = b;
            }

            void bind(Alert alert, boolean showResolve, OnResolveListener listener) {
                b.tvAlertType.setText(alert.getDisplayLabel());
                b.tvAlertValue.setText(
                        String.format("Value: %.1f  |  Threshold: %.1f",
                                alert.value, alert.threshold));
                b.tvAlertTimestamp.setText(DateUtils.formatShort(alert.timestamp));

                boolean active = alert.isActive();

                // Severity color: critical = red, warning = amber, resolved = green
                int iconColor;
                if (!active) {
                    iconColor = b.getRoot().getContext().getColor(R.color.green_700);
                } else if (alert.isCritical()) {
                    iconColor = b.getRoot().getContext().getColor(R.color.red_700);
                } else {
                    iconColor = b.getRoot().getContext().getColor(R.color.amber_700);
                }
                b.tvAlertType.setTextColor(iconColor);
                b.ivAlertIcon.setColorFilter(iconColor);

                // Severity badge
                if (alert.alertLevel != null) {
                    b.tvSeverityBadge.setText(alert.alertLevel.toUpperCase());
                    b.tvSeverityBadge.setTextColor(iconColor);
                    b.tvSeverityBadge.setVisibility(View.VISIBLE);
                } else {
                    b.tvSeverityBadge.setVisibility(View.GONE);
                }

                // Resolved timestamp
                if (!active && alert.resolvedAt != null) {
                    b.tvAlertResolved.setVisibility(View.VISIBLE);
                    b.tvAlertResolved.setText(
                            "Resolved: " + DateUtils.formatShort(alert.resolvedAt));
                } else {
                    b.tvAlertResolved.setVisibility(View.GONE);
                }

                // Resolve button (active alerts only on active tab)
                if (showResolve && active) {
                    b.btnResolve.setVisibility(View.VISIBLE);
                    b.btnResolve.setOnClickListener(v -> listener.resolve(alert.id));
                } else {
                    b.btnResolve.setVisibility(View.GONE);
                }
            }
        }
    }
}
