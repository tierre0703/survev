#pragma once
// Port of client/src/audioManager.ts on top of an engine-independent
// AudioBackend. The app wires AxmolAudioBackend (ax::AudioEngine); the host
// tests wire a recording backend.
#include "AudioBackend.h"
#include "SoundDefs.h"
#include "../core/Vec2.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace surv {
namespace audio {

struct SoundEntry {
    std::string path;
    std::string name;
    std::string channel;
};

// Options mirroring the Partial<Options> accepted by audioManager.ts.
struct PlaySoundOptions {
    std::string channel;
    bool startSilent = false;
    bool forceStart = false;
    bool loop = false;
    bool hasSoundPos = false;
    Vec2 soundPos;
    float fallOff = 0.0f;
    std::string filter;
    float delay = 0.0f;
    bool ignoreMinAllowable = false;
    float rangeMult = 1.0f;
    float offset = 0.0f;
    bool ambient = false;
    float detune = 0.0f;
    float volumeScale = 1.0f;
    bool hasLayer = false;
    int layer = 0;
    bool forceFilter = false;
    bool muffled = false;
};

class AudioManager {
public:
    static constexpr float kMinAllowedVolume = 0.003f;
    static constexpr float kDiffLayerMult = 0.5f;

    bool mute = false;
    bool forcedMute = false;
    float masterVolume = 0.5f; // CreateJS.Sound.volume starts at 0.5
    float soundVolume = 1.0f;
    float musicVolume = 1.0f;
    float baseVolume = 0.5f;

    Vec2 cameraPos;
    int activeLayer = 0;
    bool underground = false;

    explicit AudioManager(AudioBackend* backend) : _backend(backend) {}

    void preloadSounds();
    void loadSound(const std::string& name, const std::string& channel, const std::string& path);

    void update(float dt);

    SoundHandle playSound(const std::string& sound, const PlaySoundOptions& options = {});
    SoundHandle playGroup(const std::string& group, const PlaySoundOptions& options = {});
    void updateSound(SoundHandle instance, const std::string& channel, const Vec2& soundPos,
                     const PlaySoundOptions& options = {});

    void setMasterVolume(float volume);
    void setSoundVolume(float volume);
    void setMusicVolume(float volume);
    void setVolume(SoundHandle instance, float volume, const std::string& type);
    float getVolume(SoundHandle instance) const;
    void setMute(bool mute);
    void setForcedMute(bool mute);
    bool muteToggle();
    void stopSound(SoundHandle instance);
    void stopAll();

    bool allLoaded() const;
    bool isSoundLoaded(const std::string& soundName, const std::string& channel) const;
    bool isSoundPlaying(SoundHandle instance) const;
    float getSoundDefVolume(const std::string& sound, const std::string& channel) const;
    float getTypeVolume(const std::string& type) const;

    AudioBackend* backend() { return _backend; }

private:
    void ensureRegistered(const std::string& name, const std::string& channel);
    void applyInstanceVolumeType(const std::string& type, float volume);

    AudioBackend* _backend = nullptr;
    std::unordered_map<std::string, SoundEntry> _sounds; // key = name + channel
    struct Instance {
        SoundHandle handle = kInvalidSound;
        std::string type; // "sound" | "music"
    };
    std::vector<Instance> _instances;
    bool _preloaded = false;
};

} // namespace audio
} // namespace surv
