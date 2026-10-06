#include "Game.h"
#include "../net/Net.h"

#include <algorithm>
#include <cstdio>
#include <sstream>
#include <unordered_map>
#include <utility>

namespace surv {

Game::Game() = default;
Game::~Game() = default;

void Game::init(GameScene* scene, ConnectionFactory factory) {
    _scene = scene;
    _factory = std::move(factory);
}

void Game::free() {
    if (_connection) {
        _connection->resetAndClose();
        _connection.reset();
    }
    _connecting = false;
    _connected = false;
    _playing = false;
    _paused = false;

    _activePlayerId = 0;
    _localPlayerId = 0;
    _teamMode = 0;
    _started = false;
    _emotes.clear();
    _inputSeq = 0;
    _inputSeqInFlight = false;
    _playersById.clear();
    _objectsById.clear();
    _lastUpdate = UpdateMsg{};
    _hasUpdate = false;
    _map = MapMsg{};
    _hasMap = false;
    _closeCode = 0;
    _closeReason.clear();
}

void Game::tryJoinGame(const std::string& url, const std::string& joinToken) {
    if (_connecting || _connected) {
        return;
    }
    if (_connection) {
        _connection->resetAndClose();
        _connection.reset();
    }
    if (!_factory) {
        return;  // no transport configured (headless test without a fake)
    }

    _lastUrl = url;
    _lastToken = joinToken;
    _connecting = true;
    _connected = false;
    _closeCode = 0;
    _closeReason.clear();

    _connection = _factory(url);
    if (!_connection) {
        _connecting = false;
        return;
    }

    // Mirror connection.ts: onError closes, onOpen sends the JoinMsg, and each
    // binary frame is split into protocol messages on the main thread.
    _connection->onOpen = [this] { onOpen(); };
    _connection->onMessage = [this](std::vector<uint8_t>&& frame) { onFrame(std::move(frame)); };
    _connection->onError = [this] {
        if (_connection) {
            _connection->close();
        }
    };
    _connection->onClose = [this](uint16_t code, const std::string& reason) { onClose(code, reason); };
}

void Game::pause() {
    if (_paused) {
        return;
    }
    _paused = true;
    _playing = false;
    if (_connection) {
        // connection.ts close(reason) -> ws.close(3000, reason)
        _connection->close("background");
    }
    // The connection may still be mid-handshake; make the next resume eligible.
    _connecting = false;
    _connected = false;
}

void Game::resume() {
    if (!_paused) {
        return;
    }
    _paused = false;
    if (!_lastUrl.empty()) {
        // Rejoin: the server replays Map/Joined/Update so state re-syncs.
        tryJoinGame(_lastUrl, _lastToken);
    }
}

void Game::update(float dt) {
    (void)dt;
    // Pump queued WebSocket events/frames on the main thread, exactly like the
    // browser fires WebSocket events on the main thread. onOpen/onMessage/
    // onClose/onError are invoked from here, in arrival order.
    if (_connection) {
        _connection->pump();
    }
}

void Game::sendMessage(MsgType type, Msg& msg, size_t maxLen) {
    if (!_connection || _connection->state() != ConnectionState::Open) {
        return;
    }
    // Mirrors game.ts m_sendMessage(): MsgStream(maxLen) -> serializeMsg -> send.
    MsgStream stream(std::vector<uint8_t>(maxLen, 0));
    stream.serializeMsg(type, [&msg](NetBitStream& s) { msg.serialize(s); });
    const std::vector<uint8_t> buf = stream.getBuffer();
    if (!buf.empty()) {
        _connection->send(buf.data(), buf.size());
    }
}

void Game::sendInput(InputMsg& msg) {
    if (!isConnected()) {
        return;
    }
    // Mirrors game.ts: only advance the sequence once the previous input has
    // been acked by the server (UpdateMsg.ack).
    if (!_inputSeqInFlight) {
        _inputSeq = (_inputSeq + 1) % 256;
        _inputSeqInFlight = true;
    }
    msg.seq = static_cast<uint8_t>(_inputSeq);
    sendMessage(MsgType_Input, msg, 128);
}

void Game::onOpen() {
    _connecting = false;
    _connected = true;
    sendJoinMessage();
}

void Game::sendJoinMessage() {
    // Mirrors game.ts onOpen(): build the JoinMsg from config/device.
    JoinMsg msg;
    msg.protocol = defs::kProtocolVersion;
    msg.joinToken = _lastToken;
    msg.name = _playerName;  // TODO(M7): from UI/config
    msg.useTouch = true;     // mobile build always uses touch
    msg.isMobile = true;
    msg.bot = false;
    msg.outfit = "outfitBase";
    msg.melee = "fists";
    msg.heal = "bandage";
    msg.boost = "soda";
    msg.emotes = _emotes;
    sendMessage(MsgType_Join, msg, 8192);
}

void Game::onFrame(std::vector<uint8_t>&& frame) {
    if (frame.empty()) {
        return;
    }
    // Mirrors connection.ts onMessage: parse every message in the frame.
    MsgStream stream(std::move(frame));
    while (true) {
        const uint8_t type = stream.deserializeMsgType();
        if (type == MsgType_None) {
            break;
        }
        onServerMessage(type, stream.getStream());
        stream.getStream().readAlignToNextByte();
    }
}

void Game::onClose(uint16_t code, const std::string& reason) {
    _closeCode = code;
    _closeReason = reason;
    _connecting = false;
    _connected = false;
    _playing = false;
    _inputSeqInFlight = false;
    // TODO(M7): if this was not an intentional pause/quit, surface `reason`
    // (GameWsDisconnectReason) and retry another URL like main.ts joinGame().
}

void Game::onServerMessage(uint8_t type, NetBitStream& s) {
    switch (type) {
        case MsgType_Joined:
            handleJoined(s);
            break;
        case MsgType_Update:
            handleUpdate(s);
            break;
        case MsgType_Kill:
            handleKill(s);
            break;
        case MsgType_GameOver:
            handleGameOver(s);
            break;
        case MsgType_Pickup:
            handlePickup(s);
            break;
        case MsgType_Map:
            handleMap(s);
            break;
        case MsgType_UpdatePass:
            break; // no-op, like UpdatePassMsg
        default:
            break;
    }
}

void Game::handleJoined(NetBitStream& s) {
    JoinedMsg m;
    m.deserialize(s);
    _teamMode = m.teamMode;
    _localPlayerId = m.playerId;
    _started = m.started;
    _emotes = std::move(m.emotes);
    // TODO(M7): onJoin() UI transition + emote wheel + waiting-for-players.
}

void Game::handleUpdate(NetBitStream& s) {
    UpdateMsg m;
    m.deserialize(s);
    // Latency/ack tracking (mirrors game.ts m_processGameUpdate): the server
    // echoes the last input seq it consumed.
    if (_inputSeqInFlight && m.ack == static_cast<uint8_t>(_inputSeq)) {
        _inputSeqInFlight = false;
    }
    if (m.activePlayerIdDirty) {
        _activePlayerId = m.activePlayerId;
    }
    _playing = true;

    // Mirror m_processGameUpdate(): delete objects, apply full then partial
    // objects, and maintain the player-info table.
    for (uint16_t id : m.delObjIds) {
        _objectsById.erase(id);
    }
    for (const auto& obj : m.fullObjects) {
        _objectsById[obj.data.__id] = {obj.data.__id, obj.data.__type, obj.data.pos};
    }
    for (const auto& obj : m.partObjects) {
        auto it = _objectsById.find(obj.data.__id);
        const uint8_t clientType = it != _objectsById.end() ? it->second.type : 0;
        if (obj.data.__type != clientType) {
            continue;  // web logs a type mismatch and skips the update
        }
        it->second = {obj.data.__id, obj.data.__type, obj.data.pos};
    }
    for (const auto& info : m.playerInfos) {
        _playersById[info.playerId] = info;
    }
    for (uint16_t id : m.deletedPlayerIds) {
        _playersById.erase(id);
    }

    // Keep the full tick (bullets/gas/...) for the M4 renderer.
    _activePlayer = m.activePlayerData;
    _lastUpdate = std::move(m);
    _hasUpdate = true;
    // TODO(M4+): interpolate objects with _lastUpdate.
}

void Game::handleKill(NetBitStream& s) {
    KillMsg m;
    m.deserialize(s);
    // TODO(M7): killfeed UI + death screen.
    (void)m;
}

void Game::handleGameOver(NetBitStream& s) {
    GameOverMsg m;
    m.deserialize(s);
    _playing = false;
    // TODO(M7): stats screen.
    (void)m;
}

void Game::handlePickup(NetBitStream& s) {
    PickupMsg m;
    m.deserialize(s);
    // TODO(M4+): pickup feedback.
    (void)m;
}

void Game::handleMap(NetBitStream& s) {
    MapMsg m;
    m.deserialize(s);
    _map = std::move(m);
    _hasMap = true;
    // TODO(M4): construct map / terrain from _map (seed + rivers + objects).
}

GameStateSnapshot Game::snapshot() const {
    GameStateSnapshot out;
    out.activePlayerId = _activePlayerId;
    out.localPlayerId = _localPlayerId;
    out.playing = _playing;
    out.health = _activePlayer.health;

    out.players.reserve(_playersById.size());
    for (const auto& entry : _playersById) {
        const PlayerInfo& info = entry.second;
        out.players.push_back({info.playerId, info.teamId, info.groupId, info.name});
    }
    std::sort(out.players.begin(), out.players.end(),
              [](const PlayerInfoSnapshot& a, const PlayerInfoSnapshot& b) {
                  return a.playerId < b.playerId;
              });

    // Sort by id so the snapshot is independent of hash-map order.
    out.objects.reserve(_objectsById.size());
    for (const auto& entry : _objectsById) {
        out.objects.push_back(entry.second);
    }
    std::sort(out.objects.begin(), out.objects.end(),
              [](const ObjectSnapshot& a, const ObjectSnapshot& b) { return a.id < b.id; });

    return out;
}

std::string Game::snapshotText() const {
    const GameStateSnapshot s = snapshot();
    std::ostringstream os;
    char buf[128];
    std::snprintf(buf, sizeof(buf), "active=%u local=%u playing=%d health=%.3f\n",
                  static_cast<unsigned>(s.activePlayerId),
                  static_cast<unsigned>(s.localPlayerId),
                  s.playing ? 1 : 0,
                  static_cast<double>(s.health));
    os << buf;
    for (const auto& p : s.players) {
        std::snprintf(buf, sizeof(buf), "player %u team=%u group=%u name=%s\n",
                      static_cast<unsigned>(p.playerId),
                      static_cast<unsigned>(p.teamId),
                      static_cast<unsigned>(p.groupId),
                      p.name.c_str());
        os << buf;
    }
    for (const auto& o : s.objects) {
        std::snprintf(buf, sizeof(buf), "obj %u type=%u pos=%.3f,%.3f\n",
                      static_cast<unsigned>(o.id),
                      static_cast<unsigned>(o.type),
                      static_cast<double>(o.pos.x),
                      static_cast<double>(o.pos.y));
        os << buf;
    }
    return os.str();
}

} // namespace surv
