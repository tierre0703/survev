#pragma once
// Port of client/src/ui/touch.ts dual-joystick logic.
// Kept engine-independent: movement/aim math is identical to the web client;
// rendering + touch input are provided through the minimal PadGraphics
// interface so it can be driven by axmol (or a test harness).
#include "../core/MathUtil.h"
#include "../core/Vec2.h"
#include "Device.h"

#include <cmath>

namespace surv {

struct TouchPoint {
    bool active = false;   // !isDead
    Vec2 posDown;          // touch start (screen space)
    Vec2 pos;              // current (screen space)
};

// Minimal rendering interface for one joystick pad (axmol implements this).
struct PadGraphics {
    virtual ~PadGraphics() = default;
    virtual void setVisible(bool visible) = 0;
    virtual void setCenterPos(float x, float y) = 0;
    virtual void setTouchPos(float x, float y) = 0;
    virtual void setCenterScale(float s) = 0;
    virtual void setTouchScale(float s) = 0;
    virtual void update() = 0;
};

struct TouchStyle {
    enum Value {
        Locked = 0,
        Anywhere = 1,
    };
};

class Touch {
public:
    static constexpr float kDeadZone = 2.0f;
    static constexpr float kSensitivityThreshold = 0.00001f;

    // config defaults
    float padScaleBase = 1.0f;
    float padScaleDown = 0.6f;
    float padScalePos = 0.25f;
    float padPosBase = 48.0f;
    float padPosRange = 0.0f;
    float movePadDetectMult = 1.0f;
    float shotPadDetectMult = 1.075f;
    // The web client scales the locked pad offsets by the on-screen zoom
    // (touch.ts `padOffsetScale`): at a wider zoom the pads sit proportionally
    // further in. 1.0 = the unscoped view; GameWorld publishes the active
    // player's zoom each frame.
    float padOffsetScale = 1.0f;

    bool moveDetected = false;
    bool shotDetected = false;
    bool shotDetectedOld = false;
    bool touchingAim = false;
    bool display = true;

    TouchStyle::Value moveStyle = TouchStyle::Anywhere;
    TouchStyle::Value aimStyle = TouchStyle::Anywhere;
    bool touchAimLine = true;
    // While moving without touching the aim pad, the aim direction follows the
    // move direction once this cooldown has elapsed (port of touch.ts).
    float turnDirCooldown = 0.5f;
    float turnDirTicker = 0.0f;

    TouchPoint touches[4];

    Vec2 leftLockedPadCenter;
    Vec2 rightLockedPadCenter;
    Vec2 padPosBase2;

    struct Movement {
        Vec2 toMoveDir = Vec2(1.0f, 0.0f);
        float toMoveLen = 0.0f;
    };
    Movement analogMovement;

    struct AimMovement {
        Vec2 toAimDir = Vec2(1.0f, 0.0f);
        float toAimLen = 0.0f;
    };
    AimMovement aimMovement;

    struct Pad {
        bool touched = false;
        Vec2 centerPos;
        Vec2 touchPos;
    };
    Pad pads[2];

    // Last resize arguments, so `setPadOffsetScale` can re-derive the centers.
    float _screenWidth = 0.0f;
    float _screenHeight = 0.0f;
    bool _landscape = true;
    // The last `padOffsetScale` the locked centers were derived with.
    float _appliedOffsetScale = 1.0f;

    explicit Touch(float screenWidth, float screenHeight, bool isLandscape) {
        resize(screenWidth, screenHeight, isLandscape);
    }

    // Left/right locked pad centers, re-derived on resize (and whenever the
    // zoom-driven `padOffsetScale` changes).
    void resize(float screenWidth, float screenHeight, bool isLandscape) {
        padScaleBase = isLandscape ? 1.0f : 0.8f;
        // The GeoJSON-ish zoom scale may have been published since the last
        // resize; fold it into the pad geometry (touch.ts padOffsetScale).
        padPosRange = padPosBase * padScaleBase * padOffsetScale;

        _screenWidth = screenWidth;
        _screenHeight = screenHeight;
        _landscape = isLandscape;
        // touch.ts resize(): tablets shift the portrait pad offsets inward
        // (x * 1.25); the landscape offsets are unchanged (`* 1`).
        const float offX = (isLandscape ? 126.0f : 96.0f * (::ui::Device::get().tablet ? 1.25f : 1.0f)) *
            padOffsetScale;
        const float offY = (isLandscape ? 100.0f : 160.0f) * padOffsetScale;
        leftLockedPadCenter = Vec2(offX, screenHeight - offY);
        rightLockedPadCenter = Vec2(screenWidth - offX, screenHeight - offY);
        _appliedOffsetScale = padOffsetScale;
    }

