#pragma once
// Port of shared/net/*Msg.ts message classes.
#include "../core/GameConfig.h"
#include "../core/Vec2.h"
#include "Net.h"
#include "ObjectSerializeFns.h"
#include <cstdint>
#include <string>
#include <vector>

namespace surv {

// AbstractMsg equivalent.
struct Msg {
    virtual void serialize(NetBitStream& s) { (void)s; }
    virtual void deserialize(NetBitStream& s) { (void)s; }
    virtual ~Msg() = default;
};

// shared/net/joinMsg.ts
struct JoinMsg : Msg {
    uint32_t protocol = 0;
    std::string joinToken;
    std::string name;
    bool useTouch = false;
    bool isMobile = false;
    bool bot = false;
    std::string outfit;
    std::string melee;
    std::string heal;
    std::string boost;
    std::vector<std::string> emotes;

    void serialize(NetBitStream& s) override {
        // PROTOCOL VERSION SHOULD ALWAYS BE THE FIRST WITH THE SAME SIZE
        s.writeUint32(protocol);
        s.writeString(joinToken);
        s.writeString(name, Constants::PlayerNameMaxLen);
        s.writeBoolean(useTouch);
        s.writeBoolean(isMobile);
        s.writeBoolean(bot);
        s.writeGameType(outfit);
        s.writeGameType(melee);
        s.writeGameType(heal);
        s.writeGameType(boost);
        s.writeBits(static_cast<uint32_t>(emotes.size()), 8);
        for (const auto& emote : emotes) {
            s.writeGameType(emote);
        }
    }

    void deserialize(NetBitStream& s) override {
        protocol = s.readUint32();
        joinToken = s.readString();
        name = s.readString(Constants::PlayerNameMaxLen);
        useTouch = s.readBoolean();
        isMobile = s.readBoolean();
        bot = s.readBoolean();
        outfit = s.readGameType();
        melee = s.readGameType();
        heal = s.readGameType();
        boost = s.readGameType();
        const uint32_t n = s.readBits(8);
        emotes.clear();
        emotes.reserve(n);
        for (uint32_t i = 0; i < n; i++) {
            emotes.push_back(s.readGameType());
        }
    }
};

// shared/net/joinedMsg.ts
struct JoinedMsg : Msg {
    uint8_t teamMode = 0;
    uint16_t playerId = 0;
    bool started = false;
    std::vector<std::string> emotes;

    void serialize(NetBitStream& s) override {
        s.writeUint8(teamMode);
        s.writeUint16(playerId);
        s.writeBoolean(started);
        s.writeBits(static_cast<uint32_t>(emotes.size()), 8);
        for (const auto& emote : emotes) {
            s.writeGameType(emote);
        }
    }

    void deserialize(NetBitStream& s) override {
        teamMode = s.readUint8();
        playerId = s.readUint16();
        started = s.readBoolean();
        const uint32_t n = s.readBits(8);
        emotes.clear();
        emotes.reserve(n);
        for (uint32_t i = 0; i < n; i++) {
            emotes.push_back(s.readGameType());
        }
    }
};

// shared/net/inputMsg.ts
struct InputMsg : Msg {
    uint8_t seq = 0;
    bool moveLeft = false;
    bool moveRight = false;
    bool moveUp = false;
    bool moveDown = false;
    bool shootStart = false;
    bool shootHold = false;
    bool portrait = false;
    bool touchMoveActive = false;
    Vec2 touchMoveDir = Vec2(1.0f, 0.0f);
    uint8_t touchMoveLen = 255;
    Vec2 toMouseDir = Vec2(1.0f, 0.0f);
    float toMouseLen = 0.0f;
    std::vector<uint8_t> inputs;
    std::string useItem;

    void addInput(uint8_t input) {
        if (inputs.size() < 7) {
            for (uint8_t v : inputs) {
                if (v == input) {
                    return;
                }
            }
            inputs.push_back(input);
        }
    }

    void serialize(NetBitStream& s) override {
        s.writeUint8(seq);
        s.writeBoolean(moveLeft);
        s.writeBoolean(moveRight);
        s.writeBoolean(moveUp);
        s.writeBoolean(moveDown);
        s.writeBoolean(shootStart);
        s.writeBoolean(shootHold);
        s.writeBoolean(portrait);
        s.writeBoolean(touchMoveActive);
        if (touchMoveActive) {
            s.writeUnitVec(touchMoveDir, 8);
            s.writeUint8(touchMoveLen);
        }
        s.writeUnitVec(toMouseDir, 10);
        s.writeFloat(toMouseLen, 0.0f, Constants::MouseMaxDist, 8);
        s.writeBits(static_cast<uint32_t>(inputs.size()), 4);
        for (uint8_t v : inputs) {
            s.writeUint8(v);
        }
        s.writeGameType(useItem);
    }

    void deserialize(NetBitStream& s) override {
        seq = s.readUint8();
        moveLeft = s.readBoolean();
        moveRight = s.readBoolean();
        moveUp = s.readBoolean();
        moveDown = s.readBoolean();
        shootStart = s.readBoolean();
        shootHold = s.readBoolean();
        portrait = s.readBoolean();
        touchMoveActive = s.readBoolean();
        if (touchMoveActive) {
            touchMoveDir = s.readUnitVec(8);
            touchMoveLen = s.readUint8();
        }
        toMouseDir = s.readUnitVec(10);
        toMouseLen = s.readFloat(0.0f, Constants::MouseMaxDist, 8);
        const uint32_t n = s.readBits(4);
        inputs.clear();
        inputs.reserve(n);
        for (uint32_t i = 0; i < n; i++) {
            inputs.push_back(s.readUint8());
        }
        useItem = s.readGameType();
    }
};

// shared/net/editMsg.ts
struct EditMsg : Msg {
    bool zoomEnabled = false;
    uint8_t zoom = 1;
    bool speedEnabled = false;
    float speed = 0.0f;
    bool gameSpeedEnabled = false;
    float gameSpeed = 1.0f;
    bool loadNewMap = false;
    uint32_t newMapSeed = 0;
    std::string spawnLootType;
    bool promoteToRole = false;
    std::string promoteToRoleType;
    bool toggleLayer = false;
    bool noClip = false;
    bool teleportToPings = false;
    bool godMode = false;
    bool moveObjs = false;
    bool preventGameStart = false;

