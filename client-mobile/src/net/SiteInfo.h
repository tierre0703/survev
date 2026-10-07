#pragma once
// Native site info / matchmaking API for the menu (port of the site_info and
// find_game pieces of client/src/main.ts + api.ts that the main menu drives).
//
// Like net/Api.h this stays engine-facing only at the HTTP boundary
// (SiteApi.cpp uses ax::network::HttpClient), so the menu logic that consumes
// it can be host-tested.
#include "Net.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace surv {

// One entry of site_info.modes.
struct GameModeInfo {
    std::string mapName;
    int teamMode = 0;
    bool enabled = true;
};

// site_info region population entry (used to detect the local region).
struct PopInfo {
    std::string region;
    int playerCount = 0;
    std::string l10n;
};

// GET /api/site_info (subset the menu needs).
struct SiteInfo {
    bool ok = false;
    std::vector<GameModeInfo> modes;
    std::vector<PopInfo> pops;
    std::string country;
    std::string gitRevision;
    bool captchaEnabled = false;
    std::string clientTheme = "main";

    // Index of the first enabled mode with the given team mode, or -1.
    int modeIndexForTeamMode(int teamMode) const;
    bool hasTeamMode(int teamMode) const;
};

using SiteInfoCallback = std::function<void(SiteInfo)>;

// Fetches `apiBaseUrl + "/api/site_info"` and invokes `cb` on the main thread.
void fetchSiteInfo(const std::string& apiBaseUrl, SiteInfoCallback cb);

} // namespace surv