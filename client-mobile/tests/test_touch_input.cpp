// M5 host tests: Touch -> InputMsg collection (throwable latch, aim line,
// move/aim scaling). Drives the engine-independent TouchInput builder without
// axmol.
#include "TestFramework.h"
#include "../src/game/TouchInput.h"

using namespace surv;
using namespace surv_test;

namespace {

// Landscape 800x480: locked pads sit at (126,380) and (674,380), range 48.
constexpr float kWidth = 800.0f;
constexpr float kHeight = 480.0f;

void setMovePull(Touch& t, float dx, float dy) {
    t.touches[0].active = true;
    t.touches[0].posDown = Vec2(100.0f, 380.0f);
    t.touches[0].pos = Vec2(126.0f + dx, 380.0f + dy);
}

void setAimPull(Touch& t, float dx, float dy) {
    t.touches[1].active = true;
    t.touches[1].posDown = Vec2(700.0f, 380.0f);
    t.touches[1].pos = Vec2(674.0f + dx, 380.0f + dy);
}

} // namespace

TEST(touch_input_move_and_aim) {
    Touch t(kWidth, kHeight, true);
    setMovePull(t, 48.0f, 0.0f);   // full right on the move pad
    setAimPull(t, 0.0f, -60.0f);   // up, past the shot threshold

    TouchInput ti;
    const InputMsg msg = ti.build(t, kWidth, false, false, 0.016f);

    CHECK(msg.touchMoveActive);
    CHECK_EQ(static_cast<int>(msg.touchMoveLen), 255);
    CHECK_NEAR(msg.touchMoveDir.x, 1.0f, 0.01f);
    CHECK_NEAR(msg.touchMoveDir.y, 0.0f, 0.01f);

    // Aim: pull is up, so screen->game y is flipped to +1.
    CHECK_NEAR(msg.toMouseDir.x, 0.0f, 0.01f);
    CHECK_NEAR(msg.toMouseDir.y, 1.0f, 0.01f);
    CHECK_NEAR(msg.toMouseLen, PlayerConfig::throwableMaxMouseDist, 0.01f);

    CHECK(msg.shootStart);
    CHECK(msg.shootHold);
    CHECK(!msg.portrait);
}

TEST(touch_input_no_move_zero_len) {
    Touch t(kWidth, kHeight, true);
    TouchInput ti;
    const InputMsg msg = ti.build(t, kWidth, false, true, 0.016f);

    CHECK(msg.touchMoveActive);
    CHECK_EQ(static_cast<int>(msg.touchMoveLen), 0);
    CHECK(!msg.shootStart);
    CHECK(!msg.shootHold);
    CHECK(msg.portrait);
}

TEST(touch_input_throwable_latch_holds_after_priming) {
    Touch t(kWidth, kHeight, true);
    setAimPull(t, 0.0f, -60.0f); // prime past the threshold

    TouchInput ti;
    const InputMsg primed = ti.build(t, kWidth, true, false, 0.016f);
    CHECK(primed.shootHold);

    // Drag back inside the aim circle: the throwable latch keeps it held.
    setAimPull(t, 0.0f, -10.0f);
    const InputMsg held = ti.build(t, kWidth, true, false, 0.016f);
    CHECK(held.shootHold);
    CHECK(held.shootStart);
}

TEST(touch_input_no_latch_releases_when_pulled_back) {
    Touch t(kWidth, kHeight, true);
    setAimPull(t, 0.0f, -60.0f);

    TouchInput ti;
    CHECK(ti.build(t, kWidth, false, false, 0.016f).shootHold);

    setAimPull(t, 0.0f, -10.0f);
    const InputMsg released = ti.build(t, kWidth, false, false, 0.016f);
    CHECK(!released.shootHold);
    CHECK(!released.shootStart);
}

// ---------------------------------------------------------------------------
// touch.ts pad.img joystick rings (M5 parity): the geometry Touch feeds the
// GameScene::TouchPadGfx sprites.
// ---------------------------------------------------------------------------
namespace {
// Records exactly what Touch::m_update() sends to a pad's sprite.
struct PadGfxRecorder : public PadGraphics {
    bool visible = false;
    Vec2 center, touch;
    float centerScale = 0.0f, touchScale = 0.0f;
    int updates = 0;

    void setVisible(bool v) override { visible = v; }
    void setCenterPos(float x, float y) override { center = Vec2(x, y); }
    void setTouchPos(float x, float y) override { touch = Vec2(x, y); }
    void setCenterScale(float s) override { centerScale = s; }
    void setTouchScale(float s) override { touchScale = s; }
    void update() override { updates++; }
};
} // namespace

// Web `createPadSprite`: centre scale = padScaleBase * padScaleDown (0.6),
// touch scale = padScaleBase * padScalePos (0.25); the ring sits at the locked
// pad center and the dot rests on it while the pad is untouched.
TEST(touch_pad_gfx_locked_scales_and_positions) {
    Touch t(kWidth, kHeight, true);
    PadGfxRecorder move;
    PadGfxRecorder aim;
    t.m_update(move, aim);

    CHECK_EQ(move.updates, 1);
    CHECK_EQ(aim.updates, 1);
    CHECK_EQ(move.visible, true);
    CHECK_EQ(aim.visible, true);
    CHECK_NEAR(move.centerScale, 0.6f, 1e-6f);
    CHECK_NEAR(move.touchScale, 0.25f, 1e-6f);
    // Locked landscape offsets: left (126, 480-100), right (800-126, 380).
    CHECK_NEAR(move.center.x, 126.0f, 1e-4f);
    CHECK_NEAR(move.center.y, 380.0f, 1e-4f);
    CHECK_NEAR(move.touch.x, 126.0f, 1e-4f);
    CHECK_NEAR(move.touch.y, 380.0f, 1e-4f);
    CHECK_NEAR(aim.center.x, 674.0f, 1e-4f);
    CHECK_NEAR(aim.touch.x, 674.0f, 1e-4f);

    // Portrait (padScaleBase 0.8) and hideAll().
    Touch portrait(480.0f, 800.0f, false);
    PadGfxRecorder pmove;
    PadGfxRecorder paim;
    portrait.m_update(pmove, paim);
    CHECK_NEAR(pmove.centerScale, 0.48f, 1e-6f); // 0.8 * 0.6
    CHECK_NEAR(pmove.touchScale, 0.20f, 1e-6f);  // 0.8 * 0.25

    t.hideAll();
    t.m_update(move, aim);
    CHECK_EQ(move.visible, false);
    CHECK_EQ(aim.visible, false);
}

// "Anywhere" style: the ring centre follows the touch-down point, and the dot
// follows the finger (clamped to padPosRange).
TEST(touch_pad_gfx_anywhere_follows_touch) {
    Touch t(kWidth, kHeight, true);
    PadGfxRecorder move;
    PadGfxRecorder aim;
    t.setMoveStyle(TouchStyle::Anywhere);
    setMovePull(t, 20.0f, 0.0f); // posDown (100,380), pos (146,380)
    t.m_update(move, aim);

    CHECK_NEAR(move.center.x, 100.0f, 1e-4f);
    CHECK_NEAR(move.center.y, 380.0f, 1e-4f);
    CHECK_NEAR(move.touch.x, 146.0f, 1e-4f);
    CHECK_NEAR(move.touch.y, 380.0f, 1e-4f);
}