    void serialize(NetBitStream& s) override {
        s.writeBoolean(zoomEnabled);
        if (zoomEnabled) {
            s.writeUint8(zoom);
        }
        s.writeBoolean(speedEnabled);
        if (speedEnabled) {
            s.writeFloat32(speed);
        }
        s.writeBoolean(gameSpeedEnabled);
        if (gameSpeedEnabled) {
            s.writeFloat32(gameSpeed);
        }
        s.writeBoolean(loadNewMap);
        if (loadNewMap) {
            s.writeUint32(newMapSeed);
        }
        s.writeGameType(spawnLootType);
        s.writeBoolean(promoteToRole);
        if (promoteToRole) {
            s.writeGameType(promoteToRoleType);
        }
        s.writeBoolean(toggleLayer);
        s.writeBoolean(noClip);
        s.writeBoolean(teleportToPings);
        s.writeBoolean(godMode);
        s.writeBoolean(moveObjs);
        s.writeBoolean(preventGameStart);
    }

    void deserialize(NetBitStream& s) override {
        zoomEnabled = s.readBoolean();
        if (zoomEnabled) {
            zoom = s.readUint8();
        }
        speedEnabled = s.readBoolean();
        if (speedEnabled) {
            speed = s.readFloat32();
        }
        gameSpeedEnabled = s.readBoolean();
        if (gameSpeedEnabled) {
            gameSpeed = s.readFloat32();
        }
        loadNewMap = s.readBoolean();
        if (loadNewMap) {
            newMapSeed = s.readUint32();
        }
        spawnLootType = s.readGameType();
        promoteToRole = s.readBoolean();
        if (promoteToRole) {
            promoteToRoleType = s.readGameType();
        }
        toggleLayer = s.readBoolean();
        noClip = s.readBoolean();
        teleportToPings = s.readBoolean();
        godMode = s.readBoolean();
        moveObjs = s.readBoolean();
        preventGameStart = s.readBoolean();
    }
};

// shared/net/spectateMsg.ts
enum SpectateAction : uint8_t {
    SpectateAction_None,
    SpectateAction_Begin,
    SpectateAction_Next,
    SpectateAction_Prev,
};

struct SpectateMsg : Msg {
    uint8_t action = SpectateAction_None;

    void serialize(NetBitStream& s) override {
        s.writeUint8(action);
    }

    void deserialize(NetBitStream& s) override {
        action = s.readUint8();
    }
};

// shared/net/dropItemMsg.ts
struct DropItemMsg : Msg {
    std::string item;
    uint8_t weapIdx = 0;

    void serialize(NetBitStream& s) override {
        s.writeGameType(item);
        s.writeUint8(weapIdx);
    }

    void deserialize(NetBitStream& s) override {
        item = s.readGameType();
        weapIdx = s.readUint8();
    }
};

// shared/net/emoteMsg.ts
struct EmoteMsg : Msg {
    Vec2 pos;
    std::string type;
    bool isPing = false;

    void serialize(NetBitStream& s) override {
        s.writeVec(pos, 0, 0, 1024, 1024, 16);
        s.writeGameType(type);
        s.writeBoolean(isPing);
    }

    void deserialize(NetBitStream& s) override {
        pos = s.readVec(0, 0, 1024, 1024, 16);
        type = s.readGameType();
        isPing = s.readBoolean();
    }
};

// shared/net/pickupMsg.ts
struct PickupMsg : Msg {
    uint8_t type = 0;
    std::string item;
    uint8_t count = 0;

    void serialize(NetBitStream& s) override {
        s.writeUint8(type);
        s.writeGameType(item);
        s.writeUint8(count);
    }

    void deserialize(NetBitStream& s) override {
        type = s.readUint8();
        item = s.readGameType();
        count = s.readUint8();
    }
};

// shared/net/playerStatsMsg.ts
struct PlayerStatsMsg : Msg {
    uint16_t playerId = 0;
    uint16_t timeAlive = 0;
    uint8_t kills = 0;
    bool dead = false;
    uint16_t damageDealt = 0;
    uint16_t damageTaken = 0;

    void serialize(NetBitStream& s) override {
        s.writeUint16(playerId);
        s.writeUint16(timeAlive);
        s.writeUint8(kills);
        s.writeUint8(dead ? 1 : 0);
        s.writeUint16(damageDealt);
        s.writeUint16(damageTaken);
    }

    void deserialize(NetBitStream& s) override {
        playerId = s.readUint16();
        timeAlive = s.readUint16();
        kills = s.readUint8();
        dead = !!s.readUint8();
        damageDealt = s.readUint16();
        damageTaken = s.readUint16();
    }
};

// shared/net/killMsg.ts
struct KillMsg : Msg {
    std::string itemSourceType;
    std::string mapSourceType;
    uint8_t damageType = DamageType_Player;
    uint16_t targetId = 0;
    uint16_t killerId = 0;
    uint16_t killCreditId = 0;
    uint8_t killerKills = 0;
    bool downed = false;
    bool killed = false;

    void serialize(NetBitStream& s) override {
        s.writeUint8(damageType);
        s.writeGameType(itemSourceType);
        s.writeMapType(mapSourceType);
        s.writeUint16(targetId);
        s.writeUint16(killerId);
        s.writeUint16(killCreditId);
        s.writeUint8(killerKills);
        s.writeBoolean(downed);
        s.writeBoolean(killed);
    }

    void deserialize(NetBitStream& s) override {
        damageType = s.readUint8();
        itemSourceType = s.readGameType();
        mapSourceType = s.readMapType();
        targetId = s.readUint16();
        killerId = s.readUint16();
        killCreditId = s.readUint16();
        killerKills = s.readUint8();
        downed = s.readBoolean();
        killed = s.readBoolean();
    }
};

// shared/net/roleAnnouncementMsg.ts
struct RoleAnnouncementMsg : Msg {
    uint16_t playerId = 0;
    uint16_t killerId = 0;
    std::string role;
    bool assigned = false;
    bool killed = false;

