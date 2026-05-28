[![Actions Status Alpha](https://github.com/defold/extension-firebase-crashlytics/actions/workflows/bob.yml/badge.svg)](https://github.com/defold/extension-firebase-crashlytics/actions)

# Firebase Crashlytics for Defold

Defold native extension that integrates [Firebase Crashlytics](https://firebase.google.com/docs/crashlytics) for Android, iOS, and macOS. Android uses the Firebase Crashlytics NDK SDK for native crash capture. Apple platforms use the Firebase Crashlytics Apple SDK.

## Setup

Add the core Firebase extension and this extension to your project dependencies. Crashlytics requires the core Firebase extension and must be initialized after Firebase:

```lua
firebase.set_callback(function(_, message_id, message)
    if message_id == firebase.MSG_INITIALIZED then
        firebase.crashlytics.initialize()
    elseif message_id == firebase.MSG_ERROR then
        print("Firebase error: " .. tostring(message and message.error))
    end
end)
firebase.initialize()
```

Wait for `firebase.MSG_INITIALIZED` before calling `firebase.crashlytics.initialize()`.

On iOS and macOS, add `GoogleService-Info.plist` to the bundle so the core Firebase extension can configure the default Firebase app.

Place platform bundle files under the platform-specific bundle resource directories:

```text
bundle/android/res/values/google-services.xml
bundle/ios/GoogleService-Info.plist
bundle/osx/Contents/Resources/GoogleService-Info.plist
```

The macOS plist must be inside `Contents/Resources`. Files placed directly in the `.app` bundle root, such as `bundle/osx/GoogleService-Info.plist`, will make macOS code signing fail.

Current Firebase Android SDKs require Android API level 23 or newer. Set this in `game.project`:

```ini
[android]
minimum_sdk_version = 23
```

For macOS, Firebase recommends enabling Objective-C exception crashing. This extension contributes the following app `Info.plist` value:

```xml
<key>NSApplicationCrashOnExceptions</key>
<true/>
```

## API

```lua
firebase.crashlytics.set_user_id("user-123")
firebase.crashlytics.set_custom_key("level", 12)
firebase.crashlytics.set_custom_key("premium", true)
firebase.crashlytics.log("Loaded main menu")
firebase.crashlytics.record_exception("Non-fatal error")
firebase.crashlytics.record_lua_error("Lua error", "stack traceback:\n...")

if firebase.crashlytics.did_crash_on_previous_execution() then
    print("Previous execution crashed")
end
```

To report uncaught Lua runtime errors, install a Defold error handler after Crashlytics is initialized:

```lua
sys.set_error_handler(function(source, message, traceback)
    firebase.crashlytics.set_custom_key("lua_source", tostring(source or ""))
    firebase.crashlytics.record_lua_error(tostring(message or ""), tostring(traceback or ""))
end)
```

`record_lua_error()` records a non-fatal `LuaError`. On Android, parsed Lua traceback lines are sent as `StackTraceElement` frames. On iOS and macOS, parsed Lua traceback lines are sent as `FIRStackFrame` entries.

Lua errors reported this way are non-fatal reports, so check the Crashlytics non-fatal/errors view and restart the app to let Crashlytics upload queued reports. The sample prints `Firebase Crashlytics recorded LuaError non-fatal` when the Defold error handler has handed the Lua error to Crashlytics.

For setup testing:

```lua
firebase.crashlytics.test_native_crash()
if sys.get_sys_info().system_name == "Android" then
    firebase.crashlytics.test_java_crash()
end
```

Crashlytics uploads reports after the app is restarted.

When testing Apple crashes, run the app without the Xcode debugger attached, force the crash, then restart the app so Crashlytics can upload the stored report.

## Native symbols

Crashlytics can capture native crashes without symbol upload, but native stack traces will not be readable until matching unstripped symbols are uploaded.

Build Android bundles with Defold symbols enabled:

```sh
bob --platform arm64-android,armv7-android --with-symbols bundle
```

Then upload the generated symbol directory or zip with the Firebase CLI:

```sh
firebase crashlytics:symbols:upload \
  --app=FIREBASE_ANDROID_APP_ID \
  path/to/App.apk.symbols
```

`FIREBASE_ANDROID_APP_ID` is the Firebase Android app id, for example `1:1234567890:android:abcdef`, not the Android package name.

On iOS and macOS, upload the generated dSYM files to Crashlytics for readable Apple stack traces. Firebase's Apple setup guide describes the Xcode run script flow; for Defold/Bob builds, use the generated dSYMs from the bundle output with the `FirebaseCrashlytics/upload-symbols` script.

## Notes

- The extension includes `firebase-crashlytics-ndk` through `manifests/android/build.gradle`; Extender resolves the AAR and Bob packages the AAR `jni/<abi>` libraries into the final Android bundle.
- The extension includes `FirebaseCrashlytics` CocoaPods manifests for iOS and macOS, both pinned to the same Firebase Apple SDK version used by the other Firebase extensions in this workspace.
- Current Firebase dependencies require `android.minimum_sdk_version = 23`; Defold's default is 21.
- The extension adds `com.google.firebase.crashlytics.mapping_file_id` with value `none` so Crashlytics can run without the Gradle plugin.
- Defold already links Android engines with a GNU build id, which Crashlytics needs to match native crashes to uploaded symbols.
- Java deobfuscation is separate from native symbolication. If ProGuard is enabled, upload `mapping.txt` for readable Java frames.
- Very early native crashes can only be captured after the core Firebase extension and Crashlytics have both been initialized.
- Firebase's non-Gradle NDK instructions mention disabling Android native heap pointer tagging for API 30+ apps. This changes an app-level security setting, so this extension does not set it automatically; add `android:allowNativeHeapPointerTagging="false"` to the project Android manifest only if you intentionally choose that tradeoff.
