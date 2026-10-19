#include "GameScene.h"
#include "DevConfig.h"
#include "../net/Api.h"
#include "../net/Messages.h"
#include "../net/WebSocketConnection.h"
#include "../game/GameWorld.h"
#include "../game/Map.h"
#include "../game/objects/Barns.h"
#include "../render/AxmolPixi.h"
#include "../render/PixiLike.h"
#include "../render/Defs.h"
#include "../render/GeneratedDefs.h"
#include "../audio/AudioManager.h"
#include "../audio/Ambiance.h"
#include "../audio/AxmolAudioBackend.h"
#include "../audio/GeneratedSoundDefs.h"
#include "../ui/FguiHud.h"
#include "../ui/FguiPieTimer.h"
#include "../ui/FguiPadGraphics.h"

#include "fairygui/GRoot.h"
#include "fairygui/FairyGUI.h"

#include <set>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>
#include <utility>

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

// S2: FairyGUI GRoot for the HUD layer.
// Created once in init(), used by FguiHud and FguiPadGraphics.
static fairygui::GRoot* s_fguiRoot = nullptr;

fairygui::GRoot* fguiRoot() { return s_fguiRoot; }

// ---------------------------------------------------------------------------
// TouchPadGfx (touch.ts pad.img sprites) — replaced by FguiPadGraphics in S2
// ---------------------------------------------------------------------------
// Port of touch.ts createPadSprite(): the joystick ring is the web client's
// `pad.img` (client/public/img/gui/pad.svg, a grey circle stroke). The web
// client tints the centre black at 20% alpha and the touch dot white, both
// anchored at 0.5; the scale comes from Touch (padScaleBase * 0.6 / 0.25).
// `pad.img` lives in the virtual atlas (Content/atlas/**) because that is how
// the web client resolves it; `gui/pad.png` (tools/build-gui-icons.mjs) is the
// loose fallback for a build whose atlas cache is unavailable.
TouchPadGfx::TouchPadGfx(ax::Node* parent) {
    // touch.ts's 0.6/0.25 sprite scales are relative to the *point* size of the
    // 208 px `pad.img` art drawn at 0.5 device/point scale, i.e. 125 px / 52 px
    // on screen. `gui/pad.png` (the loose fallback) is 4x that, so it needs 1/4
    // of the same scale to land on the same on-screen size.
    const bool frameReady =
        ax::SpriteFrameCache::getInstance()->getSpriteFrameByName("pad.img") != nullptr;
    _padFrame = frameReady ? "pad.img" : "gui/pad.png";
    _padScale = frameReady ? 0.25f : 0.0625f;

    auto createPadSprite = [this, parent](const ax::Color3B& tint, float alpha) -> ax::Sprite* {
        ax::Sprite* sprite = ax::Sprite::create(_padFrame);
        if (!sprite) {
            return nullptr;
        }
        sprite->setAnchorPoint(ax::Vec2(0.5f, 0.5f));
        sprite->setColor(tint);
        sprite->setOpacity(static_cast<uint8_t>(alpha * 255.0f + 0.5f));
        sprite->setVisible(false);
        parent->addChild(sprite, 1000);
        return sprite;
    };
    _center = createPadSprite(ax::Color3B(0, 0, 0), 0.2f);
    _touch = createPadSprite(ax::Color3B::WHITE, 1.0f);

    // The pad sprites are created from the menu, before any map (and therefore
    // before the atlas) is loaded; `_atlasPad` reports whether the atlas frame
    // is what is currently on the sprites, and `refreshPadFrame` re-resolves it
    // once a map load has registered the atlas.
    if (_center) {
        _textureReady = _center->getTexture() != nullptr;
        _atlasPad = _textureReady && _padFrame == "pad.img";
    }
    // Always start hidden; setVisible(false) hides either sprite.
    setVisible(false);
}

void TouchPadGfx::refreshPadFrame() {
    if (_atlasPad || !_center) {
        return;
    }
    if (!ax::SpriteFrameCache::getInstance()->getSpriteFrameByName("pad.img")) {
        return;
    }
    _padFrame = "pad.img";
    _padScale = 0.25f;
    _atlasPad = true;
    for (ax::Sprite* sprite : {_center, _touch}) {
        if (sprite) {
            sprite->setSpriteFrame("pad.img");
        }
    }
}

