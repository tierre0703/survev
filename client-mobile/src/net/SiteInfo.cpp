// axmol HttpClient implementation of GET /api/site_info.
#include "SiteInfo.h"

#include "axmol.h"
#include "network/HttpClient.h"

#include "rapidjson/document.h"

#include <utility>

namespace surv {

int SiteInfo::modeIndexForTeamMode(int teamMode) const {
    for (std::size_t i = 0; i < modes.size(); i++) {
        if (modes[i].teamMode == teamMode && modes[i].enabled) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool SiteInfo::hasTeamMode(int teamMode) const {
    return modeIndexForTeamMode(teamMode) >= 0;
}

namespace {

std::string rjString(const rapidjson::Value& v, const char* key, const std::string& fallback = "") {
    auto it = v.FindMember(key);
    if (it == v.MemberEnd() || !it->value.IsString()) {
        return fallback;
    }
    return std::string(it->value.GetString(), it->value.GetStringLength());
}

SiteInfo parse(const std::string& json) {
    SiteInfo info;
    rapidjson::Document doc;
    doc.Parse(json.data(), json.size());
    if (doc.HasParseError() || !doc.IsObject()) {
        return info;
    }
    info.ok = true;
    info.country = rjString(doc, "country");
    info.gitRevision = rjString(doc, "gitRevision");
    info.clientTheme = rjString(doc, "clientTheme", "main");
    if (auto it = doc.FindMember("captchaEnabled"); it != doc.MemberEnd()) {
        info.captchaEnabled = it->value.IsBool() ? it->value.GetBool() : false;
    }
    if (auto it = doc.FindMember("modes"); it != doc.MemberEnd() && it->value.IsArray()) {
        for (const auto& m : it->value.GetArray()) {
            if (!m.IsObject()) {
                continue;
            }
            GameModeInfo mode;
            mode.mapName = rjString(m, "mapName", "main");
            if (auto tm = m.FindMember("teamMode"); tm != m.MemberEnd() && tm->value.IsInt()) {
                mode.teamMode = tm->value.GetInt();
            }
            if (auto en = m.FindMember("enabled"); en != m.MemberEnd() && en->value.IsBool()) {
                mode.enabled = en->value.GetBool();
            }
            info.modes.push_back(std::move(mode));
        }
    }
    if (auto it = doc.FindMember("pops"); it != doc.MemberEnd() && it->value.IsObject()) {
        for (const auto& entry : it->value.GetObject()) {
            if (!entry.value.IsObject()) {
                continue;
            }
            PopInfo pop;
            pop.region.assign(entry.name.GetString(), entry.name.GetStringLength());
            if (auto pc = entry.value.FindMember("playerCount");
                pc != entry.value.MemberEnd() && pc->value.IsInt()) {
                pop.playerCount = pc->value.GetInt();
            }
            pop.l10n = rjString(entry.value, "l10n");
            info.pops.push_back(std::move(pop));        }
    }
    return info;
}

} // namespace

void fetchSiteInfo(const std::string& apiBaseUrl, SiteInfoCallback cb) {
    std::string url = apiBaseUrl;
    if (!url.empty() && url.back() == '/') {
        url.pop_back();
    }
    url += "/api/site_info";

    auto* request = new ax::network::HttpRequest();
    request->setRequestType(ax::network::HttpRequest::Type::GET);
    request->setUrl(url);
    request->setCompleteCallback(
        [cb = std::move(cb)](ax::network::HttpClient*, ax::network::HttpResponse* response) mutable {
            SiteInfo info;
            if (response && response->isSucceed()) {
                const auto* data = response->getResponseData();
                const std::string json(data ? data->data() : "", data ? data->size() : 0);
                info = parse(json);
            }
            if (cb) {
                cb(std::move(info));
            }
        });
    ax::network::HttpClient::getInstance()->send(request);
    request->release();
}

} // namespace surv
