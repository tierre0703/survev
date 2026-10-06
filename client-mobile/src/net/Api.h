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

// POSTs `body` as JSON to `apiBaseUrl + "/api/find_game_v2"` (mirrors
// main.ts findGame()). The callback runs on the main thread. `apiBaseUrl` may
// be empty to post to the relative path (only meaningful when a proxy base is
// configured; the native client should pass a full origin).
void findGame(const std::string& apiBaseUrl, const FindGameBody& body, FindGameCallback cb);

} // namespace surv
