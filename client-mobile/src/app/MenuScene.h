#pragma once
// Native main menu (M7). Port of the web client's start screen
// (client/index.html `#start-menu-wrapper` + `client/src/main.ts`):
//
//   splash background        -> `img/splashes/main.webp` (#background + overlay)
//   #start-row-header        -> the survev logo (centered)
//   #player-name-input-solo  -> ui::TextField   (config "playerName")
//   #server-select-main      -> ui::Button      (config "region", from site_info.pops)
//   #btn-start-mode-0..2     -> ui::Button      (quick start Solo/Duo/Squad)
//   #btn-join-team / create  -> ui::TeamMenu    (team room screen)
//   #btn-help                -> ui::Modal       ("How to Play")
//   #news-block              -> ui::Panel       (bundled news entries)
//   #start-bottom-right      -> settings + mute icons (bottom-right)
//
// The web client lays `#start-menu` out in the horizontal centre of
// `#start-row-top` with the news column to its right; the native port reproduces
// that (the menu block is centred, the news box sits to its right) and is locked
// to landscape (AppDelegate + the manifest).
//
// The scene owns the UI state (config/localization/team client/overlay) and
// drives the game scene through the menu-driven API added in M7:
// `GameScene::enterWithFindGame()` / `enterWithJoin()` / `leaveGame()`. The
// find_game HTTP call itself stays injected via
// `GameScene::setFindGameRequest()`.
#include "../net/Messages.h"
#include "../game/Game.h"
#include "../net/SiteInfo.h"
#include "../ui/Config.h"
#include "../ui/Device.h"
#include "../ui/Localization.h"
#include "../ui/TeamMenu.h"
#include "../ui/Ui.h"
#include "../ui/LoadoutMenu.h"
#include "../ui/News.h"

#include "axmol.h"
#include "../ui/UiOverlay.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace surv {

// The global UI toolkit (declared in ui/Ui.h / UiOverlay.h). Aliased because
// `ui::` inside this namespace would otherwise collide with the vestigial
// `surv::ui` forward declarations that UiOverlay.h keeps for legacy includes.
namespace uikit = ::ui;

class GameScene;

class StartMenu;

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
    const uikit::Config& config() const { return *_config; }
    Game::JoinInfo joinInfo() const { return uikit::Loadout(_config.get()).joinInfo(); }
    void bindGame(GameScene* game);
    // The scaled design-space UI root (also used by the in-game HUD/pause
    // overlay so both share one coordinate system).
    ax::Node* uiRootNode() const { return _uiRoot; }

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

    // Aborts an in-flight matchmaking request and restores the main menu. Wired
    // to the matchmaking overlay's "Back to Main Menu" button.
    void cancelPending();

    // The overlay is owned by the menu and handed to the game scene.
    uikit::UiOverlay* overlay() const { return _overlay.get(); }

private:
    void buildTeamScreen();
    void buildMenuChrome();
    uikit::Button* makeMenuButton(const std::string& label, float w, float h, bool green, float fontSize);

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

    std::unique_ptr<uikit::UserDefaultConfigStorage> _storage;
    std::unique_ptr<uikit::Config> _config;
    std::unique_ptr<uikit::Localization> _loc;
    std::unique_ptr<uikit::TeamMenu> _team;
    std::unique_ptr<uikit::UiOverlay> _overlay;

    ax::Node* _uiRoot = nullptr;
    ax::Node* _teamRoot = nullptr;

    // Main-menu chrome (the centred menu block, the news box, the bottom-right
    // icons and the menu modals) lives in a helper class so this scene wrapper
    // never has to name the `ui::` widgets from a `using namespace ax` unit.
    std::unique_ptr<uikit::StartMenu> _startMenu;

    ax::Label* _errorLabel = nullptr;
    uikit::Button* _teamRegionBtn = nullptr;
    uikit::Button* _teamModeBtn = nullptr;
    uikit::Button* _teamAutoFillBtn = nullptr;
    ax::Label* _teamErrorLabel = nullptr;
    std::shared_ptr<bool> _alive = std::make_shared<bool>(true);

    // Team room widgets.
    uikit::Panel* _teamPanel = nullptr;
    ax::Label* _teamTitle = nullptr;
    ax::Label* _teamCode = nullptr;
    ax::Label* _teamStatus = nullptr;
    uikit::Button* _teamStartBtn = nullptr;
    uikit::Button* _teamLeaveBtn = nullptr;
    std::vector<ax::Label*> _teamPlayerLabels;

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
