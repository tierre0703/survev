// Host tests for the key/mouse bind table (client/src/inputBinds.ts): defaults,
// the CRC/base64 code, remapping, and the action resolution HardwareInput does.
#include "TestFramework.h"
#include "../src/game/HardwareInput.h"
#include "../src/game/InputBinds.h"

#include <string>
#include <vector>

using namespace surv;
using namespace surv_test;
using namespace surv::input;

namespace {

// Produced by the real web code (client/src/lib/crc.ts + the BitStream layout of
// inputBinds.ts toArray) for the inputBinds.ts default BindDefs. 46 body bytes
// (version + 36 * 10 bits) + 2 CRC bytes.
constexpr const char* kDefaultBindsBase64 =
    "AQVF1FVTAiQVVkYAAABAMck0Ew0AACxwQFEAAABAN+GUE0xFUdWUlQINxQQAAA/6";
constexpr uint16_t kDefaultBindsCrc = 4090;

} // namespace

TEST(input_binds_defaults_match_the_web_table) {
    InputBinds binds;
    CHECK_EQ(binds.toArray().size(), size_t(48));
    CHECK_EQ(binds.toBase64(), std::string(kDefaultBindsBase64));

    // The bytes before the CRC are the version + 36 * 10 bits.
    std::vector<uint8_t> bytes = binds.toArray();
    CHECK_EQ(bytes.size(), size_t(48));
    CHECK_EQ(crc16(bytes.data(), bytes.size() - 2), kDefaultBindsCrc);
    CHECK_EQ(bytes[0], uint8_t(1));

    // inputBinds.ts BindDefs spot checks (action ids from gameConfig.ts).
    CHECK(binds.bind(Input_MoveLeft) == makeKey(key::A));
    CHECK(binds.bind(Input_MoveRight) == makeKey(key::D));
    CHECK(binds.bind(Input_MoveUp) == makeKey(key::W));
    CHECK(binds.bind(Input_MoveDown) == makeKey(key::S));
    CHECK(binds.bind(Input_Fire) == makeMouseButton(mouse::Left));
    CHECK(binds.bind(Input_Reload) == makeKey(key::R));
    CHECK(binds.bind(Input_Cancel) == makeKey(key::X));
    CHECK(binds.bind(Input_Interact) == makeKey(key::F));
    CHECK(binds.bind(Input_EquipPrimary) == makeKey(key::One));
    CHECK(binds.bind(Input_EquipNextWeap) == makeMouseWheel(wheel::Down));
    CHECK(binds.bind(Input_EquipPrevWeap) == makeMouseWheel(wheel::Up));
    CHECK(binds.bind(Input_UsePainkiller) == makeKey(key::Zero));
    CHECK(binds.bind(Input_ToggleMap) == makeKey(key::M));
    CHECK(binds.bind(Input_CycleUIMode) == makeKey(key::V));
    CHECK(binds.bind(Input_EmoteMenu) == makeMouseButton(mouse::Right));
    CHECK(binds.bind(Input_TeamPingMenu) == makeKey(key::C));
    CHECK(binds.bind(Input_Fullscreen) == makeKey(key::L));
    // Actions with no default stay unbound, including the two not in BindDefs.
    CHECK(!binds.bind(Input_Revive).bound());
    CHECK(!binds.bind(Input_Use).bound());
    CHECK(!binds.bind(Input_Loot).bound());
    CHECK(!binds.bind(Input_EquipFragGrenade).bound());
    CHECK(!binds.bind(Input_EquipSmokeGrenade).bound());
    CHECK(!binds.bind(Input_HideUI).bound());
    CHECK(!binds.bind(Input_TeamPingSingle).bound());
}

