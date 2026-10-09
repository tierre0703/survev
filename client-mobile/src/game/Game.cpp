#include "Game.h"
#include "../net/Net.h"
#include "../render/Defs.h"

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

void Game::setJoinInfo(const JoinInfo& info) {
    _joinInfo = info;
    _emotes = info.emotes;
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
    _joined = false;
    _emotes = _joinInfo.emotes;
    _inputSeq = 0;
    _inputSeqInFlight = false;
    _playersById.clear();
    _playerIds.clear();
    _playerStatus.clear();
    _aliveCounts.clear();
    _killFeed.clear();
    _activePlayer = ActivePlayerData{};
    _hasGameOver = false;
    _lastUrl.clear();
    _lastToken.clear();
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
    for (auto& entry : _killFeed) entry.timeLeft -= dt;
    _killFeed.erase(std::remove_if(_killFeed.begin(), _killFeed.end(),
        [](const KillFeedEntry& entry) { return entry.timeLeft <= 0; }), _killFeed.end());
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
    // Mirrors game.ts onOpen(): build the JoinMsg from the menu's join info
    // (M7) plus the device capabilities.
    JoinMsg msg;
    msg.protocol = defs::kProtocolVersion;
    msg.joinToken = _lastToken;
    msg.name = _joinInfo.name;
    msg.useTouch = true;     // mobile build always uses touch
    msg.isMobile = true;
    msg.bot = false;
    msg.outfit = _joinInfo.outfit;
    msg.melee = _joinInfo.melee;
    msg.heal = _joinInfo.heal;
    msg.boost = _joinInfo.boost;
    msg.emotes = _joinInfo.emotes;
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
    // M7: the UI observes isConnected()/isPlaying() transitions and surfaces
    // `reason` (GameWsDisconnectReason) with a localized message.
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
        case MsgType_AliveCounts: {
            AliveCountsMsg m;
            m.deserialize(s);
            _aliveCounts = std::move(m.teamAliveCounts);
            break;
        }
        case MsgType_PlayerStats: { PlayerStatsMsg m; m.deserialize(s); break; }
        case MsgType_RoleAnnouncement: { RoleAnnouncementMsg m; m.deserialize(s); break; }
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
    _joined = true;
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
    _joined = true;
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
        if (!_playersById.count(info.playerId)) _playerIds.push_back(info.playerId);
        _playersById[info.playerId] = info;
    }
    for (uint16_t id : m.deletedPlayerIds) {
        _playersById.erase(id);
        _playerStatus.erase(id);
        _playerIds.erase(std::remove(_playerIds.begin(), _playerIds.end(), id), _playerIds.end());
    }

    // Keep the full tick (bullets/gas/...) for the M4 renderer.
    // UpdateMsg carries deltas. Do not erase inventory/health on clean ticks.
    const auto& data = m.activePlayerData;
    if (data.healthDirty) _activePlayer.health = data.health;
    if (data.boostDirty) _activePlayer.boost = data.boost;
    if (data.zoomDirty) _activePlayer.zoom = data.zoom;
    if (data.actionDirty) {
        _activePlayer.actionTime = data.actionTime;
        _activePlayer.actionDuration = data.actionDuration;
        _activePlayer.actionTargetId = data.actionTargetId;
    }
    if (data.inventoryDirty) {
        _activePlayer.inventory = data.inventory;
        _activePlayer.scope = data.scope;
    }
    if (data.weapsDirty) {
        _activePlayer.curWeapIdx = data.curWeapIdx;
        _activePlayer.weapons = data.weapons;
    }
    if (data.spectatorCountDirty) _activePlayer.spectatorCount = data.spectatorCount;
    if (m.playerStatusDirty) {
        const auto* provider = getDefProvider();
        const auto* map = provider ? provider->mapRender(_map.mapName) : nullptr;
        const bool faction = map && map->factionMode;
        const auto active = _playersById.find(_activePlayerId);
        std::vector<uint16_t> ids;
        for (auto id : _playerIds) {
            if (faction || (active != _playersById.end()
                && _playersById.at(id).teamId == active->second.teamId)) ids.push_back(id);
        }
        if (ids.size() == m.playerStatus.size()) {
            for (size_t i = 0; i < ids.size(); ++i) {
                if (m.playerStatus[i].hasData) _playerStatus[ids[i]] = m.playerStatus[i];
            }
        }
    }
    _lastUpdate = std::move(m);
    _hasUpdate = true;
    if (_updateCallback) {
        _updateCallback(_lastUpdate);
    }
    // TODO(M4+): interpolate objects with _lastUpdate.
}

void Game::handleKill(NetBitStream& s) {
    KillMsg m;
    m.deserialize(s);
    auto name = [this](uint16_t id) {
        const auto found = _playersById.find(id);
        return found == _playersById.end() ? std::string("Player") : found->second.name;
    };
    _killFeed.push_back({m, name(m.killerId), name(m.targetId), 8.0f});
    if (_killFeed.size() > 6) _killFeed.erase(_killFeed.begin());
    if (m.killed) _playerStatus[m.targetId].dead = true;
}

void Game::handleGameOver(NetBitStream& s) {
    GameOverMsg m;
    m.deserialize(s);
    _playing = false;
    _gameOver = std::move(m);
    _hasGameOver = true;
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
    if (_mapCallback) {
        _mapCallback(_map);
    }
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

std::vector<int> Game::aliveCounts() const {
    // The existing AliveCountsMsg is authoritative (including unseen enemies).
    // Before it arrives, derive the best available count from infos/statuses.
    if (!_aliveCounts.empty()) return {_aliveCounts.begin(), _aliveCounts.end()};
    const auto* provider = getDefProvider();
    const auto* map = provider ? provider->mapRender(_map.mapName) : nullptr;
    std::vector<int> result(map && map->factionMode ? 2 : 1, 0);
    for (const auto& pair : _playersById) {
        const auto status = _playerStatus.find(pair.first);
        if (status != _playerStatus.end() && status->second.dead) continue;
        const size_t team = result.size() == 2 && pair.second.teamId == 2 ? 1 : 0;
        ++result[team];
    }
    return result;
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
