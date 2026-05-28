#if defined(DM_PLATFORM_IOS) || defined(DM_PLATFORM_OSX)

#define DLIB_LOG_DOMAIN "FirebaseCrashlytics"
#include <dmsdk/dlib/log.h>

#include "firebase_crashlytics_private.h"

#import <Foundation/Foundation.h>
#import <FirebaseCore/FirebaseCore.h>
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

static FIRCrashlytics* GetCrashlytics()
{
    if (![FIRApp defaultApp])
    {
        dmLogError("FirebaseApp is not initialized. Call firebase.initialize() before firebase.crashlytics.initialize().");
        return nil;
    }
    return [FIRCrashlytics crashlytics];
}

static NSString* TrimNSString(NSString* value)
{
    if (!value)
    {
        return @"";
    }
    return [value stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
}

static NSString* CaptureGroup(NSString* value, NSTextCheckingResult* match, NSUInteger index)
{
    if (!value || !match || index >= [match numberOfRanges])
    {
        return @"";
    }

    NSRange range = [match rangeAtIndex:index];
    if (range.location == NSNotFound)
    {
        return @"";
    }
    return [value substringWithRange:range];
}

static NSString* ParseLuaSymbol(NSString* detail)
{
    if (!detail || [detail length] == 0)
    {
        return @"lua";
    }

    NSRange namedPrefix = [detail rangeOfString:@"in function '"];
    if (namedPrefix.location != NSNotFound)
    {
        NSUInteger symbolStart = namedPrefix.location + namedPrefix.length;
        NSRange searchRange = NSMakeRange(symbolStart, [detail length] - symbolStart);
        NSRange symbolEnd = [detail rangeOfString:@"'" options:0 range:searchRange];
        if (symbolEnd.location != NSNotFound && symbolEnd.location > symbolStart)
        {
            return [detail substringWithRange:NSMakeRange(symbolStart, symbolEnd.location - symbolStart)];
        }
    }

    if ([detail rangeOfString:@"in function <"].location != NSNotFound)
    {
        return @"anonymous";
    }

    if ([detail rangeOfString:@"in main chunk"].location != NSNotFound)
    {
        return @"main";
    }

    return @"lua";
}

static FIRStackFrame* ParseLuaStackFrame(NSString* line)
{
    NSString* trimmed = TrimNSString(line);
    if ([trimmed length] == 0 || [trimmed rangeOfString:@"[C]"].location != NSNotFound)
    {
        return nil;
    }

    NSError* error = nil;
    NSRegularExpression* expression = [NSRegularExpression regularExpressionWithPattern:@"^\\s*(.+):(\\d+):\\s*(.*)$" options:0 error:&error];
    if (error)
    {
        return nil;
    }

    NSTextCheckingResult* match = [expression firstMatchInString:trimmed options:0 range:NSMakeRange(0, [trimmed length])];
    if (!match)
    {
        return nil;
    }

    NSString* file = TrimNSString(CaptureGroup(trimmed, match, 1));
    if ([file length] == 0)
    {
        return nil;
    }

    NSInteger lineNumber = [CaptureGroup(trimmed, match, 2) integerValue];
    NSString* symbol = ParseLuaSymbol(CaptureGroup(trimmed, match, 3));

    return [FIRStackFrame stackFrameWithSymbol:symbol file:file line:lineNumber];
}

static NSArray* ParseLuaStackTrace(NSString* traceback)
{
    if (!traceback || [traceback length] == 0)
    {
        return @[];
    }

    NSMutableArray* frames = [NSMutableArray array];
    NSArray* lines = [traceback componentsSeparatedByCharactersInSet:[NSCharacterSet newlineCharacterSet]];
    for (NSString* line in lines)
    {
        FIRStackFrame* frame = ParseLuaStackFrame(line);
        if (frame)
        {
            [frames addObject:frame];
        }
    }
    return frames;
}

void Initialize_Ext()
{
}

bool Initialize()
{
    @try
    {
        return GetCrashlytics() != nil;
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

void RecordLuaError(const char* message, const char* traceback)
{
    FIRCrashlytics* crashlytics = GetCrashlytics();
    if (crashlytics)
    {
        NSString* tracebackString = ToNSString(traceback);
        if ([tracebackString length] > 0)
        {
            [crashlytics log:tracebackString];
        }

        FIRExceptionModel* model = [FIRExceptionModel exceptionModelWithName:@"LuaError" reason:ToNSString(message)];
        NSArray* stackTrace = ParseLuaStackTrace(tracebackString);
        if ([stackTrace count] > 0)
        {
            model.stackTrace = stackTrace;
        }
        [crashlytics recordExceptionModel:model];
    }
}

} // namespace dmFirebaseCrashlytics

#endif // DM_PLATFORM_IOS || DM_PLATFORM_OSX
