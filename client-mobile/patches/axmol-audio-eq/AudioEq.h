#pragma once
// Offline biquad EQ for the web client's muffled/club chains.
//
// createJS runs ten RBJ biquad nodes per filter preset; OpenAL EFX only has a
// single low-pass per source. This header applies the same cascade to the
// decoded PCM buffer instead (per channel, 16-bit PCM) and is used by
// AudioCache::getEqBuffer(). The preset data lives in the game's AudioFilter.h.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace ax {

struct EqBand {
    float freq = 1000.0f;
    float q = 1.4142f;
    float gainDb = 0.0f;
    int type = 0; // 0 peaking, 1 low shelf, 2 high shelf
};

struct EqChain {
    int bandCount = 0;
    EqBand bands[10]{};
    float makeup = 1.0f;
    float fallbackGain = 1.0f;
    float fallbackGainHF = 1.0f;
};

namespace audio {

struct Biquad {
    double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
};

// RBJ cookbook coefficients. Shelf filters use S=1 (WebAudio ignores their Q).
inline Biquad makeBiquad(const EqBand& band, double sampleRate) {
    const double pi = 3.14159265358979323846;
    const double A = std::pow(10.0, static_cast<double>(band.gainDb) / 40.0);
    const double f = std::clamp(static_cast<double>(band.freq), 10.0, sampleRate * 0.49);
    const double w0 = 2.0 * pi * f / sampleRate;
    const double cosw = std::cos(w0);
    const double sinw = std::sin(w0);
    double b0, b1, b2, a0, a1, a2;
    if (band.type == 1) {
        const double alpha = sinw * 0.5 * std::sqrt(2.0);
        const double k = 2.0 * std::sqrt(A) * alpha;
        b0 = A * ((A + 1.0) - (A - 1.0) * cosw + k);
        b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cosw);
        b2 = A * ((A + 1.0) - (A - 1.0) * cosw - k);
        a0 = (A + 1.0) + (A - 1.0) * cosw + k;
        a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cosw);
        a2 = (A + 1.0) + (A - 1.0) * cosw - k;
    } else if (band.type == 2) {
        const double alpha = sinw * 0.5 * std::sqrt(2.0);
        const double k = 2.0 * std::sqrt(A) * alpha;
        b0 = A * ((A + 1.0) + (A - 1.0) * cosw + k);
        b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cosw);
        b2 = A * ((A + 1.0) + (A - 1.0) * cosw - k);
        a0 = (A + 1.0) - (A - 1.0) * cosw + k;
        a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cosw);
        a2 = (A + 1.0) - (A - 1.0) * cosw - k;
    } else {
        const double Q = std::max(static_cast<double>(band.q), 0.01);
        const double alpha = sinw / (2.0 * Q);
        b0 = 1.0 + alpha * A;
        b1 = -2.0 * cosw;
        b2 = 1.0 - alpha * A;
        a0 = 1.0 + alpha / A;
        a1 = -2.0 * cosw;
        a2 = 1.0 - alpha / A;
    }
    Biquad q;
    q.b0 = b0 / a0;
    q.b1 = b1 / a0;
    q.b2 = b2 / a0;
    q.a1 = a1 / a0;
    q.a2 = a2 / a0;
    return q;
}

// Applies the cascade to interleaved signed 16-bit PCM in place. The chain's
// makeup gain can push peaks above 1.0; a final peak scale keeps the buffer
// from clipping (the web's compressor does the same job downstream).
inline void applyEq16(char* data, size_t bytes, int channels, int sampleRate, const EqChain& chain) {
    if (data == nullptr || bytes < 2 || channels < 1 || sampleRate <= 0 || chain.bandCount <= 0) {
        return;
    }
    const int bandCount = std::min(chain.bandCount, 10);
    Biquad coeffs[10];
    for (int i = 0; i < bandCount; ++i) {
        coeffs[i] = makeBiquad(chain.bands[i], static_cast<double>(sampleRate));
    }

    const size_t samples = bytes / sizeof(int16_t);
    std::vector<float> out(samples, 0.0f);
    for (int ch = 0; ch < channels; ++ch) {
        double z1[10]{};
        double z2[10]{};
        for (size_t i = static_cast<size_t>(ch); i < samples; i += static_cast<size_t>(channels)) {
            int16_t s16 = 0;
            std::memcpy(&s16, data + i * sizeof(int16_t), sizeof(s16));
            double x = (static_cast<double>(s16) / 32768.0) * chain.makeup;
            for (int b = 0; b < bandCount; ++b) {
                const double y = coeffs[b].b0 * x + z1[b];
                z1[b] = coeffs[b].b1 * x - coeffs[b].a1 * y + z2[b];
                z2[b] = coeffs[b].b2 * x - coeffs[b].a2 * y;
                x = y;
            }
            out[i] = static_cast<float>(x);
        }
    }

    float peak = 0.0f;
    for (float v : out) {
        peak = std::max(peak, std::fabs(v));
    }
    const float scale = peak > 1.0f ? 0.98f / peak : 1.0f;
    for (size_t i = 0; i < samples; ++i) {
        const long v = std::lround(static_cast<double>(out[i]) * scale * 32767.0);
        const int16_t s16 = static_cast<int16_t>(std::clamp(v, -32768L, 32767L));
        std::memcpy(data + i * sizeof(int16_t), &s16, sizeof(s16));
    }
}

// Float32 variant (OpenAL Soft's minimp3 build uses PCM_FLT32).
inline void applyEqF32(char* data, size_t bytes, int channels, int sampleRate, const EqChain& chain) {
    if (data == nullptr || bytes < 4 || channels < 1 || sampleRate <= 0 || chain.bandCount <= 0) {
        return;
    }
    const int bandCount = std::min(chain.bandCount, 10);
    Biquad coeffs[10];
    for (int i = 0; i < bandCount; ++i) {
        coeffs[i] = makeBiquad(chain.bands[i], static_cast<double>(sampleRate));
    }

    float* samples = reinterpret_cast<float*>(data);
    const size_t count = bytes / sizeof(float);
    float peak = 0.0f;
    for (int ch = 0; ch < channels; ++ch) {
        double z1[10]{};
        double z2[10]{};
        for (size_t i = static_cast<size_t>(ch); i < count; i += static_cast<size_t>(channels)) {
            double x = static_cast<double>(samples[i]) * chain.makeup;
            for (int b = 0; b < bandCount; ++b) {
                const double y = coeffs[b].b0 * x + z1[b];
                z1[b] = coeffs[b].b1 * x - coeffs[b].a1 * y + z2[b];
                z2[b] = coeffs[b].b2 * x - coeffs[b].a2 * y;
                x = y;
            }
            samples[i] = static_cast<float>(x);
            peak = std::max(peak, std::fabs(samples[i]));
        }
    }
    const float scale = peak > 1.0f ? 0.98f / peak : 1.0f;
    if (scale < 1.0f) {
        for (size_t i = 0; i < count; ++i) {
            samples[i] *= scale;
        }
    }
}

} // namespace audio
} // namespace ax
