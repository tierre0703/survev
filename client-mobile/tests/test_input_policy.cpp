// P1.6 host tests: default keyboard/mouse binds (inputBinds.ts) and the
// change-gated InputMsg send policy (game.ts:731-771). Engine independent.
#include "TestFramework.h"
#include "../src/game/HardwareInput.h"
#include "../src/game/InputSync.h"

#include <cmath>
#include <string>

using namespace surv;
using namespace surv_test;

namespace {

constexpr float kDegToRad = 3.14159265358979f / 180.0f;

InputMsg baseMessage() {
    InputMsg msg;
    msg.toMouseDir = Vec2(1.0f, 0.0f);
    msg.toMouseLen = 0.0f;
    msg.touchMoveActive = true;
    msg.touchMoveDir = Vec2(1.0f, 0.0f);
    msg.touchMoveLen = 0;
    return msg;
}

} // namespace

TEST(hardware_input_movement_and_edges) {
    HardwareInput hw;
    CHECK(!hw.anyMoveDown());

    hw.setKey(input::key::D, true);
    CHECK(hw.moveRight());
    CHECK(!hw.moveLeft());
    hw.setKey(input::key::Left, true);
    CHECK(hw.moveLeft()); // arrows are unbound by default, like the web fallback
    CHECK(hw.keyDown(input::key::D));

    hw.flush();
    CHECK(!hw.keyPressed(input::key::D));
    CHECK(!hw.keyPressed(input::key::Left));

    // A held key auto-repeats but must only count as one press while held.
    hw.setKey(input::key::R, true);
    hw.setKey(input::key::R, true);
    CHECK(hw.keyPressed(input::key::R));
    InputActions actions;
    hw.collectActions(actions);
    CHECK_EQ(actions.inputs().size(), size_t(1));
    CHECK_EQ(actions.inputs()[0], static_cast<uint8_t>(Input_Reload));

    hw.flush();
    InputActions held;
    hw.collectActions(held);
    CHECK(held.empty());

    // Release + press after a new frame queues the action again.
    hw.setKey(input::key::R, false);
    hw.flush();
    hw.setKey(input::key::R, true);
    InputActions repeat;
    hw.collectActions(repeat);
    CHECK_EQ(repeat.inputs().size(), size_t(1));
}

TEST(hardware_input_actions_wheel_and_items) {
    HardwareInput hw;
    InputActions actions;

    hw.setKey(input::key::One, true);
    hw.setKey(input::key::Two, true);
    hw.setKey(input::key::Four, true);
    hw.setKey(input::key::Three, true);
    hw.setKey(input::key::T, true);
    hw.setKey(input::key::F, true);
    hw.setKey(input::key::E, true);
    hw.collectActions(actions);
    // game.ts checkInputs order: primary, secondary, throwable, melee, ...,
    // stow, then Interact, then Swap.
    const std::vector<uint8_t> expected = {
        Input_EquipPrimary,
        Input_EquipSecondary,
        Input_EquipThrowable,
        Input_EquipMelee,
        Input_StowWeapons,
        Input_Interact,
        Input_SwapWeapSlots,
    };
    CHECK_EQ(actions.inputs().size(), expected.size());
    for (size_t i = 0; i < expected.size() && i < actions.inputs().size(); ++i) {
        CHECK_EQ(actions.inputs()[i], expected[i]);
    }

    // Wheel down = next weapon, wheel up = previous (inputBinds.ts defaults).
    hw.flush();
    InputActions next;
    hw.addWheel(HardwareInput::kWheelDown);
    hw.collectActions(next);
    CHECK_EQ(next.inputs().size(), size_t(1));
    CHECK_EQ(next.inputs()[0], static_cast<uint8_t>(Input_EquipNextWeap));

    hw.flush();
    InputActions prev;
    hw.addWheel(HardwareInput::kWheelUp);
    hw.collectActions(prev);
    CHECK_EQ(prev.inputs().size(), size_t(1));
    CHECK_EQ(prev.inputs()[0], static_cast<uint8_t>(Input_EquipPrevWeap));

    // 7/8/9/0 use the quick-use items, with the web's priority chain.
    hw.flush();
    hw.setKey(input::key::Seven, true);
    InputActions bandage;
    hw.collectActions(bandage);
    CHECK_EQ(bandage.useItem(), std::string("bandage"));

    hw.flush();
    hw.setKey(input::key::Seven, false);
    hw.setKey(input::key::Eight, true);
    InputActions medkit;
    hw.collectActions(medkit);
    CHECK_EQ(medkit.useItem(), std::string("healthkit"));

    hw.flush();
    hw.setKey(input::key::Eight, false);
    hw.setKey(input::key::Nine, true);
    InputActions soda;
    hw.collectActions(soda);
    CHECK_EQ(soda.useItem(), std::string("soda"));

    hw.flush();
    hw.setKey(input::key::Nine, false);
    hw.setKey(input::key::Zero, true);
    InputActions pills;
    hw.collectActions(pills);
    CHECK_EQ(pills.useItem(), std::string("painkiller"));
}

