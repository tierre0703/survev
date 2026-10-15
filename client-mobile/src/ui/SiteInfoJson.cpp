#include "SiteInfoJson.h"
#include "Config.h" // jsonToString (the shared encoder)

namespace ui {

namespace {

JsonValue makeString(const std::string& text) {
    JsonValue value;
    value.kind = JsonValue::Kind::String;
    value.string = text;
    return value;
}

JsonValue makeBool(bool b) {
    JsonValue value;
    value.kind = JsonValue::Kind::Bool;
    value.boolean = b;
    return value;
}

JsonValue makeInt(int i) {
    JsonValue value;
    value.kind = JsonValue::Kind::Number;
    value.number = static_cast<double>(i);
    return value;
}

} // namespace

std::string siteInfoToJson(const surv::SiteInfo& info) {
    JsonValue root;
    root.kind = JsonValue::Kind::Object;
    root.object["ok"] = makeBool(info.ok);
    root.object["country"] = makeString(info.country);
    root.object["gitRevision"] = makeString(info.gitRevision);
    root.object["captchaEnabled"] = makeBool(info.captchaEnabled);
    root.object["clientTheme"] = makeString(info.clientTheme);

    JsonValue modes;
    modes.kind = JsonValue::Kind::Array;
    modes.array.reserve(info.modes.size());
    for (const auto& mode : info.modes) {
        JsonValue item;
        item.kind = JsonValue::Kind::Object;
        item.object["mapName"] = makeString(mode.mapName);
        item.object["teamMode"] = makeInt(mode.teamMode);
        item.object["enabled"] = makeBool(mode.enabled);
        modes.array.push_back(std::move(item));
    }
    root.object["modes"] = std::move(modes);

    JsonValue pops;
    pops.kind = JsonValue::Kind::Array;
    pops.array.reserve(info.pops.size());
    for (const auto& pop : info.pops) {
        JsonValue item;
        item.kind = JsonValue::Kind::Object;
        item.object["region"] = makeString(pop.region);
        item.object["playerCount"] = makeInt(pop.playerCount);
        item.object["l10n"] = makeString(pop.l10n);
        pops.array.push_back(std::move(item));
    }
    root.object["pops"] = std::move(pops);

    return jsonToString(root);
}

} // namespace ui
