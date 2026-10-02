#include "Game.h"
#include "../app/GameScene.h"
#include "../net/Net.h"

namespace surv {

Game::Game() = default;
Game::~Game() = default;

void Game::init(GameScene* scene) {
    _scene = scene;
    // TODO(M3): create Connection via ax::network::WebSocket and register a
    // binary-message handler that calls onServerMessage() on the main thread.
}

void Game::free() {
    // TODO(M3): close connection, clear object pools.
    _activePlayerId = 0;
}

void Game::tryJoinGame(const std::string& url, const std::string& joinToken) {
    // Build + send a JoinMsg (protocol-compatible, verified by surv_tests).
    JoinMsg msg;
    msg.protocol = defs::kProtocolVersion;
    msg.joinToken = joinToken;
    msg.name = "Player";     // TODO(M7): from UI/config
    msg.useTouch = true;     // mobile build always uses touch
    msg.isMobile = true;
    msg.outfit = "outfitBase";
    msg.melee = "fists";
    msg.heal = "bandage";
    msg.boost = "soda";

    std::vector<uint8_t> buf(256, 0);
    NetBitStream s(buf.data(), buf.size());
    msg.serialize(s);

    // TODO(M3): send over the WebSocket, then wait for JoinedMsg.
    (void)url;
}

void Game::update(float dt) {
    // TODO(M3+): pump queued server messages, then run simulation update.
    // Input is collected from the Touch dual-joystick in GameScene and packed
    // into an InputMsg here (mirrors client/src/game.ts input handling).
    (void)dt;
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
    _activePlayerId = m.playerId;
    // TODO(M3): set up loadout/emote data from m.
}

void Game::handleUpdate(NetBitStream& s) {
    UpdateMsg m;
    m.deserialize(s);
    if (m.activePlayerIdDirty) {
        _activePlayerId = m.activePlayerId;
    }
    _activePlayer = m.activePlayerData;
    _fullObjects = std::move(m.fullObjects);
    _partObjects = std::move(m.partObjects);
    // TODO(M4+): feed into object barns / renderer.
}

void Game::handleKill(NetBitStream& s) {
    KillMsg m;
    m.deserialize(s);
    // TODO(M7): killfeed UI + death screen.
}

void Game::handleGameOver(NetBitStream& s) {
    GameOverMsg m;
    m.deserialize(s);
    // TODO(M7): stats screen.
}

void Game::handlePickup(NetBitStream& s) {
    PickupMsg m;
    m.deserialize(s);
    // TODO(M4+): pickup feedback.
}

void Game::handleMap(NetBitStream& s) {
    MapMsg m;
    m.deserialize(s);
    // TODO(M4): construct map / terrain from m (seed + rivers + objects).
}

} // namespace surv