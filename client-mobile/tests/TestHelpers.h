#pragma once
#include "../src/core/BitBuffer.h"
#include "../src/net/Net.h"
#include "ReferenceData.h"
#include <cstdio>
#include <string>
#include <vector>

namespace surv_test {

// Owns a byte buffer + NetBitStream writing into it.
struct Buffer {
    std::vector<uint8_t> bytes;
    surv::NetBitStream stream;

    Buffer(size_t size = 1024) : bytes(size, 0), stream(bytes.data(), bytes.size()) {}

    std::string hex() const {
        const size_t n = stream.byteIndex();
        std::string out;
        out.reserve(n * 2);
        const char* hx = "0123456789abcdef";
        for (size_t i = 0; i < n; i++) {
            out.push_back(hx[bytes[i] >> 4]);
            out.push_back(hx[bytes[i] & 0x0f]);
        }
        return out;
    }
};

inline bool matchReference(const std::string& name, const std::string& actual) {
    for (const auto& [refName, refHex] : surv_reference::fixtures()) {
        if (refName == name) {
            if (refHex == actual) {
                return true;
            }
            std::printf("  expected: %s\n  actual:   %s\n", refHex.c_str(), actual.c_str());
            return false;
        }
    }
    std::printf("  no reference fixture named '%s'\n", name.c_str());
    return false;
}

} // namespace surv_test