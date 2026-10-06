#pragma once
// Port of the touch branch of client/src/game.ts update() input collection.
// Turns the engine-independent Touch joystick state into an InputMsg, including
// the throwable priming latch (passed through to Touch::getAim) and the aim
// direction. Kept axmol-free so it can be unit-tested on the host.
#include "../core/GameConfig.h"
#include "../core/MathUtil.h"
#include "../net/Messages.h"
#include "../ui/Touch.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace surv {

class TouchInput {
public:
    // Builds this frame's InputMsg from the dual-joystick state. `dt` advances
    // the turn-to-move-direction cooldown used when the aim pad isn't touched.
    InputMsg build(Touch& touch,
                   float screenWidth,
                   bool isHoldingThrowable,
                   bool portrait,
                   float dt) {
        const Touch::Movement move = touch.getMovement(screenWidth);
        const Touch::AimResult aim = touch.getAim(isHoldingThrowable, screenWidth);

        // Aim direction: keep the last aim while waiting out the turn ticker,
        // otherwise face the move direction (matches game.ts).
        Vec2 aimDir = aim.aimMovement.toAimDir;
        touch.turnDirTicker -= dt;
        if (touch.moveDetected && !aim.touched) {
            const Vec2 touchDir = v2NormalizeSafe(move.toMoveDir, Vec2(1.0f, 0.0f));
            const Vec2 modifiedAimDir =
                touch.turnDirTicker < 0.0f ? touchDir : aim.aimMovement.toAimDir;
            touch.setAimDir(modifiedAimDir);
            aimDir = modifiedAimDir;
        }
        if (aim.touched) {
            touch.turnDirTicker = touch.turnDirCooldown;
        }

        InputMsg msg;
        if (touch.moveDetected) {
            msg.touchMoveDir = v2NormalizeSafe(move.toMoveDir, Vec2(1.0f, 0.0f));
            msg.touchMoveLen = static_cast<uint8_t>(
                std::lround(math::clamp(move.toMoveLen, 0.0f, 1.0f) * 255.0f));
        } else {
            msg.touchMoveLen = 0;
        }
        msg.touchMoveActive = true;

        // Aim length is the joystick pull scaled into throwable range.
        const float aimLen = aim.aimMovement.toAimLen;
        msg.toMouseLen =
            math::clamp(aimLen / touch.padPosRange, 0.0f, 1.0f) * PlayerConfig::throwableMaxMouseDist;
        msg.toMouseDir = aimDir;

        msg.touchMoveDir = v2NormalizeSafe(msg.touchMoveDir, Vec2(1.0f, 0.0f));
        msg.touchMoveLen = static_cast<uint8_t>(std::min<uint16_t>(msg.touchMoveLen, 255));
        msg.toMouseDir = v2NormalizeSafe(msg.toMouseDir, Vec2(1.0f, 0.0f));
        msg.toMouseLen = math::clamp(msg.toMouseLen, 0.0f, Constants::MouseMaxDist);

        // Touch fires whenever the aim pull passes the shot threshold; the
        // throwable latch keeps this held once priming has begun.
        msg.shootStart = touch.shotDetected;
        msg.shootHold = touch.shotDetected;
        msg.portrait = portrait;
        return msg;
    }
};

} // namespace surv
