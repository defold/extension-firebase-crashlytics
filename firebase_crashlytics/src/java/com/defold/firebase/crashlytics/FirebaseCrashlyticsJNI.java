package com.defold.firebase.crashlytics;

import android.app.Activity;
import android.util.Log;

import com.google.firebase.FirebaseApp;
import com.google.firebase.crashlytics.FirebaseCrashlytics;

public class FirebaseCrashlyticsJNI {
    private static final String TAG = "FirebaseCrashlyticsJNI";

    private final Activity activity;
    private FirebaseCrashlytics crashlytics;

    public FirebaseCrashlyticsJNI(Activity activity) {
        this.activity = activity;
    }

    public boolean initialize() {
        return getCrashlytics() != null;
    }

    public void setCrashlyticsCollectionEnabled(boolean enabled) {
        FirebaseCrashlytics instance = getCrashlytics();
        if (instance != null) {
            instance.setCrashlyticsCollectionEnabled(enabled);
        }
    }

    public boolean isCrashlyticsCollectionEnabled() {
        FirebaseCrashlytics instance = getCrashlytics();
        return instance != null && instance.isCrashlyticsCollectionEnabled();
    }

    public boolean didCrashOnPreviousExecution() {
        FirebaseCrashlytics instance = getCrashlytics();
        return instance != null && instance.didCrashOnPreviousExecution();
    }

    public void sendUnsentReports() {
        FirebaseCrashlytics instance = getCrashlytics();
        if (instance != null) {
            instance.sendUnsentReports();
        }
    }

    public void deleteUnsentReports() {
        FirebaseCrashlytics instance = getCrashlytics();
        if (instance != null) {
            instance.deleteUnsentReports();
        }
    }

    public void setUserId(String userId) {
        FirebaseCrashlytics instance = getCrashlytics();
        if (instance != null) {
            instance.setUserId(userId);
        }
    }

    public void setCustomKeyString(String key, String value) {
        FirebaseCrashlytics instance = getCrashlytics();
        if (instance != null) {
            instance.setCustomKey(key, value);
        }
    }

    public void setCustomKeyBoolean(String key, boolean value) {
        FirebaseCrashlytics instance = getCrashlytics();
        if (instance != null) {
            instance.setCustomKey(key, value);
        }
    }

    public void setCustomKeyDouble(String key, double value) {
        FirebaseCrashlytics instance = getCrashlytics();
        if (instance != null) {
            instance.setCustomKey(key, value);
        }
    }

    public void log(String message) {
        FirebaseCrashlytics instance = getCrashlytics();
        if (instance != null) {
            instance.log(message);
        }
    }

    public void recordException(String message) {
        FirebaseCrashlytics instance = getCrashlytics();
        if (instance != null) {
            instance.recordException(new RuntimeException(message));
        }
    }

    public void testJavaCrash() {
        getCrashlytics();
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                throw new RuntimeException("Firebase Crashlytics Java test crash");
            }
        });
    }

    private FirebaseCrashlytics getCrashlytics() {
        if (crashlytics != null) {
            return crashlytics;
        }

        if (FirebaseApp.getApps(activity.getApplicationContext()).isEmpty()) {
            Log.e(TAG, "FirebaseApp is not initialized. Call firebase.initialize() first or provide google-services.xml resources for automatic initialization.");
            return null;
        }

        crashlytics = FirebaseCrashlytics.getInstance();
        return crashlytics;
    }
}