    void serialize(NetBitStream& s) override {
        s.writeUint16(playerId);
        s.writeUint16(killerId);
        s.writeGameType(role);
        s.writeBoolean(assigned);
        s.writeBoolean(killed);
    }

    void deserialize(NetBitStream& s) override {
        playerId = s.readUint16();
        killerId = s.readUint16();
        role = s.readGameType();
        assigned = s.readBoolean();
        killed = s.readBoolean();
    }
};

// shared/net/perkModeRoleSelectMsg.ts
struct PerkModeRoleSelectMsg : Msg {
    std::string role;

    void serialize(NetBitStream& s) override {
        s.writeGameType(role);
        s.writeBits(0u, 6);
    }

    void deserialize(NetBitStream& s) override {
        role = s.readGameType();
        s.readBits(6);
    }
};

// shared/net/aliveCountsMsg.ts
struct AliveCountsMsg : Msg {
    std::vector<uint8_t> teamAliveCounts;

    void serialize(NetBitStream& s) override {
        s.writeBits(static_cast<uint32_t>(teamAliveCounts.size()), 8);
        for (uint8_t v : teamAliveCounts) {
            s.writeUint8(v);
        }
    }

    void deserialize(NetBitStream& s) override {
        const uint32_t n = s.readBits(8);
        teamAliveCounts.clear();
        teamAliveCounts.reserve(n);
        for (uint32_t i = 0; i < n; i++) {
            teamAliveCounts.push_back(s.readUint8());
        }
    }
};

// shared/net/updatePassMsg.ts
struct UpdatePassMsg : Msg {
    void serialize(NetBitStream& s) override { (void)s; }
    void deserialize(NetBitStream& s) override { (void)s; }
};

// shared/net/gameOverMsg.ts
struct GameOverMsg : Msg {
    uint8_t teamId = 0;
    uint8_t teamRank = 0;
    bool gameOver = false;
    uint8_t winningTeamId = 0;
    std::vector<PlayerStatsMsg> playerStats;

    void serialize(NetBitStream& s) override {
        s.writeUint8(teamId);
        s.writeUint8(teamRank);
        s.writeUint8(gameOver ? 1 : 0);
        s.writeUint8(winningTeamId);
        s.writeBits(static_cast<uint32_t>(playerStats.size()), 8);
        for (auto& stats : playerStats) {
            stats.serialize(s);
        }
    }

    void deserialize(NetBitStream& s) override {
        teamId = s.readUint8();
        teamRank = s.readUint8();
        gameOver = !!s.readUint8();
        winningTeamId = s.readUint8();
        const uint32_t n = s.readBits(8);
        playerStats.clear();
        playerStats.resize(n);
        for (uint32_t i = 0; i < n; i++) {
            playerStats[i].deserialize(s);
        }
    }
};

// shared/net/mapMsg.ts
struct MapRiverData {
    uint8_t width = 0;
    bool looped = false;
    std::vector<Vec2> points;
};

struct MapPlace {
    std::string name;
    Vec2 pos;
};

struct GroundPatch {
    Collider bound;
    uint32_t color = 0;
    float roughness = 0.0f;
    float offsetDist = 0.0f;
    int order = 0;
    bool useAsMapShape = false;
};

struct MapObj {
    Vec2 pos;
    float scale = 1.0f;
    std::string type;
    int ori = 0;
};

struct MapMsg : Msg {
    std::string mapName;
    uint32_t seed = 0;
    uint16_t width = 0;
    uint16_t height = 0;
    uint16_t shoreInset = 0;
    uint16_t grassInset = 0;
    std::vector<MapRiverData> rivers;
    std::vector<MapPlace> places;
    std::vector<MapObj> objects;
    std::vector<GroundPatch> groundPatches;

    static void serializeMapRiver(NetBitStream& s, const MapRiverData& data) {
        s.writeUint8(data.width);
        s.writeUint8(data.looped ? 1 : 0);
        s.writeBits(static_cast<uint32_t>(data.points.size()), 8);
        for (const auto& pos : data.points) {
            s.writeMapPos(pos);
        }
    }

    static MapRiverData deserializeMapRiver(NetBitStream& s) {
        MapRiverData d;
        d.width = s.readUint8();
        d.looped = !!s.readUint8();
        const uint32_t n = s.readBits(8);
        d.points.reserve(n);
        for (uint32_t i = 0; i < n; i++) {
            d.points.push_back(s.readMapPos());
        }
        return d;
    }

    static void serializeMapPlace(NetBitStream& s, const MapPlace& place) {
        s.writeString(place.name);
        s.writeVec(place.pos, 0, 0, 1, 1, 16);
    }

    static MapPlace deserializeMapPlace(NetBitStream& s) {
        MapPlace p;
        p.name = s.readString();
        p.pos = s.readVec(0, 0, 1, 1, 16);
        return p;
    }

    static void serializeMapGroundPatch(NetBitStream& s, const GroundPatch& patch) {
        s.writeCollider(patch.bound);
        s.writeUint32(patch.color);
        s.writeFloat32(patch.roughness);
        s.writeFloat32(patch.offsetDist);
        s.writeBits(static_cast<uint32_t>(patch.order), 7);
        s.writeBoolean(patch.useAsMapShape);
    }

    static GroundPatch deserializeMapGroundPatch(NetBitStream& s) {
        GroundPatch p;
        p.bound = s.readCollider();
        p.color = s.readUint32();
        p.roughness = s.readFloat32();
        p.offsetDist = s.readFloat32();
        p.order = static_cast<int>(s.readBits(7));
        p.useAsMapShape = s.readBoolean();
        return p;
    }

    static void serializeMapObj(NetBitStream& s, const MapObj& obj) {
        s.writeMapPos(obj.pos);
        s.writeFloat(obj.scale, Constants::MapObjectMinScale, Constants::MapObjectMaxScale, 8);
        s.writeMapType(obj.type);
        s.writeBits(static_cast<uint32_t>(obj.ori), 2);
        s.writeAlignToNextByte();
    }

