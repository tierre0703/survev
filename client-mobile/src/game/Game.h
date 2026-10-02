#pragma once
// Port of client/src/game.ts core (M3+): connection lifecycle, message
// dispatch, and the per-tick simulation update. The network protocol
// (net/*) is ported and verified; simulation/render are scaffolded here.
#include "../net/Messages.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ax {
class Scene;
}

namespace surv {

class GameScene;

struct ActivePlayerData;
struct UpdateMsg;

class Game {
public:
    Game();
    ~Game();

    void init(GameScene* scene);
    void free();

    void update(float dt);

    // Connection lifecycle (JoinMsg -> JoinedMsg -> UpdateMsg stream).
    void tryJoinGame(const std::string& url, const std::string& joinToken);

    uint16_t getActivePlayerId() const { return _activePlayerId; }

    // Message sink for the connection thread -> main thread.
    void onServerMessage(uint8_t type, NetBitStream& s);

private:
    void handleJoined(NetBitStream& s);
    void handleUpdate(NetBitStream& s);
    void handleKill(NetBitStream& s);
    void handleGameOver(NetBitStream& s);
    void handlePickup(NetBitStream& s);
    void handleMap(NetBitStream& s);

    GameScene* _scene = nullptr;
    uint16_t _activePlayerId = 0;
    uint32_t _inputSeq = 0;

    // Joined/Update state populated from the wire (game simulation source).
    ActivePlayerData _activePlayer;
    std::vector<FullObjectData> _fullObjects;
    std::vector<PartObjectData> _partObjects;
};

} // namespace surv