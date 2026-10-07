#pragma once
// Native main menu (M7). Port of the web client's start screen
// (client/index.html `#start-menu-wrapper` + `client/src/main.ts`):
//
//   splash background        -> `img/splashes/main.webp`
//   #player-name-input-solo  -> ui::TextField   (config "playerName")
//   #server-select-main      -> ui::Button      (config "region", from site_info.pops)
//   #btn-start-mode-0..2     -> ui::Button      (quick start Solo/Duo/Squad)
//   #btn-join-team / create  -> ui::TeamMenu    (team room screen)
//   #btn-help                -> ui::Modal       ("How to Play")
//   sound + volume controls  -> ui::Button/ui::Slider (AudioManager)
//
// The scene owns the UI state (config/localization/team client/overlay) and
// drives the game scene through the menu-driven API added in M7:
// `GameScene::enterWithFindGame()` / `enterWithJoin()` / `leaveGame()`. The
// find_game HTTP call itself stays injected via
// `GameScene::setFindGameRequest()`.
#include "../net/Messages.h"
#include "../net/SiteInfo.h"
#include "../ui/Config.h"
#include "../ui/Device.h"
#include "../ui/Localization.h"
#include "../ui/TeamMenu.h"
#include "../ui/Ui.h"

#include "axmol.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ui {
class UiOverlay;
}

namespace surv {

class GameScene;

class MenuScene : public ax::Scene {
public:
    static MenuScene* createScene();

    ~MenuScene() override;

    bool init() override;
    void update(float delta) override;

    // App lifecycle hooks (AppDelegate): stop/start the team keep-alive and the
    // audio while backgrounded.
    void pauseMenu();
    void resumeMenu();

    // Test/debug hooks.
    bool isInMenu() const { return !_inGame; }
    bool isPending() const { return _pending; }
    const ui::Config& config() const { return *_config; }

    // M7 wiring (AppDelegate): the menu drives the sibling game scene.
    // `scene` is the scene created alongside the menu; the callbacks let the
    // menu start a match and return from it without knowing the scene graph.
    struct GameHooks {
        std::function<GameScene*()> scene;
        std::function<Game::JoinInfo()> joinInfo;
        std::function<void()> onMatchStarted;   // menu -> game switch
        std::function<void()> onReturnToMenu;   // game -> menu switch
    };
    void setGameHooks(GameHooks hooks) { _hooks = std::move(hooks); }
    const GameHooks& gameHooks() const { return _hooks; }

    // Called by the app when a match starts (menu hides) or ends (menu shows).
    void onMatchStarted();
    void onMatchEnded(const std::string& errorL10nKey = "");

    // The overlay is owned by the menu and handed to the game scene.
    ui::UiOverlay* overlay() const { return _overlay.get(); }

private:
    void buildBackground();
    void buildMenuPanel();
    void buildModals();
    void buildTeamScreen();

    void applyConfigToUI();
    void applyAudioFromConfig();
    void onConfigChanged(const std::string& key);

    void refreshUi();

    // Matchmaking (port of main.ts tryQuickStartGame / findGame / joinGame).
    void quickStart(int teamMode);
    void setError(const std::string& l10nKey, const std::string& fallback = "");
    void clearError();

    void enterTeam(bool create);
    void leaveTeam(const std::string& errorL10nKey = "");
    void refreshTeamUi();

    void populateRegions();

    std::unique_ptr<ui::UserDefaultConfigStorage> _storage;
    std::unique_ptr<ui::Config> _config;
    std::unique_ptr<ui::Localization> _loc;
    std::unique_ptr<ui::TeamMenu> _team;
    std::unique_ptr<ui::UiOverlay> _overlay;

    ax::Node* _uiRoot = nullptr;
    ax::Node* _teamRoot = nullptr;

    // Menu widgets.
    ax::Sprite* _background = nullptr;
    ax::Sprite* _logo = nullptr;
    ui::Panel* _menuPanel = nullptr;
    ui::Button* _playBtns[3] = {nullptr, nullptr, nullptr};
    ui::TextField* _nameField = nullptr;
    ui::Button* _regionBtn = nullptr;
    ui::Button* _customizeBtn = nullptr;
    ui::Button* _joinTeamBtn = nullptr;
    ui::Button* _createTeamBtn = nullptr;
    ui::Button* _helpBtn = nullptr;
    ui::Button* _soundBtn = nullptr;
    ax::Label* _errorLabel = nullptr;
    ui::Modal _helpModal;
    ui::Modal _customizeModal;

    // Team room widgets.
    ui::Panel* _teamPanel = nullptr;
    ax::Label* _teamTitle = nullptr;
    ax::Label* _teamCode = nullptr;
    ui::Button* _teamStartBtn = nullptr;
    ui::Button* _teamLeaveBtn = nullptr;
    std::vector<ax::Label*> _teamPlayerLabels;

    // Volume sliders (kept for label refresh; owned by the panel).
    std::vector<ui::Slider*> _sliders;

    SiteInfo _siteInfo;
    GameHooks _hooks;
    std::string _region = "na";
    std::string _apiUrl;
    bool _inGame = false;
    bool _pending = false;
    bool _inTeam = false;
    float _pendingTicker = 0.0f;
};

} // namespace surv
