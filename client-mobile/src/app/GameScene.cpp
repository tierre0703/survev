#include "GameScene.h"
#include "DevConfig.h"
#include "../net/Api.h"
#include "../net/Messages.h"
#include "../net/WebSocketConnection.h"
#include "../game/GameWorld.h"
#include "../game/Map.h"
#include "../game/objects/Barns.h"
#include "../render/AxmolPixi.h"
#include "../render/GeneratedDefs.h"

USING_NS_AX;

namespace surv {

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
    _pixiFactory = std::make_unique<pix::AxPixiFactory>();
    _world = std::make_unique<GameWorld>(_pixiFactory.get(), false);
    pix::Container* worldRoot = _pixiFactory->createContainer();
    _world->attachTo(worldRoot);
    _gameRoot->addChild(static_cast<ax::Node*>(worldRoot->native()));
    _world->setScreenSize(visible.width, visible.height);
    _game->setMapCallback([this](const MapMsg& msg) { _world->loadMap(msg); });
    _game->setUpdateCallback([this](const UpdateMsg& msg) { _world->applyUpdate(msg); });

    // M3: with no menu UI yet, auto-connect if a join target is configured.
    maybeAutoConnect();

    // Drive update() every frame.
    this->scheduleUpdate();

    return true;
}

GameScene::~GameScene() {
    if (_alive) {
        *_alive = false;
    }
    if (_game) {
        _game->free();
    }
}

void GameScene::maybeAutoConnect() {
    auto* ud = ax::UserDefault::getInstance();
    const std::string url(ud->getStringForKey(dev::kKeyJoinUrl, dev::kJoinUrl));
    const std::string token(ud->getStringForKey(dev::kKeyJoinToken, dev::kJoinToken));
    const std::string apiUrl(ud->getStringForKey(dev::kKeyApiUrl, dev::kApiBaseUrl));
    const std::string region(ud->getStringForKey(dev::kKeyRegion, dev::kRegion));

    if (!url.empty()) {
        AXLOGI("Connecting to dev join url {}", url);
        connectDirect(url, token);
    } else if (!apiUrl.empty()) {
        AXLOGI("Finding a game via {}", apiUrl);
        connectViaFindGame(apiUrl, region, dev::kGameModeIdx);
    } else {
        AXLOGI("No dev join target configured (set UserDefault surv_joinUrl or surv_apiUrl)");
    }
}

void GameScene::connectDirect(const std::string& url, const std::string& joinToken) {
    if (_game) {
        _game->tryJoinGame(url, joinToken);
    }
}

void GameScene::connectViaFindGame(const std::string& apiBaseUrl,
                                   const std::string& region,
                                   int gameModeIdx) {
    FindGameBody body;
    body.region = region;
    body.version = defs::kProtocolVersion;
    body.playerCount = 1;
    body.autoFill = true;
    body.gameModeIdx = gameModeIdx;

    // Guard against the HTTP callback outliving the scene.
    std::weak_ptr<std::atomic<bool>> weak = _alive;
    findGame(apiBaseUrl, body, [this, weak](FindGameResult result) {
        const auto alive = weak.lock();
        if (!alive || !*alive) {
            return;
        }
        if (result.ok && !result.data.urls.empty()) {
            connectDirect(result.data.urls.front(), result.data.joinToken);
        } else {
            AXLOGW("find_game failed: {}", result.error);
        }
    });
}

void GameScene::pauseGame() {
    if (_game) {
        _game->pause();
    }
}

void GameScene::resumeGame() {
    if (_game) {
        _game->resume();
    }
}

void GameScene::update(float delta) {
    _game->update(delta);

    if (_world) {
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
        }
    }

    if (_game->isPlaying() && _game->getActivePlayerId() != 0) {
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