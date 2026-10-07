#include "Config.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>

namespace ui {

namespace {

const std::map<std::string, JsonValue>& webDefaults() {
    static const std::map<std::string, JsonValue> defaults = [] {
        std::map<std::string, JsonValue> d;
        auto num = [&d](const std::string& k, double v) {
            JsonValue j;
            j.kind = JsonValue::Kind::Number;
            j.number = v;
            d[k] = j;
        };
        auto boolean = [&d](const std::string& k, bool v) {
            JsonValue j;
            j.kind = JsonValue::Kind::Bool;
            j.boolean = v;
            d[k] = j;
        };
        auto text = [&d](const std::string& k, const std::string& v) {
            JsonValue j;
            j.kind = JsonValue::Kind::String;
            j.string = v;
            d[k] = j;
        };

        // config.ts defaultConfig (excluding the debug-only blocks).
        boolean("anonPlayerNames", false);
        boolean("highResTex", true);
        boolean("interpolation", true);
        boolean("localRotation", false);
        boolean("muteAudio", false);
        boolean("screenShake", true);
        num("masterVolume", 1);
        num("musicVolume", 1);
        num("soundVolume", 1);
        text("touchMoveStyle", "anywhere");
        text("touchAimStyle", "anywhere");
        boolean("touchAimLine", true);
        text("binds", "");
        text("clientTheme", "main");
        text("cachedBgImg", "img/splashes/main.webp");
        text("language", "en");
        text("playerName", "");
        text("region", "na");
        boolean("regionSelected", false);
        text("sessionCookie", "");
        num("gameModeIdx", 2);
        boolean("teamAutoFill", true);
        text("perkModeRole", "");
        num("prerollGamesPlayed", 0);
        num("lastNewsTimestamp", 0);
        num("totalGamesPlayed", 0);
        boolean("promptAppRate", true);
        num("version", 1);
        return d;
    }();
    return defaults;
}

void encodeString(std::string& out, const std::string& s) {
    out.push_back('"');
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out.push_back(c); break;
        }
    }
    out.push_back('"');
}

void encodeNumber(std::string& out, double v) {
    if (v == std::floor(v) && std::fabs(v) < 1e15) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(v));
        out += buf;
        return;
    }
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.6g", v);
    out += buf;
}

void encode(std::string& out, const JsonValue& v) {
    switch (v.kind) {
        case JsonValue::Kind::Null:
            out += "null";
            break;
        case JsonValue::Kind::Bool:
            out += v.boolean ? "true" : "false";
            break;
        case JsonValue::Kind::Number:
            encodeNumber(out, v.number);
            break;
        case JsonValue::Kind::String:
            encodeString(out, v.string);
            break;
        case JsonValue::Kind::Array: {
            out.push_back('[');
            for (std::size_t i = 0; i < v.array.size(); i++) {
                if (i > 0) out.push_back(',');
                encode(out, v.array[i]);
            }
            out.push_back(']');
            break;
        }
        case JsonValue::Kind::Object: {
            out.push_back('{');
            bool first = true;
            for (const auto& entry : v.object) {
                if (!first) out.push_back(',');
                first = false;
                encodeString(out, entry.first);
                out.push_back(':');
                encode(out, entry.second);
            }
            out.push_back('}');
            break;
        }
    }
}

JsonValue makeString(const std::string& s) {
    JsonValue v;
    v.kind = JsonValue::Kind::String;
    v.string = s;
    return v;
}

JsonValue makeArray() {
    JsonValue v;
    v.kind = JsonValue::Kind::Array;
    return v;
}

} // namespace

std::string jsonToString(const JsonValue& value) {
    std::string out;
    encode(out, value);
    return out;
}

JsonValue Config::buildDefaults() {
    JsonValue root;
    root.kind = JsonValue::Kind::Object;
    for (const auto& entry : webDefaults()) {
        root.object[entry.first] = entry.second;
    }
    root.object["loadout"] = buildDefaultLoadout();
    return root;
}

