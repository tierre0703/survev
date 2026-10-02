#pragma once
// Faithful C++ port of shared/lib/bitBuffer.ts (BitView + BitStream).
// Bit-exact with the JS implementation: little-endian bit packing, no
// big-endian support, JS ToInt32 semantics for bit assembly.
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include <stdexcept>
#include <algorithm>

namespace surv {

// Minimal error type to mirror JS throws (bounded buffer overruns).
class BitBufferError : public std::runtime_error {
public:
    explicit BitBufferError(const std::string& msg) : std::runtime_error(msg) {}
};

namespace detail {
inline int min(int a, int b) {
    return a < b ? a : b;
}
} // namespace detail

//
// BitView: random-access bit-level reads/writes over a byte buffer.
// The buffer is NOT owned; it must outlive the view (mirrors Uint8Array views).
//
class BitView {
public:
    BitView() = default;

    BitView(uint8_t* data, size_t byteLength) : _view(data), _len(byteLength) {}

    BitView(const uint8_t* data, size_t byteLength)
        : _view(const_cast<uint8_t*>(data)), _len(byteLength) {}

    uint8_t* data() { return _view; }
    const uint8_t* data() const { return _view; }
    size_t byteLength() const { return _len; }

    uint32_t getBits(size_t offset, int bits, bool signed_) const {
        const size_t available = _len * 8 - offset;
        if (bits > static_cast<int>(available)) {
            char msg[128];
            std::snprintf(msg, sizeof(msg),
                          "Cannot get %d bit(s) from offset %zu, %zu available",
                          bits, offset, available);
            throw BitBufferError(msg);
        }

        uint32_t value = 0;
        for (int i = 0; i < bits;) {
            const int remaining = bits - i;
            const int bitOffset = static_cast<int>(offset & 7);
            const uint8_t currentByte = _view[offset >> 3];

            // the max number of bits we can read from the current byte
            const int read = detail::min(remaining, 8 - bitOffset);

            // create a mask with the correct bit width
            const uint32_t mask = (1u << read) - 1u;
            // shift the bits we want to the start of the byte and mask off the rest
            const uint32_t readBits = (currentByte >> bitOffset) & mask;
            value |= readBits << i;

            offset += read;
            i += read;
        }

        if (signed_) {
            // If we're not working with a full 32 bits, check the
            // imaginary MSB for this bit count and convert to a
            // valid 32-bit signed value if set.
            if (bits != 32 && (value & (1u << (bits - 1)))) {
                value |= ~((1u << bits) - 1u);
            }
            return static_cast<int32_t>(value);
        }

        return value;
    }

    void setBits(size_t offset, uint32_t value, int bits) {
        const size_t available = _len * 8 - offset;
        if (bits > static_cast<int>(available)) {
            char msg[128];
            std::snprintf(msg, sizeof(msg),
                          "Cannot set %d bit(s) from offset %zu, %zu available",
                          bits, offset, available);
            throw BitBufferError(msg);
        }

        for (int i = 0; i < bits;) {
            int wrote;

            // Write an entire byte if we can.
            if (bits - i >= 8 && (offset & 7) == 0) {
                _view[offset >> 3] = static_cast<uint8_t>(value & 0xffu);
                wrote = 8;
            } else {
                const int remaining = bits - i;
                const int bitOffset = static_cast<int>(offset & 7);
                const size_t byteOffset = offset >> 3;
                wrote = detail::min(remaining, 8 - bitOffset);
                // create a mask with the correct bit width
                const uint32_t mask = ~(0xffu << wrote);
                // shift the bits we want to the start of the byte and mask off the rest
                const uint32_t writeBits = value & mask;

                // destination mask to zero all the bits we're changing first
                const uint32_t destMask = ~(mask << bitOffset);

                _view[byteOffset] = static_cast<uint8_t>(
                    (_view[byteOffset] & destMask) | (writeBits << bitOffset));
            }

            value = value >> wrote; // logical shift (matches JS for low-bit content)
            offset += wrote;
            i += wrote;
        }
    }

    bool getBoolean(size_t offset) const {
        return getBits(offset, 1, false) != 0;
    }

    int8_t getInt8(size_t offset) const {
        return static_cast<int8_t>(getBits(offset, 8, true));
    }

    uint8_t getUint8(size_t offset) const {
        return static_cast<uint8_t>(getBits(offset, 8, false));
    }

    int16_t getInt16(size_t offset) const {
        return static_cast<int16_t>(getBits(offset, 16, true));
    }

    uint16_t getUint16(size_t offset) const {
        return static_cast<uint16_t>(getBits(offset, 16, false));
    }

    int32_t getInt32(size_t offset) const {
        return static_cast<int32_t>(getBits(offset, 32, true));
    }

    uint32_t getUint32(size_t offset) const {
        return getBits(offset, 32, false);
    }

    // JS: DataView scratch setUint32(0, v) default big-endian; getFloat32 big-endian.
    float getFloat32(size_t offset) const {
        const uint32_t bits = getUint32(offset);
        float f;
        std::memcpy(&f, &bits, sizeof(f));
        return f;
    }

    // JS: two uint32 groups combined as a big-endian float64.
    double getFloat64(size_t offset) const {
        const uint64_t hi = getUint32(offset);
        const uint64_t lo = getUint32(offset + 32);
        const uint64_t bits = (hi << 32) | lo;
        double d;
        std::memcpy(&d, &bits, sizeof(d));
        return d;
    }

    void setBoolean(size_t offset, bool value) {
        setBits(offset, value ? 1 : 0, 1);
    }

    void setInt8(size_t offset, int8_t value) {
        setBits(offset, static_cast<uint32_t>(value), 8);
    }

