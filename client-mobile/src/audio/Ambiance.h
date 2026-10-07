#pragma once
// Port of client/src/ambiance.ts: the weighted ambience track mixer (music,
// wind, river, waves, interior immediate-mode tracks).
#include "AudioManager.h"
#include <string>
#include <vector>

namespace surv {
namespace audio {

// biome.ambience for the current map.
struct AmbienceMap {
    std::string music;
    std::string wind;
    std::string river;
    std::string waves;
};

class Ambiance {
public:
    Ambiance();

    void setMap(const AmbienceMap& map, AudioManager& audio);
    void onGameStart();
    void onGameComplete(AudioManager& audio);
    void update(float dt, AudioManager& audio, bool inGame);

    // Track accessors (used by the game to trigger interior tracks).
    void setTrackSound(const std::string& name, const std::string& sound);
    void setTrackWeight(const std::string& name, float weight);

    bool introMusic = true;

private:
    struct Track {
        std::string name;
        std::string sound;
        std::string channel;
        bool immediateMode = false;
        SoundHandle inst = kInvalidSound;
        std::string instSound;
        std::string filter;
        float weight = 0.0f;
        float volume = 0.0f;
    };

    Track* getTrack(const std::string& name);

    std::vector<Track> _tracks;
    float _soundUpdateThrottle = 0.0f;
};

} // namespace audio
} // namespace surv
