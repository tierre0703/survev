#pragma once
// Port of client/src/game.ts core (M3+): connection lifecycle, message
// dispatch, and the per-tick simulation update. The network protocol
// (net/*) is ported and verified; simulation/render are scaffolded here.
//
// This class is intentionally axmol-free: the concrete WebSocket adapter is
// injected as a ConnectionFactory (see net/Connection.h), which keeps the
// join/pump/dispatch flow host-testable against captured replays.
#include "../net/Connection.h"
#include "../net/Messages.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace ax {
class Scene;
}

namespace surv {

class GameScene;

// Compact, deterministic view of the state the headless client holds after a
// sequence of messages. Used to diff a native replay against the web client.
struct PlayerInfoSnapshot {
    uint16_t playerId = 0;
    uint8_t teamId = 0;
    uint8_t groupId = 0;
    std::string name;
};

struct ObjectSnapshot {
    uint16_t id = 0;
    uint8_t type = 0;
    Vec2 pos;
};

struct GameStateSnapshot {
    uint16_t activePlayerId = 0;
    uint16_t localPlayerId = 0;
    bool playing = false;
    float health = 0.0f;
    std::vector<PlayerInfoSnapshot> players;
    std::vector<ObjectSnapshot> objects;
};

class Game {
public:
    Game();
    ~Game();

    void init(GameScene* scene, ConnectionFactory factory = nullptr);
    void free();

    // M7: join identity supplied by the menu (name + loadout). Read when the
    // JoinMsg is built on socket open.
    struct JoinInfo {
        std::string name = "Player";
        std::string outfit = "outfitBase";
        std::string melee = "fists";
        std::string heal = "heal_basic";
        std::string boost = "boost_basic";
        std::vector<std::string> emotes;
        // Local-only cosmetics from loadout.crosshair (not serialized in JoinMsg).
        std::string crosshairType = "crosshair_default";
        uint32_t crosshairColor = 0xffffff;
        float crosshairSize = 1.0f;
        float crosshairStroke = 0.0f;
    };
    void setJoinInfo(const JoinInfo& info);
    const JoinInfo& getJoinInfo() const { return _joinInfo; }
    // True once the server has accepted us (JoinedMsg or first UpdateMsg).
    bool hasJoined() const { return _joined; }
    // JoinedMsg.started: false means the match is still waiting for players.
    bool hasStarted() const { return _started; }
    // True while the client is attempting to open the game socket.
    bool isConnecting() const { return _connecting; }

    void update(float dt);

    // Connection lifecycle (JoinMsg -> JoinedMsg -> UpdateMsg stream).
    // Mirrors client/src/game.ts tryJoinGame(): ignored while connecting or
    // connected, and any previous connection is closed first.
    void tryJoinGame(const std::string& url, const std::string& joinToken);

    // App lifecycle (priority 5). `pause()` freezes local input when the app is
    // backgrounded or the pause dialog opens, but keeps the game socket: join
    // tokens are single-use server-side, so closing here would always lose the
    // match. A socket that dies while away is surfaced by the queued close (or
    // by the liveness watchdog) when the app resumes.
    void pause();
    void resume();
    bool isPaused() const { return _paused; }

    // No server message for this long after joining marks a half-open socket as
    // dead (mobile networks switch cells without delivering a close frame).
    static constexpr float kConnectionTimeoutSeconds = 12.0f;

    uint16_t getActivePlayerId() const { return _activePlayerId; }
    uint16_t getLocalPlayerId() const { return _localPlayerId; }
    bool isPlaying() const { return _playing; }
    // In-match kill counter (Hud "N K"): killed credits for the local player.
    int localKills() const { return _localKills; }
    bool isConnected() const {
        return _connection && _connection->state() == ConnectionState::Open;
    }

    // True while the active player has the throwable slot equipped. Drives the
    // Touch throwable priming latch (client/src/ui/touch.ts).
    bool isHoldingThrowable() const {
        return _activePlayer.curWeapIdx == WeaponSlot_Throwable;
    }

    // Sends a protocol message on the current connection (no-op unless open).
    void sendMessage(MsgType type, Msg& msg, size_t maxLen = 128);
    // Sends an input message, assigning the next sequence number when the
    // previous one has been acknowledged (mirrors game.ts seq/seqInFlight).
    void sendInput(InputMsg& msg);

    // Message sink for the connection pump -> dispatch (main thread).
    void onServerMessage(uint8_t type, NetBitStream& s);

