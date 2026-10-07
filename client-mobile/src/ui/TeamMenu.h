#pragma once
// Client for the `/team_v2` service (port of client/src/ui/teamMenu.ts).
//
// The team menu is a JSON control channel over a WebSocket, separate from the
// binary game protocol. Like the game transport it is engine-independent: the
// app injects a `Connection` factory (`createWebSocketConnectionTo`), host
// tests inject an in-memory fake, and the UI only sees decoded team messages.
#include "../net/Connection.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ui {

using surv::Connection;
using surv::ConnectionState;

// shared/types/team.ts RoomData (as sent by the server).
struct RoomData {
    std::string roomUrl;
    std::string region = "na";
    int gameModeIdx = 0;
    bool autoFill = true;
    bool findingGame = false;
    bool captchaEnabled = false;
    int maxPlayers = 0;
    std::vector<int> enabledGameModeIdxs;
};

struct TeamPlayer {
    uint16_t playerId = 0;
    bool isLeader = false;
    bool inGame = false;
    std::string name;
};

// shared/types/team.ts TeamMenuErrorType.
enum class TeamErrorType {
    JoinFull,
    JoinNotFound,
    CreateFailed,
    JoinFailed,
    JoinGameFailed,
    LostConn,
    FindGameError,
    FindGameFull,
    FindGameInvalidProtocol,
    FindGameInvalidCaptcha,
    Kicked,
    Banned,
    BehindProxy,
    RateLimited,
    Unknown,
};

// Parse a server `error.data.type` string.
TeamErrorType teamErrorFromString(const std::string& type);

class TeamMenu {
public:
    // Transport factory: `url` is the full ws(s):// URL of the team endpoint.
    using ConnectionFactory = std::function<std::unique_ptr<Connection>(const std::string& url)>;

    struct MatchData {
        std::vector<std::string> urls;
        std::string joinToken;
    };

    TeamMenu() = default;
    ~TeamMenu();

    void setFactory(ConnectionFactory factory) { _factory = std::move(factory); }
    void setUrl(const std::string& url) { _teamUrl = url; }
    void update(float dt);

    // Port of TeamMenu.connect(create, roomUrl): opens `/team_v2` and sends
    // `create`/`join` on open. `roomUrl` is the invite code (no leading '#').
    void connect(bool create, const std::string& roomUrl);
    void leave();

    void setPlayerName(const std::string& name) { _playerName = name; }
    // Set the room properties the client owns (leader only).
    void setRoomRegion(const std::string& region);
    void setRoomAutoFill(bool autoFill);
    void setRoomGameMode(int gameModeIdx);
    void tryStartGame();

    bool isLeader() const { return _isLeader; }
    bool isActive() const { return _active; }
    bool isJoined() const { return _joined; }
    bool isFindingGame() const { return _roomData.findingGame; }
    uint16_t localPlayerId() const { return _localPlayerId; }
    const RoomData& roomData() const { return _roomData; }
    const std::vector<TeamPlayer>& players() const { return _players; }
    const std::string& gameError() const { return _gameError; }
    const std::string& roomUrl() const { return _roomData.roomUrl; }

    // Port of TeamMenu.onGameComplete: tells the room a match ended so it can
    // return everyone to the lobby.
    void onGameComplete(const std::string& errorMessage = "");

    // --- callbacks, invoked on the main thread ---
    std::function<void()> onRoomChanged;                              // state applied
    std::function<void(TeamErrorType, const std::string&)> onError;   // leave + reason
    std::function<void(const MatchData&)> onPlay;                     // joinGame
    std::function<void()> onLostConnection;

    // Test seam: feeds a raw JSON message into the decoder.
    void handleMessage(const std::string& json);
    void setTurnstileResolver(std::function<std::string(const std::string& region)> resolver) {
        _turnstileResolver = std::move(resolver);
    }

private:
    void sendJson(const std::string& json);
    void sendMessage(const std::string& type);
    void sendMessage(const std::string& type, const std::string& dataJson);
    void onSocketOpen();
    void onSocketClose();
    void applyState(const std::string& dataJson);
    void fail(TeamErrorType type);

    ConnectionFactory _factory;
    std::unique_ptr<Connection> _connection;
    std::string _teamUrl = "/team_v2";
    bool _active = false;
    bool _joined = false;
    bool _create = false;
    bool _joiningGame = false;
    bool _isLeader = false;
    uint16_t _localPlayerId = 0;
    float _keepAliveTicker = 0.0f;
    std::string _playerName;
    RoomData _roomData;
    std::vector<TeamPlayer> _players;
    std::string _gameError;
    std::function<std::string(const std::string&)> _turnstileResolver;
};

// Maps a team error type to the web client's localization key
// (teamMenu.ts errorTypeToString).
const char* teamErrorL10n(TeamErrorType type);

} // namespace ui