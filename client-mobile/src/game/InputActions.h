#pragma once
// One-shot gameplay actions queued by the touch HUD (port of the action half of
// client/src/game.ts update(): game.ts checkInputs, the touch reload/interaction
// buttons, ui2.ts `uiEvents` use/drop, and the use-item binds).
//
// The web client adds these to the InputMsg it builds for the current frame.
// The native client only sends InputMsg at the server's net-sync rate
// (GameScene::updateInput), so a tap that starts and ends between two sends
// would be lost. Queue the actions instead and drain them into the next
// InputMsg: `InputMsg::addInput` already caps at 7 entries and dedupes, exactly
// like shared/net/inputMsg.ts.
//
// Engine-independent so `surv_tests` can cover it without axmol.
#include "../core/GameConfig.h"
#include "../net/Messages.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace surv {

class InputActions {
public:
    // InputMsg::addInput stops at 7 entries (shared/net/inputMsg.ts).
    static constexpr size_t kMaxInputs = 7;

    // Queues one Input_* action. Duplicates and overflow are dropped, matching
    // the web's addInput().
    void press(uint8_t input) {
        if (_inputs.size() >= kMaxInputs) {
            return;
        }
        if (std::find(_inputs.begin(), _inputs.end(), input) != _inputs.end()) {
            return;
        }
        _inputs.push_back(input);
    }

    // Queues the "use" action (ui2.ts uiEvents action == "use" for heal/boost/
    // scope, or the UseBandage/... binds). The server applies it with
    // InputMsg.useItem.
    void use(const std::string& item) {
        if (!item.empty()) {
            _useItem = item;
        }
    }

    bool empty() const { return _inputs.empty() && _useItem.empty(); }
    const std::vector<uint8_t>& inputs() const { return _inputs; }
    const std::string& useItem() const { return _useItem; }

    // Writes the queued actions into `msg` (called right before sending it).
    void applyTo(InputMsg& msg) const {
        for (uint8_t input : _inputs) {
            msg.addInput(input);
        }
        if (!_useItem.empty()) {
            msg.useItem = _useItem;
        }
    }

    void clear() {
        _inputs.clear();
        _useItem.clear();
    }

private:
    std::vector<uint8_t> _inputs;
    std::string _useItem;
};

// Input_* action for a weapon slot tap (ui2.ts WeaponSlotToBind).
inline uint8_t inputForWeaponSlot(int slot) {
    switch (slot) {
        case WeaponSlot_Primary: return Input_EquipPrimary;
        case WeaponSlot_Secondary: return Input_EquipSecondary;
        case WeaponSlot_Melee: return Input_EquipMelee;
        case WeaponSlot_Throwable: return Input_EquipThrowable;
        default: return Input_Count;
    }
}

} // namespace surv
