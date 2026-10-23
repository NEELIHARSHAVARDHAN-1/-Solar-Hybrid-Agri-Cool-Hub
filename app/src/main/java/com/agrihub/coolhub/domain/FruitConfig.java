package com.agrihub.coolhub.domain;

/**
 * Per-fruit optimal storage thresholds and display metadata.
 *
 * Each entry defines:
 *  tempOptimalMin/Max — ideal storage temperature range (°C)
 *  humidityOptimalMin/Max — ideal relative humidity range (%)
 *  tempRipeningThreshold — temperature above which ripening accelerates
 *  humidityRipeningThreshold — humidity above which decay accelerates
 *
 * Sources: USDA postharvest guidelines and FAO cold-chain recommendations.
 * These are reference ranges; actual behaviour depends on cultivar and
 * storage duration.
 */
public enum FruitConfig {

    MANGO(
            "Mango", "🥭",
            10f, 13f,   // optimal temp °C
            85f, 95f,   // optimal humidity %
            25f, 90f    // ripening-accelerates thresholds
    ),
    BANANA(
            "Banana", "🍌",
            13f, 15f,
            85f, 95f,
            22f, 95f
    ),
    TOMATO(
            "Tomato", "🍅",
            8f, 12f,
            85f, 95f,
            28f, 95f
    ),
    APPLE(
            "Apple", "🍎",
            0f, 4f,
            90f, 95f,
            20f, 95f
    ),
    GRAPE(
            "Grape", "🍇",
            -1f, 2f,
            90f, 95f,
            20f, 95f
    );

    public final String displayName;
    public final String emoji;
    public final float tempOptimalMin;
    public final float tempOptimalMax;
    public final float humidityOptimalMin;
    public final float humidityOptimalMax;
    /** Temperature above this (°C) causes accelerated ripening */
    public final float tempRipeningThreshold;
    /** Humidity above this (%) risks decay/overripening */
    public final float humidityDecayThreshold;

    FruitConfig(String displayName, String emoji,
                float tempMin, float tempMax,
                float humMin, float humMax,
                float tempRipening, float humDecay) {
        this.displayName           = displayName;
        this.emoji                 = emoji;
        this.tempOptimalMin        = tempMin;
        this.tempOptimalMax        = tempMax;
        this.humidityOptimalMin    = humMin;
        this.humidityOptimalMax    = humMax;
        this.tempRipeningThreshold = tempRipening;
        this.humidityDecayThreshold = humDecay;
    }

    public String getLabel() {
        return emoji + " " + displayName;
    }

    /** Returns the enum from its ordinal index, defaulting to MANGO. */
    public static FruitConfig fromIndex(int index) {
        FruitConfig[] values = values();
        if (index < 0 || index >= values.length) return MANGO;
        return values[index];
    }
}