TEST(hardware_input_mouse_state_and_touch_handoff) {
    HardwareInput hw;
    CHECK(!hw.mouseUsed());
    CHECK(!hw.fireDown());

    hw.setMousePos(100.0f, 50.0f);
    CHECK(hw.mouseUsed());
    CHECK_EQ(hw.mousePos().x, 100.0f);
    CHECK_EQ(hw.mousePos().y, 50.0f);

    hw.setMouseButton(0, true);
    CHECK(hw.firePressed());
    CHECK(hw.fireDown());
    hw.flush();
    CHECK(!hw.firePressed());
    CHECK(hw.fireDown());

    hw.setMouseButton(0, false);
    hw.flush();
    CHECK(!hw.fireDown());

    // A touch event takes the aim back from the mouse.
    hw.noteTouchInput();
    CHECK(!hw.mouseUsed());

    // reset() drops everything (pause/background).
    hw.setKey(input::key::W, true);
    hw.setMouseButton(0, true);
    hw.reset();
    CHECK(!hw.keyDown(input::key::W));
    CHECK(!hw.fireDown());
    CHECK(!hw.mouseUsed());
}

TEST(gamepad_axes_map_to_move_aim_and_fire) {
    HardwareInput hw;
    CHECK(!hw.gamepadMoving());
    CHECK(!hw.gamepadAiming());
    CHECK(!hw.gamepadFire());

    // Inside the radial deadzone nothing is reported.
    hw.setGamepadAxes(0.1f, -0.1f, 0.05f, 0.0f);
    CHECK(!hw.gamepadMoving());
    CHECK(!hw.gamepadAiming());

    // Full left stick: direction preserved, magnitude 1 (+Y up).
    hw.setGamepadAxes(1.0f, 0.0f, 0.0f, 0.0f);
    CHECK(hw.gamepadMoving());
    CHECK_NEAR(v2Length(hw.gamepadMove()), 1.0f, 1e-4f);
    CHECK_NEAR(hw.gamepadMove().x, 1.0f, 1e-4f);

    // Half deflection: the remaining travel is rescaled to 0..1
    // (0.6 -> (0.6 - 0.2) / 0.8 = 0.5).
    hw.setGamepadAxes(0.0f, 0.6f, 0.0f, 0.0f);
    CHECK_NEAR(hw.gamepadMove().y, 0.5f, 1e-4f);
    CHECK_NEAR(hw.gamepadMove().x, 0.0f, 1e-4f);

    // Right stick: normalized direction latched, pull kept for toMouseLen.
    hw.setGamepadAxes(0.0f, 0.0f, 0.0f, -1.0f);
    CHECK(hw.gamepadAiming());
    CHECK_NEAR(hw.gamepadAim().y, -1.0f, 1e-4f);
    CHECK_NEAR(hw.gamepadAimPull(), 1.0f, 1e-4f);
    // Releasing the stick keeps the aim direction but stops reporting move.
    hw.setGamepadAxes(0.0f, 0.0f, 0.0f, 0.0f);
    CHECK(hw.gamepadAiming());
    CHECK_NEAR(hw.gamepadAim().y, -1.0f, 1e-4f);
    CHECK(!hw.gamepadMoving());

    // A touch hands the aim back to the pads (hybrid device).
    hw.noteTouchInput();
    CHECK(!hw.gamepadAiming());

    // Trigger threshold + one-frame press edge.
    hw.setGamepadTrigger(0.4f);
    CHECK(!hw.gamepadFire());
    hw.setGamepadTrigger(0.8f);
    CHECK(hw.gamepadFire());
    CHECK(hw.gamepadFirePressed());
    hw.flush();
    CHECK(hw.gamepadFire());
    CHECK(!hw.gamepadFirePressed());
    hw.reset();
    CHECK(!hw.gamepadFire());
    CHECK(!hw.gamepadAiming());
    CHECK(!hw.gamepadMoving());
}

