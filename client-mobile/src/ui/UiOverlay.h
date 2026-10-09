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

// --- Main menu -------------------------------------------------------------
// The native start screen (port of `#start-menu-wrapper` + main.ts). Built as
// a standalone class so the axmol scene wrapper (src/app/MenuScene.h) never has
// to name the `ui::` widgets from a translation unit with `using namespace ax`.
class StartMenu {
public:
    // Builds the whole menu (background, logo, name/region/play buttons, news
    // box, bottom-right settings/mute icons, modals) under `root`.
    void build(ax::Node* root, Config* config, Localization* loc);
    // Hides all menu chrome (match started) / shows it again (match ended).
    void setVisible(bool visible);
    void setTeamVisible(bool team);
    void refresh();
    // Enables/disables one of the Play Solo/Duo/Squad buttons (mode 0/1/2).
    void setPlayEnabled(int index, bool enabled);
    // Targeted refresh hooks used by the scene (each widget owns its own
    // renderer, so the scene only updates what changed).
    void setRegionLabel(const std::string& label);
    void setMuteState(bool muted);
    void setLanguageLabel(const std::string& label);
    // The invite code collected by the join-team modal (consumed by the scene).
    const std::string& pendingRoomCode() const;
    // Localized status line (menu errors / "Finding game...").
    void setError(const std::string& l10nKey, const std::string& fallback);
    void clearError();

    std::function<void(int teamMode)> onQuickStart;
    std::function<void(bool create)> onTeamRequested;

private:
    void buildPanel();
    void buildNewsBox();
    void buildBottomIcons();
    void buildSettingsModal();
    void buildHelpModal();
    void buildJoinModal();
    void requestTeam(bool create);

    Config* _config = nullptr;
    Localization* _loc = nullptr;
    ax::Node* _root = nullptr;
    ax::Sprite* _background = nullptr;
    ax::LayerColor* _overlayDim = nullptr;
    ax::Sprite* _logo = nullptr;
    Panel* _panel = nullptr;
    TextField* _nameField = nullptr;
    Button* _regionBtn = nullptr;
    Button* _playBtns[3] = {nullptr, nullptr, nullptr};
    Button* _joinTeamBtn = nullptr;
    Button* _createTeamBtn = nullptr;
    Button* _customizeBtn = nullptr;
    Button* _helpBtn = nullptr;
    Button* _settingsBtn = nullptr;
    Button* _muteBtn = nullptr;
    Panel* _newsPanel = nullptr;
    ax::Label* _errorLabel = nullptr;
    Modal _settingsModal;
    Modal _helpModal;
    Modal _joinModal;
    TextField* _roomField = nullptr;
    Button* _languageBtn = nullptr;
    Button* _modalMuteBtn = nullptr;
    std::vector<Slider*> _sliders;
    std::string _pendingRoomCode;
    bool _visible = true;
};

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
    ax::Label* _killFeedText = nullptr;
    ax::Label* _resultText = nullptr;
    Button* _resultQuit = nullptr;
    std::vector<Slider*> _volumeSliders;
    void syncSettings();

    // Pause menu state.
    Button* _moveStyleBtn = nullptr;
    Button* _aimStyleBtn = nullptr;
    Button* _aimLineBtn = nullptr;
    Button* _soundBtn = nullptr;
    ax::Label* _errorText = nullptr;
    std::function<void(const std::string&, const std::string&)> _onMenuError;

    float _health = 0.0f;
    float _boost = 0.0f;
    int _aliveCount = 0;
    std::string _weapon = "";
    int _ammo = -1;
};

} // namespace ui