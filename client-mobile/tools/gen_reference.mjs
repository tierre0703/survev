// Generates reference hex fixtures from the real shared/ TS code so the
// C++ port can be verified bit-for-bit. Output is a C++ test header.
// Usage: node --experimental-transform-types tools/gen_reference.mjs > tests/ReferenceData.h
import { fileURLToPath } from "node:url";
import * as path from "node:path";
import * as fs from "node:fs";

const here = path.dirname(fileURLToPath(import.meta.url));
const sharedDir = path.resolve(here, "../../shared");

function pathToFile(dir, rel) {
    return new URL(`file:///${path.join(dir, rel).replace(/\\/g, "/")}`);
}

const bb = await import(pathToFile(sharedDir, "lib/bitBuffer.ts"));
const net = await import(pathToFile(sharedDir, "net/net.ts"));
const msgs = net;

function hexOf(stream) {
    // stream is a BitStream; read bytes 0..byteIndex
    const bytes = new Uint8Array(stream.buffer, 0, stream.byteIndex);
    return Array.from(bytes, (b) => b.toString(16).padStart(2, "0")).join("");
}

const fixtures = [];

// ---- 1. BitView/BitStream primitives ----
{
    const buf = new ArrayBuffer(64);
    const s = new net.BitStream(buf);
    s.writeBoolean(true);
    s.writeUint8(0xab);
    s.writeInt16(-1234);
    s.writeUint16(0xbeef);
    s.writeInt32(-2000000000);
    s.writeUint32(0xffffffff);
    s.writeFloat(3.14, 0, 10, 8);
    s.writeFloat32(1.5);
    s.writeFloat64(12345.6789);
    s.writeASCIIString("hello");
    s.writeUTF8String("héllo");
    s.writeUint8(0x00);
    fixtures.push(["primitives", hexOf(s)]);
}

// ---- 2. Vec/unit/map pos ----
{
    const buf = new ArrayBuffer(64);
    const s = new net.BitStream(buf);
    s.writeMapPos({ x: 100.5, y: 200.25 });
    s.writeUnitVec({ x: 0.707, y: 0.707 }, 8);
    s.writeVec32({ x: -5.5, y: 6.25 });
    fixtures.push(["vecs", hexOf(s)]);
}

// ---- 3. Strings with fixed length + align ----
{
    const buf = new ArrayBuffer(64);
    const s = new net.BitStream(buf);
    s.writeString("player1", 16);
    s.writeAlignToNextByte();
    s.writeUint8(7);
    fixtures.push(["string-fixed-align", hexOf(s)]);
}

// ---- 4. JoinMsg ----
{
    const buf = new ArrayBuffer(256);
    const s = new net.BitStream(buf);
    const m = new msgs.JoinMsg();
    m.protocol = 1028;
    m.joinToken = "tok_abc123";
    m.name = "TestPlayer";
    m.useTouch = true;
    m.isMobile = true;
    m.loadout = {
        outfit: "outfitBase",
        melee: "fists",
        heal: "bandage",
        boost: "soda",
        emotes: ["emote_happyface", "emote_thumbsup"],
    };
    m.serialize(s);
    fixtures.push(["join", hexOf(s)]);
}

// ---- 5. JoinedMsg ----
{
    const buf = new ArrayBuffer(64);
    const s = new net.BitStream(buf);
    const m = new msgs.JoinedMsg();
    m.teamMode = 2;
    m.playerId = 42;
    m.started = true;
    m.emotes = ["emote_happyface", "emote_sadface"];
    m.serialize(s);
    fixtures.push(["joined", hexOf(s)]);
}

// ---- 6. InputMsg ----
{
    const buf = new ArrayBuffer(64);
    const s = new net.BitStream(buf);
    const m = new msgs.InputMsg();
    m.seq = 7;
    m.moveUp = true;
    m.moveDown = true;
    m.shootHold = true;
    m.portrait = true;
    m.touchMoveActive = true;
    m.touchMoveDir = { x: 0.5, y: 0.5 };
    m.touchMoveLen = 128;
    m.toMouseDir = { x: 1, y: 0 };
    m.toMouseLen = 32;
    m.inputs = [1, 2, 3];
    m.useItem = "bandage";
    m.serialize(s);
    fixtures.push(["input", hexOf(s)]);
}

// ---- 7. EditMsg ----
{
    const buf = new ArrayBuffer(64);
    const s = new net.BitStream(buf);
    const m = new msgs.EditMsg();
    m.zoomEnabled = true;
    m.zoom = 2;
    m.speed = 0.5;
    m.speedEnabled = true;
    m.godMode = true;
    m.serialize(s);
    fixtures.push(["edit", hexOf(s)]);
}

