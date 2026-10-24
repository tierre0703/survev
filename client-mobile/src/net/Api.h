#pragma once
// Port of client/src/api.ts + the find_game flow from client/src/main.ts.
//
// Only the HTTP call needs axmol (Api.cpp); the request/response types stay
// engine-independent so Game/GameScene can use them without the engine.
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace surv {

// shared/types/api.ts zFindGameBody
struct FindGameBody {
    std::string region;
    std::vector<std::string> zones;
    uint32_t version = 0;
    int playerCount = 1;
    bool autoFill = true;
    int gameModeIdx = 0;
    std::string turnstileToken;  // optional
};

// shared/types/api.ts FindGameMatchData
struct FindGameMatchData {
    std::vector<std::string> urls;
    std::string joinToken;
};

// Result of a find_game attempt. `error` is one of the FindGameError strings
// (e.g. "full", "invalid_protocol", "find_game_failed").
struct FindGameResult {
    bool ok = false;
    std::string error;
    FindGameMatchData data;
};

using FindGameCallback = std::function<void(FindGameResult)>;

// Dev diagnostics (see `DevConfig.h` kKeyDebug): when the `surv_debug`
// UserDefault is "1", find_game logs its request body and, on success, the
// advertised join URLs. Implemented in Api.cpp; kept out of the header so the
// host tests do not need the engine.
bool isDevLogEnabled();

// POSTs `body` as JSON to `apiBaseUrl + "/api/find_game_v2"` (mirrors
// main.ts findGame()). The callback runs on the main thread. `apiBaseUrl` may
// be empty to post to the relative path (only meaningful when a proxy base is
// configured; the native client should pass a full origin).
void findGame(const std::string& apiBaseUrl, const FindGameBody& body, FindGameCallback cb);

// POSTs an already-encoded JSON document to `url` (used by net/ErrorLog's
// /api/report_error writer). Fire and forget: transport failures are logged
// only in debug builds, like errorLogs.ts ignoring the fetch result.
void postJson(const std::string& url, const std::string& bodyJson);

} // namespace surv
