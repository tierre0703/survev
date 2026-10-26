#pragma once
// InputMsg send gating (P1.6), a direct port of game.ts:731-771:
//
//   * send when the message differs from the last one sent,
//   * send at least once per second ("keepalive"),
//   * one-shot `inputs` force a send only while they are non-empty (the web
//     deliberately does not treat a just-cleared action list as a change),
//   * aim directions use a 0.1 degree threshold and aim length a 0.5 epsilon.
//
// Engine-independent so `surv_tests` can cover the policy without a socket.
#include "../core/MathUtil.h"
#include "../core/Vec2.h"
#include "../net/Messages.h"

#include <cmath>

namespace surv {

class InputSync {
public:
    static constexpr float kKeepaliveSeconds = 1.0f;
    static constexpr float kAimAngleDegrees = 0.1f;
    static constexpr float kAimLenEpsilon = 0.5f;

    // `dt` is the frame delta; the timeout is decremented exactly like
    // `m_inputMsgTimeout -= dt` in game.ts.
    bool shouldSend(const InputMsg& msg, float dt) {
        _timeout -= dt;
        if (_timeout < 0.0f) {
            return true;
        }
        return differs(msg);
    }

    // Call after a successful Game::sendInput().
    void markSent(const InputMsg& msg) {
        _prev = msg;
        _prev.seq = 0; // never part of the comparison
        _hasPrev = true;
        _timeout = kKeepaliveSeconds;
    }

    void reset() {
        _hasPrev = false;
        _timeout = 0.0f;
        _prev = InputMsg{};
    }

    bool hasPrevious() const { return _hasPrev; }

private:
    static float angleDegrees(const Vec2& a, const Vec2& b) {
        const float dot = math::clamp(v2Dot(a, b), -1.0f, 1.0f);
        return std::acos(dot) * (180.0f / 3.14159265358979f);
    }

    bool differs(const InputMsg& msg) const {
        if (!_hasPrev) {
            // The first message after a (re)join always goes out.
            return true;
        }
        // game.ts: `k == "inputs"` -> diff only while non-empty.
        if (!msg.inputs.empty()) {
            return true;
        }
        if (msg.toMouseLen != _prev.toMouseLen &&
            std::fabs(msg.toMouseLen - _prev.toMouseLen) > kAimLenEpsilon) {
            return true;
        }
        if (angleDegrees(msg.toMouseDir, _prev.toMouseDir) > kAimAngleDegrees) {
            return true;
        }
        if (msg.touchMoveActive && _prev.touchMoveActive &&
            angleDegrees(msg.touchMoveDir, _prev.touchMoveDir) > kAimAngleDegrees) {
            return true;
        }
        // `shootStart` is sent even when it matches true, so a held press is not
        // lost if the previous message was dropped/reordered.
        if (msg.shootStart) {
            return true;
        }
        return msg.moveLeft != _prev.moveLeft || msg.moveRight != _prev.moveRight ||
            msg.moveUp != _prev.moveUp || msg.moveDown != _prev.moveDown ||
            msg.shootHold != _prev.shootHold || msg.portrait != _prev.portrait ||
            msg.touchMoveActive != _prev.touchMoveActive ||
            msg.touchMoveLen != _prev.touchMoveLen || msg.useItem != _prev.useItem;
    }

    InputMsg _prev;
    float _timeout = 0.0f;
    bool _hasPrev = false;
};

} // namespace surv
