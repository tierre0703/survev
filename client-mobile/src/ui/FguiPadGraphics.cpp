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

// The base size of the pad ring — stubbed; actual FGUI circles need testing.
constexpr float kPadBaseSize = 104.0f;

} // namespace

FguiPadGraphics::FguiPadGraphics(ax::Scene* scene, bool isMovePad)
    : _isMovePad(isMovePad)
{
    (void)scene;
    // Stub — FGUI programmatic GGraph.create() without a loaded package
    // may crash. The old TouchPadGfx handles the pads in the meantime.
    setVisible(false);
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
        _centerRing->setPosition(x - kPadBaseSize * 0.5f * _centerScale,
                                  y - kPadBaseSize * 0.5f * _centerScale);
    }
}

void FguiPadGraphics::setTouchPos(float x, float y) {
    if (_touchDot) {
        float s = kPadBaseSize * 0.4f * _touchScale;
        _touchDot->setPosition(x - s * 0.5f, y - s * 0.5f);
    }
}

void FguiPadGraphics::setCenterScale(float s) {
    _centerScale = s;
}

void FguiPadGraphics::setTouchScale(float s) {
    _touchScale = s;
}

void FguiPadGraphics::update() {}

} // namespace surv

#endif // SURV_UI_NO_AXMOL