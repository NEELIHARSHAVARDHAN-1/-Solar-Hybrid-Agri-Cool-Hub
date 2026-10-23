package com.agrihub.coolhub.ui;

import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import com.agrihub.coolhub.R;
import com.agrihub.coolhub.databinding.FragmentSettingsBinding;
import com.agrihub.coolhub.domain.FruitConfig;
import com.agrihub.coolhub.util.Prefs;
import com.google.android.material.snackbar.Snackbar;

/**
 * Settings fragment — configure backend URL, select fruit type, and logout.
 *
 * Nielsen principles:
 *  #3  User control and freedom     — logout, change fruit, change URL
 *  #4  Consistency                  — same TextInputLayout style as login
 *  #7  Flexibility and efficiency   — persists all settings across launches
 *  #10 Help and documentation       — threshold summary + About section
 */
public class SettingsFragment extends Fragment {

    private FragmentSettingsBinding binding;
    private Prefs prefs;

    @Override
    public View onCreateView(@NonNull LayoutInflater inflater,
                             ViewGroup container, Bundle savedInstanceState) {
        binding = FragmentSettingsBinding.inflate(inflater, container, false);
        return binding.getRoot();
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        prefs = new Prefs(requireContext());

        // ── Session info ─────────────────────────────────────────────
        binding.tvLoggedInAs.setText(
                getString(R.string.label_logged_in_as, prefs.getFarmerId()));

        // Show display name if available and different from ID
        String displayName = prefs.getDisplayName();
        if (!displayName.equals(prefs.getFarmerId())) {
            binding.tvUserDisplayName.setText(getString(R.string.label_user_name, displayName));
            binding.tvUserDisplayName.setVisibility(android.view.View.VISIBLE);
        } else {
            binding.tvUserDisplayName.setVisibility(android.view.View.GONE);
        }

        // ── Backend URL ──────────────────────────────────────────────
        if (binding.etBackendUrl.getText() == null
                || binding.etBackendUrl.getText().toString().isEmpty()) {
            binding.etBackendUrl.setText(prefs.getBackendUrl());
        }

        // ── Fruit selection chips ────────────────────────────────────
        restoreFruitChip(prefs.getFruitIndex());

        binding.chipGroupFruit.setOnCheckedStateChangeListener((group, checkedIds) -> {
            if (checkedIds.isEmpty()) return;
            int index = chipIdToIndex(checkedIds.get(0));
            prefs.setFruitIndex(index);
            FruitConfig fruit = FruitConfig.fromIndex(index);
            binding.tvSelectedFruit.setText(fruit.getLabel());
            updateThresholdSummary(fruit);
        });

        // Initialise threshold summary for current fruit
        updateThresholdSummary(FruitConfig.fromIndex(prefs.getFruitIndex()));

        // ── Button listeners ─────────────────────────────────────────
        binding.btnSave.setOnClickListener(v -> saveUrl());
        binding.btnLogout.setOnClickListener(v -> logout());
    }

    // ── Fruit chip helpers ────────────────────────────────────────────────────

    private void restoreFruitChip(int fruitIndex) {
        switch (fruitIndex) {
            case 1: binding.chipBanana.setChecked(true); break;
            case 2: binding.chipTomato.setChecked(true); break;
            case 3: binding.chipApple.setChecked(true);  break;
            case 4: binding.chipGrape.setChecked(true);  break;
            default: binding.chipMango.setChecked(true); break;
        }
        binding.tvSelectedFruit.setText(FruitConfig.fromIndex(fruitIndex).getLabel());
    }

    private int chipIdToIndex(int chipId) {
        if (chipId == R.id.chipBanana) return 1;
        if (chipId == R.id.chipTomato) return 2;
        if (chipId == R.id.chipApple)  return 3;
        if (chipId == R.id.chipGrape)  return 4;
        return 0; // Mango default
    }

    private void updateThresholdSummary(FruitConfig fruit) {
        if (binding == null) return;
        String summary = "📦 Optimal storage for " + fruit.displayName + ":\n"
                + "🌡️ Temperature: " + fruit.tempOptimalMin + " – " + fruit.tempOptimalMax + " °C\n"
                + "💧 Humidity: " + fruit.humidityOptimalMin + " – " + fruit.humidityOptimalMax + " %\n"
                + "⚡ Ripening accelerates above " + fruit.tempRipeningThreshold + " °C";
        binding.tvFruitThresholds.setText(summary);
    }

    // ── Save URL ──────────────────────────────────────────────────────────────

    private void saveUrl() {
        String url = binding.etBackendUrl.getText() != null
                ? binding.etBackendUrl.getText().toString().trim() : "";
        if (TextUtils.isEmpty(url)) {
            binding.tilBackendUrl.setError("URL cannot be empty");
            return;
        }
        binding.tilBackendUrl.setError(null);
        prefs.setBackendUrl(url);
        Snackbar.make(binding.getRoot(),
                getString(R.string.saved_confirmation), Snackbar.LENGTH_SHORT).show();
    }

    // ── Logout ────────────────────────────────────────────────────────────────

    private void logout() {
        prefs.clearSession();
        Intent intent = new Intent(requireContext(), LoginActivity.class);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
        startActivity(intent);
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        binding = null;
    }
}
