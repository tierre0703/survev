#include "AudioManager.h"

#include "../core/MathUtil.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace surv {
namespace audio {

static bool sameAudioLayer(int a, int b) {
    return a == b || (a & 0x2) != 0 || (b & 0x2) != 0;
}

void AudioManager::loadSound(const std::string& name, const std::string& channel,
                             const std::string& path) {
    const std::string key = name + channel;
    if (_sounds.find(key) != _sounds.end()) {
        return;
    }
    if (_backend) {
        _backend->load(key, path);
    }
    _sounds[key] = SoundEntry{path, name, channel};
}

void AudioManager::ensureRegistered(const std::string& name, const std::string& channel) {
    const std::string key = name + channel;
    if (_sounds.find(key) != _sounds.end()) {
        return;
    }
    const SoundDefProvider* provider = getSoundDefProvider();
    const SoundDef* def = provider ? provider->sound(channel, name) : nullptr;
    if (def) {
        loadSound(name, channel, def->path);
    }
}

void AudioManager::preloadSounds() {
    if (_preloaded) {
        return;
    }
    _preloaded = true;
    const SoundDefProvider* provider = getSoundDefProvider();
    if (!provider) {
        return;
    }
    std::vector<RegisteredSound> list = provider->registeredSounds();
    std::stable_sort(list.begin(), list.end(), [](const RegisteredSound& a, const RegisteredSound& b) {
        return a.loadPriority > b.loadPriority;
    });
    for (const auto& sound : list) {
        if (!sound.preload) {
            continue;
        }
        loadSound(sound.name, sound.channel, sound.path);
    }
}

void AudioManager::update(float dt) {
    if (_backend) {
        for (size_t i = _instances.size(); i-- > 0;) {
            if (!_backend->isPlaying(_instances[i].handle)) {
                _instances.erase(_instances.begin() + static_cast<long>(i));
            }
        }
        _backend->update(dt);
    }
}

SoundHandle AudioManager::playSound(const std::string& sound, const PlaySoundOptions& options) {
    if (sound.empty() || sound == "none") {
        return kInvalidSound;
    }
    if (forcedMute) {
        return kInvalidSound;
    }
    const SoundDefProvider* provider = getSoundDefProvider();
    if (!provider) {
        return kInvalidSound;
    }
    const std::string channel = options.channel.empty() ? "activePlayer" : options.channel;
    const ChannelDef* a = provider->channel(channel);
    if (!a) {
        return kInvalidSound;
    }
    if (mute && !options.forceStart) {
        return kInvalidSound;
    }
    ensureRegistered(sound, channel);
    const std::string key = sound + channel;
    if (_sounds.find(key) == _sounds.end() || !_backend) {
        return kInvalidSound;
    }

    const bool ambient = channel == "ambient" || channel == "music" || options.ambient;
    const float baseVol = baseVolume * masterVolume *
                          getTypeVolume(a->isMusic ? "music" : "sound") * options.volumeScale;
    const bool diffLayer = options.hasLayer && !sameAudioLayer(options.layer, activeLayer);

    SoundHandle instance = kInvalidSound;
    if (channel != "activePlayer" && options.hasSoundPos) {
        const Vec2 diff = v2Sub(cameraPos, options.soundPos);
        const float dist = v2Length(diff);
        float range = a->maxRange * options.rangeMult;
        if (math::eqAbs(range, 0.0f)) {
            range = 1.0f;
        }
        const float distNormal = math::clamp(std::fabs(dist / range), 0.0f, 1.0f);
        const float scaledVolume = std::pow(1.0f - distNormal, 1.0f + options.fallOff * 2.0f);
        float clipVolume = a->volume * scaledVolume * baseVol;
        if (diffLayer) {
            clipVolume *= kDiffLayerMult;
        }
        if (clipVolume > kMinAllowedVolume || options.ignoreMinAllowable) {
            PlayOptions po;
            po.loop = options.loop;
            po.volume = options.startSilent ? 0.0f : clipVolume;
            po.pan = math::clamp((diff.x / range) * -1.0f, -1.0f, 1.0f);
            po.delay = options.delay;
            po.offset = options.offset;
            po.ambient = ambient;
            po.detune = options.detune;
            instance = _backend->play(key, po);
        }
    } else {
        float clipVolume = a->volume * baseVol;
        if (diffLayer) {
            clipVolume *= kDiffLayerMult;
        }
        PlayOptions po;
        po.loop = options.loop;
        po.volume = options.startSilent ? 0.0f : clipVolume;
        po.delay = options.delay;
        po.offset = options.offset;
        po.ambient = ambient;
        po.detune = options.detune;
        instance = _backend->play(key, po);
    }

    if (instance != kInvalidSound && (options.loop || channel == "music")) {
        _instances.push_back(Instance{instance, channel == "music" ? "music" : "sound"});
    }
    return instance;
}

SoundHandle AudioManager::playGroup(const std::string& group, const PlaySoundOptions& options) {
    const SoundDefProvider* provider = getSoundDefProvider();
    const SoundGroupDef* def = provider ? provider->group(group) : nullptr;
    if (!def || def->sounds.empty()) {
        return kInvalidSound;
    }
    const size_t index = static_cast<size_t>(std::rand()) % def->sounds.size();
    PlaySoundOptions opts = options;
    opts.channel = def->channel;
    return playSound(def->sounds[index], opts);
}

void AudioManager::updateSound(SoundHandle instance, const std::string& channel,
                               const Vec2& soundPos, const PlaySoundOptions& options) {
    const SoundDefProvider* provider = getSoundDefProvider();
    const ChannelDef* a = provider ? provider->channel(channel) : nullptr;
    if (instance == kInvalidSound || !a || !_backend) {
        return;
    }
    const float baseVol = baseVolume * masterVolume *
                          getTypeVolume(a->isMusic ? "music" : "sound") * options.volumeScale;
    const Vec2 diff = v2Sub(cameraPos, soundPos);
    const float dist = v2Length(diff);
    float range = a->maxRange * options.rangeMult;
    if (math::eqAbs(range, 0.0f)) {
        range = 1.0f;
    }
    const float distNormal = math::clamp(std::fabs(dist / range), 0.0f, 1.0f);
    const float scaledVolume = std::pow(1.0f - distNormal, 1.0f + options.fallOff * 2.0f);
    float clipVolume = a->volume * scaledVolume * baseVol;
    const bool sameLayer = !options.hasLayer || sameAudioLayer(options.layer, activeLayer);
    if (!sameLayer) {
        clipVolume *= kDiffLayerMult;
    }
    if (clipVolume > kMinAllowedVolume || options.ignoreMinAllowable) {
        _backend->setVolume(instance, clipVolume);
        _backend->setPan(instance, math::clamp((diff.x / range) * -1.0f, -1.0f, 1.0f));
    }
}

void AudioManager::setMasterVolume(float volume) {
    masterVolume = math::clamp(volume, 0.0f, 1.0f);
}

void AudioManager::applyInstanceVolumeType(const std::string& type, float volume) {
    volume = math::clamp(volume, 0.0f, 1.0f);
    const float typeVolume = getTypeVolume(type);
    const float scaledVolume = typeVolume > 0.0001f ? volume / typeVolume : 0.0f;
    for (auto& inst : _instances) {
        if (inst.type == type && _backend) {
            _backend->setVolume(inst.handle, _backend->getVolume(inst.handle) * scaledVolume);
        }
    }
}

void AudioManager::setSoundVolume(float volume) {
    applyInstanceVolumeType("sound", volume);
    soundVolume = volume;
}

void AudioManager::setMusicVolume(float volume) {
    applyInstanceVolumeType("music", volume);
    musicVolume = volume;
}

void AudioManager::setVolume(SoundHandle instance, float volume, const std::string& type) {
    if (instance == kInvalidSound || !_backend) {
        return;
    }
    const std::string t = type.empty() ? "sound" : type;
    _backend->setVolume(instance, volume * getTypeVolume(t));
}

float AudioManager::getVolume(SoundHandle instance) const {
    return _backend ? _backend->getVolume(instance) : 0.0f;
}

void AudioManager::setMute(bool value) {
    mute = value;
    if (_backend) {
        _backend->setMute(mute || forcedMute);
    }
}

void AudioManager::setForcedMute(bool value) {
    forcedMute = value;
    if (forcedMute) {
        stopAll();
    }
    if (_backend) {
        _backend->setMute(mute || forcedMute);
    }
}

bool AudioManager::muteToggle() {
    setMute(!mute);
    return mute;
}

void AudioManager::stopSound(SoundHandle instance) {
    if (_backend) {
        _backend->stop(instance);
    }
}

void AudioManager::stopAll() {
    if (_backend) {
        _backend->stopAll();
    }
}

bool AudioManager::allLoaded() const {
    for (const auto& kv : _sounds) {
        if (!isSoundLoaded(kv.second.name, kv.second.channel)) {
            return false;
        }
    }
    return true;
}

bool AudioManager::isSoundLoaded(const std::string& soundName, const std::string& channel) const {
    const std::string key = soundName + channel;
    auto it = _sounds.find(key);
    return it != _sounds.end() && _backend && _backend->isLoaded(key);
}

bool AudioManager::isSoundPlaying(SoundHandle instance) const {
    return _backend && _backend->isPlaying(instance);
}

float AudioManager::getSoundDefVolume(const std::string& sound, const std::string& channel) const {
    const SoundDefProvider* provider = getSoundDefProvider();
    const SoundDef* def = provider ? provider->sound(channel, sound) : nullptr;
    const ChannelDef* ch = provider ? provider->channel(channel) : nullptr;
    if (def && ch) {
        return def->volume * ch->volume;
    }
    return 1.0f;
}

float AudioManager::getTypeVolume(const std::string& type) const {
    if (type == "music") {
        return musicVolume;
    }
    return soundVolume;
}

} // namespace audio
} // namespace surv
