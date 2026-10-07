// Host tests for the M6 audio port: the codegen'd sound-def provider, the
// AudioManager (volume/attenuation/group/mute), and the Ambiance track mixer.
#include "TestFramework.h"

#include "audio/Ambiance.h"
#include "audio/AudioManager.h"
#include "audio/GeneratedSoundDefs.h"
#include "audio/SoundDefs.h"

#include <string>
#include <unordered_map>
#include <vector>

using namespace surv;
using namespace surv_test;

namespace {

class RecordingBackend : public audio::AudioBackend {
public:
    struct Played {
        std::string id;
        bool loop = false;
        float volume = 0.0f;
        float pan = 0.0f;
    };

    std::vector<Played> played;
    std::unordered_map<std::string, std::string> paths;
    std::unordered_map<std::string, bool> loaded;
    std::unordered_map<audio::SoundHandle, float> volumes;
    bool muted = false;
    int nextHandle = 1;

    bool load(const std::string& id, const std::string& path) override {
        paths[id] = path;
        loaded[id] = true;
        return true;
    }
    bool isLoaded(const std::string& id) const override {
        auto it = loaded.find(id);
        return it != loaded.end() && it->second;
    }
    audio::SoundHandle play(const std::string& id, const audio::PlayOptions& opts) override {
        played.push_back(Played{id, opts.loop, opts.volume, opts.pan});
        const audio::SoundHandle handle = nextHandle++;
        volumes[handle] = opts.volume;
        return handle;
    }
    void stop(audio::SoundHandle) override {}
    void stopAll() override {}
    void setVolume(audio::SoundHandle handle, float volume) override { volumes[handle] = volume; }
    float getVolume(audio::SoundHandle handle) const override {
        auto it = volumes.find(handle);
        return it == volumes.end() ? 0.0f : it->second;
    }
    void setPan(audio::SoundHandle, float) override {}
    void setMute(bool mute) override { muted = mute; }
    bool isPlaying(audio::SoundHandle) const override { return true; }
};

int countPlayed(const RecordingBackend& b, const std::string& id) {
    int n = 0;
    for (const auto& p : b.played) {
        if (p.id == id) {
            n++;
        }
    }
    return n;
}

} // namespace

// ---------------------------------------------------------------------------
TEST(audio_defs_provider) {
    audio::installGeneratedSoundDefs();
    const audio::SoundDefProvider* provider = audio::getSoundDefProvider();
    CHECK(provider != nullptr);
    if (!provider) {
        return;
    }

    const audio::ChannelDef* sfx = provider->channel("sfx");
    CHECK(sfx != nullptr);
    if (sfx) {
        CHECK_NEAR(sfx->volume, 1.0f, 1e-4f);
        CHECK_EQ(sfx->list, std::string("sfx"));
    }

    const audio::SoundDef* explosion = provider->sound("sfx", "explosion_01");
    CHECK(explosion != nullptr);
    if (explosion) {
        CHECK_EQ(explosion->path, std::string("audio/sfx/explosion_01.mp3"));
        CHECK_NEAR(explosion->volume, 1.0f, 1e-4f);
    }

    // activePlayer/otherPlayers resolve the same "players" list.
    CHECK(provider->sound("activePlayer", "ak47_01") != nullptr);
    CHECK(provider->sound("otherPlayers", "ak47_01") != nullptr);
    CHECK(provider->sound("activePlayer", "explosion_01") == nullptr);

    const audio::SoundGroupDef* whiz = provider->group("bullet_whiz");
    CHECK(whiz != nullptr);
    if (whiz) {
        CHECK_EQ(whiz->channel, std::string("sfx"));
        CHECK_EQ(whiz->sounds.size(), 3u);
    }

    CHECK(provider->reverb("cathedral") != nullptr);
    CHECK(provider->registeredSounds().size() > 100u);
}

