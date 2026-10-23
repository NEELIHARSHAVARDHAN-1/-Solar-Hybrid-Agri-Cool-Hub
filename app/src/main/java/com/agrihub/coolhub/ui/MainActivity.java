package com.agrihub.coolhub.ui;

import android.os.Bundle;

import androidx.appcompat.app.AppCompatActivity;
import androidx.navigation.NavController;
import androidx.navigation.fragment.NavHostFragment;
import androidx.navigation.ui.AppBarConfiguration;
import androidx.navigation.ui.NavigationUI;

import com.agrihub.coolhub.R;
import com.agrihub.coolhub.databinding.ActivityMainBinding;
import com.agrihub.coolhub.util.Prefs;

/**
 * Main shell activity hosting the bottom navigation and NavHostFragment.
 *
 * Nielsen principles addressed:
 *  #4 Consistency and standards — persistent bottom nav with standard icons + labels
 *  #6 Recognition rather than recall — always-visible navigation bar
 */
public class MainActivity extends AppCompatActivity {

    private ActivityMainBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // Set the toolbar as the action bar
        setSupportActionBar(binding.toolbar);

        // Wire up Navigation Component with bottom nav
        NavHostFragment navHostFragment = (NavHostFragment)
                getSupportFragmentManager().findFragmentById(R.id.navHostFragment);

        if (navHostFragment != null) {
            NavController navController = navHostFragment.getNavController();

            // Top-level destinations — no "Up" button shown
            AppBarConfiguration appBarConfig = new AppBarConfiguration.Builder(
                    R.id.dashboardFragment,
                    R.id.historyFragment,
                    R.id.alertsFragment,
                    R.id.mapFragment,
                    R.id.settingsFragment
            ).build();

            NavigationUI.setupActionBarWithNavController(this, navController, appBarConfig);
            NavigationUI.setupWithNavController(binding.bottomNavView, navController);
        }
    }
}
