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
