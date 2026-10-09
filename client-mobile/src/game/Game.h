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
    };
    void setJoinInfo(const JoinInfo& info);
    const JoinInfo& getJoinInfo() const { return _joinInfo; }
    // True once the server has accepted us (JoinedMsg or first UpdateMsg).
    bool hasJoined() const { return _joined; }
    // True while the client is attempting to open the game socket.
    bool isConnecting() const { return _connecting; }

    void update(float dt);

    // Connection lifecycle (JoinMsg -> JoinedMsg -> UpdateMsg stream).
    // Mirrors client/src/game.ts tryJoinGame(): ignored while connecting or
    // connected, and any previous connection is closed first.
    void tryJoinGame(const std::string& url, const std::string& joinToken);

    // App lifecycle (plan.md 5.8): close gracefully when backgrounded and
    // rejoin (server sends a fresh snapshot) when foregrounded again.
    void pause();
    void resume();
    bool isPaused() const { return _paused; }

    uint16_t getActivePlayerId() const { return _activePlayerId; }
    uint16_t getLocalPlayerId() const { return _localPlayerId; }
    bool isPlaying() const { return _playing; }
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
    void setMapCallback(MapCallback cb) { _mapCallback = std::move(cb); }
    void setUpdateCallback(UpdateCallback cb) { _updateCallback = std::move(cb); }

    // Headless state snapshot for replay verification.
    GameStateSnapshot snapshot() const;
    // Deterministic text form of snapshot(), suitable for diffing a native
    // replay against a web-client state dump.
    std::string snapshotText() const;

    // Last close info (code/reason) for UI/retry decisions.
    uint16_t getCloseCode() const { return _closeCode; }
    const std::string& getCloseReason() const { return _closeReason; }
    const ActivePlayerData& activePlayerData() const { return _activePlayer; }
    struct KillFeedEntry {
        KillMsg message;
        std::string killerName, targetName;
        float timeLeft = 8.0f;
    };
    const std::vector<KillFeedEntry>& killFeed() const { return _killFeed; }
    std::vector<int> aliveCounts() const;
    const GameOverMsg* gameOver() const { return _hasGameOver ? &_gameOver : nullptr; }

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
    ConnectionFactory _factory;

    std::unique_ptr<Connection> _connection;
    bool _connecting = false;
    bool _connected = false;
    bool _playing = false;
    bool _paused = false;
    uint16_t _closeCode = 0;
    std::string _closeReason;

    // Join info retained so a foreground resume can rejoin.
    std::string _lastUrl;
    std::string _lastToken;

    uint16_t _activePlayerId = 0;
    uint16_t _localPlayerId = 0;
    uint8_t _teamMode = 0;
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
    std::unordered_map<uint16_t, PlayerInfo> _playersById;
    std::vector<uint16_t> _playerIds; // wire insertion order for playerStatus
    std::unordered_map<uint16_t, PlayerStatus> _playerStatus;
    std::vector<uint8_t> _aliveCounts;
    std::vector<KillFeedEntry> _killFeed;
    GameOverMsg _gameOver;
    bool _hasGameOver = false;
    std::unordered_map<uint16_t, ObjectSnapshot> _objectsById;

    // Full last update (bullets/explosions/gas/... for M4+ rendering).
    UpdateMsg _lastUpdate;
    bool _hasUpdate = false;

    // MapMsg for M4 terrain; kept here so the headless client can report it.
    MapMsg _map;
    bool _hasMap = false;

    MapCallback _mapCallback;
    UpdateCallback _updateCallback;
};

} // namespace surv
