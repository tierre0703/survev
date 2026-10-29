// M5 host tests: Touch -> InputMsg collection (throwable latch, aim line,
// move/aim scaling). Drives the engine-independent TouchInput builder without
// axmol.
#include "TestFramework.h"
#include "../src/game/TouchInput.h"
#include "../src/ui/Device.h"

using namespace surv;
using namespace surv_test;

namespace {

// Landscape 800x480. The locked centres use the playtest spacing: the empty
// gap from the ring edge to the side/bottom screen edges is doubled from the
// web defaults (padScaleBase 1, padScaleDown 0.6, padSizeScale 1.75).
constexpr float kWidth = 800.0f;
constexpr float kHeight = 480.0f;
constexpr float kPadRadius = 104.0f * 1.0f * 0.6f * 1.75f;
constexpr float kPadOffX = kPadRadius + (160.0f - kPadRadius) * 2.0f;
constexpr float kPadOffY = kPadRadius + (140.0f - kPadRadius) * 2.0f;
constexpr float nkLeftX = kPadOffX;
constexpr float nkRightX = kWidth - kPadOffX;
constexpr float nkPadY = kHeight - kPadOffY;

void setMovePull(Touch& t, float dx, float dy) {
    t.touches[0].active = true;
    t.touches[0].posDown = Vec2(nkLeftX, nkPadY);
    t.touches[0].pos = Vec2(nkLeftX + dx, nkPadY + dy);
}

void setAimPull(Touch& t, float dx, float dy) {
    t.touches[1].active = true;
    t.touches[1].posDown = Vec2(nkRightX, nkPadY);
    t.touches[1].pos = Vec2(nkRightX + dx, nkPadY + dy);
}

} // namespace