    // M4 render hooks: invoked on the main thread as map/update messages are
    // dispatched, so the render layer can build terrain and apply object deltas.
    using MapCallback = std::function<void(const MapMsg&)>;
    using UpdateCallback = std::function<void(const UpdateMsg&)>;
    // Fired after the app registers a map's sprite atlases, i.e. after the map
    // callback has run: sprites that were created before any map (the touch
    // pads) re-resolve their atlas frames here.
    using AtlasCallback = std::function<void()>;
    void setMapCallback(MapCallback cb) { _mapCallback = std::move(cb); }
    // The last MapMsg the server sent for the current session, so a render
    // world created *after* the map arrived (GameScene builds it on join, the
    // server's MapMsg can land first) can load it (game.ts keeps mapData).
    bool hasMap() const { return _hasMap; }
    const MapMsg& map() const { return _map; }
    void setUpdateCallback(UpdateCallback cb) { _updateCallback = std::move(cb); }
    void setAtlasCallback(AtlasCallback cb) { _atlasCallback = std::move(cb); }

    // Played when the server confirms a successful item pickup (PickupMsg
    // type == Success). The app (GameScene) installs this to play the item's
    // ui-channel pickup sound; the headless host tests leave it unset.
    using PickupSoundCallback = std::function<void(const std::string& item)>;
    void setPickupSoundCallback(PickupSoundCallback cb) { _pickupSoundCb = std::move(cb); }
    // Non-success PickupMsg types (ui2.ts displayPickupMessage: full bag,
    // already owned/equipped, max perks, ...). The app maps the type to text.
    using PickupErrorCallback = std::function<void(int type)>;
    void setPickupErrorCallback(PickupErrorCallback cb) { _pickupErrorCb = std::move(cb); }

    // Headless state snapshot for replay verification.
    GameStateSnapshot snapshot() const;
    // Deterministic text form of snapshot(), suitable for diffing a native
    // replay against a web-client state dump.
    std::string snapshotText() const;

    // Last close info (code/reason) for UI/retry decisions.
    uint16_t getCloseCode() const { return _closeCode; }
    const std::string& getCloseReason() const { return _closeReason; }
    const ActivePlayerData& activePlayerData() const { return _activePlayer; }
    // Death stats from PlayerStatsMsg (team death while teammates are still
    // alive) or the local entry in GameOverMsg. Null until the local player dies.
    const PlayerStatsMsg* localStats() const { return _hasLocalStats ? &_localStats : nullptr; }
    bool isLocalDead() const { return _hasLocalStats && _localStats.dead; }
    // game.ts `m_spectating = m_activeId != m_localId`.
    bool isSpectating() const {
        return _activePlayerId != 0 && _localPlayerId != 0 && _activePlayerId != _localPlayerId;
    }
    // SpectateMsg action: 1 = Begin, 2 = Next, 3 = Prev (shared/net/spectateMsg.ts).
    void sendSpectate(uint8_t action);
    struct KillFeedEntry {
        KillMsg message;
        std::string killerName, targetName;
        float timeLeft = 8.0f;
        // Role-announcement lines (game.ts RoleAnnouncement kill feed) carry
        // their own text/colour instead of a KillMsg.
        std::string text;
        uint32_t color = 0xffffff;
        bool custom = false;
    };
    const std::vector<KillFeedEntry>& killFeed() const { return _killFeed; }
    // Adds a pre-formatted kill-feed line (role promoted/dead, reuses Game's
    // 6-row cap and 8 s lifetime).
    void addKillFeedText(const std::string& text, uint32_t color);
    // JoinedMsg/JoinMsg emote loadout (4 emote types for the emote wheel).
    const std::vector<std::string>& emotes() const { return _emotes; }
    // JoinedMsg team mode (gameConfig.ts TeamMode: Solo = 1).
    uint8_t teamMode() const { return _teamMode; }
    // config `anonPlayerNames`: hide names of players outside the local group.
    void setAnonPlayerNames(bool enabled) { _anonPlayerNames = enabled; }
    bool anonPlayerNames() const { return _anonPlayerNames; }
    // playerBarn.getPlayerName(): the raw name, or "Player" for other groups
    // when anon names is on (kill feed / kill leader / spectate).
    std::string displayName(uint16_t playerId) const;
    // PlayerInfo.teamId for role-feed colours/names (0 when unknown).
    uint8_t playerTeamId(uint16_t playerId) const;
    // Live in-game leaderboard (pause dialog): player names, teams, kills and
    // alive state as a JSON array + activePlayerId (UiBridge -> Java modal).
    std::string leaderboardJson() const;
    // EmoteMsg send (game.ts newEmotes/newPings loop). The chat/emote wheel
    // queues into EmoteManager and GameScene forwards the queue here.
    void sendEmote(const std::string& type, const Vec2& pos, bool isPing);
    std::vector<int> aliveCounts() const;
    const GameOverMsg* gameOver() const { return _hasGameOver ? &_gameOver : nullptr; }

