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
        // Map the web's impulse-response reverb names to axmol's per-instance
        // parameterised reverb.  Since axmol applies reverb per AudioPlayer
        // (not a global bus), we phase this out by setting the effective
        // reverb volume on a dedicated global reverb instance created at init.
        //
        // For now, the C++ port uses the cathedral/cave data without a true
        // convolution engine — just store the target volume so the
        // AudioManager's reverb logic is live even when the backend cannot
        // yet apply it.
        _reverbVolumes[reverbName] = volume;
    }

    void update(float) override {
        // Stub: future axmol integration would iterate _reverbVolumes and call
        // ax::AudioEngine::setReverbProperties(instance, &props) here.
    }

private:
    std::unordered_map<std::string, std::string> _paths;
    std::unordered_map<std::string, bool> _loaded;
    std::unordered_map<SoundHandle, float> _handles;
};

} // namespace audio
} // namespace surv
