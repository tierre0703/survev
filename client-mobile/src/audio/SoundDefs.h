#pragma once
// Port of client/src/soundDefs.ts. The tables (Sounds/Groups/Channels/Reverbs)
// are emitted into src/audio/GeneratedSoundDefs.cpp by
// tools/codegen_sound_defs.mjs; this header is the provider interface so the
// AudioManager/Ambiance stay decoupled and host-testable.
#include <string>
#include <vector>

namespace surv {
namespace audio {

// One entry of a Sounds.<list> group, with its owning channel resolved.
struct SoundDef {
    std::string name;
    std::string list; // players | hits | sfx | ambient | ui | music
    std::string path;
    float volume = 1.0f;
    bool canCoalesce = false;
    int maxInstances = 0;
    bool preload = true;
    int loadPriority = 0;
};

// A channel definition (soundDefs.Channels).
struct ChannelDef {
    std::string name;
    float volume = 1.0f;
    float maxRange = 48.0f;
    std::string list;
    bool isMusic = false;
};

struct SoundGroupDef {
    std::string name;
    std::string channel;
    std::vector<std::string> sounds;
};

struct ReverbDef {
    std::string name;
    std::string path;
    float volume = 1.0f;
    float stereoSpread = 0.0f;
    float echoVolume = 0.0f;
    float echoDelay = 0.0f;
    float echoLowPass = 0.0f;
};

// A sound registered for a concrete channel (activePlayer and otherPlayers
// both register the same "players" sounds). This is what preloadSounds()
// iterates.
struct RegisteredSound {
    std::string channel;
    std::string name;
    std::string path;
    float volume = 1.0f;
    bool canCoalesce = false;
    int maxInstances = 0;
    bool preload = true;
    int loadPriority = 0;
};

class SoundDefProvider {
public:
    virtual ~SoundDefProvider() = default;
    virtual const ChannelDef* channel(const std::string& name) const = 0;
    virtual const SoundDef* sound(const std::string& channel, const std::string& name) const = 0;
    virtual const SoundGroupDef* group(const std::string& name) const = 0;
    virtual const ReverbDef* reverb(const std::string& name) const = 0;
    virtual const std::vector<RegisteredSound>& registeredSounds() const = 0;
};

// Process-wide provider, installed by installGeneratedSoundDefs(). May be null.
const SoundDefProvider* getSoundDefProvider();
void setSoundDefProvider(const SoundDefProvider* provider);

} // namespace audio
} // namespace surv
