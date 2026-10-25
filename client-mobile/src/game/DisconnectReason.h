#pragma once
// Port of client/src/main.ts getErrorString() for the game WebSocket: the
// server closes with code 3000 and a reason string (shared/types/api.ts
// GameWsDisconnectReason), never with the old invented 4001-4011 codes.
#include <string>

namespace surv {

inline std::string disconnectReasonKey(const std::string& reason, bool hasJoined,
                                       bool reconnected) {
    // A late rejection after a rejoin attempt (the ticket window ran out with
    // invalid_token) is a lost connection, not a bad token.
    if (reconnected) {
        return "index-host-closed";
    }
    if (!reason.empty() && reason != "host_closed") {
        if (reason == "invalid_token") return "index-invalid-token";
        if (reason == "invalid_protocol") return "index-invalid-protocol";
        if (reason == "invalid_packet") return "index-invalid-packet";
        if (reason == "behind_proxy") return "index-behind-proxy";
        if (reason == "player_not_found") return "index-player-not-found";
        if (reason == "ip_banned") return "index-ip-banned";
        if (reason == "rate_limited") return "index-rate-limited";
        if (reason == "server_crashed") return "index-server-crashed";
        if (reason == "server_restart") return "index-server-restart";
        if (reason == "full") return "index-failed-finding-game";
    }
    if (!hasJoined) {
        return "index-failed-joining-game";
    }
    return "index-host-closed";
}

} // namespace surv
