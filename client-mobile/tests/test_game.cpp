// M3 host tests: the Game connection lifecycle, frame pump and message
// dispatch, driven through an in-memory Connection (no axmol required).
#include "TestFramework.h"
#include "FakeConnection.h"
#include "../src/game/Game.h"

#include <memory>
#include <string>
#include <vector>

using namespace surv;
using namespace surv_test;

namespace {

template <typename T>
std::vector<uint8_t> makeFrame(MsgType type, T& msg) {
    MsgStream stream(std::vector<uint8_t>(1024, 0));
    stream.serializeMsg(type, [&msg](NetBitStream& s) { msg.serialize(s); });
    return stream.getBuffer();
}

// Builds a small but valid UpdateMsg: one active player + one full player object.
UpdateMsg makeUpdate() {
    UpdateMsg u;

    FullObjectData fo;
    fo.data.__id = 42;
    fo.data.__type = ObjectType_Player;
    fo.data.pos = Vec2(12.5f, 25.75f);
    fo.data.dir = Vec2(0.1f, -0.99f);
    fo.data.outfit = "outfitBase";
    fo.data.backpack = "backpack00";
    fo.data.helmet = "helmet02";
    fo.data.chest = "chest02";
    fo.data.activeWeapon = "ak47";
    fo.data.layer = 1;
    fo.data.scale = 1.1f;
    fo.data.role = "";
    fo.data.perks.push_back({"steelskin", false});
    u.fullObjects.push_back(fo);

    u.activePlayerId = 42;
    u.activePlayerIdDirty = true;
    u.activePlayerData.healthDirty = true;
    u.activePlayerData.health = 100.0f;

    PlayerInfo pi;
    pi.playerId = 42;
    pi.teamId = 2;
    pi.groupId = 0;
    pi.name = "Player";
    pi.heal = "bandage";
    pi.boost = "soda";
    u.playerInfos.push_back(pi);

    u.ack = 3;
    return u;
}

struct Harness {
    Game game;
    std::vector<FakeConnection*> created;

    Harness() {
        game.init(nullptr, [this](const std::string& url) -> std::unique_ptr<Connection> {
            auto* c = new FakeConnection(url);
            created.push_back(c);
            return std::unique_ptr<Connection>(c);
        });
    }

    FakeConnection* conn() const { return created.empty() ? nullptr : created.back(); }
};

} // namespace

TEST(game_join_sends_joinmsg) {
    Harness h;
    // M7: the menu supplies the join identity; check a custom loadout round-trips.
    Game::JoinInfo info;
    info.name = "Player";
    info.outfit = "outfitBase";
    info.melee = "fists";
    info.heal = "bandage";
    info.boost = "soda";
    h.game.setJoinInfo(info);
    h.game.tryJoinGame("ws://dev.example:9000/play", "tok-123");
    CHECK_EQ(h.created.size(), static_cast<size_t>(1));
    CHECK_EQ(h.conn()->url, std::string("ws://dev.example:9000/play"));

    // Nothing is sent until the socket opens.
    CHECK_EQ(h.conn()->sent.size(), static_cast<size_t>(0));

    h.conn()->open();
    h.game.update(0.0f);

    CHECK(h.game.isConnected());
    CHECK_EQ(h.conn()->sent.size(), static_cast<size_t>(1));

    MsgStream ms(h.conn()->sent[0]);
    CHECK_EQ(static_cast<int>(ms.deserializeMsgType()), static_cast<int>(MsgType_Join));
    JoinMsg jm;
    jm.deserialize(ms.getStream());
    CHECK_EQ(jm.protocol, static_cast<uint32_t>(defs::kProtocolVersion));
    CHECK_EQ(jm.joinToken, std::string("tok-123"));
    CHECK_EQ(jm.name, std::string("Player"));
    CHECK(jm.useTouch);
    CHECK(jm.isMobile);
    CHECK(!jm.bot);
    CHECK_EQ(jm.outfit, std::string("outfitBase"));
    CHECK_EQ(jm.melee, std::string("fists"));
    CHECK_EQ(jm.heal, std::string("bandage"));
    CHECK_EQ(jm.boost, std::string("soda"));
}

