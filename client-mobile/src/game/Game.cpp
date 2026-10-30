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
    _timedOut = false;
    _lastMsgTime = _elapsed;

    _activePlayerId = 0;
    _localPlayerId = 0;
    _teamMode = 0;
    _anonPlayerNames = false;
    _localKills = 0;
    _started = false;
    _joined = false;
    _emotes = _joinInfo.emotes;
    _inputSeq = 0;
    _inputSeqInFlight = false;
    _playersById.clear();
    _playerKills.clear();
    _playerIds.clear();
    _playerStatus.clear();
    _aliveCounts.clear();
    _killFeed.clear();
    _roleAnnouncements.clear();
    _killLeaderId = 0;
    _killLeaderKills = 0;
    _activePlayer = ActivePlayerData{};
    _localStats = PlayerStatsMsg{};
    _hasLocalStats = false;
    _hasGameOver = false;
    _gameOverSeq = 0;
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
    _lastMsgTime = _elapsed;
    _timedOut = false;

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
        // A socket that failed to open may never deliver onClose (axmol reports
        // the error only), which would leave `_connecting` stuck and block the
        // reconnect/URL fallback. Surface it as an abnormal close.
        if (!_connected && _closeCode == 0) {
            onClose(1006, "error");
        }
    };
    _connection->onClose = [this](uint16_t code, const std::string& reason) { onClose(code, reason); };
}

void Game::pause() {
    if (_paused) {
        return;
    }
    _paused = true;
    // Priority 5 lifecycle: keep the socket. The server deletes the join token
    // on join (game/client.ts), so closing here could never rejoin the match -
    // it just dropped the player to the menu. Background input stops because the
    // app stops ticking; a socket that dies while away is surfaced on resume.
}

void Game::resume() {
    if (!_paused) {
        return;
    }
    _paused = false;
    // The socket usually survived a short background: keep playing. If it died,
    // the queued close is pumped by update() and GameScene leaves with the
    // localized reason; if it hangs half-open, the watchdog closes it.
}

