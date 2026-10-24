#pragma once
// Maps the web client's createJS EQ filter names onto the single per-source
// low-pass that OpenAL EFX provides. The web chains ten biquad peaking/shelf
// nodes (client/src/lib/createJS.ts eqTypes); a low-pass with a matching
// high-frequency gain is the closest cheap approximation.
#include <string>

namespace surv {
namespace audio {

struct FilterParams {
    bool enabled = false;
    float gain = 1.0f;   // AL_LOWPASS_GAIN (whole signal)
    float gainHF = 1.0f; // AL_LOWPASS_GAINHF (high-frequency band)
};

inline FilterParams filterParamsFor(const std::string& filter) {
    if (filter == "muffled") {
        // createJS: -10..-30 dB above ~320 Hz.
        return {true, 0.90f, 0.05f};
    }
    if (filter == "club") {
        // createJS: -36..-50 dB above ~500 Hz.
        return {true, 0.85f, 0.02f};
    }
    // "none" and "reverb" (reverb is applied by the EAX path, not EQ).
    return {};
}

} // namespace audio
} // namespace surv
