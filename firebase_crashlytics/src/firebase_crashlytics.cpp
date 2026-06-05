#define EXTENSION_NAME FirebaseCrashlyticsExt
#define LIB_NAME "FirebaseCrashlytics"
#define MODULE_NAME "firebase.crashlytics"

#define DLIB_LOG_DOMAIN LIB_NAME
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <dmsdk/dlib/hash.h>
#include <dmsdk/dlib/log.h>
#include <dmsdk/sdk.h>

#include "firebase_crashlytics_private.h"

#if defined(DM_FIREBASE_CRASHLYTICS_SUPPORTED)

namespace dmFirebaseCrashlytics {

static bool g_Initialized = false;
static bool g_LogListenerRegistered = false;
static const uint32_t MAX_LOG_EXCEPTION_SIGNATURE_LENGTH = 192;
static const uint32_t MAX_RECORDED_LOG_EXCEPTIONS = 8;
static uint64_t g_RecordedLogExceptionSignatures[MAX_RECORDED_LOG_EXCEPTIONS];
static uint32_t g_RecordedLogExceptionSignatureCount = 0;

static const char* LogSeverityToString(LogSeverity severity)
{
    switch (severity)
    {
        case LOG_SEVERITY_WARNING: return "WARNING";
        case LOG_SEVERITY_ERROR: return "ERROR";
        case LOG_SEVERITY_FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

static bool IsReportableLogSeverity(LogSeverity severity)
{
    return severity == LOG_SEVERITY_ERROR || severity == LOG_SEVERITY_FATAL;
}

static bool IsWhitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static bool IsPathCharacter(char c)
{
    return c == '/' || c == '\\';
}

static bool IsTokenBoundary(char c)
{
    return c == 0 || IsWhitespace(c);
}

static void AppendChar(char* out, uint32_t out_size, uint32_t* out_length, char c)
{
    if (*out_length + 1 >= out_size)
    {
        return;
    }

    out[*out_length] = c;
    ++(*out_length);
    out[*out_length] = 0;
}

static void AppendString(char* out, uint32_t out_size, uint32_t* out_length, const char* value)
{
    while (*value != 0 && *out_length + 1 < out_size)
    {
        AppendChar(out, out_size, out_length, *value);
        ++value;
    }
}

static const char* StripFormattedLogPrefix(LogSeverity severity, const char* domain, const char* formatted_string)
{
    const char* message = formatted_string;
    const char* first_separator = strchr(message, ':');
    if (first_separator == 0)
    {
        return message;
    }

    const char* second_separator = strchr(first_separator + 1, ':');
    if (second_separator == 0)
    {
        return message;
    }

    const char* severity_name = LogSeverityToString(severity);
    uint32_t severity_length = (uint32_t)strlen(severity_name);
    if ((uint32_t)(first_separator - message) != severity_length || strncmp(message, severity_name, severity_length) != 0)
    {
        return message;
    }

    if (domain != 0)
    {
        uint32_t domain_length = (uint32_t)strlen(domain);
        const char* domain_start = first_separator + 1;
        if ((uint32_t)(second_separator - domain_start) != domain_length || strncmp(domain_start, domain, domain_length) != 0)
        {
            return message;
        }
    }

    message = second_separator + 1;
    if (*message == ' ')
    {
        ++message;
    }
    return message;
}

static void AppendPlaceholder(char* out, uint32_t out_size, uint32_t* out_length, const char* placeholder)
{
    AppendString(out, out_size, out_length, placeholder);
}

static void NormalizeLogMessage(const char* message, char* out, uint32_t out_size)
{
    out[0] = 0;
    uint32_t out_length = 0;
    bool previous_space = false;

    while (*message != 0 && IsWhitespace(*message))
    {
        ++message;
    }

    while (*message != 0 && out_length + 1 < out_size)
    {
        char c = *message;
        if (IsWhitespace(c))
        {
            if (out_length > 0 && !previous_space)
            {
                AppendChar(out, out_size, &out_length, ' ');
                previous_space = true;
            }
            ++message;
            continue;
        }

        if (c == '\'' || c == '"' || c == '`')
        {
            char quote = c;
            AppendPlaceholder(out, out_size, &out_length, "<value>");
            ++message;
            while (*message != 0 && *message != quote)
            {
                ++message;
            }
            if (*message == quote)
            {
                ++message;
            }
            previous_space = false;
            continue;
        }

        bool token_has_path_separator = false;
        const char* token = message;
        while (!IsTokenBoundary(*token))
        {
            if (IsPathCharacter(*token))
            {
                token_has_path_separator = true;
            }
            ++token;
        }
        if (token_has_path_separator)
        {
            AppendPlaceholder(out, out_size, &out_length, "<path>");
            message = token;
            previous_space = false;
            continue;
        }

        bool negative_number = c == '-' && isdigit((unsigned char)message[1]);
        bool number = isdigit((unsigned char)c) || negative_number;
        if (number)
        {
            AppendPlaceholder(out, out_size, &out_length, "#");
            if (negative_number)
            {
                ++message;
            }
            while (*message != 0 && (isalnum((unsigned char)*message) || *message == '.' || *message == 'x' || *message == 'X'))
            {
                ++message;
            }
            previous_space = false;
            continue;
        }

        AppendChar(out, out_size, &out_length, c);
        previous_space = false;
        ++message;
    }

    while (out_length > 0 && IsWhitespace(out[out_length - 1]))
    {
        --out_length;
        out[out_length] = 0;
    }

    if (out_length == 0)
    {
        AppendString(out, out_size, &out_length, "empty log message");
    }
}

static void BuildLogExceptionSignature(LogSeverity severity, const char* domain, const char* formatted_string, char* out, uint32_t out_size)
{
    char normalized_message[MAX_LOG_EXCEPTION_SIGNATURE_LENGTH];
    NormalizeLogMessage(StripFormattedLogPrefix(severity, domain, formatted_string), normalized_message, sizeof(normalized_message));

    const char* safe_domain = domain != 0 ? domain : "";
    snprintf(out, out_size, "%s:%s:%s", LogSeverityToString(severity), safe_domain, normalized_message);
    out[out_size - 1] = 0;
}

static bool IsCrashBacktraceLog(const char* domain, const char* formatted_string)
{
    if (domain != 0 && strcmp(domain, "CRASH") == 0)
    {
        return true;
    }
    return strstr(formatted_string, "CALL STACK") != 0;
}

static void ResetRecordedLogExceptionSignatures()
{
    memset(g_RecordedLogExceptionSignatures, 0, sizeof(g_RecordedLogExceptionSignatures));
    g_RecordedLogExceptionSignatureCount = 0;
}

static bool WasLogExceptionSignatureRecorded(uint64_t signature_hash)
{
    for (uint32_t i = 0; i < g_RecordedLogExceptionSignatureCount; ++i)
    {
        if (g_RecordedLogExceptionSignatures[i] == signature_hash)
        {
            return true;
        }
    }
    return false;
}

static bool ShouldRecordLogException(LogSeverity severity, const char* signature)
{
    uint64_t signature_hash = dmHashString64(signature);
    if (WasLogExceptionSignatureRecorded(signature_hash))
    {
        return false;
    }

    if (g_RecordedLogExceptionSignatureCount < MAX_RECORDED_LOG_EXCEPTIONS)
    {
        g_RecordedLogExceptionSignatures[g_RecordedLogExceptionSignatureCount] = signature_hash;
        ++g_RecordedLogExceptionSignatureCount;
        return true;
    }

    if (severity == LOG_SEVERITY_FATAL)
    {
        g_RecordedLogExceptionSignatures[MAX_RECORDED_LOG_EXCEPTIONS - 1] = signature_hash;
        return true;
    }

    return false;
}

static void CrashlyticsLogListener(LogSeverity severity, const char* domain, const char* formatted_string)
{
    if (!g_Initialized || formatted_string == 0)
    {
        return;
    }

    if (severity == LOG_SEVERITY_WARNING || IsReportableLogSeverity(severity))
    {
        Log(formatted_string);
    }

    if (!IsReportableLogSeverity(severity) || IsCrashBacktraceLog(domain, formatted_string))
    {
        return;
    }

    char signature[MAX_LOG_EXCEPTION_SIGNATURE_LENGTH];
    BuildLogExceptionSignature(severity, domain, formatted_string, signature, sizeof(signature));
    if (ShouldRecordLogException(severity, signature))
    {
        RecordLogException(LogSeverityToString(severity), domain != 0 ? domain : "", formatted_string, signature);
    }
}

static void RegisterCrashlyticsLogListener()
{
    if (g_LogListenerRegistered)
    {
        return;
    }

    dmLogRegisterListener(CrashlyticsLogListener);
    ResetRecordedLogExceptionSignatures();
    g_LogListenerRegistered = true;
}

static void UnregisterCrashlyticsLogListener()
{
    if (!g_LogListenerRegistered)
    {
        return;
    }

    dmLogUnregisterListener(CrashlyticsLogListener);
    g_LogListenerRegistered = false;
}

static bool IsFirebaseCoreExtensionAvailable(lua_State* L)
{
    int top = lua_gettop(L);

    lua_getglobal(L, "firebase");
    bool available = lua_istable(L, -1);
    if (available)
    {
        lua_getfield(L, -1, "initialize");
        available = lua_isfunction(L, -1);
        lua_pop(L, 1);
    }
    if (available)
    {
        lua_getfield(L, -1, "set_callback");
        available = lua_isfunction(L, -1);
        lua_pop(L, 1);
    }

    lua_settop(L, top);
    return available;
}

static bool RequireFirebaseCoreExtension(lua_State* L)
{
    if (IsFirebaseCoreExtensionAvailable(L))
    {
        return true;
    }

    dmLogError("Firebase Crashlytics requires the Firebase core extension. Add extension-firebase and call firebase.initialize() before firebase.crashlytics.initialize().");
    return false;
}

static bool RequireInitialized()
{
    if (g_Initialized)
    {
        return true;
    }

    dmLogError("Firebase Crashlytics is not initialized. Call firebase.initialize() before firebase.crashlytics.initialize().");
    return false;
}

static int Lua_Initialize(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    if (!RequireFirebaseCoreExtension(L))
    {
        g_Initialized = false;
        UnregisterCrashlyticsLogListener();
        lua_pushboolean(L, false);
        return 1;
    }

    g_Initialized = Initialize();
    if (g_Initialized)
    {
        RegisterCrashlyticsLogListener();
    }
    else
    {
        UnregisterCrashlyticsLogListener();
    }

    lua_pushboolean(L, g_Initialized);
    return 1;
}

static int Lua_SetEnabled(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    if (!RequireInitialized())
    {
        return 0;
    }
    SetCrashlyticsCollectionEnabled(lua_toboolean(L, 1));
    return 0;
}

static int Lua_IsEnabled(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    if (!RequireInitialized())
    {
        lua_pushboolean(L, false);
        return 1;
    }
    lua_pushboolean(L, IsCrashlyticsCollectionEnabled());
    return 1;
}

static int Lua_DidCrashOnPreviousExecution(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    if (!RequireInitialized())
    {
        lua_pushboolean(L, false);
        return 1;
    }
    lua_pushboolean(L, DidCrashOnPreviousExecution());
    return 1;
}

static int Lua_SendUnsentReports(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    if (!RequireInitialized())
    {
        return 0;
    }
    SendUnsentReports();
    return 0;
}

static int Lua_DeleteUnsentReports(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    if (!RequireInitialized())
    {
        return 0;
    }
    DeleteUnsentReports();
    return 0;
}

static int Lua_SetUserId(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    if (!RequireInitialized())
    {
        return 0;
    }
    SetUserId(luaL_checkstring(L, 1));
    return 0;
}

static int Lua_SetCustomKey(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    if (!RequireInitialized())
    {
        return 0;
    }

    const char* key = luaL_checkstring(L, 1);
    int value_type = lua_type(L, 2);

    switch (value_type)
    {
        case LUA_TBOOLEAN:
            SetCustomKeyBool(key, lua_toboolean(L, 2));
            break;
        case LUA_TNUMBER:
            SetCustomKeyNumber(key, lua_tonumber(L, 2));
            break;
        case LUA_TSTRING:
            SetCustomKeyString(key, lua_tostring(L, 2));
            break;
        default:
            return luaL_error(L, "Custom key value must be string, number or boolean, got '%s'", luaL_typename(L, 2));
    }

    return 0;
}

static int Lua_Log(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    if (!RequireInitialized())
    {
        return 0;
    }
    Log(luaL_checkstring(L, 1));
    return 0;
}

static int Lua_RecordException(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    if (!RequireInitialized())
    {
        return 0;
    }
    RecordException(luaL_checkstring(L, 1));
    return 0;
}

static int Lua_RecordLuaError(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    if (!RequireInitialized())
    {
        return 0;
    }
    RecordLuaError(luaL_checkstring(L, 1), luaL_optstring(L, 2, ""));
    return 0;
}

#if defined(DM_PLATFORM_ANDROID)
static int Lua_TestJavaCrash(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    if (!RequireInitialized())
    {
        return 0;
    }
    TestJavaCrash();
    return 0;
}
#endif

static int Lua_TestNativeCrash(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    if (!RequireInitialized())
    {
        return 0;
    }
    volatile int* crash = (volatile int*)0;
    *crash = 1;
    return 0;
}

static const luaL_reg Module_methods[] =
{
    {"initialize", Lua_Initialize},
    {"set_enabled", Lua_SetEnabled},
    {"is_enabled", Lua_IsEnabled},
    {"did_crash_on_previous_execution", Lua_DidCrashOnPreviousExecution},
    {"send_unsent_reports", Lua_SendUnsentReports},
    {"delete_unsent_reports", Lua_DeleteUnsentReports},
    {"set_user_id", Lua_SetUserId},
    {"set_custom_key", Lua_SetCustomKey},
    {"log", Lua_Log},
    {"record_exception", Lua_RecordException},
    {"record_lua_error", Lua_RecordLuaError},
#if defined(DM_PLATFORM_ANDROID)
    {"test_java_crash", Lua_TestJavaCrash},
#endif
    {"test_native_crash", Lua_TestNativeCrash},
    {0, 0}
};

static void LuaInit(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    lua_getglobal(L, "firebase");
    if (lua_isnil(L, -1))
    {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_setglobal(L, "firebase");
        lua_getglobal(L, "firebase");
    }

    lua_pushstring(L, "crashlytics");
    lua_newtable(L);
    luaL_register(L, NULL, Module_methods);
    lua_settable(L, -3);

    lua_pop(L, 1);
}

dmExtension::Result AppInitializeFirebaseCrashlyticsExtension(dmExtension::AppParams* params)
{
    return dmExtension::RESULT_OK;
}

dmExtension::Result InitializeFirebaseCrashlyticsExtension(dmExtension::Params* params)
{
    dmLogInfo("Initialize Firebase Crashlytics");

    LuaInit(params->m_L);
    Initialize_Ext();

    return dmExtension::RESULT_OK;
}

dmExtension::Result AppFinalizeFirebaseCrashlyticsExtension(dmExtension::AppParams* params)
{
    return dmExtension::RESULT_OK;
}

dmExtension::Result FinalizeFirebaseCrashlyticsExtension(dmExtension::Params* params)
{
    UnregisterCrashlyticsLogListener();
    g_Initialized = false;
    return dmExtension::RESULT_OK;
}

dmExtension::Result UpdateFirebaseCrashlyticsExtension(dmExtension::Params* params)
{
    return dmExtension::RESULT_OK;
}

} // namespace dmFirebaseCrashlytics

DM_DECLARE_EXTENSION(EXTENSION_NAME, LIB_NAME, dmFirebaseCrashlytics::AppInitializeFirebaseCrashlyticsExtension, dmFirebaseCrashlytics::AppFinalizeFirebaseCrashlyticsExtension, dmFirebaseCrashlytics::InitializeFirebaseCrashlyticsExtension, dmFirebaseCrashlytics::UpdateFirebaseCrashlyticsExtension, 0, dmFirebaseCrashlytics::FinalizeFirebaseCrashlyticsExtension)

#else // DM_FIREBASE_CRASHLYTICS_SUPPORTED

static dmExtension::Result InitializeFirebaseCrashlytics(dmExtension::Params* params)
{
    dmLogInfo("Registered extension Firebase Crashlytics (null)");
    return dmExtension::RESULT_OK;
}

static dmExtension::Result FinalizeFirebaseCrashlytics(dmExtension::Params* params)
{
    return dmExtension::RESULT_OK;
}

DM_DECLARE_EXTENSION(EXTENSION_NAME, LIB_NAME, 0, 0, InitializeFirebaseCrashlytics, 0, 0, FinalizeFirebaseCrashlytics)

#endif // DM_FIREBASE_CRASHLYTICS_SUPPORTED