    static MapObj deserializeMapObj(NetBitStream& s) {
        MapObj obj;
        obj.pos = s.readMapPos();
        obj.scale = s.readFloat(Constants::MapObjectMinScale, Constants::MapObjectMaxScale, 8);
        obj.type = s.readMapType();
        obj.ori = static_cast<int>(s.readBits(2));
        s.readAlignToNextByte();
        return obj;
    }

    void serialize(NetBitStream& s) override {
        s.writeString(mapName, Constants::MapNameMaxLen);
        s.writeUint32(seed);
        s.writeUint16(width);
        s.writeUint16(height);
        s.writeUint16(shoreInset);
        s.writeUint16(grassInset);
        s.writeBits(static_cast<uint32_t>(rivers.size()), 8);
        for (const auto& river : rivers) {
            serializeMapRiver(s, river);
        }
        s.writeBits(static_cast<uint32_t>(places.size()), 8);
        for (const auto& place : places) {
            serializeMapPlace(s, place);
        }
        s.writeBits(static_cast<uint32_t>(objects.size()), 16);
        for (const auto& obj : objects) {
            serializeMapObj(s, obj);
        }
        s.writeBits(static_cast<uint32_t>(groundPatches.size()), 8);
        for (const auto& patch : groundPatches) {
            serializeMapGroundPatch(s, patch);
        }
    }

    void deserialize(NetBitStream& s) override {
        mapName = s.readString(Constants::MapNameMaxLen);
        seed = s.readUint32();
        width = s.readUint16();
        height = s.readUint16();
        shoreInset = s.readUint16();
        grassInset = s.readUint16();
        uint32_t n = s.readBits(8);
        rivers.clear();
        rivers.reserve(n);
        for (uint32_t i = 0; i < n; i++) {
            rivers.push_back(deserializeMapRiver(s));
        }
        n = s.readBits(8);
        places.clear();
        places.reserve(n);
        for (uint32_t i = 0; i < n; i++) {
            places.push_back(deserializeMapPlace(s));
        }
        n = s.readBits(16);
        objects.clear();
        objects.reserve(n);
        for (uint32_t i = 0; i < n; i++) {
            objects.push_back(deserializeMapObj(s));
        }
        n = s.readBits(8);
        groundPatches.clear();
        groundPatches.reserve(n);
        for (uint32_t i = 0; i < n; i++) {
            groundPatches.push_back(deserializeMapGroundPatch(s));
        }
    }
};

// shared/net/updateMsg.ts
struct UpdateExtFlags {
    static constexpr uint32_t DeletedObjects = 1u << 0;
    static constexpr uint32_t FullObjects = 1u << 1;
    static constexpr uint32_t ActivePlayerId = 1u << 2;
    static constexpr uint32_t Gas = 1u << 3;
    static constexpr uint32_t GasCircle = 1u << 4;
    static constexpr uint32_t PlayerInfos = 1u << 5;
    static constexpr uint32_t DeletePlayerIds = 1u << 6;
    static constexpr uint32_t PlayerStatus = 1u << 7;
    static constexpr uint32_t GroupStatus = 1u << 8;
    static constexpr uint32_t Bullets = 1u << 9;
    static constexpr uint32_t Explosions = 1u << 10;
    static constexpr uint32_t Emotes = 1u << 11;
    static constexpr uint32_t Planes = 1u << 12;
    static constexpr uint32_t AirstrikeZones = 1u << 13;
    static constexpr uint32_t MapIndicators = 1u << 14;
    static constexpr uint32_t KillLeader = 1u << 15;
};

struct Bullet {
    uint16_t playerId = 0;
    Vec2 startPos;
    Vec2 pos;
    Vec2 dir;
    std::string bulletType;
    int layer = 0;
    float varianceT = 0.0f;
    int distAdjIdx = 0;
    bool clipDistance = false;
    float distance = 0.0f;
    bool shotFx = false;
    std::string shotSourceType;
    bool shotOffhand = false;
    bool lastShot = false;
    int reflectCount = 0;
    uint16_t reflectObjId = 0;
    float speedMult = 1.0f;
    float distanceMult = 1.0f;
    bool hasModifier = false;
    bool hasSpecialFx = false;
    bool shotAlt = false;
    bool splinter = false;
    bool trailSaturated = false;
    bool apRounds = false;
    bool highVelocity = false;
    bool combatStims = false;
    bool trailSmall = false;
    bool trailThick = false;
};

struct Explosion {
    Vec2 pos;
    std::string type;
    int layer = 0;
};

struct Emote {
    uint16_t playerId = 0;
    std::string type;
    std::string itemType;
    bool isPing = false;
    Vec2 pos;
    bool hasPos = false;
};

struct Airstrike {
    Vec2 pos;
    float duration = 0.0f;
    float rad = 0.0f;
};

struct PlaneData {
    Vec2 planeDir;
    Vec2 pos;
    bool actionComplete = false;
    int action = 0;
    uint8_t id = 0;
};

struct MapIndicator {
    int id = 0;
    bool dead = false;
    bool equipped = false;
    std::string type;
    Vec2 pos;
};

struct PlayerInfo {
    uint16_t playerId = 0;
    uint8_t teamId = 0;
    uint8_t groupId = 0;
    std::string name;
    std::string heal;
    std::string boost;
};

struct GasData {
    uint8_t mode = 0;
    float duration = 0.0f;
    Vec2 posOld;
    Vec2 posNew;
    float radOld = 0.0f;
    float radNew = 0.0f;
};

struct PlayerStatus {
    bool hasData = false;
    Vec2 pos;
    bool visible = false;
    bool dead = false;
    bool downed = false;
    std::string role;
};

struct GroupStatus {
    float health = 0.0f;
    bool disconnected = false;
};

struct WeaponData {
    std::string type;
    uint8_t ammo = 0;
};

struct ActivePlayerData {
    bool healthDirty = false;
    float health = 0.0f;
    bool boostDirty = false;
    float boost = 0.0f;
    bool zoomDirty = false;
    uint8_t zoom = 0;
    bool actionDirty = false;
    float actionTime = 0.0f;
    float actionDuration = 0.0f;
    uint16_t actionTargetId = 0;
    bool inventoryDirty = false;
    std::string scope;
    // inventory counts, indexed by defs::kBagSizeKeys order
    std::vector<uint16_t> inventory;
    bool weapsDirty = false;
    int curWeapIdx = 0;
    std::vector<WeaponData> weapons;
    bool spectatorCountDirty = false;
    uint8_t spectatorCount = 0;
};

struct FullObjectData {
    ObjectData data;
};

struct PartObjectData {
    ObjectData data;
};

inline void serializeActivePlayer(NetBitStream& s, const ActivePlayerData& data) {
    s.writeBoolean(data.healthDirty);
    if (data.healthDirty) {
        s.writeFloat(data.health, 0, 100, 8);
    }
    s.writeBoolean(data.boostDirty);
    if (data.boostDirty) {
        s.writeFloat(data.boost, 0, 100, 8);
    }
    s.writeBoolean(data.zoomDirty);
    if (data.zoomDirty) {
        s.writeUint8(data.zoom);
    }
    s.writeBoolean(data.actionDirty);
    if (data.actionDirty) {
        s.writeFloat(data.actionTime, 0, Constants::ActionMaxDuration, 8);
        s.writeFloat(data.actionDuration, 0, Constants::ActionMaxDuration, 8);
        s.writeUint16(data.actionTargetId);
    }
    s.writeBoolean(data.inventoryDirty);
    if (data.inventoryDirty) {
        s.writeGameType(data.scope);
        for (size_t i = 0; i < defs::kBagSizeKeys.size(); i++) {
            const uint16_t count = i < data.inventory.size() ? data.inventory[i] : 0;
            const bool hasItem = count > 0;
            s.writeBoolean(hasItem);
            if (hasItem) {
                s.writeBits(count, 9);
            }
        }
    }
    s.writeBoolean(data.weapsDirty);
    if (data.weapsDirty) {
        s.writeBits(static_cast<uint32_t>(data.curWeapIdx), 2);
        for (int i = 0; i < WeaponSlot_Count; i++) {
            s.writeGameType(data.weapons[i].type);
            s.writeUint8(data.weapons[i].ammo);
        }
    }
    s.writeBoolean(data.spectatorCountDirty);
    if (data.spectatorCountDirty) {
        s.writeUint8(data.spectatorCount);
    }
    s.writeAlignToNextByte();
}

inline void deserializeActivePlayer(NetBitStream& s, ActivePlayerData& data) {
    data.healthDirty = s.readBoolean();
    if (data.healthDirty) {
        data.health = s.readFloat(0, 100, 8);
    }
    data.boostDirty = s.readBoolean();
    if (data.boostDirty) {
        data.boost = s.readFloat(0, 100, 8);
    }
    data.zoomDirty = s.readBoolean();
    if (data.zoomDirty) {
        data.zoom = s.readUint8();
    }
    data.actionDirty = s.readBoolean();
    if (data.actionDirty) {
        data.actionTime = s.readFloat(0, Constants::ActionMaxDuration, 8);
        data.actionDuration = s.readFloat(0, Constants::ActionMaxDuration, 8);
        data.actionTargetId = s.readUint16();
    }
    data.inventoryDirty = s.readBoolean();
    if (data.inventoryDirty) {
        data.scope = s.readGameType();
        data.inventory.assign(defs::kBagSizeKeys.size(), 0);
        for (size_t i = 0; i < defs::kBagSizeKeys.size(); i++) {
            uint16_t count = 0;
            if (s.readBoolean()) {
                count = static_cast<uint16_t>(s.readBits(9));
            }
            data.inventory[i] = count;
        }
    }
    data.weapsDirty = s.readBoolean();
    if (data.weapsDirty) {
        data.curWeapIdx = static_cast<int>(s.readBits(2));
        data.weapons.resize(WeaponSlot_Count);
        for (int i = 0; i < WeaponSlot_Count; i++) {
            data.weapons[i].type = s.readGameType();
            data.weapons[i].ammo = s.readUint8();
        }
    }
    data.spectatorCountDirty = s.readBoolean();
    if (data.spectatorCountDirty) {
        data.spectatorCount = s.readUint8();
    }
    s.readAlignToNextByte();
}

struct UpdateMsg : Msg {
    std::vector<uint16_t> delObjIds;
    std::vector<FullObjectData> fullObjects;
    std::vector<PartObjectData> partObjects;