TEST(touch_input_move_and_aim) {
    Touch t(kWidth, kHeight, true);
    // Touch defaults to anywhere (web config); exercise the locked pad path.
    t.setMoveStyle(TouchStyle::Locked);
    t.setAimStyle(TouchStyle::Locked);
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
// touch scale = padScalePos (0.25) — both multiplied by the `padSizeScale`
// 1.75x playtesting multiplier (drawn sizes only). The ring sits at the locked
// pad center and the dot rests on it while the pad is untouched.
TEST(touch_pad_gfx_locked_scales_and_positions) {
    Touch t(kWidth, kHeight, true);
    const float sizeScale = t.padSizeScale;
    PadGfxRecorder move;
    PadGfxRecorder aim;
    // The zoom-driven offset scale is process-wide state on Touch (GameWorld
    // publishes it every frame); pin it to the unscoped view for this test.
    t.setPadOffsetScale(1.0f);
    t.m_update(move, aim);

    CHECK_EQ(move.updates, 1);
    CHECK_EQ(aim.updates, 1);
    CHECK_EQ(move.visible, true);
    CHECK_EQ(aim.visible, true);
    CHECK_NEAR(move.centerScale, 0.6f * sizeScale, 1e-6f);
    CHECK_NEAR(move.touchScale, 0.25f * sizeScale, 1e-6f);
    // Locked landscape centres: doubled ring-to-edge gaps.
    CHECK_NEAR(t.leftLockedPadCenter.x, kPadOffX, 1e-4f);
    CHECK_NEAR(t.rightLockedPadCenter.x, kWidth - kPadOffX, 1e-4f);
    CHECK_NEAR(t.leftLockedPadCenter.y, nkPadY, 1e-4f);
    CHECK_NEAR(t.rightLockedPadCenter.y, nkPadY, 1e-4f);
    CHECK_NEAR(move.center.y, nkPadY, 1e-4f);
    CHECK_NEAR(move.touch.x, kPadOffX, 1e-4f);
    CHECK_NEAR(move.touch.y, nkPadY, 1e-4f);
    CHECK_NEAR(aim.center.x, kWidth - kPadOffX, 1e-4f);
    CHECK_NEAR(aim.touch.x, kWidth - kPadOffX, 1e-4f);

    // Portrait (padScaleBase 0.8) and hideAll().
    Touch portrait(480.0f, 800.0f, false);
    PadGfxRecorder pmove;
    PadGfxRecorder paim;
    portrait.m_update(pmove, paim);
    CHECK_NEAR(pmove.centerScale, 0.48f * sizeScale, 1e-6f);
    CHECK_NEAR(pmove.touchScale, 0.20f * sizeScale, 1e-6f);

    t.hideAll();
    t.m_update(move, aim);
    CHECK_EQ(move.visible, false);
    CHECK_EQ(aim.visible, false);
}

// touch.ts resize(): tablets shift the portrait pad offsets inward (x * 1.25)
// — the landscape offsets are unchanged (`x * 1` there). Session 52 shifted
// both a little above/inside: landscape (160, 140), portrait (120, 176).
TEST(touch_pad_tablet_portrait_offsets) {
    ui::Device& device = ui::Device::get();
    device.tablet = true;

    // Portrait ring is padScaleBase 0.8 smaller (208 * 0.8 * 0.6 * 1.75 / 2).
    constexpr float kPortraitRadius = 104.0f * 0.8f * 0.6f * 1.75f;
    constexpr float kTabletOff = kPortraitRadius + (120.0f * 1.25f - kPortraitRadius) * 2.0f;
    constexpr float kPhoneOff = kPortraitRadius + (120.0f - kPortraitRadius) * 2.0f;

    Touch portrait(480.0f, 800.0f, false);
    CHECK_NEAR(portrait.leftLockedPadCenter.x, kTabletOff, 1e-4f);
    CHECK_NEAR(portrait.rightLockedPadCenter.x, 480.0f - kTabletOff, 1e-4f);

    Touch landscape(kWidth, kHeight, true);
    CHECK_NEAR(landscape.leftLockedPadCenter.x, kPadOffX, 1e-4f);
    CHECK_NEAR(landscape.rightLockedPadCenter.x, kWidth - kPadOffX, 1e-4f);

    // Non-tablet portrait keeps the shifted defaults.
    device.tablet = false;
    Touch phone(480.0f, 800.0f, false);
    CHECK_NEAR(phone.leftLockedPadCenter.x, kPhoneOff, 1e-4f);
}

// touch.ts `padOffsetScale`: the locked offsets (and the pad radius) scale with
// the active player's zoom, so a scoped player moves the joysticks inward
// proportionally and shifts their detect radius with them.
TEST(touch_pad_offsets_scale_with_zoom) {
    Touch t(kWidth, kHeight, true);
    CHECK_NEAR(t.leftLockedPadCenter.x, kPadOffX, 1e-4f);
    CHECK_NEAR(t.rightLockedPadCenter.x, kWidth - kPadOffX, 1e-4f);
    CHECK_NEAR(t.padPosRange, 48.0f, 1e-4f);

    t.setPadOffsetScale(1.5f);
    CHECK_NEAR(t.leftLockedPadCenter.x, kPadOffX * 1.5f, 1e-4f);
    CHECK_NEAR(t.leftLockedPadCenter.y, kHeight - kPadOffY * 1.5f, 1e-4f);
    CHECK_NEAR(t.rightLockedPadCenter.x, kWidth - kPadOffX * 1.5f, 1e-4f);
    // The pad radius scales with the zoom too (padPosBase * padScaleBase *
    // padOffsetScale), so the aim threshold moves with the offsets.
    CHECK_NEAR(t.padPosRange, 72.0f, 1e-4f);

    // A zoom <= 0 is ignored (keep the last valid scale).
    t.setPadOffsetScale(0.0f);
    CHECK_NEAR(t.leftLockedPadCenter.x, kPadOffX * 1.5f, 1e-4f);

    // Back to the unscoped view.
    t.setPadOffsetScale(1.0f);
    CHECK_NEAR(t.leftLockedPadCenter.x, kPadOffX, 1e-4f);
    CHECK_NEAR(t.padPosRange, 48.0f, 1e-4f);
}

// "Anywhere" style: the ring centre follows the touch-down point, and the dot
// follows the finger (clamped to padPosRange).
TEST(touch_pad_gfx_anywhere_follows_touch) {
    Touch t(kWidth, kHeight, true);
    PadGfxRecorder move;
    PadGfxRecorder aim;
    // The zoom-driven offset scale shifts the locked centres; the anywhere
    // style must not depend on it, so pin it to the unscoped view.
    t.setPadOffsetScale(1.0f);
    t.setMoveStyle(TouchStyle::Anywhere);
    setMovePull(t, 20.0f, 0.0f); // posDown (160,340), pos (180,340)
    t.getMovement(kWidth);
    t.m_update(move, aim);

    CHECK_NEAR(move.center.x, nkLeftX, 1e-4f);
    CHECK_NEAR(move.center.y, nkPadY, 1e-4f);
    CHECK_NEAR(move.touch.x, nkLeftX + 20.0f, 1e-4f);
    CHECK_NEAR(move.touch.y, nkPadY, 1e-4f);
}
