#pragma once
// Keyboard/mouse + gamepad state for the hardware-input path (P1.6).
//
// Port of the non-touch half of `client/src/game.ts` update() plus the
// `client/src/inputBinds.ts` bind model. The Android client is touch-first, but
// Bluetooth keyboards/mice (and emulator/desktop mouse input) use this path;
// touch always keeps working, and a touch event switches the active aim back to
// the pads.
//
// Every gameplay action is resolved through `input::InputBinds` (the web's
// `isBindDown`/`isBindPressed`), so the Java Key Binds screen can remap keys
// and mouse buttons/wheel and the change applies on the next frame. Physical
// key state is stored by input.ts `Key` code (0..255).
//
// Android gamepads are a native-only addition (the web has no controller
// support): the left stick moves, the right stick aims, triggers fire and the
// buttons map onto fixed bind actions (`setPadAction`), because a pad button is
// not a keyboard/mouse bind value in the web table.
//
// Engine-independent: `GameScene` translates axmol key/mouse events and the
// Java gamepad motion events into `setKey()`/`setMouse*()`/`setGamepad*()` and
// the host tests drive the same API directly.
#include "InputActions.h"
#include "InputBinds.h"

#include <array>
#include <cmath>
#include <cstdint>

namespace surv {

class HardwareInput {
public:
    static constexpr int kMouseButtonCount = 5;
    // `MouseWheel.Up`/`Down` as signed steps (scroll up = previous, down = next).
    static constexpr int kWheelUp = -1;
    static constexpr int kWheelDown = 1;

    // ---- Binds (inputBinds.ts) ----
    const input::InputBinds& binds() const { return _binds; }
    void setBinds(const input::InputBinds& binds) { _binds = binds; }

    // ---- Events (edge state is kept until flush()) ----
    // `keyCode` is an input.ts Key code (input::key::*). A down event latches a
    // one-frame press so a tap that starts and ends between two frames is not
    // lost (the web handles key events immediately).
    void setKey(int keyCode, bool down) {
        if (keyCode >= 0 && keyCode < static_cast<int>(_keys.size())) {
            const size_t index = static_cast<size_t>(keyCode);
            if (down && !_keys[index]) {
                _keyPressedLatch[index] = true;
            }
            _keys[index] = down;
        }
    }

    void setMousePos(float x, float y) {
        _mousePos = Vec2(x, y);
        _mouseUsed = true;
    }

    void setMouseButton(int button, bool down) {
        if (button >= 0 && button < kMouseButtonCount) {
            const size_t index = static_cast<size_t>(button);
            if (down && !_mouseButtons[index]) {
                _mousePressedLatch[index] = true;
            }
            _mouseButtons[index] = down;
        }
        _mouseUsed = true;
    }

    void addWheel(int step) {
        if (step == 0) {
            return;
        }
        // Store the input.ts MouseWheel code (1 = up, 2 = down) so the bind
        // table's MouseWheel values resolve directly.
        _wheel = step < 0 ? input::wheel::Up : input::wheel::Down;
        _mouseUsed = true;
    }

    // A touch/joystick event takes the aim back from the mouse (hybrid device).
    void noteTouchInput() {
        _mouseUsed = false;
        // ... and from the gamepad (its aim is latched like the cursor).
        _padAimUsed = false;
    }

    // ---- Gamepad (native-only; Android pads deliver axes via MotionEvents) ----
    //
    // The web client has no controller support (`inputBinds.ts` is keyboard/mouse
    // only), so this is a native adaptation. Axes are -1..1 with +Y up (the Java
    // side negates Android's down-positive AXIS_Y/AXIS_RZ):
    //   left stick  -> analog move (the same InputMsg fields as the touch pads)
    //   right stick -> aim direction + pull (like the mouse cursor distance)
    //   RT / R2     -> fire
    // Buttons map onto fixed bind actions through `setPadAction` (A interact,
    // B cancel, X reload, Y swap slots, L1/R1 prev/next weapon, L2 bandage,
    // L3 stow, R3 large map), independent of the keyboard/mouse bind table.
    static constexpr float kPadDeadzone = 0.2f;
    static constexpr float kPadFireThreshold = 0.5f;

    void setGamepadAxes(float moveX, float moveY, float aimX, float aimY) {
        _padMove = padCurve(moveX, moveY);
        const Vec2 aim = padCurve(aimX, aimY);
        const float aimLen = v2Length(aim);
        if (aimLen > 0.0f) {
            _padAim = Vec2(aim.x / aimLen, aim.y / aimLen);
            _padAimPull = math::clamp(aimLen, 0.0f, 1.0f);
            _padAimUsed = true;
        }
    }

