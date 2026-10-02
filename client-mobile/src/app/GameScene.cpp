#include "GameScene.h"
#include "../net/Messages.h"

USING_NS_AX;

namespace surv {

namespace {
const char* kPadTexture = "img/gui/pad.img";
const char* kDotTexture = "img/gui/dot.img";
}

// ---------------------------------------------------------------------------
// TouchPadGfx
// ---------------------------------------------------------------------------
TouchPadGfx::TouchPadGfx(ax::Node* parent) {
    _center = ax::Sprite::create(kPadTexture);
    _center->setAnchorPoint(Vec2(0.5f, 0.5f));
    _center->setOpacity(50); // alpha ~0.2
    _center->setVisible(false);
    parent->addChild(_center, 1000);

    _touch = ax::Sprite::create(kPadTexture);
    _touch->setAnchorPoint(Vec2(0.5f, 0.5f));
    _touch->setColor(Color3B(255, 255, 255));
    _touch->setVisible(false);
    parent->addChild(_touch, 1001);
}

void TouchPadGfx::setVisible(bool visible) {
    _center->setVisible(visible);
    _touch->setVisible(visible);
}

void TouchPadGfx::setCenterPos(float x, float y) {
    _center->setPosition(Vec2(x, y));
}

void TouchPadGfx::setTouchPos(float x, float y) {
    _touch->setPosition(Vec2(x, y));
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
ax::Scene* GameScene::createScene() {
    auto scene = new GameScene();
    if (scene && scene->init()) {
        scene->autorelease();
        return scene;
    }
    delete scene;
    return nullptr;
}

bool GameScene::init() {
    if (!Scene::init()) {
        return false;
    }

    const Size visible = Director::getInstance()->getVisibleSize();
    const bool isLandscape = visible.width >= visible.height;

    _gameRoot = ax::Node::create();
    this->addChild(_gameRoot);

    // Touch input (dual joystick). In axmol, touch events are delivered via
    // EventListenerTouchOneByOne; wire them here (TODO M5):
    //   auto listener = EventListenerTouchOneByOne::create();
    //   listener->onTouchBegan = [this](Touch* t, Event*) {
    //       onTouchBegan(t->getLocation().x, t->getLocation().y, 0); return true; };
    //   listener->onTouchMoved = ...; listener->onTouchEnded = ...;
    //   _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

    _touch = std::make_unique<Touch>(visible.width, visible.height, isLandscape);
    _movePad = std::make_unique<TouchPadGfx>(this);
    _aimPad = std::make_unique<TouchPadGfx>(this);

    _game = std::make_unique<Game>();
    _game->init(this);

    return true;
}

void GameScene::update(float delta) {
    _game->update(delta);
    if (_game->getActivePlayerId() != 0) {
        // Feed current joystick state into the input message (see Game::update).
        const Touch::Movement move = _touch->getMovement(getContentSize().width);
        (void)move;
    }
    _touch->m_update(*_movePad, *_aimPad);
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