    // Returns true if a touch on the left side should control movement.
    bool isLeftSideTouch(float posX, float screenWidth) const {
        return posX < screenWidth * 0.5f;
    }

    // touch.ts `padOffsetScale`: the locked offsets (and the pad radius) grow
    // with the active player's zoom, so the joysticks keep their relative
    // on-screen position when the view widens (1.0 = the unscoped view).
    void setPadOffsetScale(float scale) {
        if (scale <= 0.0f || scale == padOffsetScale) {
            return;
        }
        padOffsetScale = scale;
        resize(_screenWidth, _screenHeight, _landscape);
        updatePadLayout(moveStyle, true);
        updatePadLayout(aimStyle, false);
    }

    // Re-derives one pad's drawn centre/dot from its last move/aim sample.
    // getMovement/getAim run before the draw pass each frame, so a resize or
    // zoom change between frames is picked up without losing the sample.
    void updatePadLayout(TouchStyle::Value style, bool movePad) {
        const Vec2& locked = movePad ? leftLockedPadCenter : rightLockedPadCenter;
        Pad& pad = pads[movePad ? 0 : 1];
        if (pad.touched && style == TouchStyle::Anywhere) {
            pad.centerPos = pad.touchPos;
        } else {
            pad.centerPos = locked;
        }
        if (!pad.touched) {
            pad.touchPos = locked;
        }
    }

    Movement getMovement(float screenWidth) {
        Vec2 posDown;
        Vec2 pos;
        bool touched = false;
        moveDetected = false;

        for (int i = 0; i < 4; i++) {
            const TouchPoint& t = touches[i];
            if (t.active && isLeftSideTouch(t.posDown.x, screenWidth)) {
                const Vec2 center = moveStyle == TouchStyle::Anywhere ? t.posDown : leftLockedPadCenter;
                const Vec2 pull = v2Sub(t.pos, center);
                const float dist = v2Length(pull);

                if (dist > kDeadZone) {
                    const float toMoveLen = (dist - kDeadZone)
                        / (padPosRange / movePadDetectMult - kDeadZone);
                    const Vec2 toMoveDir = toMoveLen > kSensitivityThreshold
                        ? v2Div(pull, toMoveLen)
                        : analogMovement.toMoveDir;
                    analogMovement.toMoveDir = Vec2(toMoveDir.x, toMoveDir.y * -1.0f);
                    analogMovement.toMoveLen = toMoveLen;
                    moveDetected = true;
                }
                pos = getConstrainedPos(center, t.pos, dist);
                posDown = center;
                touched = true;
                break;
            }
        }

        pads[0].touched = touched;
        if (touched && moveStyle == TouchStyle::Anywhere) {
            pads[0].centerPos = posDown;
        } else {
            pads[0].centerPos = leftLockedPadCenter;
        }
        pads[0].touchPos = touched ? pos : leftLockedPadCenter;
        return analogMovement;
    }

    struct AimResult {
        AimMovement aimMovement;
        bool touched = false;
    };

