#define EXTENSION_NAME FirebaseCrashlyticsExt
#define LIB_NAME "FirebaseCrashlytics"
#define MODULE_NAME "firebase.crashlytics"

#define DLIB_LOG_DOMAIN LIB_NAME
#include <dmsdk/dlib/log.h>
#include <dmsdk/sdk.h>

#if defined(DM_PLATFORM_ANDROID)

#include "firebase_crashlytics_private.h"

namespace dmFirebaseCrashlytics {

static int Lua_Initialize(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    lua_pushboolean(L, Initialize());
    return 1;
}

static int Lua_SetEnabled(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    SetCrashlyticsCollectionEnabled(lua_toboolean(L, 1));
    return 0;
}

static int Lua_IsEnabled(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    lua_pushboolean(L, IsCrashlyticsCollectionEnabled());
    return 1;
}

static int Lua_DidCrashOnPreviousExecution(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    lua_pushboolean(L, DidCrashOnPreviousExecution());
    return 1;
}

static int Lua_SendUnsentReports(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    SendUnsentReports();
    return 0;
}

static int Lua_DeleteUnsentReports(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    DeleteUnsentReports();
    return 0;
}

static int Lua_SetUserId(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    SetUserId(luaL_checkstring(L, 1));
    return 0;
}

static int Lua_SetCustomKey(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

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
    Log(luaL_checkstring(L, 1));
    return 0;
}

static int Lua_RecordException(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    RecordException(luaL_checkstring(L, 1));
    return 0;
}

static int Lua_TestJavaCrash(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    TestJavaCrash();
    return 0;
}

static int Lua_TestNativeCrash(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
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
    {"test_java_crash", Lua_TestJavaCrash},
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
    Initialize();

    return dmExtension::RESULT_OK;
}

dmExtension::Result AppFinalizeFirebaseCrashlyticsExtension(dmExtension::AppParams* params)
{
    return dmExtension::RESULT_OK;
}

dmExtension::Result FinalizeFirebaseCrashlyticsExtension(dmExtension::Params* params)
{
    return dmExtension::RESULT_OK;
}

dmExtension::Result UpdateFirebaseCrashlyticsExtension(dmExtension::Params* params)
{
    return dmExtension::RESULT_OK;
}

} // namespace dmFirebaseCrashlytics

DM_DECLARE_EXTENSION(EXTENSION_NAME, LIB_NAME, dmFirebaseCrashlytics::AppInitializeFirebaseCrashlyticsExtension, dmFirebaseCrashlytics::AppFinalizeFirebaseCrashlyticsExtension, dmFirebaseCrashlytics::InitializeFirebaseCrashlyticsExtension, dmFirebaseCrashlytics::UpdateFirebaseCrashlyticsExtension, 0, dmFirebaseCrashlytics::FinalizeFirebaseCrashlyticsExtension)

#else // DM_PLATFORM_ANDROID

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

#endif // DM_PLATFORM_ANDROID
