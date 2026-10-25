#include "AudioManager.h"

#include "../core/MathUtil.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace surv {
namespace audio {

// Re-exported from MathUtil.h for conciseness.
static bool sameAudioLayer(int a, int b) {
    return math::sameAudioLayer(a, b);
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
    _time += dt;
    // Delayed plays (createJS `delay`, in seconds for the native API; shot.ts
    // converts its millisecond values before calling us).
    for (size_t i = _pending.size(); i-- > 0;) {
        Pending& pending = _pending[i];
        pending.remaining -= dt;
        if (pending.remaining <= 0.0f) {
            const std::string sound = pending.sound;
            const PlaySoundOptions options = pending.options;
            _pending.erase(_pending.begin() + static_cast<long>(i));
            playSound(sound, options);
        }
    }

    if (_backend) {
        for (size_t i = _instances.size(); i-- > 0;) {
            if (!_backend->isPlaying(_instances[i].handle)) {
                _instances.erase(_instances.begin() + static_cast<long>(i));
            }
        }

        // Reverb: match audioManager.ts update() — compute cathedral reverb
        // volume from the active layer when underground.
        const float layerVolumeMap[] = {0.0f, 1.0f, 1.0f / 3.0f, 2.0f / 3.0f};
        const int clampedLayer = math::clamp(activeLayer, 0, 3);
        const float reverbVolume = underground ? layerVolumeMap[clampedLayer] : 0.0f;
        _backend->setReverbVolume("cathedral", reverbVolume);

        _backend->update(dt);
    }
}

AudioManager::Instance* AudioManager::findInstance(SoundHandle handle) {
    for (auto& inst : _instances) {
        if (inst.handle == handle) {
            return &inst;
        }
    }
    return nullptr;
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
    const SoundDef* soundDef = provider->sound(channel, sound);
    // createJS.play() accepts delay in milliseconds; the native API uses
    // seconds (shot.ts converts `30` -> 0.03 before calling). Schedule it and
    // return a null handle, like a sound that is still loading on the web.
    if (options.delay > 0.0f) {
        Pending pending;
        pending.remaining = options.delay;
        pending.sound = sound;
        pending.options = options;
        pending.options.delay = 0.0f;
        _pending.push_back(std::move(pending));
        return kInvalidSound;
    }

    const float defVolume = soundDef ? soundDef->volume : 1.0f;
    const bool ambient = channel == "ambient" || channel == "music" || options.ambient;
    // CreateJS's global volume (master) is applied separately; keep the
    // type-scaled value so master can be changed later even from 0.
    const float baseVol = baseVolume * getTypeVolume(a->isMusic ? "music" : "sound") *
                          options.volumeScale * defVolume;
    const bool diffLayer = options.hasLayer && !sameAudioLayer(options.layer, activeLayer);
    // audioManager.ts: a filtered sound uses its EQ only when it is on another
    // layer (or forceFilter is set); otherwise it goes through the reverb send.
    const std::string filter = options.filter.empty() ? "none"
        : ((diffLayer || options.forceFilter) ? options.filter : "reverb");

    float preVolume = a->volume * baseVol;
    float pan = 0.0f;
    if (diffLayer) {
        preVolume *= kDiffLayerMult;
    }
    const bool positional = channel != "activePlayer" && options.hasSoundPos;
    if (positional) {
        const Vec2 diff = v2Sub(cameraPos, options.soundPos);
        const float dist = v2Length(diff);
        float range = a->maxRange * options.rangeMult;
        if (math::eqAbs(range, 0.0f)) {
            range = 1.0f;
        }
        const float distNormal = math::clamp(std::fabs(dist / range), 0.0f, 1.0f);
        const float scaledVolume = std::pow(1.0f - distNormal, 1.0f + options.fallOff * 2.0f);
        preVolume = a->volume * scaledVolume * baseVol;
        if (diffLayer) {
            preVolume *= kDiffLayerMult;
        }
        // The web's min-volume gate runs on the type-scaled volume (the global
        // master is a separate gain node), so keep it before the master.
        if (preVolume <= kMinAllowedVolume && !options.ignoreMinAllowable) {
            return kInvalidSound;
        }
        pan = math::clamp((diff.x / range) * -1.0f, -1.0f, 1.0f);
    }
    const float clipVolume = math::clamp(preVolume * masterVolume, 0.0f, 1.0f);

    // createJS canCoalesce: merge a same-sound instance started in the same
    // 30 ms window on an equal-power basis instead of starting another voice.
    if (soundDef && soundDef->canCoalesce) {
        constexpr float kCoalesceTime = 0.03f;
        for (auto& inst : _instances) {
            if (inst.key != key || !_backend->isPlaying(inst.handle)) {
                continue;
            }
            if (std::fabs(inst.startTime - _time) > kCoalesceTime) {
                continue;
            }
            const float vv = inst.volume * inst.volume + clipVolume * clipVolume;
            const float vp = inst.volume * inst.pan + clipVolume * pan;
            const float v = inst.volume + clipVolume;
            inst.volume = std::sqrt(vv);
            inst.pan = vp / std::max(0.001f, v);
            inst.base = masterVolume > 0.0001f ? inst.volume / masterVolume : inst.volume;
            _backend->setVolume(inst.handle, math::clamp(inst.volume, 0.0f, 1.0f));
            _backend->setPan(inst.handle, inst.pan);
            return kInvalidSound;
        }
    }

    // createJS maxInstances: kill the oldest voice of this sound before
    // starting another one.
    const int maxInstances = (soundDef && soundDef->maxInstances > 0) ? soundDef->maxInstances : 16;
    int live = 0;
    for (const auto& inst : _instances) {
        if (inst.key == key && _backend->isPlaying(inst.handle)) {
            ++live;
        }
    }
    while (live >= maxInstances) {
        Instance* oldest = nullptr;
        for (auto& inst : _instances) {
            if (inst.key == key && _backend->isPlaying(inst.handle) &&
                (!oldest || inst.startTime < oldest->startTime)) {
                oldest = &inst;
            }
        }
        if (!oldest) {
            break;
        }
        const SoundHandle handle = oldest->handle;
        _backend->stop(handle);
        for (size_t i = 0; i < _instances.size(); ++i) {
            if (_instances[i].handle == handle) {
                _instances.erase(_instances.begin() + static_cast<long>(i));
                break;
            }
        }
        --live;
    }

    PlayOptions po;
    po.loop = options.loop;
    po.volume = options.startSilent ? 0.0f : clipVolume;
    po.pan = pan;
    po.offset = options.offset;
    po.ambient = ambient;
    po.detune = options.detune;
    po.filter = filter;
    const SoundHandle instance = _backend->play(key, po);
    if (instance != kInvalidSound) {
        Instance inst;
        inst.handle = instance;
        inst.key = key;
        inst.type = channel == "music" ? "music" : "sound";
        inst.base = preVolume;
        inst.volume = po.volume;
        inst.pan = po.pan;
        inst.ambient = ambient;
        inst.loop = options.loop;
        inst.startTime = _time;
        _instances.push_back(inst);
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
    const float baseVol = baseVolume * getTypeVolume(a->isMusic ? "music" : "sound") * options.volumeScale;
    const Vec2 diff = v2Sub(cameraPos, soundPos);
    const float dist = v2Length(diff);
    float range = a->maxRange * options.rangeMult;
    if (math::eqAbs(range, 0.0f)) {
        range = 1.0f;
    }
    const float distNormal = math::clamp(std::fabs(dist / range), 0.0f, 1.0f);
    const float scaledVolume = std::pow(1.0f - distNormal, 1.0f + options.fallOff * 2.0f);
    float preVolume = a->volume * scaledVolume * baseVol;
    const bool sameLayer = !options.hasLayer || sameAudioLayer(options.layer, activeLayer);
    if (!sameLayer) {
        preVolume *= kDiffLayerMult;
    }
    if (preVolume > kMinAllowedVolume || options.ignoreMinAllowable) {
        const float base = math::clamp(preVolume, 0.0f, 1.0f);
        const float effective = math::clamp(base * masterVolume, 0.0f, 1.0f);
        _backend->setVolume(instance, effective);
        _backend->setPan(instance, math::clamp((diff.x / range) * -1.0f, -1.0f, 1.0f));
        if (Instance* inst = findInstance(instance)) {
            inst->base = base;
            inst->volume = effective;
        }
    }
}

void AudioManager::setMasterVolume(float volume) {
    masterVolume = math::clamp(volume, 0.0f, 1.0f);
    // CreateJS.Sound.volume is a global gain applied to every live instance.
    // Recompute from the pre-master value so 0 -> 1 works too.
    if (_backend) {
        for (auto& inst : _instances) {
            inst.volume = math::clamp(inst.base * masterVolume, 0.0f, 1.0f);
            _backend->setVolume(inst.handle, inst.volume);
        }
    }
}

void AudioManager::applyInstanceVolumeType(const std::string& type, float volume) {
    volume = math::clamp(volume, 0.0f, 1.0f);
    const float typeVolume = getTypeVolume(type);
    const float scaledVolume = typeVolume > 0.0001f ? volume / typeVolume : 0.0f;
    for (auto& inst : _instances) {
        if (inst.type == type && _backend) {
            inst.base = math::clamp(inst.base * scaledVolume, 0.0f, 1.0f);
            inst.volume = math::clamp(inst.base * masterVolume, 0.0f, 1.0f);
            _backend->setVolume(inst.handle, inst.volume);
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
    // CreateJS global volume (master) applies on top of the per-type volume;
    // ambience/interior/emitter updates overwrite instance.volume, so the
    // master has to be folded in here or it is lost after the first update.
    const float base = math::clamp(volume * getTypeVolume(t), 0.0f, 1.0f);
    const float effective = math::clamp(base * masterVolume, 0.0f, 1.0f);
    _backend->setVolume(instance, effective);
    if (Instance* inst = findInstance(instance)) {
        inst->base = base;
        inst->volume = effective;
    }
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

void AudioManager::stopAll(bool retainAmbient) {
    if (!_backend) {
        return;
    }
    for (size_t i = _instances.size(); i-- > 0;) {
        Instance& inst = _instances[i];
        if (retainAmbient && inst.ambient) {
            continue;
        }
        _backend->stop(inst.handle);
        _instances.erase(_instances.begin() + static_cast<long>(i));
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