    AimResult getAim(bool isHoldingThrowable, float screenWidth) {
        Vec2 posDown;
        Vec2 pos;
        bool touched = false;

        for (int i = 0; i < 4; i++) {
            const TouchPoint& t = touches[i];
            if (t.active && !isLeftSideTouch(t.posDown.x, screenWidth)) {
                const Vec2 center = aimStyle == TouchStyle::Anywhere ? t.posDown : rightLockedPadCenter;
                const Vec2 pull = v2Sub(t.pos, center);
                const float dist = v2Length(pull);

                if (dist > kDeadZone) {
                    const Vec2 toAimPos = v2Sub(t.pos, center);
                    const float toAimLen = v2Length(toAimPos);
                    const Vec2 toAimDir = toAimLen > kSensitivityThreshold
                        ? v2Div(toAimPos, toAimLen)
                        : aimMovement.toAimDir;
                    aimMovement.toAimDir = Vec2(toAimDir.x, toAimDir.y * -1.0f);
                    aimMovement.toAimLen = toAimLen;
                } else {
                    aimMovement.toAimLen = 0.0f;
                }

                pos = getConstrainedPos(center, t.pos, dist);
                posDown = center;
                touched = true;
                break;
            }
        }

        // Detect if user has moved far enough from center to shoot.
        shotDetectedOld = shotDetected;
        shotDetected = aimMovement.toAimLen > padPosRange / shotPadDetectMult && touched;
        touchingAim = touched;

        // Special-case throwable logic: once priming a grenade, dragging back
        // into the aim circle won't release it; only lifting the finger throws.
        if (isHoldingThrowable && shotDetectedOld && touched) {
            shotDetected = true;
        }

        pads[1].touched = touched;
        if (touched && aimStyle == TouchStyle::Anywhere) {
            pads[1].centerPos = posDown;
        } else {
            pads[1].centerPos = rightLockedPadCenter;
        }
        pads[1].touchPos = touched ? pos : rightLockedPadCenter;

        AimResult r;
        r.aimMovement = aimMovement;
        r.touched = pads[1].touched;
        return r;
    }

    // Update pad graphics through the renderer interface. The scale arguments
    // mirror touch.ts (`padScaleBase * padScaleDown` / `* padScalePos`); the
    // axmol implementation maps them onto the texture it loaded.
    void m_update(PadGraphics& movePad, PadGraphics& aimPad) {
        // GameScene publishes the active player's zoom here every frame; the
        // host tests drive m_update directly, so make sure the locked centers
        // and the pad layout reflect the current scale before reading them.
        if (padOffsetScale != _appliedOffsetScale) {
            resize(_screenWidth, _screenHeight, _landscape);
        }
        for (int i = 0; i < 2; i++) {
            PadGraphics* gfx = i == 0 ? &movePad : &aimPad;
            // getMovement/getAim (which publish the pad centres) only run while
            // a match is live; when they are idle the pads keep their locked
            // position, so refresh that here rather than drawing a stale centre.
            const Vec2& locked = i == 0 ? leftLockedPadCenter : rightLockedPadCenter;
            if (pads[i].centerPos.x == 0.0f && pads[i].centerPos.y == 0.0f) {
                gfx->setCenterPos(locked.x, locked.y);
                gfx->setTouchPos(locked.x, locked.y);
            } else {
                gfx->setCenterPos(pads[i].centerPos.x, pads[i].centerPos.y);
                gfx->setTouchPos(pads[i].touchPos.x, pads[i].touchPos.y);
            }
            gfx->setVisible(display);
            gfx->setCenterScale(padScaleBase * padScaleDown);
            gfx->setTouchScale(padScaleBase * padScalePos);
            gfx->update();
        }
    }

    Vec2 getConstrainedPos(const Vec2& posDown, const Vec2& pos, float dist) const {
        if (dist <= padPosRange) {
            return pos;
        }
        const float x = pos.x - posDown.x;
        const float y = pos.y - posDown.y;
        const float radians = std::atan2(y, x);
        return Vec2(std::cos(radians) * padPosRange + posDown.x,
                    std::sin(radians) * padPosRange + posDown.y);
    }

    void toggleMoveStyle() {
        setMoveStyle(moveStyle == TouchStyle::Locked ? TouchStyle::Anywhere : TouchStyle::Locked);
    }

    void setMoveStyle(TouchStyle::Value style) {
        moveStyle = style;
    }

    void toggleAimStyle() {
        setAimStyle(aimStyle == TouchStyle::Locked ? TouchStyle::Anywhere : TouchStyle::Locked);
    }

    void setAimStyle(TouchStyle::Value style) {
        aimStyle = style;
    }

    // Overrides the current aim direction (used by the turn-to-move logic).
    void setAimDir(const Vec2& dir) {
        aimMovement.toAimDir = v2Copy(dir);
    }

    void toggleAimLine() {
        touchAimLine = !touchAimLine;
    }

    void setTouchAimLine(bool isOn) {
        touchAimLine = isOn;
    }

    void hideAll() {
        display = false;
    }
};

} // namespace surv