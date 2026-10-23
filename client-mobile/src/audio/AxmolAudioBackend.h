#pragma once
// ax::AudioEngine implementation of the audio backend (app build only; not
// compiled by the host tests).
#include "AudioBackend.h"
#include "ReverbModel.h"
#include "SoundDefs.h"
#include "axmol.h"
#include "audio/AudioEngine.h"

#include <cmath>
#include <string>
#include <unordered_map>

namespace surv {
namespace audio {

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
        ax::AudioPlayerSettings settings;
        settings.loop = opts.loop;
        settings.volume = volume;
        settings.time = opts.offset;
        const int handle = ax::AudioEngine::play2d(it->second, settings);
        if (handle == ax::AudioEngine::INVALID_AUDIO_ID) {
            return kInvalidSound;
        }
        if (opts.pan != 0.0f) {
            ax::AudioEngine::setPan(handle, opts.pan);
        }
        _handles[handle] = volume;
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
        if (mute) {
            ax::AudioEngine::pauseAll();
        } else {
            ax::AudioEngine::resumeAll();
        }
    }

    bool isPlaying(SoundHandle handle) const override {
        return handle != kInvalidSound &&
               ax::AudioEngine::getState(handle) == ax::AudioEngine::AudioState::PLAYING;
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
};

} // namespace audio
} // namespace surv