bool TouchPadGfx::hasTexture() const { return _textureReady; }
bool TouchPadGfx::isAtlasPad() const { return _atlasPad; }
const std::string& TouchPadGfx::padFrame() const { return _padFrame; }

void TouchPadGfx::setVisible(bool visible) {
    if (_center) _center->setVisible(visible);
    if (_touch) _touch->setVisible(visible);
}

// `scale` is the raw product Touch computes (touch.ts scales the pad.img
// sprites by padScaleBase * padScaleDown for the ring and * padScalePos for the
// dot); `_padScale` maps that onto the texture actually in use.
void TouchPadGfx::sizeSprite(ax::Sprite* sprite, float scale) {
    if (sprite) {
        sprite->setScale(scale * _padScale);
    }
}

void TouchPadGfx::setCenterPos(float x, float y) {
    if (_center) _center->setPosition(ax::Vec2(x, y));
}

void TouchPadGfx::setTouchPos(float x, float y) {
    if (_touch) _touch->setPosition(ax::Vec2(x, y));
}

void TouchPadGfx::setCenterScale(float s) {
    sizeSprite(_center, s);
}

void TouchPadGfx::setTouchScale(float s) {
    sizeSprite(_touch, s);
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

    // S2: create the FairyGUI root for gameplay HUD overlay.
    s_fguiRoot = fairygui::GRoot::create(this, 500);
    if (!s_fguiRoot) {
        AXLOGW("Failed to create FairyGUI root; HUD will be missing");
    } else {
        AXLOGI("FairyGUI GRoot created");
    }

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
    // S2: replace the axmol Sprite-based TouchPadGfx with FairyGUI pad graphics.
    _movePad = std::make_unique<FguiPadGraphics>(this, true);
    _aimPad = std::make_unique<FguiPadGraphics>(this, false);
    // Touch keeps feeding setCenterScale/setTouchScale with the raw touch.ts
    // ratios (asserted by the host tests); TouchPadGfx::sizeSprite maps those
    // onto the texture actually in use, so the on-screen ring size no longer
    // depends on the sprite's pixel size (the 208 px atlas frame vs the 4x
    // gui/pad.png fallback). Prime the sprite scale once here.
    _touch->m_update(*_movePad, *_aimPad);

    _alive = std::make_shared<std::atomic<bool>>(true);

    _game = std::make_unique<Game>();
    _game->init(this, createWebSocketConnection);

    // S2: build the FairyGUI HUD.
    _hud = std::make_unique<FguiHud>();
    _hud->build(this);

    // Priority 2: build the pie timer.
    _pieTimer = std::make_unique<PieTimer>();
    _pieTimer->build();

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
    // S2: remove atlas callback for old TouchPadGfx; FguiPadGraphics draws
    // programmatic circles and does not need pad.img frames.
    _game->setAtlasCallback([this] {
        // No-op: the pads no longer use atlas frames.
    });
    resetWorld();
    _game->setMapCallback([this](const MapMsg& msg) {
        loadAtlasesForMap(msg.mapName);
        _world->loadMap(msg);
    });
    _game->setUpdateCallback([this](const UpdateMsg& msg) { _world->applyUpdate(msg); });
    _game->setPickupSoundCallback(
        [this](const std::string& item) { onPickupSound(item); });

    // M7: the menu starts a game via enterWithJoin()/enterWithFindGame().
    _gameRoot->setVisible(false);
    _movePad->setVisible(false);
    _aimPad->setVisible(false);

    this->scheduleUpdate();
    return true;
}

void GameScene::resetWorld() {
    if (_world) {
        // The adapter nodes (and therefore every object's sprite) are owned by
        // _pixiFactory; tell the world before the factory goes away.
        _world->onFactoryReset();
    }
    _world.reset();
    if (_worldRoot) {
        static_cast<ax::Node*>(_worldRoot->native())->removeFromParent();
        _worldRoot.reset();
    }
    // Drop the previous factory (and all of its nodes) together; a stale sprite
    // from the old factory must never reach the new world's renderer.
    _pixiFactory = std::make_unique<pix::AxPixiFactory>();
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
    // The pads stay hidden until the server confirms the join (game.ts shows
    // them from the first started frame, not during matchmaking).
    _touch->display = false;
    _movePad->setVisible(false);
    _aimPad->setVisible(false);
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
    // touch.ts hideAll(): the pads stay hidden until the next match starts.
    _touch->hideAll();
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
    // S6: the pause menu is now Java (GamePauseDialog); errors are routed
    // through the bridge. The C++ overlay no longer holds the pause menu.
}

