#pragma once
// Port of shared/net/objectSerializeFns.ts
#include "../core/GameConfig.h"
#include "../core/Vec2.h"
#include "Net.h"
#include <cstdint>
#include <string>
#include <vector>

namespace surv {

enum ObjectType : uint8_t {
    ObjectType_Invalid = 0,
    ObjectType_Player = 1,
    ObjectType_Obstacle = 2,
    ObjectType_Loot = 3,
    ObjectType_LootSpawner = 4, // NOTE: unused
    ObjectType_DeadBody = 5,
    ObjectType_Building = 6,
    ObjectType_Structure = 7,
    ObjectType_Decal = 8,
    ObjectType_Projectile = 9,
    ObjectType_Smoke = 10,
    ObjectType_Airdrop = 11,
};

// A single pooled object struct covering the union of all object types
// (mirrors ObjectsFullData & ObjectsPartialData).
struct ObjectData {
    uint16_t __id = 0;
    uint8_t __type = ObjectType_Invalid;

    // shared
    Vec2 pos;
    Vec2 dir;
    int ori = 0;
    float scale = 1.0f;
    int layer = 0;
    std::string type;

    // player
    std::string outfit;
    std::string backpack;
    std::string helmet;
    std::string chest;
    std::string activeWeapon;
    bool dead = false;
    bool downed = false;
    int animType = 0;
    int animSeq = 0;
    int actionType = 0;
    int actionSeq = 0;
    bool wearingPan = false;
    bool healEffect = false;
    bool lastStandEffect = false;
    bool frozen = false;
    int frozenOri = 0;
    std::string frozenType;
    int hasteType = HasteType_None;
    int hasteSeq = -1;
    std::string actionItem;
    std::string role;
    struct Perk {
        std::string type;
        bool droppable = false;
    };
    std::vector<Perk> perks;

    // obstacle
    float healthT = 0.0f;
    bool isDoor = false;
    bool doorOpen = false;
    bool doorCanUse = false;
    bool doorLocked = false;
    int doorSeq = 0;
    bool isButton = false;
    bool buttonOnOff = false;
    bool buttonCanUse = false;
    int buttonSeq = 0;
    bool isPuzzlePiece = false;
    uint16_t parentBuildingId = 0;
    bool isSkin = false;
    uint16_t skinPlayerId = 0;

    // building
    bool ceilingDead = false;
    bool occupied = false;
    bool ceilingDamaged = false;
    bool hasPuzzle = false;
    bool puzzleSolved = false;
    int puzzleErrSeq = 0;

    // structure
    bool interiorSoundEnabled = false;
    bool interiorSoundAlt = false;
    uint16_t layerObjIds[2] = {0, 0};

    // loot
    bool isOld = false;
    bool isPreloadedGun = false;
    uint8_t count = 0;
    bool hasOwner = false;
    uint16_t ownerId = 0;

    // deadbody
    uint16_t playerId = 0;

    // projectile
    float posZ = 0.0f;

    // smoke
    float rad = 0.0f;
    int interior = 0;

