#pragma once
// axmol WebSocket adapter (shared/net/connection.ts WebsocketConnection).
//
// The declaration is deliberately axmol-free so host builds / tests can include
// it without the engine; WebSocketConnection.cpp is the only translation unit
// that pulls in axmol and yasio.
#include "Connection.h"
#include <memory>
#include <string>

namespace surv {

// Creates a binary WebSocket connection to `url` (e.g. "ws://10.0.2.2:9000/play").
// Returns nullptr if the URL cannot be opened.
std::unique_ptr<Connection> createWebSocketConnection(const std::string& url);

// Same transport, named for the M7 UI: used for the game socket (`/play`) and
// the team socket (`/team_v2`). `url` is the full ws(s):// URL.
std::unique_ptr<Connection> createWebSocketConnectionTo(const std::string& url);

} // namespace surv
