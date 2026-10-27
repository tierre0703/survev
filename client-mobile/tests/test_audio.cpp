// Host tests for the M6 audio port: the codegen'd sound-def provider, the
// AudioManager (volume/attenuation/group/mute), and the Ambiance track mixer.
#include "TestFramework.h"

#include "audio/Ambiance.h"
#include "audio/AudioFilter.h"
#include "audio/AudioManager.h"
#include "audio/GeneratedSoundDefs.h"
#include "audio/ReverbModel.h"
#include "audio/SoundDefs.h"

#include <cmath>
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
        std::string filter;
        float delay = 0.0f;
        float offset = 0.0f;
        float detune = 0.0f;
        bool ambient = false;
    };

    std::vector<Played> played;
    std::unordered_map<std::string, std::string> paths;
    std::unordered_map<std::string, bool> loaded;
    std::unordered_map<audio::SoundHandle, float> volumes;
    std::vector<audio::SoundHandle> stopped;
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
        played.push_back(Played{id, opts.loop, opts.volume, opts.pan, opts.filter,
                                opts.delay, opts.offset, opts.detune, opts.ambient});
        const audio::SoundHandle handle = nextHandle++;
        volumes[handle] = opts.volume;
        return handle;
    }
    void stop(audio::SoundHandle handle) override { stopped.push_back(handle); }
    void stopAll() override { stopped.clear(); }
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