TEST(input_binds_code_roundtrip_and_corruption) {
    InputBinds binds;
    CHECK(binds.fromBase64(kDefaultBindsBase64));
    CHECK(binds.toBase64() == std::string(kDefaultBindsBase64));

    // Any changed body byte fails the CRC (fromArray returns false and leaves
    // the current table alone).
    std::string corrupt = kDefaultBindsBase64;
    corrupt[3] = corrupt[3] == 'A' ? 'B' : 'A';
    InputBinds other;
    other.setBind(Input_MoveLeft, makeKey(key::Z));
    CHECK(!other.fromBase64(corrupt));
    CHECK(other.bind(Input_MoveLeft) == makeKey(key::Z));

    // Garbage/empty input is rejected, short buffers too.
    CHECK(!other.fromBase64(""));
    CHECK(!other.fromBase64("not base64!"));
    const uint8_t tiny[2] = {1, 2};
    CHECK(!other.fromArray(tiny, sizeof(tiny)));

    // loadFromConfigString falls back to the defaults for a fresh profile.
    InputBinds fresh;
    fresh.setBind(Input_Reload, makeKey(key::K));
    CHECK(!fresh.loadFromConfigString(""));
    CHECK(fresh.bind(Input_Reload) == makeKey(key::R));
    CHECK(fresh.loadFromConfigString(kDefaultBindsBase64));
    CHECK(fresh.toBase64() == std::string(kDefaultBindsBase64));

    // Web whitespace-tolerant paste (base64Decode skips it).
    InputBinds padded;
    CHECK(padded.fromBase64("  " + std::string(kDefaultBindsBase64) + "\n"));
}

TEST(input_binds_set_bind_is_exclusive_and_labels_match) {
    InputBinds binds;
    // Binding a value already in use clears the previous action (setBind).
    binds.setBind(Input_Reload, makeKey(key::A)); // A was Move Left
    CHECK(!binds.bind(Input_MoveLeft).bound());
    CHECK(binds.bind(Input_Reload) == makeKey(key::A));
    CHECK(binds.isKeyBound(key::A));
    CHECK(!binds.isKeyBound(key::R));

    // Unbinding, and out-of-range actions are ignored.
    binds.setBind(Input_Reload, InputValue{});
    CHECK(!binds.bind(Input_Reload).bound());
    binds.setBind(-1, makeKey(key::Z));
    binds.setBind(Input_Count, makeKey(key::Z));
    CHECK(!binds.isKeyBound(key::Z));

    // input.ts InputValue.toString() labels.
    CHECK_EQ(valueLabel(makeKey(key::A)), std::string("A"));
    CHECK_EQ(valueLabel(makeKey(key::Zero)), std::string("0"));
    CHECK_EQ(valueLabel(makeKey(key::Left)), std::string("Left"));
    CHECK_EQ(valueLabel(makeKey(key::F1)), std::string("F1"));
    CHECK_EQ(valueLabel(makeKey(key::Backspace)), std::string("Backspace"));
    CHECK_EQ(valueLabel(makeMouseButton(mouse::Left)), std::string("Left Mouse"));
    CHECK_EQ(valueLabel(makeMouseButton(mouse::Right)), std::string("Right Mouse"));
    CHECK_EQ(valueLabel(makeMouseWheel(wheel::Up)), std::string("Mouse Wheel Up"));
    CHECK_EQ(valueLabel(makeMouseWheel(wheel::Down)), std::string("Mouse Wheel Down"));
    CHECK_EQ(valueLabel(makeKey(200)), std::string("Key 200"));
    CHECK_EQ(valueLabel(InputValue{}), std::string(""));

    // The Java list carries the def name and the current label.
    binds.loadDefaultBinds();
    const std::string json = bindsJson(binds);
    CHECK(json.find("\"action\":0,\"name\":\"Move Left\",\"label\":\"A\"") != std::string::npos);
    CHECK(json.find("\"action\":17,\"name\":\"Equip Next Weapon\",\"label\":\"Mouse Wheel Down\"") !=
        std::string::npos);
    CHECK(json.find("\"action\":8,\"name\":\"Revive\",\"label\":\"\",\"type\":0") !=
        std::string::npos);

    // Restore defaults clears every remap.
    binds.setBind(Input_MoveLeft, makeKey(key::Z));
    binds.loadDefaultBinds();
    CHECK(binds.bind(Input_MoveLeft) == makeKey(key::A));
    CHECK(binds.toBase64() == std::string(kDefaultBindsBase64));
}

