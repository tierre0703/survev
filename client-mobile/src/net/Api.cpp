// axmol HttpClient implementation of the find_game call (client/src/api.ts +
// main.ts findGame()).
#include "Api.h"

#include "axmol.h"
#include "network/HttpClient.h"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include <utility>

namespace surv {
namespace {

using ax::network::HttpClient;
using ax::network::HttpRequest;
using ax::network::HttpResponse;

std::string buildRequestBody(const FindGameBody& body) {
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> w(sb);

    w.StartObject();
    w.Key("region");
    w.String(body.region.data(), static_cast<rapidjson::SizeType>(body.region.size()));
    w.Key("zones");
    w.StartArray();
    for (const auto& zone : body.zones) {
        w.String(zone.data(), static_cast<rapidjson::SizeType>(zone.size()));
    }
    w.EndArray();
    w.Key("version");
    w.Uint(body.version);
    w.Key("playerCount");
    w.Int(body.playerCount);
    w.Key("autoFill");
    w.Bool(body.autoFill);
    w.Key("gameModeIdx");
    w.Int(body.gameModeIdx);
    if (!body.turnstileToken.empty()) {
        w.Key("turnstileToken");
        w.String(body.turnstileToken.data(),
                 static_cast<rapidjson::SizeType>(body.turnstileToken.size()));
    }
    w.EndObject();
    return std::string(sb.GetString(), sb.GetSize());
}

FindGameResult parseResponse(const std::string& json) {
    FindGameResult result;
    rapidjson::Document doc;
    doc.Parse(json.data(), json.size());
    if (doc.HasParseError() || !doc.IsObject()) {
        result.error = "find_game_failed";
        return result;
    }

    auto typeIt = doc.FindMember("type");
    if (typeIt == doc.MemberEnd() || !typeIt->value.IsString()) {
        result.error = "find_game_failed";
        return result;
    }
    const std::string type(typeIt->value.GetString(), typeIt->value.GetStringLength());

    if (type == "success") {
        auto resIt = doc.FindMember("res");
        if (resIt == doc.MemberEnd() || !resIt->value.IsObject()) {
            result.error = "find_game_failed";
            return result;
        }
        const auto& res = resIt->value;
        auto urlsIt = res.FindMember("urls");
        if (urlsIt != res.MemberEnd() && urlsIt->value.IsArray()) {
            for (const auto& url : urlsIt->value.GetArray()) {
                if (url.IsString()) {
                    result.data.urls.emplace_back(url.GetString(), url.GetStringLength());
                }
            }
        }
        auto tokenIt = res.FindMember("joinToken");
        if (tokenIt != res.MemberEnd() && tokenIt->value.IsString()) {
            result.data.joinToken.assign(tokenIt->value.GetString(),
                                         tokenIt->value.GetStringLength());
        }
        result.ok = !result.data.urls.empty();
        if (!result.ok) {
            result.error = "find_game_failed";
        }
        return result;
    }

    if (type == "banned") {
        result.error = "banned";
        return result;
    }

    auto errIt = doc.FindMember("error");
    if (errIt != doc.MemberEnd() && errIt->value.IsString()) {
        result.error.assign(errIt->value.GetString(), errIt->value.GetStringLength());
    } else {
        result.error = "find_game_failed";
    }
    return result;
}

} // namespace

void findGame(const std::string& apiBaseUrl, const FindGameBody& body, FindGameCallback cb) {
    std::string url = apiBaseUrl;
    if (!url.empty() && url.back() == '/') {
        url.pop_back();
    }
    url += "/api/find_game_v2";

    const std::string payload = buildRequestBody(body);

    auto* request = new HttpRequest();
    request->setRequestType(HttpRequest::Type::POST);
    request->setUrl(url);
    request->setRequestData(payload.data(), payload.size());
    request->setHeaders({"Content-Type: application/json; charset=utf-8"});
    request->setCompleteCallback(
        [cb = std::move(cb)](HttpClient* /*client*/, HttpResponse* response) mutable {
            FindGameResult result;
            if (response && response->isSucceed()) {
                const auto* data = response->getResponseData();
                const std::string json(data ? data->data() : "", data ? data->size() : 0);
                result = parseResponse(json);
            } else {
                result.error = "find_game_failed";
            }
            if (cb) {
                cb(std::move(result));
            }
        });

    HttpClient::getInstance()->send(request);
    request->release();
}

} // namespace surv
