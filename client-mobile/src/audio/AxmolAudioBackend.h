#pragma once
// ax::AudioEngine implementation of the audio backend (app build only; not
// compiled by the host tests).
#include "AudioBackend.h"
#include "axmol.h"
#include "audio/AudioEngine.h"

#include <string>
#include <unordered_map>

namespace surv {
namespace audio {

class AxmolAudioBackend : public AudioBackend {
public:
    // Tracks the current reverb volume per name so we can avoid redundant
    // axmol API calls and reduce log noise.
    std::unordered_map<std::string, float> _reverbVolumes;

    // The dedicated global reverb instance (created at init, nullptr until then).
    SoundHandle _reverbHandle = kInvalidSound;

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

    void initReverbInstance() {
        if (_reverbHandle == kInvalidSound) {
            // Create a silent looping reverb instance.  We use the first loaded
            // path as a carrier; axmol's reverb is applied per-AudioPlayer and
            // cannot exist without a playing track.  A near-silent loop at
            // volume 0 lets us toggle reverb properties on it without audible
            // side effects.
            for (const auto& kv : _paths) {
                ax::AudioPlayerSettings settings;
                settings.loop = true;
                settings.volume = 0.0f;
                const int h = ax::AudioEngine::play2d(kv.second, settings);
                if (h != ax::AudioEngine::INVALID_AUDIO_ID) {
                    _reverbHandle = h;
                    break;
                }
            }
        }
    }

    void update(float dt) override {
        // Apply reverb properties on the dedicated reverb instance when the
        // target volume changes (the AudioManager calls setReverbVolume once
        // per frame, so we don't need per-instance tracking here).

        // Ensure the reverb carrier exists.  If we don't have any loaded paths
        // yet, skip — there is nothing to attach reverb to.
        if (_reverbHandle == kInvalidSound) {
            if (!_paths.empty()) {
                initReverbInstance();
            }
            if (_reverbHandle == kInvalidSound) {
                return;
            }
        }

        // Pick the largest non-zero reverb volume (typically "cathedral").
        float targetVolume = 0.0f;
        for (const auto& kv : _reverbVolumes) {
            if (kv.second > targetVolume) {
                targetVolume = kv.second;
            }
        }

        // The ReverbDef for "cathedral" provides the source-path impulse
        // response, but axmol's setReverbProperties uses a parameterised EAX
        // model, not a convolution IR.  We map the web's volume-only signal
        // to a generic small-room/underground cathedral reverb by adjusting
        // the core EAX gain/decay from the target volume.

        // Build a ReverbProperties struct that mirrors the web's "cathedral"
        // feel scaled by the target volume.
        ax::ReverbProperties props{};
        props.flDensity = 1.0f;                 // dense
        props.flDiffusion = 0.8f;               // moderately diffused
        props.flGain = 0.3f * targetVolume;     // master reverb gain
        props.flGainHF = 0.5f;                  // high-frequency damping
        props.flGainLF = 1.0f;                  // low-frequency pass
        props.flDecayTime = 1.5f * targetVolume + 0.2f;  // 0.2–1.7 s
        props.flDecayHFRatio = 0.3f;
        props.flDecayLFRatio = 1.0f;
        props.flReflectionsGain = 0.2f * targetVolume;
        props.flReflectionsDelay = 0.02f;
        props.flReflectionsPan[0] = 0.0f;
        props.flReflectionsPan[1] = 0.0f;
        props.flReflectionsPan[2] = 0.0f;
        props.flLateReverbGain = 0.5f * targetVolume;
        props.flLateReverbDelay = 0.03f;
        props.flLateReverbPan[0] = 0.0f;
        props.flLateReverbPan[1] = 0.0f;
        props.flLateReverbPan[2] = 0.0f;
        props.flEchoTime = 0.1f;
        props.flEchoDepth = 0.0f;
        props.flModulationTime = 0.0f;
        props.flModulationDepth = 0.0f;
        props.flAirAbsorptionGainHF = 0.9f;
        props.flHFReference = 5000.0f;
        props.flLFReference = 250.0f;
        props.flRoomRolloffFactor = 0.0f;
        props.iDecayHFLimit = 1;

        if (targetVolume > 0.001f) {
            ax::AudioEngine::setReverbProperties(_reverbHandle, &props);
        } else {
            // Disable reverb (pass nullptr).
            ax::AudioEngine::setReverbProperties(_reverbHandle, nullptr);
        }
    }

private:
    std::unordered_map<std::string, std::string> _paths;
    std::unordered_map<std::string, bool> _loaded;
    std::unordered_map<SoundHandle, float> _handles;
};

} // namespace audio
} // namespace surv
