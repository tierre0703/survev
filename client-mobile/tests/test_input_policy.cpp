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

    hw.setKey(HwKey::D, true);
    CHECK(hw.moveRight());
    CHECK(!hw.moveLeft());
    hw.setKey(HwKey::Left, true);
    CHECK(hw.moveLeft()); // arrows are unbound by default, like the web fallback
    CHECK(hw.keyDown(HwKey::D));

    hw.flush();
    CHECK(!hw.keyPressed(HwKey::D));
    CHECK(!hw.keyPressed(HwKey::Left));

    // A held key auto-repeats but must only count as one press while held.
    hw.setKey(HwKey::R, true);
    hw.setKey(HwKey::R, true);
    CHECK(hw.keyPressed(HwKey::R));
    InputActions actions;
    hw.collectActions(actions);
    CHECK_EQ(actions.inputs().size(), size_t(1));
    CHECK_EQ(actions.inputs()[0], static_cast<uint8_t>(Input_Reload));

    hw.flush();
    InputActions held;
    hw.collectActions(held);
    CHECK(held.empty());

    // Release + press after a new frame queues the action again.
    hw.setKey(HwKey::R, false);
    hw.flush();
    hw.setKey(HwKey::R, true);
    InputActions repeat;
    hw.collectActions(repeat);
    CHECK_EQ(repeat.inputs().size(), size_t(1));
}

TEST(hardware_input_actions_wheel_and_items) {
    HardwareInput hw;
    InputActions actions;

    hw.setKey(HwKey::Num1, true);
    hw.setKey(HwKey::Num2, true);
    hw.setKey(HwKey::Num4, true);
    hw.setKey(HwKey::Num3, true);
    hw.setKey(HwKey::T, true);
    hw.setKey(HwKey::F, true);
    hw.setKey(HwKey::E, true);
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
    hw.setKey(HwKey::Num7, true);
    InputActions bandage;
    hw.collectActions(bandage);
    CHECK_EQ(bandage.useItem(), std::string("bandage"));

    hw.flush();
    hw.setKey(HwKey::Num7, false);
    hw.setKey(HwKey::Num8, true);
    InputActions medkit;
    hw.collectActions(medkit);
    CHECK_EQ(medkit.useItem(), std::string("healthkit"));

    hw.flush();
    hw.setKey(HwKey::Num8, false);
    hw.setKey(HwKey::Num9, true);
    InputActions soda;
    hw.collectActions(soda);
    CHECK_EQ(soda.useItem(), std::string("soda"));

    hw.flush();
    hw.setKey(HwKey::Num9, false);
    hw.setKey(HwKey::Num0, true);
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
    hw.setKey(HwKey::W, true);
    hw.setMouseButton(0, true);
    hw.reset();
    CHECK(!hw.keyDown(HwKey::W));
    CHECK(!hw.fireDown());
    CHECK(!hw.mouseUsed());
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
