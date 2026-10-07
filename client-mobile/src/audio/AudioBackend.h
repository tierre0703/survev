#pragma once
// Engine-independent audio backend interface. The AudioManager (a port of
// client/src/audioManager.ts) drives this; the app build uses
// AxmolAudioBackend (ax::AudioEngine), the host tests use a recording backend.
#include <string>

namespace surv {
namespace audio {

using SoundHandle = int;
constexpr SoundHandle kInvalidSound = -1;

struct PlayOptions {
    bool loop = false;
    float volume = 1.0f;
    float pan = 0.0f;
    float delay = 0.0f;
    float offset = 0.0f;
    bool ambient = false;
    float detune = 0.0f;
};

class AudioBackend {
public:
    virtual ~AudioBackend() = default;

    // Register a sound id -> file path (preloads the file where supported).
    virtual bool load(const std::string& id, const std::string& path) = 0;
    virtual bool isLoaded(const std::string& id) const = 0;
    virtual SoundHandle play(const std::string& id, const PlayOptions& opts) = 0;
    virtual void stop(SoundHandle handle) = 0;
    virtual void stopAll() = 0;
    virtual void setVolume(SoundHandle handle, float volume) = 0;
    virtual float getVolume(SoundHandle handle) const = 0;
    virtual void setPan(SoundHandle handle, float pan) = 0;
    virtual void setMute(bool mute) = 0;
    virtual bool isPlaying(SoundHandle handle) const = 0;
    virtual void update(float dt) { (void)dt; }
};

} // namespace audio
} // namespace surv