void GameScene::clearError() {
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
    // `debug.surv_debug=1` keeps the join trace (same switch MenuScene reads).
    if (ax::UserDefault::getInstance()->getStringForKey(dev::kKeyDebug, "0") == "1" && index == 0) {
        AXLOGI("[dev] joining {} (advertised urls={})", urls[0], urls.size());
    }
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

void GameScene::onPickupSound(const std::string& item) {
    if (_audio) {
        // The item id maps to a ui-channel pickup sound like "gun_pickup_01".
        const std::string soundKey = item + "_pickup_01";
        audio::PlaySoundOptions opts;
        opts.channel = "ui";
        opts.rangeMult = 48.0f;
        _audio->playSound(soundKey, opts);
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
            // The match is actually running; show the joystick pads (game.ts
            // only enables the touch display once the game starts).
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
            // touch.ts scales the locked joystick offsets with the player's
            // zoom (a scoped player pushes the pads inward proportionally).
            _touch->setPadOffsetScale(active->getZoom(_world->mobile()));
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

    // S2: route the HUD update through FguiHud instead of UiOverlay.
    if (_hud) {
        _hud->update(delta, isStarted(), isPlaying(), _game.get());
        // Emote wheel: when a touch is active on the emote button area,
        // feed the position to the wheel.
        if (_emoteButtonActive) {
            _hud->feedEmoteWheel(_emoteScreenX, _emoteScreenY, true);
        }
    }

    // Priority 2: pie timer — driven by active player action data each frame.
    if (_pieTimer && _game && isStarted()) {
        const auto& apd = _game->activePlayerData();
        if (apd.actionDirty && apd.actionDuration > 0.0f) {
            _pieTimer->start("", apd.actionTime, apd.actionDuration);
        }
        if (_pieTimer->isActive()) {
            _pieTimer->update(delta, _world ? _world->camera() : Camera(), false, 0.0f);
            // Stop when action completes.
            if (apd.actionDuration > 0.0f && apd.actionTime >= apd.actionDuration) {
                _pieTimer->stop();
            }
        }
    }

    // M7: crosshair sprite — rendered at screen center via the PIXI adapter
    // at the highest z-order so it always sits on top of the game world.
    // Crosshair defs are in GeneratedDefs.cpp and have texture names like
    // "crosshair000.img". The sprite is created once and shown/hidden.
    if (_world && _pixiFactory && isStarted() && _game->isPlaying() && _game->hasJoined()) {
        static bool crossInit = false;
        static pix::Sprite* crossSprite = nullptr;
        if (!crossInit && _worldRoot) {
            crossSprite = _pixiFactory->createSprite();
            crossSprite->setAnchor(0.5f, 0.5f);
            crossSprite->setFrame("crosshair000.img");
            crossSprite->setTint(0xffffff);
            crossSprite->setAlpha(0.8f);
            crossSprite->setScale(1.0f, 1.0f);
            _worldRoot->addChild(crossSprite);
            crossInit = true;
        }
        if (crossSprite) {
            // Position at screen center each frame (camera independent).
            const auto size = ax::Director::getInstance()->getVisibleSize();
            crossSprite->setPosition(size.width * 0.5f, size.height * 0.5f);
            crossSprite->setVisible(true);
        }
    }

    // S6: the pause menu is now Java (GamePauseDialog).
    // The C++ _overlay is never created; skip the isOpen check.
    if (_game->isPlaying() && _game->getActivePlayerId() != 0) {
        updateInput(delta);
    }
    _touch->m_update(*_movePad, *_aimPad);

    // M5: pass touchingAim + touchAimLine state from Touch to GameWorld
    // for the LineSprites aim-line rendering in the world update.
    if (_world) {
        _world->setAimLineState(_touch->touchingAim, _touch->touchAimLine);
    }
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
    // S6: _overlay is never created; the Java GamePauseDialog is the
    // actual pause. Don't block touches because of a null overlay.
    if (!_inGame || !_notifiedStarted) return;
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
