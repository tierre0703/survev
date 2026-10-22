#include "PadGraphicsAxmol.h"

#include "axmol.h"

USING_NS_AX;

namespace surv {

namespace {
// `Content/gui/pad.png` is the 4x (832 px) copy of the web client's 208 px
// `pad.img` ring. touch.ts sizes the ring by `padScaleBase * padScaleDown`
// (0.6), i.e. 208 * 0.6 = 124.8 px, and axmol's sprite content size for this
// loose (unsheeted) PNG is its raw 832 px. The multiplier that turns the Touch
// scale into that size is therefore 124.8 / (0.6 * 832) = 0.25.
constexpr float kPadSpriteScale = 0.25f;
// Small white dot at the touch position: pad.img at `padScalePos` (0.25) is
// 208 * 0.25 = 52 px across, so the unit DrawNode circle needs a 26 px radius
// at that Touch scale (0.25 * 104 = 26).
constexpr float kDotBaseRadius = 104.0f;
} // namespace

PadGraphicsAxmol::PadGraphicsAxmol(ax::Node* parent, bool isMovePad)
    : _isMovePad(isMovePad) {
    const Size visible = Director::getInstance()->getVisibleSize();
    _screenHeight = visible.height;
    _root = ax::Node::create();
    _root->setPosition(ax::Vec2::ZERO);
    if (parent) {
        // The pad lives on the game root (the same node the HUD uses), so it is
        // hidden/shown with the match and draws above the world layers.
        parent->addChild(_root, 1000);
    }

    // Ring: the web client's pad.img. Its SVG is a black circle, so the ring is
    // a faint dark ring (touch.ts draws it at alpha 0.2).
    _center = ax::Sprite::create("gui/pad.png");
    if (_center) {
        _center->setAnchorPoint(ax::Vec2(0.5f, 0.5f));
        _center->setOpacity(51); // ~0.2, like touch.ts
        _root->addChild(_center);
    }
    // Touch dot: a solid white circle (the black pad art cannot be tinted
    // white), sized like the web's small pad.img copy.
    _dot = ax::DrawNode::create();
    const Color4F dotColor = isMovePad ? Color4F(1.0f, 1.0f, 1.0f, 0.85f)
                                       : Color4F(1.0f, 0.85f, 0.6f, 0.9f);
    _dot->drawSolidCircle(ax::Vec2::ZERO, 1.0f, 0.0f, 32, dotColor);
    _root->addChild(_dot);
    setVisible(false);
}

ax::Vec2 PadGraphicsAxmol::toNodePos(float x, float y) const {
    return ax::Vec2(x, _screenHeight - y);
}

void PadGraphicsAxmol::setVisible(bool visible) {
    if (_root) {
        _root->setVisible(visible);
    }
}

void PadGraphicsAxmol::setCenterPos(float x, float y) {
    if (_center) {
        _center->setPosition(toNodePos(x, y));
    }
}

void PadGraphicsAxmol::setTouchPos(float x, float y) {
    if (_dot) {
        _dot->setPosition(toNodePos(x, y));
    }
}

void PadGraphicsAxmol::setCenterScale(float s) {
    if (_center) {
        _center->setScale(s * kPadSpriteScale);
    }
}

void PadGraphicsAxmol::setTouchScale(float s) {
    if (_dot) {
        _dot->setScale(s * kDotBaseRadius);
    }
}

void PadGraphicsAxmol::update() {}

} // namespace surv
