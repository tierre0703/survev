#pragma once
// The web client's createJS EQ chains (client/src/lib/createJS.ts eqTypes).
// Each preset is a cascade of ten RBJ biquads; the native backend filters the
// decoded PCM buffer with the same coefficients. `fallbackGain/HF` keep the old
// EFX low-pass approximation for formats/streams that cannot be filtered.
#include <string>

namespace surv {
namespace audio {

enum class EqBandType : int {
    Peaking = 0,
    LowShelf = 1,
    HighShelf = 2,
};

struct EqBand {
    float freq;   // Hz
    float q;      // WebAudio Q (ignored by the shelf formulas, which use S=1)
    float gainDb; // dB
    int type;     // EqBandType
};

struct FilterParams {
    bool enabled = false;
    int bandCount = 0;
    EqBand bands[10]{};
    // createJS starts the chain with a gain node set to 16 (+24 dB) so the
    // deep cuts stay audible.
    float makeup = 1.0f;
    // Used when the PCM path cannot run (streaming/unsupported format).
    float fallbackGain = 1.0f;
    float fallbackGainHF = 1.0f;
};

inline FilterParams filterParamsFor(const std::string& filter) {
    FilterParams p;
    if (filter == "muffled") {
        p.enabled = true;
        p.makeup = 16.0f;
        p.fallbackGain = 0.90f;
        p.fallbackGainHF = 0.05f;
        p.bandCount = 10;
        p.bands[0] = {20.0f, 1.4142f, -6.0f, static_cast<int>(EqBandType::Peaking)};
        p.bands[1] = {40.0f, 1.4142f, -7.0f, static_cast<int>(EqBandType::Peaking)};
        p.bands[2] = {80.0f, 1.4142f, -10.0f, static_cast<int>(EqBandType::Peaking)};
        p.bands[3] = {160.0f, 1.4142f, -13.0f, static_cast<int>(EqBandType::Peaking)};
        p.bands[4] = {320.0f, 1.4142f, -22.0f, static_cast<int>(EqBandType::Peaking)};
        p.bands[5] = {640.0f, 1.4142f, -18.0f, static_cast<int>(EqBandType::Peaking)};
        p.bands[6] = {1280.0f, 1.4142f, -25.0f, static_cast<int>(EqBandType::Peaking)};
        p.bands[7] = {2560.0f, 1.4142f, -10.0f, static_cast<int>(EqBandType::Peaking)};
        p.bands[8] = {5120.0f, 1.4142f, -30.0f, static_cast<int>(EqBandType::Peaking)};
        p.bands[9] = {10240.0f, 1.4142f, -25.0f, static_cast<int>(EqBandType::Peaking)};
        return p;
    }
    if (filter == "club") {
        p.enabled = true;
        p.makeup = 16.0f;
        p.fallbackGain = 0.85f;
        p.fallbackGainHF = 0.02f;
        p.bandCount = 10;
        p.bands[0] = {20.0f, 1.4142f, -6.0f, static_cast<int>(EqBandType::LowShelf)};
        p.bands[1] = {63.0f, 1.4142f, -3.0f, static_cast<int>(EqBandType::LowShelf)};
        p.bands[2] = {125.0f, 1.4142f, -3.0f, static_cast<int>(EqBandType::LowShelf)};
        p.bands[3] = {250.0f, 1.4142f, -6.0f, static_cast<int>(EqBandType::LowShelf)};
        p.bands[4] = {500.0f, 1.4142f, -18.0f, static_cast<int>(EqBandType::Peaking)};
        p.bands[5] = {1000.0f, 1.4142f, -36.0f, static_cast<int>(EqBandType::Peaking)};
        p.bands[6] = {2000.0f, 1.4142f, -48.0f, static_cast<int>(EqBandType::Peaking)};
        p.bands[7] = {4000.0f, 1.4142f, -50.0f, static_cast<int>(EqBandType::HighShelf)};
        p.bands[8] = {8000.0f, 1.4142f, -50.0f, static_cast<int>(EqBandType::HighShelf)};
        p.bands[9] = {16000.0f, 1.4142f, -50.0f, static_cast<int>(EqBandType::HighShelf)};
        return p;
    }
    // "none" and "reverb" (reverb is applied by the EAX path, not EQ).
    return p;
}

} // namespace audio
} // namespace surv
