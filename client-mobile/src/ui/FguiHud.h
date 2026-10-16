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

#include <memory>
#include <string>
#include <vector>

namespace ax {
class Scene;
}

namespace fairygui {
class GComponent;
class GObject;
class GGraph;
class GBasicTextField;
}

namespace surv {
class Touch;
class Game;

// In-game HUD rendered with FairyGUI objects (health/boost bars, ammo,
// weapon name, alive count, kill feed).
class FguiHud {
public:
    FguiHud();
    ~FguiHud();

    // Builds the HUD component tree under the scene's FG root. Must be called
    // after GRoot::create (see GameScene).
    void build(ax::Scene* scene);

    // Per-frame update from the game state.
    void update(float dt, bool started, bool playing, const Game* game);

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

private:
    void buildHudPanel(fairygui::GComponent* root);
    void buildResultOverlay(fairygui::GComponent* root);
    void buildKillFeed(fairygui::GComponent* root);
    void buildAliveCounter(fairygui::GComponent* root);
    void buildSquadBars(fairygui::GComponent* root);
    void buildMinimap(fairygui::GComponent* root);

    // Minimap: FGUI sprite + overlay
    fairygui::GObject* _minimapContainer = nullptr;
    fairygui::GGraph* _minimapFrame = nullptr;
    fairygui::GGraph* _minimapGasRing = nullptr;
    fairygui::GGraph* _minimapSafeZone = nullptr;
    std::vector<fairygui::GGraph*> _minimapDots;

    // Health bar: fill rect (redrawn each frame with varying width+color)
    fairygui::GObject* _healthText = nullptr;
    fairygui::GGraph* _healthFill = nullptr;

    // Boost bar: 4 segments (one GGraph each)
    fairygui::GGraph* _boostSegments[4]{};

    // Weapon + ammo display
    fairygui::GObject* _weaponText = nullptr;
    fairygui::GObject* _ammoText = nullptr;

    // Alive counter
    fairygui::GObject* _aliveText = nullptr;

    // Kill feed (6 lines)
    fairygui::GObject* _killFeedText = nullptr;

    // Result overlay
    fairygui::GObject* _resultText = nullptr;
    fairygui::GObject* _resultPanel = nullptr;
    bool _resultShown = false;

    // Squad health bars (4 team members)
    struct SquadMember {
        fairygui::GObject* container = nullptr;
        fairygui::GObject* nameText = nullptr;
        fairygui::GGraph* healthBar = nullptr;
        fairygui::GObject* statusText = nullptr;
        // Cached state
        int prevHealth = -1;
        bool prevDead = false;
        bool prevDowned = false;
        bool prevDisconnected = false;
    };
    std::array<SquadMember, 4> _squadMembers{};

    // Root panel containing all HUD elements.
    fairygui::GObject* _hudPanel = nullptr;

    bool _visible = true;
};

} // namespace surv

#endif // SURV_FGUI_HUD_H