    // airdrop
    float fallT = 0.0f;
    bool landed = false;
};

namespace serializeFns {

inline void deserializePart(ObjectType ot, NetBitStream& s, ObjectData& d) {
    switch (ot) {
        case ObjectType_Player:
            d.pos = s.readMapPos();
            d.dir = s.readUnitVec(8);
            break;
        case ObjectType_Obstacle:
            d.pos = s.readMapPos();
            d.ori = static_cast<int>(s.readBits(2));
            d.scale = s.readFloat(Constants::MapObjectMinScale, Constants::MapObjectMaxScale, 8);
            break;
        case ObjectType_Loot:
            d.pos = s.readMapPos();
            break;
        case ObjectType_Building:
            d.ceilingDead = s.readBoolean();
            d.occupied = s.readBoolean();
            d.ceilingDamaged = s.readBoolean();
            d.hasPuzzle = s.readBoolean();
            if (d.hasPuzzle) {
                d.puzzleSolved = s.readBoolean();
                d.puzzleErrSeq = static_cast<int>(s.readBits(7));
            }
            break;
        case ObjectType_LootSpawner:
            d.pos = s.readMapPos();
            d.type = s.readMapType();
            d.layer = static_cast<int>(s.readBits(2));
            break;
        case ObjectType_DeadBody:
            d.pos = s.readMapPos();
            break;
        case ObjectType_Projectile:
            d.pos = s.readMapPos();
            d.posZ = s.readFloat(0.0f, defs::kProjectileMaxHeight, 10);
            d.dir = s.readUnitVec(7);
            break;
        case ObjectType_Smoke:
            d.pos = s.readMapPos();
            d.rad = s.readFloat(0.0f, Constants::SmokeMaxRad, 8);
            break;
        case ObjectType_Airdrop:
            d.fallT = s.readFloat(0.0f, 1.0f, 7);
            d.landed = s.readBoolean();
            break;
        case ObjectType_Structure:
        case ObjectType_Decal:
        case ObjectType_Invalid:
        default:
            break;
    }
}

inline void deserializeFull(ObjectType ot, NetBitStream& s, ObjectData& d) {
    switch (ot) {
        case ObjectType_Player:
            d.outfit = s.readGameType();
            d.backpack = s.readGameType();
            d.helmet = s.readGameType();
            d.chest = s.readGameType();
            d.activeWeapon = s.readGameType();
            d.layer = static_cast<int>(s.readBits(2));
            d.dead = s.readBoolean();
            d.downed = s.readBoolean();
            d.animType = static_cast<int>(s.readBits(BitSizes::Anim));
            d.animSeq = static_cast<int>(s.readBits(3));
            d.actionType = static_cast<int>(s.readBits(BitSizes::Action));
            d.actionSeq = static_cast<int>(s.readBits(3));
            d.wearingPan = s.readBoolean();
            d.healEffect = s.readBoolean();
            d.lastStandEffect = s.readBoolean();
            d.frozen = s.readBoolean();
            d.frozenOri = 0;
            d.frozenType.clear();
            if (d.frozen) {
                d.frozenOri = static_cast<int>(s.readBits(2));
                d.frozenType = s.readGameType();
            }
            d.hasteType = HasteType_None;
            d.hasteSeq = -1;
            if (s.readBoolean()) {
                d.hasteType = static_cast<int>(s.readBits(BitSizes::Haste));
                d.hasteSeq = static_cast<int>(s.readBits(3));
            }
            d.actionItem = s.readBoolean() ? s.readGameType() : "";
            d.scale = s.readBoolean()
                ? s.readFloat(Constants::PlayerMinScale, Constants::PlayerMaxScale, 8)
                : 1.0f;
            d.role = s.readBoolean() ? s.readGameType() : "";
            d.perks.clear();
            if (s.readBoolean()) {
                const uint32_t n = s.readBits(BitSizes::Perks);
                d.perks.reserve(n);
                for (uint32_t i = 0; i < n; i++) {
                    ObjectData::Perk p;
                    p.type = s.readGameType();
                    p.droppable = s.readBoolean();
                    d.perks.push_back(std::move(p));
                }
            }
            break;
        case ObjectType_Obstacle:
            d.healthT = s.readFloat(0.0f, 1.0f, 8);
            d.type = s.readMapType();
            d.layer = static_cast<int>(s.readBits(2));
            d.dead = s.readBoolean();
            d.isDoor = s.readBoolean();
            if (d.isDoor) {
                d.doorOpen = s.readBoolean();
                d.doorCanUse = s.readBoolean();
                d.doorLocked = s.readBoolean();
                d.doorSeq = static_cast<int>(s.readBits(5));
            }
            d.isButton = s.readBoolean();
            if (d.isButton) {
                d.buttonOnOff = s.readBoolean();
                d.buttonCanUse = s.readBoolean();
                d.buttonSeq = static_cast<int>(s.readBits(6));
            }
            d.isPuzzlePiece = s.readBoolean();
            if (d.isPuzzlePiece) {
                d.parentBuildingId = s.readUint16();
            }
            d.isSkin = s.readBoolean();
            if (d.isSkin) {
                d.skinPlayerId = s.readUint16();
            }
            break;
        case ObjectType_Building:
            d.pos = s.readMapPos();
            d.type = s.readMapType();
            d.ori = static_cast<int>(s.readBits(2));
            d.layer = static_cast<int>(s.readBits(2));
            break;
        case ObjectType_Structure:
            d.pos = s.readMapPos();
            d.type = s.readMapType();
            d.ori = static_cast<int>(s.readBits(2));
            d.interiorSoundEnabled = s.readBoolean();
            d.interiorSoundAlt = s.readBoolean();
            for (int r = 0; r < defs::kStructureLayerCount; r++) {
                d.layerObjIds[r] = s.readUint16();
            }
            break;
        case ObjectType_Loot:
            d.type = s.readGameType();
            d.count = s.readUint8();
            d.layer = static_cast<int>(s.readBits(2));
            d.isOld = s.readBoolean();
            d.isPreloadedGun = s.readBoolean();
            d.hasOwner = s.readBoolean();
            if (d.hasOwner) {
                d.ownerId = s.readUint16();
            }
            break;
        case ObjectType_DeadBody:
            d.layer = s.readUint8();
            d.playerId = s.readUint16();
            break;
        case ObjectType_Decal:
            d.pos = s.readMapPos();
            d.scale = s.readFloat(Constants::MapObjectMinScale, Constants::MapObjectMaxScale, 8);
            d.type = s.readMapType();
            d.ori = static_cast<int>(s.readBits(2));
            d.layer = static_cast<int>(s.readBits(2));
            d.count = s.readUint8(); // goreKills
            break;
        case ObjectType_Projectile:
            d.type = s.readGameType();
            d.layer = static_cast<int>(s.readBits(2));
            break;
        case ObjectType_Smoke:
            d.layer = static_cast<int>(s.readBits(2));
            d.interior = static_cast<int>(s.readBits(6));
            break;
        case ObjectType_Airdrop:
            d.pos = s.readMapPos();
            break;
        case ObjectType_LootSpawner:
        case ObjectType_Invalid:
        default:
            break;
    }
}

inline void serializePart(ObjectType ot, NetBitStream& s, const ObjectData& d) {
    switch (ot) {
        case ObjectType_Player:
            s.writeMapPos(d.pos);
            s.writeUnitVec(d.dir, 8);
            break;
        case ObjectType_Obstacle:
            s.writeMapPos(d.pos);
            s.writeBits(static_cast<uint32_t>(d.ori), 2);
            s.writeFloat(d.scale, Constants::MapObjectMinScale, Constants::MapObjectMaxScale, 8);
            break;
        case ObjectType_Loot:
            s.writeMapPos(d.pos);
            break;
        case ObjectType_Building:
            s.writeBoolean(d.ceilingDead);
            s.writeBoolean(d.occupied);
            s.writeBoolean(d.ceilingDamaged);
            s.writeBoolean(d.hasPuzzle);
            if (d.hasPuzzle) {
                s.writeBoolean(d.puzzleSolved);
                s.writeBits(static_cast<uint32_t>(d.puzzleErrSeq), 7);
            }
            break;
        case ObjectType_LootSpawner:
            s.writeMapPos(d.pos);
            s.writeMapType(d.type);
            s.writeBits(static_cast<uint32_t>(d.layer), 2);
            break;
        case ObjectType_DeadBody:
            s.writeMapPos(d.pos);
            break;
        case ObjectType_Projectile:
            s.writeMapPos(d.pos);
            s.writeFloat(d.posZ, 0.0f, defs::kProjectileMaxHeight, 10);
            s.writeUnitVec(d.dir, 7);
            break;
        case ObjectType_Smoke:
            s.writeMapPos(d.pos);
            s.writeFloat(d.rad, 0.0f, Constants::SmokeMaxRad, 8);
            break;
        case ObjectType_Airdrop:
            s.writeFloat(d.fallT, 0.0f, 1.0f, 7);
            s.writeBoolean(d.landed);
            break;
        case ObjectType_Structure:
        case ObjectType_Decal:
        case ObjectType_Invalid:
        default:
            break;
    }
}

inline void serializeFull(ObjectType ot, NetBitStream& s, const ObjectData& d) {
    switch (ot) {
        case ObjectType_Player: {
            s.writeGameType(d.outfit);
            s.writeGameType(d.backpack);
            s.writeGameType(d.helmet);
            s.writeGameType(d.chest);
            s.writeGameType(d.activeWeapon);
            s.writeBits(static_cast<uint32_t>(d.layer), 2);
            s.writeBoolean(d.dead);
            s.writeBoolean(d.downed);
            s.writeBits(static_cast<uint32_t>(d.animType), BitSizes::Anim);
            s.writeBits(static_cast<uint32_t>(d.animSeq), 3);
            s.writeBits(static_cast<uint32_t>(d.actionType), BitSizes::Action);
            s.writeBits(static_cast<uint32_t>(d.actionSeq), 3);
            s.writeBoolean(d.wearingPan);
            s.writeBoolean(d.healEffect);
            s.writeBoolean(d.lastStandEffect);
            s.writeBoolean(d.frozen);
            if (d.frozen) {
                s.writeBits(static_cast<uint32_t>(d.frozenOri), 2);
                s.writeGameType(d.frozenType);
            }
            s.writeBoolean(d.hasteType != HasteType_None);
            if (d.hasteType != HasteType_None) {
                s.writeBits(static_cast<uint32_t>(d.hasteType), BitSizes::Haste);
                s.writeBits(static_cast<uint32_t>(d.hasteSeq), 3);
            }
            s.writeBoolean(!d.actionItem.empty());
            if (!d.actionItem.empty()) {
                s.writeGameType(d.actionItem);
            }
            const bool hasScale = d.scale != 1.0f;
            s.writeBoolean(hasScale);
            if (hasScale) {
                s.writeFloat(d.scale, Constants::PlayerMinScale, Constants::PlayerMaxScale, 8);
            }
            const bool hasRole = !d.role.empty();
            s.writeBoolean(hasRole);
            if (hasRole) {
                s.writeGameType(d.role);
            }
            const bool hasPerks = !d.perks.empty();
            s.writeBoolean(hasPerks);
            if (hasPerks) {
                s.writeBits(static_cast<uint32_t>(d.perks.size()), BitSizes::Perks);
                for (const auto& perk : d.perks) {
                    s.writeGameType(perk.type);
                    s.writeBoolean(perk.droppable);
                }
            }
            break;
        }
        case ObjectType_Obstacle:
            s.writeFloat(d.healthT, 0.0f, 1.0f, 8);
            s.writeMapType(d.type);
            s.writeBits(static_cast<uint32_t>(d.layer), 2);
            s.writeBoolean(d.dead);
            s.writeBoolean(d.isDoor);
            if (d.isDoor) {
                s.writeBoolean(d.doorOpen);
                s.writeBoolean(d.doorCanUse);
                s.writeBoolean(d.doorLocked);
                s.writeBits(static_cast<uint32_t>(d.doorSeq), 5);
            }
            s.writeBoolean(d.isButton);
            if (d.isButton) {
                s.writeBoolean(d.buttonOnOff);
                s.writeBoolean(d.buttonCanUse);
                s.writeBits(static_cast<uint32_t>(d.buttonSeq), 6);
            }
            s.writeBoolean(d.isPuzzlePiece);
            if (d.isPuzzlePiece) {
                s.writeUint16(d.parentBuildingId);
            }
            s.writeBoolean(d.isSkin);
            if (d.isSkin) {
                s.writeUint16(d.skinPlayerId);
            }
            break;
        case ObjectType_Building:
            s.writeMapPos(d.pos);
            s.writeMapType(d.type);
            s.writeBits(static_cast<uint32_t>(d.ori), 2);
            s.writeBits(static_cast<uint32_t>(d.layer), 2);
            break;
        case ObjectType_Structure:
            s.writeMapPos(d.pos);
            s.writeMapType(d.type);
            s.writeBits(static_cast<uint32_t>(d.ori), 2);
            s.writeBoolean(d.interiorSoundEnabled);
            s.writeBoolean(d.interiorSoundAlt);
            for (int r = 0; r < defs::kStructureLayerCount; r++) {
                s.writeUint16(d.layerObjIds[r]);
            }
            break;
        case ObjectType_Loot:
            s.writeGameType(d.type);
            s.writeUint8(d.count);
            s.writeBits(static_cast<uint32_t>(d.layer), 2);
            s.writeBoolean(d.isOld);
            s.writeBoolean(d.isPreloadedGun);
            s.writeBoolean(d.ownerId != 0);
            if (d.ownerId != 0) {
                s.writeUint16(d.ownerId);
            }
            break;
        case ObjectType_DeadBody:
            s.writeUint8(static_cast<uint8_t>(d.layer));
            s.writeUint16(d.playerId);
            break;
        case ObjectType_Decal:
            s.writeMapPos(d.pos);
            s.writeFloat(d.scale, Constants::MapObjectMinScale, Constants::MapObjectMaxScale, 8);
            s.writeMapType(d.type);
            s.writeBits(static_cast<uint32_t>(d.ori), 2);
            s.writeBits(static_cast<uint32_t>(d.layer), 2);
            s.writeUint8(d.count);
            break;
        case ObjectType_Projectile:
            s.writeGameType(d.type);
            s.writeBits(static_cast<uint32_t>(d.layer), 2);
            break;
        case ObjectType_Smoke:
            s.writeBits(static_cast<uint32_t>(d.layer), 2);
            s.writeBits(static_cast<uint32_t>(d.interior), 6);
            break;
        case ObjectType_Airdrop:
            s.writeMapPos(d.pos);
            break;
        case ObjectType_LootSpawner:
        case ObjectType_Invalid:
        default:
            break;
    }
}

} // namespace serializeFns

} // namespace surv