JsonValue Config::buildDefaultLoadout() {
    // shared/utils/loadout.ts loadout.validate({}): outfitBase/fists/
    // heal_basic/boost_basic, the default emote wheel, and the default
    // crosshair. (The web client also fills `player_icon: ""`.)
    JsonValue loadout;
    loadout.kind = JsonValue::Kind::Object;
    loadout.object["outfit"] = makeString("outfitBase");
    loadout.object["melee"] = makeString("fists");
    loadout.object["heal"] = makeString("heal_basic");
    loadout.object["boost"] = makeString("boost_basic");
    loadout.object["player_icon"] = makeString("");

    JsonValue crosshair;
    crosshair.kind = JsonValue::Kind::Object;
    crosshair.object["type"] = makeString("crosshair_default");
    JsonValue color;
    color.kind = JsonValue::Kind::Number;
    color.number = 0xffffff;
    crosshair.object["color"] = color;
    crosshair.object["size"] = makeString("1.00");
    crosshair.object["stroke"] = makeString("0.00");
    loadout.object["crosshair"] = crosshair;

    // GameConfig.defaultEmoteLoadout (EmoteSlot has 5 slots:
    // Top/Right/Bottom/Left/Win).
    JsonValue emotes = makeArray();
    const char* def[] = { "emote_happyface", "emote_thumbsup", "emote_surviv", "emote_sadface",
                          "" };
    for (const char* e : def) {
        emotes.array.push_back(makeString(e));
    }
    loadout.object["emotes"] = emotes;
    return loadout;
}

const Config& Config::defaults() {
    static const Config c;
    return c;
}

std::string Config::defaultString(const std::string& key) {
    auto it = webDefaults().find(key);
    return it != webDefaults().end() ? it->second.asString() : "";
}

float Config::defaultFloat(const std::string& key) {
    auto it = webDefaults().find(key);
    return it != webDefaults().end() ? it->second.asFloat() : 0.0f;
}

int Config::defaultInt(const std::string& key) {
    auto it = webDefaults().find(key);
    return it != webDefaults().end() ? it->second.asInt() : 0;
}

bool Config::defaultBool(const std::string& key) {
    auto it = webDefaults().find(key);
    return it != webDefaults().end() ? it->second.asBool() : false;
}

void Config::load() {
    _root = buildDefaults();
    if (_storage) {
        const std::string blob = _storage->read();
        if (!blob.empty()) {
            JsonValue stored;
            if (JsonParser::parse(blob, stored) && stored.isObject()) {
                for (const auto& entry : stored.object) {
                    _root.object[entry.first] = entry.second;
                }
            }
        }
    }
    // Client-config upgrades (config.ts checkUpgradeConfig): the loadout is
    // always re-validated so stale blobs cannot produce an invalid selection.
    const JsonValue* loadout = getJson("loadout");
    if (!loadout || !loadout->isObject()) {
        JsonValue fresh = buildDefaultLoadout();
        _root.object["loadout"] = fresh;
        persist();
    }
}

std::string Config::getString(const std::string& key, const std::string& fallback) const {
    const JsonValue* v = _root.get(key);
    if (v && v->isString()) {
        return v->string;
    }
    if (v) {
        return v->asString();
    }
    return fallback.empty() ? defaultString(key) : fallback;
}

float Config::getFloat(const std::string& key, float fallback) const {
    const JsonValue* v = _root.get(key);
    if (v && !v->isNull()) {
        return v->asFloat(fallback);
    }
    return fallback;
}

int Config::getInt(const std::string& key, int fallback) const {
    const JsonValue* v = _root.get(key);
    if (v && !v->isNull()) {
        return v->asInt(fallback);
    }
    return fallback;
}

bool Config::getBool(const std::string& key, bool fallback) const {
    const JsonValue* v = _root.get(key);
    if (v && !v->isNull()) {
        return v->asBool(fallback);
    }
    return fallback;
}

void Config::setString(const std::string& key, const std::string& value) {
    _root.object[key] = makeString(value);
    notify(key);
}

void Config::setFloat(const std::string& key, float value) {
    JsonValue v;
    v.kind = JsonValue::Kind::Number;
    v.number = value;
    _root.object[key] = v;
    notify(key);
}

void Config::setInt(const std::string& key, int value) {
    JsonValue v;
    v.kind = JsonValue::Kind::Number;
    v.number = value;
    _root.object[key] = v;
    notify(key);
}

void Config::setBool(const std::string& key, bool value) {
    JsonValue v;
    v.kind = JsonValue::Kind::Bool;
    v.boolean = value;
    _root.object[key] = v;
    notify(key);
}

void Config::setJson(const std::string& key, const JsonValue& value) {
    _root.object[key] = value;
    notify(key);
}

const JsonValue* Config::getJson(const std::string& key) const {
    return _root.get(key);
}

void Config::notify(const std::string& key) {
    persist();
    for (auto& listener : _listeners) {
        listener(key);
    }
}

void Config::persist() {
    if (_storage) {
        _storage->write(jsonToString(_root));
    }
}

} // namespace ui