TEST(game_hud_preserves_delta_inventory_and_health) {
    Harness h;
    h.game.tryJoinGame("ws://dev/play", "tok");
    h.conn()->open();
    auto update = makeUpdate();
    update.activePlayerData.boostDirty = true;
    update.activePlayerData.boost = 50;
    update.activePlayerData.weapsDirty = true;
    update.activePlayerData.curWeapIdx = 0;
    update.activePlayerData.weapons.resize(WeaponSlot_Count);
    update.activePlayerData.weapons[0] = {"ak47", 20};
    update.activePlayerData.inventoryDirty = true;
    update.activePlayerData.inventory.assign(defs::kBagSizeKeys.size(), 10);
    h.conn()->pushFrame(makeFrame(MsgType_Update, update));
    h.game.update(0);
    UpdateMsg clean;
    h.conn()->pushFrame(makeFrame(MsgType_Update, clean));
    h.game.update(0);
    CHECK_NEAR(h.game.activePlayerData().health, 100, 0.01f);
    CHECK_NEAR(h.game.activePlayerData().boost, 50, 0.3f);
    CHECK_EQ(h.game.activePlayerData().weapons[0].type, std::string("ak47"));
    CHECK_EQ(h.game.activePlayerData().weapons[0].ammo, uint8_t(20));
    CHECK_EQ(h.game.activePlayerData().inventory[0], uint16_t(10));
    h.game.free();
    CHECK(h.game.activePlayerData().weapons.empty());
    CHECK_NEAR(h.game.activePlayerData().health, 0, 0.01f);
}

TEST(game_hud_killfeed_counts_and_gameover) {
    Harness h;
    h.game.tryJoinGame("ws://dev/play", "tok");
    h.conn()->open();
    auto update = makeUpdate();
    PlayerInfo target;
    target.playerId = 43;
    target.name = "Target";
    update.playerInfos.push_back(target);
    h.conn()->pushFrame(makeFrame(MsgType_Update, update));
    h.game.update(0);
    CHECK_EQ(h.game.aliveCounts()[0], 2);
    KillMsg kill;
    kill.killerId = 42;
    kill.targetId = 43;
    kill.killed = true;
    kill.itemSourceType = "ak47";
    for (int i = 0; i < 7; ++i) h.conn()->pushFrame(makeFrame(MsgType_Kill, kill));
    h.game.update(0);
    CHECK_EQ(h.game.killFeed().size(), size_t(6));
    CHECK_EQ(h.game.killFeed()[0].killerName, std::string("Player"));
    CHECK_EQ(h.game.killFeed()[0].targetName, std::string("Target"));
    CHECK_EQ(h.game.aliveCounts()[0], 1);
    // Existing AliveCounts must be consumed without corrupting the next message.
    AliveCountsMsg alive;
    alive.teamAliveCounts = {12, 13};
    GameOverMsg over;
    over.teamRank = 2;
    MsgStream stream(std::vector<uint8_t>(1024, 0));
    stream.serializeMsg(MsgType_AliveCounts, [&alive](NetBitStream& s) { alive.serialize(s); });
    stream.serializeMsg(MsgType_GameOver, [&over](NetBitStream& s) { over.serialize(s); });
    h.conn()->pushFrame(stream.getBuffer());
    h.game.update(0);
    CHECK_EQ(h.game.aliveCounts()[0], 12);
    CHECK_EQ(h.game.aliveCounts()[1], 13);
    CHECK(h.game.gameOver() != nullptr);
    CHECK_EQ(h.game.gameOver()->teamRank, uint8_t(2));
    h.game.update(9);
    CHECK(h.game.killFeed().empty());
    h.game.free();
    CHECK(h.game.gameOver() == nullptr);
    CHECK_EQ(h.game.aliveCounts()[0], 0);
}

TEST(game_dispatch_joined_map_update) {
    Harness h;
    h.game.tryJoinGame("ws://dev/play", "tok");
    h.conn()->open();
    h.game.update(0.0f);

    JoinedMsg joined;
    joined.teamMode = 2;
    joined.playerId = 42;
    joined.started = true;
    joined.emotes = {"emote_happyface"};
    h.conn()->pushFrame(makeFrame(MsgType_Joined, joined));

    MapMsg map;
    map.mapName = "main";
    map.seed = 1234;
    map.width = 512;
    map.height = 512;
    h.conn()->pushFrame(makeFrame(MsgType_Map, map));

    UpdateMsg update = makeUpdate();
    h.conn()->pushFrame(makeFrame(MsgType_Update, update));

    h.game.update(0.0f);

    CHECK_EQ(h.game.getLocalPlayerId(), static_cast<uint16_t>(42));
    CHECK_EQ(h.game.getActivePlayerId(), static_cast<uint16_t>(42));
    CHECK(h.game.isPlaying());

    const GameStateSnapshot snap = h.game.snapshot();
    CHECK(snap.playing);
    CHECK_EQ(snap.activePlayerId, static_cast<uint16_t>(42));
    CHECK_EQ(snap.localPlayerId, static_cast<uint16_t>(42));
    CHECK_EQ(snap.health, 100.0f);
    CHECK_EQ(snap.players.size(), static_cast<size_t>(1));
    CHECK_EQ(snap.players[0].playerId, static_cast<uint16_t>(42));
    CHECK_EQ(snap.players[0].teamId, static_cast<uint8_t>(2));
    CHECK_EQ(snap.players[0].name, std::string("Player"));
    CHECK_EQ(snap.objects.size(), static_cast<size_t>(1));
    CHECK_EQ(snap.objects[0].id, static_cast<uint16_t>(42));
    CHECK_EQ(static_cast<int>(snap.objects[0].type), static_cast<int>(ObjectType_Player));
    CHECK_NEAR(snap.objects[0].pos.x, 12.5f, 0.1f);
    CHECK_NEAR(snap.objects[0].pos.y, 25.75f, 0.1f);
}

