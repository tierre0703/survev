#include "WsUrl.h"

namespace surv {

std::string WebSocketEndpoint::socketUrl() const {
    std::string s = secure ? "wss://" : "ws://";
    s += host;
    if (port != 0) {
        s += ":" + std::to_string(port);
    }
    return s;
}

std::string WebSocketEndpoint::requestPath() const {
    return path.empty() ? std::string("/") : path;
}

bool parseWebSocketUrl(const std::string& url, WebSocketEndpoint& out) {
    if (url.empty()) {
        return false;
    }
    std::string rest = url;

    // Scheme
    const std::size_t schemeEnd = rest.find("://");
    if (schemeEnd != std::string::npos) {
        out.scheme = rest.substr(0, schemeEnd);
        rest = rest.substr(schemeEnd + 3);
    } else {
        out.scheme = "ws";
    }
    out.secure = out.scheme == "wss" || out.scheme == "https";
    out.port = out.secure ? 443 : 80;

    // Authority (up to the first '/', '?' or '#')
    std::size_t authorityEnd = rest.size();
    for (std::size_t i = 0; i < rest.size(); i++) {
        if (rest[i] == '/' || rest[i] == '?' || rest[i] == '#') {
            authorityEnd = i;
            break;
        }
    }
    std::string authority = rest.substr(0, authorityEnd);
    std::string tail = rest.substr(authorityEnd);

    // Userinfo (ignored, but skip it)
    const std::size_t at = authority.find('@');
    if (at != std::string::npos) {
        authority = authority.substr(at + 1);
    }

    // Host / port (handle bracketed IPv6).
    if (!authority.empty() && authority[0] == '[') {
        const std::size_t close = authority.find(']');
        if (close == std::string::npos) {
            return false;
        }
        out.host = authority.substr(1, close - 1);
        if (close + 1 < authority.size() && authority[close + 1] == ':') {
            out.port = static_cast<uint16_t>(std::stoi(authority.substr(close + 2)));
        }
    } else {
        const std::size_t colon = authority.rfind(':');
        if (colon != std::string::npos) {
            out.host = authority.substr(0, colon);
            try {
                out.port = static_cast<uint16_t>(std::stoi(authority.substr(colon + 1)));
            } catch (...) {
                return false;
            }
        } else {
            out.host = authority;
        }
    }
    if (out.host.empty()) {
        return false;
    }

    out.path = tail.empty() ? "/" : tail;
    return true;
}

} // namespace surv
