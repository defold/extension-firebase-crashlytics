#pragma once

#if defined(DM_PLATFORM_ANDROID) || defined(DM_PLATFORM_IOS) || defined(DM_PLATFORM_OSX)
#define DM_FIREBASE_CRASHLYTICS_SUPPORTED 1

namespace dmFirebaseCrashlytics {
    void Initialize_Ext();
    bool Initialize();
    void SetCrashlyticsCollectionEnabled(bool enabled);
    bool IsCrashlyticsCollectionEnabled();
    bool DidCrashOnPreviousExecution();
    void SendUnsentReports();
    void DeleteUnsentReports();
    void SetUserId(const char* user_id);
    void SetCustomKeyString(const char* key, const char* value);
    void SetCustomKeyBool(const char* key, bool value);
    void SetCustomKeyNumber(const char* key, double value);
    void Log(const char* message);
    void RecordException(const char* message);
    void RecordLuaError(const char* message, const char* traceback);
    void RecordLogException(const char* severity, const char* domain, const char* message, const char* signature);
#if defined(DM_PLATFORM_ANDROID)
    void TestJavaCrash();
#endif
} // namespace dmFirebaseCrashlytics

#endif
