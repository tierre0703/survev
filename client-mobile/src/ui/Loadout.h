#pragma once
// Engine-independent selection/validation port of shared/utils/loadout.ts.
#include "Config.h"
#include "../game/Game.h"
#include "../render/Defs.h"

namespace ui {

std::string sanitizePlayerName(const std::string& input);
std::string teamEndpoint(const std::string& apiUrl);
std::string teamInviteCode(const std::string& input);

class Loadout {
public:
    explicit Loadout(Config* config) : _config(config) {}
    void setAccountItems(const std::vector<std::string>& items) { _accountItems = items; }
    std::vector<std::string> items(const std::string& category) const;
    JsonValue validated() const;
    surv::Game::JoinInfo joinInfo() const;
    bool select(const std::string& category, const std::string& type, size_t emoteSlot = 0);

private:
    Config* _config;
    std::vector<std::string> _accountItems;
};

} // namespace ui
