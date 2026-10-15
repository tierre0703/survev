#pragma once
// FairyGUI-based HUD (M7/S2): replaces UiOverlay's hand-rolled HUD widgets
// with programmatically-constructed FairyGUI components.
//
// This is an incremental step toward ARCHITECTURE.md §3.1:
//   * The HUD is built from FairyGUI objects (GProgressBar, GTextField, etc.)
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
class GProgressBar;
class GGraph;
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

    fairygui::GProgressBar* _healthBar = nullptr;
    fairygui::GProgressBar* _boostBar = nullptr;
    fairygui::GObject* _healthText = nullptr;
    fairygui::GObject* _weaponText = nullptr;
    fairygui::GObject* _ammoText = nullptr;
    fairygui::GObject* _aliveText = nullptr;
    fairygui::GObject* _killFeedText = nullptr;
    fairygui::GObject* _resultText = nullptr;
    fairygui::GObject* _resultPanel = nullptr;
    fairygui::GComponent* _hudRoot = nullptr;
    fairygui::GComponent* _hudPanel = nullptr;
    bool _visible = true;
};

} // namespace surv

#endif // SURV_FGUI_HUD_H