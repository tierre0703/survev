#pragma once
// Port of shared/utils/util.ts seededRand (Park-Miller PRNG) and a couple of
// random helpers used by terrain generation / object spawning.
#include "MathUtil.h"
#include <cstdint>

namespace surv {

// util.seededRand(seed): deterministic per-map randomness. The web client
// stores `rng` as a JS number and does `rng = (rng * 16807) % 2147483647`,
// which is exact in float64 for all valid states, so uint64 arithmetic here
// reproduces it bit-for-bit.
class SeededRand {
public:
    explicit SeededRand(uint32_t seed) : _state(seed) {}

    // Returns a value in [min, max]. Mirrors `math.lerp(t, min, max)`.
    float operator()(float min = 0.0f, float max = 1.0f) {
        _state = static_cast<uint32_t>((static_cast<uint64_t>(_state) * 16807ull) % 2147483647ull);
        const float t = static_cast<float>(_state) / 2147483647.0f;
        return math::lerp(t, min, max);
    }

    // Raw [0,1) sample, mirrors calling the closure with no args.
    float next() { return (*this)(0.0f, 1.0f); }

private:
    uint32_t _state;
};

// util.random(min, max) using an explicit rand source.
inline float randRange(SeededRand& rng, float min, float max) {
    return rng(min, max);
}

} // namespace surv
