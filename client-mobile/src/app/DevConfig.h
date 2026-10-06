#pragma once
// M3 development defaults for the headless networking gate.
//
// The native client has no menu UI yet (M7), so a direct join URL/token can be
// supplied here at build time. At runtime these are overridden by the axmol
// UserDefault keys (SharedPreferences):
//   surv_joinUrl   e.g. "ws://10.0.2.2:9000/play"
//   surv_joinToken token returned by POST /api/find_game_v2
//   surv_apiUrl    e.g. "http://10.0.2.2:8000"
//   surv_region    e.g. "local"
//
// Android emulator host loopback is 10.0.2.2; a physical device needs the
// dev machine's LAN IP (and the server must listen on 0.0.0.0, which it does).
namespace surv {
namespace dev {

inline constexpr const char* kJoinUrl = "";      // direct ws:// join URL
inline constexpr const char* kJoinToken = "";    // join token for kJoinUrl
inline constexpr const char* kApiBaseUrl = "";   // e.g. "http://10.0.2.2:8000"
inline constexpr const char* kRegion = "local";  // find_game region
inline constexpr int kGameModeIdx = 0;           // index into server modes

// UserDefault keys used to override the values above at runtime.
inline constexpr const char* kKeyJoinUrl = "surv_joinUrl";
inline constexpr const char* kKeyJoinToken = "surv_joinToken";
inline constexpr const char* kKeyApiUrl = "surv_apiUrl";
inline constexpr const char* kKeyRegion = "surv_region";

} // namespace dev
} // namespace surv
