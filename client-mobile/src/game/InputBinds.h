#pragma once
// Key/mouse bind table (port of the model half of `client/src/inputBinds.ts`).
//
// The web client stores its binds as a compact bit stream in the `binds`
// config key (base64 of [version byte][per-action 2-bit type + 8-bit code] +
// CRC-16), and `HardwareInput` resolves every gameplay action through that
// table. The native client used to hard-code the `inputBinds.ts` defaults; this
// header adds the same table, serialization and labels so the Java "Key Binds"
// screen can remap keyboard/mouse input and share/import a bind code.
//
// Layout (must stay bit-compatible with `InputBinds.toArray()`):
//   byte 0        version (1)
//   per action    10 bits: 2-bit InputType (LSB-first) + 8-bit code
//   last 2 bytes  CRC-16/IBM of everything before, big-endian
// The bit stream is LSB-first inside each byte (shared/lib/bitBuffer.ts
// `BitView.setBits`), and `binds` has one entry per `shared/gameConfig.ts`
// `Input` value (36), including the actions that have no default.
//
// Engine-independent: host-tested by `tests/test_input_binds.cpp` and used by
// `HardwareInput`, `GameScene` and the Android bridge.
#include "../core/GameConfig.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace surv {
namespace input {

// input.ts `InputType` (serialized as 2 bits).
enum class InputType : uint8_t {
    None = 0,
    Key = 1,
    MouseButton = 2,
    MouseWheel = 3,
};

// input.ts `Key` codes (browser keyCode values; the stream stores 8 bits).
namespace key {
constexpr int Backspace = 8;
constexpr int Tab = 9;
constexpr int Enter = 13;
constexpr int Shift = 16;
constexpr int Control = 17;
constexpr int Alt = 18;
constexpr int Escape = 27;
constexpr int Space = 32;
constexpr int Left = 37;
constexpr int Up = 38;
constexpr int Right = 39;
constexpr int Down = 40;
constexpr int Delete = 46;
constexpr int Zero = 48;
constexpr int One = 49;
constexpr int Two = 50;
constexpr int Three = 51;
constexpr int Four = 52;
constexpr int Five = 53;
constexpr int Six = 54;
constexpr int Seven = 55;
constexpr int Eight = 56;
constexpr int Nine = 57;
constexpr int A = 65;
constexpr int B = 66;
constexpr int C = 67;
constexpr int D = 68;
constexpr int E = 69;
constexpr int F = 70;
constexpr int G = 71;
constexpr int H = 72;
constexpr int I = 73;
constexpr int J = 74;
constexpr int K = 75;
constexpr int L = 76;
constexpr int M = 77;
constexpr int N = 78;
constexpr int O = 79;
constexpr int P = 80;
constexpr int Q = 81;
constexpr int R = 82;
constexpr int S = 83;
constexpr int T = 84;
constexpr int U = 85;
constexpr int V = 86;
constexpr int W = 87;
constexpr int X = 88;
constexpr int Y = 89;
constexpr int Z = 90;
constexpr int F1 = 112;
constexpr int F2 = 113;
constexpr int F3 = 114;
constexpr int F4 = 115;
constexpr int F5 = 116;
constexpr int F6 = 117;
constexpr int F7 = 118;
constexpr int F8 = 119;
constexpr int F9 = 120;
constexpr int F10 = 121;
constexpr int F11 = 122;
constexpr int F12 = 123;
constexpr int Plus = 187; // "=" (input.ts Equal = Plus)
constexpr int Minus = 189;
constexpr int FwdSlash = 191;
constexpr int Tilde = 192; // "`" (input.ts Backquote = Tilde)
} // namespace key

// input.ts `MouseButton` / `MouseWheel`.
namespace mouse {
constexpr int Left = 0;
constexpr int Middle = 1;
constexpr int Right = 2;
constexpr int Thumb1 = 3;
constexpr int Thumb2 = 4;
} // namespace mouse
namespace wheel {
constexpr int None = 0;
constexpr int Up = 1;
constexpr int Down = 2;
} // namespace wheel

// One action's bound input value. `type == None` means unbound.
struct InputValue {
    InputType type = InputType::None;
    uint8_t code = 0;

