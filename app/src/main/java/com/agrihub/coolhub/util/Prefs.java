package com.agrihub.coolhub.util;

import android.content.Context;
import android.content.SharedPreferences;

/**
 * Centralised SharedPreferences access.
 *
 * Keys stored:
 *  - backend_url     : backend server base URL
 *  - farmer_id       : logged-in user ID
 *  - display_name    : logged-in user display name
 *  - is_logged_in    : boolean session flag
 *  - fruit_index     : selected FruitConfig ordinal (0 = Mango default)
 *  - history_filter  : last selected history time filter (0=1h, 1=6h, 2=24h)
 */
public class Prefs {

    private static final String PREF_FILE       = "agrihub_prefs";
    private static final String KEY_BACKEND      = "backend_url";
    private static final String KEY_FARMER       = "farmer_id";
    private static final String KEY_DISPLAY_NAME = "display_name";
    private static final String KEY_LOGGED       = "is_logged_in";
    private static final String KEY_FRUIT        = "fruit_index";
    private static final String KEY_HIST_FILTER  = "history_filter";

    public static final String DEFAULT_BACKEND = "http://10.0.2.2:8000";
    public static final String DEFAULT_DEVICE  = "agrihub-001";

    private final SharedPreferences sp;

    public Prefs(Context context) {
        sp = context.getApplicationContext()
                    .getSharedPreferences(PREF_FILE, Context.MODE_PRIVATE);
    }

    // ── backend URL ─────────────────────────────────────────────────────

    public String getBackendUrl() {
        return sp.getString(KEY_BACKEND, DEFAULT_BACKEND);
    }

    public void setBackendUrl(String url) {
        sp.edit().putString(KEY_BACKEND, url).apply();
    }

    // ── session ──────────────────────────────────────────────────────────

    public boolean isLoggedIn() {
        return sp.getBoolean(KEY_LOGGED, false);
    }

    public String getFarmerId() {
        return sp.getString(KEY_FARMER, "");
    }

    public String getDisplayName() {
        return sp.getString(KEY_DISPLAY_NAME, getFarmerId());
    }

    public void saveSession(String farmerId, String displayName) {
        sp.edit()
          .putBoolean(KEY_LOGGED, true)
          .putString(KEY_FARMER, farmerId)
          .putString(KEY_DISPLAY_NAME, displayName != null ? displayName : farmerId)
          .apply();
    }

    /** Legacy overload — display name defaults to farmerId. */
    public void saveSession(String farmerId) {
        saveSession(farmerId, farmerId);
    }

    public void clearSession() {
        sp.edit()
          .putBoolean(KEY_LOGGED, false)
          .remove(KEY_FARMER)
          .remove(KEY_DISPLAY_NAME)
          .apply();
    }

    // ── fruit selection ──────────────────────────────────────────────────

    /** Returns selected FruitConfig ordinal index (default 0 = MANGO). */
    public int getFruitIndex() {
        return sp.getInt(KEY_FRUIT, 0);
    }

    public void setFruitIndex(int index) {
        sp.edit().putInt(KEY_FRUIT, index).apply();
    }

    // ── history filter ───────────────────────────────────────────────────

    /** Returns 0=1h, 1=6h, 2=24h (default 2). */
    public int getHistoryFilter() {
        return sp.getInt(KEY_HIST_FILTER, 2);
    }

    public void setHistoryFilter(int filter) {
        sp.edit().putInt(KEY_HIST_FILTER, filter).apply();
    }
}
