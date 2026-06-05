#if defined(DM_PLATFORM_ANDROID)

#include <dmsdk/dlib/android.h>
#include "firebase_crashlytics_private.h"

namespace dmFirebaseCrashlytics {

    struct FirebaseCrashlyticsJNI
    {
        jobject   m_JNI;
        jmethodID m_Initialize;
        jmethodID m_SetCrashlyticsCollectionEnabled;
        jmethodID m_IsCrashlyticsCollectionEnabled;
        jmethodID m_DidCrashOnPreviousExecution;
        jmethodID m_SendUnsentReports;
        jmethodID m_DeleteUnsentReports;
        jmethodID m_SetUserId;
        jmethodID m_SetCustomKeyString;
        jmethodID m_SetCustomKeyBoolean;
        jmethodID m_SetCustomKeyDouble;
        jmethodID m_Log;
        jmethodID m_RecordException;
        jmethodID m_RecordLuaError;
        jmethodID m_RecordLogException;
        jmethodID m_TestJavaCrash;
    };

    static FirebaseCrashlyticsJNI g_firebase_crashlytics;

    static void CallVoidMethod(jobject instance, jmethodID method)
    {
        dmAndroid::ThreadAttacher thread_attacher;
        JNIEnv* env = thread_attacher.GetEnv();
        env->CallVoidMethod(instance, method);
    }

    static bool CallBooleanMethod(jobject instance, jmethodID method)
    {
        dmAndroid::ThreadAttacher thread_attacher;
        JNIEnv* env = thread_attacher.GetEnv();
        return env->CallBooleanMethod(instance, method) == JNI_TRUE;
    }

    static void CallVoidMethodBool(jobject instance, jmethodID method, bool value)
    {
        dmAndroid::ThreadAttacher thread_attacher;
        JNIEnv* env = thread_attacher.GetEnv();
        env->CallVoidMethod(instance, method, value ? JNI_TRUE : JNI_FALSE);
    }

    static void CallVoidMethodChar(jobject instance, jmethodID method, const char* cstr)
    {
        dmAndroid::ThreadAttacher thread_attacher;
        JNIEnv* env = thread_attacher.GetEnv();

        jstring jstr = env->NewStringUTF(cstr);
        env->CallVoidMethod(instance, method, jstr);
        env->DeleteLocalRef(jstr);
    }

    static void CallVoidMethodCharChar(jobject instance, jmethodID method, const char* cstr0, const char* cstr1)
    {
        dmAndroid::ThreadAttacher thread_attacher;
        JNIEnv* env = thread_attacher.GetEnv();

        jstring jstr0 = env->NewStringUTF(cstr0);
        jstring jstr1 = env->NewStringUTF(cstr1);
        env->CallVoidMethod(instance, method, jstr0, jstr1);
        env->DeleteLocalRef(jstr0);
        env->DeleteLocalRef(jstr1);
    }

    static void CallVoidMethodCharCharCharChar(jobject instance, jmethodID method, const char* cstr0, const char* cstr1, const char* cstr2, const char* cstr3)
    {
        dmAndroid::ThreadAttacher thread_attacher;
        JNIEnv* env = thread_attacher.GetEnv();

        jstring jstr0 = env->NewStringUTF(cstr0);
        jstring jstr1 = env->NewStringUTF(cstr1);
        jstring jstr2 = env->NewStringUTF(cstr2);
        jstring jstr3 = env->NewStringUTF(cstr3);
        env->CallVoidMethod(instance, method, jstr0, jstr1, jstr2, jstr3);
        env->DeleteLocalRef(jstr0);
        env->DeleteLocalRef(jstr1);
        env->DeleteLocalRef(jstr2);
        env->DeleteLocalRef(jstr3);
    }

    static void CallVoidMethodCharBool(jobject instance, jmethodID method, const char* cstr, bool value)
    {
        dmAndroid::ThreadAttacher thread_attacher;
        JNIEnv* env = thread_attacher.GetEnv();

        jstring jstr = env->NewStringUTF(cstr);
        env->CallVoidMethod(instance, method, jstr, value ? JNI_TRUE : JNI_FALSE);
        env->DeleteLocalRef(jstr);
    }

    static void CallVoidMethodCharDouble(jobject instance, jmethodID method, const char* cstr, double value)
    {
        dmAndroid::ThreadAttacher thread_attacher;
        JNIEnv* env = thread_attacher.GetEnv();

        jstring jstr = env->NewStringUTF(cstr);
        env->CallVoidMethod(instance, method, jstr, value);
        env->DeleteLocalRef(jstr);
    }