    uint16_t activePlayerId = 0;
    bool activePlayerIdDirty = false;
    ActivePlayerData activePlayerData;

    GasData gasData;
    bool gasDirty = false;
    float gasT = 0.0f;
    bool gasTDirty = false;

    std::vector<PlayerInfo> playerInfos;
    std::vector<uint16_t> deletedPlayerIds;

    std::vector<PlayerStatus> playerStatus;
    bool playerStatusDirty = false;

    std::vector<GroupStatus> groupStatus;
    bool groupStatusDirty = false;

    std::vector<Bullet> bullets;
    std::vector<Explosion> explosions;
    std::vector<Emote> emotes;
    std::vector<PlaneData> planes;
    std::vector<Airstrike> airstrikeZones;
    std::vector<MapIndicator> mapIndicators;

    uint16_t killLeaderId = 0;
    uint8_t killLeaderKills = 0;
    bool killLeaderDirty = false;
    uint8_t ack = 0;

    // --- server-side fields not carried by UpdateMsg -----------------------
    // The mobile port applies the same map updates the web client does, but we
    // only track the one-time ids here (used to skip destroy/drop FX on join).
    float updateInterval = 0.0f; // seconds since the previous UpdateMsg

    void serialize(NetBitStream& s) override {
        uint32_t flags = 0;
        const size_t flagsIdx = s.byteIndex();
        s.writeUint16(0);

        if (!delObjIds.empty()) {
            s.writeBits(static_cast<uint32_t>(delObjIds.size()), 16);
            for (uint16_t id : delObjIds) {
                s.writeUint16(id);
            }
            flags |= UpdateExtFlags::DeletedObjects;
        }

if (!fullObjects.empty()) {
            s.writeBits(static_cast<uint32_t>(fullObjects.size()), 16);
            for (const auto& obj : fullObjects) {
                // mirror the server stream layout: [type, id, part, align] + [full, align]
                s.writeUint8(obj.data.__type);
                s.writeUint16(obj.data.__id);
                serializeFns::serializePart(static_cast<ObjectType>(obj.data.__type), s, obj.data);
                s.writeAlignToNextByte();
                serializeFns::serializeFull(static_cast<ObjectType>(obj.data.__type), s, obj.data);
                s.writeAlignToNextByte();
            }
            flags |= UpdateExtFlags::FullObjects;
        }

        s.writeBits(static_cast<uint32_t>(partObjects.size()), 16);
        for (const auto& obj : partObjects) {
            // mirror the server partialStream layout: [type, id, part, align]
            s.writeUint8(obj.data.__type);
            s.writeUint16(obj.data.__id);
            serializeFns::serializePart(static_cast<ObjectType>(obj.data.__type), s, obj.data);
            s.writeAlignToNextByte();
        }

        if (activePlayerIdDirty) {
            s.writeUint16(activePlayerId);
            flags |= UpdateExtFlags::ActivePlayerId;
        }

        serializeActivePlayer(s, activePlayerData);

        if (gasDirty) {
            s.writeUint8(gasData.mode);
            s.writeFloat32(gasData.duration);
            s.writeMapPos(gasData.posOld);
            s.writeMapPos(gasData.posNew);
            s.writeFloat(gasData.radOld, 0, 2048, 16);
            s.writeFloat(gasData.radNew, 0, 2048, 16);
            flags |= UpdateExtFlags::Gas;
        }

        if (gasTDirty) {
            s.writeFloat(gasT, 0, 1, 16);
            flags |= UpdateExtFlags::GasCircle;
        }

        if (!playerInfos.empty()) {
            s.writeBits(static_cast<uint32_t>(playerInfos.size()), 8);
            for (const auto& info : playerInfos) {
                s.writeUint16(info.playerId);
                s.writeUint8(info.teamId);
                s.writeUint8(info.groupId);
                s.writeString(info.name);
                s.writeGameType(info.heal);
                s.writeGameType(info.boost);
                s.writeAlignToNextByte();
            }
            flags |= UpdateExtFlags::PlayerInfos;
        }

        if (!deletedPlayerIds.empty()) {
            s.writeBits(static_cast<uint32_t>(deletedPlayerIds.size()), 8);
            for (uint16_t id : deletedPlayerIds) {
                s.writeUint16(id);
            }
            flags |= UpdateExtFlags::DeletePlayerIds;
        }

        if (playerStatusDirty) {
            serializePlayerStatus(s);
            flags |= UpdateExtFlags::PlayerStatus;
        }

        if (groupStatusDirty) {
            s.writeBits(static_cast<uint32_t>(groupStatus.size()), 8);
            for (const auto& status : groupStatus) {
                s.writeFloat(status.health, 0, 100, 7);
                s.writeBoolean(status.disconnected);
            }
            flags |= UpdateExtFlags::GroupStatus;
        }

        if (!bullets.empty()) {
            s.writeBits(static_cast<uint32_t>(bullets.size()), 8);
            for (const auto& bullet : bullets) {
                serializeBullet(s, bullet);
            }
            s.writeAlignToNextByte();
            flags |= UpdateExtFlags::Bullets;
        }

        if (!explosions.empty()) {
            s.writeBits(static_cast<uint32_t>(explosions.size()), 8);
            for (const auto& explosion : explosions) {
                s.writeMapPos(explosion.pos);
                s.writeGameType(explosion.type);
                s.writeBits(static_cast<uint32_t>(explosion.layer), 2);
                s.writeAlignToNextByte();
            }
            flags |= UpdateExtFlags::Explosions;
        }

        if (!emotes.empty()) {
            s.writeBits(static_cast<uint32_t>(emotes.size()), 8);
            for (const auto& emote : emotes) {
                s.writeUint16(emote.playerId);
                s.writeGameType(emote.type);
                s.writeGameType(emote.itemType);
                s.writeBoolean(emote.isPing);
                if (emote.isPing && emote.hasPos) {
                    s.writeMapPos(emote.pos);
                }
                s.writeAlignToNextByte();
            }
            flags |= UpdateExtFlags::Emotes;
        }

        if (!planes.empty()) {
            s.writeBits(static_cast<uint32_t>(planes.size()), 8);
            for (const auto& plane : planes) {
                s.writeUint8(plane.id);
                s.writeVec(plane.pos, -256, -256, Constants::MaxPosition + 256, Constants::MaxPosition + 256, 10);
                s.writeUnitVec(plane.planeDir, 8);
                s.writeBoolean(plane.actionComplete);
                s.writeBits(static_cast<uint32_t>(plane.action), 3);
            }
            flags |= UpdateExtFlags::Planes;
        }

        if (!airstrikeZones.empty()) {
            s.writeBits(static_cast<uint32_t>(airstrikeZones.size()), 8);
            for (const auto& zone : airstrikeZones) {
                s.writeMapPos(zone.pos, 12);
                s.writeFloat(zone.rad, 0, Constants::AirstrikeZoneMaxRad, 8);
                s.writeFloat(zone.duration, 0, Constants::AirstrikeZoneMaxDuration, 8);
            }
            s.writeAlignToNextByte();
            flags |= UpdateExtFlags::AirstrikeZones;
        }

        if (!mapIndicators.empty()) {
            s.writeBits(static_cast<uint32_t>(mapIndicators.size()), BitSizes::MapIndicators);
            for (const auto& indicator : mapIndicators) {
                s.writeBits(static_cast<uint32_t>(indicator.id), BitSizes::MapIndicators);
                s.writeBoolean(indicator.dead);
                s.writeBoolean(indicator.equipped);
                s.writeGameType(indicator.type);
                s.writeMapPos(indicator.pos);
            }
            s.writeAlignToNextByte();
            flags |= UpdateExtFlags::MapIndicators;
        }

        if (killLeaderDirty) {
            s.writeUint16(killLeaderId);
            s.writeUint8(killLeaderKills);
            flags |= UpdateExtFlags::KillLeader;
        }

        s.writeUint8(ack);
        const size_t idx = s.byteIndex();
        s.setByteIndex(flagsIdx);
        s.writeUint16(static_cast<uint16_t>(flags));
        s.setByteIndex(idx);
    }

void deserialize(NetBitStream& s) override {
        const uint32_t flags = s.readUint16();

        if ((flags & UpdateExtFlags::DeletedObjects) != 0) {
            const uint32_t n = s.readBits(16);
            delObjIds.clear();
            delObjIds.reserve(n);
            for (uint32_t i = 0; i < n; i++) {
                delObjIds.push_back(s.readUint16());
            }
        }

        if ((flags & UpdateExtFlags::FullObjects) != 0) {
            const uint32_t n = s.readBits(16);
            fullObjects.clear();
            fullObjects.resize(n);
            for (uint32_t i = 0; i < n; i++) {
                ObjectData& d = fullObjects[i].data;
                d.__type = s.readUint8();
                d.__id = s.readUint16();
serializeFns::deserializePart(static_cast<ObjectType>(d.__type), s, d);
                s.readAlignToNextByte();
                serializeFns::deserializeFull(static_cast<ObjectType>(d.__type), s, d);
                s.readAlignToNextByte();
            }
        }

        {
            const uint32_t n = s.readBits(16);
            partObjects.clear();
            partObjects.resize(n);
            for (uint32_t i = 0; i < n; i++) {
                ObjectData& d = partObjects[i].data;
                d.__type = s.readUint8();
                d.__id = s.readUint16();
                serializeFns::deserializePart(static_cast<ObjectType>(d.__type), s, d);
                s.readAlignToNextByte();
            }
        }

        if ((flags & UpdateExtFlags::ActivePlayerId) != 0) {
            activePlayerId = s.readUint16();
            activePlayerIdDirty = true;
        }
deserializeActivePlayer(s, activePlayerData);
if ((flags & UpdateExtFlags::Gas) != 0) {
            gasData.mode = s.readUint8();
            gasData.duration = s.readFloat32();
            gasData.posOld = s.readMapPos();
            gasData.posNew = s.readMapPos();
            gasData.radOld = s.readFloat(0, 2048, 16);
            gasData.radNew = s.readFloat(0, 2048, 16);
            gasDirty = true;
        }
if ((flags & UpdateExtFlags::GasCircle) != 0) {
            gasT = s.readFloat(0, 1, 16);
            gasTDirty = true;
        }

if ((flags & UpdateExtFlags::PlayerInfos) != 0) {
            const uint32_t n = s.readBits(8);
            playerInfos.clear();
            playerInfos.resize(n);
            for (uint32_t i = 0; i < n; i++) {
                playerInfos[i].playerId = s.readUint16();
                playerInfos[i].teamId = s.readUint8();
                playerInfos[i].groupId = s.readUint8();
                playerInfos[i].name = s.readString();
                playerInfos[i].heal = s.readGameType();
                playerInfos[i].boost = s.readGameType();
                s.readAlignToNextByte();
            }

        }

        if ((flags & UpdateExtFlags::DeletePlayerIds) != 0) {
            const uint32_t n = s.readBits(8);
            deletedPlayerIds.clear();
            deletedPlayerIds.reserve(n);
            for (uint32_t i = 0; i < n; i++) {
                deletedPlayerIds.push_back(s.readUint16());
            }
        }

        if ((flags & UpdateExtFlags::PlayerStatus) != 0) {
            const uint32_t n = s.readBits(8);
            playerStatus.clear();
            playerStatus.resize(n);
            for (uint32_t i = 0; i < n; i++) {
                PlayerStatus& p = playerStatus[i];
                p.hasData = s.readBoolean();
                if (p.hasData) {
                    p.pos = s.readMapPos(11);
                    p.visible = s.readBoolean();
                    p.dead = s.readBoolean();
                    p.downed = s.readBoolean();
                    p.role = s.readBoolean() ? s.readGameType() : "";
                }
            }
            s.readAlignToNextByte();
            playerStatusDirty = true;
        }

        if ((flags & UpdateExtFlags::GroupStatus) != 0) {
            const uint32_t n = s.readBits(8);
            groupStatus.clear();
            groupStatus.resize(n);
            for (uint32_t i = 0; i < n; i++) {
                groupStatus[i].health = s.readFloat(0, 100, 7);
                groupStatus[i].disconnected = s.readBoolean();
            }
            groupStatusDirty = true;
        }

if ((flags & UpdateExtFlags::Bullets) != 0) {
            const uint32_t n = s.readBits(8);
            bullets.clear();
            bullets.resize(n);
            for (uint32_t i = 0; i < n; i++) {
                deserializeBullet(s, bullets[i]);
            }
            s.readAlignToNextByte();

        }

        if ((flags & UpdateExtFlags::Explosions) != 0) {
            const uint32_t n = s.readBits(8);
            explosions.clear();
            explosions.resize(n);
            for (uint32_t i = 0; i < n; i++) {
                explosions[i].pos = s.readMapPos();
                explosions[i].type = s.readGameType();
                explosions[i].layer = static_cast<int>(s.readBits(2));
                s.readAlignToNextByte();
            }
        }

        if ((flags & UpdateExtFlags::Emotes) != 0) {
            const uint32_t n = s.readBits(8);
            emotes.clear();
            emotes.resize(n);
            for (uint32_t i = 0; i < n; i++) {
                Emote& e = emotes[i];
                e.playerId = s.readUint16();
                e.type = s.readGameType();
                e.itemType = s.readGameType();
                e.isPing = s.readBoolean();
                if (e.isPing) {
                    e.pos = s.readMapPos();
                    e.hasPos = true;
                }
                s.readAlignToNextByte();
            }
        }

        if ((flags & UpdateExtFlags::Planes) != 0) {
            const uint32_t n = s.readBits(8);
            planes.clear();
            planes.resize(n);
            for (uint32_t i = 0; i < n; i++) {
                PlaneData& p = planes[i];
                p.id = s.readUint8();
                p.pos = s.readVec(-256, -256, Constants::MaxPosition + 256, Constants::MaxPosition + 256, 10);
                p.planeDir = s.readUnitVec(8);
                p.actionComplete = s.readBoolean();
                p.action = static_cast<int>(s.readBits(3));
            }
        }

        if ((flags & UpdateExtFlags::AirstrikeZones) != 0) {
            const uint32_t n = s.readBits(8);
            airstrikeZones.clear();
            airstrikeZones.resize(n);
            for (uint32_t i = 0; i < n; i++) {
                airstrikeZones[i].pos = s.readMapPos(12);
                airstrikeZones[i].rad = s.readFloat(0, Constants::AirstrikeZoneMaxRad, 8);
                airstrikeZones[i].duration = s.readFloat(0, Constants::AirstrikeZoneMaxDuration, 8);
            }
            s.readAlignToNextByte();
        }

        if ((flags & UpdateExtFlags::MapIndicators) != 0) {
            const uint32_t n = s.readBits(BitSizes::MapIndicators);
            mapIndicators.clear();
            mapIndicators.resize(n);
            for (uint32_t i = 0; i < n; i++) {
                mapIndicators[i].id = static_cast<int>(s.readBits(BitSizes::MapIndicators));
                mapIndicators[i].dead = s.readBoolean();
                mapIndicators[i].equipped = s.readBoolean();
                mapIndicators[i].type = s.readGameType();
                mapIndicators[i].pos = s.readMapPos();
            }
            s.readAlignToNextByte();
        }

        if ((flags & UpdateExtFlags::KillLeader) != 0) {
            killLeaderId = s.readUint16();
            killLeaderKills = s.readUint8();
            killLeaderDirty = true;
        }
        ack = s.readUint8();
    }

private:
    static void serializeBullet(NetBitStream& s, const Bullet& bullet) {
        s.writeUint16(bullet.playerId);
        s.writeMapPos(bullet.startPos);
        s.writeUnitVec(bullet.dir, 8);
        s.writeGameType(bullet.bulletType);
        s.writeBits(static_cast<uint32_t>(bullet.layer), 2);
        s.writeFloat(bullet.varianceT, 0, 1, 4);
        s.writeBits(static_cast<uint32_t>(bullet.distAdjIdx), 4);
        s.writeBoolean(bullet.clipDistance);
        if (bullet.clipDistance) {
            s.writeFloat(bullet.distance, 0, Constants::MaxPosition, 16);
        }
        s.writeBoolean(bullet.shotFx);
        if (bullet.shotFx) {
            s.writeGameType(bullet.shotSourceType);
            s.writeBoolean(bullet.shotOffhand);
            s.writeBoolean(bullet.lastShot);
        }
        s.writeBoolean(bullet.reflectCount > 0);
        if (bullet.reflectCount > 0) {
            s.writeBits(static_cast<uint32_t>(bullet.reflectCount), 2);
            s.writeUint16(bullet.reflectObjId);
        }
        s.writeBoolean(bullet.hasModifier);
        if (bullet.hasModifier) {
            s.writeFloat(bullet.speedMult, 0.5f, 2, 8);
            s.writeFloat(bullet.distanceMult, 0.5f, 2, 8);
        }
        s.writeBoolean(bullet.hasSpecialFx);
        if (bullet.hasSpecialFx) {
            s.writeBoolean(bullet.shotAlt);
            s.writeBoolean(bullet.splinter);
            s.writeBoolean(bullet.trailSaturated);
            s.writeBoolean(bullet.apRounds);
            s.writeBoolean(bullet.highVelocity);
            s.writeBoolean(bullet.combatStims);
            s.writeBoolean(bullet.trailSmall);
            s.writeBoolean(bullet.trailThick);
        }
    }