    bool bound() const { return type != InputType::None; }
    bool operator==(const InputValue& other) const {
        return type == other.type && code == other.code;
    }
    bool operator!=(const InputValue& other) const { return !(*this == other); }
};

inline InputValue makeKey(int code) {
    return InputValue{InputType::Key, static_cast<uint8_t>(code)};
}
inline InputValue makeMouseButton(int code) {
    return InputValue{InputType::MouseButton, static_cast<uint8_t>(code)};
}
inline InputValue makeMouseWheel(int code) {
    return InputValue{InputType::MouseWheel, static_cast<uint8_t>(code)};
}

constexpr int kActionCount = static_cast<int>(Input_Count); // 36 (gameConfig.ts)

// inputBinds.ts `BindDefs`: name + default value, in `Input` id order.
struct BindDef {
    uint8_t action;
    const char* name;
    InputValue def;
};

inline const std::vector<BindDef>& bindDefs() {
    static const std::vector<BindDef> defs = {
        {Input_MoveLeft, "Move Left", makeKey(key::A)},
        {Input_MoveRight, "Move Right", makeKey(key::D)},
        {Input_MoveUp, "Move Up", makeKey(key::W)},
        {Input_MoveDown, "Move Down", makeKey(key::S)},
        {Input_Fire, "Fire", makeMouseButton(mouse::Left)},
        {Input_Reload, "Reload", makeKey(key::R)},
        {Input_Cancel, "Cancel", makeKey(key::X)},
        {Input_Interact, "Interact", makeKey(key::F)},
        {Input_Revive, "Revive", InputValue{}},
        {Input_Use, "Open/Use", InputValue{}},
        {Input_Loot, "Loot", InputValue{}},
        {Input_EquipPrimary, "Equip Primary", makeKey(key::One)},
        {Input_EquipSecondary, "Equip Secondary", makeKey(key::Two)},
        {Input_EquipMelee, "Equip Melee", makeKey(key::Three)},
        {Input_EquipThrowable, "Equip Throwable", makeKey(key::Four)},
        {Input_EquipNextWeap, "Equip Next Weapon", makeMouseWheel(wheel::Down)},
        {Input_EquipPrevWeap, "Equip Previous Weapon", makeMouseWheel(wheel::Up)},
        {Input_EquipLastWeap, "Equip Last Weapon", makeKey(key::Q)},
        {Input_StowWeapons, "Stow Weapons", makeKey(key::E)},
        {Input_EquipPrevScope, "Equip Previous Scope", InputValue{}},
        {Input_EquipNextScope, "Equip Next Scope", InputValue{}},
        {Input_UseBandage, "Use Bandage", makeKey(key::Seven)},
        {Input_UseHealthKit, "Use Med Kit", makeKey(key::Eight)},
        {Input_UseSoda, "Use Soda", makeKey(key::Nine)},
        {Input_UsePainkiller, "Use Pills", makeKey(key::Zero)},
        {Input_SwapWeapSlots, "Switch Gun Slots", makeKey(key::T)},
        {Input_ToggleMap, "Toggle Map", makeKey(key::M)},
        {Input_CycleUIMode, "Toggle Minimap", makeKey(key::V)},
        {Input_EmoteMenu, "Emote Menu", makeMouseButton(mouse::Right)},
        {Input_TeamPingMenu, "Team Ping Hold", makeKey(key::C)},
        {Input_EquipOtherGun, "Equip Other Gun", InputValue{}},
        {Input_Fullscreen, "Full Screen", makeKey(key::L)},
        {Input_HideUI, "Hide UI", InputValue{}},
        {Input_TeamPingSingle, "Team Ping Menu", InputValue{}},
    };
    return defs;
}

// input.ts KeyNames (only the keys this port can deliver) with the web's
// fallback for unknown codes.
inline std::string keyLabel(int code) {
    if (code >= key::A && code <= key::Z) {
        return std::string(1, static_cast<char>('A' + (code - key::A)));
    }
    if (code >= key::Zero && code <= key::Nine) {
        return std::string(1, static_cast<char>('0' + (code - key::Zero)));
    }
    if (code >= key::F1 && code <= key::F12) {
        return "F" + std::to_string(code - key::F1 + 1);
    }
    switch (code) {
        case key::Backspace: return "Backspace";
        case key::Tab: return "Tab";
        case key::Enter: return "Enter";
        case key::Shift: return "Shift";
        case key::Control: return "Control";
        case key::Alt: return "Alt";
        case key::Escape: return "Escape";
        case key::Space: return "Space";
        case key::Left: return "Left";
        case key::Up: return "Up";
        case key::Right: return "Right";
        case key::Down: return "Down";
        case key::Delete: return "Delete";
        case key::Plus: return "=";
        case key::Minus: return "-";
        case key::FwdSlash: return "/";
        case key::Tilde: return "Backquote";
        default: return "Key " + std::to_string(code);
    }
}

// InputValue.toString(): the label the web keybind editor shows.
inline std::string valueLabel(const InputValue& value) {
    switch (value.type) {
        case InputType::Key: return keyLabel(value.code);
        case InputType::MouseButton:
            switch (value.code) {
                case mouse::Left: return "Left Mouse";
                case mouse::Middle: return "Middle Mouse";
                case mouse::Right: return "Right Mouse";
                case mouse::Thumb1: return "Thumb Mouse 1";
                case mouse::Thumb2: return "Thumb Mouse 2";
                default: return "Mouse " + std::to_string(value.code);
            }
        case InputType::MouseWheel:
            if (value.code == wheel::Up) return "Mouse Wheel Up";
            if (value.code == wheel::Down) return "Mouse Wheel Down";
            return "Mouse Wheel " + std::to_string(value.code);
        default: return "";
    }
}

// CRC-16/IBM, identical to client/src/lib/crc.ts `crc16` (reflected 0xA001
// polynomial, init 0, no final xor).
inline uint16_t crc16(const uint8_t* data, size_t size) {
    uint16_t crc = 0;
    for (size_t i = 0; i < size; ++i) {
        crc ^= static_cast<uint16_t>(data[i]);
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc & 1) ? static_cast<uint16_t>((crc >> 1) ^ 0xA001) : static_cast<uint16_t>(crc >> 1);
        }
    }
    return crc;
}

