#include "ErrorLog.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <utility>

namespace surv {

namespace {

double steadyNowMs() {
    using namespace std::chrono;
    return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
}

// errorLogs.ts: `requests > 5` starts the throttle.
constexpr int kMaxRequests = 5;
constexpr double kThrottleMs = 180.0 * 1000.0;
// errorLogs.ts: `Math.random() <= 0.01`.
constexpr double kSampleRate = 0.01;
constexpr int kMaxWindowErrors = 2;

} // namespace

std::string reportErrorUrl(const std::string& apiBaseUrl) {
    std::string url = apiBaseUrl;
    if (!url.empty() && url.back() == '/') {
        url.pop_back();
    }
    url += "/api/report_error";
    return url;
}

std::string ErrorLog::escape(const std::string& text) {
    std::string out;
    out.reserve(text.size() + 2);
    for (char c : text) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[7];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned>(c));
                    out += buf;
                } else {
                    out += c;
                }
                break;
        }
    }
    return out;
}

std::string ErrorLog::makeBody(const std::string& loc, const std::string& dataJson) {
    // errorLogs.ts: JSON.stringify({ loc, data }); `logError(error)` with no
    // data lets JSON.stringify drop the key, so an empty dataJson is omitted.
    std::string body = "{\"loc\":\"";
    body += escape(loc);
    body += '"';
    if (!dataJson.empty()) {
        body += ",\"data\":";
        body += dataJson;
    }
    body += '}';
    return body;
}

ErrorLog::ErrorLog(Poster poster, std::string apiBaseUrl, Clock clock, Random random)
    : _poster(std::move(poster)),
      _url(reportErrorUrl(apiBaseUrl)),
      _clock(clock ? std::move(clock) : Clock(steadyNowMs)),
      _random(random ? std::move(random) : Random([] {
          // Uniform [0,1); the engine-free tests always inject a value.
          return static_cast<double>(std::rand()) / (static_cast<double>(RAND_MAX) + 1.0);
      })) {}

void ErrorLog::tick() {
    const double now = _clock();
    if (_throttle) {
        if (now > _throttleTimeout) {
            _throttle = false;
            _requests = 0;
        }
    } else {
        _requests = std::max(_requests - 1, 0);
    }
}

bool ErrorLog::sample() {
    return _random() <= kSampleRate;
}

void ErrorLog::store(const std::string& loc, const std::string& dataJson) {
    ++_requests;
    if (_requests > kMaxRequests) {
        _throttleTimeout = _clock() + kThrottleMs;
        _throttle = true;
        return;
    }
    const std::string body = makeBody(loc, dataJson);
    _lastBody = body;
    ++_reportsSent;
    if (_poster) {
        _poster(_url, body);
    }
}

void ErrorLog::storeGeneric(const std::string& parent, const std::string& child) {
    // errorLogs.ts: account errors only happen when a request fails.
    if (parent == "account") {
        return;
    }
    if (!sample()) {
        return;
    }
    std::string data = "{\"parent\":\"";
    data += escape(parent);
    data += "\",\"child\":\"";
    data += escape(child);
    data += "\"}";
    store("storeGeneric", data);
}

void ErrorLog::logWindowOnError(const std::string& errorJson) {
    if (_errorLogCount >= kMaxWindowErrors) {
        return;
    }
    ++_errorLogCount;
    store("windowOnError", errorJson);
}

void ErrorLog::logError(const std::string& error, const std::string& dataJson) {
    std::string data = "{\"error\":\"";
    data += escape(error);
    data += '"';
    if (!dataJson.empty()) {
        data += ",\"data\":";
        data += dataJson;
    }
    data += '}';
    store("errorLog", data);
}

} // namespace surv