    static void deserializeBullet(NetBitStream& s, Bullet& bullet) {
        bullet.playerId = s.readUint16();
        bullet.startPos = s.readMapPos();
        bullet.pos = bullet.startPos;
        bullet.dir = s.readUnitVec(8);
        bullet.bulletType = s.readGameType();
        bullet.layer = static_cast<int>(s.readBits(2));
        bullet.varianceT = s.readFloat(0, 1, 4);
        bullet.distAdjIdx = static_cast<int>(s.readBits(4));
        bullet.clipDistance = s.readBoolean();
        if (bullet.clipDistance) {
            bullet.distance = s.readFloat(0, Constants::MaxPosition, 16);
        }
        bullet.shotFx = s.readBoolean();
        if (bullet.shotFx) {
            bullet.shotSourceType = s.readGameType();
            bullet.shotOffhand = s.readBoolean();
            bullet.lastShot = s.readBoolean();
        }
        bullet.reflectCount = 0;
        bullet.reflectObjId = 0;
        if (s.readBoolean()) {
            bullet.reflectCount = static_cast<int>(s.readBits(2));
            bullet.reflectObjId = s.readUint16();
        }
        bullet.speedMult = 1.0f;
        bullet.distanceMult = 1.0f;
        bullet.hasModifier = s.readBoolean();
        if (bullet.hasModifier) {
            bullet.speedMult = s.readFloat(0.5f, 2, 8);
            bullet.distanceMult = s.readFloat(0.5f, 2, 8);
        }
        bullet.hasSpecialFx = s.readBoolean();
        if (bullet.hasSpecialFx) {
            bullet.shotAlt = s.readBoolean();
            bullet.splinter = s.readBoolean();
            bullet.trailSaturated = s.readBoolean();
            bullet.apRounds = s.readBoolean();
            bullet.highVelocity = s.readBoolean();
            bullet.combatStims = s.readBoolean();
            bullet.trailSmall = s.readBoolean();
            bullet.trailThick = s.readBoolean();
        }
    }

    void serializePlayerStatus(NetBitStream& s) {
        s.writeBits(static_cast<uint32_t>(playerStatus.size()), 8);
        for (const auto& info : playerStatus) {
            s.writeBoolean(info.hasData);
            if (info.hasData) {
                s.writeMapPos(info.pos, 11);
                s.writeBoolean(info.visible);
                s.writeBoolean(info.dead);
                s.writeBoolean(info.downed);
                const bool hasRole = !info.role.empty();
                s.writeBoolean(hasRole);
                if (hasRole) {
                    s.writeGameType(info.role);
                }
            }
        }
        s.writeAlignToNextByte();
    }
};

} // namespace surv


