# Building the Survev Android app (axmol)

Converted from the web client (`../client`) to native C++ using **axmol 2.11.5**.
See `plan.md` for architecture and `src/` for the ported code.

## Prerequisites (this machine)
- axmol 2.11.5: `E:\work\survev-tools\axmol-2.11.5`
- Android SDK: `E:\work\survev-tools\Sdk` (platform android-36, build-tools 35.0.0)
- Android NDK: `E:\work\survev-tools\Sdk\ndk\27.1.12297006`
- JDK 21: `C:\Program Files\Eclipse Adoptium\jdk-21.0.11.10-hotspot`
- CMake 4.4 (system) + ninja (`Sdk\cmake\3.22.1\bin\ninja.exe`)

All of these are wired up by `tools/android-env.ps1`. Dot-source it first:
```powershell
. .\tools\android-env.ps1
```

## 1. Host tests (protocol core, no Android needed)
```powershell
cmake -S tests -B build-tests -G "Visual Studio 17 2022" -A x64
cmake --build build-tests --config Release
.\build-tests\Release\surv_tests.exe    # 22 tests, verified bit-exact vs the TS code
```

## 2. Stage assets
```powershell
Copy-Item -Recurse -Force ..\client\public\* Content\
```
(`Content/` is git-ignored; it is the APK asset payload.)

## 3. Build the native library
```powershell
$ndk = "$env:ANDROID_HOME\ndk\27.1.12297006"
$ninja = "$env:ANDROID_HOME\cmake\3.22.1\bin\ninja.exe"
$cmake = "C:\Program Files\CMake\bin\cmake.exe"
$proj = (Resolve-Path .\proj.android).Path

foreach ($abi in 'arm64-v8a','x86_64') {
  $b = if ($abi -eq 'arm64-v8a') { 'build-android' } else { 'build-android-x86_64' }
  & $cmake -S . -B $b -G Ninja `
    -DCMAKE_TOOLCHAIN_FILE="$ndk/build/cmake/android.toolchain.cmake" `
    -DANDROID_ABI=$abi -DANDROID_PLATFORM=android-24 -DANDROID_STL=c++_shared `
    -DANDROID_TOOLCHAIN=clang -DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON `
    -DANDROID_USE_LEGACY_TOOLCHAIN_FILE=false -DCMAKE_MAKE_PROGRAM="$ninja" `
    -DCMAKE_BUILD_TYPE=Release -D_AX_ANDROID_PROJECT_DIR="$proj"
  & $cmake --build $b --config Release
}
```

## 4. Package a signed APK
```powershell
powershell -File .\tools\build-apk.ps1 -Abis "arm64-v8a,x86_64"
# -> SurvevMobile-debug.apk (v2/v3 signed)
```

## 5. Install & run
```powershell
adb install -r SurvevMobile-debug.apk
adb shell am start -n com.survev.mobile/dev.axmol.app.AppActivity
```

## The Gradle path (standard, but blocked here)
`cd proj.android; ./gradlew assembleDebug` is the canonical build. It **cannot run
on this machine while Astrill VPN is active**: Astrill installs a Winsock LSP
(`C:\Windows\System32\ASProxy64.dll`) that breaks Java NIO, and the Gradle daemon
communicates over TCP loopback via NIO. Symptom: the JVM crashes with
`EXCEPTION_ILLEGAL_INSTRUCTION` in `sun.nio.ch.WEPoll.ctl`, or the daemon fails
with "operation attempted on something that is not a socket".

To use Gradle: stop the Astrill `ASProxy` service (admin) or whitelist `java.exe`,
then run `gradlew assembleDebug`. `tools/build-apk.ps1` reproduces the Gradle
packaging steps without Java sockets, so it works regardless.

## Emulator
An AVD `SurvevTest` (android-35 x86_64, WHPX) can be created/booted with:
```powershell
& "$env:ANDROID_HOME\emulator\emulator.exe" -avd SurvevTest -no-window -no-audio `
    -no-boot-anim -no-snapshot -gpu swiftshader_indirect -accel on
```
