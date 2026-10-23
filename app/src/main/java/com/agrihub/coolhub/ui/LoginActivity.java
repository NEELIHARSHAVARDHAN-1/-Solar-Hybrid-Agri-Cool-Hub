package com.agrihub.coolhub.ui;

import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;

import androidx.appcompat.app.AppCompatActivity;

import com.agrihub.coolhub.databinding.ActivityLoginBinding;
import com.agrihub.coolhub.util.Prefs;

/**
 * FactorySense login screen.
 *
 * Authentication:
 *  Demo mode (DEMO_MODE = true) accepts the ten built-in accounts below.
 *  All demo accounts work without internet access.
 *
 *  In production, replace DEMO_MODE with a real authentication endpoint call.
 *  Never store plaintext passwords in persistent preferences or logs.
 *
 * Demo accounts (development only):
 *  farmer01 / Farm@101  — Aarav Reddy
 *  farmer02 / Farm@102  — Priya Sharma
 *  farmer03 / Farm@103  — Ravi Kumar
 *  farmer04 / Farm@104  — Ananya Patel
 *  farmer05 / Farm@105  — Suresh Naidu
 *  farmer06 / Farm@106  — Lakshmi Devi
 *  farmer07 / Farm@107  — Arjun Singh
 *  farmer08 / Farm@108  — Kavya Rao
 *  farmer09 / Farm@109  — Venkatesh Goud
 *  farmer10 / Farm@110  — Meera Nair
 */
public class LoginActivity extends AppCompatActivity {

    /**
     * Set to false to disable demo mode and use real backend authentication.
     * Do not bypass this flag in production; replace it with a build flavor variable.
     */
    private static final boolean DEMO_MODE = true;

    /**
     * Demo credential store.
     * Index 0 = User ID, Index 1 = Password, Index 2 = Display name.
     * NEVER log or persist passwords.
     */
    private static final String[][] DEMO_ACCOUNTS = {
            {"farmer01", "Farm@101", "Aarav Reddy"},
            {"farmer02", "Farm@102", "Priya Sharma"},
            {"farmer03", "Farm@103", "Ravi Kumar"},
            {"farmer04", "Farm@104", "Ananya Patel"},
            {"farmer05", "Farm@105", "Suresh Naidu"},
            {"farmer06", "Farm@106", "Lakshmi Devi"},
            {"farmer07", "Farm@107", "Arjun Singh"},
            {"farmer08", "Farm@108", "Kavya Rao"},
            {"farmer09", "Farm@109", "Venkatesh Goud"},
            {"farmer10", "Farm@110", "Meera Nair"},
    };

    private ActivityLoginBinding binding;
    private Prefs prefs;
    private boolean loginInProgress = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        binding = ActivityLoginBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());
        prefs = new Prefs(this);

        setupListeners();
    }

    private void setupListeners() {
        binding.btnLogin.setOnClickListener(v -> attemptLogin());

        binding.etPassword.setOnEditorActionListener((v, actionId, event) -> {
            if (actionId == EditorInfo.IME_ACTION_DONE
                    || (event != null && event.getKeyCode() == KeyEvent.KEYCODE_ENTER)) {
                attemptLogin();
                return true;
            }
            return false;
        });
    }

    private void attemptLogin() {
        // Prevent double-tap while a login is already in progress
        if (loginInProgress) return;

        String userId   = getText(binding.etFarmerId);
        String password = getText(binding.etPassword);

        clearErrors();

        if (TextUtils.isEmpty(userId) || TextUtils.isEmpty(password)) {
            showError(getString(com.agrihub.coolhub.R.string.login_error_empty));
            return;
        }

        loginInProgress = true;
        setLoading(true);
        hideKeyboard();

        // Brief async delay to show the loading state, then validate
        binding.getRoot().postDelayed(() -> {
            boolean valid = DEMO_MODE ? checkDemoCredentials(userId, password) : false;
            setLoading(false);
            loginInProgress = false;
            if (valid) {
                String displayName = getDemoDisplayName(userId);
                onLoginSuccess(userId, displayName);
            } else {
                showError(getString(com.agrihub.coolhub.R.string.login_error_invalid));
            }
        }, 700);
    }

    private boolean checkDemoCredentials(String id, String pw) {
        for (String[] account : DEMO_ACCOUNTS) {
            if (account[0].equals(id) && account[1].equals(pw)) return true;
        }
        return false;
    }

    private String getDemoDisplayName(String id) {
        for (String[] account : DEMO_ACCOUNTS) {
            if (account[0].equals(id)) return account[2];
        }
        return id;
    }

    private void onLoginSuccess(String userId, String displayName) {
        prefs.saveSession(userId, displayName);
        Intent intent = new Intent(this, MainActivity.class);
        startActivity(intent);
        overridePendingTransition(android.R.anim.fade_in, android.R.anim.fade_out);
        finish();
    }

    // ── UI helpers ────────────────────────────────────────────────────────────

    private void setLoading(boolean loading) {
        binding.loginProgress.setVisibility(loading ? View.VISIBLE : View.GONE);
        binding.btnLogin.setEnabled(!loading);
    }

    private void showError(String msg) {
        binding.tvLoginError.setText(msg);
        binding.tvLoginError.setVisibility(View.VISIBLE);
    }

    private void clearErrors() {
        binding.tvLoginError.setVisibility(View.GONE);
        binding.tilFarmerId.setError(null);
        binding.tilPassword.setError(null);
    }

    private String getText(android.widget.EditText et) {
        return et.getText() != null ? et.getText().toString().trim() : "";
    }

    private void hideKeyboard() {
        View view = getCurrentFocus();
        if (view != null) {
            InputMethodManager imm = (InputMethodManager)
                    getSystemService(INPUT_METHOD_SERVICE);
            imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
        }
    }
}
