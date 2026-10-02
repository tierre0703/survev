#pragma once
// Port of shared/net/net.ts: Constants, BitSizes, MsgType, the BitStream
// extensions (NetBitStream), MsgStream and the definition registry.
#include "../core/BitBuffer.h"
#include "../core/Collider.h"
#include "../core/GameConfig.h"
#include "../core/MathUtil.h"
#include "../core/Vec2.h"
#include "generatedDefs.inc"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>

namespace surv {

struct Constants {
    static constexpr float MaxPosition = 1024.0f;
    static constexpr int MapNameMaxLen = 24;
    static constexpr int PlayerNameMaxLen = 16;
    static constexpr float MouseMaxDist = 64.0f;
    static constexpr float SmokeMaxRad = 10.0f;
    static constexpr float ActionMaxDuration = 8.5f;
    static constexpr float AirstrikeZoneMaxRad = 256.0f;
    static constexpr float AirstrikeZoneMaxDuration = 60.0f;
    static constexpr float PlayerMinScale = 0.75f;
    static constexpr float PlayerMaxScale = 2.0f;
    static constexpr float MapObjectMinScale = 0.125f;
    static constexpr float MapObjectMaxScale = 2.5f;
    static constexpr int MaxPerks = 8;
    static constexpr int MaxMapIndicators = 16;
};

inline constexpr int bitsNeeded(int n) {
    // ceil(log2(n)); n >= 2
    int bits = 0;
    int v = n - 1;
    while (v > 0) {
        v >>= 1;
        bits++;
    }
    return bits;
}

struct BitSizes {
    static constexpr int Action = bitsNeeded(Action_Count);      // 3
    static constexpr int Anim = bitsNeeded(Anim_Count);          // 4
    static constexpr int Haste = bitsNeeded(HasteType_Count);    // 2
    static constexpr int Perks = bitsNeeded(Constants::MaxPerks);      // 3
    static constexpr int MapIndicators = bitsNeeded(Constants::MaxMapIndicators); // 4
};

// Mirrors MsgType enum. Never reorder; always append new types at the end.
enum MsgType : uint8_t {
    MsgType_None = 0,
    MsgType_Join = 1,
    MsgType_Disconnect = 2, // now unused
    MsgType_Input = 3,
    MsgType_Edit = 4,
    MsgType_Joined = 5,
    MsgType_Update = 6,
    MsgType_Kill = 7,
    MsgType_GameOver = 8,
    MsgType_Pickup = 9,
    MsgType_Map = 10,
    MsgType_Spectate = 11,
    MsgType_DropItem = 12,
    MsgType_Emote = 13,
    MsgType_PlayerStats = 14,
    MsgType_AdStatus = 15,
    MsgType_Loadout = 16,
    MsgType_RoleAnnouncement = 17,
    MsgType_Stats = 18,
    MsgType_UpdatePass = 19,
    MsgType_AliveCounts = 20,
    MsgType_PerkModeRoleSelect = 21,
};

enum PickupMsgType : uint8_t {
    PickupMsgType_Full,
    PickupMsgType_AlreadyOwned,
    PickupMsgType_AlreadyEquipped,
    PickupMsgType_BetterItemEquipped,
    PickupMsgType_Success,
    PickupMsgType_GunCannotFire,
    PickupMsgType_MaxPerks,
};

//
// Definition registry: maps type string <-> id, exactly like
// DefinitionRegister in shared/defs/register.ts (id 0 == "").
//
class DefinitionRegistry {
public:
    explicit DefinitionRegistry(const std::string_view* types, size_t count) {
        _types.reserve(count);
        for (size_t i = 0; i < count; i++) {
            _types.push_back(std::string(types[i]));
        }
        for (size_t i = 0; i < _types.size(); i++) {
            _typeToId[_types[i]] = static_cast<int>(i);
            _idToType.push_back(_types[i]);
        }
    }

    int typeToId(const std::string& type) const {
        auto it = _typeToId.find(type);
        if (it == _typeToId.end()) {
            throw std::runtime_error("Invalid type " + type);
        }
        return it->second;
    }

    std::string idToType(int id) const {
        if (id < 0 || id >= static_cast<int>(_idToType.size())) {
            return "";
        }
        return _idToType[id];
    }

    size_t size() const { return _types.size(); }

private:
    std::vector<std::string> _types;
    std::vector<std::string> _idToType;
    std::unordered_map<std::string, int> _typeToId;
};

inline const DefinitionRegistry& GameObjectDefs() {
    static const DefinitionRegistry reg(defs::kGameTypes.data(), defs::kGameTypeCount);
    return reg;
}

inline const DefinitionRegistry& MapObjectDefs() {
    static const DefinitionRegistry reg(defs::kMapTypes.data(), defs::kMapTypeCount);
    return reg;
}

//
// BitStream with the shared/net/net.ts extensions.
//
class NetBitStream : public BitStream {
public:
    using BitStream::BitStream;

    explicit NetBitStream(BitStream& other)
        : BitStream(other.view().data(), other.view().byteLength()) {
        _index = other.index();
    }

    void writeString(const std::string& str, int len = 0) {
        writeASCIIString(str, len);
    }

    std::string readString(int len = 0) {
        return BitStream::readString(len, false);
    }

    void writeFloat(float f, float min, float max, int bits) {
        const uint32_t range = (1u << bits) - 1u;
        const float x = math::clamp(f, min, max);
        const float t = (x - min) / (max - min);
        // JS: (t * range + 0.5) coerced via ToInt32 (truncation toward zero).
        const double v = static_cast<double>(t) * static_cast<double>(range) + 0.5;
        writeBits(static_cast<uint32_t>(v), bits);
    }

