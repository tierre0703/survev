#ifdef SURV_UI_NO_AXMOL
// Host build stubs
#else

#include "FguiPadGraphics.h"

#include "fairygui/GRoot.h"
#include "fairygui/GGraph.h"
#include "fairygui/GComponent.h"

#include "cocos2d.h"

USING_NS_AX;

namespace surv {

namespace {

// The base size of the pad ring in FG design-space pixels.
// Matches the 208 px atlas frame relative to a 0.25 pad scale.
constexpr float kPadBaseSize = 208.0f;

// Ring line width
constexpr float kRingLineWidth = 4.0f;

// Center dot radius ratio to the ring size
constexpr float kDotRatio = 0.375f;

// Ring colours: move pad = blue/white, aim pad = red/orange (matches web client)
const Color4F kMoveRingColor(0.4f, 0.6f, 1.0f, 0.5f);   // blue semi-transparent
const Color4F kMoveRingFill(0.0f, 0.0f, 0.0f, 0.0f);     // no fill
const Color4F kMoveDotColor(1.0f, 1.0f, 1.0f, 0.8f);     // white dot
const Color4F kAimRingColor(1.0f, 0.5f, 0.2f, 0.5f);     // orange semi-transparent
const Color4F kAimRingFill(0.0f, 0.0f, 0.0f, 0.0f);       // no fill
const Color4F kAimDotColor(1.0f, 0.8f, 0.4f, 0.8f);       // light orange dot

} // namespace

FguiPadGraphics::FguiPadGraphics(ax::Scene* scene, bool isMovePad)
    : _isMovePad(isMovePad)
{
    fairygui::GRoot* root = fairygui::GRoot::getInstance();
    if (!root) {
        return;
    }

    // Center ring (circle outline + no fill)
    _centerRing = fairygui::GGraph::create();
    const Color4F& ringColor = _isMovePad ? kMoveRingColor : kAimRingColor;
    const Color4F& ringFill = _isMovePad ? kMoveRingFill : kAimRingFill;
    _centerRing->drawEllipse(kPadBaseSize, kPadBaseSize,
                             static_cast<int>(kRingLineWidth),
                             ringColor, ringFill);
    _centerRing->setVisible(false);
    _centerRing->setPivot(0.5f, 0.5f);
    root->addChild(_centerRing);

    // Touch dot (solid circle)
    _touchDot = fairygui::GGraph::create();
    const Color4F& dotColor = _isMovePad ? kMoveDotColor : kAimDotColor;
    const float dotSize = kPadBaseSize * kDotRatio;
    _touchDot->drawEllipse(dotSize, dotSize, 0,
                           Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                           dotColor);
    _touchDot->setVisible(false);
    _touchDot->setPivot(0.5f, 0.5f);
    root->addChild(_touchDot);
}

FguiPadGraphics::~FguiPadGraphics() {
    if (_centerRing) {
        _centerRing->removeFromParent();
    }
    if (_touchDot) {
        _touchDot->removeFromParent();
    }
}

void FguiPadGraphics::setVisible(bool visible) {
    if (_centerRing) _centerRing->setVisible(visible);
    if (_touchDot) _touchDot->setVisible(visible);
}

void FguiPadGraphics::setCenterPos(float x, float y) {
    if (_centerRing) {
        _centerRing->setPosition(x, y);
    }
}

void FguiPadGraphics::setTouchPos(float x, float y) {
    if (_touchDot) {
        _touchDot->setPosition(x, y);
    }
}

void FguiPadGraphics::setCenterScale(float s) {
    _centerScale = s;
    if (_centerRing) {
        _centerRing->setScale(_centerScale, _centerScale);
    }
}

void FguiPadGraphics::setTouchScale(float s) {
    _touchScale = s;
    if (_touchDot) {
        _touchDot->setScale(_touchScale, _touchScale);
    }
}

void FguiPadGraphics::update() {
    // No per-frame work needed — positions/scales are set reactively.
}

} // namespace surv

#endif // SURV_UI_NO_AXMOL