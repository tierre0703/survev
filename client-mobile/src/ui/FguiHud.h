#pragma once
// FairyGUI-based HUD (M7/S2): replaces UiOverlay's hand-rolled HUD widgets
// with programmatically-constructed FairyGUI components.
//
// This is an incremental step toward ARCHITECTURE.md §3.1:
//   * The HUD is built from FairyGUI objects (GGraph, GBasicTextField, etc.)
//     that mirror the layout of the web client's HUD.
//   * Later (when the FairyGUI Editor is available and HUD_pkg is authored),
//     this file is replaced by a simple UIPackage::createObject("HUD_pkg", "Main").
//
// The adapter rule (ARCHITECTURE.md §3.1 rule 4) still applies: nothing outside
// src/render/AxmolPixi.h may touch axmol draw APIs directly. FairyGUI objects
// are engine-level, so this file lives alongside the engine adapter.
#ifndef SURV_FGUI_HUD_H
#define SURV_FGUI_HUD_H

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "../core/Vec2.h"
#include "../net/Messages.h"         // WeaponData, PerkData
#include "FguiEmoteWheel.h"

namespace ax {
class Scene;
}

namespace fairygui {
class GBasicTextField;
class GComponent;
class GGraph;
class GObject;
class GRichTextField;
}

namespace surv {
class Touch;
class Game;

// Inventory UI v2 — in-game HUD rendered with FairyGUI objects.
// Mirrors the web client's ui2.ts: weapon cards, gear slots, perk slots,
// scope inventory, loot counts (ammo/heals/throwables), pickup/kill messages,
// multi-line kill feed, interaction prompt, and the existing health/boost bars.
class FguiHud {
public:
    FguiHud();
    ~FguiHud();

    // Builds the HUD component tree under the scene's FG root. Must be called
    // after GRoot::create (see GameScene).
    void build(ax::Scene* scene);

    // Per-frame update from the game state.
    void update(float dt, bool started, bool playing, const Game* game);

    // Emote wheel: access for GameScene to feed touch input.
    // Returns true if the emote wheel consumed the touch event.
    bool feedEmoteWheel(float screenX, float screenY, bool touched);
    // Set the emote loadout (4 emote type names from JoinedMsg).
    void setEmoteLoadout(const std::array<std::string, 4>& emotes);
    // Check if the emote wheel is currently visible.
    bool emoteWheelActive() const;
    // Consume any pending emote/ping to send to the server.
    // Stores the result in outType ("emote"/"ping") and outId.
    bool consumeEmoteOutput(std::string& outType, std::string& outId, Vec2* worldPos);

    // Show/hide the whole HUD.
    void setVisible(bool visible);
    bool isVisible() const;

    // Per-frame HUD data updates (called from update()).
    void refreshHealth(float health);
    void refreshBoost(float boost);
    void refreshWeapon(const std::string& weapon, int ammo, int reserve);
    void refreshAlive(int count, int teamCount);
    void refreshKillFeed(const std::string& text);
    void refreshResult(const std::string& text);
    void showResult(bool show);

    // Inventory v2 methods
    void refreshWeaponSlots(const std::vector<WeaponData>& weapons, int curWeapIdx);
    void refreshLootCounts(const std::vector<uint16_t>& inventory);
    void refreshGear(const std::string& chest, const std::string& helmet, const std::string& backpack);
    void refreshPerks(const std::vector<PerkData>& perks);
    void refreshScopes(const std::string& scope, const std::vector<uint16_t>& inventory);
    void refreshKillMessage(const std::string& text, const std::string& count);
    void refreshPickupMessage(const std::string& text);
    void displayPickupMessage(uint8_t type);
    void displayKillMessage(const std::string& text, const std::string& count);

    // Minimap update — called each frame from update().
    void refreshMinimap(const Game* game);

    // Squad health bars — called each frame from update().
    void refreshSquad(const Game* game);

// Inter-process: the emote wheel's input/output state lives here so the
    // GameScene can feed raw touch data into it each frame.
    surv::FguiEmoteWheel* emoteWheel() { return _emoteWheel.get(); }

private:
    void buildHudPanel(fairygui::GComponent* root);
    void buildResultOverlay(fairygui::GComponent* root);
    void buildKillFeed(fairygui::GComponent* root);
    void buildAliveCounter(fairygui::GComponent* root);
    void buildSquadBars(fairygui::GComponent* root);
    void buildMinimap(fairygui::GComponent* root);
    void buildWeaponSlots(fairygui::GComponent* root);
    void buildGearSlots(fairygui::GComponent* root);
    void buildPerkSlots(fairygui::GComponent* root);
    void buildScopeSlots(fairygui::GComponent* root);
    void buildLootGrid(fairygui::GComponent* root);
    void buildKillMessage(fairygui::GComponent* root);
    void buildPickupMessage(fairygui::GComponent* root);
    void buildInteractionPrompt(fairygui::GComponent* root);

    // ---- Weapon slots (4: Primary, Secondary, Melee, Throwable) ----
    static constexpr int kWeaponSlotCount = 4;
    struct WeaponSlotWidgets {
        fairygui::GGraph* bg = nullptr;
        fairygui::GBasicTextField* nameText = nullptr;
        fairygui::GBasicTextField* ammoText = nullptr;
        fairygui::GBasicTextField* keybindText = nullptr;
        fairygui::GGraph* equippedHighlight = nullptr;
    };
    std::array<WeaponSlotWidgets, kWeaponSlotCount> _weaponSlots{};

