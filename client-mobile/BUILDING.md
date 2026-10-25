# Building the Survev Android app (axmol)

This project is a native C++ conversion of the web client (`../client`) built with
**axmol 2.11.5**. It is designed to build **offline / self-contained**:

- the axmol engine is **vendored into `./axmol`** (no `AX_ROOT` env, no network),
- game assets are staged into `./Content` (copy of `../client/public`),
- the build uses the **Android SDK's bundled CMake 3.22.1 + ninja** (not the
  system CMake), and the NDK toolchain,
- the APK is produced **without Gradle** (Gradle downloads Maven deps and cannot
  run here — see "Astrill / Gradle" below).

See `plan.md` for the architecture and porting status, and **`ARCHITECTURE.md`**
for the current UI architecture: gameplay (world, touch pads, HUD) is rendered by
**axmol + FairyGUI**, while every other screen (menu, settings, loadout, team,
profile/pass) is **Android Java** over a single JNI bridge — one `Activity`, no
WebView. Anything UI-shaped should follow `ARCHITECTURE.md` §5/§7.

## Prerequisites
| Tool | Location (this machine) |
|---|---|
| Android SDK (platform android-36, build-tools 35.0.0, CMake 3.22.1, ninja) | `E:\work\survev-tools\Sdk` |
| Android NDK r27b | `E:\work\survev-tools\Sdk\ndk\27.1.12297006` |
| JDK 21 | `C:\Program Files\Eclipse Adoptium\jdk-21.0.11.10-hotspot` |
| bundletool (AAB) | any `bundletool-all.jar`; pass `-BundletoolJar` or `$env:BUNDLETOOL_JAR` |
| axmol 2.11.5 source (to vendor) | `E:\work\survev-tools\axmol-2.11.5` |

Everything is wired by `tools/android-env.ps1` (it also puts the SDK's
`cmake\3.22.1\bin` first on `PATH` so `cmake` is the SDK one).

## One-time setup (offline)
```powershell
# 1. Vendor the engine into ./axmol (copies ~590 MB, patches fetch.cmake for CMake 3.22)
powershell -File tools\vendor-axmol.ps1 -Source E:\work\survev-tools\axmol-2.11.5

# 2. Stage the game assets into ./Content
powershell -File tools\sync-content.ps1
```
Both `axmol/` and `Content/` are git-ignored (large); they live on disk to make
the checkout self-contained.

## Build
```powershell
# 3. Native libraries for the ARM ABIs (armeabi-v7a + arm64-v8a)
powershell -File tools\build-native.ps1 -Abis "armeabi-v7a,arm64-v8a"

# 4. Package + sign the APK (both ABIs)
powershell -File tools\build-apk.ps1 -Abis "armeabi-v7a,arm64-v8a"
# -> SurvevMobile-debug.apk
```
`build-native.ps1` configures one build dir per ABI:
`build-android-armeabi-v7a/`, `build-android-arm64-v8a/` (and
`build-android-x86_64/` if you pass `x86_64`). The first build compiles the whole
axmol engine, so it takes a while; later builds are incremental.

## Release APK + Play AAB (store build)

Store builds use the same Gradle-free pipeline. Google Play requires 64-bit, so
build **arm64-v8a** first:

```powershell
. .\tools\android-env.ps1
.\tools\build-native.ps1 -Abis "arm64-v8a"
```

Create an upload keystore **once** (never commit it):

```powershell
keytool -genkeypair -keystore upload.jks -alias upload -keyalg RSA -keysize 4096 -validity 10000
```

Prefer environment variables so the passwords stay out of shell history:

```powershell
$env:SURVEV_KS_ALIAS = "upload"
$env:SURVEV_KS_PASS  = "<store password>"
$env:SURVEV_KEY_PASS = "<key password>"
```

Signed release APK (version defaults from `../package.json`: `0.4.3` -> code
`403`; `-ApiUrl` replaces the manifest `surv_apiUrl`):

```powershell
.\tools\build-apk.ps1 -Release -Abis "arm64-v8a" `
    -ApiUrl "https://survev.io" -KeystorePath "upload.jks"
# -> SurvevMobile-release.apk
```

- Use `-VersionName`/`-VersionCode` to override; bump `package.json` or pass a
  new code for every Play upload (version codes must increase).
- Release builds strip the native symbols, sign with APK Signature Scheme v2/v3,
  print the signer certificate, and still verify that `resources.arsc` is stored
  uncompressed + 4-byte aligned.
- The release manifest is not debuggable; the debug APK path is unchanged.

Play app bundle (AAB):

```powershell
.\tools\build-aab.ps1 -Abis "arm64-v8a" `
    -BundletoolJar "C:\tools\bundletool-all.jar"
# -> SurvevMobile-release.aab
```

- Build the release APK first: `build-aab.ps1` reads the version, API URL and ABI
  list from it, assembles the `base` module (protobuf manifest/resources + dex +
  stripped native libs + assets) and calls `bundletool build-bundle`.
- `bundletool-all.jar` is not vendored (it is large); pass `-BundletoolJar`,
  `$env:BUNDLETOOL_JAR`, or put it in `tools/`.
- Add the same keystore arguments (`-KeystorePath/-KeystoreAlias/
  -KeystorePassword/-KeyPassword`) to JAR-sign the AAB for upload. Without them
  the AAB is unsigned (Play can sign it with the app signing key).