TEST(game_multiple_messages_in_one_frame) {
    Harness h;
    h.game.tryJoinGame("ws://dev/play", "tok");
    h.conn()->open();
    h.game.update(0.0f);

    JoinedMsg joined;
    joined.teamMode = 1;
    joined.playerId = 7;
    joined.started = true;
    std::vector<uint8_t> frame = makeFrame(MsgType_Joined, joined);

    UpdateMsg update = makeUpdate();
    std::vector<uint8_t> second = makeFrame(MsgType_Update, update);
    frame.insert(frame.end(), second.begin(), second.end());

    h.conn()->pushFrame(std::move(frame));
    h.game.update(0.0f);

    CHECK_EQ(h.game.getLocalPlayerId(), static_cast<uint16_t>(7));
    CHECK(h.game.isPlaying());
    CHECK_EQ(h.game.snapshot().objects.size(), static_cast<size_t>(1));
}

TEST(game_snapshot_text_is_deterministic) {
    Harness h;
    h.game.tryJoinGame("ws://dev/play", "tok");
    h.conn()->open();
    h.game.update(0.0f);

    JoinedMsg joined;
    joined.teamMode = 2;
    joined.playerId = 42;
    joined.started = true;
    h.conn()->pushFrame(makeFrame(MsgType_Joined, joined));

    UpdateMsg update = makeUpdate();
    h.conn()->pushFrame(makeFrame(MsgType_Update, update));
    h.game.update(0.0f);

    const std::string a = h.game.snapshotText();
    const std::string b = h.game.snapshotText();
    CHECK_EQ(a, b);
    CHECK(a.find("active=42 local=42 playing=1 health=100.000") != std::string::npos);
    CHECK(a.find("player 42 team=2 group=0 name=Player") != std::string::npos);
    CHECK(a.find("obj 42 type=") != std::string::npos);
}

TEST(game_gameover_stops_playing) {
    Harness h;
    h.game.tryJoinGame("ws://dev/play", "tok");
    h.conn()->open();
    h.game.update(0.0f);

    UpdateMsg update = makeUpdate();
    h.conn()->pushFrame(makeFrame(MsgType_Update, update));
    h.game.update(0.0f);
    CHECK(h.game.isPlaying());

    GameOverMsg over;
    over.teamId = 2;
    over.teamRank = 1;
    over.gameOver = true;
    over.winningTeamId = 2;
    h.conn()->pushFrame(makeFrame(MsgType_GameOver, over));
    h.game.update(0.0f);

    CHECK(!h.game.isPlaying());
}

TEST(game_pause_closes_and_resume_rejoins) {
    Harness h;
    h.game.tryJoinGame("ws://dev/play", "tok");
    h.conn()->open();
    h.game.update(0.0f);
    CHECK(h.game.isConnected());

    FakeConnection* first = h.conn();
    h.game.pause();
    CHECK(h.game.isPaused());
    CHECK_EQ(first->closeCount, 1);
    CHECK_EQ(first->closeReason, std::string("background"));

    h.game.resume();
    CHECK(!h.game.isPaused());
    CHECK_EQ(h.created.size(), static_cast<size_t>(2));
    CHECK_EQ(h.conn()->url, std::string("ws://dev/play"));

    // The rejoined socket re-sends the JoinMsg on open.
    h.conn()->open();
    h.game.update(0.0f);
    CHECK_EQ(h.conn()->sent.size(), static_cast<size_t>(1));
}

TEST(game_close_resets_connected_state) {
    Harness h;
    h.game.tryJoinGame("ws://dev/play", "tok");
    h.conn()->open();
    h.game.update(0.0f);
    CHECK(h.game.isConnected());

    h.conn()->pushClose(3000, "server_restart");
    h.game.update(0.0f);

    CHECK(!h.game.isConnected());
    CHECK_EQ(h.game.getCloseCode(), static_cast<uint16_t>(3000));
    CHECK_EQ(h.game.getCloseReason(), std::string("server_restart"));
}
