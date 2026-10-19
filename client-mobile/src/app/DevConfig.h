#pragma once
// Dev defaults for the native client (the counterpart of the web client's
// `?api=` / `?region=` URL parameters in client/src/main.ts).
//
// At runtime every value below is overridable through a `surv_*` UserDefault
// (SharedPreferences); `proj.android/app/AndroidManifest.xml` declares matching
// `<meta-data>` entries and axmol's AxmolActivity copies any `debug.<name>`
// system property into `<name>` at startup, so a debug build can be retargeted
// without a rebuild:
//   adb shell setprop debug.surv_apiUrl http://10.0.2.2:8000
//   adb shell setprop debug.surv_region local
//
// With `adb reverse tcp:8000 tcp:8000` the emulator reaches the host on
// 127.0.0.1, so the loopback URL below works through the reverse tunnel.
// A physical device needs the dev machine's LAN IP (and the server must
// listen on 0.0.0.0, which it does).
namespace surv {
namespace dev {

inline constexpr const char* kJoinUrl = "";      // direct ws:// join URL
inline constexpr const char* kJoinToken = "";    // join token for kJoinUrl
// M3 dev gate: with `adb reverse tcp:8000 tcp:8000` (and tcp:9000) the emulator
// reaches the host dev server on 127.0.0.1, so find_game's advertised
// ws://127.0.0.1:9000/play works without changing the server region config.
inline constexpr const char* kApiBaseUrl = "http://127.0.0.1:8000";
inline constexpr const char* kRegion = "local";  // find_game region
inline constexpr const char* kApiUrl = kApiBaseUrl;  // manifest default name
inline constexpr int kGameModeIdx = 0;           // index into server modes

// UserDefault keys used to override the values above at runtime. All of them are
// settable from adb: `AxmolActivity` copies every `debug.<key>` system property
// into SharedPreferences "<key>" at startup, and the default manifest ships the
// same names so the app reads them the same way on every device:
//   adb shell setprop debug.surv_region local
//   adb shell setprop debug.surv_apiUrl http://127.0.0.1:8000
inline constexpr const char* kKeyJoinUrl = "surv_joinUrl";
inline constexpr const char* kKeyJoinToken = "surv_joinToken";
inline constexpr const char* kKeyApiUrl = "surv_apiUrl";
inline constexpr const char* kKeyRegion = "surv_region";
// Diagnostic-only: log the find_game request and every advertised join URL so a
// dev run can see exactly which address the emulator was told to dial.
//   adb shell setprop debug.surv_debug 1
inline constexpr const char* kKeyDebug = "surv_debug";

} // namespace dev
} // namespace surv
