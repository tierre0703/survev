# proj.android

Android Studio / Gradle project for the Survev axmol app (package
`com.survev.mobile`, native lib `libSurvevMobile.so`).

- `app/` — application module (Java/Kotlin, manifest, res, JNI `main.cpp`)
- `app/jni/main.cpp` — JNI entry that instantiates the global `AppDelegate`
  (see `../src/app/AppDelegate.{h,cpp}`)
- `app/src/dev/axmol/app/AppActivity.java` — activity (extends axmol's
  `AxmolActivity`)
- `gradle.properties` — ABI list (`__1K_ARCHS`) and a `SelectorProvider`
  workaround for the JVM crash caused by Astrill's Winsock LSP
- `../CMakeLists.txt` — the CMake target this project builds via
  `externalNativeBuild`

## Building

See [`../BUILDING.md`](../BUILDING.md). The canonical Gradle build is:

```sh
./gradlew assembleDebug
```

but Gradle cannot run on this machine while Astrill VPN is active (its Winsock LSP
breaks Java NIO). Use `../tools/build-native.ps1` + `../tools/build-apk.ps1`
instead, which build and package the app without Gradle. To use Gradle, stop the
Astrill `ASProxy` service (admin) or whitelist `java.exe`.
