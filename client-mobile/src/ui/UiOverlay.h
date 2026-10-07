#pragma once
// Native in-game UI (M7): the pause/settings overlay plus the HUD.
//
// Web counterpart: `#ui-game` in client/index.html (pause menu, touch style
// toggles, aim line, sound + volume sliders, quit) and the HUD built by
// client/src/ui/ui2.ts (health/boost, ammo, killfeed, alive count).
//
// The overlay owns its axmol nodes and is driven by GameScene::update(); it
// never talks to the network itself.
#include "Config.h"
#include "Device.h"
#include "Localization.h"
#include "Ui.h"

#include <functional>
#include <string>
#include <vector>

namespace ax {
class Scene;
class Node;
} // namespace ax

namespace surv {
class Game;
class Touch;
}

namespace ui {

class UiOverlay {
public:
    UiOverlay() = default;
    ~UiOverlay();

    // Builds the HUD + pause menu under `scene`. `config`/`localization` are
    // owned by the menu scene and must outlive the overlay.
    void build(ax::Scene* scene, Config* config, Localization* localization,
               surv::Touch* touch);
    // Per-frame HUD refresh + pause-menu visibility.
    void update(float dt, bool started, bool playing, bool connected, const surv::Game* game);

    void show();
    void hide();
    bool isOpen() const { return _visible; }
    void toggle();

    // The overlay drives these (the scene wires them to the game).
    std::function<void()> onQuit;
    std::function<void(bool locked)> onMoveStyleChanged;
    std::function<void(bool locked)> onAimStyleChanged;
    std::function<void(bool enabled)> onAimLineChanged;
    std::function<void(bool muted)> onMuteChanged;
    std::function<void(float)> onMasterVolume;
    std::function<void(float)> onSoundVolume;
    std::function<void(float)> onMusicVolume;

    // Sets the localized error/status line shown in the pause menu (used by the
    // scene for matchmaking/disconnect feedback).
    void setMenuError(const std::string& l10nKey, const std::string& fallback);

    // Test seam: expose the HUD values the update pushed.
    float debugHealth() const { return _health; }
    int debugAlive() const { return _aliveCount; }

private:
    void buildHud(ax::Scene* scene);
    void buildPauseMenu(ax::Scene* scene);
    void refreshHud(const surv::Game* game, bool started, bool playing);

    Config* _config = nullptr;
    Localization* _loc = nullptr;
    surv::Touch* _touch = nullptr;

    ax::Node* _hudRoot = nullptr;
    ax::Node* _pauseRoot = nullptr;
    bool _visible = false;

    // HUD widgets.
    Panel* _hudPanel = nullptr;
    ax::DrawNode* _healthBar = nullptr;
    ax::DrawNode* _boostBar = nullptr;
    ax::Label* _healthText = nullptr;
    ax::Label* _weaponText = nullptr;
    ax::Label* _ammoText = nullptr;
    ax::Label* _aliveText = nullptr;

    // Pause menu state.
    Button* _moveStyleBtn = nullptr;
    Button* _aimStyleBtn = nullptr;
    Button* _aimLineBtn = nullptr;
    Button* _soundBtn = nullptr;
    ax::Label* _errorText = nullptr;

    float _health = 0.0f;
    float _boost = 0.0f;
    int _aliveCount = 0;
    std::string _weapon = "";
    int _ammo = -1;
};

} // namespace ui