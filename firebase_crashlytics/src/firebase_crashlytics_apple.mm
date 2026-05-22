#if defined(DM_PLATFORM_IOS) || defined(DM_PLATFORM_OSX)

#define DLIB_LOG_DOMAIN "FirebaseCrashlytics"
#include <dmsdk/dlib/log.h>

#include "firebase_crashlytics_private.h"

#import <Foundation/Foundation.h>
#import <FirebaseCore/FirebaseCore.h>
#import <FirebaseCore/FIROptions.h>
#import <FirebaseCrashlytics/FirebaseCrashlytics.h>

namespace dmFirebaseCrashlytics {

static NSString* ToNSString(const char* value)
{
    if (value == 0)
    {
        return @"";
    }

    NSString* string = [NSString stringWithUTF8String:value];
    return string ? string : @"";
}

static void AddExistingPath(NSMutableArray* paths, NSString* path)
{
    if (path && ![paths containsObject:path])
    {
        [paths addObject:path];
    }
}

static NSString* FindGoogleServiceInfoPlist()
{
    NSBundle* bundle = [NSBundle mainBundle];
    NSFileManager* file_manager = [NSFileManager defaultManager];
    NSMutableArray* paths = [NSMutableArray array];

    AddExistingPath(paths, [bundle pathForResource:@"GoogleService-Info" ofType:@"plist"]);
    AddExistingPath(paths, [[bundle resourcePath] stringByAppendingPathComponent:@"GoogleService-Info.plist"]);
    AddExistingPath(paths, [[bundle bundlePath] stringByAppendingPathComponent:@"GoogleService-Info.plist"]);

    NSString* current_directory = [file_manager currentDirectoryPath];
    AddExistingPath(paths, [current_directory stringByAppendingPathComponent:@"GoogleService-Info.plist"]);
    AddExistingPath(paths, [current_directory stringByAppendingPathComponent:@"bundle/osx/Contents/Resources/GoogleService-Info.plist"]);
    AddExistingPath(paths, [current_directory stringByAppendingPathComponent:@"bundle/osx/GoogleService-Info.plist"]);
    AddExistingPath(paths, [current_directory stringByAppendingPathComponent:@"bundle/common/GoogleService-Info.plist"]);

    for (NSString* path in paths)
    {
        if ([file_manager fileExistsAtPath:path])
        {
            return path;
        }
    }
    return nil;
}

static bool ConfigureFirebaseApp()
{
    if ([FIRApp defaultApp])
    {
        return true;
    }

    NSString* plist_path = FindGoogleServiceInfoPlist();
    if (!plist_path)
    {
        dmLogError("GoogleService-Info.plist not found. Checked app bundle resources, app bundle root, current directory, bundle/osx/Contents/Resources, bundle/osx, and bundle/common.");
        [FIRApp configure];
        return [FIRApp defaultApp] != nil;
    }

    FIROptions* options = [[FIROptions alloc] initWithContentsOfFile:plist_path];
    if (!options)
    {
        dmLogError("Unable to load Firebase options from %s", [plist_path UTF8String]);
        return false;
    }

    dmLogInfo("Configuring Firebase from %s", [plist_path UTF8String]);
    [FIRApp configureWithOptions:options];
    return [FIRApp defaultApp] != nil;
}

static FIRCrashlytics* GetCrashlytics()
{
    if (![FIRApp defaultApp])
    {
        dmLogError("FirebaseApp is not initialized. Call firebase.initialize() first, or call firebase.crashlytics.initialize() with GoogleService-Info.plist in the bundle.");
        return nil;
    }
    return [FIRCrashlytics crashlytics];
}

void Initialize_Ext()
{
}

bool Initialize()
{
    @try
    {
        if (!ConfigureFirebaseApp())
        {
            return false;
        }
        return [FIRApp defaultApp] != nil && [FIRCrashlytics crashlytics] != nil;
    }
    @catch (NSException* exception)
    {
        const char* reason = exception.reason ? [exception.reason UTF8String] : "unknown";
        dmLogError("Unable to initialize Firebase Crashlytics (%s)", reason);
        return false;
    }
}

void SetCrashlyticsCollectionEnabled(bool enabled)
{
    FIRCrashlytics* crashlytics = GetCrashlytics();
    if (crashlytics)
    {
        [crashlytics setCrashlyticsCollectionEnabled:enabled ? YES : NO];
    }
}

bool IsCrashlyticsCollectionEnabled()
{
    FIRCrashlytics* crashlytics = GetCrashlytics();
    return crashlytics && [crashlytics isCrashlyticsCollectionEnabled];
}

bool DidCrashOnPreviousExecution()
{
    FIRCrashlytics* crashlytics = GetCrashlytics();
    return crashlytics && [crashlytics didCrashDuringPreviousExecution];
}

void SendUnsentReports()
{
    FIRCrashlytics* crashlytics = GetCrashlytics();
    if (crashlytics)
    {
        [crashlytics sendUnsentReports];
    }
}

void DeleteUnsentReports()
{
    FIRCrashlytics* crashlytics = GetCrashlytics();
    if (crashlytics)
    {
        [crashlytics deleteUnsentReports];
    }
}

void SetUserId(const char* user_id)
{
    FIRCrashlytics* crashlytics = GetCrashlytics();
    if (crashlytics)
    {
        [crashlytics setUserID:ToNSString(user_id)];
    }
}

void SetCustomKeyString(const char* key, const char* value)
{
    FIRCrashlytics* crashlytics = GetCrashlytics();
    if (crashlytics)
    {
        [crashlytics setCustomValue:ToNSString(value) forKey:ToNSString(key)];
    }
}

void SetCustomKeyBool(const char* key, bool value)
{
    FIRCrashlytics* crashlytics = GetCrashlytics();
    if (crashlytics)
    {
        [crashlytics setCustomValue:[NSNumber numberWithBool:value ? YES : NO] forKey:ToNSString(key)];
    }
}

void SetCustomKeyNumber(const char* key, double value)
{
    FIRCrashlytics* crashlytics = GetCrashlytics();
    if (crashlytics)
    {
        [crashlytics setCustomValue:[NSNumber numberWithDouble:value] forKey:ToNSString(key)];
    }
}

void Log(const char* message)
{
    FIRCrashlytics* crashlytics = GetCrashlytics();
    if (crashlytics)
    {
        [crashlytics log:ToNSString(message)];
    }
}

void RecordException(const char* message)
{
    FIRCrashlytics* crashlytics = GetCrashlytics();
    if (crashlytics)
    {
        FIRExceptionModel* model = [FIRExceptionModel exceptionModelWithName:@"LuaError" reason:ToNSString(message)];
        [crashlytics recordExceptionModel:model];
    }
}

} // namespace dmFirebaseCrashlytics

#endif // DM_PLATFORM_IOS || DM_PLATFORM_OSX
