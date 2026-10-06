# Building the Survev Android app (axmol)

This project is a native C++ conversion of the web client (`../client`) built with
**axmol 2.11.5**. It is designed to build **offline / self-contained**:

- the axmol engine is **vendored into `./axmol`** (no `AX_ROOT` env, no network),
- game assets are staged into `./Content` (copy of `../client/public`),
- the build uses the **Android SDK's bundled CMake 3.22.1 + ninja** (not the
  system CMake), and the NDK toolchain,
- the APK is produced **without Gradle** (Gradle downloads Maven deps and cannot
  run here — see "Astrill / Gradle" below).

See `plan.md` for the architecture and porting status.

## Prerequisites
| Tool | Location (this machine) |
|---|---|
| Android SDK (platform android-36, build-tools 35.0.0, CMake 3.22.1, ninja) | `E:\work\survev-tools\Sdk` |
| Android NDK r27b | `E:\work\survev-tools\Sdk\ndk\27.1.12297006` |
| JDK 21 | `C:\Program Files\Eclipse Adoptium\jdk-21.0.11.10-hotspot` |
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
.\build-tests\Release\surv_tests.exe     # 40 tests, bit-exact vs the TypeScript code
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
