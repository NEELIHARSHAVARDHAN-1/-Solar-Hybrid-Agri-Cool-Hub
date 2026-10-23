package com.agrihub.coolhub.ui;

import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.view.animation.AccelerateDecelerateInterpolator;

import androidx.appcompat.app.AppCompatActivity;

import com.agrihub.coolhub.databinding.ActivitySplashBinding;
import com.agrihub.coolhub.util.Prefs;

/**
 * Splash / Welcome screen.
 *
 * Nielsen principles addressed:
 *  #1  Visibility of system status — Lottie animation shows app is loading.
 *  #4  Consistency and standards  — familiar branded splash before any workflow.
 *
 * Flow:
 *  1. Lottie animation plays (pulsing ring).
 *  2. App name + tagline fade in with slide-up effect.
 *  3. After 2.8 s:
 *     - If already logged in  → MainActivity
 *     - Otherwise             → LoginActivity
 */
public class SplashActivity extends AppCompatActivity {

    private static final long SPLASH_DURATION_MS = 2800L;

    private ActivitySplashBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        binding = ActivitySplashBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // Start Lottie (autoPlay is set in XML, but force it here too)
        binding.lottieView.playAnimation();

        // Fade + slide-up text after 800ms
        new Handler(Looper.getMainLooper()).postDelayed(this::animateText, 800);

        // Navigate after full splash duration
        new Handler(Looper.getMainLooper()).postDelayed(this::navigate, SPLASH_DURATION_MS);
    }

    /** Fade-in + translate-Y animation for the app name and tagline. */
    private void animateText() {
        View tvAppName = binding.tvAppName;
        View tvTagline = binding.tvTagline;

        // Slide up from 40dp below
        float startY = 40f * getResources().getDisplayMetrics().density;

        tvAppName.setTranslationY(startY);
        tvTagline.setTranslationY(startY);

        ObjectAnimator fadeAppName  = ObjectAnimator.ofFloat(tvAppName, "alpha", 0f, 1f);
        ObjectAnimator slideAppName = ObjectAnimator.ofFloat(tvAppName, "translationY", startY, 0f);
        ObjectAnimator fadeTagline  = ObjectAnimator.ofFloat(tvTagline, "alpha", 0f, 1f);
        ObjectAnimator slideTagline = ObjectAnimator.ofFloat(tvTagline, "translationY", startY, 0f);

        fadeAppName.setDuration(600);
        slideAppName.setDuration(600);
        fadeTagline.setDuration(600);
        slideTagline.setDuration(600);

        fadeTagline.setStartDelay(200);
        slideTagline.setStartDelay(200);

        AccelerateDecelerateInterpolator interp = new AccelerateDecelerateInterpolator();
        fadeAppName.setInterpolator(interp);
        slideAppName.setInterpolator(interp);
        fadeTagline.setInterpolator(interp);
        slideTagline.setInterpolator(interp);

        AnimatorSet set = new AnimatorSet();
        set.playTogether(fadeAppName, slideAppName, fadeTagline, slideTagline);
        set.start();
    }

    private void navigate() {
        Prefs prefs = new Prefs(this);
        Intent intent;
        if (prefs.isLoggedIn()) {
            intent = new Intent(this, MainActivity.class);
        } else {
            intent = new Intent(this, LoginActivity.class);
        }
        startActivity(intent);
        overridePendingTransition(android.R.anim.fade_in, android.R.anim.fade_out);
        finish();
    }
}
