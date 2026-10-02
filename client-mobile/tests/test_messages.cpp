#include "TestFramework.h"
#include "TestHelpers.h"
#include "../src/net/Messages.h"

using namespace surv;
using namespace surv_test;

TEST(join) {
    Buffer b;
    JoinMsg m;
    m.protocol = 1028;
    m.joinToken = "tok_abc123";
    m.name = "TestPlayer";
    m.useTouch = true;
    m.isMobile = true;
    m.outfit = "outfitBase";
    m.melee = "fists";
    m.heal = "bandage";
    m.boost = "soda";
    m.emotes = {"emote_happyface", "emote_thumbsup"};
    m.serialize(b.stream);
    CHECK(matchReference("join", b.hex()));
}

TEST(joined) {
    Buffer b;
    JoinedMsg m;
    m.teamMode = 2;
    m.playerId = 42;
    m.started = true;
    m.emotes = {"emote_happyface", "emote_sadface"};
    m.serialize(b.stream);
    CHECK(matchReference("joined", b.hex()));
}

TEST(input) {
    Buffer b;
    InputMsg m;
    m.seq = 7;
    m.moveUp = true;
    m.moveDown = true;
    m.shootHold = true;
    m.portrait = true;
    m.touchMoveActive = true;
    m.touchMoveDir = Vec2(0.5f, 0.5f);
    m.touchMoveLen = 128;
    m.toMouseDir = Vec2(1.0f, 0.0f);
    m.toMouseLen = 32.0f;
    m.inputs = {1, 2, 3};
    m.useItem = "bandage";
    m.serialize(b.stream);
    CHECK(matchReference("input", b.hex()));
}

TEST(edit) {
    Buffer b;
    EditMsg m;
    m.zoomEnabled = true;
    m.zoom = 2;
    m.speed = 0.5f;
    m.speedEnabled = true;
    m.godMode = true;
    m.serialize(b.stream);
    CHECK(matchReference("edit", b.hex()));
}

TEST(map) {
    Buffer b;
    MapMsg m;
    m.mapName = "main";
    m.seed = 123456;
    m.width = 512;
    m.height = 512;
    m.shoreInset = 4;
    m.grassInset = 2;
    m.rivers.push_back({3, false, {Vec2(10, 20), Vec2(30, 40)}});
    m.places.push_back({"Town", Vec2(0.5f, 0.5f)});
    m.objects.push_back({Vec2(100, 200), 1.0f, "barrel_01", 2});
    GroundPatch g;
    g.bound = Collider::createAabb(Vec2(1, 2), Vec2(3, 4));
    g.color = 0x112233;
    g.roughness = 0.5f;
    g.offsetDist = 0.25f;
    g.order = 3;
    g.useAsMapShape = true;
    m.groundPatches.push_back(g);
    m.serialize(b.stream);
    CHECK(matchReference("map", b.hex()));
}

TEST(update_full) {
    Buffer b;
    UpdateMsg u;

    u.delObjIds = {1, 2, 3};

    FullObjectData fo;
    fo.data.__id = 5;
    fo.data.__type = ObjectType_Player;
    fo.data.pos = Vec2(12.5f, 25.75f);
    fo.data.dir = Vec2(0.1f, -0.99f);
    fo.data.outfit = "outfitBase";
    fo.data.backpack = "backpack00";
    fo.data.helmet = "helmet02";
    fo.data.chest = "chest02";
    fo.data.activeWeapon = "ak47";
    fo.data.layer = 1;
    fo.data.dead = false;
    fo.data.downed = false;
    fo.data.animType = 1;
    fo.data.animSeq = 2;
    fo.data.actionType = 2;
    fo.data.actionSeq = 3;
    fo.data.wearingPan = true;
    fo.data.healEffect = false;
    fo.data.lastStandEffect = false;
    fo.data.frozen = false;
    fo.data.hasteType = 1;
    fo.data.hasteSeq = 1;
    fo.data.actionItem = "bandage";
    fo.data.scale = 1.1f;
    fo.data.role = "";
    fo.data.perks.push_back({"steelskin", false});
    u.fullObjects.push_back(fo);

    u.activePlayerId = 5;
    u.activePlayerIdDirty = true;
    u.activePlayerData.healthDirty = true;
    u.activePlayerData.health = 75.5f;
    u.activePlayerData.zoomDirty = true;
    u.activePlayerData.zoom = 2;
    u.activePlayerData.inventoryDirty = true;
    u.activePlayerData.scope = "2xscope";
    u.activePlayerData.inventory.assign(defs::kBagSizeKeys.size(), 0);
    // find "9mm" and "bandage" indices in kBagSizeKeys
    for (size_t i = 0; i < defs::kBagSizeKeys.size(); i++) {
        if (defs::kBagSizeKeys[i] == "9mm") {
            u.activePlayerData.inventory[i] = 30;
        }
        if (defs::kBagSizeKeys[i] == "bandage") {
            u.activePlayerData.inventory[i] = 3;
        }
    }
    u.activePlayerData.weapsDirty = true;
    u.activePlayerData.curWeapIdx = 1;
    u.activePlayerData.weapons.resize(WeaponSlot_Count);
    u.activePlayerData.weapons[0] = {"ak47", 30};
    u.activePlayerData.weapons[1] = {"fists", 0};
    u.activePlayerData.weapons[2] = {"bandage", 0};
    u.activePlayerData.weapons[3] = {"frag", 0};

    u.gasDirty = true;
    u.gasData.mode = 1;
    u.gasData.duration = 10.5f;
    u.gasData.posOld = Vec2(0, 0);
    u.gasData.posNew = Vec2(100, 100);
    u.gasData.radOld = 50.0f;
    u.gasData.radNew = 25.0f;

    PlayerInfo pi;
    pi.playerId = 5;
    pi.teamId = 1;
    pi.groupId = 0;
    pi.name = "TestPlayer";
    pi.heal = "bandage";
    pi.boost = "soda";
    u.playerInfos.push_back(pi);

    Bullet bl;
    bl.playerId = 5;
    bl.startPos = Vec2(12.5f, 25.75f);
    bl.pos = bl.startPos;
    bl.dir = Vec2(0.1f, -0.99f);
    bl.bulletType = "bullet_ak47";
    bl.layer = 1;
    bl.varianceT = 0.5f;
    bl.distAdjIdx = 2;
    bl.clipDistance = true;
    bl.distance = 100.0f;
    bl.shotFx = true;
    bl.shotSourceType = "ak47";
    bl.shotOffhand = false;
    bl.lastShot = false;
    bl.reflectCount = 1;
    bl.reflectObjId = 9;
    bl.hasModifier = true;
    bl.speedMult = 1.2f;
    bl.distanceMult = 1.5f;
    bl.hasSpecialFx = true;
    bl.shotAlt = true;
    bl.splinter = false;
    bl.trailSaturated = false;
    bl.apRounds = true;
    bl.highVelocity = false;
    bl.combatStims = true;
    bl.trailSmall = false;
    bl.trailThick = true;
    u.bullets.push_back(bl);

    u.killLeaderDirty = true;
    u.killLeaderId = 5;
    u.killLeaderKills = 7;
    u.ack = 9;

    u.serialize(b.stream);
    CHECK(matchReference("update-full", b.hex()));

    // roundtrip: deserialize from the produced bytes and re-serialize
    const size_t written = b.stream.byteIndex();
    Buffer rb(written);
    std::memcpy(rb.bytes.data(), b.bytes.data(), written);
    NetBitStream in(rb.bytes.data(), written);
    UpdateMsg u2;
    u2.deserialize(in);
    Buffer rb2(1024);
    u2.serialize(rb2.stream);
    CHECK(matchReference("update-full", rb2.hex()));
}

