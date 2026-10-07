#pragma once
// URL helpers for the native client's WebSocket transports.
//
// axmol's `ax::network::WebSocket` distinguishes between a socket (host+port)
// and a resource path; opening a connection therefore needs the URL split into
// those two parts. The join URLs the server hands out look like
// `ws://127.0.0.1:9000/play` while the team service is always
// `ws(s)://<host>/team_v2`.
#include <cstdint>
#include <string>

namespace surv {

struct WebSocketEndpoint {
    std::string scheme;   // "ws" | "wss"
    std::string host;     // "127.0.0.1" | "example.com"
    uint16_t port = 80;   // default derived from the scheme
    std::string path;     // "/play" (always leading slash; "/" when empty)
    bool secure = false;

    // "ws://host:port/path" (used for the Socket-based open).
    std::string socketUrl() const;
    // The resource path including any query string.
    std::string requestPath() const;
};

// Parses `ws://`, `wss://`, `http://` or `https://` URLs. Falls back to the
// given defaults for a bare "host:port/path" string. Returns false when the
// URL has no host.
bool parseWebSocketUrl(const std::string& url, WebSocketEndpoint& out);

} // namespace surv
