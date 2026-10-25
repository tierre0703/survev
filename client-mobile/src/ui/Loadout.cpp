#include "Loadout.h"
#include "../net/WsUrl.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>

namespace ui {
namespace {
std::string defCategory(const std::string& category) {
    if (category == "heal") return "heal_effect";
    if (category == "boost") return "boost_effect";
    if (category == "player_icon") return "emote";
    return category;
}
JsonValue stringValue(const std::string& text) {
    JsonValue v;
    v.kind = JsonValue::Kind::String;
    v.string = text;
    return v;
}
JsonValue numberValue(double value) {
    JsonValue v;
    v.kind = JsonValue::Kind::Number;
    v.number = value;
    return v;
}
// loadout.ts crosshair.color is a number, but the Java editor writes #RRGGBB.
uint32_t parseColour(const JsonValue* value, uint32_t fallback) {
    if (!value) return fallback;
    if (value->isNumber()) return static_cast<uint32_t>(value->number);
    std::string text = value->asString();
    if (!text.empty() && text[0] == '#') text.erase(0, 1);
    if (text.size() != 6 && text.size() != 8) return fallback;
    try {
        return static_cast<uint32_t>(std::stoul(text, nullptr, 16));
    } catch (...) {
        return fallback;
    }
}
std::string formatFloat(float value) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.2f", value);
    return buffer;
}
} // namespace
std::string sanitizePlayerName(const std::string& input) {
    std::string out;
    for (size_t i = 0; i < input.size();) {
        const auto c = static_cast<unsigned char>(input[i]);
        if (c < 32 || c == 127) { ++i; continue; }
        size_t length = c < 128 ? 1 : c >= 0xc2 && c <= 0xdf ? 2
            : c >= 0xe0 && c <= 0xef ? 3 : c >= 0xf0 && c <= 0xf4 ? 4 : 0;
        if (!length || i + length > input.size()) { ++i; continue; }
        bool valid = true;
        for (size_t j = 1; j < length; ++j) {
            valid = valid && (static_cast<unsigned char>(input[i + j]) & 0xc0) == 0x80;
        }
        if (!valid) { ++i; continue; }
        out.append(input, i, length);
        i += length;
    }
    while (!out.empty() && out.front() == ' ') out.erase(out.begin());
    while (!out.empty() && out.back() == ' ') out.pop_back();
    // The wire string is byte-limited; never split a UTF-8 codepoint.
    size_t end = std::min(out.size(), static_cast<size_t>(surv::Constants::PlayerNameMaxLen));
    if (end < out.size()) {
        while (end && (static_cast<unsigned char>(out[end]) & 0xc0) == 0x80) --end;
    }
    out.resize(end);
    return out;
}

std::string teamEndpoint(const std::string& apiUrl) {
    std::string url = apiUrl;
    if (url.rfind("https://", 0) == 0) url.replace(0, 8, "wss://");
    else if (url.rfind("http://", 0) == 0) url.replace(0, 7, "ws://");
    surv::WebSocketEndpoint endpoint;
    if (!surv::parseWebSocketUrl(url, endpoint)) return "";
    const auto path = url.find('/', url.find("://") + 3);
    return url.substr(0, path) + "/team_v2";
}

std::string teamInviteCode(const std::string& input) {
    std::string code = input;
    const auto hash = code.find('#');
    if (hash != std::string::npos) code = code.substr(hash + 1);
    while (!code.empty() && std::isspace(static_cast<unsigned char>(code.front()))) code.erase(code.begin());
    while (!code.empty() && std::isspace(static_cast<unsigned char>(code.back()))) code.pop_back();
    return code;
}

std::vector<std::string> Loadout::items(const std::string& category) const {
    std::vector<std::string> candidates = _accountItems;
    // Account items are persisted by the Java profile bridge (account.items);
    // the config is the source of truth so the per-call Loadout instances the
    // bridge creates all see the same owned set.
    if (_config) {
        if (const auto* stored = _config->getJson("accountItems");
            stored && stored->isArray()) {
            for (const auto& value : stored->array) {
                if (value.isString() && !value.string.empty()) candidates.push_back(value.string);
            }
        }
    }
    const auto* provider = surv::getDefProvider();
    if (!provider) return {};
    if (const auto* unlock = provider->gameObject("unlock_default")) {
        candidates.insert(candidates.begin(), unlock->unlocks.begin(), unlock->unlocks.end());
    }
    std::vector<std::string> result;
    for (const auto& type : candidates) {
        const auto* def = provider->gameObject(type);
        if (def && def->category == defCategory(category)
            && std::find(result.begin(), result.end(), type) == result.end()) result.push_back(type);
    }
    return result;
}