    void setGamepadTrigger(float value) { _padFire = value > kPadFireThreshold; }

    // A gamepad button held down; OR-ed into the bind query for `action`.
    void setPadAction(int action, bool down) {
        if (action >= 0 && action < input::kActionCount) {
            const size_t index = static_cast<size_t>(action);
            if (down && !_padActions[index]) {
                _padPressedLatch[index] = true;
            }
            _padActions[index] = down;
        }
    }

    // ---- Queries ----
    bool gamepadMoving() const { return v2LengthSqr(_padMove) > 0.0f; }
    const Vec2& gamepadMove() const { return _padMove; }
    // The aim direction latches (like the mouse cursor) until a touch takes it
    // back; `gamepadAimPull` is the last right-stick deflection (0..1).
    bool gamepadAiming() const { return _padAimUsed; }
    const Vec2& gamepadAim() const { return _padAim; }
    float gamepadAimPull() const { return _padAimPull; }
    bool gamepadFire() const { return _padFire; }
    bool gamepadFirePressed() const { return _padFire && !_oldPadFire; }

    bool keyDown(int keyCode) const {
        return keyCode >= 0 && keyCode < static_cast<int>(_keys.size()) &&
            _keys[static_cast<size_t>(keyCode)];
    }

    bool keyPressed(int keyCode) const {
        if (keyCode < 0 || keyCode >= static_cast<int>(_keys.size())) {
            return false;
        }
        const size_t index = static_cast<size_t>(keyCode);
        return _keyPressedLatch[index] || (_keys[index] && !_oldKeys[index]);
    }

    bool mouseDown(int button) const {
        return button >= 0 && button < kMouseButtonCount &&
            _mouseButtons[static_cast<size_t>(button)];
    }

    bool mousePressed(int button) const {
        if (button < 0 || button >= kMouseButtonCount) {
            return false;
        }
        const size_t index = static_cast<size_t>(button);
        return _mousePressedLatch[index] || (_mouseButtons[index] && !_oldMouseButtons[index]);
    }

    // input.ts MouseWheel code for this frame (1 = up, 2 = down), 0 otherwise.
    int wheel() const { return _wheel; }
    const Vec2& mousePos() const { return _mousePos; }
    bool mouseUsed() const { return _mouseUsed; }

    // inputBinds.ts isBindDown / isBindPressed (+ the native pad buttons).
    bool bindDown(int action) const { return bindState(action, false); }
    bool bindPressed(int action) const { return bindState(action, true); }

    // Fire is a normal bind (mouse left by default, remappable).
    bool fireDown() const { return bindDown(Input_Fire); }
    bool firePressed() const { return bindPressed(Input_Fire); }

    // WASD binds, plus the web's unbound-arrow fallback (game.ts:535-547).
    bool moveLeft() const {
        return bindDown(Input_MoveLeft) ||
            (keyDown(input::key::Left) && !_binds.isKeyBound(input::key::Left));
    }
    bool moveRight() const {
        return bindDown(Input_MoveRight) ||
            (keyDown(input::key::Right) && !_binds.isKeyBound(input::key::Right));
    }
    bool moveUp() const {
        return bindDown(Input_MoveUp) ||
            (keyDown(input::key::Up) && !_binds.isKeyBound(input::key::Up));
    }
    bool moveDown() const {
        return bindDown(Input_MoveDown) ||
            (keyDown(input::key::Down) && !_binds.isKeyBound(input::key::Down));
    }
    bool anyMoveDown() const { return moveLeft() || moveRight() || moveUp() || moveDown(); }

    // Port of game.ts checkInputs (order preserved for InputMsg's 7-entry cap),
    // the Interact split and the use-item binds.
    void collectActions(InputActions& actions) const {
        const int checkInputs[] = {
            Input_Reload,
            Input_Revive,
            Input_Use,
            Input_Loot,
            Input_Cancel,
            Input_EquipPrimary,
            Input_EquipSecondary,
            Input_EquipThrowable,
            Input_EquipMelee,
            Input_EquipNextWeap,
            Input_EquipPrevWeap,
            Input_EquipLastWeap,
            Input_EquipOtherGun,
            Input_EquipPrevScope,
            Input_EquipNextScope,
            Input_StowWeapons,
        };
        for (int input : checkInputs) {
            if (bindPressed(input)) {
                actions.press(static_cast<uint8_t>(input));
            }
        }

        // Interact should not activate Revive, Use or Loot when those inputs are
        // bound separately (game.ts:590-608).
        if (bindPressed(Input_Interact)) {
            const int interactBinds[] = {Input_Revive, Input_Use, Input_Loot};
            bool allUnbound = true;
            for (int bind : interactBinds) {
                if (_binds.bind(bind).bound()) {
                    allUnbound = false;
                }
            }
            if (allUnbound) {
                actions.press(Input_Interact);
            } else {
                for (int bind : interactBinds) {
                    if (!_binds.bind(bind).bound()) {
                        actions.press(static_cast<uint8_t>(bind));
                    }
                }
            }
        }

        if (bindPressed(Input_SwapWeapSlots)) {
            actions.press(Input_SwapWeapSlots);
        }

        if (bindPressed(Input_UseBandage)) {
            actions.use("bandage");
        } else if (bindPressed(Input_UseHealthKit)) {
            actions.use("healthkit");
        } else if (bindPressed(Input_UseSoda)) {
            actions.use("soda");
        } else if (bindPressed(Input_UsePainkiller)) {
            actions.use("painkiller");
        }
    }