TEST(hardware_input_resolves_actions_through_the_bind_table) {
    HardwareInput hw;
    // Defaults: D moves right, A moves left, arrows are the unbound fallback.
    hw.setKey(key::D, true);
    CHECK(hw.moveRight());
    hw.setKey(key::D, false);
    hw.setKey(key::Left, true);
    CHECK(hw.moveLeft());
    hw.setKey(key::Left, false);

    // Remap Move Left to K: A and the arrow fallback stop moving left.
    InputBinds binds = hw.binds();
    binds.setBind(Input_MoveLeft, makeKey(key::K));
    hw.setBinds(binds);
    hw.setKey(key::A, true);
    CHECK(!hw.moveLeft());
    hw.setKey(key::K, true);
    CHECK(hw.moveLeft());
    hw.setKey(key::K, false);
    hw.setKey(key::A, false);

    // Arrows only move when that arrow key is not itself bound.
    binds.setBind(Input_MoveLeft, makeKey(key::K));
    binds.setBind(Input_Reload, makeKey(key::Left));
    hw.setBinds(binds);
    hw.setKey(key::Left, true);
    CHECK(!hw.moveLeft());
    InputActions reload;
    hw.collectActions(reload);
    CHECK_EQ(reload.inputs().size(), size_t(1));
    CHECK_EQ(reload.inputs()[0], static_cast<uint8_t>(Input_Reload));
    hw.setKey(key::Left, false);

    // Reload remapped to G; R no longer reloads.
    binds = InputBinds{};
    binds.setBind(Input_Reload, makeKey(key::G));
    hw.setBinds(binds);
    hw.setKey(key::R, true);
    InputActions none;
    hw.collectActions(none);
    CHECK(none.empty());
    hw.setKey(key::R, false);
    hw.setKey(key::G, true);
    InputActions g;
    hw.collectActions(g);
    CHECK_EQ(g.inputs().size(), size_t(1));
    CHECK_EQ(g.inputs()[0], static_cast<uint8_t>(Input_Reload));
    hw.setKey(key::G, false);

    // Fire is a bind: Space can fire without any mouse state.
    binds = InputBinds{};
    binds.setBind(Input_Fire, makeKey(key::Space));
    hw.setBinds(binds);
    hw.setKey(key::Space, true);
    CHECK(hw.fireDown());
    CHECK(hw.firePressed());
    hw.flush();
    CHECK(hw.fireDown());
    CHECK(!hw.firePressed());
    hw.setKey(key::Space, false);
    hw.flush();
    CHECK(!hw.fireDown());
}

