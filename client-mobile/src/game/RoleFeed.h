#pragma once
// game.ts RoleAnnouncement -> kill-feed line (getRoleAssignedKillFeedText /
// getRoleKilledKillFeedText / getRoleKillFeedColor). Engine-independent so the
// host tests can cover the text/colour rules.
#include "../core/GameConfig.h"
#include "../render/Defs.h"

#include <string>

namespace surv {

struct RoleFeedLine {
    bool show = false;
    std::string text;
    uint32_t color = 0xffffff;
};

inline RoleFeedLine roleFeedLine(const RoleRenderDef& role, const std::string& roleType,
                                 bool assigned, bool killed, uint8_t teamId,
                                 const std::string& playerName, const std::string& killerName) {
    RoleFeedLine out;
    if (assigned) {
        if (!role.killFeedAssign) {
            return out;
        }
    } else if (killed) {
        if (!role.killFeedDead) {
            return out;
        }
    } else {
        return out;
    }

    std::string roleName = role.name.empty() ? roleType : role.name;
    if (roleType == "leader") {
        roleName = teamId == 1 ? role.nameRed : role.nameBlue;
        if (roleName.empty()) {
            roleName = role.name.empty() ? roleType : role.name;
        }
    }
    if (roleName.empty()) {
        roleName = roleType;
    }

    if (assigned) {
        out.text = playerName + " promoted to " + roleName + "!";
    } else if (!killerName.empty()) {
        out.text = killerName + " killed " + roleName + "!";
    } else {
        out.text = roleName + " is dead!";
    }

    if (role.hasKillFeedColor) {
        out.color = role.killFeedColor;
    } else if (teamId >= 1 && teamId <= 2) {
        out.color = kTeamColors[teamId - 1];
    }
    out.show = true;
    return out;
}

} // namespace surv
