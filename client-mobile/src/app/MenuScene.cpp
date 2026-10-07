#include "MenuScene.h"
#include "GameScene.h"
#include "DevConfig.h"
#include "../net/WebSocketConnection.h"
#include "../net/WsUrl.h"
#include "../ui/Files.h"
#include "../ui/UiOverlay.h"

#include <algorithm>
#include <cctype>
#include <cmath>

USING_NS_AX;

namespace surv {

namespace {

// helpers.sanitizeNameInput: trim, drop control characters, cap at the
// protocol's player-name length.
std::string sanitizeName(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (char c : input) {
        const unsigned char u = static_cast<unsigned char>(c);
        if (u < 0x20 || u == 0x7f) {
            continue;
        }
        out.push_back(c);
    }
    while (!out.empty() && out.front() == ' ') out.erase(out.begin());
    while (!out.empty() && out.back() == ' ') out.pop_back();
    if (out.size() > static_cast<size_t>(Constants::PlayerNameMaxLen)) {
        out.resize(Constants::PlayerNameMaxLen);
    }
    return out;
}

std::string regionDisplayName(ui::Localization* loc, const std::string& region) {
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

MenuScene::~MenuScene() = default;

bool MenuScene::init() {
    if (!Scene::init()) {
        return false;
    }
    const ax::Size visible = ax::Director::getInstance()->getVisibleSize();
    ui::Device::get().resize(visible.width, visible.height);

    // --- config + localization ------------------------------------------
    _storage = std::make_unique<ui::UserDefaultConfigStorage>("surviv_config");
    _config = std::make_unique<ui::Config>(_storage.get());
    _config->load();

    _loc = std::make_unique<ui::Localization>();
    const std::string english = ui::Files::readText("l10n/en.json");
    if (!english.empty()) {
        _loc->registerEnglish(english);
    } else {
        AXLOGW("Missing l10n/en.json; UI strings fall back to keys");
    }
    _loc->setLoader([](const std::string& locale) {
        return ui::Files::readText("l10n/" + locale + ".json");
    });
    _loc->setLocale(_config->getString("language", "en"));

    _apiUrl = ax::UserDefault::getInstance()->getStringForKey(dev::kKeyApiUrl, dev::kApiBaseUrl);
    _region = _config->getString("region", "na");

    // --- UI --------------------------------------------------------------
    _uiRoot = ui::uiRoot(this, 10);
    ui::layoutUiRoot(_uiRoot, ax::Vec2(visible.width, visible.height),
                     ax::Vec2(ui::kit::designWidth(), ui::kit::designHeight()));
    buildBackground();
    buildMenuPanel();
    buildModals();
    buildTeamScreen();

    // --- in-game overlay (handed to the game scene) ----------------------
    _overlay = std::make_unique<ui::UiOverlay>();

    // --- team client -----------------------------------------------------
    _team = std::make_unique<ui::TeamMenu>();
    _team->setFactory(createWebSocketConnectionTo);
    _team->setUrl(_apiUrl + "/team_v2");
    _team->onRoomChanged = [this] { refreshTeamUi(); };
    _team->onError = [this](ui::TeamErrorType type, const std::string&) {
        leaveTeam(ui::teamErrorL10n(type));
    };
    _team->onPlay = [this](const ui::TeamMenu::MatchData& match) {
        if (_inGame || match.urls.empty()) {
            return;
        }
        if (GameScene* game = _hooks.scene ? _hooks.scene() : nullptr) {
            _inGame = true;
            _inTeam = false;
            _pending = false;
            clearError();
            refreshUi();
            game->setJoinInfo(_hooks.joinInfo ? _hooks.joinInfo() : Game::JoinInfo{});
            game->enterWithJoin(match.urls, match.joinToken);
            onMatchStarted();
        }
    };

    populateRegions();
    applyConfigToUI();
    refreshUi();

    this->scheduleUpdate();
    return true;
}

// ---------------------------------------------------------------------------
// Panel construction
// ---------------------------------------------------------------------------
void MenuScene::buildBackground() {
    const float w = ui::kit::designWidth();
    const float h = ui::kit::designHeight();

    // `cachedBgImg` mirrors the web client's splash (default main.webp).
    std::string bgPath = _config->getString("cachedBgImg", "img/splashes/main.webp");
    if (bgPath.empty()) {
        bgPath = "img/splashes/main.webp";
    }
    _background = ax::Sprite::create(bgPath);
    if (_background) {
        _background->setAnchorPoint(ax::Vec2(0.0f, 0.0f));
        _background->setPosition(ax::Vec2::ZERO);
        const ax::Size size = _background->getContentSize();
        if (size.width > 0.0f && size.height > 0.0f) {
            _background->setScale(std::fmax(w / size.width, h / size.height));
        }
        _uiRoot->addChild(_background, 0);
    } else {
        auto* dim = ax::LayerColor::create(ax::Color4B(33, 39, 43, 255), w, h);
        dim->setAnchorPoint(ax::Vec2(0.0f, 1.0f));
        dim->setIgnoreAnchorPointForPosition(false);
        dim->setPosition(ax::Vec2(0.0f, h));
        _uiRoot->addChild(dim, 0);
    }

    _logo = ax::Sprite::create("img/survev_logo_full.png");
    if (_logo) {
        _logo->setAnchorPoint(ax::Vec2(0.5f, 1.0f));
        _logo->setPosition(ax::Vec2(w * 0.5f, ui::kit::fromCssY(6.0f, h)));
        const ax::Size size = _logo->getContentSize();
        if (size.width > 0.0f) {
            _logo->setScale(420.0f / size.width);
        }
        _uiRoot->addChild(_logo, 1);
    }
}

void MenuScene::buildMenuPanel() {
    const float panelW = 460.0f;
    const float panelH = 540.0f;
    _menuPanel = ui::Panel::create(panelW, panelH);
    _menuPanel->setBackgroundColor(ax::Color3B(28, 32, 36), 0.82f);
    _menuPanel->placeCenter();
    _uiRoot->addChild(_menuPanel);

    const float pad = 24.0f;
    const float fieldW = panelW - pad * 2.0f;
    float y = 20.0f;

    // Player name (config "playerName", protocol max 16 chars).
    std::string nameHint = _loc->translate("index-enter-name-here");
    if (nameHint.empty()) {
        nameHint = "Enter your name here";
    }
    _nameField = ui::TextField::create(nameHint, fieldW, 46.0f);
    _nameField->setCssPosition(pad, y);
    _nameField->setMaxLength(16);
    _nameField->setOnChanged([this](const std::string& text) {
        _config->setString("playerName", sanitizeName(text));
    });
    _menuPanel->addChild(_nameField);
    y += 58.0f;

    // Region selector (cycles site_info.pops).
    _regionBtn = ui::Button::create("Region: NA", fieldW, 44.0f);
    _regionBtn->setCssPosition(pad, y);
    _regionBtn->onClick = [this] {
        if (_siteInfo.pops.empty()) {
            return;
        }
        size_t idx = 0;
        for (size_t i = 0; i < _siteInfo.pops.size(); i++) {
            if (_siteInfo.pops[i].region == _region) {
                idx = i;
                break;
            }
        }
        idx = (idx + 1) % _siteInfo.pops.size();
        _region = _siteInfo.pops[idx].region;
        _config->setString("region", _region);
        applyConfigToUI();
    };
    _menuPanel->addChild(_regionBtn);
    y += 56.0f;

    // Play Solo / Duo / Squad (team modes 1 / 2 / 4).
    const int teamModes[3] = { 1, 2, 4 };
    const char* keys[3] = { "index-play-solo", "index-play-duo", "index-play-squad" };
    const char* fallbacks[3] = { "Play Solo", "Play Duo", "Play Squad" };
    for (int i = 0; i < 3; i++) {
        std::string label = _loc->translate(keys[i]);
        if (label.empty()) {
            label = fallbacks[i];
        }
        _playBtns[i] = ui::Button::create(label, fieldW, 48.0f);
        _playBtns[i]->setCssPosition(pad, y);
        const int teamMode = teamModes[i];
        _playBtns[i]->onClick = [this, teamMode] { quickStart(teamMode); };
        _menuPanel->addChild(_playBtns[i]);
        y += 60.0f;
    }

    // Customize (T2 replaces the stub with the loadout screen).
    {
        std::string label = _loc->translate("index-customize-loadout");
        if (label.empty()) {
            label = "Customize";
        }
        _customizeBtn = ui::Button::create(label, fieldW, 44.0f);
        _customizeBtn->setCssPosition(pad, y);
        _customizeBtn->onClick = [this] { _customizeModal.show(); };
        _menuPanel->addChild(_customizeBtn);
        y += 56.0f;
    }

    // Join Team / Create Team.
    {
        const float half = (fieldW - 12.0f) * 0.5f;
        std::string joinLabel = _loc->translate("index-join-team");
        if (joinLabel.empty()) {
            joinLabel = "Join Team";
        }
        _joinTeamBtn = ui::Button::create(joinLabel, half, 44.0f);
        _joinTeamBtn->setCssPosition(pad, y);
        _joinTeamBtn->onClick = [this] { enterTeam(false); };
        _menuPanel->addChild(_joinTeamBtn);

        std::string createLabel = _loc->translate("index-create-team");
        if (createLabel.empty()) {
            createLabel = "Create Team";
        }
        _createTeamBtn = ui::Button::create(createLabel, half, 44.0f);
        _createTeamBtn->setCssPosition(pad + half + 12.0f, y);
        _createTeamBtn->onClick = [this] { enterTeam(true); };
        _menuPanel->addChild(_createTeamBtn);
        y += 56.0f;
    }

    // How to Play + Sound.
    {
        const float half = (fieldW - 12.0f) * 0.5f;
        std::string helpLabel = _loc->translate("index-how-to-play");
        if (helpLabel.empty()) {
            helpLabel = "How to Play";
        }
        _helpBtn = ui::Button::create(helpLabel, half, 42.0f);
        _helpBtn->setCssPosition(pad, y);
        _helpBtn->onClick = [this] { _helpModal.show(); };
        _menuPanel->addChild(_helpBtn);

        _soundBtn = ui::Button::create("Sound", half, 42.0f);
        _soundBtn->setCssPosition(pad + half + 12.0f, y);
        _soundBtn->onClick = [this] {
            _config->setBool("muteAudio", !_config->boolOrDefault("muteAudio"));
            applyConfigToUI();
        };
        _menuPanel->addChild(_soundBtn);
        y += 54.0f;
    }

    // Master / SFX / Music volume (config keys shared with the game scene).
    const char* volKeys[3] = { "masterVolume", "soundVolume", "musicVolume" };
    const char* volL10n[3] = { "index-master-volume", "index-sfx-volume", "index-music-volume" };
    for (int i = 0; i < 3; i++) {
        std::string label = _loc->translate(volL10n[i]);
        if (label.empty()) {
            label = volKeys[i];
        }
        auto* text = ax::Label::createWithSystemFont(label, "sans-serif", 14);
        text->setAnchorPoint(ax::Vec2(0.0f, 1.0f));
        text->setPosition(ax::Vec2(pad, ui::kit::fromCssY(y, panelH)));
        _menuPanel->addChild(text);

        auto* slider = ui::Slider::create(fieldW, _config->floatOrDefault(volKeys[i]));
        slider->setCssPosition(pad, y + 16.0f);
        const std::string key = volKeys[i];
        slider->onChanged = [this, key](float v) { _config->setFloat(key, v); };
        _menuPanel->addChild(slider);
        _sliders.push_back(slider);
        y += 42.0f;
    }

    // Error / status line.
    _errorLabel = ax::Label::createWithSystemFont("", "sans-serif", 16);
    _errorLabel->setAnchorPoint(ax::Vec2(0.5f, 0.0f));
    _errorLabel->setPosition(ax::Vec2(panelW * 0.5f, 10.0f));
    _errorLabel->setTextColor(ax::Color4B(255, 120, 120, 255));
    _menuPanel->addChild(_errorLabel);
}

void MenuScene::buildModals() {
    // How to Play (web `#btn-help` -> `#start-help`).
    auto* helpPanel = _helpModal.build(640.0f, 420.0f,
                                       _loc->translate("index-how-to-play").empty()
                                           ? "How to Play"
                                           : _loc->translate("index-how-to-play"));
    const char* lines[] = {
        "index-movement", "index-aim", "index-punch", "index-shoot",
        "index-change-weapons", "index-reload", "index-scope-zoom",
    };
    float y = 70.0f;
    for (const char* key : lines) {
        const std::string text = _loc->translate(key);
        if (text.empty()) {
            continue;
        }
        auto* label = ax::Label::createWithSystemFont(text, "sans-serif", 16);
        label->setAnchorPoint(ax::Vec2(0.0f, 1.0f));
        label->setPosition(ax::Vec2(28.0f, ui::kit::fromCssY(y, 420.0f)));
        label->setTextColor(ax::Color4B::WHITE);
        helpPanel->addChild(label);
        y += 40.0f;
    }

    // Customize stub (T2 replaces this with the loadout screen).
    auto* customPanel = _customizeModal.build(520.0f, 200.0f, "Customize");
    auto* note = ax::Label::createWithSystemFont("Loadout menu (M7 T2) coming soon.",
                                                 "sans-serif", 16);
    note->setAnchorPoint(ax::Vec2(0.5f, 0.5f));
    note->setPosition(ax::Vec2(260.0f, ui::kit::fromCssY(110.0f, 200.0f)));
    note->setTextColor(ax::Color4B::WHITE);
    customPanel->addChild(note);
}

void MenuScene::buildTeamScreen() {
    const float panelW = 620.0f;
    const float panelH = 440.0f;
    _teamRoot = ax::Node::create();
    _teamRoot->setContentSize(ax::Size(ui::kit::designWidth(), ui::kit::designHeight()));
    _teamRoot->setVisible(false);
    _uiRoot->addChild(_teamRoot, 20);
    ui::layoutUiRoot(_teamRoot, ax::Director::getInstance()->getVisibleSize(),
                     ax::Vec2(ui::kit::designWidth(), ui::kit::designHeight()));

    _teamPanel = ui::Panel::create(panelW, panelH);
    _teamPanel->setBackgroundColor(ax::Color3B(28, 32, 36), 0.94f);
    _teamPanel->placeCenter();
    _teamRoot->addChild(_teamPanel);

    _teamTitle = ax::Label::createWithSystemFont("Team", "sans-serif", 24);
    _teamTitle->setAnchorPoint(ax::Vec2(0.5f, 1.0f));
    _teamTitle->setPosition(ax::Vec2(panelW * 0.5f, ui::kit::fromCssY(16.0f, panelH)));
    _teamTitle->setTextColor(ax::Color4B::WHITE);
    _teamPanel->addChild(_teamTitle);

    _teamCode = ax::Label::createWithSystemFont("", "sans-serif", 26);
    _teamCode->setAnchorPoint(ax::Vec2(0.5f, 1.0f));
    _teamCode->setPosition(ax::Vec2(panelW * 0.5f, ui::kit::fromCssY(56.0f, panelH)));
    _teamCode->setTextColor(ax::Color4B(140, 220, 140, 255));
    _teamPanel->addChild(_teamCode);

    _teamStartBtn = ui::Button::create("Start Game", 240.0f, 48.0f);
    _teamStartBtn->setCssPosition((panelW - 240.0f) * 0.5f, panelH - 116.0f);
    _teamStartBtn->onClick = [this] {
        if (_team) {
            _team->tryStartGame();
        }
    };
    _teamPanel->addChild(_teamStartBtn);

    _teamLeaveBtn = ui::Button::create("Leave Team", 240.0f, 48.0f);
    _teamLeaveBtn->setCssPosition((panelW - 240.0f) * 0.5f, panelH - 60.0f);
    _teamLeaveBtn->onClick = [this] { leaveTeam(); };
    _teamPanel->addChild(_teamLeaveBtn);
}

// ---------------------------------------------------------------------------
// Config / localization application
// ---------------------------------------------------------------------------
void MenuScene::populateRegions() {
    fetchSiteInfo(_apiUrl, [this](SiteInfo info) {
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
            }
        }
        applyConfigToUI();
    });
}

void MenuScene::applyConfigToUI() {
    if (_nameField) {
        _nameField->setText(_config->getString("playerName", ""));
    }
    if (_regionBtn) {
        _regionBtn->setLabel("Region: " + regionDisplayName(_loc.get(), _region));
    }
    if (_soundBtn) {
        const bool muted = _config->boolOrDefault("muteAudio");
        std::string label = muted ? _loc->translate("game-sound-off") : _loc->translate("game-sound");
        if (label.empty()) {
            label = muted ? "Sound: Off" : "Sound";
        }
        _soundBtn->setLabel(label);
    }
}

void MenuScene::applyAudioFromConfig() {
    // The game scene owns the AudioManager and applies these keys on entry; the
    // menu only persists them. Kept as a hook for a future shared manager.
}

void MenuScene::onConfigChanged(const std::string& key) {
    (void)key;
    applyConfigToUI();
}

// ---------------------------------------------------------------------------
// Matchmaking (port of main.ts tryQuickStartGame)
// ---------------------------------------------------------------------------
void MenuScene::quickStart(int teamMode) {
    if (_pending || _inGame || _inTeam) {
        return;
    }
    if (!_siteInfo.modes.empty() && !_siteInfo.hasTeamMode(teamMode)) {
        setError("index-failed-finding-game", "Mode unavailable");
        return;
    }
    int gameModeIdx = _siteInfo.modeIndexForTeamMode(teamMode);
    if (gameModeIdx < 0) {
        gameModeIdx = (teamMode == 1) ? 0 : (teamMode == 2 ? 1 : 2);
    }

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
    std::string text = _loc ? _loc->translate(l10nKey) : "";
    if (text.empty()) {
        text = fallback;
    }
    if (_errorLabel) {
        _errorLabel->setString(text);
    }
    if (_overlay) {
        _overlay->setMenuError(l10nKey, fallback);
    }
}

void MenuScene::clearError() {
    if (_errorLabel) {
        _errorLabel->setString("");
    }
    if (_overlay) {
        _overlay->setMenuError("", "");
    }
}

void MenuScene::refreshUi() {
    if (_menuPanel) {
        _menuPanel->setVisible(!_inGame && !_inTeam);
    }
    if (_teamRoot) {
        _teamRoot->setVisible(_inTeam);
    }
    for (int i = 0; i < 3; i++) {
        if (_playBtns[i]) {
            _playBtns[i]->setEnabled(!_pending);
        }
    }
}

// ---------------------------------------------------------------------------
// Team
// ---------------------------------------------------------------------------
void MenuScene::enterTeam(bool create) {
    if (_inGame || _inTeam) {
        return;
    }
    _inTeam = true;
    clearError();
    refreshUi();
    if (_team) {
        _team->setPlayerName(_config->getString("playerName", "Player"));
        _team->connect(create, "");
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
    _teamCode->setString(_team->roomUrl());
    const std::string title = _loc->translate("index-team-menu");
    _teamTitle->setString(title.empty() ? "Team" : title);
    if (_teamStartBtn) {
        _teamStartBtn->setEnabled(_team->isLeader() && !_team->isFindingGame());
    }
    for (auto* label : _teamPlayerLabels) {
        label->removeFromParent();
    }
    _teamPlayerLabels.clear();
    float y = 104.0f;
    for (const auto& player : _team->players()) {
        std::string text = player.name;
        if (player.isLeader) {
            text += " *";
        }
        if (player.inGame) {
            text += " (in game)";
        }
        auto* label = ax::Label::createWithSystemFont(text, "sans-serif", 16);
        label->setAnchorPoint(ax::Vec2(0.5f, 1.0f));
        label->setPosition(ax::Vec2(310.0f, ui::kit::fromCssY(y, 440.0f)));
        label->setTextColor(ax::Color4B::WHITE);
        _teamPanel->addChild(label);
        _teamPlayerLabels.push_back(label);
        y += 28.0f;
    }
}

// ---------------------------------------------------------------------------
// Menu <-> game transitions
// ---------------------------------------------------------------------------
void MenuScene::onMatchStarted() {
    _pending = false;
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
        // The game scene switches to the match on success; a timeout resets the
        // buttons so the player can retry (main.ts lockout behaviour).
        if (_pendingTicker > 30.0f) {
            _pending = false;
            setError("index-failed-finding-game", "Failed to find game");
            refreshUi();
        }
    }
}

void MenuScene::pauseMenu() {
    if (_config) {
        _config->save();
    }
}

void MenuScene::resumeMenu() {}

} // namespace surv
