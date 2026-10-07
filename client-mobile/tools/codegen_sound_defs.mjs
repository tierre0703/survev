// Generates src/audio/GeneratedSoundDefs.cpp from client/src/soundDefs.ts.
// soundDefs.ts has no imports, so it can be loaded directly by the TS
// transform loader.
//
// Usage: node --experimental-transform-types tools/codegen_sound_defs.mjs
import { fileURLToPath } from "node:url";
import * as path from "node:path";
import * as fs from "node:fs";

const here = path.dirname(fileURLToPath(import.meta.url));
const clientDir = path.resolve(here, "../../client");

function pathToFile(dir, rel) {
    return new URL(`file:///${path.join(dir, rel).replace(/\\/g, "/")}`);
}

const { default: soundDefs } = await import(pathToFile(clientDir, "src/soundDefs.ts"));
const { Sounds, Groups, Channels, Reverbs } = soundDefs;

const n = (x, d = 0) => (Number.isFinite(Number(x)) ? Number(x) : d);
const esc = (s) => String(s ?? "").replace(/\\/g, "\\\\").replace(/"/g, '\\"');
const str = (s) => `"${esc(s)}"`;

let soundArrays = "";
let soundEntries = "";
// list -> [names] so the provider can build list/name lookups.
for (const [list, sounds] of Object.entries(Sounds)) {
    for (const [name, def] of Object.entries(sounds)) {
        soundEntries += `    {${str(list)},${str(name)},${str(def.path)},${n(def.volume, 1)},${
            def.canCoalesce ? 1 : 0
        },${n(def.maxInstances, 0)},${def.preload === undefined || def.preload ? 1 : 0},${n(
            def.loadPriority,
            0,
        )}},\n`;
    }
}

let channelEntries = "";
for (const [name, def] of Object.entries(Channels)) {
    channelEntries += `    {${str(name)},${n(def.volume, 1)},${n(def.maxRange, 48)},${str(
        def.list,
    )},${def.type === "music" ? 1 : 0}},\n`;
}

let groupArrays = "";
let groupEntries = "";
let groupIdx = 0;
for (const [name, def] of Object.entries(Groups)) {
    const arrName = `kGroupSounds_${groupIdx++}`;
    groupArrays += `static const char* const ${arrName}[] = {${def.sounds.map(str).join(",")}};\n`;
    groupEntries += `    {${str(name)},${str(def.channel)},${arrName},${def.sounds.length}},\n`;
}

let reverbEntries = "";
for (const [name, def] of Object.entries(Reverbs)) {
    reverbEntries += `    {${str(name)},${str(def.path || "")},${n(def.volume, 1)},${n(
        def.stereoSpread,
        0,
    )},${n(def.echoVolume, 0)},${n(def.echoDelay, 0)},${n(def.echoLowPass, 0)}},\n`;
}

// Registered sounds: for each channel, every sound in its Sounds list. This is
// what AudioManager::preloadSounds iterates.
let registeredEntries = "";
for (const [channelName, channel] of Object.entries(Channels)) {
    const list = Sounds[channel.list];
    if (!list) continue;
    for (const [name, def] of Object.entries(list)) {
        registeredEntries += `    {${str(channelName)},${str(name)},${str(def.path)},${n(
            def.volume,
            1,
        )},${def.canCoalesce ? 1 : 0},${n(def.maxInstances, 0)},${
            def.preload === undefined || def.preload ? 1 : 0
        },${n(def.loadPriority, 0)}},\n`;
    }
}

const out = `// GENERATED FILE - do not edit. Run tools/codegen_sound_defs.mjs.
#include "SoundDefs.h"
#include <string>
#include <unordered_map>

namespace surv {
namespace audio {
namespace {

struct RawSound { const char* list; const char* name; const char* path; float volume; int canCoalesce; int maxInstances; int preload; int loadPriority; };
struct RawChannel { const char* name; float volume; float maxRange; const char* list; int isMusic; };
struct RawGroup { const char* name; const char* channel; const char* const* sounds; int count; };
struct RawReverb { const char* name; const char* path; float volume; float stereoSpread; float echoVolume; float echoDelay; float echoLowPass; };
struct RawRegistered { const char* channel; const char* name; const char* path; float volume; int canCoalesce; int maxInstances; int preload; int loadPriority; };

static const RawSound kSounds[] = {
${soundEntries}};
static const RawChannel kChannels[] = {
${channelEntries}};
${groupArrays}static const RawGroup kGroups[] = {
${groupEntries}};
static const RawReverb kReverbs[] = {
${reverbEntries}};
static const RawRegistered kRegistered[] = {
${registeredEntries}};

class GeneratedSoundDefProvider : public SoundDefProvider {
public:
    GeneratedSoundDefProvider() {
        for (const auto& r : kSounds) {
            SoundDef d;
            d.name = r.name;
            d.list = r.list;
            d.path = r.path;
            d.volume = r.volume;
            d.canCoalesce = r.canCoalesce != 0;
            d.maxInstances = r.maxInstances;
            d.preload = r.preload != 0;
            d.loadPriority = r.loadPriority;
            _sounds[std::string(r.list) + "\\x1f" + r.name] = std::move(d);
        }
        for (const auto& r : kChannels) {
            ChannelDef d;
            d.name = r.name;
            d.volume = r.volume;
            d.maxRange = r.maxRange;
            d.list = r.list;
            d.isMusic = r.isMusic != 0;
            _channels[r.name] = std::move(d);
        }
        for (const auto& r : kGroups) {
            SoundGroupDef d;
            d.name = r.name;
            d.channel = r.channel;
            for (int i = 0; i < r.count; i++) d.sounds.push_back(r.sounds[i]);
            _groups[d.name] = std::move(d);
        }
        for (const auto& r : kReverbs) {
            ReverbDef d;
            d.name = r.name;
            d.path = r.path;
            d.volume = r.volume;
            d.stereoSpread = r.stereoSpread;
            d.echoVolume = r.echoVolume;
            d.echoDelay = r.echoDelay;
            d.echoLowPass = r.echoLowPass;
            _reverbs[d.name] = std::move(d);
        }
        for (const auto& r : kRegistered) {
            RegisteredSound d;
            d.channel = r.channel;
            d.name = r.name;
            d.path = r.path;
            d.volume = r.volume;
            d.canCoalesce = r.canCoalesce != 0;
            d.maxInstances = r.maxInstances;
            d.preload = r.preload != 0;
            d.loadPriority = r.loadPriority;
            _registered.push_back(std::move(d));
        }
    }

    const ChannelDef* channel(const std::string& name) const override {
        auto it = _channels.find(name);
        return it == _channels.end() ? nullptr : &it->second;
    }
    const SoundDef* sound(const std::string& channel, const std::string& name) const override {
        auto ch = _channels.find(channel);
        if (ch == _channels.end()) return nullptr;
        auto it = _sounds.find(ch->second.list + "\\x1f" + name);
        return it == _sounds.end() ? nullptr : &it->second;
    }
    const SoundGroupDef* group(const std::string& name) const override {
        auto it = _groups.find(name);
        return it == _groups.end() ? nullptr : &it->second;
    }
    const ReverbDef* reverb(const std::string& name) const override {
        auto it = _reverbs.find(name);
        return it == _reverbs.end() ? nullptr : &it->second;
    }
    const std::vector<RegisteredSound>& registeredSounds() const override {
        return _registered;
    }

private:
    std::unordered_map<std::string, SoundDef> _sounds;
    std::unordered_map<std::string, ChannelDef> _channels;
    std::unordered_map<std::string, SoundGroupDef> _groups;
    std::unordered_map<std::string, ReverbDef> _reverbs;
    std::vector<RegisteredSound> _registered;
};

} // namespace

void installGeneratedSoundDefs() {
    static GeneratedSoundDefProvider provider;
    setSoundDefProvider(&provider);
}

} // namespace audio
} // namespace surv
`;

const dest = path.join(here, "../src/audio/GeneratedSoundDefs.cpp");
fs.mkdirSync(path.dirname(dest), { recursive: true });
fs.writeFileSync(dest, out);
console.log(`wrote ${dest}`);
console.log(
    `sounds=${Object.values(Sounds).reduce((a, s) => a + Object.keys(s).length, 0)} channels=${
        Object.keys(Channels).length
    } groups=${Object.keys(Groups).length} reverbs=${Object.keys(Reverbs).length} registered=${
        registeredEntries.split("\n").length - 1
    }`,
);