inline std::string base64Encode(const std::vector<uint8_t>& bytes) {
    static const char* kAlphabet =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve((bytes.size() + 2) / 3 * 4);
    size_t i = 0;
    for (; i + 2 < bytes.size(); i += 3) {
        const uint32_t n = (static_cast<uint32_t>(bytes[i]) << 16) |
            (static_cast<uint32_t>(bytes[i + 1]) << 8) | bytes[i + 2];
        out += kAlphabet[(n >> 18) & 63];
        out += kAlphabet[(n >> 12) & 63];
        out += kAlphabet[(n >> 6) & 63];
        out += kAlphabet[n & 63];
    }
    if (i + 1 == bytes.size()) {
        const uint32_t n = static_cast<uint32_t>(bytes[i]) << 16;
        out += kAlphabet[(n >> 18) & 63];
        out += kAlphabet[(n >> 12) & 63];
        out += "==";
    } else if (i + 2 == bytes.size()) {
        const uint32_t n = (static_cast<uint32_t>(bytes[i]) << 16) |
            (static_cast<uint32_t>(bytes[i + 1]) << 8);
        out += kAlphabet[(n >> 18) & 63];
        out += kAlphabet[(n >> 12) & 63];
        out += kAlphabet[(n >> 6) & 63];
        out += '=';
    }
    return out;
}

