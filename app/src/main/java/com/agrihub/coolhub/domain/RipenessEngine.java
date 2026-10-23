package com.agrihub.coolhub.domain;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.agrihub.coolhub.data.model.TelemetryReading;

import java.util.ArrayList;
import java.util.List;

/**
 * RipenessEngine — threshold-based ripeness stage estimation.
 *
 * IMPORTANT: This is NOT an AI model or calibrated classifier.
 * All estimates are derived from temperature and humidity readings
 * compared against USDA/FAO reference storage thresholds for the
 * selected fruit type.
 *
 * The result is labelled "Experimental estimate" in the UI to be
 * transparent with the user that no trained ML model is involved.
 */
public class RipenessEngine {

    public enum Stage {
        UNKNOWN, UNRIPE, RIPENING, RIPE, OVERRIPE
    }

    public static class Result {
        public final Stage stage;
        public final String stageLabel;
        public final int progressPercent;   // 0–100 visual indicator
        public final List<String> insights; // human-readable messages
        public final boolean isExperimental;

        Result(Stage stage, String stageLabel, int progress,
               List<String> insights, boolean experimental) {
            this.stage           = stage;
            this.stageLabel      = stageLabel;
            this.progressPercent = progress;
            this.insights        = insights;
            this.isExperimental  = experimental;
        }
    }

    /**
     * Estimates ripeness stage from sensor data for the given fruit config.
     *
     * Returns a Result with stage = UNKNOWN and empty insights if
     * temperature or humidity are null.
     */
    @NonNull
    public static Result estimate(@Nullable TelemetryReading reading,
                                  @NonNull FruitConfig fruit) {
        List<String> insights = new ArrayList<>();

        if (reading == null || (reading.temperature == null && reading.humidity == null)) {
            insights.add("No sensor data available for analysis.");
            return new Result(Stage.UNKNOWN, "Classification unavailable", 0, insights, false);
        }

        double temp = reading.temperature != null ? reading.temperature : Double.NaN;
        double hum  = reading.humidity    != null ? reading.humidity    : Double.NaN;

        boolean tempAvailable = !Double.isNaN(temp);
        boolean humAvailable  = !Double.isNaN(hum);

        // ── Temp insights ──────────────────────────────────────────────
        if (tempAvailable) {
            if (temp > fruit.tempRipeningThreshold) {
                insights.add("🌡️ Temperature " + String.format("%.1f°C", temp)
                        + " exceeds ripening threshold (" + fruit.tempRipeningThreshold + "°C) for " + fruit.displayName + ".");
            } else if (temp > fruit.tempOptimalMax) {
                insights.add("🌡️ Temperature above optimal range (" + fruit.tempOptimalMin
                        + "–" + fruit.tempOptimalMax + "°C) for " + fruit.displayName + " storage.");
            } else if (temp < fruit.tempOptimalMin) {
                insights.add("❄️ Temperature below optimal range for " + fruit.displayName + " storage. Risk of chilling injury.");
            } else {
                insights.add("✅ Temperature within optimal range for " + fruit.displayName + " storage.");
            }
        }

        // ── Humidity insights ─────────────────────────────────────────
        if (humAvailable) {
            if (hum > fruit.humidityDecayThreshold) {
                insights.add("💧 Humidity " + String.format("%.1f%%", hum)
                        + " is very high — risk of mould/decay on " + fruit.displayName + ".");
            } else if (hum > fruit.humidityOptimalMax) {
                insights.add("💧 Humidity above optimal range for " + fruit.displayName + " storage.");
            } else if (hum < fruit.humidityOptimalMin) {
                insights.add("🏜️ Humidity below optimal range — " + fruit.displayName + " may dehydrate.");
            } else {
                insights.add("✅ Humidity within optimal range for " + fruit.displayName + " storage.");
            }
        }

        // ── Gas sensor note (0–100 firmware scale) ─────────────────────
        // MQ-3 warning at 20% of firmware range (backend default warning threshold)
        if (reading.mq3 != null && reading.mq3 > 20.0) {
            insights.add("⚠️ MQ-3 normalized output elevated ("
                    + String.format("%.1f%%", reading.mq3)
                    + "). Check storage ventilation.");
        }

        // ── Stage determination ────────────────────────────────────────
        // Priority: temperature deviation dominates; humidity is secondary.
        Stage stage;
        int progress;

        if (!tempAvailable) {
            stage    = Stage.UNKNOWN;
            progress = 0;
        } else if (temp <= fruit.tempOptimalMax && temp >= fruit.tempOptimalMin
                && (!humAvailable || (hum >= fruit.humidityOptimalMin && hum <= fruit.humidityOptimalMax))) {
            // Within ideal range — classified as Unripe (good cold storage)
            stage    = Stage.UNRIPE;
            progress = 10;
        } else if (temp > fruit.tempOptimalMax && temp <= fruit.tempRipeningThreshold) {
            stage    = Stage.RIPENING;
            progress = 45;
        } else if (temp > fruit.tempRipeningThreshold && temp <= fruit.tempRipeningThreshold + 5) {
            stage    = Stage.RIPE;
            progress = 75;
        } else if (temp > fruit.tempRipeningThreshold + 5) {
            stage    = Stage.OVERRIPE;
            progress = 95;
        } else {
            stage    = Stage.UNRIPE;
            progress = 10;
        }

        String stageLabel = stageName(stage);
        insights.add("ℹ️ Threshold-based estimate only — no AI model connected.");

        return new Result(stage, stageLabel, progress, insights, true);
    }

    private static String stageName(Stage s) {
        switch (s) {
            case UNRIPE:   return "Unripe";
            case RIPENING: return "Ripening";
            case RIPE:     return "Ripe";
            case OVERRIPE: return "Overripe";
            default:       return "Classification unavailable";
        }
    }
}
