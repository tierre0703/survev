// Host tests for the Android join crash fix: GameScene's render world used to
// be built by resetWorld() *after* the socket had already pumped the session's
// MapMsg, so the map callback dereferenced a null GameWorld and the app died
// with SIGSEGV in GameWorld::loadMap(). These cover the pieces that keep the
// first MapMsg safe and loadable.
#include "TestFramework.h"
#include "FakeConnection.h"
#include "../src/game/Game.h"
#include "../src/game/GameWorld.h"
#include "../src/render/NullPixi.h"

#include <vector>

using namespace surv;
using namespace surv_test;

namespace {

template <typename T>
std::vector<uint8_t> makeFrame(MsgType type, T& msg) {
    MsgStream stream(std::vector<uint8_t>(4096, 0));
    stream.serializeMsg(type, [&msg](NetBitStream& s) { msg.serialize(s); });
    return stream.getBuffer();
}

// Game with an in-memory connection (same shape as test_game.cpp's Harness).
struct MapHarness {
    Game game;
    std::vector<FakeConnection*> created;

    MapHarness() {
        game.init(nullptr, [this](const std::string& url) -> std::unique_ptr<Connection> {
            auto* c = new FakeConnection(url);
            created.push_back(c);
            return std::unique_ptr<Connection>(c);
        });
    }

    FakeConnection* conn() const { return created.empty() ? nullptr : created.back(); }
};

} // namespace

// The frame that used to crash: MapMsg dispatched while no world exists (the
// scene builds it on join), so no map callback must run against a null world.
TEST(game_map_without_world_callback) {
    MapHarness h;
    int mapCallbacks = 0;
    h.game.setMapCallback([&](const MapMsg&) { ++mapCallbacks; });

    h.game.tryJoinGame("ws://dev/play", "tok");
    h.conn()->open();
    h.game.update(0.0f);

    MapMsg map;
    map.mapName = "main";
    map.seed = 1234;
    map.width = 512;
    map.height = 512;
    h.conn()->pushFrame(makeFrame(MsgType_Map, map));
    h.game.update(0.0f);

    CHECK_EQ(mapCallbacks, 1);
    // The scene replays this cached map into the world it builds on join.
    CHECK(h.game.hasMap());
    CHECK_EQ(h.game.map().mapName, std::string("main"));
}

// GameWorld::replayMap() loads the cache exactly like game.ts's setMapCallback
// did, i.e. the world ends up with terrain instead of staying unloaded.
TEST(gameworld_replay_cached_map) {
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    CHECK(!world.mapLoaded());

    MapMsg map;
    map.mapName = "main";
    map.seed = 99;
    map.width = 256;
    map.height = 256;
    world.replayMap(map);

    CHECK(world.mapLoaded());
    CHECK_EQ(world.map().width, static_cast<uint16_t>(256));
}
