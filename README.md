[![Actions Status Alpha](https://github.com/defold/extension-firebase-crashlytics/actions/workflows/bob.yml/badge.svg)](https://github.com/defold/extension-firebase-crashlytics/actions)

# Firebase Crashlytics for Defold

Defold native extension that integrates [Firebase Crashlytics](https://firebase.google.com/docs/crashlytics) for Android, including native crash capture through the Firebase Crashlytics NDK SDK.

This extension is Android-focused. Other platforms register a no-op extension.

## Setup

Add this extension and the core Firebase extension to your project dependencies. The core Firebase extension is responsible for creating the default Firebase app:

```lua
firebase.initialize()
firebase.crashlytics.initialize()
```

If your project provides `google-services.xml` resources, Firebase can initialize automatically through `FirebaseInitProvider`, and this extension will try to initialize Crashlytics during extension startup. If you initialize Firebase manually from Lua, call `firebase.crashlytics.initialize()` after `firebase.initialize()`.

Current Firebase Android SDKs require Android API level 23 or newer. Set this in `game.project`:

```ini
[android]
minimum_sdk_version = 23
```

## API

```lua
firebase.crashlytics.set_user_id("user-123")
firebase.crashlytics.set_custom_key("level", 12)
firebase.crashlytics.set_custom_key("premium", true)
firebase.crashlytics.log("Loaded main menu")
firebase.crashlytics.record_exception("Non-fatal error")

if firebase.crashlytics.did_crash_on_previous_execution() then
    print("Previous execution crashed")
end
```

For setup testing:

```lua
firebase.crashlytics.test_java_crash()
firebase.crashlytics.test_native_crash()
```

Crashlytics uploads reports after the app is restarted.

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

## Notes

- The extension includes `firebase-crashlytics-ndk` through `manifests/android/build.gradle`; Extender resolves the AAR and Bob packages the AAR `jni/<abi>` libraries into the final Android bundle.
- Current Firebase dependencies require `android.minimum_sdk_version = 23`; Defold's default is 21.
- The extension adds `com.google.firebase.crashlytics.mapping_file_id` with value `none` so Crashlytics can run without the Gradle plugin.
- Defold already links Android engines with a GNU build id, which Crashlytics needs to match native crashes to uploaded symbols.
- Java deobfuscation is separate from native symbolication. If ProGuard is enabled, upload `mapping.txt` for readable Java frames.
- Very early native crashes can only be captured if Firebase and Crashlytics are initialized before the crash, usually through Firebase's automatic resource-based initialization.
- Firebase's non-Gradle NDK instructions mention disabling Android native heap pointer tagging for API 30+ apps. This changes an app-level security setting, so this extension does not set it automatically; add `android:allowNativeHeapPointerTagging="false"` to the project Android manifest only if you intentionally choose that tradeoff.