    // game.ts RoleAnnouncement: role assigned/dead sounds + kill-leader UI.
    struct RoleAnnouncement {
        uint16_t playerId = 0;
        uint16_t killerId = 0;
        std::string role;
        bool assigned = false;
        bool killed = false;
    };
    std::vector<RoleAnnouncement> consumeRoleAnnouncements();
    uint16_t killLeaderId() const { return _killLeaderId; }
    uint8_t killLeaderKills() const { return _killLeaderKills; }
    // Priority 2: squad health bar data.
    const std::unordered_map<uint16_t, PlayerInfo>& allPlayerInfo() const { return _playersById; }
    const std::unordered_map<uint16_t, PlayerStatus>& allPlayerStatus() const { return _playerStatus; }
    const std::vector<GroupStatus>& groupStatus() const { return _groupStatus; }
    uint16_t teamIdForPlayer(uint16_t playerId) const {
        auto it = _playersById.find(playerId);
        return it != _playersById.end() ? it->second.teamId : 0;
    }
    std::vector<uint16_t> getTeamPlayerIds() const {
        const auto active = _playersById.find(_activePlayerId);
        if (active == _playersById.end()) return {};
        std::vector<uint16_t> ids;
        for (auto id : _playerIds) {
            auto it = _playersById.find(id);
            if (it != _playersById.end() && it->second.teamId == active->second.teamId) {
                ids.push_back(id);
            }
        }
        return ids;
    }

private:
    void handleJoined(NetBitStream& s);
    void handleUpdate(NetBitStream& s);
    void handleKill(NetBitStream& s);
    void handleGameOver(NetBitStream& s);
    void handlePickup(NetBitStream& s);
    void handleMap(NetBitStream& s);

    void onOpen();
    void onFrame(std::vector<uint8_t>&& frame);
    void onClose(uint16_t code, const std::string& reason);
    void sendJoinMessage();

    GameScene* _scene = nullptr;
    PickupSoundCallback _pickupSoundCb;
    PickupErrorCallback _pickupErrorCb;
    ConnectionFactory _factory;

    std::unique_ptr<Connection> _connection;
    bool _connecting = false;
    bool _connected = false;
    bool _playing = false;
    bool _paused = false;
    uint16_t _closeCode = 0;
    std::string _closeReason;

    // Liveness watchdog (half-open mobile sockets). `_elapsed` is the sum of
    // update(dt); `_lastMsgTime` is refreshed by every received frame.
    float _elapsed = 0.0f;
    float _lastMsgTime = 0.0f;
    bool _timedOut = false;

    // Last join target; kept for diagnostics and the reconnect-less resume path.
    std::string _lastUrl;
    std::string _lastToken;

    uint16_t _activePlayerId = 0;
    uint16_t _localPlayerId = 0;
    uint8_t _teamMode = 0;
    bool _anonPlayerNames = false;
    int _localKills = 0;
    bool _started = false;
    JoinInfo _joinInfo;
    bool _joined = false;
    std::vector<std::string> _emotes;
    uint32_t _inputSeq = 0;
    bool _inputSeqInFlight = false;

    // Joined/Update state populated from the wire (game simulation source).
    // Players/objects persist across ticks, mirroring the web client's barns:
    // full objects create/update, partial objects update, and del lists remove.
    ActivePlayerData _activePlayer;
    PlayerStatsMsg _localStats;
    bool _hasLocalStats = false;
    std::unordered_map<uint16_t, PlayerInfo> _playersById;
    // Latest KillMsg.killerKills per player (live leaderboard).
    std::unordered_map<uint16_t, uint8_t> _playerKills;
    std::vector<uint16_t> _playerIds; // wire insertion order for playerStatus
    std::unordered_map<uint16_t, PlayerStatus> _playerStatus;
    std::vector<uint8_t> _aliveCounts;
    std::vector<KillFeedEntry> _killFeed;
    GameOverMsg _gameOver;
    bool _hasGameOver = false;
    std::vector<RoleAnnouncement> _roleAnnouncements;
    uint16_t _killLeaderId = 0;
    uint8_t _killLeaderKills = 0;
    std::unordered_map<uint16_t, ObjectSnapshot> _objectsById;
    // GroupStatus from latest UpdateMsg (health + disconnected per group member).
    std::vector<GroupStatus> _groupStatus;

    // Full last update (bullets/explosions/gas/... for M4+ rendering).
    UpdateMsg _lastUpdate;
    bool _hasUpdate = false;

    // MapMsg for M4 terrain; kept here so the headless client can report it.
    MapMsg _map;
    bool _hasMap = false;

    MapCallback _mapCallback;
    UpdateCallback _updateCallback;
    AtlasCallback _atlasCallback;
};

} // namespace surv