// ---- 8. MapMsg ----
{
    const buf = new ArrayBuffer(512);
    const s = new net.BitStream(buf);
    const m = new msgs.MapMsg();
    m.mapName = "main";
    m.seed = 123456;
    m.width = 512;
    m.height = 512;
    m.shoreInset = 4;
    m.grassInset = 2;
    m.rivers = [
        { width: 3, looped: false, points: [{ x: 10, y: 20 }, { x: 30, y: 40 }] },
    ];
    m.places = [{ name: "Town", pos: { x: 0.5, y: 0.5 } }];
    m.objects = [
        { pos: { x: 100, y: 200 }, scale: 1.0, type: "barrel_01", ori: 2 },
    ];
    m.groundPatches = [
        {
            bound: { type: 1, min: { x: 1, y: 2 }, max: { x: 3, y: 4 } },
            color: 0x112233,
            roughness: 0.5,
            offsetDist: 0.25,
            order: 3,
            useAsMapShape: true,
        },
    ];
    m.serialize(s);
    fixtures.push(["map", hexOf(s)]);
}

// ---- 9. UpdateMsg (full objects + active player + gas + bullets) ----
{
    const buf = new ArrayBuffer(1024);
    const s = new net.BitStream(buf);

// build the partial + full streams the same way the server does
    // (see server/src/game/objects/gameObject.ts serializePartial/serializeFull)
    const objType = 1; // Player
    const objId = 5;
    const data = {
        pos: { x: 12.5, y: 25.75 },
        dir: { x: 0.1, y: -0.99 },
        outfit: "outfitBase",
        backpack: "backpack00",
        helmet: "helmet02",
        chest: "chest02",
        activeWeapon: "ak47",
        layer: 1,
        dead: false,
        downed: false,
        animType: 1,
        animSeq: 2,
        actionType: 2,
        actionSeq: 3,
        wearingPan: true,
        healEffect: false,
        lastStandEffect: false,
        frozen: false,
        hasteType: 1,
        hasteSeq: 1,
        actionItem: "bandage",
        scale: 1.1,
        role: "",
        perks: [{ type: "steelskin", droppable: false }],
    };
    const objSer = (await import(pathToFile(sharedDir, "net/objectSerializeFns.ts"))).ObjectSerializeFns;
    const partial = new net.BitStream(new ArrayBuffer(64));
    partial.writeUint8(objType);
    partial.writeUint16(objId);
    objSer[objType].serializePart(partial, data);
    partial.writeAlignToNextByte();

    const full = new net.BitStream(new ArrayBuffer(128));
    objSer[objType].serializeFull(full, data);
    full.writeAlignToNextByte();

    const u = new msgs.UpdateMsg();
    u.delObjIds = [1, 2, 3];
    u.fullObjects = [{
        ...data,
        __id: 5,
        __type: objType,
        partialStream: partial,
        fullStream: full,
    }];
    u.activePlayerId = 5;
    u.activePlayerIdDirty = true;
    u.activePlayerData = {
        healthDirty: true,
        health: 75.5,
        boostDirty: false,
        boost: 0,
        zoomDirty: true,
        zoom: 2,
        actionDirty: false,
        inventoryDirty: true,
        scope: "2xscope",
        inventory: { "9mm": 30, "bandage": 3 },
        weapsDirty: true,
        curWeapIdx: 1,
        weapons: [
            { type: "ak47", ammo: 30 },
            { type: "fists", ammo: 0 },
            { type: "bandage", ammo: 0 },
            { type: "frag", ammo: 0 },
        ],
        spectatorCountDirty: false,
        spectatorCount: 0,
    };
    u.gasDirty = true;
    u.gasData = {
        mode: 1,
        duration: 10.5,
        posOld: { x: 0, y: 0 },
        posNew: { x: 100, y: 100 },
        radOld: 50,
        radNew: 25,
    };
    u.playerInfos = [{
        playerId: 5,
        teamId: 1,
        groupId: 0,
        name: "TestPlayer",
        loadout: { heal: "bandage", boost: "soda" },
    }];
    u.bullets = [{
        playerId: 5,
        startPos: { x: 12.5, y: 25.75 },
        pos: { x: 12.5, y: 25.75 },
        dir: { x: 0.1, y: -0.99 },
        bulletType: "bullet_ak47",
        layer: 1,
        varianceT: 0.5,
        distAdjIdx: 2,
        clipDistance: true,
        distance: 100,
        shotFx: true,
        shotSourceType: "ak47",
        shotOffhand: false,
        lastShot: false,
        reflectCount: 1,
        reflectObjId: 9,
        hasModifier: true,
        speedMult: 1.2,
        distanceMult: 1.5,
        hasSpecialFx: true,
        shotAlt: true,
        splinter: false,
        trailSaturated: false,
        apRounds: true,
        highVelocity: false,
        combatStims: true,
        trailSmall: false,
        trailThick: true,
    }];
    u.killLeaderDirty = true;
    u.killLeaderId = 5;
    u.killLeaderKills = 7;
    u.ack = 9;
    u.serialize(s);
    fixtures.push(["update-full", hexOf(s)]);
}

