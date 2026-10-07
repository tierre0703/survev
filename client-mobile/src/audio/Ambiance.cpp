#include "Ambiance.h"

#include "../core/MathUtil.h"

#include <cmath>

namespace surv {
namespace audio {

Ambiance::Ambiance() {
    auto addTrack = [this](const std::string& name, const std::string& sound,
                           const std::string& channel, bool immediateMode) {
        Track t;
        t.name = name;
        t.sound = sound;
        t.channel = channel;
        t.immediateMode = immediateMode;
        _tracks.push_back(t);
    };
    // Added in order of weight from least to greatest.
    addTrack("music", "menu_music_01", "music", false);
    addTrack("wind", "ambient_wind_01", "ambient", false);
    addTrack("river", "ambient_stream_01", "ambient", false);
    addTrack("waves", "ambient_waves_01", "ambient", false);
    addTrack("interior_0", "", "ambient", true);
    addTrack("interior_1", "", "ambient", true);
}

Ambiance::Track* Ambiance::getTrack(const std::string& name) {
    for (auto& t : _tracks) {
        if (t.name == name) {
            return &t;
        }
    }
    return nullptr;
}

void Ambiance::setTrackSound(const std::string& name, const std::string& sound) {
    if (Track* t = getTrack(name)) {
        t->sound = sound;
    }
}

void Ambiance::setTrackWeight(const std::string& name, float weight) {
    if (Track* t = getTrack(name)) {
        t->weight = weight;
    }
}

void Ambiance::setMap(const AmbienceMap& map, AudioManager& audio) {
    const SoundDefProvider* provider = getSoundDefProvider();
    struct Entry {
        const char* track;
        const std::string* sound;
    };
    const Entry entries[] = {
        {"music", &map.music},
        {"wind", &map.wind},
        {"river", &map.river},
        {"waves", &map.waves},
    };
    for (const auto& entry : entries) {
        Track* track = getTrack(entry.track);
        if (!track) {
            continue;
        }
        const std::string& sound = *entry.sound;
        track->sound = sound;
        if (sound.empty()) {
            continue;
        }
        const bool isAmbient = provider && provider->sound("ambient", sound) != nullptr;
        const std::string channel = isAmbient ? "ambient" : "music";
        const SoundDef* def = provider ? provider->sound(channel, sound) : nullptr;
        if (def) {
            audio.loadSound(sound, channel, def->path);
        }
    }
}

void Ambiance::onGameStart() {
    introMusic = false;
    for (auto& track : _tracks) {
        track.weight = 0.0f;
    }
    if (Track* wind = getTrack("wind")) {
        wind->weight = 1.0f;
    }
    _soundUpdateThrottle = 0.0f;
}

void Ambiance::onGameComplete(AudioManager& audio) {
    (void)audio;
    for (auto& track : _tracks) {
        if (track.immediateMode) {
            track.weight = 0.0f;
        }
    }
    if (Track* river = getTrack("river")) {
        river->weight = 0.0f;
    }
}

void Ambiance::update(float dt, AudioManager& audio, bool inGame) {
    bool updateVolume = false;
    _soundUpdateThrottle -= dt;
    if (_soundUpdateThrottle <= 0.0f) {
        _soundUpdateThrottle = 0.2f;
        updateVolume = true;
    }

    float totalVolume = 0.0f;
    for (size_t idx = _tracks.size(); idx-- > 0;) {
        Track& track = _tracks[idx];

        // Start sound if it's loaded.
        if (track.inst == kInvalidSound && !track.sound.empty() &&
            audio.isSoundLoaded(track.sound, track.channel)) {
            PlaySoundOptions opts;
            opts.channel = track.channel;
            opts.startSilent = true;
            opts.loop = track.channel == "ambient";
            opts.forceStart = true;
            opts.filter = track.filter;
            opts.forceFilter = true;
            track.inst = audio.playSound(track.sound, opts);
            track.instSound = track.sound;
        }

        // Update sound volume.
        if (track.inst != kInvalidSound && updateVolume) {
            const float volume = track.weight * (1.0f - totalVolume);
            totalVolume += volume;
            track.volume = volume;
            const float defVolume = audio.getSoundDefVolume(track.sound, track.channel);
            audio.setVolume(track.inst, volume * defVolume, track.channel);
        }

        // Stop sound if it's no longer set and audible, or the track changed.
        if (track.inst != kInvalidSound &&
            ((track.sound.empty() && math::eqAbs(audio.getVolume(track.inst), 0.0f)) ||
             (!track.sound.empty() && track.sound != track.instSound))) {
            audio.stopSound(track.inst);
            track.inst = kInvalidSound;
            track.instSound.clear();
        }

        // Reset immediate-mode sounds.
        if (track.immediateMode) {
            track.sound.clear();
            track.weight = 0.0f;
        }
    }

    if (introMusic) {
        Track* music = getTrack("music");
        if (music && music->inst != kInvalidSound) {
            music->weight = math::min(music->weight + dt, 1.0f);
        }
        Track* wind = getTrack("wind");
        if (music && wind && music->inst != kInvalidSound && !audio.isSoundPlaying(music->inst)) {
            wind->weight = math::min(wind->weight + dt, 1.0f);
        }
    }
}

} // namespace audio
} // namespace surv