// ---------------------------------------------------------------------------
TEST(audio_play_volume) {
    audio::installGeneratedSoundDefs();
    RecordingBackend backend;
    audio::AudioManager manager(&backend);
    manager.preloadSounds();

    // sfx channel volume 1 * baseVolume 0.5 * masterVolume 0.5 = 0.25.
    audio::PlaySoundOptions opts;
    opts.channel = "sfx";
    const audio::SoundHandle h = manager.playSound("explosion_01", opts);
    CHECK(h != audio::kInvalidSound);
    CHECK_EQ(backend.played.size(), 1u);
    if (!backend.played.empty()) {
        CHECK_EQ(backend.played[0].id, std::string("explosion_01sfx"));
        CHECK_NEAR(backend.played[0].volume, 0.25f, 1e-3f);
    }

    // Muting suppresses new sounds.
    manager.setMute(true);
    CHECK_EQ(manager.playSound("explosion_01", opts), audio::kInvalidSound);
    manager.setMute(false);
    CHECK(manager.playSound("explosion_01", opts) != audio::kInvalidSound);
}

// ---------------------------------------------------------------------------
TEST(audio_distance_attenuation) {
    audio::installGeneratedSoundDefs();
    RecordingBackend backend;
    audio::AudioManager manager(&backend);
    manager.preloadSounds();
    manager.cameraPos = Vec2(0.0f, 0.0f);

    // Out of range (maxRange 48): attenuated to zero, not played.
    audio::PlaySoundOptions far;
    far.channel = "sfx";
    far.hasSoundPos = true;
    far.soundPos = Vec2(1000.0f, 0.0f);
    CHECK_EQ(manager.playSound("explosion_01", far), audio::kInvalidSound);
    CHECK_EQ(backend.played.size(), 0u);

    // Half range: volume 0.5 * 0.25 = 0.125, panned right of the listener.
    audio::PlaySoundOptions mid;
    mid.channel = "sfx";
    mid.hasSoundPos = true;
    mid.soundPos = Vec2(24.0f, 0.0f);
    CHECK(manager.playSound("explosion_01", mid) != audio::kInvalidSound);
    CHECK_EQ(backend.played.size(), 1u);
    if (!backend.played.empty()) {
        CHECK_NEAR(backend.played[0].volume, 0.125f, 1e-3f);
        CHECK(backend.played[0].pan > 0.0f);
    }
}

// ---------------------------------------------------------------------------
TEST(audio_group_play) {
    audio::installGeneratedSoundDefs();
    RecordingBackend backend;
    audio::AudioManager manager(&backend);
    manager.preloadSounds();

    audio::PlaySoundOptions opts;
    CHECK(manager.playGroup("bullet_whiz", opts) != audio::kInvalidSound);
    CHECK_EQ(backend.played.size(), 1u);
    if (!backend.played.empty()) {
        const std::string& id = backend.played[0].id;
        CHECK(id.rfind("bullet_whiz_0", 0) == 0);
        CHECK(id.size() > std::string("bullet_whiz_0").size());
    }
}

// ---------------------------------------------------------------------------
TEST(ambiance_tracks) {
    audio::installGeneratedSoundDefs();
    RecordingBackend backend;
    audio::AudioManager manager(&backend);
    manager.preloadSounds();

    audio::Ambiance ambiance;
    audio::AmbienceMap map;
    map.music = "";
    map.wind = "ambient_wind_01";
    map.river = "ambient_stream_01";
    map.waves = "ambient_waves_01";
    ambiance.setMap(map, manager);
    ambiance.onGameStart();

    for (int i = 0; i < 5; i++) {
        ambiance.update(0.2f, manager, true);
    }

    // The wind track starts looping (ambient channel) once loaded.
    CHECK(countPlayed(backend, "ambient_wind_01ambient") >= 1);
    bool windLoops = false;
    for (const auto& p : backend.played) {
        if (p.id == "ambient_wind_01ambient" && p.loop) {
            windLoops = true;
        }
    }
    CHECK(windLoops);
}