inline int base64Value(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+' || c == '-') return 62;
    if (c == '/' || c == '_') return 63;
    return -1;
}

inline bool base64Decode(const std::string& text, std::vector<uint8_t>& out) {
    out.clear();
    uint32_t buffer = 0;
    int bits = 0;
    for (char c : text) {
        if (c == '=' || c == '\r' || c == '\n' || c == ' ' || c == '\t') {
            continue;
        }
        const int value = base64Value(c);
        if (value < 0) {
            out.clear();
            return false;
        }
        buffer = (buffer << 6) | static_cast<uint32_t>(value);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<uint8_t>((buffer >> bits) & 0xff));
        }
    }
    return true;
}

// The bind table for all 36 `Input` actions.
class InputBinds {
public:
    InputBinds() { loadDefaultBinds(); }

    void clearAllBinds() { _binds.fill(InputValue{}); }

    void loadDefaultBinds() {
        clearAllBinds();
        for (const BindDef& def : bindDefs()) {
            setBind(def.action, def.def);
        }
    }

    // inputBinds.ts setBind: a value can only be bound to one action.
    void setBind(int action, InputValue value) {
        if (action < 0 || action >= kActionCount) {
            return;
        }
        if (value.bound()) {
            for (auto& bind : _binds) {
                if (bind == value) {
                    bind = InputValue{};
                }
            }
        }
        _binds[static_cast<size_t>(action)] = value;
    }

    const InputValue& bind(int action) const {
        static const InputValue kNone{};
        if (action < 0 || action >= kActionCount) {
            return kNone;
        }
        return _binds[static_cast<size_t>(action)];
    }

    // inputBinds.ts isKeyBound: used by the arrow-key fallback in game.ts.
    bool isKeyBound(int keyCode) const {
        for (const auto& bind : _binds) {
            if (bind.type == InputType::Key && bind.code == keyCode) {
                return true;
            }
        }
        return false;
    }

    const std::array<InputValue, static_cast<size_t>(kActionCount)>& all() const { return _binds; }

    // inputBinds.ts toArray(): version byte + 10 bits per action + CRC-16.
    std::vector<uint8_t> toArray() const {
        const int totalBits = 8 + kActionCount * 10;
        std::vector<uint8_t> bytes(static_cast<size_t>((totalBits + 7) / 8), 0);
        int bitPos = 0;
        const auto writeBits = [&bytes, &bitPos](uint32_t value, int bits) {
            for (int i = 0; i < bits; ++i, ++bitPos) {
                if ((value >> i) & 1u) {
                    bytes[static_cast<size_t>(bitPos >> 3)] |= static_cast<uint8_t>(1u << (bitPos & 7));
                }
            }
        };
        writeBits(1, 8); // version
        for (const auto& bind : _binds) {
            writeBits(static_cast<uint32_t>(bind.type) & 3u, 2);
            writeBits(bind.code, 8);
        }
        const uint16_t checksum = crc16(bytes.data(), bytes.size());
        bytes.push_back(static_cast<uint8_t>((checksum >> 8) & 0xff));
        bytes.push_back(static_cast<uint8_t>(checksum & 0xff));
        return bytes;
    }