// ---- 10. KillMsg ----
{
    const buf = new ArrayBuffer(64);
    const s = new net.BitStream(buf);
    const m = new msgs.KillMsg();
    m.itemSourceType = "ak47";
    m.mapSourceType = "barrel_01";
    m.damageType = 0;
    m.targetId = 5;
    m.killerId = 6;
    m.killCreditId = 6;
    m.killerKills = 3;
    m.downed = false;
    m.killed = true;
    m.serialize(s);
    fixtures.push(["kill", hexOf(s)]);
}

// ---- 11. GameOverMsg ----
{
    const buf = new ArrayBuffer(256);
    const s = new net.BitStream(buf);
    const m = new msgs.GameOverMsg();
    m.teamId = 1;
    m.teamRank = 2;
    m.gameOver = true;
    m.winningTeamId = 1;
    m.playerStats = [
        { playerId: 5, timeAlive: 120, kills: 3, dead: true, damageDealt: 100.4, damageTaken: 50.6 },
    ];
    m.serialize(s);
    fixtures.push(["gameover", hexOf(s)]);
}

// ---- 12. EmoteMsg ----
{
    const buf = new ArrayBuffer(64);
    const s = new net.BitStream(buf);
    const m = new msgs.EmoteMsg();
    m.pos = { x: 10, y: 20 };
    m.type = "emote_happyface";
    m.isPing = true;
    m.serialize(s);
    fixtures.push(["emote", hexOf(s)]);
}

// ---- 13. PickupMsg ----
{
    const buf = new ArrayBuffer(64);
    const s = new net.BitStream(buf);
    const m = new msgs.PickupMsg();
    m.type = 4;
    m.item = "ak47";
    m.count = 1;
    m.serialize(s);
    fixtures.push(["pickup", hexOf(s)]);
}

// ---- 14. DropItemMsg / PerkModeRoleSelectMsg / RoleAnnouncementMsg / AliveCountsMsg / SpectateMsg ----
{
    const buf = new ArrayBuffer(64);
    const s = new net.BitStream(buf);
    const m = new msgs.DropItemMsg();
    m.item = "ak47";
    m.weapIdx = 1;
    m.serialize(s);
    fixtures.push(["dropitem", hexOf(s)]);
}
{
    const buf = new ArrayBuffer(64);
    const s = new net.BitStream(buf);
    const m = new msgs.PerkModeRoleSelectMsg();
    m.role = "medic";
    m.serialize(s);
    fixtures.push(["perkmoderoleselect", hexOf(s)]);
}
{
    const buf = new ArrayBuffer(64);
    const s = new net.BitStream(buf);
    const m = new msgs.RoleAnnouncementMsg();
    m.playerId = 1;
    m.killerId = 2;
    m.role = "medic";
    m.assigned = true;
    m.killed = false;
    m.serialize(s);
    fixtures.push(["roleannouncement", hexOf(s)]);
}
{
    const buf = new ArrayBuffer(64);
    const s = new net.BitStream(buf);
    const m = new msgs.AliveCountsMsg();
    m.teamAliveCounts = [4, 2, 3, 1];
    m.serialize(s);
    fixtures.push(["alivecounts", hexOf(s)]);
}
{
    const buf = new ArrayBuffer(64);
    const s = new net.BitStream(buf);
    const m = new msgs.SpectateMsg();
    m.action = 1;
    m.serialize(s);
    fixtures.push(["spectate", hexOf(s)]);
}

// ---- 15. MsgStream serializeMsg ----
{
    const stream = new net.MsgStream(new ArrayBuffer(256));
    const join = new msgs.JoinMsg();
    join.protocol = 1028;
    join.joinToken = "tok";
    join.name = "Bob";
    join.loadout = { outfit: "outfitBase", melee: "fists", heal: "bandage", boost: "soda", emotes: [] };
    stream.serializeMsg(net.MsgType.Join, join);
    fixtures.push(["msgstream-join", hexOf(stream.stream)]);
}

let out = `// GENERATED FILE - do not edit. Run tools/gen_reference.mjs to regenerate.
#pragma once
#include <string>
#include <vector>
#include <utility>

namespace surv_reference {

inline const std::vector<std::pair<std::string, std::string>>& fixtures() {
    static const std::vector<std::pair<std::string, std::string>> data = {
`;
for (const [name, hex] of fixtures) {
    out += `        {"${name}", "${hex}"},\n`;
}
out += `    };
    return data;
}

} // namespace surv_reference
`;

fs.writeFileSync(path.join(here, "../tests/ReferenceData.h"), out);
console.log(`wrote ${fixtures.length} fixtures`);