    float readFloat(float min, float max, int bits) {
        const uint32_t range = (1u << bits) - 1u;
        const uint32_t x = readBits(bits);
        const float t = static_cast<float>(x) / static_cast<float>(range);
        return min + t * (max - min);
    }

    void writeVec(const Vec2& vec, float minX, float minY, float maxX, float maxY, int bitCount) {
        writeFloat(vec.x, minX, maxX, bitCount);
        writeFloat(vec.y, minY, maxY, bitCount);
    }

    Vec2 readVec(float minX, float minY, float maxX, float maxY, int bitCount) {
        const float x = readFloat(minX, maxX, bitCount);
        const float y = readFloat(minY, maxY, bitCount);
        return Vec2(x, y);
    }

    void writeMapPos(const Vec2& vec, int bitCount = 16) {
        writeVec(vec, 0.0f, 0.0f, Constants::MaxPosition, Constants::MaxPosition, bitCount);
    }

    Vec2 readMapPos(int bitCount = 16) {
        return readVec(0.0f, 0.0f, Constants::MaxPosition, Constants::MaxPosition, bitCount);
    }

    void writeUnitVec(const Vec2& vec, int bitCount) {
        writeVec(vec, -1.0001f, -1.0001f, 1.0001f, 1.0001f, bitCount);
    }

    Vec2 readUnitVec(int bitCount) {
        return readVec(-1.0001f, -1.0001f, 1.0001f, 1.0001f, bitCount);
    }

    void writeVec32(const Vec2& vec) {
        writeFloat32(vec.x);
        writeFloat32(vec.y);
    }

    Vec2 readVec32() {
        const float x = readFloat32();
        const float y = readFloat32();
        return Vec2(x, y);
    }

    void writeBytes(BitStream& src, size_t offset, size_t length) {
        std::memcpy(view().data() + index() / 8, src.view().data() + offset, length);
        setIndex(index() + length * 8);
    }

    void writeAlignToNextByte() {
        const size_t offset = 8 - (index() % 8);
        if (offset < 8) {
            writeBits(0u, static_cast<int>(offset));
        }
    }

    void readAlignToNextByte() {
        const size_t offset = 8 - (index() % 8);
        if (offset < 8) {
            readBits(static_cast<int>(offset));
        }
    }

    void writeGameType(const std::string& type) {
        writeBits(static_cast<uint32_t>(GameObjectDefs().typeToId(type)), 10);
    }

    std::string readGameType() {
        return GameObjectDefs().idToType(readBits(10));
    }

    void writeMapType(const std::string& type) {
        writeBits(static_cast<uint32_t>(MapObjectDefs().typeToId(type)), 12);
    }

    std::string readMapType() {
        return MapObjectDefs().idToType(readBits(12));
    }

    template <typename T>
    void writeArray(const std::vector<T>& array, int bits, void (*writeFn)(NetBitStream&, const T&)) {
        uint32_t length = static_cast<uint32_t>(array.size());
        const uint32_t maxSize = (1u << bits) - 1u;
        if (length > maxSize) {
            length = maxSize;
        }
        writeBits(length, bits);
        for (uint32_t i = 0; i < length; i++) {
            writeFn(*this, array[i]);
        }
    }

    template <typename T>
    std::vector<T> readArray(int bits, T (*readFn)(NetBitStream&)) {
        const uint32_t length = readBits(bits);
        std::vector<T> array;
        array.reserve(length);
        for (uint32_t i = 0; i < length; i++) {
            array.push_back(readFn(*this));
        }
        return array;
    }

    void writeCollider(const Collider& col) {
        writeUint8(static_cast<uint8_t>(col.type));
        if (col.type == Collider::Type::Circle) {
            writeMapPos(col.pos);
            writeFloat(col.rad, 0.0f, Constants::MaxPosition, 16);
        } else {
            writeMapPos(col.min);
            writeMapPos(col.max);
        }
    }

    Collider readCollider() {
        const uint8_t type = readUint8();
        if (type == Collider::Type::Circle) {
            return Collider::createCircle(readMapPos(), readFloat(0.0f, Constants::MaxPosition, 16));
        }
        return Collider::createAabb(readMapPos(), readMapPos());
    }
};

//
// MsgStream: wraps a byte buffer and tracks a NetBitStream position.
// Mirrors shared/net/net.ts MsgStream (client-side serialization helpers).
//
class MsgStream {
public:
    MsgStream() = default;

    explicit MsgStream(std::vector<uint8_t> buf) : _buf(std::move(buf)) {
        _stream = std::make_unique<NetBitStream>(_buf.data(), _buf.size());
    }

    NetBitStream& getStream() { return *_stream; }

    std::vector<uint8_t> getBuffer() {
        const size_t bytes = _stream->byteIndex();
        std::vector<uint8_t> out(bytes);
        std::memcpy(out.data(), _buf.data(), bytes);
        return out;
    }

    void serializeMsg(MsgType type, const std::function<void(NetBitStream&)>& fn) {
        _stream->writeUint8(static_cast<uint8_t>(type));
        fn(*_stream);
        _stream->writeAlignToNextByte();
    }

    void serializeMsgStream(MsgType type, NetBitStream& stream) {
        _stream->writeUint8(static_cast<uint8_t>(type));
        _stream->writeBytes(stream, 0, stream.byteIndex());
    }

    uint8_t deserializeMsgType() {
        if (_stream->length() - _stream->byteIndex() * 8 >= 1) {
            return _stream->readUint8();
        }
        return MsgType_None;
    }

private:
    std::vector<uint8_t> _buf;
    std::unique_ptr<NetBitStream> _stream;
};

} // namespace surv