    void setUint8(size_t offset, uint8_t value) {
        setBits(offset, value, 8);
    }

    void setInt16(size_t offset, int16_t value) {
        setBits(offset, static_cast<uint32_t>(value), 16);
    }

    void setUint16(size_t offset, uint16_t value) {
        setBits(offset, value, 16);
    }

    void setInt32(size_t offset, int32_t value) {
        setBits(offset, static_cast<uint32_t>(value), 32);
    }

    void setUint32(size_t offset, uint32_t value) {
        setBits(offset, value, 32);
    }

    void setFloat32(size_t offset, float value) {
        uint32_t bits;
        std::memcpy(&bits, &value, sizeof(bits));
        setBits(offset, bits, 32);
    }

    void setFloat64(size_t offset, double value) {
        uint64_t bits;
        std::memcpy(&bits, &value, sizeof(bits));
        setBits(offset, static_cast<uint32_t>(bits >> 32), 32);
        setBits(offset + 32, static_cast<uint32_t>(bits & 0xffffffffu), 32);
    }

private:
    uint8_t* _view = nullptr;
    size_t _len = 0;
};

//
// BitStream: sequential read/write wrapper over a BitView.
//
class BitStream {
public:
    BitStream() = default;

    explicit BitStream(uint8_t* data, size_t byteLength)
        : _view(data, byteLength), _length(byteLength * 8) {}

    explicit BitStream(const uint8_t* data, size_t byteLength)
        : _view(data, byteLength), _length(byteLength * 8) {}

    explicit BitStream(const std::string& bytes)
        : BitStream(reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size()) {}

    explicit BitStream(const std::vector<uint8_t>& bytes)
        : BitStream(bytes.data(), bytes.size()) {}

    const BitView& view() const { return _view; }
    BitView& view() { return _view; }

    size_t index() const { return _index - _startIndex; }
    void setIndex(size_t val) { _index = val + _startIndex; }

    size_t length() const { return _length - _startIndex; }
    void setLength(size_t val) { _length = val + _startIndex; }

    size_t bitsLeft() const { return _length - _index; }

    // Ceil the returned value, over-compensating for the amount of
    // bits written to the stream.
    size_t byteIndex() const { return (_index + 7) / 8; }
    void setByteIndex(size_t val) { _index = val * 8; }

    // ---- raw bit ops ----
    uint32_t readBits(int bits, bool signed_ = false) {
        const uint32_t val = _view.getBits(_index, bits, signed_);
        _index += bits;
        return val;
    }

    void writeBits(uint32_t value, int bits) {
        _view.setBits(_index, value, bits);
        _index += bits;
    }

    bool readBoolean() {
        return readBits(1) != 0;
    }

    int8_t readInt8() {
        return static_cast<int8_t>(readBits(8, true));
    }

    uint8_t readUint8() {
        return static_cast<uint8_t>(readBits(8));
    }

    int16_t readInt16() {
        return static_cast<int16_t>(readBits(16, true));
    }

    uint16_t readUint16() {
        return static_cast<uint16_t>(readBits(16));
    }

    int32_t readInt32() {
        return static_cast<int32_t>(readBits(32, true));
    }

    uint32_t readUint32() {
        return readBits(32);
    }

    float readFloat32() {
        const float v = _view.getFloat32(_index);
        _index += 32;
        return v;
    }

    double readFloat64() {
        const double v = _view.getFloat64(_index);
        _index += 64;
        return v;
    }

    void writeBoolean(bool value) {
        writeBits(value ? 1 : 0, 1);
    }

    void writeInt8(int8_t value) {
        writeBits(static_cast<uint32_t>(value), 8);
    }

    void writeUint8(uint8_t value) {
        writeBits(value, 8);
    }

    void writeInt16(int16_t value) {
        writeBits(static_cast<uint32_t>(value), 16);
    }

    void writeUint16(uint16_t value) {
        writeBits(value, 16);
    }

    void writeInt32(int32_t value) {
        writeBits(static_cast<uint32_t>(value), 32);
    }

    void writeUint32(uint32_t value) {
        writeBits(value, 32);
    }

    void writeFloat32(float value) {
        _view.setFloat32(_index, value);
        _index += 32;
    }

    void writeFloat64(double value) {
        _view.setFloat64(_index, value);
        _index += 64;
    }

    // Read `bytes` bytes (or up to the end of the stream if bytes == 0),
    // stopping the output at the first 0x00. Consumes the full fixed length.
    std::string readString(int bytes = 0, bool utf8 = false) {
        (void)utf8; // ASCII and UTF-8 both map byte-for-byte onto std::string
        const bool fixedLength = bytes != 0;
        if (bytes == 0) {
            bytes = static_cast<int>((length() - index()) / 8);
        }
        int i = 0;
        std::string chars;
        bool append = true;
        while (i < bytes) {
            const uint8_t c = readUint8();
            if (c == 0x00) {
                append = false;
                if (!fixedLength) {
                    break;
                }
            }
            if (append) {
                chars.push_back(static_cast<char>(c));
            }
            i++;
        }
        return chars;
    }

    void writeASCIIString(const std::string& string, int bytes = 0) {
        const int length = bytes ? bytes : static_cast<int>(string.size()) + 1; // +1 for NULL
        for (int i = 0; i < length; i++) {
            writeUint8(i < static_cast<int>(string.size()) ? static_cast<uint8_t>(string[i]) : 0x00);
        }
    }

    void writeUTF8String(const std::string& string, int bytes = 0) {
        writeASCIIString(string, bytes);
    }

protected:
    BitView _view;
    size_t _index = 0;
    size_t _startIndex = 0;
    size_t _length = 0;
};

} // namespace surv