void Game::update(float dt) {
    _elapsed += dt;

    for (auto& entry : _killFeed) entry.timeLeft -= dt;
    _killFeed.erase(std::remove_if(_killFeed.begin(), _killFeed.end(),
        [](const KillFeedEntry& entry) { return entry.timeLeft <= 0; }), _killFeed.end());
    // Pump queued WebSocket events/frames on the main thread, exactly like the
    // browser fires WebSocket events on the main thread. onOpen/onMessage/
    // onClose/onError are invoked from here, in arrival order.
    if (_connection) {
        _connection->pump();
    }

    // Liveness watchdog (priority 5): a half-open mobile socket never delivers a
    // close frame, so the client would sit in a frozen match forever. After
    // joining, silence for kConnectionTimeoutSeconds means the connection is
    // dead; synthesize the close the transport failed to report.
    if (_connected && _joined && !_paused && !_timedOut && !_hasGameOver &&
        _elapsed - _lastMsgTime > kConnectionTimeoutSeconds) {
        _timedOut = true;
        if (_connection) {
            _connection->resetAndClose();
        }
        onClose(1006, "timeout");
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
    // Backgrounded/paused: stop streaming input but keep the socket (priority 5).
    if (!isConnected() || _paused) {
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

void Game::sendSpectate(uint8_t action) {
    if (!isConnected()) {
        return;
    }
    SpectateMsg msg;
    msg.action = action;
    sendMessage(MsgType_Spectate, msg, 128);
}

void Game::onOpen() {
    _connecting = false;
    _connected = true;
    _lastMsgTime = _elapsed;
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
    // Any traffic proves the socket (and the path to the server) is alive.
    _lastMsgTime = _elapsed;
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
        case MsgType_PlayerStats: {
            PlayerStatsMsg m;
            m.deserialize(s);
            // Team death while teammates are still alive: the web shows the
            // death/stats screen with a Spectate button immediately.
            if (m.dead || m.playerId == _localPlayerId) {
                _localStats = m;
                _hasLocalStats = true;
            }
            break;
        }
        case MsgType_RoleAnnouncement: {
            RoleAnnouncementMsg m;
            m.deserialize(s);
            _roleAnnouncements.push_back({m.playerId, m.killerId, m.role, m.assigned, m.killed});
            break;
        }
        default:
            break;
    }
}

void Game::handleJoined(NetBitStream& s) {
    JoinedMsg m;
    m.deserialize(s);
    _teamMode = m.teamMode;
    _localPlayerId = m.playerId;
    // The active player is the local player until an UpdateMsg says otherwise
    // (spectating). Setting it here lets the first full object's gear/perks be
    // extracted even if that update does not repeat activePlayerIdDirty.
    _activePlayerId = m.playerId;
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

    // Extract gear/role/perks from the active player's ObjectData in full/part objects
    // (mirrors player.ts m_updateData where chest/helmet/backpack/role/perks come from ObjectData)
    {
        auto extractPlayerData = [this](const ObjectData& od, bool isPart) {
            const uint16_t activeId = _activePlayerId != 0 ? _activePlayerId : _localPlayerId;
            if (od.__id != activeId) return;
            if (od.__type != ObjectType_Player) return;
            if (isPart) {
                // Part updates may have empty fields if not dirty
                if (!od.chest.empty()) _activePlayer.chest = od.chest;
                if (!od.helmet.empty()) _activePlayer.helmet = od.helmet;
                if (!od.backpack.empty()) _activePlayer.backpack = od.backpack;
                if (!od.role.empty()) _activePlayer.role = od.role;
                if (!od.perks.empty()) {
                    _activePlayer.perks.clear();
                    _activePlayer.perks.reserve(od.perks.size());
                    for (const auto& p : od.perks) {
                        _activePlayer.perks.push_back({p.type, p.droppable});
                    }
                }
            } else {
                _activePlayer.chest = od.chest;
                _activePlayer.helmet = od.helmet;
                _activePlayer.backpack = od.backpack;
                _activePlayer.role = od.role;
                _activePlayer.perks.clear();
                _activePlayer.perks.reserve(od.perks.size());
                for (const auto& p : od.perks) {
                    _activePlayer.perks.push_back({p.type, p.droppable});
                }
            }
        };
        for (const auto& obj : m.fullObjects) {
            extractPlayerData(obj.data, false);
        }
        for (const auto& obj : m.partObjects) {
            extractPlayerData(obj.data, true);
        }
    }

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
    // Priority 2: store GroupStatus for squad health bars.
    if (m.groupStatusDirty) {
        _groupStatus = m.groupStatus;
    }
    if (m.killLeaderDirty) {
        _killLeaderId = m.killLeaderId;
        _killLeaderKills = m.killLeaderKills;
    }
    _lastUpdate = std::move(m);
    _hasUpdate = true;
    if (_updateCallback) {
        _updateCallback(_lastUpdate);
    }
    // TODO(M4+): interpolate objects with _lastUpdate.
}

std::string Game::displayName(uint16_t playerId) const {
    const auto found = _playersById.find(playerId);
    if (found == _playersById.end()) {
        return "Player";
    }
    if (_anonPlayerNames) {
        const auto active = _playersById.find(_activePlayerId);
        if (active != _playersById.end() && active->second.groupId != found->second.groupId) {
            return "Player";
        }
    }
    return found->second.name;
}

uint8_t Game::playerTeamId(uint16_t playerId) const {
    const auto found = _playersById.find(playerId);
    return found == _playersById.end() ? 0 : found->second.teamId;
}

namespace {
std::string jsonEscape(const std::string& value) {
    std::string out;
    out.reserve(value.size() + 8);
    for (const char c : value) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}
} // namespace

std::string Game::leaderboardJson() const {
    struct Row {
        uint16_t id;
        uint8_t teamId;
        uint8_t kills;
        bool dead;
        std::string name;
    };
    std::vector<Row> rows;
    rows.reserve(_playersById.size());
    for (const auto& kv : _playersById) {
        const uint16_t id = kv.first;
        const PlayerInfo& info = kv.second;
        bool dead = false;
        const auto status = _playerStatus.find(id);
        if (status != _playerStatus.end() && status->second.hasData) {
            dead = status->second.dead;
        }
        const auto kills = _playerKills.find(id);
        rows.push_back({id, info.teamId,
                        static_cast<uint8_t>(kills == _playerKills.end() ? 0 : kills->second), dead,
                        displayName(id)});
    }
    std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
        if (a.kills != b.kills) return a.kills > b.kills;
        return a.name < b.name;
    });

    std::string out = "{\"activePlayerId\":" + std::to_string(_activePlayerId) + ",\"players\":[";
    for (size_t i = 0; i < rows.size(); ++i) {
        if (i > 0) out += ",";
        const Row& r = rows[i];
        out += "{\"playerId\":" + std::to_string(r.id);
        out += ",\"name\":\"" + jsonEscape(r.name) + "\"";
        out += ",\"teamId\":" + std::to_string(r.teamId);
        out += ",\"kills\":" + std::to_string(r.kills);
        out += ",\"dead\":";
        out += r.dead ? "true" : "false";
        out += "}";
    }
    out += "]}";
    return out;
}