    static void InitJNIMethods(JNIEnv* env, jclass cls)
    {
        g_firebase_crashlytics.m_Initialize = env->GetMethodID(cls, "initialize", "()Z");
        g_firebase_crashlytics.m_SetCrashlyticsCollectionEnabled = env->GetMethodID(cls, "setCrashlyticsCollectionEnabled", "(Z)V");
        g_firebase_crashlytics.m_IsCrashlyticsCollectionEnabled = env->GetMethodID(cls, "isCrashlyticsCollectionEnabled", "()Z");
        g_firebase_crashlytics.m_DidCrashOnPreviousExecution = env->GetMethodID(cls, "didCrashOnPreviousExecution", "()Z");
        g_firebase_crashlytics.m_SendUnsentReports = env->GetMethodID(cls, "sendUnsentReports", "()V");
        g_firebase_crashlytics.m_DeleteUnsentReports = env->GetMethodID(cls, "deleteUnsentReports", "()V");
        g_firebase_crashlytics.m_SetUserId = env->GetMethodID(cls, "setUserId", "(Ljava/lang/String;)V");
        g_firebase_crashlytics.m_SetCustomKeyString = env->GetMethodID(cls, "setCustomKeyString", "(Ljava/lang/String;Ljava/lang/String;)V");
        g_firebase_crashlytics.m_SetCustomKeyBoolean = env->GetMethodID(cls, "setCustomKeyBoolean", "(Ljava/lang/String;Z)V");
        g_firebase_crashlytics.m_SetCustomKeyDouble = env->GetMethodID(cls, "setCustomKeyDouble", "(Ljava/lang/String;D)V");
        g_firebase_crashlytics.m_Log = env->GetMethodID(cls, "log", "(Ljava/lang/String;)V");
        g_firebase_crashlytics.m_RecordException = env->GetMethodID(cls, "recordException", "(Ljava/lang/String;)V");
        g_firebase_crashlytics.m_RecordLuaError = env->GetMethodID(cls, "recordLuaError", "(Ljava/lang/String;Ljava/lang/String;)V");
        g_firebase_crashlytics.m_RecordLogException = env->GetMethodID(cls, "recordLogException", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
        g_firebase_crashlytics.m_TestJavaCrash = env->GetMethodID(cls, "testJavaCrash", "()V");
    }

    void Initialize_Ext()
    {
        dmAndroid::ThreadAttacher thread_attacher;
        JNIEnv* env = thread_attacher.GetEnv();
        jclass cls = dmAndroid::LoadClass(env, "com.defold.firebase.crashlytics.FirebaseCrashlyticsJNI");

        InitJNIMethods(env, cls);

        jmethodID jni_constructor = env->GetMethodID(cls, "<init>", "(Landroid/app/Activity;)V");
        g_firebase_crashlytics.m_JNI = env->NewGlobalRef(env->NewObject(cls, jni_constructor, thread_attacher.GetActivity()->clazz));
    }

    bool Initialize()
    {
        return CallBooleanMethod(g_firebase_crashlytics.m_JNI, g_firebase_crashlytics.m_Initialize);
    }

    void SetCrashlyticsCollectionEnabled(bool enabled)
    {
        CallVoidMethodBool(g_firebase_crashlytics.m_JNI, g_firebase_crashlytics.m_SetCrashlyticsCollectionEnabled, enabled);
    }

    bool IsCrashlyticsCollectionEnabled()
    {
        return CallBooleanMethod(g_firebase_crashlytics.m_JNI, g_firebase_crashlytics.m_IsCrashlyticsCollectionEnabled);
    }

    bool DidCrashOnPreviousExecution()
    {
        return CallBooleanMethod(g_firebase_crashlytics.m_JNI, g_firebase_crashlytics.m_DidCrashOnPreviousExecution);
    }

    void SendUnsentReports()
    {
        CallVoidMethod(g_firebase_crashlytics.m_JNI, g_firebase_crashlytics.m_SendUnsentReports);
    }

    void DeleteUnsentReports()
    {
        CallVoidMethod(g_firebase_crashlytics.m_JNI, g_firebase_crashlytics.m_DeleteUnsentReports);
    }

    void SetUserId(const char* user_id)
    {
        CallVoidMethodChar(g_firebase_crashlytics.m_JNI, g_firebase_crashlytics.m_SetUserId, user_id);
    }

    void SetCustomKeyString(const char* key, const char* value)
    {
        CallVoidMethodCharChar(g_firebase_crashlytics.m_JNI, g_firebase_crashlytics.m_SetCustomKeyString, key, value);
    }

    void SetCustomKeyBool(const char* key, bool value)
    {
        CallVoidMethodCharBool(g_firebase_crashlytics.m_JNI, g_firebase_crashlytics.m_SetCustomKeyBoolean, key, value);
    }

    void SetCustomKeyNumber(const char* key, double value)
    {
        CallVoidMethodCharDouble(g_firebase_crashlytics.m_JNI, g_firebase_crashlytics.m_SetCustomKeyDouble, key, value);
    }

    void Log(const char* message)
    {
        CallVoidMethodChar(g_firebase_crashlytics.m_JNI, g_firebase_crashlytics.m_Log, message);
    }

    void RecordException(const char* message)
    {
        CallVoidMethodChar(g_firebase_crashlytics.m_JNI, g_firebase_crashlytics.m_RecordException, message);
    }

    void RecordLuaError(const char* message, const char* traceback)
    {
        CallVoidMethodCharChar(g_firebase_crashlytics.m_JNI, g_firebase_crashlytics.m_RecordLuaError, message, traceback);
    }

    void RecordLogException(const char* severity, const char* domain, const char* message, const char* signature)
    {
        CallVoidMethodCharCharCharChar(g_firebase_crashlytics.m_JNI, g_firebase_crashlytics.m_RecordLogException, severity, domain, message, signature);
    }

    void TestJavaCrash()
    {
        CallVoidMethod(g_firebase_crashlytics.m_JNI, g_firebase_crashlytics.m_TestJavaCrash);
    }

} // namespace dmFirebaseCrashlytics

#endif // DM_PLATFORM_ANDROID
