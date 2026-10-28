#pragma once
// ax::AudioEngine implementation of the audio backend (app build only; not
// compiled by the host tests).
#include "AudioBackend.h"
#include "AudioFilter.h"
#include "ReverbModel.h"
#include "SoundDefs.h"
#include "axmol.h"
#include "audio/AudioEngine.h"

#include <cmath>
#include <string>
#include <unordered_map>

#if defined(__ANDROID__)
#    include <sys/system_properties.h>
#endif

namespace surv {
namespace audio {

// `adb shell setprop debug.surv_debug 1` (DevConfig.h) is the documented dev
// switch; the release APK is not debuggable, so axmol's UserDefault copy may be
// unavailable. Read the system property directly as well.
inline bool audioTraceEnabled() {
#if defined(__ANDROID__)
    char value[PROP_VALUE_MAX] = {0};
    if (__system_property_get("debug.surv_debug", value) > 0) {
        return std::string(value) == "1";
    }
#endif
    return ax::UserDefault::getInstance()->getStringForKey("surv_debug", "0") == "1";
}

inline ax::EqChain makeEqChain(const FilterParams& params) {
    ax::EqChain chain;
    chain.bandCount = params.bandCount;
    for (int i = 0; i < params.bandCount && i < 10; ++i) {
        chain.bands[i].freq = params.bands[i].freq;
        chain.bands[i].q = params.bands[i].q;
        chain.bands[i].gainDb = params.bands[i].gainDb;
        chain.bands[i].type = params.bands[i].type;
    }
    chain.makeup = params.makeup;
    chain.fallbackGain = params.fallbackGain;
    chain.fallbackGainHF = params.fallbackGainHF;
    return chain;
}

// Static chains are the lifetime contract of AudioPlayerSettings::eq.
inline const ax::EqChain* eqChainFor(const std::string& filter) {
    static const ax::EqChain muffled = makeEqChain(filterParamsFor("muffled"));
    static const ax::EqChain club = makeEqChain(filterParamsFor("club"));
    if (filter == "muffled") {
        return &muffled;
    }
    if (filter == "club") {
        return &club;
    }
    return nullptr;
}

class AxmolAudioBackend : public AudioBackend {
public:
    // Current reverb target per name (AudioManager recomputes it every frame).
    std::unordered_map<std::string, float> _reverbVolumes;
    // Last applied target; avoids re-setting EAX properties on every sound and
    // every frame. axmol has no global reverb bus, so the model is applied to
    // each active AudioPlayer (OpenAL EAX).
    float _appliedReverb = -1.0f;
    std::string _reverbName = "cathedral";

    bool load(const std::string& id, const std::string& path) override {
        _paths[id] = path;
        // Optimistic: AudioEngine::play2d will load on demand; preload warms
        // the cache where the platform supports it.
        ax::AudioEngine::preload(path);
        _loaded[id] = true;
        return true;
    }

    bool isLoaded(const std::string& id) const override {
        auto it = _loaded.find(id);
        return it != _loaded.end() && it->second;
    }

    SoundHandle play(const std::string& id, const PlayOptions& opts) override {
        auto it = _paths.find(id);
        if (it == _paths.end()) {
            return kInvalidSound;
        }
        const float volume = opts.volume < 0.0f ? 0.0f : (opts.volume > 1.0f ? 1.0f : opts.volume);
        // createJS muffled/club chains run as a 10-band biquad cascade on the
        // decoded PCM buffer inside axmol (AudioCache::getEqBuffer); the player
        // falls back to the old EFX low-pass for streams/unsupported formats.
        const FilterParams filterParams = filterParamsFor(opts.filter);
        ax::AudioPlayerSettings settings;
        settings.loop = opts.loop;
        settings.volume = volume;
        settings.time = opts.offset;
        settings.eq = filterParams.enabled ? eqChainFor(opts.filter) : nullptr;
        const int handle = ax::AudioEngine::play2d(it->second, settings);
        if (handle == ax::AudioEngine::INVALID_AUDIO_ID) {
            return kInvalidSound;
        }
        if (opts.pan != 0.0f) {
            ax::AudioEngine::setPan(handle, opts.pan);
        }
        // createJS `detune` is in cents (shot.ts bonus rounds etc.). axmol's
        // pitch multiplier is 2^(cents/1200), clamped to its 0.5..2.0 range.
        if (std::fabs(opts.detune) > 0.001f) {
            const float pitch = std::pow(2.0f, opts.detune / 1200.0f);
            ax::AudioEngine::setPitch(handle, pitch < 0.5f ? 0.5f : (pitch > 2.0f ? 2.0f : pitch));
        }
        if (audioTraceEnabled()) {
            if (filterParams.enabled) {
                AXLOGI("[audio] play {} filter={} eq=biquad bands={} makeup={:.2f} "
                       "fallback=lowpass gain={:.2f} gainHF={:.2f} vol={:.3f} loop={}",
                       id, opts.filter, filterParams.bandCount, filterParams.makeup,
                       filterParams.fallbackGain, filterParams.fallbackGainHF, volume,
                       opts.loop ? 1 : 0);
            } else {
                AXLOGI("[audio] play {} filter={} vol={:.3f} loop={}", id, opts.filter, volume,
                       opts.loop ? 1 : 0);
            }
        }
        _handles[handle] = volume;
        if (_muted) {
            // pauseAll() only covers voices that existed when mute was applied;
            // loops started while muted (Ambiance forceStart) must also stay
            // paused or they would play through the mute.
            ax::AudioEngine::pause(handle);
        }
        if (_appliedReverb > 0.001f) {
            applyReverb(handle);
        }
        return handle;
    }

