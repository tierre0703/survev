#pragma once
// Port of client/src/ui/touch.ts dual-joystick logic.
// Kept engine-independent: movement/aim math is identical to the web client;
// rendering + touch input are provided through the minimal PadGraphics
// interface so it can be driven by axmol (or a test harness).
#include "../core/MathUtil.h"
#include "../core/Vec2.h"

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

    bool moveDetected = false;
    bool shotDetected = false;
    bool shotDetectedOld = false;
    bool touchingAim = false;
    bool display = true;

    TouchStyle::Value moveStyle = TouchStyle::Locked;
    TouchStyle::Value aimStyle = TouchStyle::Locked;
    bool touchAimLine = true;

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

    explicit Touch(float screenWidth, float screenHeight, bool isLandscape) {
        resize(screenWidth, screenHeight, isLandscape);
    }

    // Left/right locked pad centers, re-derived on resize.
    void resize(float screenWidth, float screenHeight, bool isLandscape) {
        padScaleBase = isLandscape ? 1.0f : 0.8f;
        padPosRange = padPosBase * padScaleBase;

        const float offX = isLandscape ? 126.0f : 96.0f;
        const float offY = isLandscape ? 100.0f : 160.0f;
        leftLockedPadCenter = Vec2(offX, screenHeight - offY);
        rightLockedPadCenter = Vec2(screenWidth - offX, screenHeight - offY);
    }

    // Returns true if a touch on the left side should control movement.
    bool isLeftSideTouch(float posX, float screenWidth) const {
        return posX < screenWidth * 0.5f;
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

    // Update pad graphics through the renderer interface.
    void m_update(PadGraphics& movePad, PadGraphics& aimPad) {
        for (int i = 0; i < 2; i++) {
            PadGraphics* gfx = i == 0 ? &movePad : &aimPad;
            const Pad& pad = pads[i];
            gfx->setVisible(display);
            gfx->setCenterPos(pad.centerPos.x, pad.centerPos.y);
            gfx->setTouchPos(pad.touchPos.x, pad.touchPos.y);
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

    void toggleAimLine() {
        touchAimLine = !touchAimLine;
    }

    void hideAll() {
        display = false;
    }
};

} // namespace surv