    // ---- Gear slots (helmet, chest, backpack) ----
    static constexpr int kGearSlotCount = 3;
    static constexpr std::array<const char*, kGearSlotCount> kGearTypeNames = {"helmet", "chest", "backpack"};
    struct GearSlotWidgets {
        fairygui::GGraph* bg = nullptr;
        fairygui::GBasicTextField* levelText = nullptr;
        fairygui::GBasicTextField* nameText = nullptr;
    };
    std::array<GearSlotWidgets, kGearSlotCount> _gearSlots{};

    // ---- Perk slots (up to 4) ----
    static constexpr int kPerkSlotCount = 4;
    struct PerkSlotWidgets {
        fairygui::GGraph* bg = nullptr;
        fairygui::GBasicTextField* nameText = nullptr;
    };
    std::array<PerkSlotWidgets, kPerkSlotCount> _perkSlots{};

    // ---- Scope slots (5: 1x–15x) ----
    static constexpr int kScopeSlotCount = 5;
    static constexpr std::array<const char*, kScopeSlotCount> kScopeTypeNames = {
        "1xscope", "2xscope", "4xscope", "8xscope", "15xscope"
    };
    struct ScopeSlotWidgets {
        fairygui::GGraph* bg = nullptr;
        fairygui::GBasicTextField* labelText = nullptr;
    };
    std::array<ScopeSlotWidgets, kScopeSlotCount> _scopeSlots{};

    // ---- Loot grid (ammo/heals/throwables) ----
    // Indices into kBagSizeKeys order: ammo[0-7], throwables[8-15], heals[16-19], scopes[20-24]
    static constexpr int kLootGridCount = 25; // matches kBagSizeKeys size
    struct LootSlotWidgets {
        fairygui::GGraph* bg = nullptr;
        fairygui::GBasicTextField* nameText = nullptr;
        fairygui::GBasicTextField* countText = nullptr;
    };
    std::array<LootSlotWidgets, kLootGridCount> _lootSlots{};

    // ---- Kill message (center screen) ----
    fairygui::GComponent* _killMessageBg = nullptr;
    fairygui::GBasicTextField* _killMessageText = nullptr;
    fairygui::GBasicTextField* _killMessageCount = nullptr;

    // ---- Pickup message (below center) ----
    fairygui::GBasicTextField* _pickupMessageText = nullptr;

    // ---- Interaction prompt ----
    fairygui::GComponent* _interactionBg = nullptr;
    fairygui::GBasicTextField* _interactionKeyText = nullptr;
    fairygui::GBasicTextField* _interactionDescText = nullptr;

    // Minimap: FGUI sprite + overlay
    fairygui::GComponent* _minimapContainer = nullptr;
    fairygui::GGraph* _minimapFrame = nullptr;
    fairygui::GGraph* _minimapGasRing = nullptr;
    fairygui::GGraph* _minimapSafeZone = nullptr;
    std::vector<fairygui::GGraph*> _minimapDots;

    // Health bar: fill rect (redrawn each frame with varying width+color)
    fairygui::GBasicTextField* _healthText = nullptr;
    fairygui::GGraph* _healthFill = nullptr;

    // Boost bar: 4 segments (one GGraph each)
    fairygui::GGraph* _boostSegments[4]{};

    // Weapon + ammo display (compact top-right of HUD panel)
    fairygui::GBasicTextField* _weaponText = nullptr;
    fairygui::GBasicTextField* _ammoText = nullptr;

    // Alive counter
    fairygui::GBasicTextField* _aliveText = nullptr;

    // Kill feed (multi-line via multi-line text field)
    fairygui::GBasicTextField* _killFeedText = nullptr;

    // Result overlay
    fairygui::GBasicTextField* _resultText = nullptr;
    fairygui::GComponent* _resultPanel = nullptr;
    bool _resultShown = false;

    // Squad health bars (4 team members)
    struct SquadMember {
        fairygui::GComponent* container = nullptr;
        fairygui::GBasicTextField* nameText = nullptr;
        fairygui::GGraph* healthBar = nullptr;
        fairygui::GBasicTextField* statusText = nullptr;
        // Cached state
        int prevHealth = -1;
        bool prevDead = false;
        bool prevDowned = false;
        bool prevDisconnected = false;
    };
    std::array<SquadMember, 4> _squadMembers{};

    // Emote wheel
    std::unique_ptr<surv::FguiEmoteWheel> _emoteWheel;
    surv::EmoteWheelInput _emoteWheelInput{};

    // Root panel containing all HUD elements
    fairygui::GComponent* _hudPanel = nullptr;

    // Message ticker state (mirrors ui2.ts pickupMessage/killMessage)
    float _pickupTicker = 0.0f;
    float _pickupDuration = 0.0f;
    std::string _pickupMessage;
    float _killMsgTicker = 0.0f;
    float _killMsgDuration = 0.0f;
    std::string _killMsgText;
    std::string _killMsgCount;

    bool _visible = true;
};

} // namespace surv

#endif // SURF_FGUI_HUD_H