TEST(hardware_input_interact_split_and_pad_actions) {
    // Interact splits into Revive/Use/Loot only when those binds are unbound
    // (game.ts:590-608). With Revive bound, F queues the other two.
    HardwareInput hw;
    InputBinds binds = hw.binds();
    binds.setBind(Input_Revive, makeKey(key::V)); // clears CycleUIMode
    hw.setBinds(binds);
    hw.setKey(key::F, true);
    InputActions split;
    hw.collectActions(split);
    CHECK_EQ(split.inputs().size(), size_t(2));
    CHECK_EQ(split.inputs()[0], static_cast<uint8_t>(Input_Use));
    CHECK_EQ(split.inputs()[1], static_cast<uint8_t>(Input_Loot));
    hw.setKey(key::F, false);

    // All three bound -> Interact itself is not sent.
    binds.setBind(Input_Use, makeKey(key::U));
    binds.setBind(Input_Loot, makeKey(key::H));
    hw.setBinds(binds);
    hw.setKey(key::F, true);
    InputActions none;
    hw.collectActions(none);
    CHECK(none.empty());
    hw.setKey(key::F, false);

    // Native gamepad buttons OR into the same action queries (fixed mapping,
    // independent of the keyboard/mouse table).
    HardwareInput pad;
    pad.setPadAction(Input_Interact, true);
    CHECK(pad.bindPressed(Input_Interact));
    InputActions interact;
    pad.collectActions(interact);
    CHECK_EQ(interact.inputs().size(), size_t(1));
    CHECK_EQ(interact.inputs()[0], static_cast<uint8_t>(Input_Interact));
    pad.flush();
    CHECK(pad.bindDown(Input_Interact));
    CHECK(!pad.bindPressed(Input_Interact));
    InputActions held;
    pad.collectActions(held);
    CHECK(held.empty());
    pad.reset();
    CHECK(!pad.bindDown(Input_Interact));

    // Wheel binds: next/previous weapon still resolve through the table.
    HardwareInput wheel;
    InputActions next;
    wheel.addWheel(HardwareInput::kWheelDown);
    wheel.collectActions(next);
    CHECK_EQ(next.inputs().size(), size_t(1));
    CHECK_EQ(next.inputs()[0], static_cast<uint8_t>(Input_EquipNextWeap));
}

TEST(hardware_input_subframe_taps_are_not_lost) {
    // A key down+up between two rendered frames still queues its action (the
    // web handles key events immediately; the native poll would otherwise miss
    // a quick tap).
    HardwareInput hw;
    hw.setKey(key::R, true);
    hw.setKey(key::R, false);
    InputActions actions;
    hw.collectActions(actions);
    CHECK_EQ(actions.inputs().size(), size_t(1));
    CHECK_EQ(actions.inputs()[0], static_cast<uint8_t>(Input_Reload));
    hw.flush();
    InputActions again;
    hw.collectActions(again);
    CHECK(again.empty());

    // Same for a gamepad button (AppActivity's keyevent down+up can land in one
    // frame).
    HardwareInput pad;
    pad.setPadAction(Input_Interact, true);
    pad.setPadAction(Input_Interact, false);
    InputActions padActions;
    pad.collectActions(padActions);
    CHECK_EQ(padActions.inputs().size(), size_t(1));
    CHECK_EQ(padActions.inputs()[0], static_cast<uint8_t>(Input_Interact));
    pad.flush();
    InputActions padHeld;
    pad.collectActions(padHeld);
    CHECK(padHeld.empty());
}

TEST(android_key_codes_map_to_input_key_codes) {
    CHECK_EQ(webKeyFromAndroidKeyCode(29), key::A); // KEYCODE_A
    CHECK_EQ(webKeyFromAndroidKeyCode(54), key::Z); // KEYCODE_Z
    CHECK_EQ(webKeyFromAndroidKeyCode(7), key::Zero); // KEYCODE_0
    CHECK_EQ(webKeyFromAndroidKeyCode(16), key::Nine); // KEYCODE_9
    CHECK_EQ(webKeyFromAndroidKeyCode(19), key::Up); // KEYCODE_DPAD_UP
    CHECK_EQ(webKeyFromAndroidKeyCode(21), key::Left); // KEYCODE_DPAD_LEFT
    CHECK_EQ(webKeyFromAndroidKeyCode(62), key::Space); // KEYCODE_SPACE
    CHECK_EQ(webKeyFromAndroidKeyCode(67), key::Backspace); // KEYCODE_DEL
    CHECK_EQ(webKeyFromAndroidKeyCode(131), key::F1); // KEYCODE_F1
    CHECK_EQ(webKeyFromAndroidKeyCode(142), key::F12); // KEYCODE_F12
    // Gamepad buttons and unsupported keys are not keyboard binds.
    CHECK_EQ(webKeyFromAndroidKeyCode(96), -1); // KEYCODE_BUTTON_A
    CHECK_EQ(webKeyFromAndroidKeyCode(0), -1);
}