    // Call once per rendered frame after updateInput(): moves the current
    // state into the `old` snapshot and clears the one-frame wheel.
    void flush() {
        _oldKeys = _keys;
        _oldMouseButtons = _mouseButtons;
        _oldPadActions = _padActions;
        _oldPadFire = _padFire;
        _keyPressedLatch.fill(false);
        _mousePressedLatch.fill(false);
        _padPressedLatch.fill(false);
        _wheel = 0;
    }

    // Pause/background: drop every held key so movement cannot stick.
    void reset() {
        _keys.fill(false);
        _oldKeys.fill(false);
        _keyPressedLatch.fill(false);
        _mouseButtons.fill(false);
        _oldMouseButtons.fill(false);
        _mousePressedLatch.fill(false);
        _padActions.fill(false);
        _oldPadActions.fill(false);
        _padPressedLatch.fill(false);
        _wheel = 0;
        _mouseUsed = false;
        _padMove = Vec2(0.0f, 0.0f);
        _padFire = false;
        _oldPadFire = false;
        _padAimUsed = false;
    }

private:
    // The bind value's physical state (input.ts isInputValueDown/Pressed), OR
    // the native gamepad button mapped onto that action.
    bool bindState(int action, bool pressed) const {
        if (action < 0 || action >= input::kActionCount) {
            return false;
        }
        const size_t index = static_cast<size_t>(action);
        const input::InputValue& value = _binds.bind(action);
        bool down = false;
        if (value.bound()) {
            switch (value.type) {
                case input::InputType::Key:
                    down = pressed ? keyPressed(value.code) : keyDown(value.code);
                    break;
                case input::InputType::MouseButton:
                    down = pressed ? mousePressed(value.code) : mouseDown(value.code);
                    break;
                case input::InputType::MouseWheel:
                    // input.ts: the one-frame wheel delta counts as pressed/down.
                    down = _wheel == static_cast<int>(value.code);
                    break;
                default:
                    break;
            }
        }
        if (_padActions[index]) {
            down = down || (pressed ? !_oldPadActions[index] : true);
        }
        if (pressed && _padPressedLatch[index]) {
            down = true;
        }
        return down;
    }

    // Radial deadzone with the remaining travel rescaled to 0..1.
    static Vec2 padCurve(float x, float y) {
        const float len = std::sqrt(x * x + y * y);
        if (len <= kPadDeadzone) {
            return Vec2(0.0f, 0.0f);
        }
        const float scaled = math::clamp((len - kPadDeadzone) / (1.0f - kPadDeadzone), 0.0f, 1.0f);
        return Vec2(x / len * scaled, y / len * scaled);
    }

    input::InputBinds _binds;
    std::array<bool, 256> _keys{};
    std::array<bool, 256> _oldKeys{};
    std::array<bool, 256> _keyPressedLatch{};
    std::array<bool, kMouseButtonCount> _mouseButtons{};
    std::array<bool, kMouseButtonCount> _oldMouseButtons{};
    std::array<bool, kMouseButtonCount> _mousePressedLatch{};
    std::array<bool, static_cast<size_t>(input::kActionCount)> _padActions{};
    std::array<bool, static_cast<size_t>(input::kActionCount)> _oldPadActions{};
    std::array<bool, static_cast<size_t>(input::kActionCount)> _padPressedLatch{};
    Vec2 _mousePos;
    int _wheel = 0;
    bool _mouseUsed = false;
    Vec2 _padMove;
    Vec2 _padAim{1.0f, 0.0f};
    float _padAimPull = 0.5f;
    bool _padAimUsed = false;
    bool _padFire = false;
    bool _oldPadFire = false;
};

} // namespace surv
