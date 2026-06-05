package com.defold.firebase.crashlytics;

import android.app.Activity;
import android.util.Log;

import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import com.google.firebase.FirebaseApp;
import com.google.firebase.crashlytics.FirebaseCrashlytics;

public class FirebaseCrashlyticsJNI {
    private static final String TAG = "FirebaseCrashlyticsJNI";
    private static final Pattern LUA_FRAME_PATTERN = Pattern.compile("^\\s*(.+):(\\d+):\\s*(.*)$");
    private static final Pattern LUA_NAMED_FUNCTION_PATTERN = Pattern.compile("in function '([^']+)'");
    private static final Pattern LUA_ANONYMOUS_FUNCTION_PATTERN = Pattern.compile("in function <(.+)>");

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

    public void recordLuaError(String message, String traceback) {
        FirebaseCrashlytics instance = getCrashlytics();
        if (instance != null) {
            if (traceback != null && traceback.length() > 0) {
                instance.log(traceback);
            }

            LuaError exception = new LuaError(safeString(message));
            StackTraceElement[] stackTrace = parseLuaStackTrace(traceback);
            exception.setStackTrace(stackTrace);
            instance.recordException(exception);
            Log.i(TAG, "Recorded LuaError non-fatal with " + stackTrace.length + " Lua stack frame(s).");
        }
    }

    public void recordLogException(String severity, String domain, String message, String signature) {
        FirebaseCrashlytics instance = getCrashlytics();
        if (instance != null) {
            String safeSeverity = safeString(severity);
            String safeDomain = safeString(domain);
            String safeSignature = safeString(signature);
            boolean fatal = "FATAL".equals(safeSeverity);
            RuntimeException exception = fatal ? new DefoldLogFatal(safeSignature) : new DefoldLogError(safeSignature);
            exception.setStackTrace(new StackTraceElement[] {
                    new StackTraceElement("dmLog", sanitizeStackSymbol(safeSignature), safeDomain.length() == 0 ? "Defold" : safeDomain, 0)
            });
            instance.recordException(exception);
            Log.i(TAG, "Recorded " + (fatal ? "DefoldLogFatal" : "DefoldLogError") + " non-fatal.");
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
            Log.e(TAG, "FirebaseApp is not initialized. Call firebase.initialize() before firebase.crashlytics.initialize().");
            return null;
        }

        crashlytics = FirebaseCrashlytics.getInstance();
        return crashlytics;
    }

    private static StackTraceElement[] parseLuaStackTrace(String traceback) {
        if (traceback == null || traceback.length() == 0) {
            return new StackTraceElement[0];
        }

        String[] lines = traceback.split("\\r?\\n");
        List<StackTraceElement> frames = new ArrayList<StackTraceElement>();
        for (String line : lines) {
            StackTraceElement frame = parseLuaStackFrame(line);
            if (frame != null) {
                frames.add(frame);
            }
        }
        return frames.toArray(new StackTraceElement[frames.size()]);
    }

    private static StackTraceElement parseLuaStackFrame(String line) {
        if (line == null) {
            return null;
        }

        String trimmed = line.trim();
        if (trimmed.length() == 0 || trimmed.contains("[C]")) {
            return null;
        }

        Matcher matcher = LUA_FRAME_PATTERN.matcher(trimmed);
        if (!matcher.matches()) {
            return null;
        }

        String file = matcher.group(1).trim();
        if (file.length() == 0) {
            return null;
        }

        int lineNumber;
        try {
            lineNumber = Integer.parseInt(matcher.group(2));
        } catch (NumberFormatException e) {
            return null;
        }

        return new StackTraceElement("lua", parseLuaSymbol(matcher.group(3)), file, lineNumber);
    }

    private static String parseLuaSymbol(String detail) {
        if (detail == null || detail.length() == 0) {
            return "lua";
        }

        Matcher namedFunction = LUA_NAMED_FUNCTION_PATTERN.matcher(detail);
        if (namedFunction.find()) {
            return namedFunction.group(1);
        }

        Matcher anonymousFunction = LUA_ANONYMOUS_FUNCTION_PATTERN.matcher(detail);
        if (anonymousFunction.find()) {
            return "anonymous";
        }

        if (detail.contains("in main chunk")) {
            return "main";
        }

        return "lua";
    }

    private static String safeString(String value) {
        return value == null ? "" : value;
    }

    private static String sanitizeStackSymbol(String value) {
        String safeValue = safeString(value);
        if (safeValue.length() == 0) {
            return "dmLog";
        }

        StringBuilder builder = new StringBuilder();
        for (int i = 0; i < safeValue.length() && builder.length() < 120; ++i) {
            char c = safeValue.charAt(i);
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
                builder.append(c);
            } else {
                builder.append('_');
            }
        }
        return builder.toString();
    }
}

class LuaError extends RuntimeException {
    LuaError(String message) {
        super(message);
    }
}

class DefoldLogError extends RuntimeException {
    DefoldLogError(String message) {
        super(message);
    }
}

class DefoldLogFatal extends RuntimeException {
    DefoldLogFatal(String message) {
        super(message);
    }
}
