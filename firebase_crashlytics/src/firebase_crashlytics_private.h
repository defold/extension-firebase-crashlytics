#if defined(DM_PLATFORM_ANDROID)

#pragma once

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
    void TestJavaCrash();
} // namespace dmFirebaseCrashlytics

#endif