JsonValue Loadout::validated() const {
    JsonValue result = Config::defaultLoadout();
    const auto* stored = _config->getJson("loadout");
    if (!stored || !stored->isObject()) return result;
    for (const auto* category : {"outfit", "melee", "heal", "boost", "player_icon"}) {
        const auto value = stored->getString(category);
        const auto available = items(category);
        if (std::find(available.begin(), available.end(), value) != available.end()) {
            result.object[category] = stringValue(value);
        }
    }
    const auto available = items("emote");
    if (const auto* emotes = stored->get("emotes")) {
        auto& slots = result.object["emotes"].array;
        for (size_t i = 0; i < slots.size() && i < emotes->size(); ++i) {
            const auto type = emotes->at(i)->asString();
            if (std::find(available.begin(), available.end(), type) != available.end()) slots[i] = stringValue(type);
        }
    }

    // crosshair: nested object (loadout.ts shape). Keep the default when a
    // field is absent or invalid.
    JsonValue& crosshair = result.object["crosshair"];
    const auto* storedCrosshair = stored->get("crosshair");    if (storedCrosshair && storedCrosshair->isObject()) {
        const auto type = storedCrosshair->getString("type", crosshair.getString("type"));
        const auto* provider = surv::getDefProvider();
        const auto* def = provider ? provider->gameObject(type) : nullptr;
        if (def && def->category == "crosshair") {
            crosshair.object["type"] = stringValue(type);
        }
        crosshair.object["color"] = numberValue(
            parseColour(storedCrosshair->get("color"),
                        static_cast<uint32_t>(crosshair.getFloat("color", 0xffffff))));
        const float size = std::clamp(storedCrosshair->getFloat("size", 1.0f), 0.25f, 4.0f);
        const float stroke = std::clamp(storedCrosshair->getFloat("stroke", 0.0f), 0.0f, 8.0f);
        crosshair.object["size"] = stringValue(formatFloat(size));
        crosshair.object["stroke"] = stringValue(formatFloat(stroke));
    }
    // Legacy Java editor shape: {"crosshair":"custom","crosshairColor":"#...",
    // "crosshairSize":0..2,"crosshairStroke":0..2}. Migrate it so existing
    // installs keep their settings (and their loadout, which the old bridge
    // replaced wholesale). A nested object always wins over stale flat keys.
    const bool hasNestedCrosshair = storedCrosshair && storedCrosshair->isObject();
    if (!hasNestedCrosshair && (stored->get("crosshairColor") ||
                                stored->get("crosshairSize") ||
                                stored->get("crosshairStroke"))) {
        crosshair.object["color"] = numberValue(
            parseColour(stored->get("crosshairColor"), 0xffffff));
        static const float kSizes[] = {0.7f, 1.0f, 1.4f};
        static const float kStrokes[] = {0.0f, 1.0f, 2.0f};
        const int sizeIdx = std::clamp(stored->getInt("crosshairSize", 1), 0, 2);
        const int strokeIdx = std::clamp(stored->getInt("crosshairStroke", 0), 0, 2);
        crosshair.object["size"] = stringValue(formatFloat(kSizes[sizeIdx]));
        crosshair.object["stroke"] = stringValue(formatFloat(kStrokes[strokeIdx]));
    }
    return result;
}

surv::Game::JoinInfo Loadout::joinInfo() const {
    const auto value = validated();
    surv::Game::JoinInfo info;
    info.name = sanitizePlayerName(_config->getString("playerName"));
    if (info.name.empty()) info.name = "Player";
    info.outfit = value.getString("outfit");
    info.melee = value.getString("melee");
    info.heal = value.getString("heal");
    info.boost = value.getString("boost");
    for (const auto& slot : value.get("emotes")->array) info.emotes.push_back(slot.asString());
    if (const auto* crosshair = value.get("crosshair")) {
        info.crosshairType = crosshair->getString("type", info.crosshairType);
        info.crosshairColor = parseColour(crosshair->get("color"), info.crosshairColor);
        info.crosshairSize = std::clamp(crosshair->getFloat("size", 1.0f), 0.25f, 4.0f);
        info.crosshairStroke = std::clamp(crosshair->getFloat("stroke", 0.0f), 0.0f, 8.0f);
    }
    return info;
}

bool Loadout::select(const std::string& category, const std::string& type, size_t emoteSlot) {
    const auto available = items(category);
    if (std::find(available.begin(), available.end(), type) == available.end()
        && !(category == "player_icon" && type.empty())) return false;
    auto value = validated();
    if (category == "emote") {
        auto& slots = value.object["emotes"].array;
        if (emoteSlot >= slots.size()) return false;
        slots[emoteSlot] = stringValue(type);
    } else {
        value.object[category] = stringValue(type);
    }
    _config->setJson("loadout", value);
    return true;
}
} // namespace ui
