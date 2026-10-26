#pragma once
// Keyboard/mouse state for the hardware-input path (P1.6).
//
// Port of the non-touch half of `client/src/game.ts` update() plus the default
// key/mouse binds from `client/src/inputBinds.ts`. The Android client is
// touch-first, but Bluetooth keyboards/mice (and emulator/desktop mouse input)
// use this path; touch always keeps working, and a touch event switches the
// active aim back to the pads.
//
// Engine-independent: `GameScene` translates axmol key/mouse events into
// `setKey()`/`setMouse*()` and the host tests drive the same API directly.
//
// The web keybind editor serializes a bind CRC for sharing; the native client
// does not expose remapping yet, so the defaults below are the whole bind
// model (documented in REMAININGS.md P1.6).
#include "../core/GameConfig.h"
#include "../core/Vec2.h"
#include "InputActions.h"

#include <array>
#include <cstdint>

namespace surv {

// `inputBinds.ts` BindDefs defaults, reduced to the keys this port handles.
enum class HwKey : uint8_t {
    A, // Move Left
    D, // Move Right
    W, // Move Up
    S, // Move Down
    R, // Reload
    X, // Cancel
    F, // Interact
    Q, // Equip Last Weapon
    E, // Stow Weapons
    T, // Switch Gun Slots
    M, // Toggle Map
    V, // Toggle Minimap (web CycleUIMode)
    C, // Team Ping Hold
    G, // Toggle Map fallback (unbound Key.G)
    L, // Full Screen (no-op on Android)
    Num0, // Use Pills
    Num1, // Equip Primary
    Num2, // Equip Secondary
    Num3, // Equip Melee
    Num4, // Equip Throwable
    Num5,
    Num6,
    Num7, // Use Bandage
    Num8, // Use Med Kit
    Num9, // Use Soda
    Left, // Move Left + spectate previous
    Right, // Move Right + spectate next
    Up,
    Down,
    Count,
};

class HardwareInput {
public:
    static constexpr int kMouseButtonCount = 5;
    // `MouseWheel.Up`/`Down` as signed steps (scroll up = previous, down = next).
    static constexpr int kWheelUp = -1;
    static constexpr int kWheelDown = 1;

    // ---- Events (edge state is kept until flush()) ----
    void setKey(HwKey key, bool down) {
        const int idx = static_cast<int>(key);
        if (idx >= 0 && idx < static_cast<int>(HwKey::Count)) {
            _keys[static_cast<size_t>(idx)] = down;
        }
    }

    void setMousePos(float x, float y) {
        _mousePos = Vec2(x, y);
        _mouseUsed = true;
    }

    void setMouseButton(int button, bool down) {
        if (button >= 0 && button < kMouseButtonCount) {
            _mouseButtons[static_cast<size_t>(button)] = down;
        }
        _mouseUsed = true;
    }

    void addWheel(int step) {
        if (step == 0) {
            return;
        }
        _wheel = step < 0 ? kWheelUp : kWheelDown;
        _mouseUsed = true;
    }

    // A touch/joystick event takes the aim back from the mouse (hybrid device).
    void noteTouchInput() { _mouseUsed = false; }

    // ---- Queries ----
    bool keyDown(HwKey key) const {
        const int idx = static_cast<int>(key);
        return idx >= 0 && idx < static_cast<int>(HwKey::Count) &&
            _keys[static_cast<size_t>(idx)];
    }

    bool keyPressed(HwKey key) const {
        const int idx = static_cast<int>(key);
        return idx >= 0 && idx < static_cast<int>(HwKey::Count) &&
            _keys[static_cast<size_t>(idx)] && !_oldKeys[static_cast<size_t>(idx)];
    }

    bool mouseDown(int button) const {
        return button >= 0 && button < kMouseButtonCount &&
            _mouseButtons[static_cast<size_t>(button)];
    }

    bool mousePressed(int button) const {
        return button >= 0 && button < kMouseButtonCount &&
            _mouseButtons[static_cast<size_t>(button)] &&
            !_oldMouseButtons[static_cast<size_t>(button)];
    }

    // -1 while scrolling up, +1 while scrolling down, 0 otherwise.
    int wheel() const { return _wheel; }
    const Vec2& mousePos() const { return _mousePos; }
    bool mouseUsed() const { return _mouseUsed; }
    bool fireDown() const { return mouseDown(0); }
    bool firePressed() const { return mousePressed(0); }

    // WASD or the arrow keys (`IsBindDown(Input.Move*) || unbound arrows`).
    bool moveLeft() const { return keyDown(HwKey::A) || keyDown(HwKey::Left); }
    bool moveRight() const { return keyDown(HwKey::D) || keyDown(HwKey::Right); }
    bool moveUp() const { return keyDown(HwKey::W) || keyDown(HwKey::Up); }
    bool moveDown() const { return keyDown(HwKey::S) || keyDown(HwKey::Down); }
    bool anyMoveDown() const { return moveLeft() || moveRight() || moveUp() || moveDown(); }

    // Port of game.ts checkInputs (order preserved for InputMsg's 7-entry cap),
    // then the Interact merge and the use-item binds.
    void collectActions(InputActions& actions) const {
        if (keyPressed(HwKey::R)) actions.press(Input_Reload);
        if (keyPressed(HwKey::X)) actions.press(Input_Cancel);
        if (keyPressed(HwKey::Num1)) actions.press(Input_EquipPrimary);
        if (keyPressed(HwKey::Num2)) actions.press(Input_EquipSecondary);
        if (keyPressed(HwKey::Num4)) actions.press(Input_EquipThrowable);
        if (keyPressed(HwKey::Num3)) actions.press(Input_EquipMelee);
        if (_wheel == kWheelDown) actions.press(Input_EquipNextWeap);
        if (_wheel == kWheelUp) actions.press(Input_EquipPrevWeap);
        if (keyPressed(HwKey::Q)) actions.press(Input_EquipLastWeap);
        if (keyPressed(HwKey::E)) actions.press(Input_StowWeapons);
        // Revive/Use/Loot are unbound by default, so Interact alone (game.ts
        // only splits them out when one of those binds exists).
        if (keyPressed(HwKey::F)) actions.press(Input_Interact);
        if (keyPressed(HwKey::T)) actions.press(Input_SwapWeapSlots);
        if (keyPressed(HwKey::Num7)) {
            actions.use("bandage");
        } else if (keyPressed(HwKey::Num8)) {
            actions.use("healthkit");
        } else if (keyPressed(HwKey::Num9)) {
            actions.use("soda");
        } else if (keyPressed(HwKey::Num0)) {
            actions.use("painkiller");
        }
    }

    // Call once per rendered frame after updateInput(): moves the current
    // state into the `old` snapshot and clears the one-frame wheel.
    void flush() {
        _oldKeys = _keys;
        _oldMouseButtons = _mouseButtons;
        _wheel = 0;
    }

    // Pause/background: drop every held key so movement cannot stick.
    void reset() {
        _keys.fill(false);
        _oldKeys.fill(false);
        _mouseButtons.fill(false);
        _oldMouseButtons.fill(false);
        _wheel = 0;
        _mouseUsed = false;
    }

private:
    std::array<bool, static_cast<size_t>(HwKey::Count)> _keys{};
    std::array<bool, static_cast<size_t>(HwKey::Count)> _oldKeys{};
    std::array<bool, kMouseButtonCount> _mouseButtons{};
    std::array<bool, kMouseButtonCount> _oldMouseButtons{};
    Vec2 _mousePos;
    int _wheel = 0;
    bool _mouseUsed = false;
};

} // namespace surv