TEST(input_sync_change_gating_and_keepalive) {
    InputSync sync;
    const InputMsg base = baseMessage();

    // First message always goes out (fresh join / reconnect).
    CHECK(sync.shouldSend(base, 0.016f));
    sync.markSent(base);
    CHECK(!sync.shouldSend(base, 0.016f));

    // Movement flags are plain scalar changes.
    {
        InputMsg move = base;
        move.moveLeft = true;
        CHECK(sync.shouldSend(move, 0.016f));
        sync.markSent(move);
        CHECK(!sync.shouldSend(move, 0.016f));
    }

    // Aim direction uses the 0.1 degree threshold.
    sync.reset();
    CHECK(sync.shouldSend(base, 0.0f));
    sync.markSent(base);
    InputMsg aim = base;
    aim.toMouseDir = Vec2(std::cos(0.05f * kDegToRad), std::sin(0.05f * kDegToRad));
    CHECK(!sync.shouldSend(aim, 0.016f));
    aim.toMouseDir = Vec2(std::cos(0.5f * kDegToRad), std::sin(0.5f * kDegToRad));
    CHECK(sync.shouldSend(aim, 0.016f));
    sync.markSent(aim);

    // Aim length uses the 0.5 epsilon.
    InputMsg len = aim;
    len.toMouseLen = 0.4f;
    CHECK(!sync.shouldSend(len, 0.016f));
    len.toMouseLen = 0.6f;
    CHECK(sync.shouldSend(len, 0.016f));
    sync.markSent(len);

    // Portrait switches on resize.
    InputMsg portrait = len;
    portrait.portrait = true;
    CHECK(sync.shouldSend(portrait, 0.016f));
    sync.markSent(portrait);

    // shootStart is always sent while set (a held click must not be lost).
    InputMsg shoot = portrait;
    shoot.shootStart = true;
    CHECK(sync.shouldSend(shoot, 0.016f));
    sync.markSent(shoot);
    CHECK(sync.shouldSend(shoot, 0.016f));
}

TEST(input_sync_actions_and_keepalive_timeout) {
    InputSync sync;
    const InputMsg base = baseMessage();
    CHECK(sync.shouldSend(base, 0.0f));
    sync.markSent(base);

    // A queued action forces a send; the just-cleared list does not (game.ts
    // treats `inputs` as a change only while it is non-empty).
    InputMsg action = base;
    action.addInput(Input_Reload);
    CHECK(sync.shouldSend(action, 0.016f));
    sync.markSent(action);
    CHECK(!sync.shouldSend(base, 0.016f));

    // useItem is a normal scalar change (the clear is sent too, unlike inputs).
    InputMsg use = base;
    use.useItem = "bandage";
    CHECK(sync.shouldSend(use, 0.016f));
    sync.markSent(use);
    CHECK(!sync.shouldSend(use, 0.016f));
    CHECK(sync.shouldSend(base, 0.016f));

    // No change for 1 s -> keepalive send.
    sync.reset();
    CHECK(sync.shouldSend(base, 0.0f));
    sync.markSent(base);
    CHECK(!sync.shouldSend(base, 0.50f));
    CHECK(!sync.shouldSend(base, 0.49f));
    CHECK(sync.shouldSend(base, 0.02f)); // timeout went below zero
}
