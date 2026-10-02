# proj.android

This directory is where the axmol Android project for `client-mobile` lives.

The axmol engine ships a project generator (`axmol new`) that scaffolds the
Android Studio project (Gradle + CMake + JNI glue) from a template. We do not
commit a generated project here because it is tied to the installed axmol
version, SDK/NDK paths, and the application id.

## Build steps (M0 in `plan.md`)

Prerequisites: JDK 21, Android SDK + NDK (27.x), CMake 3.22+, Android Studio
with the NDK/CMake components. No Android SDK is installed on this machine yet.

```sh
# 1. Get axmol (https://github.com/axmolengine/axmol)
git clone --branch release/2.x https://github.com/axmolengine/axmol.git ../axmol
cd ../axmol
./setup.py          # configure NDK/CMake paths (Windows: setup.py)
export PATH="$PWD/tools:$PATH"

# 2. Scaffold the app project (run inside this repo)
axmol new -p com.survev.mobile -d . -l cpp SurvevMobile
# This creates proj.android/ (Android Studio project) that includes the
# generated CMake target. Point its CMakeLists at ../app/CMakeLists.txt and
# link the surv_core/surv_app sources.

# 3. Configure the app
#    - set SURV_AXMOL_DIR to the axmol checkout
#    - point the generated CMake at ../app/CMakeLists.txt
#    - copy client/public assets into assets/ (or symlink; see plan.md 6)

# 4. Build (apk)
axmol -p android -xc "-PKEY_STORE_FILE=... ,-PKEY_STORE_PASSWORD=...,-PKEY_ALIAS=...,-PKEY_PASSWORD=..."
# or open proj.android/ in Android Studio and run on a device.
```

## What the generated project needs to include

1. `surv_core` (src/core + src/net) and `surv_app` (src/app + src/game + src/ui)
   as library targets — see the top-level `CMakeLists.txt` and `app/CMakeLists.txt`.
2. AndroidManifest orientation lock:
   ```xml
   <activity android:screenOrientation="sensorLandscape" ... />
   ```
3. Asset packaging of `client/public/` (images, audio, fonts, l10n) via
   `android.sourceSets.main.assets.srcDirs`.

## Layout (final)

```
proj.android/
  app/                    Android application module (Java/Kotlin + JNI)
    src/main/AndroidManifest.xml
    src/main/java/com/survev/mobile/AppActivity.java
  app/build.gradle
  CMakeLists.txt          includes ../app/CMakeLists.txt + surv_core/surv_app
```