- Inspect/install locally with bundletool:
  ```powershell
  java -jar bundletool-all.jar build-apks --bundle=SurvevMobile-release.aab `
      --output=SurvevMobile-release.apks --mode=universal `
      --ks=upload.jks --ks-key-alias=upload
  ```
- These scripts are the canonical store path on this machine. If Gradle is
  usable (Astrill disabled), `cd proj.android; ./gradlew bundleRelease
  -PKEY_STORE_FILE=... -PKEY_STORE_PASSWORD=... -PKEY_ALIAS=... -PKEY_PASSWORD=...`
  is the standard alternative.

## Install & run
```powershell
adb install -r SurvevMobile-debug.apk
adb shell am start -n com.survev.mobile/dev.axmol.app.AppActivity
```

## Host tests (protocol core, no Android)
```powershell
. .\tools\android-env.ps1
cmake -S tests -B build-tests -G "Visual Studio 17 2022" -A x64
cmake --build build-tests --config Release
.\build-tests\Release\surv_tests.exe     # 163 tests, bit-exact vs the TypeScript code
```

## Emulator (x86_64)
```powershell
& "$env:ANDROID_HOME\emulator\emulator.exe" -avd SurvevTest -no-window -no-audio `
    -no-boot-anim -no-snapshot -gpu swiftshader_indirect -accel on
# build/package with -Abis "x86_64" to run on it
```

## Why not Gradle?
`cd proj.android; ./gradlew assembleDebug` is the canonical path, but it **cannot
run on this machine while Astrill VPN is active**: Astrill installs a Winsock LSP
(`C:\Windows\System32\ASProxy64.dll`) that breaks Java NIO, and the Gradle daemon
talks over TCP loopback using NIO. Symptoms: `EXCEPTION_ILLEGAL_INSTRUCTION` in
`sun.nio.ch.WEPoll.ctl`, or "operation attempted on something that is not a
socket". `tools/build-apk.ps1` reproduces Gradle's packaging with `aapt2`/`javac`/
`d8`/`zipalign`/`apksigner` (which use no sockets), so it works regardless. To use
Gradle, stop the Astrill `ASProxy` service (admin) or whitelist `java.exe`.

## Notes
- **CMake version**: axmol 2.11.5's `1k/fetch.cmake` declares
  `cmake_minimum_required(3.23...)`, but the Android SDK ships CMake **3.22.1**.
  `tools/vendor-axmol.ps1` lowers that to `3.22` in the vendored copy so the SDK
  CMake is used. (The system CMake 4.4 is intentionally not used.)
- **ABIs**: `armeabi-v7a` and `arm64-v8a` by default; `x86_64` is available for
  emulator testing. Prebuilt axmol 3rdparty libs exist for all three.
- **Offline**: after vendoring, configure/build performs no downloads. The axmol
  `cache/` (prebuilt 3rdparty) is part of `axmol/`.
- **Shaders (`axslc`)**: the engine build (`build-native.ps1`) compiles the
  axmol GLSL into `proj.android/build/runtime/axslc`, and `build-apk.ps1` now
  packages those files as `assets/axslc/**` (matching the Gradle build). If they
  are missing, axmol compiles empty shaders and the app renders a **black
  screen** with `The vertex attribute 'a_position' ... not exist` warnings in
  logcat.
- **M4 rendering status (on-device)**: terrain/ground/grid/labels, sprite atlases
  (`Content/atlas`), obstacles/buildings/ceilings, particles, loot, skeletal
  players and the minimap/big map render on device. See `M4_REMAINING.md` for
  the session-by-session coverage and remaining polish.
- **Java UI (from 2026-10-14)**: the APK also packages `assets/fgui/**` once
  FairyGUI packages exist, and `proj.android/app/libs/*.jar` are linked into
  `javac`/`d8` by `build-apk.ps1`. See `ARCHITECTURE.md` §6 for the UI split,
  the FairyGUI export step and the OkHttp vendoring rule.

## Java UI build without Gradle (S1)

`tools/build-apk.ps1` compiles `proj.android/app/src/**/*.java` (the axmol
`dev.axmol.lib` sources plus our `dev.axmol.app.*` bridge/UI classes) with
`javac -source 8 -target 8`, so **only Java 8 APIs are available**. In
particular `java.util.function.*` and AndroidX are *not* on the classpath: write
the UI with the platform `View`/`View.OnClickListener` types (anonymous inner
classes) rather than lambdas.

The Java UI layer for the S1 bridge step is:

| File | Role |
|---|---|
| `src/dev/axmol/app/SurvBridge.java` | the single JNI bridge: `native` methods (C++ implementations in `src/app/UiBridge.cpp`) + the `onNative*` handlers the C++ side calls. |
| `src/dev/axmol/app/JavaUiHost.java` | owns the `surv_java_ui` `FrameLayout` above the GL surface and switches between screens; the Java end of the C++ callbacks. |
| `src/dev/axmol/app/MenuFragment.java` | the S1 stand-in menu (name field + bridge self test); replaced by the real menu in S3. |
| `src/dev/axmol/app/Json.java` | tiny key reader for the bridge JSON (config/l10n/news). |
| `src/dev/axmol/app/AppActivity.java` | adds the Java UI container after `super.onCreate()`. |

The APK build does not need extra flags for these — it globs
`proj.android/app/src` already. When OkHttp is vendored (S3, `app/libs/*.jar`)
the same script links and dexes it.
