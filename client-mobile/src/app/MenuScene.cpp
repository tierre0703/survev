#include "MenuScene.h"
#include "GameScene.h"
#include "DevConfig.h"
#include "../net/WebSocketConnection.h"
#include "../net/WsUrl.h"
#include "../ui/Files.h"
#include "../ui/UiOverlay.h"
#include "../audio/AudioManager.h"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace surv {

namespace menuui = ::ui;

namespace {

// Localization helper (key -> translated text, else fallback literal).
std::string tr(menuui::Localization* loc, const std::string& key, const std::string& fallback) {
    const std::string text = loc ? loc->translate(key) : "";
    return text.empty() ? fallback : text;
}

std::string regionDisplayName(menuui::Localization* loc, const std::string& region) {
    if (loc) {
        const std::string translated = loc->translate("index-" + region);
        if (!translated.empty()) {
            return translated;
        }
    }
    std::string upper = region;
    std::transform(upper.begin(), upper.end(), upper.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return upper;
}

} // namespace

MenuScene* MenuScene::createScene() {
    auto* scene = new MenuScene();
    if (scene && scene->init()) {
        scene->autorelease();
        return scene;
    }
    AX_SAFE_DELETE(scene);
    return nullptr;
}

MenuScene::~MenuScene() { *_alive = false; }

void MenuScene::bindGame(GameScene* game) {
    GameHooks hooks;
    hooks.scene = [game] { return game; };
    hooks.joinInfo = [this] { return joinInfo(); };
    hooks.onMatchStarted = [game] { game->setVisible(true); };
    hooks.onReturnToMenu = [game] { game->setVisible(false); };
    setGameHooks(std::move(hooks));
    // The overlay/game reuse the menu's scaled design-space root so HUD and
    // pause geometry (and the toolkit's tap mapping) match the menu exactly.
    game->setUiRoot(_uiRoot);
    _overlay->build(game, _config.get(), _loc.get(), game->getTouch());
    game->setOverlay(_overlay.get());
    _overlay->onQuit = [this, game] { game->leaveGame(); onMatchEnded(); };
    game->onMatchStarted = [this] { onMatchStarted(); };
    game->onMatchEnded = [this](const std::string& key) { onMatchEnded(key); };
    game->onError = [this](const std::string& key, const std::string& fallback) { setError(key, fallback); };
    game->getTouch()->moveStyle = _config->getString("touchMoveStyle") == "locked"
        ? TouchStyle::Locked : TouchStyle::Anywhere;
    game->getTouch()->aimStyle = _config->getString("touchAimStyle") == "locked"
        ? TouchStyle::Locked : TouchStyle::Anywhere;
    game->getTouch()->touchAimLine = _config->boolOrDefault("touchAimLine");
    game->getTouch()->display = false;
    applyAudioFromConfig();
}

bool MenuScene::init() {
    if (!Scene::init()) {
        return false;
    }
    const ax::Size visible = ax::Director::getInstance()->getVisibleSize();
    menuui::Device::get().resize(visible.width, visible.height);

    // --- config + localization ------------------------------------------
    _storage = std::make_unique<menuui::UserDefaultConfigStorage>("surviv_config");
    _config = std::make_unique<menuui::Config>(_storage.get());
    _config->load();

    _loc = std::make_unique<menuui::Localization>();
    const std::string english = menuui::Files::readText("l10n/en.json");
    if (!english.empty()) {
        _loc->registerEnglish(english);
    } else {
        AXLOGW("Missing l10n/en.json; UI strings fall back to keys");
    }
    _loc->setLoader([](const std::string& locale) {
        return menuui::Files::readText("l10n/" + locale + ".json");
    });
    _loc->setLocale(_config->getString("language", "en"));

    _apiUrl = ax::UserDefault::getInstance()->getStringForKey(dev::kKeyApiUrl, dev::kApiBaseUrl);
    _region = _config->getString("region", "na");

    // --- UI --------------------------------------------------------------
    _uiRoot = menuui::uiRoot(this, 10);
    menuui::layoutUiRoot(_uiRoot, ax::Vec2(visible.width, visible.height),
                         ax::Vec2(menuui::kit::designWidth(), menuui::kit::designHeight()));
    _uiRoot->setTag(menuui::kUiRootTag); // shared with UiOverlay's HUD/pause
    // --- main-menu chrome (web `#start-menu-wrapper`) ---------------------
    buildMenuChrome();

    // --- in-game overlay (handed to the game scene) ----------------------
    _overlay = std::make_unique<menuui::UiOverlay>();

    // --- team client -----------------------------------------------------
    _team = std::make_unique<menuui::TeamMenu>();
    _team->setFactory(createWebSocketConnectionTo);
    _team->setUrl(menuui::teamEndpoint(_apiUrl));
    _team->onRoomChanged = [this] { refreshTeamUi(); };
    _team->onError = [this](menuui::TeamErrorType type, const std::string&) {
        leaveTeam(menuui::teamErrorL10n(type));
    };
    _team->onLostConnection = [this] { leaveTeam(menuui::teamErrorL10n(menuui::TeamErrorType::LostConn)); };
    _team->onPlay = [this](const menuui::TeamMenu::MatchData& match) {
        if (_inGame || match.urls.empty()) {
            return;
        }
        if (GameScene* game = _hooks.scene ? _hooks.scene() : nullptr) {
            _pending = true;
            clearError();
            refreshUi();
            game->setJoinInfo(_hooks.joinInfo ? _hooks.joinInfo() : Game::JoinInfo{});
            game->enterWithJoin(match.urls, match.joinToken);
        }
    };

    populateRegions();
    buildTeamScreen();
    applyConfigToUI();
    _config->addListener([this](const std::string& key) { onConfigChanged(key); });
    refreshUi();

    this->scheduleUpdate();
    return true;
}

// ---------------------------------------------------------------------------
// Main-menu chrome (the StartMenu helper builds the web `#start-menu-wrapper`)
// ---------------------------------------------------------------------------
void MenuScene::buildMenuChrome() {
    _startMenu = std::make_unique<menuui::StartMenu>();
    _startMenu->build(_uiRoot, _config.get(), _loc.get());
    _startMenu->onQuickStart = [this](int teamMode) { quickStart(teamMode); };
    _startMenu->onTeamRequested = [this](bool create) { enterTeam(create); };
}

// ---------------------------------------------------------------------------
// Team screen
// ---------------------------------------------------------------------------
void MenuScene::buildTeamScreen() {
    // Web `#team-menu`: invite link/code, the roster, region + queue-mode +
    // auto-fill properties and the green Play button.
    const float panelW = 620.0f;
    const float panelH = 560.0f;
    _teamRoot = ax::Node::create();
    _teamRoot->setContentSize(ax::Size(menuui::kit::designWidth(), menuui::kit::designHeight()));
    _teamRoot->setVisible(false);
    _uiRoot->addChild(_teamRoot, 20);

    _teamPanel = menuui::Panel::create(panelW, panelH);
    _teamPanel->setBackgroundColor(ax::Color3B(0, 0, 0), 0.8f);
    _teamPanel->placeCenter();
    _teamRoot->addChild(_teamPanel);

    _teamTitle = menuui::makeLabel(tr(_loc.get(), "index-create-team", "Team"), 24, true);
    _teamTitle->setAnchorPoint(ax::Vec2(0.5f, 1.0f));
    _teamTitle->setPosition(ax::Vec2(panelW * 0.5f, menuui::kit::fromCssY(16.0f, panelH)));
    _teamTitle->setTextColor(ax::Color4B::WHITE);
    _teamPanel->addChild(_teamTitle);

    _teamCode = menuui::makeLabel("", 22, true);
    _teamCode->setAnchorPoint(ax::Vec2(0.5f, 1.0f));
    _teamCode->setPosition(ax::Vec2(panelW * 0.5f, menuui::kit::fromCssY(56.0f, panelH)));
    _teamCode->setTextColor(ax::Color4B(140, 220, 140, 255));
    _teamPanel->addChild(_teamCode);

    _teamStatus = menuui::makeLabel("", 15, false);
    _teamStatus->setAnchorPoint(ax::Vec2(0.5f, 1.0f));
    _teamStatus->setPosition(ax::Vec2(panelW * 0.5f, menuui::kit::fromCssY(84.0f, panelH)));
    _teamStatus->setTextColor(ax::Color4B(200, 200, 200, 255));
    _teamPanel->addChild(_teamStatus);

    // Roster header (web `#team-menu-members`).
    auto* members = menuui::makeLabel(tr(_loc.get(), "index-players", "Players"), 16, true);
    members->setAnchorPoint(ax::Vec2(0.0f, 1.0f));
    members->setTextColor(ax::Color4B::WHITE);
    members->setPosition(ax::Vec2(30.0f, menuui::kit::fromCssY(112.0f, panelH)));
    _teamPanel->addChild(members);

    // Region / Mode / Auto Fill (web `#team-menu-options`), leader-editable.
    _teamRegionBtn = makeMenuButton("Region", 180, 42, false, 15);
    _teamModeBtn = makeMenuButton("Mode", 180, 42, false, 15);
    _teamAutoFillBtn = makeMenuButton("Auto Fill", 180, 42, false, 15);
    menuui::Button* props[] = {_teamRegionBtn, _teamModeBtn, _teamAutoFillBtn};
    for (int i = 0; i < 3; ++i) {
        _teamPanel->addChild(props[i]);
        props[i]->setCssPosition(28 + i * 192, 340);
    }
    _teamRegionBtn->onClick = [this] {
        if (_siteInfo.pops.empty()) return;
        size_t index = 0;
        for (size_t i = 0; i < _siteInfo.pops.size(); ++i) {
            if (_siteInfo.pops[i].region == _team->roomData().region) index = i;
        }
        _team->setRoomRegion(_siteInfo.pops[(index + 1) % _siteInfo.pops.size()].region);
        refreshTeamUi();
    };
    _teamModeBtn->onClick = [this] {
        const auto& modes = _team->roomData().enabledGameModeIdxs;
        if (modes.empty()) return;
        auto found = std::find(modes.begin(), modes.end(), _team->roomData().gameModeIdx);
        const size_t next = found == modes.end() ? 0 : (found - modes.begin() + 1) % modes.size();
        _team->setRoomGameMode(modes[next]);
        refreshTeamUi();
    };
    _teamAutoFillBtn->onClick = [this] {
        _team->setRoomAutoFill(!_team->roomData().autoFill);
        refreshTeamUi();
    };

    _teamStartBtn = makeMenuButton(tr(_loc.get(), "index-play", "Play"), 260.0f, 52.0f, true, 18);
    _teamStartBtn->setCssPosition((panelW - 260.0f) * 0.5f, panelH - 128.0f);
    _teamStartBtn->onClick = [this] {
        if (_team) {
            _team->tryStartGame();
        }
    };
    _teamPanel->addChild(_teamStartBtn);

    _teamLeaveBtn = makeMenuButton(tr(_loc.get(), "index-leave-team", "Leave Team"), 260.0f, 48.0f, false, 16);
    _teamLeaveBtn->setCssPosition((panelW - 260.0f) * 0.5f, panelH - 68.0f);
    _teamLeaveBtn->onClick = [this] { leaveTeam(); };
    _teamPanel->addChild(_teamLeaveBtn);

    _teamErrorLabel = menuui::makeLabel("", 15, false);
    _teamErrorLabel->setAnchorPoint(ax::Vec2(0.5f, 1.0f));
    _teamErrorLabel->setPosition(ax::Vec2(310, menuui::kit::fromCssY(400.0f, panelH)));
    _teamErrorLabel->setTextColor(ax::Color4B(255, 120, 120, 255));
    _teamPanel->addChild(_teamErrorLabel);
}

menuui::Button* MenuScene::makeMenuButton(const std::string& label, float w, float h,
                                          bool green, float fontSize) {
    auto* button = menuui::Button::create(label, w, h);
    button->setColors(green ? ax::Color3B(131, 175, 80) : ax::Color3B(122, 122, 122),
                      green ? ax::Color3B(91, 122, 56) : ax::Color3B(62, 62, 62));
    button->setFontSize(fontSize);
    return button;
}

// ---------------------------------------------------------------------------
// Config / localization application
// ---------------------------------------------------------------------------
void MenuScene::populateRegions() {
    std::weak_ptr<bool> weak = _alive;
    fetchSiteInfo(_apiUrl, [this, weak](SiteInfo info) {
        const auto alive = weak.lock();
        if (!alive || !*alive) return;
        _siteInfo = std::move(info);
        if (!_siteInfo.pops.empty()) {
            bool found = false;
            for (const auto& pop : _siteInfo.pops) {
                if (pop.region == _region) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                _region = _siteInfo.pops.front().region;
                _config->setString("region", _region);
            }
        }
        applyConfigToUI();
        refreshUi();
        if (!_siteInfo.ok) setError("index-failed-finding-game", "Cannot reach API");
    });
}

void MenuScene::applyConfigToUI() {
    if (_startMenu) {
        _startMenu->refresh();
    }
}

void MenuScene::applyAudioFromConfig() {
    auto* game = _hooks.scene ? _hooks.scene() : nullptr;
    auto* audio = game ? game->audioManager() : nullptr;
    if (!audio) return;
    audio->setMute(_config->boolOrDefault("muteAudio"));
    audio->setMasterVolume(_config->floatOrDefault("masterVolume"));
    audio->setSoundVolume(_config->floatOrDefault("soundVolume"));
    audio->setMusicVolume(_config->floatOrDefault("musicVolume"));
}

void MenuScene::onConfigChanged(const std::string& key) {
    if (key == "muteAudio" || key == "language" || key == "region") {
        applyConfigToUI();
    }
    applyAudioFromConfig();
}

// ---------------------------------------------------------------------------
// Matchmaking (port of main.ts tryQuickStartGame)
// ---------------------------------------------------------------------------
void MenuScene::quickStart(int teamMode) {
    if (_pending || _inGame || _inTeam) {
        return;
    }
    if (!_siteInfo.ok || !_siteInfo.hasTeamMode(teamMode)) {
        setError("index-failed-finding-game", "Mode unavailable");
        return;
    }
    int gameModeIdx = _siteInfo.modeIndexForTeamMode(teamMode);

    GameScene* game = _hooks.scene ? _hooks.scene() : nullptr;
    if (!game) {
        setError("index-failed-finding-game", "Game scene unavailable");
        return;
    }

    _pending = true;
    _pendingTicker = 0.0f;
    clearError();
    refreshUi();

    // The scene owns the anti-spam delay/retries; this only asks for the mode.
    game->setJoinInfo(_hooks.joinInfo ? _hooks.joinInfo() : Game::JoinInfo{});
    game->enterWithFindGame(_region, gameModeIdx);
}

void MenuScene::setError(const std::string& l10nKey, const std::string& fallback) {
    if (_startMenu) {
        _startMenu->setError(l10nKey, fallback);
    }
    if (_overlay) {
        _overlay->setMenuError(l10nKey, fallback);
    }
}

void MenuScene::clearError() {
    if (_startMenu) {
        _startMenu->clearError();
    }
    if (_overlay) {
        _overlay->setMenuError("", "");
    }
}

void MenuScene::refreshUi() {
    const bool chromeVisible = !_inGame && !_inTeam;
    if (_startMenu) {
        _startMenu->setVisible(chromeVisible);
    }
    if (_teamRoot) {
        _teamRoot->setVisible(_inTeam);
    }

    // Disable matchmaking while a request is pending (per mode availability).
    if (_startMenu) {
        for (int i = 0; i < 3; ++i) {
            _startMenu->setPlayEnabled(i, !_pending && _siteInfo.hasTeamMode(i == 0 ? 1 : i == 1 ? 2 : 4));
        }
    }
    if (_pending) {
        const int dots = static_cast<int>(_pendingTicker * 2) % 4;
        setError("", tr(_loc.get(), "index-joining-game", "Finding game") + std::string(dots, '.'));
    }
}

// ---------------------------------------------------------------------------
// Team
// ---------------------------------------------------------------------------
void MenuScene::enterTeam(bool create) {
    if (_inGame || _inTeam || _pending) {
        return;
    }
    _inTeam = true;
    clearError();
    refreshUi();
    if (_team) {
        _team->setPlayerName(_config->getString("playerName", "Player"));
        _team->connect(create, create ? "" : _startMenu ? _startMenu->pendingRoomCode() : std::string());
        refreshTeamUi();
    }
}

void MenuScene::leaveTeam(const std::string& errorL10nKey) {
    if (_team) {
        _team->leave();
    }
    _inTeam = false;
    refreshUi();
    if (!errorL10nKey.empty()) {
        setError(errorL10nKey);
    }
}

void MenuScene::refreshTeamUi() {
    if (!_team || !_teamCode) {
        return;
    }
    _teamCode->setString(_team->isJoined() ? "#" + _team->roomUrl() : "");
    _teamStatus->setString(_team->isJoined()
                               ? tr(_loc.get(), "index-invite-code", "Invite code")
                               : tr(_loc.get(), "index-joining-team", "Joining Team") + "...");
    const auto& room = _team->roomData();
    const bool editable = _team->isLeader() && !_team->isFindingGame() && !_pending;
    _teamRegionBtn->setEnabled(editable);
    _teamModeBtn->setEnabled(editable);
    _teamAutoFillBtn->setEnabled(editable);
    _teamRegionBtn->setLabel(regionDisplayName(_loc.get(), room.region));
    std::string modeLabel = "Mode " + std::to_string(room.gameModeIdx + 1);
    if (room.gameModeIdx >= 0 && static_cast<size_t>(room.gameModeIdx) < _siteInfo.modes.size()) {
        const int tm = _siteInfo.modes[room.gameModeIdx].teamMode;
        modeLabel = tr(_loc.get(),
                       tm == 2 ? "index-play-duo" : tm == 4 ? "index-play-squad" : "index-play-solo",
                       tm == 2 ? "Play Duo" : tm == 4 ? "Play Squad" : "Play Solo");
    }
    _teamModeBtn->setLabel(modeLabel);
    _teamAutoFillBtn->setLabel(room.autoFill ? tr(_loc.get(), "index-auto-fill", "Auto Fill")
                                             : tr(_loc.get(), "index-no-fill", "No Fill"));
    _teamErrorLabel->setString(_team->gameError());
    _teamTitle->setString(tr(_loc.get(), "index-create-team", "Team"));
    if (_teamStartBtn) {
        _teamStartBtn->setVisible(_team->isLeader());
        _teamStartBtn->setEnabled(editable);
        _teamStartBtn->setLabel(_team->isFindingGame()
                                    ? tr(_loc.get(), "index-joining-game", "Finding game")
                                    : tr(_loc.get(), "index-play", "Play"));
    }
    for (auto* label : _teamPlayerLabels) {
        label->removeFromParent();
    }
    _teamPlayerLabels.clear();
    float y = 140.0f;
    for (const auto& player : _team->players()) {
        std::string text = player.name;
        if (player.isLeader) {
            text += " *";
        }
        if (player.inGame) {
            text += " (" + tr(_loc.get(), "index-joining-game", "in game") + ")";
        }
        auto* label = menuui::makeLabel(text, 16, player.isLeader);
        label->setAnchorPoint(ax::Vec2(0.5f, 1.0f));
        label->setTextColor(player.inGame ? ax::Color4B(180, 180, 180, 255) : ax::Color4B::WHITE);
        label->setPosition(ax::Vec2(310.0f, menuui::kit::fromCssY(y, 560.0f)));
        _teamPanel->addChild(label);
        _teamPlayerLabels.push_back(label);
        y += 26.0f;
    }
}

// ---------------------------------------------------------------------------
// Menu <-> game transitions
// ---------------------------------------------------------------------------
void MenuScene::onMatchStarted() {
    _inGame = true;
    _pending = false;
    if (_startMenu) {
        _startMenu->setVisible(false);
    }
    refreshUi();
    if (_uiRoot) {
        _uiRoot->setVisible(false);
    }
    if (_hooks.onMatchStarted) {
        _hooks.onMatchStarted();
    }
}

void MenuScene::onMatchEnded(const std::string& errorL10nKey) {
    _inGame = false;
    _pending = false;
    _inTeam = _team && _team->isActive();
    if (_inTeam) _team->onGameComplete(errorL10nKey.empty() ? "" : _loc->translate(errorL10nKey));
    applyConfigToUI();
    if (_uiRoot) {
        _uiRoot->setVisible(true);
    }
    refreshUi();
    if (!errorL10nKey.empty()) {
        setError(errorL10nKey);
    }
    if (_hooks.onReturnToMenu) {
        _hooks.onReturnToMenu();
    }
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------
void MenuScene::update(float delta) {
    if (_team) {
        _team->update(delta);
    }
    if (_pending) {
        _pendingTicker += delta;
        refreshUi();
        if (_inTeam) refreshTeamUi();
    }
}

void MenuScene::pauseMenu() {
    if (_config) {
        _config->save();
    }
}

void MenuScene::resumeMenu() {}

} // namespace surv