void Game::addKillFeedText(const std::string& text, uint32_t color) {
    if (text.empty()) {
        return;
    }
    KillFeedEntry entry;
    entry.text = text;
    entry.color = color;
    entry.custom = true;
    entry.timeLeft = 8.0f;
    _killFeed.push_back(std::move(entry));
    if (_killFeed.size() > 6) {
        _killFeed.erase(_killFeed.begin());
    }
}

void Game::handleKill(NetBitStream& s) {
    KillMsg m;
    m.deserialize(s);
    _killFeed.push_back({m, displayName(m.killerId), displayName(m.targetId), 8.0f});
    if (m.killerId != 0) {
        auto& kills = _playerKills[m.killerId];
        kills = std::max<uint8_t>(kills, m.killerKills);
    }
    if (m.killed && (m.killCreditId != 0 ? m.killCreditId : m.killerId) == _localPlayerId) {
        ++_localKills;
    }
    if (_killFeed.size() > 6) _killFeed.erase(_killFeed.begin());
    if (m.killed) {
        PlayerStatus& status = _playerStatus[m.targetId];
        status.hasData = true;
        status.dead = true;
    }
}

void Game::handleGameOver(NetBitStream& s) {
    GameOverMsg m;
    m.deserialize(s);
    _playing = false;
    // Keep the local entry for the spectate stats panel / result fallback.
    for (const auto& stats : m.playerStats) {
        if (stats.playerId == _localPlayerId) {
            _localStats = stats;
            _localStats.dead = true;
            _hasLocalStats = true;
            break;
        }
    }
    _gameOver = std::move(m);
    _hasGameOver = true;
    ++_gameOverSeq;
}

void Game::handlePickup(NetBitStream& s) {
    PickupMsg m;
    m.deserialize(s);
    // Priority 1: play item pickup sound for the active player (player.ts ~line 2095),
    // routed through the app's audio callback so host tests stay silent.
    // shared/net/net.ts PickupMsgType.Success == 4.
    if (m.type == 4 && !m.item.empty()) {
        if (_pickupSoundCb) {
            _pickupSoundCb(m.item);
        }
    } else if (_pickupErrorCb) {
        _pickupErrorCb(m.type);
    }
}

void Game::sendEmote(const std::string& type, const Vec2& pos, bool isPing) {
    EmoteMsg msg;
    msg.type = type;
    msg.pos = pos;
    msg.isPing = isPing;
    sendMessage(MsgType_Emote, msg, 128);
}

std::vector<Game::RoleAnnouncement> Game::consumeRoleAnnouncements() {
    std::vector<RoleAnnouncement> out = std::move(_roleAnnouncements);
    _roleAnnouncements.clear();
    return out;
}

void Game::handleMap(NetBitStream& s) {
    MapMsg m;
    m.deserialize(s);
    _map = std::move(m);
    _hasMap = true;
    if (_mapCallback) {
        _mapCallback(_map);
    }
    // The map callback is where the app registers the map's sprite atlases; tell
    // sprites that were built before any map that their frames may now resolve.
    if (_atlasCallback) {
        _atlasCallback();
    }
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
