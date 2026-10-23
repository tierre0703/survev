#pragma once
// Engine-independent mapping from the web's convolution reverb signal
// (audioManager.ts: CreateJS.Sound.setReverbs({ cathedral: volume }) with a
// cathedral IR) to OpenAL/EAX reverb parameters used by ax::AudioEngine.
//
// The web mixes every sound through one global convolver; axmol only exposes
// per-source EAX reverb, so the native backend applies these parameters to all
// active sound handles. Kept separate from AxmolAudioBackend.h so the mapping
// is covered by the host tests.
#include "SoundDefs.h"

namespace surv {
namespace audio {

struct ReverbModel {
    bool active = false;
    float density = 1.0f;
    float diffusion = 0.8f;
    float gain = 0.0f;
    float gainHF = 0.5f;
    float gainLF = 1.0f;
    float decayTime = 0.2f;
    float decayHFRatio = 0.3f;
    float decayLFRatio = 1.0f;
    float reflectionsGain = 0.0f;
    float reflectionsDelay = 0.02f;
    float lateReverbGain = 0.0f;
    float lateReverbDelay = 0.03f;
    float echoTime = 0.1f;
    float echoDepth = 0.0f;
    float modulationTime = 0.0f;
    float modulationDepth = 0.0f;
    float airAbsorptionGainHF = 0.9f;
    float hfReference = 5000.0f;
    float lfReference = 250.0f;
};

// `volume` is the 0..1 per-layer reverb amount from AudioManager; `def` is the
// named ReverbDef (cathedral/cave). Below 0.001 the model is inactive.
inline ReverbModel reverbModel(float volume, const ReverbDef* def = nullptr) {
    ReverbModel m;
    if (volume <= 0.001f) {
        return m;
    }
    const float v = volume > 1.0f ? 1.0f : volume;
    const float defVolume = def && def->volume > 0.0f ? def->volume : 1.0f;

    m.active = true;
    // Keep the web's IR loudness in the master gain; the EAX decay gives the
    // cathedral/cave size.
    m.gain = 0.30f * v * defVolume;
    m.decayTime = 0.2f + 1.5f * v;
    m.reflectionsGain = 0.20f * v;
    m.lateReverbGain = 0.50f * v;
    if (def) {
        // Cave IR carries an echo; cathedral leaves echoVolume at 0.
        m.echoTime = def->echoDelay > 0.0f ? def->echoDelay : m.echoTime;
        m.echoDepth = def->echoVolume;
        m.reflectionsDelay = def->stereoSpread > 0.0f ? 0.02f : m.reflectionsDelay;
    }
    return m;
}

} // namespace audio
} // namespace surv