    void stop(SoundHandle handle) override {
        if (handle != kInvalidSound) {
            ax::AudioEngine::stop(handle);
        }
    }

    void stopAll() override { ax::AudioEngine::stopAll(); }

    void setVolume(SoundHandle handle, float volume) override {
        if (handle == kInvalidSound) {
            return;
        }
        const float v = volume < 0.0f ? 0.0f : (volume > 1.0f ? 1.0f : volume);
        _handles[handle] = v;
        ax::AudioEngine::setVolume(handle, v);
    }

    float getVolume(SoundHandle handle) const override {
        auto it = _handles.find(handle);
        return it == _handles.end() ? 0.0f : it->second;
    }

    void setPan(SoundHandle handle, float pan) override {
        if (handle != kInvalidSound) {
            ax::AudioEngine::setPan(handle, pan);
        }
    }

    void setMute(bool mute) override {
        _muted = mute;
        if (mute) {
            ax::AudioEngine::pauseAll();
        } else {
            ax::AudioEngine::resumeAll();
        }
    }

    bool isPlaying(SoundHandle handle) const override {
        if (handle == kInvalidSound) {
            return false;
        }
        // PAUSED still owns a live AudioPlayer (mute uses pauseAll/resumeAll), so
        // it must not be garbage-collected as finished.
        const ax::AudioEngine::AudioState state = ax::AudioEngine::getState(handle);
        return state == ax::AudioEngine::AudioState::PLAYING ||
               state == ax::AudioEngine::AudioState::PAUSED;
    }

    void setReverbVolume(const std::string& reverbName, float volume) override {
        _reverbVolumes[reverbName] = volume;
    }

    // axmol exposes per-AudioPlayer EAX reverb, not a convolution bus. Apply
    // the mapped cathedral/cave properties to every active handle (the web's
    // CreateJS convolver is global), and to new handles in play().
    void update(float dt) override {
        (void)dt;
        // Drop finished handles so reverb updates never touch dead players.
        for (auto it = _handles.begin(); it != _handles.end();) {
            if (!isPlaying(it->first)) {
                it = _handles.erase(it);
            } else {
                ++it;
            }
        }

        float targetVolume = 0.0f;
        std::string targetName = "cathedral";
        for (const auto& kv : _reverbVolumes) {
            if (kv.second > targetVolume) {
                targetVolume = kv.second;
                targetName = kv.first;
            }
        }
        if (std::fabs(targetVolume - _appliedReverb) < 0.01f && targetName == _reverbName) {
            return;
        }
        _appliedReverb = targetVolume;
        _reverbName = targetName;
        for (const auto& kv : _handles) {
            applyReverb(kv.first);
        }
    }

private:
    void applyReverb(SoundHandle handle) {
        if (handle == kInvalidSound) {
            return;
        }
        const ReverbModel model = reverbModel(
            _appliedReverb,
            getSoundDefProvider() ? getSoundDefProvider()->reverb(_reverbName) : nullptr);
        if (!model.active) {
            ax::AudioEngine::setReverbProperties(handle, nullptr);
            return;
        }
        ax::ReverbProperties props{};
        props.flDensity = model.density;
        props.flDiffusion = model.diffusion;
        props.flGain = model.gain;
        props.flGainHF = model.gainHF;
        props.flGainLF = model.gainLF;
        props.flDecayTime = model.decayTime;
        props.flDecayHFRatio = model.decayHFRatio;
        props.flDecayLFRatio = model.decayLFRatio;
        props.flReflectionsGain = model.reflectionsGain;
        props.flReflectionsDelay = model.reflectionsDelay;
        props.flReflectionsPan[0] = 0.0f;
        props.flReflectionsPan[1] = 0.0f;
        props.flReflectionsPan[2] = 0.0f;
        props.flLateReverbGain = model.lateReverbGain;
        props.flLateReverbDelay = model.lateReverbDelay;
        props.flLateReverbPan[0] = 0.0f;
        props.flLateReverbPan[1] = 0.0f;
        props.flLateReverbPan[2] = 0.0f;
        props.flEchoTime = model.echoTime;
        props.flEchoDepth = model.echoDepth;
        props.flModulationTime = model.modulationTime;
        props.flModulationDepth = model.modulationDepth;
        props.flAirAbsorptionGainHF = model.airAbsorptionGainHF;
        props.flHFReference = model.hfReference;
        props.flLFReference = model.lfReference;
        props.flRoomRolloffFactor = 0.0f;
        props.iDecayHFLimit = 1;
        ax::AudioEngine::setReverbProperties(handle, &props);
    }

    std::unordered_map<std::string, std::string> _paths;
    std::unordered_map<std::string, bool> _loaded;
    std::unordered_map<SoundHandle, float> _handles;
    bool _muted = false;
};

} // namespace audio
} // namespace surv