    // inputBinds.ts fromArray(): CRC-checked; rejects short/corrupt buffers.
    bool fromArray(const uint8_t* data, size_t size) {
        if (!data || size < 3) {
            return false;
        }
        const uint16_t dataCrc =
            static_cast<uint16_t>((static_cast<uint16_t>(data[size - 2]) << 8) | data[size - 1]);
        const size_t bodySize = size - 2;
        if (crc16(data, bodySize) != dataCrc) {
            return false;
        }
        clearAllBinds();
        // Read the bit stream LSB-first (BitView.getBits).
        size_t bitPos = 0;
        const auto readBits = [data, bodySize, &bitPos](int bits) -> uint32_t {
            uint32_t value = 0;
            for (int i = 0; i < bits; ++i, ++bitPos) {
                const size_t byteIndex = bitPos >> 3;
                if (byteIndex >= bodySize) {
                    return 0;
                }
                if ((data[byteIndex] >> (bitPos & 7)) & 1u) {
                    value |= 1u << i;
                }
            }
            return value;
        };
        const uint32_t version = readBits(8);
        (void)version; // future bind tables upgrade here (inputBinds.ts upgradeBinds)
        const size_t totalBits = bodySize * 8;
        for (int action = 0; bitPos + 10 <= totalBits && action < kActionCount; ++action) {
            const uint32_t type = readBits(2);
            const uint32_t code = readBits(8);
            if (type == 0 || type > 3) {
                continue;
            }
            _binds[static_cast<size_t>(action)] = InputValue{
                static_cast<InputType>(type), static_cast<uint8_t>(code)};
        }
        return true;
    }

    std::string toBase64() const { return base64Encode(toArray()); }

    bool fromBase64(const std::string& text) {
        std::vector<uint8_t> bytes;
        if (!base64Decode(text, bytes)) {
            return false;
        }
        return fromArray(bytes.data(), bytes.size());
    }

    // Config `binds` ("" = never saved).
    bool loadFromConfigString(const std::string& text) {
        if (text.empty() || !fromBase64(text)) {
            loadDefaultBinds();
            return false;
        }
        return true;
    }
    std::string toConfigString() const { return toBase64(); }

private:
    std::array<InputValue, static_cast<size_t>(kActionCount)> _binds{};
};

// The Java Key Binds screen reads this list: action id, name, current label.
inline std::string bindsJson(const InputBinds& binds) {
    std::string out = "[";
    bool first = true;
    for (const BindDef& def : bindDefs()) {
        if (!first) {
            out += ",";
        }
        first = false;
        const InputValue& value = binds.bind(def.action);
        out += "{\"action\":";
        out += std::to_string(static_cast<int>(def.action));
        out += ",\"name\":\"";
        out += def.name;
        out += "\",\"label\":\"";
        out += valueLabel(value);
        out += "\",\"type\":";
        out += std::to_string(static_cast<int>(value.type));
        out += ",\"code\":";
        out += std::to_string(static_cast<int>(value.code));
        out += "}";
    }
    out += "]";
    return out;
}

// Android KeyEvent.KEYCODE_* -> input.ts Key code, for the Java capture path
// and AppActivity's hardware-key bridge. Returns -1 for unsupported keys
// (gamepad buttons have their own fixed native mapping, see HardwareInput.h).
inline int webKeyFromAndroidKeyCode(int androidKeyCode) {
    // KEYCODE_A (29) .. KEYCODE_Z (54)
    if (androidKeyCode >= 29 && androidKeyCode <= 54) {
        return key::A + (androidKeyCode - 29);
    }
    // KEYCODE_0 (7) .. KEYCODE_9 (16)
    if (androidKeyCode >= 7 && androidKeyCode <= 16) {
        return key::Zero + (androidKeyCode - 7);
    }
    // KEYCODE_F1 (131) .. KEYCODE_F12 (142)
    if (androidKeyCode >= 131 && androidKeyCode <= 142) {
        return key::F1 + (androidKeyCode - 131);
    }
    switch (androidKeyCode) {
        case 19: return key::Up; // KEYCODE_DPAD_UP
        case 20: return key::Down; // KEYCODE_DPAD_DOWN
        case 21: return key::Left; // KEYCODE_DPAD_LEFT
        case 22: return key::Right; // KEYCODE_DPAD_RIGHT
        case 61: return key::Tab; // KEYCODE_TAB
        case 62: return key::Space; // KEYCODE_SPACE
        case 66: return key::Enter; // KEYCODE_ENTER
        case 67: return key::Backspace; // KEYCODE_DEL
        case 112: return key::Delete; // KEYCODE_FORWARD_DEL
        default: return -1;
    }
}

} // namespace input
} // namespace surv