TEST(reverb_model_matches_cathedral_and_cave) {
    audio::installGeneratedSoundDefs();
    const audio::SoundDefProvider* provider = audio::getSoundDefProvider();
    CHECK(provider != nullptr);
    if (!provider) {
        return;
    }

    const audio::ReverbDef* cathedral = provider->reverb("cathedral");
    CHECK(cathedral != nullptr);
    if (cathedral) {
        CHECK_NEAR(cathedral->volume, 0.7f, 1e-4f);
        CHECK_NEAR(cathedral->echoVolume, 0.0f, 1e-4f);
    }

    // Layer 0 underground is silent; the model reports inactive.
    audio::ReverbModel off = audio::reverbModel(0.0f, cathedral);
    CHECK(!off.active);
    CHECK_NEAR(off.gain, 0.0f, 1e-6f);

    // audioManager.ts cathedral: gain = 0.3 * IR volume, decay 0.2..1.7s.
    audio::ReverbModel half = audio::reverbModel(0.5f, cathedral);
    CHECK(half.active);
    CHECK_NEAR(half.gain, 0.3f * 0.5f * 0.7f, 1e-5f);
    CHECK_NEAR(half.decayTime, 0.2f + 1.5f * 0.5f, 1e-5f);
    CHECK_NEAR(half.reflectionsGain, 0.2f * 0.5f, 1e-5f);
    CHECK_NEAR(half.lateReverbGain, 0.5f * 0.5f, 1e-5f);
    CHECK_NEAR(half.echoDepth, 0.0f, 1e-6f);

    audio::ReverbModel full = audio::reverbModel(1.0f, cathedral);
    CHECK_NEAR(full.gain, 0.21f, 1e-5f);
    CHECK_NEAR(full.decayTime, 1.7f, 1e-5f);

    // Volumes above 1 clamp to the full model.
    audio::ReverbModel over = audio::reverbModel(2.0f, cathedral);
    CHECK_NEAR(over.gain, full.gain, 1e-6f);

    // Cave adds the def's echo (echoVolume/echoDelay).
    const audio::ReverbDef* cave = provider->reverb("cave");
    CHECK(cave != nullptr);
    if (cave) {
        audio::ReverbModel c = audio::reverbModel(1.0f, cave);
        CHECK_NEAR(c.echoTime, 0.25f, 1e-5f);
        CHECK_NEAR(c.echoDepth, 0.5f, 1e-5f);
    }

    // No def: default IR volume 1.
    audio::ReverbModel d = audio::reverbModel(1.0f, nullptr);
    CHECK_NEAR(d.gain, 0.3f, 1e-5f);
    CHECK_NEAR(d.decayTime, 1.7f, 1e-5f);
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
TEST(audio_master_volume_scales_live_instances) {
    audio::installGeneratedSoundDefs();
    RecordingBackend backend;
    audio::AudioManager manager(&backend);
    manager.preloadSounds();

    // Startup config (master 1) replaces CreateJS's default 0.5 global volume.
    manager.setMasterVolume(1.0f);
    audio::PlaySoundOptions loop;
    loop.channel = "ambient";
    loop.loop = true;
    const audio::SoundHandle wind = manager.playSound("ambient_wind_01", loop);
    CHECK(wind != audio::kInvalidSound);
    CHECK_NEAR(backend.getVolume(wind), 0.5f, 1e-4f); // base 0.5 * master 1

    // A slider change applies to the already-playing loop (CreateJS global).
    manager.setMasterVolume(0.5f);
    CHECK_NEAR(backend.getVolume(wind), 0.25f, 1e-4f);

    // Per-track volume updates keep the master folded in (Ambiance setVolume).
    manager.setVolume(wind, 0.4f, "sound");
    CHECK_NEAR(backend.getVolume(wind), 0.4f * 1.0f * 0.5f, 1e-4f);
}

// ---------------------------------------------------------------------------
// The Java UI's one-shot cues (SurvBridge.playSound) resolve their volume from
// the channel's type: only the "music" channel follows musicVolume; UI/avatar
// cues ("ui"/"sfx") and — the web's quirk — map/ambient music on the "ambient"
// channel follow soundVolume. Lock it so a refactor cannot silently route the
// avatar/UI cues through the music slider.
TEST(audio_ui_cues_follow_the_channel_type_volume) {
    audio::installGeneratedSoundDefs();
    RecordingBackend backend;
    audio::AudioManager manager(&backend);
    manager.preloadSounds();
    // Startup config replaces CreateJS's default 0.5 global volume.
    manager.setMasterVolume(1.0f);
    manager.setMusicVolume(1.0f);
    manager.setSoundVolume(0.0f);

    audio::PlaySoundOptions music;
    music.channel = "music";
    const audio::SoundHandle menu = manager.playSound("menu_music_01", music);
    CHECK(menu != audio::kInvalidSound);
    // baseVolume 0.5 * music channel 1 * menu_music_01 volume 1.
    CHECK_NEAR(backend.getVolume(menu), 0.5f, 1e-4f);

    // Changing soundVolume leaves the music channel alone.
    manager.setSoundVolume(0.5f);
    CHECK_NEAR(backend.getVolume(menu), 0.5f, 1e-4f);

    // Map/ambient music is on the ambient channel (typed "sound"), so it
    // follows soundVolume: base 0.5 * soundVolume 0.5.
    audio::PlaySoundOptions ambient;
    ambient.channel = "ambient";
    ambient.loop = true;
    const audio::SoundHandle club = manager.playSound("club_music_01", ambient);
    CHECK(club != audio::kInvalidSound);
    CHECK_NEAR(backend.getVolume(club), 0.25f, 1e-4f);

    // Outfit pickup (menu avatar): ui channel 0.75 * base 0.5 * soundVolume 0.5.
    audio::PlaySoundOptions ui;
    ui.channel = "ui";
    const audio::SoundHandle pickup = manager.playSound("clothes_pickup_01", ui);
    CHECK(pickup != audio::kInvalidSound);
    CHECK_NEAR(backend.getVolume(pickup), 0.5f * 0.75f * 0.5f, 1e-4f);

    // Melee swing (avatar attack effect): sfx channel 1 * base 0.5 * 0.5.
    audio::PlaySoundOptions swing;
    swing.channel = "sfx";
    const audio::SoundHandle hit = manager.playSound("punch_swing_01", swing);
    CHECK(hit != audio::kInvalidSound);
    CHECK_NEAR(backend.getVolume(hit), 0.25f, 1e-4f);

    // musicVolume 0 mutes the music channel without touching UI cues.
    manager.setMusicVolume(0.0f);
    const audio::SoundHandle muted = manager.playSound("menu_music_01", music);
    CHECK(muted != audio::kInvalidSound);
    CHECK_NEAR(backend.getVolume(muted), 0.0f, 1e-5f);
    const audio::SoundHandle stillAudible = manager.playSound("clothes_pickup_01", ui);
    CHECK_NEAR(backend.getVolume(stillAudible), 0.5f * 0.75f * 0.5f, 1e-4f);
}

// ---------------------------------------------------------------------------
TEST(audio_stop_all_retains_ambient_voices) {
    audio::installGeneratedSoundDefs();
    RecordingBackend backend;
    audio::AudioManager manager(&backend);
    manager.preloadSounds();

    audio::PlaySoundOptions ambience;
    ambience.channel = "ambient";
    ambience.loop = true;
    const audio::SoundHandle wind = manager.playSound("ambient_wind_01", ambience);

    audio::PlaySoundOptions sfx;
    sfx.channel = "sfx";
    const audio::SoundHandle hit = manager.playSound("explosion_01", sfx);

    manager.stopAll(true);
    CHECK_EQ(backend.stopped.size(), 1u);
    if (!backend.stopped.empty()) {
        CHECK_EQ(backend.stopped[0], hit);
    }
    CHECK(wind != hit);

    // stopAll(false) is the hard stop (backend.stopAll path is separate).
    manager.stopAll(false);
    CHECK_EQ(backend.stopped.size(), 2u);
}

// ---------------------------------------------------------------------------
TEST(audio_channel_semantics) {
    audio::installGeneratedSoundDefs();
    const audio::SoundDefProvider* provider = audio::getSoundDefProvider();
    CHECK(provider != nullptr);
    if (!provider) {
        return;
    }
    const audio::SoundDef* m9 = provider->sound("activePlayer", "m9_01");
    const audio::SoundDef* stone = provider->sound("hits", "stone_bullet_hit_01");
    CHECK(m9 && stone);
    if (!m9 || !stone) {
        return;
    }
    CHECK_EQ(m9->maxInstances, 5);
    CHECK(stone->canCoalesce);

    RecordingBackend backend;
    audio::AudioManager manager(&backend);
    manager.preloadSounds();
    manager.setMasterVolume(1.0f);

    // maxInstances: the 6th m9 shot kills the oldest voice.
    audio::PlaySoundOptions gun;
    gun.channel = "activePlayer";
    std::vector<audio::SoundHandle> shots;
    for (int i = 0; i < 6; ++i) {
        shots.push_back(manager.playSound("m9_01", gun));
    }
    CHECK_EQ(shots.size(), 6u);
    CHECK_EQ(backend.stopped.size(), 1u);
    if (!backend.stopped.empty()) {
        CHECK_EQ(backend.stopped[0], shots[0]);
    }

    // canCoalesce: two hits in the same 30 ms window share one voice on an
    // equal-power basis (hits channel volume 0.4 * base 0.5 = 0.2 each).
    backend.played.clear();
    audio::PlaySoundOptions hit;
    hit.channel = "hits";
    const audio::SoundHandle first = manager.playSound("stone_bullet_hit_01", hit);
    const audio::SoundHandle second = manager.playSound("stone_bullet_hit_01", hit);
    CHECK(first != audio::kInvalidSound);
    CHECK_EQ(second, audio::kInvalidSound);
    CHECK_EQ(backend.played.size(), 1u);
    CHECK_NEAR(backend.getVolume(first), std::sqrt(0.2f * 0.2f + 0.2f * 0.2f), 1e-3f);

    // The window closes once 30 ms have passed.
    manager.update(0.04f);
    CHECK(manager.playSound("stone_bullet_hit_01", hit) != audio::kInvalidSound);
    CHECK_EQ(backend.played.size(), 2u);
}

// ---------------------------------------------------------------------------
TEST(audio_delay_and_detune) {
    audio::installGeneratedSoundDefs();
    RecordingBackend backend;
    audio::AudioManager manager(&backend);
    manager.preloadSounds();

    audio::PlaySoundOptions opts;
    opts.channel = "sfx";
    opts.delay = 0.05f;
    opts.detune = 1200.0f;
    CHECK_EQ(manager.playSound("explosion_01", opts), audio::kInvalidSound);
    CHECK_EQ(backend.played.size(), 0u);

    manager.update(0.02f);
    CHECK_EQ(backend.played.size(), 0u);
    manager.update(0.04f);
    CHECK_EQ(backend.played.size(), 1u);
    if (!backend.played.empty()) {
        CHECK_NEAR(backend.played[0].delay, 0.0f, 1e-6f); // delay consumed by the manager
        CHECK_NEAR(backend.played[0].detune, 1200.0f, 1e-4f);
    }
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

// ---------------------------------------------------------------------------
TEST(audio_filter_mapping) {
    // The web's createJS EQ chains are approximated by a single EFX low-pass.
    const audio::FilterParams none = audio::filterParamsFor("none");
    CHECK(!none.enabled);
    CHECK_NEAR(none.gain, 1.0f, 1e-6f);

    // "reverb" is a send, not an EQ; the EAX path handles it.
    CHECK(!audio::filterParamsFor("reverb").enabled);

    const audio::FilterParams muffled = audio::filterParamsFor("muffled");
    CHECK(muffled.enabled);
    CHECK(muffled.gainHF < 0.1f);
    CHECK(muffled.gain > muffled.gainHF);

    const audio::FilterParams club = audio::filterParamsFor("club");
    CHECK(club.enabled);
    CHECK(club.gainHF < muffled.gainHF); // club cuts the highs harder
    CHECK(club.gain > club.gainHF);
}

// ---------------------------------------------------------------------------
TEST(audio_filter_routing) {
    audio::installGeneratedSoundDefs();
    RecordingBackend backend;
    audio::AudioManager manager(&backend);
    manager.preloadSounds();
    manager.activeLayer = 0;

    audio::PlaySoundOptions opts;
    opts.channel = "sfx";
    opts.filter = "muffled";
    opts.hasLayer = true;
    opts.layer = 0;

    // Same layer without forceFilter -> the web uses the reverb send.
    CHECK(manager.playSound("explosion_01", opts) != audio::kInvalidSound);
    CHECK_EQ(backend.played.back().filter, std::string("reverb"));

    // Different layer -> the EQ is applied.
    manager.activeLayer = 1;
    CHECK(manager.playSound("explosion_01", opts) != audio::kInvalidSound);
    CHECK_EQ(backend.played.back().filter, std::string("muffled"));

    // forceFilter bypasses the layer rule (audioManager.ts).
    manager.activeLayer = 0;
    opts.forceFilter = true;
    CHECK(manager.playSound("explosion_01", opts) != audio::kInvalidSound);
    CHECK_EQ(backend.played.back().filter, std::string("muffled"));

    // Club filter and unfiltered sounds.
    opts.forceFilter = false;
    manager.activeLayer = 1;
    opts.filter = "club";
    CHECK(manager.playSound("explosion_01", opts) != audio::kInvalidSound);
    CHECK_EQ(backend.played.back().filter, std::string("club"));

    audio::PlaySoundOptions plain;
    plain.channel = "sfx";
    CHECK(manager.playSound("explosion_01", plain) != audio::kInvalidSound);
    CHECK_EQ(backend.played.back().filter, std::string("none"));
}
