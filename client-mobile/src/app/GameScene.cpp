#include "GameScene.h"
#include "DevConfig.h"
#include "../net/Api.h"
#include "../net/Messages.h"
#include "../net/WebSocketConnection.h"
#include "../game/GameWorld.h"
#include "../game/Map.h"
#include "../game/objects/Barns.h"
#include "../render/AxmolPixi.h"
#include "../render/Defs.h"
#include "../render/GeneratedDefs.h"
#include "../audio/AudioManager.h"
#include "../audio/Ambiance.h"
#include "../audio/AxmolAudioBackend.h"
#include "../audio/GeneratedSoundDefs.h"
#include "../ui/UiOverlay.h"

#include <set>
#include <algorithm>
#include <sstream>

USING_NS_AX;

namespace surv {
namespace svui = ::ui;

// M4/T1: register the sprite atlases a map needs with the SpriteFrameCache.
// tools/build-atlas.mjs writes Content/atlas/<atlas>/{*.plist,index.txt}; the
// plist frame keys are the web client's `*.img` names, so AxSprite::setFrame()
// resolves them directly. Loading is deferred until the map arrives because the
// map def lists its atlases.
static void loadAtlasesForMap(const std::string& mapName) {
    const DefProvider* provider = getDefProvider();
    const MapRenderDef* mapDef = provider ? provider->mapRender(mapName) : nullptr;
    if (!mapDef) {
        return;
    }
    static std::set<std::string> loaded;
    auto* cache = ax::SpriteFrameCache::getInstance();
    auto* fileUtils = ax::FileUtils::getInstance();
    for (const auto& atlas : mapDef->atlases) {
        if (atlas.empty() || loaded.count(atlas)) {
            continue;
        }
        const std::string index = "atlas/" + atlas + "/index.txt";
        const std::string list = fileUtils->getStringFromFile(index);
        if (list.empty()) {
            AXLOGW("Atlas index missing ({}); run tools/build-atlas.mjs", index);
            continue;
        }
        std::istringstream stream(list);
        std::string line;
        while (std::getline(stream, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (line.empty()) {
                continue;
            }
            cache->addSpriteFramesWithFile(line);
        }
        loaded.insert(atlas);
        AXLOGI("Loaded atlas {}", atlas);
    }
}

// ---------------------------------------------------------------------------
// TouchPadGfx (vector-drawn joystick pads; no external texture required)
// ---------------------------------------------------------------------------
TouchPadGfx::TouchPadGfx(ax::Node* parent) {
    _center = ax::DrawNode::create();
    _center->drawDot(ax::Vec2(0, 0), 24.0f, ax::Color4F(1.0f, 1.0f, 1.0f, 0.20f));
    _center->setVisible(false);
    parent->addChild(_center, 1000);

    _touch = ax::DrawNode::create();
    _touch->drawDot(ax::Vec2(0, 0), 12.0f, ax::Color4F(1.0f, 1.0f, 1.0f, 0.90f));
    _touch->setVisible(false);
    parent->addChild(_touch, 1001);
}

void TouchPadGfx::setVisible(bool visible) {
    _center->setVisible(visible);
    _touch->setVisible(visible);
}

void TouchPadGfx::setCenterPos(float x, float y) {
    _center->setPosition(ax::Vec2(x, y));
}

void TouchPadGfx::setTouchPos(float x, float y) {
    _touch->setPosition(ax::Vec2(x, y));
}

void TouchPadGfx::setCenterScale(float s) {
    _center->setScale(s);
}

void TouchPadGfx::setTouchScale(float s) {
    _touch->setScale(s);
}

void TouchPadGfx::update() {}

// ---------------------------------------------------------------------------
// GameScene
// ---------------------------------------------------------------------------
GameScene* GameScene::createScene() {
    auto scene = new GameScene();
    if (scene && scene->init()) {
        scene->autorelease();
        return scene;
    }
    AX_SAFE_DELETE(scene);
    return nullptr;
}

bool GameScene::init() {
    if (!Scene::init()) {
        return false;
    }

    const ax::Size visible = ax::Director::getInstance()->getVisibleSize();
    const bool isLandscape = visible.width >= visible.height;

    _gameRoot = ax::Node::create();
    this->addChild(_gameRoot);

    // Dual-joystick touch input (multi-touch).
    auto listener = ax::EventListenerTouchAllAtOnce::create();
    listener->onTouchesBegan = [this](const std::vector<ax::Touch*>& touches, ax::Event*) {
        for (auto* t : touches) {
            onTouchBegan(t->getLocation().x, t->getLocation().y, t->getID() & 3);
        }
    };
    listener->onTouchesMoved = [this](const std::vector<ax::Touch*>& touches, ax::Event*) {
        for (auto* t : touches) {
            onTouchMoved(t->getLocation().x, t->getLocation().y, t->getID() & 3);
        }
    };
    listener->onTouchesEnded = [this](const std::vector<ax::Touch*>& touches, ax::Event*) {
        for (auto* t : touches) {
            onTouchEnded(t->getID() & 3);
        }
    };
    listener->onTouchesCancelled = [this](const std::vector<ax::Touch*>& touches, ax::Event*) {
        for (auto* t : touches) {
            onTouchEnded(t->getID() & 3);
        }
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

    _touch = std::make_unique<Touch>(visible.width, visible.height, isLandscape);
    _movePad = std::make_unique<TouchPadGfx>(this);
    _aimPad = std::make_unique<TouchPadGfx>(this);

    _alive = std::make_shared<std::atomic<bool>>(true);

    _game = std::make_unique<Game>();
    _game->init(this, createWebSocketConnection);

    // M4: render world (terrain + z-sorted object layers) driven by the map /
    // update messages the game dispatches.
    installGeneratedDefs();
    // The atlas converter emits the low-resolution sheets (the web client's
    // mobile path); 0.5 makes axmol render those frames at full point size.
    ax::Director::getInstance()->setContentScaleFactor(0.5f);

    // M6: audio backend + manager + ambience mixer.
    audio::installGeneratedSoundDefs();
    _audioBackend = std::make_unique<audio::AxmolAudioBackend>();
    _audio = std::make_unique<audio::AudioManager>(_audioBackend.get());
    _ambiance = std::make_unique<audio::Ambiance>();
    _audio->preloadSounds();

    _pixiFactory = std::make_unique<pix::AxPixiFactory>();
    resetWorld();
    _game->setMapCallback([this](const MapMsg& msg) {
        loadAtlasesForMap(msg.mapName);
        _world->loadMap(msg);
    });
    _game->setUpdateCallback([this](const UpdateMsg& msg) { _world->applyUpdate(msg); });

    // M7: the menu starts a game via enterWithJoin()/enterWithFindGame().
    _gameRoot->setVisible(false);
    _movePad->setVisible(false);
    _aimPad->setVisible(false);

    this->scheduleUpdate();
    return true;
}

void GameScene::resetWorld() {
    _world.reset();
    if (_worldRoot) {
        static_cast<ax::Node*>(_worldRoot->native())->removeFromParent();
        _worldRoot.reset();
    }
    _world = std::make_unique<GameWorld>(_pixiFactory.get(), false);
    _world->setAudio(_audio.get(), _ambiance.get());
    _worldRoot.reset(_pixiFactory->createContainer());
    _world->attachTo(_worldRoot.get());
    _gameRoot->addChild(static_cast<ax::Node*>(_worldRoot->native()));
    const auto visible = ax::Director::getInstance()->getVisibleSize();
    _world->setScreenSize(visible.width, visible.height);
}

GameScene::~GameScene() {
    if (_alive) {
        *_alive = false;
    }
    if (_game) {
        _game->free();
    }
}

void GameScene::enterWithJoin(const std::vector<std::string>& urls, const std::string& joinToken) {
    if (!_game || urls.empty()) {
        return;
    }
    clearError();
    _game->free();
    resetWorld();
    _inGame = true;
    _notifiedStarted = false;
    _joinElapsed = 0;
    _gameRoot->setVisible(true);
    _movePad->setVisible(true);
    _aimPad->setVisible(true);
    _game->setJoinInfo(_joinInfo);
    _awaitingJoin = true;
    _joinRetryUrls.clear();
    tryJoinUrls(urls, joinToken, 0);
}

void GameScene::enterWithFindGame(const std::string& region, int gameModeIdx) {
    _region = region;
    _gameModeIdx = gameModeIdx;
    _pendingFind = true;
    _findInFlight = false;
    ++_findGeneration;
    _findAttempts = 0;
    _lastAttemptClock = 0.0;
    _findDelay = 0.0f;
    _findTime = 0.0f;
    // First request fires on the next update tick (main.ts uses a 0ms timeout
    // on the first attempt so it can be skipped).
}

void GameScene::leaveGame() {
    if (_game) {
        _game->free();
    }
    _inGame = false;
    _pendingFind = false;
    _findInFlight = false;
    ++_findGeneration;
    _touch->display = false;
    for (auto& touch : _touch->touches) touch.active = false;
    _awaitingJoin = false;
    _joinRetryUrls.clear();
    _gameRoot->setVisible(false);
    _movePad->setVisible(false);
    _aimPad->setVisible(false);
    if (_audio) {
        _audio->stopAll();
    }
}

void GameScene::setError(const std::string& key, const std::string& fallback) {
    if (onError) onError(key, fallback);
    if (_overlay) {
        _overlay->setMenuError(key, fallback);
    }
}

void GameScene::clearError() {
    if (_overlay) {
        _overlay->setMenuError("", "");
    }
}

void GameScene::tryJoinUrls(const std::vector<std::string>& urls, const std::string& joinToken,
                            size_t index) {
    if (!_game || index >= urls.size()) {
        setError("index-failed-joining-game", "Failed to join game");
        leaveGame();
        return;
    }
    // main.ts keeps a failure callback per URL so a dead host falls through to
    // the next advertised address; the socket close path marks the attempt.
    _joinRetryUrls = urls;
    _joinRetryToken = joinToken;
    _joinRetryIndex = index;
    _awaitingJoin = true;
    _game->tryJoinGame(urls[index], joinToken);
}

void GameScene::runFindGameAttempt() {
    if (!_findGameRequest) {
        setError("index-failed-finding-game", "Failed to find game");
        return;
    }
    _findAttempts++;
    _findInFlight = true;
    _findTime = 0.0f;
    std::weak_ptr<std::atomic<bool>> weak = _alive;
    const std::string region = _region;
    const int mode = _gameModeIdx;
    const unsigned generation = _findGeneration;
    _findGameRequest(region, mode, [this, weak, generation](const FindGameResultInfo& result) {
        const auto alive = weak.lock();
        if (!alive || !*alive || generation != _findGeneration) {
            return;
        }
        _findInFlight = false;
        if (result.ok && !result.urls.empty()) {
            _pendingFind = false;
            _findAttempts = 0;
            enterWithJoin(result.urls, result.joinToken);
            return;
        }
        const std::string key = result.error == "banned" ? "index-ip-banned"
            : result.error == "behind_proxy" ? "index-behind-proxy"
            : result.error == "invalid_protocol" ? "index-invalid-protocol"
            : result.error == "invalid_captcha" ? "index-invalid-captcha"
            : result.error == "rate_limited" ? "index-rate-limited" : "index-failed-finding-game";
        setError(key, "Failed to find game");
        // main.ts retries every 500ms until maxAttempts (2).
        if (_findAttempts < 2 && key == "index-failed-finding-game") {
            _findDelay = std::min(_findAttempts * 2.5f, 7.5f);
            _pendingFind = true;
        } else {
            _pendingFind = false;
            if (onMatchEnded) onMatchEnded(key);
        }
    });
}

void GameScene::pauseGame() {
    if (_game) {
        _game->pause();
    }
    if (_audio) {
        _audio->stopAll();
    }
}

void GameScene::returnToMenu() {
    // Stop any in-flight matchmaking/join first so a late find_game response
    // cannot pull the player back out of the menu.
    if (_inGame || _pendingFind || _findInFlight) {
        leaveGame();
    }
    if (onMatchEnded) {
        onMatchEnded("");
    }
}

void GameScene::resumeGame() {
    if (_game) {
        _game->resume();
    }
}

void GameScene::update(float delta) {
    _game->update(delta);
    if (_findInFlight) {
        _findTime += delta;
        if (_findTime > 30) {
            leaveGame();
            if (onMatchEnded) onMatchEnded("index-failed-finding-game");
        }
    }
    if (_inGame && !_notifiedStarted) {
        _joinElapsed += delta;
        if (_game->hasJoined()) {
            _notifiedStarted = true;
            _touch->display = true;
            if (onMatchStarted) onMatchStarted();
        } else if (_joinElapsed > 30) {
            leaveGame();
            if (onMatchEnded) onMatchEnded("index-failed-joining-game");
        }
    }

    // M7 quick-start: wait out the anti-spam delay, then issue the request.
    if (_pendingFind) {
        if (_findDelay > 0.0f) {
            _findDelay -= delta;
        } else {
            _pendingFind = false;
            runFindGameAttempt();
        }
    }

    if (_world && _inGame) {
        _world->update(delta);
        if (Player* active = _world->activePlayer()) {
            // Camera follows the active player (M5 adds the smoothing/shake).
            _world->camera().m_pos = active->pos;
        }
    }

    // Low-noise lifecycle logging (once per transition) for the M3 net gate.
    const bool connected = _game->isConnected();
    if (connected != _wasConnected) {
        _wasConnected = connected;
        if (connected) {
            AXLOGI("Connected to game server");
        } else {
            AXLOGI("Disconnected: code={} reason={}", _game->getCloseCode(), _game->getCloseReason());
        }
    }
    if (_game->isPlaying() != _wasPlaying) {
        _wasPlaying = _game->isPlaying();
        if (_wasPlaying) {
            AXLOGI("Receiving game updates: activePlayerId={}", _game->getActivePlayerId());
            if (_ambiance) {
                _ambiance->onGameStart();
            }
        }
    }

    // A closed game socket (not an intentional leave) returns to the menu with
    // a localized reason, mirroring main.ts onQuit().
    if (_inGame && !_game->isConnected() && _game->getCloseCode() != 0) {
        // If the server never accepted us, try the next advertised URL with the
        // same join token (main.ts joinGame()) before giving up.
        if (_awaitingJoin && !_game->hasJoined()
            && _joinRetryIndex + 1 < _joinRetryUrls.size()) {
            _awaitingJoin = false;
            const size_t next = _joinRetryIndex + 1;
            const std::vector<std::string> urls = _joinRetryUrls;
            const std::string token = _joinRetryToken;
            AXLOGW("Join failed (code={}); trying next url", _game->getCloseCode());
            tryJoinUrls(urls, token, next);
            return;
        }
        _awaitingJoin = false;
        static const struct {
            uint16_t code;
            const char* key;
        } kCloseKeys[] = {
            {4001, "index-invalid-token"},      {4002, "index-invalid-protocol"},
            {4003, "index-invalid-packet"},     {4004, "index-behind-proxy"},
            {4005, "index-player-not-found"},   {4006, "index-ip-banned"},
            {4007, "index-rate-limited"},       {4008, "index-server-crashed"},
            {4009, "index-server-restart"},     {4010, "index-invalid-captcha"},
            {4011, "index-failed-finding-game"},
        };
        const char* key = "index-host-closed";
        for (const auto& entry : kCloseKeys) {
            if (entry.code == _game->getCloseCode()) {
                key = entry.key;
                break;
            }
        }
        setError(key, "Connection lost");
        leaveGame();
        if (onMatchEnded) onMatchEnded(key);
    } else if (_game->hasJoined()) {
        _awaitingJoin = false;
    }

    if (_overlay) {
        _overlay->update(delta, isStarted(), isPlaying(), isConnected(), _game.get());
    }

    if (_game->isPlaying() && _game->getActivePlayerId() != 0 && (!_overlay || !_overlay->isOpen())) {
        updateInput(delta);
    }
    _touch->m_update(*_movePad, *_aimPad);
}

void GameScene::updateInput(float dt) {
    const ax::Size size = getContentSize();

    // Feed the dual-joystick state into an InputMsg. isHoldingThrowable drives
    // the throwable priming latch in Touch::getAim; the aim line direction is
    // taken from Touch's aim movement (see TouchInput::build).
    InputMsg msg = _touchInput.build(*_touch,
                                     size.width,
                                     _game->isHoldingThrowable(),
                                     size.width < size.height,
                                     dt);

    // A quick tap can begin and end between two sends; latch shootStart so the
    // server still observes the press.
    _shootStartPending = _shootStartPending || msg.shootStart;

    // Send at the server's net-sync/input rate rather than once per frame.
    _inputMsgTimeout -= dt;
    if (_inputMsgTimeout < 0.0f) {
        msg.shootStart = _shootStartPending;
        _shootStartPending = false;
        _game->sendInput(msg);
        _inputMsgTimeout = 1.0f / kNetSyncTps;
    }
}

void GameScene::onTouchBegan(float x, float y, int id) {
    if (!_inGame || !_notifiedStarted || (_overlay && _overlay->isOpen())) return;
    _touch->touches[id].active = true;
    _touch->touches[id].posDown = Vec2(x, y);
    _touch->touches[id].pos = Vec2(x, y);
}

void GameScene::onTouchMoved(float x, float y, int id) {
    if (_touch->touches[id].active) {
        _touch->touches[id].pos = Vec2(x, y);
    }
}

void GameScene::onTouchEnded(int id) {
    _touch->touches[id].active = false;
}

} // namespace surv
