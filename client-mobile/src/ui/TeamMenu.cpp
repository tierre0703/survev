#include "TeamMenu.h"
#include "../net/Net.h"
#include "Config.h"
#include "Json.h"

#include <utility>

namespace ui {

namespace {

std::string jsonString(const std::string& s) {
    std::string out = "\"";
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
    out += "\"";
    return out;
}

std::string roomDataJson(const RoomData& room) {
    return "{\"roomUrl\":" + jsonString(room.roomUrl)
           + ",\"region\":" + jsonString(room.region)
           + ",\"gameModeIdx\":" + std::to_string(room.gameModeIdx)
           + ",\"autoFill\":" + (room.autoFill ? "true" : "false")
           + ",\"findingGame\":" + (room.findingGame ? "true" : "false")
           + ",\"lastError\":null}";
}

} // namespace

TeamErrorType teamErrorFromString(const std::string& type) {
    if (type == "join_full") return TeamErrorType::JoinFull;
    if (type == "join_not_found") return TeamErrorType::JoinNotFound;
    if (type == "create_failed") return TeamErrorType::CreateFailed;
    if (type == "join_failed") return TeamErrorType::JoinFailed;
    if (type == "join_game_failed") return TeamErrorType::JoinGameFailed;
    if (type == "lost_conn") return TeamErrorType::LostConn;
    if (type == "find_game_error") return TeamErrorType::FindGameError;
    if (type == "find_game_full") return TeamErrorType::FindGameFull;
    if (type == "find_game_invalid_protocol") return TeamErrorType::FindGameInvalidProtocol;
    if (type == "find_game_invalid_captcha") return TeamErrorType::FindGameInvalidCaptcha;
    if (type == "kicked") return TeamErrorType::Kicked;
    if (type == "banned") return TeamErrorType::Banned;
    if (type == "behind_proxy") return TeamErrorType::BehindProxy;
    if (type == "rate_limited") return TeamErrorType::RateLimited;
    return TeamErrorType::Unknown;
}

const char* teamErrorL10n(TeamErrorType type) {
    switch (type) {
        case TeamErrorType::JoinFull: return "index-team-is-full";
        case TeamErrorType::JoinNotFound: return "index-failed-joining-team";
        case TeamErrorType::CreateFailed: return "index-failed-creating-team";
        case TeamErrorType::JoinFailed: return "index-failed-joining-team";
        case TeamErrorType::JoinGameFailed: return "index-failed-joining-game";
        case TeamErrorType::LostConn: return "index-lost-connection";
        case TeamErrorType::FindGameError: return "index-failed-finding-game";
        case TeamErrorType::FindGameFull: return "index-failed-finding-game";
        case TeamErrorType::FindGameInvalidProtocol: return "index-invalid-protocol";
        case TeamErrorType::FindGameInvalidCaptcha: return "index-invalid-captcha";
        case TeamErrorType::Kicked: return "index-team-kicked";
        case TeamErrorType::Banned: return "index-ip-banned";
        case TeamErrorType::BehindProxy: return "behind_proxy";
        case TeamErrorType::RateLimited: return "index-rate-limited";
        case TeamErrorType::Unknown: break;
    }
    return "index-lost-connection";
}

TeamMenu::~TeamMenu() {
    leave();
}

void TeamMenu::connect(bool create, const std::string& roomUrl) {
    leave();

    _active = true;
    _create = create;
    _joined = false;
    _joiningGame = false;
    _isLeader = create;
    _players.clear();
    _gameError.clear();

    _roomData.roomUrl = roomUrl;
    _roomData.findingGame = false;

    if (!_factory) {
        fail(create ? TeamErrorType::CreateFailed : TeamErrorType::JoinFailed);
        return;
    }
    // The room service lives on the API host at /team_v2 (the app resolves the
    // host, matching api.resolveRoomHost()).
    _connection = _factory(_teamUrl);
    if (!_connection) {
        fail(create ? TeamErrorType::CreateFailed : TeamErrorType::JoinFailed);
        return;
    }
    _connection->onOpen = [this] { onSocketOpen(); };
    _connection->onMessage = [this](std::vector<uint8_t>&& frame) {
        std::string text;
        if (!frame.empty()) {
            text.assign(reinterpret_cast<const char*>(frame.data()), frame.size());
        } else {
            text = "{}";
        }
        handleMessage(text);
    };
    _connection->onError = [this] {
        if (_connection) {
            _connection->close();
        }
    };
    _connection->onClose = [this](uint16_t, const std::string&) { onSocketClose(); };
}

void TeamMenu::leave() {
    if (_connection) {
        _connection->resetAndClose();
        _connection.reset();
    }
    _active = false;
    _joined = false;
    _joiningGame = false;
    _isLeader = false;
    _localPlayerId = 0;
    _players.clear();
}

void TeamMenu::update(float dt) {
    if (_connection) {
        _connection->pump();
    }
    if (!_active || !_joined) {
        return;
    }
    // teamMenu.ts pings the room every 10s so the server keeps the seat.
    _keepAliveTicker += dt;
    if (_keepAliveTicker >= 10.0f) {
        _keepAliveTicker = 0.0f;
        sendMessage("keepAlive");
    }
}

void TeamMenu::onSocketOpen() {
    if (_create) {
        sendJson("{\"type\":\"create\",\"data\":{\"roomData\":" + roomDataJson(_roomData)
                 + ",\"playerData\":{\"name\":" + jsonString(_playerName) + "}}}");
    } else {
        sendJson("{\"type\":\"join\",\"data\":{\"roomUrl\":" + jsonString(_roomData.roomUrl)
                 + ",\"playerData\":{\"name\":" + jsonString(_playerName) + "}}}");
    }
}

void TeamMenu::onSocketClose() {
    TeamErrorType err = TeamErrorType::Unknown;
    bool haveError = false;
    if (!_joiningGame) {
        err = _joined ? TeamErrorType::LostConn
                      : (_create ? TeamErrorType::CreateFailed : TeamErrorType::JoinFailed);
        haveError = true;
    }
    const bool wasActive = _active;
    leave();
    if (wasActive) {
        if (haveError) {
            if (onError) {
                onError(err, "");
            }
        } else if (onLostConnection) {
            onLostConnection();
        }
    }
}

void TeamMenu::fail(TeamErrorType type) {
    leave();
    if (onError) {
        onError(type, "");
    }
}

void TeamMenu::sendJson(const std::string& json) {
    if (_connection && _connection->state() == ConnectionState::Open) {
        _connection->send(reinterpret_cast<const uint8_t*>(json.data()), json.size());
    } else if (_connection) {
        _connection->close();
    }
}

void TeamMenu::sendMessage(const std::string& type) {
    sendJson("{\"type\":" + jsonString(type) + "}");
}

void TeamMenu::sendMessage(const std::string& type, const std::string& dataJson) {
    sendJson("{\"type\":" + jsonString(type) + ",\"data\":" + dataJson + "}");
}

void TeamMenu::setRoomRegion(const std::string& region) {
    if (!_isLeader || _roomData.region == region) {
        return;
    }
    _roomData.region = region;
    sendMessage("setRoomProps", roomDataJson(_roomData));
}

void TeamMenu::setRoomAutoFill(bool autoFill) {
    if (!_isLeader || _roomData.autoFill == autoFill) {
        return;
    }
    _roomData.autoFill = autoFill;
    sendMessage("setRoomProps", roomDataJson(_roomData));
}

void TeamMenu::setRoomGameMode(int gameModeIdx) {
    if (!_isLeader || _roomData.gameModeIdx == gameModeIdx) {
        return;
    }
    _roomData.gameModeIdx = gameModeIdx;
    sendMessage("setRoomProps", roomDataJson(_roomData));
}

void TeamMenu::tryStartGame() {
    if (!_isLeader || _roomData.findingGame) {
        return;
    }
    _roomData.findingGame = true;
    _gameError.clear();
    std::string turnstile;
    if (_turnstileResolver) {
        turnstile = _turnstileResolver(_roomData.region);
    }
    std::string data = "{\"region\":" + jsonString(_roomData.region) + ",\"version\":"
                       + std::to_string(surv::defs::kProtocolVersion);
    if (!turnstile.empty()) {
        data += ",\"turnstileToken\":" + jsonString(turnstile);
    }
    data += "}";
    sendMessage("playGame", data);
    if (onRoomChanged) {
        onRoomChanged();
    }
}

void TeamMenu::onGameComplete(const std::string& errorMessage) {
    if (!_active) {
        return;
    }
    _joiningGame = false;
    _gameError = errorMessage;
    sendMessage("gameComplete");
    if (onRoomChanged) {
        onRoomChanged();
    }
}

void TeamMenu::applyState(const std::string& dataJson) {
    JsonValue data;
    if (!JsonParser::parse(dataJson, data) || !data.isObject()) {
        return;
    }
    _joined = true;

    const RoomData own = _roomData;
    if (const JsonValue* room = data.get("room")) {
        _roomData.roomUrl = room->getString("roomUrl", _roomData.roomUrl);
        _roomData.region = room->getString("region", own.region);
        _roomData.gameModeIdx = room->getInt("gameModeIdx", own.gameModeIdx);
        _roomData.autoFill = room->getBool("autoFill", own.autoFill);
        _roomData.findingGame = room->getBool("findingGame", false);
        _roomData.maxPlayers = room->getInt("maxPlayers", 0);
        _roomData.captchaEnabled = room->getBool("captchaEnabled", false);
        if (const JsonValue* modes = room->get("enabledGameModeIdxs")) {
            _roomData.enabledGameModeIdxs.clear();
            for (std::size_t i = 0; i < modes->size(); i++) {
                if (const JsonValue* v = modes->at(i)) {
                    _roomData.enabledGameModeIdxs.push_back(v->asInt());
                }
            }
        }
    }

    _players.clear();
    if (const JsonValue* players = data.get("players")) {
        for (std::size_t i = 0; i < players->size(); i++) {
            const JsonValue* p = players->at(i);
            if (!p) {
                continue;
            }
            TeamPlayer tp;
            tp.playerId = static_cast<uint16_t>(p->getInt("playerId", 0));
            tp.isLeader = p->getBool("isLeader", false);
            tp.inGame = p->getBool("inGame", false);
            tp.name = p->getString("name", "");
            _players.push_back(tp);
        }
    }
    _localPlayerId = static_cast<uint16_t>(data.getInt("localPlayerId", _localPlayerId));

    // The leader keeps its local room properties (teamMenu.ts overrides the
    // server state with the client's own values for region/autoFill).
    for (const auto& p : _players) {
        if (p.playerId == _localPlayerId) {
            _isLeader = p.isLeader;
            break;
        }
    }
    if (_isLeader) {
        _roomData.region = own.region;
        _roomData.autoFill = own.autoFill;
    }
    if (onRoomChanged) {
        onRoomChanged();
    }
}

void TeamMenu::handleMessage(const std::string& json) {
    JsonValue msg;
    if (!JsonParser::parse(json, msg) || !msg.isObject()) {
        return;
    }
    const std::string type = msg.getString("type");
    if (type == "state") {
        const JsonValue* data = msg.get("data");
        applyState(data ? jsonToString(*data) : std::string("{}"));
        return;
    }
    if (type == "joinGame") {
        _joiningGame = true;
        MatchData match;
        if (const JsonValue* data = msg.get("data")) {
            if (const JsonValue* urls = data->get("urls")) {
                for (std::size_t i = 0; i < urls->size(); i++) {
                    if (const JsonValue* u = urls->at(i)) {
                        match.urls.push_back(u->asString());
                    }
                }
            }
            match.joinToken = data->getString("joinToken", "");
        }
        if (onPlay) {
            onPlay(match);
        }
        return;
    }
    if (type == "keepAlive") {
        return;
    }
    if (type == "kicked") {
        fail(TeamErrorType::Kicked);
        return;
    }
    if (type == "error") {
        std::string errType = "lost_conn";
        if (const JsonValue* data = msg.get("data")) {
            errType = data->getString("type", errType);
        }
        fail(teamErrorFromString(errType));
    }
}

} // namespace ui