TEST(kill) {
    Buffer b;
    KillMsg m;
    m.itemSourceType = "ak47";
    m.mapSourceType = "barrel_01";
    m.damageType = 0;
    m.targetId = 5;
    m.killerId = 6;
    m.killCreditId = 6;
    m.killerKills = 3;
    m.downed = false;
    m.killed = true;
    m.serialize(b.stream);
    CHECK(matchReference("kill", b.hex()));
}

TEST(gameover) {
    Buffer b;
    GameOverMsg m;
    m.teamId = 1;
    m.teamRank = 2;
    m.gameOver = true;
    m.winningTeamId = 1;
    PlayerStatsMsg p;
    p.playerId = 5;
    p.timeAlive = 120;
    p.kills = 3;
    p.dead = true;
    p.damageDealt = 100;
    p.damageTaken = 51;
    m.playerStats.push_back(p);
    m.serialize(b.stream);
    CHECK(matchReference("gameover", b.hex()));
}

TEST(emote) {
    Buffer b;
    EmoteMsg m;
    m.pos = Vec2(10, 20);
    m.type = "emote_happyface";
    m.isPing = true;
    m.serialize(b.stream);
    CHECK(matchReference("emote", b.hex()));
}

TEST(pickup) {
    Buffer b;
    PickupMsg m;
    m.type = 4;
    m.item = "ak47";
    m.count = 1;
    m.serialize(b.stream);
    CHECK(matchReference("pickup", b.hex()));
}

TEST(dropitem) {
    Buffer b;
    DropItemMsg m;
    m.item = "ak47";
    m.weapIdx = 1;
    m.serialize(b.stream);
    CHECK(matchReference("dropitem", b.hex()));
}

TEST(perkmoderoleselect) {
    Buffer b;
    PerkModeRoleSelectMsg m;
    m.role = "medic";
    m.serialize(b.stream);
    CHECK(matchReference("perkmoderoleselect", b.hex()));
}

TEST(roleannouncement) {
    Buffer b;
    RoleAnnouncementMsg m;
    m.playerId = 1;
    m.killerId = 2;
    m.role = "medic";
    m.assigned = true;
    m.killed = false;
    m.serialize(b.stream);
    CHECK(matchReference("roleannouncement", b.hex()));
}

TEST(alivecounts) {
    Buffer b;
    AliveCountsMsg m;
    m.teamAliveCounts = {4, 2, 3, 1};
    m.serialize(b.stream);
    CHECK(matchReference("alivecounts", b.hex()));
}

TEST(spectate) {
    Buffer b;
    SpectateMsg m;
    m.action = 1;
    m.serialize(b.stream);
    CHECK(matchReference("spectate", b.hex()));
}

TEST(msgstream_join) {
    MsgStream ms(std::vector<uint8_t>(256, 0));
    ms.serializeMsg(MsgType_Join, [&](NetBitStream& s) {
        JoinMsg m;
        m.protocol = 1028;
        m.joinToken = "tok";
        m.name = "Bob";
        m.outfit = "outfitBase";
        m.melee = "fists";
        m.heal = "bandage";
        m.boost = "soda";
        m.serialize(s);
    });
    std::vector<uint8_t> out = ms.getBuffer();
    std::string hex;
    const char* hx = "0123456789abcdef";
    for (uint8_t byte : out) {
        hex.push_back(hx[byte >> 4]);
        hex.push_back(hx[byte & 0x0f]);
    }
    CHECK(matchReference("msgstream-join", hex));
}