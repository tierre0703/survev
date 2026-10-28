// Generates src/render/GeneratedDefs.cpp from the real shared/ TS defs. Emits a
// concrete DefProvider (biome colors, map-object images/colliders, structure
// layers/stairs/mask, building floor/ceiling images + vision, loot images, and
// the particles.ts particle/emitter tables, player skins/held images, and
// animData.ts idle poses/animation keyframes) used by the M4 rendering port.
//
// Usage: node --experimental-transform-types tools/codegen_render_defs.mjs
import * as fs from "node:fs";
import * as path from "node:path";
import { fileURLToPath } from "node:url";

const here = path.dirname(fileURLToPath(import.meta.url));
const sharedDir = path.resolve(here, "../../shared");
const clientDir = path.resolve(here, "../../client");

function pathToFile(dir, rel) {
    return new URL(`file:///${path.join(dir, rel).replace(/\\/g, "/")}`);
}

const { MapObjectDefs, GameObjectDefs } = await import(pathToFile(sharedDir, "defs/register.ts"));
const { DecalDefs } = await import(pathToFile(sharedDir, "defs/mapObjects/decalDefs.ts"));
const { EmotesDefs, EmoteCategory } = await import(pathToFile(sharedDir, "defs/gameObjects/emoteDefs.ts"));
const { FactionTeam } = await import(pathToFile(sharedDir, "gameConfig.ts"));
const { PingDefs } = await import(pathToFile(sharedDir, "defs/gameObjects/pingDefs.ts"));
const { RoleDefs } = await import(pathToFile(sharedDir, "defs/gameObjects/roleDefs.ts"));
const { MapDefs } = await import(pathToFile(sharedDir, "defs/mapDefs.ts"));
const { util } = await import(pathToFile(sharedDir, "utils/util.ts"));
const { math } = await import(pathToFile(sharedDir, "utils/math.ts"));
const { v2 } = await import(pathToFile(sharedDir, "utils/v2.ts"));
const { IdlePoses, Animations } = await import(pathToFile(clientDir, "src/animData.ts"));
const { collider } = await import(pathToFile(sharedDir, "utils/collider.ts"));
const { coldet } = await import(pathToFile(sharedDir, "utils/coldet.ts"));
const { ExplosionEffectDefs } = await import(pathToFile(sharedDir, "defs/explosionEffectDefs.ts"));

const mapTypes = MapObjectDefs.getAllTypes();
const gameTypes = GameObjectDefs.getAllTypes();

const n = (x, d = 0) => (Number.isFinite(Number(x)) ? Number(x) : d);
// shared/defs/mapDefs biome.sound.riverShore (SurfaceType string) -> Defs.h
// SurfaceType enum ordinal.
const surfaceTypeNames = [
    "water",
    "sand",
    "grass",
    "tile",
    "asphalt",
    "brick",
    "stone",
    "carpet",
    "bunker",
    "container",
    "warehouse",
    "house",
    "shack",
    "snow",
];
const surfaceTypeCode = (s) => {
    const i = surfaceTypeNames.indexOf(String(s ?? ""));
    return i < 0 ? 1 : i;
};
// Float literal helper: clamps out-of-float-range constants (Number.MAX_VALUE)
// so the generated braced initializers don't trip -Wc++11-narrowing.
const f = (x, d = 0) => {
    let v = Number.isFinite(Number(x)) ? Number(x) : d;
    if (v > 3.402823466e38) v = 3.402823466e38;
    if (v < -3.402823466e38) v = -3.402823466e38;
    return v;
};
const esc = (s) => String(s ?? "").replace(/\\/g, "\\\\").replace(/"/g, "\\\"");
const str = (s) => `"${esc(s)}"`;

function colInit(c) {
    if (!c || typeof c !== "object") return "{1,0,0,0,0}";
    if (c.type === 0) return `{0,${n(c.pos.x)},${n(c.pos.y)},${n(c.rad)},0}`;
    return `{1,${n(c.min.x)},${n(c.min.y)},${n(c.max.x)},${n(c.max.y)}}`;
}
function imgInit(img) {
    const i = img || {};
    return `{${str(i.sprite)},${n(i.scale, 1)},${n(i.alpha, 1)},${n(i.tint, 0xffffff)},${
        n(
            i.zIdx,
            0,
        )
    },${i.mirrorX ? 1 : 0},${i.mirrorY ? 1 : 0},${n(i.ori, 0)}}`;
}
// building FloorImage -> RawBImg.
function bimgInit(img) {
    const i = img || {};
    return `{${str(i.sprite)},${n(i.scale, 1)},${n(i.alpha, 1)},${n(i.tint, 0xffffff)},${
        n(
            i.pos?.x,
            0,
        )
    },${n(i.pos?.y, 0)},${n(i.rot, 0)},${i.mirrorX ? 1 : 0},${i.mirrorY ? 1 : 0},${i.removeOnDamaged ? 1 : 0}}`;
}
function unionBounds(cols) {
    let min = { x: Infinity, y: Infinity };
    let max = { x: -Infinity, y: -Infinity };
    for (const c of cols) {
        if (!c || typeof c !== "object") continue;
        let a, b;
        if (c.type === 0) {
            a = { x: c.pos.x - c.rad, y: c.pos.y - c.rad };
            b = { x: c.pos.x + c.rad, y: c.pos.y + c.rad };
        } else {
            a = c.min;
            b = c.max;
        }
        min.x = Math.min(min.x, a.x);
        min.y = Math.min(min.y, a.y);
        max.x = Math.max(max.x, b.x);
        max.y = Math.max(max.y, b.y);
    }
    if (!Number.isFinite(min.x)) return null;
    return { type: 1, min, max };
}

// Deterministic port of mapHelpers.computeBoundingCollider. Weighted map-object
// type maps are unioned over every candidate (the web picks one at random) so
// the generated file is reproducible; the candidates are décor inside the
// building footprint.
const boundingCache = new Map();
const boundingBusy = new Set();
function computeBoundingCollider(type) {
    if (boundingCache.has(type)) return boundingCache.get(type);
    if (boundingBusy.has(type)) return null;
    boundingBusy.add(type);
    const def = MapObjectDefs.typeToDef(type);
    let out = null;
    if (def.type === "structure") {
        const aabbs = [];
        for (const layer of def.layers || []) {
            const sub = computeBoundingCollider(layer.type);
            if (!sub) continue;
            const rot = math.oriToRad(layer.ori);
            aabbs.push(collider.toAabb(collider.transform(sub, layer.pos, rot, 1.0)));
        }
        for (const stair of def.stairs || []) aabbs.push(stair.collision);
        if (aabbs.length) {
            const aabb = coldet.boundingAabb(aabbs);
            // mapHelpers expands structure bounds by 1 unit (stairwell loot).
            out = collider.createAabb(
                v2.create(aabb.min.x - 1.0, aabb.min.y - 1.0),
                v2.create(aabb.max.x + 1.0, aabb.max.y + 1.0),
            );
        }
    } else if (def.type === "building") {
        const aabbs = [];
        for (const surface of def.floor?.surfaces || []) {
            for (const c of surface.collision || []) aabbs.push(c);
        }
        for (const region of def.ceiling?.zoomRegions || []) {
            if (region.zoomIn) aabbs.push(region.zoomIn);
            if (region.zoomOut) aabbs.push(region.zoomOut);
        }
        for (const obj of def.mapObjects || []) {
            const mt = obj.type;
            const types = mt && typeof mt === "object" ? Object.keys(mt).filter((k) => mt[k]) : [mt];
            for (const t of types) {
                if (!t) continue;
                const sub = computeBoundingCollider(t);
                if (!sub) continue;
                const rot = math.oriToRad(obj.ori);
                aabbs.push(collider.toAabb(collider.transform(sub, obj.pos, rot, obj.scale)));
            }
        }
        if (aabbs.length) {
            const aabb = coldet.boundingAabb(aabbs);
            out = collider.createAabb(aabb.min, aabb.max);
        }
    } else if (def.type === "decal") {
        out = collider.toAabb(def.collision);
    } else if (def.type === "loot_spawner") {
        out = collider.createCircle(v2.create(0.0, 0.0), 3.0);
    } else {
        out = def.collision || null;
    }
    boundingBusy.delete(type);
    boundingCache.set(type, out);
    return out;
}

let arrays = "";
let entries = "";
const emittedLayers = [];

for (let i = 0; i < mapTypes.length; i++) {
    const type = mapTypes[i];
    let def;
    try {
        def = MapObjectDefs.typeToDef(type);
    } catch {
        def = null;
    }
    if (!def) continue;
    const kind = def.type || "";

    const img = def.img || null;
    const hasImg = !!(img && img.sprite);
    const collision = def.collision || null;
    const hasCollision = !!collision;
    const map = def.map || null;
    const shapes = (map && Array.isArray(map.shapes) ? map.shapes : []).filter(Boolean);

    let layers = null;
    let stairs = null;
    let mask = null;
    let interiorSound = null;
    if (kind === "structure") {
        layers = def.layers || [];
        stairs = def.stairs || [];
        mask = def.mask || [];
        interiorSound = def.interiorSound || null;
    }

    let bounds = null;
    if (kind === "structure" || kind === "building" || kind === "decal" || kind === "loot_spawner" || collision) {
        bounds = computeBoundingCollider(type);
    }

    const refs = {
        layers: "nullptr",
        stairs: "nullptr",
        mask: "nullptr",
        shapes: "nullptr",
        floorImgs: "nullptr",
        ceilingImgs: "nullptr",
        emitters: "nullptr",
        zoomIns: "nullptr",
        surfaces: "nullptr",
        soundEmitters: "nullptr",
    };
    if (layers && layers.length) {
        const name = `kLayers_${i}`;
        arrays += `static const RawLayer ${name}[] = {\n`;
        for (const l of layers) {
            arrays += `    {${str(l.type)},${n(l.pos.x)},${n(l.pos.y)},${n(l.ori, 0)},${
                l.inheritOri === undefined || l.inheritOri ? 1 : 0
            },${l.underground === undefined ? 0 : 1},${l.underground ? 1 : 0}},\n`;
        }
        arrays += `};\n`;
        refs.layers = name;
        emittedLayers.push(name);
    }
    if (stairs && stairs.length) {
        const name = `kStairs_${i}`;
        arrays += `static const RawStair ${name}[] = {\n`;
        for (const s of stairs) {
            arrays += `    {${colInit(s.collision)},${n(s.downDir?.x)},${n(s.downDir?.y)},${
                s.noCeilingReveal ? 1 : 0
            },${s.lootOnly ? 1 : 0}},\n`;
        }
        arrays += `};\n`;
        refs.stairs = name;
    }
    if (mask && mask.length) {
        const name = `kMask_${i}`;
        arrays += `static const RawCollider ${name}[] = {\n`;
        for (const m of mask) arrays += `    ${colInit(m)},\n`;
        arrays += `};\n`;
        refs.mask = name;
    }
    if (shapes.length) {
        const name = `kShapes_${i}`;
        arrays += `static const RawShape ${name}[] = {\n`;
        for (const s of shapes) {
            arrays += `    {${colInit(s.collider)},${n(s.scale, 1)},${n(s.color, 0)}},\n`;
        }
        arrays += `};\n`;
        refs.shapes = name;
    }

    // Building floor/ceiling images + occupied emitters.
    let zIdx = 0;
    let vision = { dist: 5.5, width: 2.75, linger: 0, fadeRate: 12 };
    let destroyResidue = "";
    let floorImgs = [];
    let ceilingImgs = [];
    let emitters = [];
    let zoomIns = [];
    let floorSurfaces = [];
    let buildingSoundEmitters = [];
    if (kind === "building") {
        zIdx = n(def.zIdx, 0);
        floorImgs = (def.floor && def.floor.imgs) || [];
        ceilingImgs = (def.ceiling && def.ceiling.imgs) || [];
        floorSurfaces = (def.floor && def.floor.surfaces) || [];
        buildingSoundEmitters = def.soundEmitters || [];
        vision = Object.assign({ dist: 5.5, width: 2.75, linger: 0, fadeRate: 12 }, def.ceiling?.vision);
        destroyResidue = def.ceiling?.destroy?.residue || "";
        emitters = def.occupiedEmitters || [];
        zoomIns = (def.ceiling?.zoomRegions || [])
            .map((r) => r.zoomIn)
            .filter(Boolean);
    }
    if (floorImgs.length) {
        const name = `kFloorImgs_${i}`;
        arrays += `static const RawBImg ${name}[] = {\n`;
        for (const im of floorImgs) arrays += `    ${bimgInit(im)},\n`;
        arrays += `};\n`;
        refs.floorImgs = name;
    }
    if (ceilingImgs.length) {
        const name = `kCeilImgs_${i}`;
        arrays += `static const RawBImg ${name}[] = {\n`;
        for (const im of ceilingImgs) arrays += `    ${bimgInit(im)},\n`;
        arrays += `};\n`;
        refs.ceilingImgs = name;
    }
    if (emitters.length) {
        const name = `kEmitters_${i}`;
        arrays += `static const RawBEmitter ${name}[] = {\n`;
        for (const e of emitters) {
            arrays += `    {${str(e.type)},${n(e.pos?.x)},${n(e.pos?.y)},${n(e.rot, 0)},${
                n(
                    e.scale,
                    1,
                )
            },${n(e.layer, 0)},${e.parentToCeiling ? 1 : 0},${n(e.dir?.x, 1)},${n(e.dir?.y, 0)}},\n`;
        }
        arrays += `};\n`;
        refs.emitters = name;
    }
    if (zoomIns.length) {
        const name = `kZoomIns_${i}`;
        arrays += `static const RawCollider ${name}[] = {\n`;
        for (const z of zoomIns) arrays += `    ${colInit(z)},\n`;
        arrays += `};\n`;
        refs.zoomIns = name;
    }
    // building.ts `def.floor.surfaces[]`: floor material + collision AABBs for
    // map.ts getGroundSurface / footstep groups.
    if (floorSurfaces.length) {
        const name = `kSurfaces_${i}`;
        const colRefs = [];
        for (let j = 0; j < floorSurfaces.length; j++) {
            const cols = floorSurfaces[j].collision || [];
            if (!cols.length) {
                colRefs.push({ name: "nullptr", count: 0 });
                continue;
            }
            const cname = `kSurfCols_${i}_${j}`;
            arrays += `static const RawCollider ${cname}[] = {\n`;
            for (const c of cols) arrays += `    ${colInit(c)},\n`;
            arrays += `};\n`;
            colRefs.push({ name: cname, count: cols.length });
        }
        arrays += `static const RawBSurface ${name}[] = {\n`;
        for (let j = 0; j < floorSurfaces.length; j++) {
            const s = floorSurfaces[j];
            arrays += `    {${str(s.type)},${colRefs[j].name},${colRefs[j].count},${
                s.data && s.data.waterColor !== undefined ? 1 : 0
            },${n(s.data?.waterColor, 0)},${s.data && s.data.rippleColor !== undefined ? 1 : 0},${
                n(s.data?.rippleColor, 0)
            }},\n`;
        }
        arrays += `};\n`;
        refs.surfaces = name;
    }
    // building.ts `soundEmitters`: looping positional ambience points.
    if (buildingSoundEmitters.length) {
        const name = `kSoundEmitters_${i}`;
        arrays += `static const RawSoundEmitter ${name}[] = {\n`;
        for (const e of buildingSoundEmitters) {
            arrays += `    {${str(e.sound)},${str(e.channel)},${n(e.pos?.x)},${n(e.pos?.y)},${n(e.range?.min)},${
                n(e.range?.max)
            },${n(e.falloff, 1)},${n(e.volume, 1)}},\n`;
        }
        arrays += `};\n`;
        refs.soundEmitters = name;
    }

    // obstacle.ts collision/sound fields + decal.ts surface/gore fields.
    const defHeight = n(def.height, 0);
    const defCollidable = !!def.collidable;
    const defIsWindow = !!def.isWindow;
    const hitParticle = def.hitParticle || "";
    const punchSound = def.sound?.punch || "";
    const surface = def.surface || null;
    const gore = def.gore || null;
    const surfaceTypeId = surface ? (surface.type === "grass" ? 2 : surface.type === "sand" ? 1 : 0) : 0;
    const lifetime = def.lifetime;
    let lifetimeMin = 0, lifetimeMax = 0, hasLifetime = false;
    if (lifetime !== undefined) {
        hasLifetime = true;
        if (typeof lifetime === "number") {
            lifetimeMin = lifetimeMax = n(lifetime);
        } else {
            lifetimeMin = n(lifetime.min);
            lifetimeMax = n(lifetime.max);
        }
    }

    entries += `    {${str(type)},${imgInit(img)},${hasImg ? 1 : 0},${colInit(collision)},${hasCollision ? 1 : 0},${
        def.isDoor ? 1 : 0
    },${def.isButton ? 1 : 0},${def.isTree ? 1 : 0},${def.isBush ? 1 : 0},${def.isWall ? 1 : 0},${
        def.img && def.img.randomRotation ? 1 : 0
    },${def.explosion !== undefined ? 1 : 0},${
        str(
            Array.isArray(def.explosion) ? def.explosion[0] : def.explosion,
        )
    },${str(Array.isArray(def.explodeParticle) ? def.explodeParticle[0] : def.explodeParticle)},${
        str(Array.isArray(def.explodeParticle) && def.explodeParticle.length > 1 ? def.explodeParticle[1] : "")
    },${n(def.door?.slideOffset, 0)},${n(def.door?.openSpeed, 2)},${str(def.door?.casingImg?.sprite)},${
        n(
            def.door?.casingImg?.pos?.x,
            0,
        )
    },${n(def.door?.casingImg?.pos?.y, 0)},${n(def.door?.casingImg?.scale, 1)},${
        n(
            def.door?.casingImg?.tint,
            0xffffff,
        )
    },${n(def.door?.casingImg?.alpha, 1)},${n(def.door?.spriteAnchor?.x, 0.5)},${
        n(
            def.door?.spriteAnchor?.y,
            0.5,
        )
    },${defHeight},${defCollidable ? 1 : 0},${defIsWindow ? 1 : 0},${str(hitParticle)},${
        str(
            punchSound,
        )
    },${str(def.door?.sound?.change)},${str(def.door?.sound?.open)},${str(def.door?.sound?.close)},${
        str(
            def.door?.sound?.unlock,
        )
    },${str(def.button?.sound?.on)},${str(def.button?.sound?.off)},${str(def.sound?.explode)},${
        str(def.sound?.bullet)
    },${str(def.sound?.enter)},${str(def.button?.useImg)},${str(def.button?.offImg)},${str(def.button?.useParticle)},${
        surface ? 1 : 0
    },${surfaceTypeId},${n(surface?.data?.waterColor, 0)},${
        n(
            surface?.data?.rippleColor,
            0,
        )
    },${gore ? 1 : 0},${n(gore?.fade?.start, 0)},${n(gore?.fade?.end, 1)},${
        n(
            gore?.fade?.pow,
            1,
        )
    },${n(gore?.fade?.speed, 1)},${gore && gore.tint !== undefined ? 1 : 0},${
        n(
            gore?.tint,
            0xffffff,
        )
    },${gore && gore.alpha !== undefined ? 1 : 0},${n(gore?.alpha, 1)},${
        gore && gore.waterColor !== undefined ? 1 : 0
    },${n(gore?.waterColor, 0)},${gore && gore.rippleColor !== undefined ? 1 : 0},${
        n(
            gore?.rippleColor,
            0,
        )
    },${img?.flicker ? 1 : 0},${n(img?.flickerMin, 1)},${n(img?.flickerMax, 1)},${
        n(
            img?.flickerRate,
            1,
        )
    },${img?.ignoreAdjust ? 1 : 0},${hasLifetime ? 1 : 0},${lifetimeMin},${lifetimeMax},${
        map && map.display === false ? 0 : 1
    },${map && map.color !== undefined ? 1 : 0},${n(map?.color, 0)},${n(map?.scale, 1)},${
        colInit(
            bounds,
        )
    },${bounds ? 1 : 0},${refs.layers},${layers ? layers.length : 0},${refs.stairs},${
        stairs ? stairs.length : 0
    },${refs.mask},${
        mask ? mask.length : 0
    },${refs.shapes},${shapes.length},${zIdx},${refs.floorImgs},${floorImgs.length},${refs.ceilingImgs},${ceilingImgs.length},${
        n(vision.dist, 5.5)
    },${
        n(
            vision.width,
            2.75,
        )
    },${n(vision.linger, 0)},${n(vision.fadeRate, 12)},${
        str(destroyResidue)
    },${refs.emitters},${emitters.length},${refs.zoomIns},${zoomIns.length},${refs.surfaces},${floorSurfaces.length},${
        interiorSound ? 1 : 0
    },${str(interiorSound?.sound)},${str(interiorSound?.soundAlt)},${str(interiorSound?.filter)},${
        n(interiorSound?.transitionTime, 1)
    },${n(interiorSound?.outsideMaxDist, 10)},${n(interiorSound?.outsideVolume, 0)},${
        interiorSound && interiorSound.undergroundVolume !== undefined ? 1 : 0
    },${n(interiorSound?.undergroundVolume, 1)},${refs.soundEmitters},${buildingSoundEmitters.length},${
        str(def.puzzle?.sound?.fail)
    },${str(def.puzzle?.sound?.complete)},${str(def.ceiling?.destroy?.sound)},${str(def.ceiling?.destroy?.particle)},${
        n(def.ceiling?.destroy?.particleCount, 0)
    }},\n`;
}

// Map render defs.
let mapRenderEntries = "";
let mapAtlasArrays = "";
for (const [name, def] of Object.entries(MapDefs)) {
    const c = def?.biome?.colors || {};
    const gm = def?.gameMode || {};
    const atlases = (def?.assets?.atlases || []).filter(Boolean);
    const amb = def?.biome?.ambience || {};
    const atlasArr = `kMapAtlases_${mapRenderEntries.length}`;
    mapAtlasArrays += `static const char* const ${atlasArr}[] = {${atlases.map(str).join(",")}};\n`;
    // gameMode.perkModeRoles: ui.ts setRoleMenuOptions' class list (perk maps).
    const perkRoles = (gm.perkModeRoles || []).filter(Boolean);
    const rolesArr = `kMapRoles_${mapRenderEntries.length}`;
    if (perkRoles.length > 0) {
        mapAtlasArrays += `static const char* const ${rolesArr}[] = {${perkRoles.map(str).join(",")}};\n`;
    }
    mapRenderEntries += `    {${str(name)},${n(c.background, 0x2b2b2b)},${
        n(
            c.beach,
            0xd9c38a,
        )
    },${n(c.grass, 0x5a9e4c)},${n(c.riverbank, 0xc2b280)},${n(c.water, 0x3a6ea5)},${
        n(
            c.waterRipple,
            0x5a8ec5,
        )
    },${n(c.lakeRiverbank, n(c.riverbank, 0xc2b280))},${n(c.lakeWater, n(c.water, 0x3a6ea5))},${
        n(
            c.lakeWaterRipple,
            n(c.waterRipple, 0x5a8ec5),
        )
    },${n(c.underground, 0x1b0e0b)},${gm.factionMode ? 1 : 0},${gm.potatoMode ? 1 : 0},${gm.perkMode ? 1 : 0},${
        gm.turkeyMode ? 1 : 0
    },${n(def?.biome?.valueAdjust, 1)},${
        str(
            def?.biome?.particles?.camera || "",
        )
    },${atlasArr},${atlases.length},${str(amb.music || "")},${str(amb.wind || "")},${
        str(
            amb.river || "",
        )
    },${str(amb.waves || "")},${str(def?.biome?.airdrop?.planeImg || "map-plane-01.img")},${
        str(
            def?.biome?.airdrop?.planeSound || "plane_01",
        )
    },${str(def?.biome?.airdrop?.airdropImg || "map-chute-01.img")},${
        surfaceTypeCode(def?.biome?.sound?.riverShore || "sand")
    },${perkRoles.length > 0 ? rolesArr : "nullptr"},${perkRoles.length}},\n`;
}

// Game object render defs (loot images and separate player skin/held images).
let gameEntries = "";
let playerEntries = "";
const fp = (v, d = 0) => {
    const value = String(f(v, d));
    return `${value.includes(".") || /e/i.test(value) ? value : value + ".0"}f`;
};
const vec = (v, d = 0) => `Vec2(${fp(v?.x, d)},${fp(v?.y, d)})`;
function skinAssignments(s, dst) {
    if (!s) return "";
    return `${dst}.baseSprite=${str(s.baseSprite)};
        ${dst}.handL=${str(typeof s.handSprite === "string" ? s.handSprite : s.handSprite?.left)};
        ${dst}.handR=${str(typeof s.handSprite === "string" ? s.handSprite : s.handSprite?.right)};
        ${dst}.footSprite=${str(s.footSprite)}; ${dst}.backpackSprite=${str(s.backpackSprite)};
        ${dst}.baseTint=${n(s.baseTint, 0xffffff)}; ${dst}.handTint=${n(s.handTint, 0xffffff)};
        ${dst}.footTint=${n(s.footTint, 0xffffff)}; ${dst}.backpackTint=${n(s.backpackTint, 0xffffff)};
        ${dst}.baseTintRed=${n(s.baseTintRed, n(s.baseTint, 0xffffff))};
        ${dst}.baseTintBlue=${n(s.baseTintBlue, n(s.baseTint, 0xffffff))};
        ${dst}.spriteScale=${fp(s.spriteScale, 0.15)};`;
}
function heldAssignments(im, dst) {
    if (!im) return "";
    const scale = typeof im.scale === "number" ? { x: im.scale, y: im.scale } : im.scale;
    return `${dst}.sprite=${str(im.sprite)}; ${dst}.pos=${vec(im.pos)};
        ${dst}.scale=${vec(scale, 1)}; ${dst}.rot=${fp(im.rot)}; ${dst}.tint=${n(im.tint, 0xffffff)};
        ${dst}.renderOnHand=${!!im.renderOnHand}; ${dst}.leftHandOnTop=${!!im.leftHandOntop};
        ${dst}.handsBelow=${!!im.handsBelow};`;
}
for (const type of gameTypes) {
    let def;
    try {
        def = GameObjectDefs.typeToDef(type);
    } catch {
        def = null;
    }
    if (!def) continue;
    const img = def.lootImg || def.img || null;
    const atk = def.attack || null;
    // loot.ts: the item's outer ring, tint (falls back to the ammo's dark tint),
    // inner art scale/rotation/mirror.
    const lootImg = def.lootImg || null;
    let lootBorderTint = lootImg?.borderTint ?? 0;
    if (lootImg && lootImg.borderTint === undefined && def.ammo) {
        try {
            const ammo = GameObjectDefs.typeToDef(def.ammo);
            if (ammo?.lootImg?.tintDark !== undefined) lootBorderTint = ammo.lootImg.tintDark;
        } catch {
            // unknown ammo type: keep the default black ring tint
        }
    }
    const lootBorder = lootImg?.border || "";
    const lootInnerScale = lootImg?.innerScale ?? 0.8;
    const lootRot = lootImg?.rot ?? 0;
    const lootMirror = !!lootImg?.mirror;
    // projectile.ts: throwable trail + spin/physics.
    const trailDef = def.trail || null;
    const throwPhysics = def.throwPhysics || {};
    if (def.unlocks || def.ammoType || def.ammo) {
        playerEntries += `        _gameObjs.at(${str(type)}).unlocks={${(def.unlocks || []).map(str).join(",")}};
        _gameObjs.at(${str(type)}).ammoType=${str(def.ammo || def.ammoType || "")};\n`;
    }
    gameEntries += `    {${str(type)},${str(def.type || "")},${imgInit(img)},${img?.sprite ? 1 : 0},${
        str(def.emitter || "")
    },${str(def.aura?.sprite)},${n(def.aura?.tint, 0xff00ff)},${def.aura !== undefined ? 1 : 0},${atk ? 1 : 0},${
        n(atk?.offset?.x, 0)
    },${n(atk?.offset?.y, 0)},${n(atk?.rad, 0)},${def.cleave ? 1 : 0},${str(def.sound?.playerHit)},${
        str(def.sound?.punch)
    },${def.useThrowParticles ? 1 : 0},${str(def.bulletType)},${fp(def.barrelLength)},${
        str(def.mapIndicator?.sprite || "")
    },${n(def.mapIndicator?.tint, 0xffffff)},${def.mapIndicator?.pulse ? 1 : 0},${
        n(def.mapIndicator?.pulseTint, 0xffffff)
    },${str(def.sound?.reload)},${str(def.sound?.reloadAlt)},${str(def.sound?.deploy)},${str(def.sound?.use)},${
        str(def.sound?.pickup)
    },${str(lootBorder)},${n(lootBorderTint, 0)},${f(lootInnerScale, 0.8)},${f(lootRot, 0)},${lootMirror ? 1 : 0},${
        str(def.explosionEffectType || "")
    },${trailDef ? 1 : 0},${f(trailDef?.maxLength, 0)},${f(trailDef?.width, 0)},${f(trailDef?.alpha, 1)},${
        n(trailDef?.tint, 0xffffff)
    },${f(throwPhysics.speed, 20)},${f(throwPhysics.spinVel, 0)},${f(throwPhysics.spinDrag, 1)},${
        throwPhysics.randomizeSpinDir ? 1 : 0
    },${def.alwaysRenderOntop ? 1 : 0},${f(def.rad, 1)},${str(def.particle?.casing || def.ammo || "")},${
        def.caseTiming === "reload" ? 1 : 0
    },${n(def.maxReload, 1)},${f(def.particle?.shellScale, 1)},${f(def.particle?.shellOffset, 0.375)},${
        f(def.particle?.shellOffsetY, 0)
    },${f(def.particle?.shellForward, 0)},${def.particle?.shellReverse ? 1 : 0},${str(def.sound?.shoot)},${
        str(def.sound?.shootLast)
    },${str(def.sound?.shootAlt)},${str(def.sound?.shootTeam?.[FactionTeam.Red])},${
        str(def.sound?.shootTeam?.[FactionTeam.Blue])
    },${str(def.sound?.cycle)},${str(def.sound?.pull)},${f(def.sound?.fallOff, 0)},${def.ignoreDetune ? 1 : 0},${
        def.fireMode === "single" ? 1 : 0
    },${f(def.pullDelay, 0)},${def.pullDelay !== undefined ? 1 : 0},${def.explodeOnImpact ? 1 : 0},${
        str(def.sound?.empty)
    },${def.ammoInfinite ? 1 : 0}},\n`;
    if (
        !def.skinImg && !def.visorImg && !def.worldImg && !def.hipImg && !def.anim && def.type !== "backpack"
        && !def.frozenSprites
    ) continue;
    let idlePose = def.anim?.idlePose || "fists";
    if (def.type === "gun") {
        idlePose = def.pistol
            ? (def.isDual ? "dualPistol" : "pistol")
            : def.isBullpup
            ? "bullpup"
            : def.isLauncher
            ? "launcher"
            : def.isMinigun
            ? "minigun"
            : def.isDual
            ? "dualRifle"
            : "rifle";
    }
    if (def.type === "throwable") idlePose = "throwable";
    playerEntries += `        { auto& d = _gameObjs.at(${str(type)});
        ${skinAssignments(def.skinImg, "d.skin")} ${skinAssignments(def.visorImg, "d.visor")}
        ${heldAssignments(def.worldImg, "d.worldImg")} ${heldAssignments(def.hipImg, "d.hipImg")}
        d.ghillie=${!!def.ghillie}; d.isDual=${!!def.isDual}; d.level=${n(def.level)};
        d.gunOffset=${vec(def.worldImg?.gunOffset)}; d.leftHandOffset=${vec(def.worldImg?.leftHandOffset)};
        d.magSprite=${str(def.worldImg?.magImg?.sprite)}; d.magPos=${vec(def.worldImg?.magImg?.pos)};
        d.magTop=${!!def.worldImg?.magImg?.top}; d.idlePose=${str(idlePose)};
        d.recoil=${fp(def.worldImg?.recoil)};
        d.attackAnims={${(def.anim?.attackAnims || []).map(str).join(",")}};
        d.deployAnims={${(def.anim?.deployAnims || []).map(str).join(",")}};
        d.idleAnims={${(def.anim?.idleAnims || []).map(str).join(",")}};
        d.frozenSprites={${(def.frozenSprites || []).map(str).join(",")}};
        ${
        ["equip", "cook", "throwing"].flatMap((state, i) =>
            ["left", "right"].map((side, j) => heldAssignments(def.handImg?.[state]?.[side], `d.handImgs[${i}][${j}]`))
        ).join("\n")
    }
        }\n`;
}

// Loadout item metadata (shared/defs BaseLoadoutItem): the native customize
// screen lists the real outfit/melee/emote/heal/boost/crosshair defs with their
// display name, lore, sprite and rarity instead of the old hardcoded IDs.
const LOADOUT_DEF_TYPES = new Set(["outfit", "melee", "emote", "heal_effect", "boost_effect", "crosshair"]);
// emoteDefs.ts EmoteCategory -> localization key suffix ("faces", "logos", ...).
// Numeric enums keep their declaration order in Object.keys, so this tracks the
// enum instead of hardcoding the list.
const emoteCategoryKeys = Object.keys(EmoteCategory)
    .filter((key) => Number.isNaN(Number(key)))
    .map((key) => key.toLowerCase());
let loadoutEntries = "";
for (const type of gameTypes) {
    let def;
    try {
        def = GameObjectDefs.typeToDef(type);
    } catch {
        def = null;
    }
    if (!def || !LOADOUT_DEF_TYPES.has(def.type)) continue;
    const sprite = def.lootImg?.sprite || def.img?.sprite || def.texture || "";
    const subcat = def.type === "emote" ? (emoteCategoryKeys[def.category] || "") : "";
    loadoutEntries += `    {${str(type)},${str(def.type)},${str(def.name || "")},${str(def.lore || "")},${
        str(sprite)
    },${str(subcat)},${n(def.rarity, 0)},${n(def.lootImg?.tint, 0xffffff)}},\n`;
}

// Item "source" def display names: the server's items.source is a def type and
// the web resolves it through GameObjectDefs.typeToDefSafe(source)?.name
// (loadoutMenu.ts getItemSourceName). Only the grant-provider defs can be a
// source, and only unlock defs currently carry a name.
const SOURCE_DEF_TYPES = new Set(["unlock", "pass", "quest", "event"]);
let loadoutSourceEntries = "";
for (const type of gameTypes) {
    let def;
    try {
        def = GameObjectDefs.typeToDef(type);
    } catch {
        def = null;
    }
    if (!def || !SOURCE_DEF_TYPES.has(def.type) || !def.name) continue;
    loadoutSourceEntries += `    {${str(type)},${str(def.name)}},\n`;
}

const boneInit = (p) => `{${vec(p?.pivot)},${fp(p?.rot)},${vec(p?.pos)}}`;
const poseInit = (bones) => `{{${Array.from({ length: 6 }, (_, i) => boneInit(bones[i])).join(",")}}}`;
const easings = [
    [math.easeInSine, "InSine"],
    [math.easeOutSine, "OutSine"],
    [math.easeInOutSine, "InOutSine"],
    [math.easeOutQuart, "OutQuart"],
    [math.easeOutBounce, "OutBounce"],
    [math.easeOutQuad, "OutQuad"],
];
let poseEntries = "";
for (const [name, bones] of Object.entries(IdlePoses)) {
    poseEntries += `        _poses[${str(name)}]=${poseInit(bones)};\n`;
}
for (const [name, animation] of Object.entries(Animations)) {
    let noMask = 0;
    for (const frame of animation.keyframes) {
        for (const key of Object.keys(frame.bones)) noMask |= 1 << Number(key);
    }
    const frames = animation.keyframes.map((frame) => {
        const mask = Object.keys(frame.bones).reduce((bits, key) => bits | (1 << Number(key)), 0);
        const easing = easings.find(([fn]) => fn === frame.easing)?.[1] || "Linear";
        if (frame.easing && easing === "Linear") throw new Error(`Unknown easing in ${name}`);
        return `{${fp(frame.time)},${mask},${poseInit(frame.bones)},PoseEasing::${easing},${noMask & ~mask}}`;
    });
    poseEntries += `        _animations[${str(name)}].keyframes={${frames.join(",")}};\n`;
    const effects = (animation.effects || []).map((e) => {
        let fn = "AnimEffectFn::PlaySound", sound = "", playerHit = "";
        if (e.fn === "animMeleeCollision") {
            fn = "AnimEffectFn::MeleeCollision";
            playerHit = e.args?.playerHit || "";
        } else if (e.fn === "animThrowableParticles") {
            fn = "AnimEffectFn::ThrowableParticles";
        } else if (e.fn === "animSetThrowableState") {
            // throwableState is already derived from the cook/throw state each
            // frame, so this effect has no ported runtime side effect.
            return `{${fp(e.time)},AnimEffectFn::ThrowableParticles,"__noop__",""}`;
        } else if (e.fn === "animPlaySound") {
            sound = e.args?.sound || "";
        } else {
            throw new Error(`Unknown anim effect fn ${e.fn} in ${name}`);
        }
        return `{${fp(e.time)},${fn},${str(sound)},${str(playerHit)}}`;
    });
    poseEntries += `        _animations[${str(name)}].effects={${effects.join(",")}};\n`;
}

// --- particles.ts particle/emitter defs -----------------------------------
// particles.ts is a client module (imports PIXI/SDK), so we don't import it;
// instead we extract the two plain object literals and evaluate them with the
// shared utils + a local Range shim. The color closures are sampled to a
// representative RGB (the runtime varies per particle).
class Range {
    constructor(min, max) {
        this.min = min;
        this.max = max;
    }
    getRandom() {
        return util.random(this.min, this.max);
    }
}
function extractLiteral(src, name) {
    const marker = `const ${name}`;
    let at = src.indexOf(marker);
    if (at < 0) throw new Error(`codegen: missing ${name} in particles.ts`);
    at = src.indexOf("{", at);
    let depth = 0;
    let i = at;
    for (; i < src.length; i++) {
        const c = src[i];
        if (c === "{") depth++;
        else if (c === "}") {
            depth--;
            if (depth === 0) {
                i++;
                break;
            }
        }
    }
    return src.slice(at, i);
}

const particlesSrc = fs.readFileSync(path.join(clientDir, "src/objects/particles.ts"), "utf8");
// eslint-disable-next-line no-eval
const ParticleDefs = eval("(" + extractLiteral(particlesSrc, "ParticleDefs") + ")");
// eslint-disable-next-line no-eval
const EmitterDefs = eval("(" + extractLiteral(particlesSrc, "EmitterDefs") + ")");

function flat(v, dmin = 0, dmax = 0) {
    if (v === undefined || v === null) return { min: dmin, max: dmax, isConstant: 1 };
    if (typeof v === "number") return { min: v, max: v, isConstant: 1 };
    if (typeof v === "object" && typeof v.min === "number") {
        return { min: v.min, max: v.max, isConstant: v.min === v.max ? 1 : 0 };
    }
    return { min: dmin, max: dmax, isConstant: 1 };
}
const rangeInit = (r) => `{${n(r.min)},${n(r.max)},${r.isConstant}}`;

function sampleColor(color) {
    if (color === undefined) {
        return {
            has: 0,
            value: 0xffffff,
            variance: 0,
            hMin: 0,
            hMax: 0,
            sMin: 1,
            sMax: 1,
            vMin: 1,
            vMax: 1,
        };
    }
    if (typeof color === "function") {
        // Most particles.ts colors are `rgbToInt(hsvToRgb(H, S, random(v0, v1)))`.
        // Record the HSV arguments at the random endpoints so the runtime can
        // pick a fresh color per particle instead of a codegen-time average.
        const origRandom = util.random;
        const origHsv = util.hsvToRgb;
        const origMathRandom = Math.random;
        const samples = [];
        let recorded = null;
        util.hsvToRgb = (h, s, v) => {
            recorded = { h, s, v };
            return origHsv(h, s, v);
        };
        try {
            for (const t of [0, 1]) {
                recorded = null;
                // Some closures branch on Math.random (e.g. particles.ts
                // xp_common); pin it too so regeneration stays deterministic.
                Math.random = () => t;
                util.random = (a, b) => a + (b - a) * t;
                color();
                if (recorded) samples.push(recorded);
            }
        } finally {
            util.random = origRandom;
            util.hsvToRgb = origHsv;
            Math.random = origMathRandom;
        }
        if (samples.length === 2) {
            const [lo, hi] = samples;
            return {
                has: 1,
                value: 0,
                variance: 1,
                hMin: lo.h,
                hMax: hi.h,
                sMin: lo.s,
                sMax: hi.s,
                vMin: lo.v,
                vMax: hi.v,
            };
        }
        return {
            has: 1,
            value: n(color()) & 0xffffff,
            variance: 0,
            hMin: 0,
            hMax: 0,
            sMin: 1,
            sMax: 1,
            vMin: 1,
            vMax: 1,
        };
    }
    return { has: 1, value: n(color) & 0xffffff, variance: 0, hMin: 0, hMax: 0, sMin: 1, sMax: 1, vMin: 1, vMax: 1 };
}

let particleArrays = "";
let particleEntries = "";
for (const [name, def] of Object.entries(ParticleDefs)) {
    const images = Array.isArray(def.image) ? def.image : [def.image];
    const arrName = `kParticleImgs_${particleEntries.length}`;
    particleArrays += `static const char* const ${arrName}[] = {${images.map(str).join(",")}};\n`;
    const life = flat(def.life, 1, 1);
    const drag = flat(def.drag, 0, 0);
    const rotVel = flat(def.rotVel, 0, 0);
    const scaleStart = flat(def.scale?.start, 1, 1);
    const scaleEnd = flat(def.scale?.end, 0, 0);
    const scaleLerp = flat(def.scale?.lerp, 0, 1);
    const scaleUseExp = def.scale?.exp !== undefined ? 1 : 0;
    const alphaLerp = flat(def.alpha?.lerp, 0, 1);
    const alphaUseExp = def.alpha?.exp !== undefined ? 1 : 0;
    const hasAlphaIn = def.alphaIn !== undefined ? 1 : 0;
    const alphaInLerp = flat(def.alphaIn?.lerp, 0, 1);
    const color = sampleColor(def.color);
    particleEntries += `    {${str(name)},${arrName},${images.length},${n(def.zOrd, 20)},${
        rangeInit(
            life,
        )
    },${rangeInit(drag)},${rangeInit(rotVel)},${rangeInit(scaleStart)},${
        rangeInit(
            scaleEnd,
        )
    },${rangeInit(scaleLerp)},${scaleUseExp},${n(def.scale?.exp)},${n(def.alpha?.start, 1)},${
        n(
            def.alpha?.end,
            0,
        )
    },${rangeInit(alphaLerp)},${alphaUseExp},${n(def.alpha?.exp)},${hasAlphaIn},${
        n(
            def.alphaIn?.start,
            0,
        )
    },${n(def.alphaIn?.end, 0)},${rangeInit(alphaInLerp)},${color.has},${color.value},${
        def.ignoreValueAdjust ? 1 : 0
    },${color.variance},${f(color.hMin)},${f(color.hMax)},${f(color.sMin)},${f(color.sMax)},${f(color.vMin)},${
        f(color.vMax)
    }},\n`;
}

let emitterEntries = "";
for (const [name, def] of Object.entries(EmitterDefs)) {
    const rate = flat(def.rate, 1, 1);
    const speed = flat(def.speed, 0, 0);
    const rot = flat(def.rot, 0, 0);
    const hasRot = def.rot !== undefined ? 1 : 0;
    const maxRate = flat(def.maxRate, 0, 0);
    const hasMaxRate = def.maxRate !== undefined ? 1 : 0;
    const hasZOrd = def.zOrd !== undefined ? 1 : 0;
    emitterEntries += `    {${str(name)},${str(def.particle)},${rangeInit(rate)},${
        n(
            def.radius,
        )
    },${rangeInit(speed)},${n(def.angle)},${hasRot},${rangeInit(rot)},${
        f(
            def.maxCount,
            Number.MAX_VALUE,
        )
    },${hasMaxRate},${rangeInit(maxRate)},${n(def.maxElapsed)},${hasZOrd},${
        n(
            def.zOrd,
        )
    }},\n`;
}

// Emote defs (shared/defs/gameObjects/emoteDefs.ts): world emote sprites and
// the emote wheel take the atlas frame, sound and teamOnly flag from here.
let emoteEntries = "";
for (const type of Object.keys(EmotesDefs)) {
    const d = EmotesDefs[type] || {};
    emoteEntries += `    {${str(type)},${str(d.texture || "")},${str(d.sound || "")},${str(d.channel || "")},${
        str(d.name || "")
    },${d.teamOnly ? 1 : 0},${n(d.category)}},\n`;
}

// Ping defs (shared/defs/gameObjects/pingDefs.ts): the world ping indicator,
// the team-ping wheel icons and the minimap ping sprites all read this table.
let pingEntries = "";
for (const type of Object.keys(PingDefs)) {
    const d = PingDefs[type] || {};
    pingEntries += `    {${str(type)},${str(d.texture || "")},${str(d.mapTexture || "")},${str(d.sound || "")},${
        str(d.soundLeader || "")
    },${d.pingMap ? 1 : 0},${f(d.pingLife, 4)},${f(d.mapLife, 4)},${d.worldDisplay ? 1 : 0},${d.mapEvent ? 1 : 0},${
        n(d.tint, 0xffffff)
    }},\n`;
}

// Role defs (shared/defs/gameObjects/roleDefs.ts): role-specific map icons
// (leader/captain/medic/last-man) used by the minimap/big-map player dots, plus
// the kill-feed/announcement metadata (web getRoleAssignedKillFeedText etc.).
const enStrings = JSON.parse(fs.readFileSync(path.join(clientDir, "src/en.json"), "utf8"));
const roleName = (key, fallback) => enStrings[key] || fallback;
const hexColor = (value) => {
    if (typeof value !== "string") return 0;
    const m = /^#?([0-9a-f]{6})$/i.exec(value.trim());
    return m ? parseInt(m[1], 16) : 0;
};
let roleEntries = "";
let roleArrays = "";
let roleIndex = 0;
for (const type of Object.keys(RoleDefs)) {
    const d = RoleDefs[type] || {};
    const icon = d.mapIcon || {};
    const sound = d.sound || {};
    const kf = d.killFeed || {};
    const name = roleName(`game-${type}`, type);
    // ui.ts setRoleMenuInfo: class art + border colour + the perk list. The web
    // guiImg is an SVG (`img/gui/role-healer.svg`); the native GUI build
    // rasterizes it to `Content/gui/role-healer.png` (tools/build-gui-icons).
    const guiImg = d.guiImg ? d.guiImg.replace(/^img\//, "").replace(/\.svg$/, ".png") : "";
    // A few roles (last_man) pick perks with weightedRandom closures; pin
    // Math.random so the generated table is deterministic.
    const origMathRandom = Math.random;
    Math.random = () => 0;
    const perkTypes = (d.perks || []).map((p) => String(typeof p === "function" ? p() : p));
    Math.random = origMathRandom;
    let perksPtr = "nullptr";
    let perkNamesPtr = "nullptr";
    if (perkTypes.length > 0) {
        perksPtr = `kRolePerks_${roleIndex}`;
        perkNamesPtr = `kRolePerkNames_${roleIndex}`;
        roleArrays += `static const char* const ${perksPtr}[] = {${perkTypes.map(str).join(",")}};\n`;
        roleArrays += `static const char* const ${perkNamesPtr}[] = {${
            perkTypes.map((p) => str(roleName(`game-${p}`, p))).join(",")
        }};\n`;
    }
    roleEntries += `    {${str(type)},${str(icon.alive || "")},${str(icon.dead || "")},${str(sound.assign || "")},${
        str(sound.dead || "")
    },${str(name)},${str(roleName("game-red-leader", "Red Commander"))},${
        str(roleName("game-blue-leader", "Blue Commander"))
    },${kf.assign ? 1 : 0},${kf.dead ? 1 : 0},${kf.color ? 1 : 0},${n(hexColor(kf.color), 0)},${str(guiImg)},${
        d.color !== undefined ? 1 : 0
    },${n(d.color, 0)},${perksPtr},${perkTypes.length},${perkNamesPtr}},\n`;
    roleIndex++;
}

// GameConfig plane tuning + bullet tracer colours (plane.ts / flare.ts / shot.ts).
const { GameConfig } = await import(pathToFile(sharedDir, "gameConfig.ts"));
const { BulletDefs } = await import(pathToFile(sharedDir, "defs/gameObjects/bulletDefs.ts"));
let gameConfigArrays = "";
let tracerNamesCount = 0;
let bulletNamesCount = 0;
let explosionEffectCount = 0;
{
    const planeEntries = [
        ["Airdrop", GameConfig.airdrop],
        ["Airstrike", GameConfig.airstrike],
    ];
    gameConfigArrays += `static const RawPlane kPlanes[] = {\n`;
    for (const [, c] of planeEntries) {
        gameConfigArrays += `    {${n(c.actionOffset, 0)},${f(c.planeVel, 48)},${f(c.planeRad, 150)},${
            f(
                c.soundRangeMult,
                2.5,
            )
        },${f(c.soundRangeDelta, 0.25)},${f(c.soundRangeMax, 92)},${f(c.fallOff, 0)}},\n`;
    }
    gameConfigArrays += `};\n`;

    const tracerNames = Object.keys(GameConfig.tracerColors || {});
    const tracerArray = "kTracerColors";
    gameConfigArrays += `static const char* const ${tracerArray}_names[] = {${
        tracerNames
            .map(str)
            .join(",")
    }};\n`;
    gameConfigArrays += `static const RawTracer ${tracerArray}[] = {\n`;
    for (const name of tracerNames) {
        const t = GameConfig.tracerColors[name] || {};
        gameConfigArrays += `    {${n(t.regular, 0xffffff)},${n(t.saturated, 0xffffff)},${
            n(
                t.chambered,
                0xffffff,
            )
        },${n(t.apSaturated, 0xffffff)},${f(t.alphaRate, 0)},${f(t.alphaMin, 0)}},\n`;
    }
    gameConfigArrays += `};\n`;

    const bulletNames = Object.keys(BulletDefs || {});
    tracerNamesCount = tracerNames.length;
    bulletNamesCount = bulletNames.length;
    const bulletArray = "kBulletRender";
    gameConfigArrays += `static const char* const ${bulletArray}_names[] = {${
        bulletNames
            .map(str)
            .join(",")
    }};\n`;
    gameConfigArrays += `static const RawBulletRender ${bulletArray}[] = {\n`;
    for (const name of bulletNames) {
        const b = BulletDefs[name] || {};
        let flareColor = 0xffffff;
        if (typeof b.flareColor === "string") {
            const parsed = Number.parseInt(b.flareColor.replace(/^#/, ""), 16);
            flareColor = Number.isFinite(parsed) ? parsed : 0xffffff;
        } else if (Number.isFinite(Number(b.flareColor))) {
            flareColor = Number(b.flareColor);
        }
        gameConfigArrays += `    {${f(b.distance, 0)},${f(b.speed, 0)},${
            f(
                b.variance,
                0,
            )
        },${str(b.tracerColor || "")},${f(b.tracerWidth, 1)},${f(b.tracerLength, 0)},${
            n(
                flareColor,
                0xffffff,
            )
        },${f(b.maxFlareScale, 1)},${b.suppressed ? 1 : 0}},\n`;
    }
    gameConfigArrays += `};\n`;

    const effectNames = Object.keys(ExplosionEffectDefs || {});
    explosionEffectCount = effectNames.length;
    gameConfigArrays += `static const char* const kExplosionEffect_names[] = {${effectNames.map(str).join(",")}};\n`;
    gameConfigArrays += `static const RawExplosionEffect kExplosionEffects[] = {\n`;
    for (const name of effectNames) {
        const e = ExplosionEffectDefs[name] || {};
        const burst = e.burst || {};
        const sound = burst.sound || {};
        const scatter = e.scatter || null;
        gameConfigArrays += `    {${str(name)},${str(burst.particle || "")},${f(burst.scale, 1)},${
            str(sound.grass || "")
        },${str(sound.water || "")},${f(sound.detune, 0)},${f(sound.volume, 1)},${str(scatter?.particle || "")},${
            n(scatter?.count, 0)
        },${f(scatter?.speed?.min, 0)},${f(scatter?.speed?.max, 0)},${n(e.rippleCount, 0)},${f(e.shakeStr, 0)},${
            f(e.shakeDur, 0)
        },${f(e.lifetime, 1)}},\n`;
    }
    gameConfigArrays += `};\n`;
}
const tracerCount = tracerNamesCount;
const bulletCount = bulletNamesCount;
const out = `// GENERATED FILE - do not edit. Run tools/codegen_render_defs.mjs.
#include "Defs.h"
#include <string>
#include <unordered_map>

namespace surv {
namespace {

struct RawCollider { int type; float a, b, c, d; };
struct RawImg { const char* sprite; float scale; float alpha; unsigned tint; int zIdx; int mirrorX; int mirrorY; float ori; };
struct RawBImg { const char* sprite; float scale; float alpha; unsigned tint; float px, py, rot; int mirrorX, mirrorY, removeOnDamaged; };
struct RawBEmitter { const char* type; float px, py, rot, scale; int layer, parentToCeiling; float dirx, diry; };
struct RawLayer { const char* type; float x, y; int ori; int inheritOri; int hasUnderground; int underground; };
struct RawStair { RawCollider col; float dx, dy; int noCeilingReveal; int lootOnly; };
struct RawShape { RawCollider col; float scale; unsigned color; };
struct RawBSurface {
    const char* type; const RawCollider* colliders; int count;
    int hasWaterColor; unsigned waterColor; int hasRippleColor; unsigned rippleColor;
};
struct RawSoundEmitter {
    const char* sound; const char* channel; float x, y, rangeMin, rangeMax, falloff, volume;
};
struct RawMapObj {
    const char* type; RawImg img; int hasImg; RawCollider collision; int hasCollision;
    int isDoor, isButton, isTree, isBush, isWall; int randomRotation, hasExplosion;
    const char* explosionParticle; const char* explodeParticle; const char* explodeParticleAlt;
    float doorSlideOffset, doorOpenSpeed;
    const char* doorCasingSprite; float doorCasingPx, doorCasingPy, doorCasingScale; unsigned doorCasingTint; float doorCasingAlpha;
    float doorSpriteAnchorX, doorSpriteAnchorY;
    float height; int collidable, isWindow;
    const char* hitParticle; const char* punchSound;
    const char* doorSoundChange; const char* doorSoundOpen; const char* doorSoundClose; const char* doorSoundUnlock;
    const char* buttonSoundOn; const char* buttonSoundOff; const char* explodeSound;
    const char* bulletSound;
    const char* soundEnter; const char* buttonUseImg; const char* buttonOffImg; const char* buttonUseParticle;
    int hasSurface, surfaceType; unsigned surfaceWaterColor, surfaceRippleColor;
    int hasGore; float goreFadeStart, goreFadeEnd, goreFadePow, goreFadeSpeed;
    int goreHasTint; unsigned goreTint; int goreHasAlpha; float goreAlpha;
    int goreHasWaterColor; unsigned goreWaterColor; int goreHasRippleColor; unsigned goreRippleColor;
    int imgFlicker; float imgFlickerMin, imgFlickerMax, imgFlickerRate; int imgIgnoreAdjust;
    int hasLifetime; float lifetimeMin, lifetimeMax;
    int mapDisplay, mapHasColor; unsigned mapColor; float mapScale;
    RawCollider bounding; int hasBounding;
    const RawLayer* layers; int layerCount;
    const RawStair* stairs; int stairCount;
    const RawCollider* mask; int maskCount;
    const RawShape* shapes; int shapeCount;
    int zIdx;
    const RawBImg* floorImgs; int floorCount;
    const RawBImg* ceilingImgs; int ceilingCount;
    float visionDist, visionWidth, visionLinger, visionFadeRate;
    const char* destroyResidue;
    const RawBEmitter* emitters; int emitterCount;
    const RawCollider* zoomIns; int zoomCount;
    const RawBSurface* surfaces; int surfaceCount;
    int hasInteriorSound; const char* interiorSound; const char* interiorSoundAlt; const char* interiorFilter;
    float interiorTransitionTime, interiorOutsideMaxDist, interiorOutsideVolume;
    int hasInteriorUndergroundVolume; float interiorUndergroundVolume;
    const RawSoundEmitter* soundEmitters; int soundEmitterCount;
    const char* puzzleSoundFail; const char* puzzleSoundComplete; const char* ceilingDestroySound;
    const char* ceilingDestroyParticle; int ceilingDestroyParticleCount;
};
struct RawMapRender {
    const char* name;
    unsigned background, beach, grass, riverbank, water, waterRipple, lakeRiverbank, lakeWater, lakeWaterRipple, underground;
    int faction, potato, perk, turkey; float valueAdjust; const char* cameraEmitter;
    const char* const* atlases; int atlasCount;
    const char* ambMusic; const char* ambWind; const char* ambRiver; const char* ambWaves;
    const char* airdropPlaneImg; const char* airdropPlaneSound; const char* airdropChuteImg;
    int riverShoreSurface;
    const char* const* perkModeRoles; int perkModeRoleCount;
};
struct RawGameObj { const char* type; const char* category; RawImg img; int hasImg; const char* emitter; const char* auraSprite; unsigned auraTint; int hasAura;
    int hasAttack; float attackOffX, attackOffY, attackRad; int cleave;
    const char* playerHit; const char* punchSound; int useThrowParticles;
    const char* bulletType; float barrelLength;
    const char* mapIndicatorSprite; unsigned mapIndicatorTint; int mapIndicatorPulse; unsigned mapIndicatorPulseTint;
    const char* soundReload; const char* soundReloadAlt; const char* soundDeploy; const char* soundUse; const char* soundPickup;
    const char* lootBorder; unsigned lootBorderTint; float lootInnerScale, lootRot; int lootMirror;
    const char* explosionEffectType;
    int hasTrail; float trailMaxLength, trailWidth, trailAlpha; unsigned trailTint;
    float throwSpeed, spinVel, spinDrag; int randomizeSpinDir, alwaysRenderOntop; float projectileRad;
    const char* casing; int casingOnReload; int maxReload;
    float shellScale, shellOffset, shellOffsetY, shellForward; int shellReverse;
    const char* soundShoot; const char* soundShootLast; const char* soundShootAlt;
    const char* soundShootTeamRed; const char* soundShootTeamBlue;
    const char* cycleSound; const char* pullSound;
    float soundFallOff; int ignoreDetune; int fireModeSingle;
    float pullDelay; int hasPullDelay; int explodeOnImpact;
    const char* soundEmpty; int ammoInfinite; };
struct RawRange { float min; float max; int isConstant; };
struct RawParticle {
    const char* name; const char* const* images; int imageCount; int zOrd;
    RawRange life, drag, rotVel, scaleStart, scaleEnd, scaleLerp;
    int scaleUseExp; float scaleExp;
    float alphaStart, alphaEnd; RawRange alphaLerp; int alphaUseExp; float alphaExp;
    int hasAlphaIn; float alphaInStart, alphaInEnd; RawRange alphaInLerp;
    int hasColor; unsigned color; int ignoreValueAdjust;
    int colorVariance; float colorHMin, colorHMax, colorSMin, colorSMax, colorVMin, colorVMax;
};
struct RawEmitter {
    const char* name; const char* particle; RawRange rate; float radius; RawRange speed; float angle;
    int hasRot; RawRange rot; float maxCount; int hasMaxRate; RawRange maxRate; float maxElapsed;
    int hasZOrd; int zOrd;
};
struct RawPlane { float actionOffset; float planeVel, planeRad, soundRangeMult, soundRangeDelta, soundRangeMax, fallOff; };
struct RawTracer { unsigned regular, saturated, chambered, apSaturated; float alphaRate, alphaMin; };
struct RawBulletRender { float distance, speed, variance; const char* tracerColor; float tracerWidth, tracerLength; unsigned flareColor; float maxFlareScale; int suppressed; };
struct RawExplosionEffect {
    const char* name; const char* burstParticle; float burstScale;
    const char* grassSound; const char* waterSound; float detune, volume;
    const char* scatterParticle; int scatterCount; float scatterSpeedMin, scatterSpeedMax;
    int rippleCount; float shakeStr, shakeDur, lifetime;
};
struct RawEmote { const char* type; const char* texture; const char* sound; const char* channel; const char* name; int teamOnly; int category; };
struct RawLoadoutItem {
    const char* type; const char* category; const char* name; const char* lore; const char* sprite;
    const char* subcat; int rarity; unsigned tint;
};
struct RawLoadoutSource { const char* type; const char* name; };
struct RawPing {
    const char* type; const char* texture; const char* mapTexture; const char* sound; const char* soundLeader;
    int pingMap; float pingLife, mapLife; int worldDisplay; int mapEvent; unsigned tint;
};
struct RawRole {
    const char* type; const char* mapIconAlive; const char* mapIconDead;
    const char* soundAssign; const char* soundDead;
    const char* name; const char* nameRed; const char* nameBlue;
    int killFeedAssign, killFeedDead, hasKillFeedColor; unsigned killFeedColor;
    const char* guiImg; int hasColor; unsigned color;
    const char* const* perks; int perkCount; const char* const* perkNames;
};

static PlaneDef toPlane(const RawPlane& r) {
    PlaneDef d;
    d.planeVel = r.planeVel;
    d.planeRad = r.planeRad;
    d.soundRangeMult = r.soundRangeMult;
    d.soundRangeDelta = r.soundRangeDelta;
    d.soundRangeMax = r.soundRangeMax;
    d.fallOff = r.fallOff;
    return d;
}
static TracerColorDef toTracer(const RawTracer& r) {
    TracerColorDef d;
    d.regular = r.regular;
    d.saturated = r.saturated;
    d.chambered = r.chambered;
    d.apSaturated = r.apSaturated;
    d.alphaRate = r.alphaRate;
    d.alphaMin = r.alphaMin;
    return d;
}
static BulletRenderDef toBulletRender(const RawBulletRender& r) {
    BulletRenderDef d;
    d.distance = r.distance;
    d.speed = r.speed;
    d.variance = r.variance;
    d.tracerColor = r.tracerColor ? r.tracerColor : "";
    d.tracerWidth = r.tracerWidth;
    d.tracerLength = r.tracerLength;
    d.flareColor = r.flareColor;
    d.maxFlareScale = r.maxFlareScale;
    d.suppressed = r.suppressed != 0;
    return d;
}
static ExplosionEffectRenderDef toExplosionEffect(const RawExplosionEffect& r) {
    ExplosionEffectRenderDef d;
    d.burstParticle = r.burstParticle ? r.burstParticle : "";
    d.burstScale = r.burstScale;
    d.grassSound = r.grassSound ? r.grassSound : "";
    d.waterSound = r.waterSound ? r.waterSound : "";
    d.detune = r.detune;
    d.volume = r.volume;
    d.scatterParticle = r.scatterParticle ? r.scatterParticle : "";
    d.scatterCount = r.scatterCount;
    d.scatterSpeedMin = r.scatterSpeedMin;
    d.scatterSpeedMax = r.scatterSpeedMax;
    d.rippleCount = r.rippleCount;
    d.shakeStr = r.shakeStr;
    d.shakeDur = r.shakeDur;
    d.lifetime = r.lifetime;
    return d;
}

static Collider toCollider(const RawCollider& c) {
    if (c.type == 0) return Collider::createCircle(Vec2(c.a, c.b), c.c);
    return Collider::createAabb(Vec2(c.a, c.b), Vec2(c.c, c.d));
}
static ImgDef toImg(const RawImg& i) {
    ImgDef d;
    d.sprite = i.sprite ? i.sprite : "";
    d.scale = i.scale;
    d.alpha = i.alpha;
    d.tint = i.tint;
    d.zIdx = i.zIdx;
    d.ori = i.ori;
    d.mirrorX = i.mirrorX != 0;
    d.mirrorY = i.mirrorY != 0;
    return d;
}
static BuildingImageDef toBImg(const RawBImg& i) {
    BuildingImageDef d;
    d.sprite = i.sprite ? i.sprite : "";
    d.scale = i.scale;
    d.alpha = i.alpha;
    d.tint = i.tint;
    d.pos = Vec2(i.px, i.py);
    d.rot = i.rot;
    d.mirrorX = i.mirrorX != 0;
    d.mirrorY = i.mirrorY != 0;
    d.removeOnDamaged = i.removeOnDamaged != 0;
    return d;
}
static RangeDef toRange(const RawRange& r) {
    RangeDef d;
    d.min = r.min;
    d.max = r.max;
    d.isConstant = r.isConstant != 0;
    return d;
}

${arrays}${particleArrays}${gameConfigArrays}
static const RawMapObj kMapObjs[] = {
${entries}};
${mapAtlasArrays}static const RawMapRender kMapRenders[] = {
${mapRenderEntries}};
static const RawEmote kEmotes[] = {
${emoteEntries}};
static const RawPing kPings[] = {
${pingEntries}};
${roleArrays}static const RawRole kRoles[] = {
${roleEntries}};
static const RawGameObj kGameObjs[] = {
${gameEntries}};
static const RawLoadoutItem kLoadoutItems[] = {
${loadoutEntries}};
static const RawLoadoutSource kLoadoutSources[] = {
${loadoutSourceEntries}};
static const RawParticle kParticles[] = {
${particleEntries}};
static const RawEmitter kEmitters[] = {
${emitterEntries}};

class GeneratedProvider : public DefProvider {
public:
    GeneratedProvider() {
        for (const auto& r : kMapObjs) {
            MapObjectDef d;
            d.type = r.type;
            d.img = toImg(r.img);
            d.collision = toCollider(r.collision);
            d.hasCollision = r.hasCollision != 0;
            d.isDoor = r.isDoor != 0;
            d.isButton = r.isButton != 0;
            d.isTree = r.isTree != 0;
            d.isBush = r.isBush != 0;
            d.isWall = r.isWall != 0;
            d.randomRotation = r.randomRotation != 0;
            d.hasExplosion = r.hasExplosion != 0;
            d.explosionParticle = r.explosionParticle ? r.explosionParticle : "";
            d.explodeParticle = r.explodeParticle ? r.explodeParticle : "";
            d.explodeParticleAlt = r.explodeParticleAlt ? r.explodeParticleAlt : "";
            d.doorSlideOffset = r.doorSlideOffset;
            d.doorOpenSpeed = r.doorOpenSpeed;
            d.doorCasingSprite = r.doorCasingSprite ? r.doorCasingSprite : "";
            d.doorCasingPos = Vec2(r.doorCasingPx, r.doorCasingPy);
            d.doorCasingScale = r.doorCasingScale;
            d.doorCasingTint = r.doorCasingTint;
            d.doorCasingAlpha = r.doorCasingAlpha;
            d.doorSpriteAnchor = Vec2(r.doorSpriteAnchorX, r.doorSpriteAnchorY);
            d.height = r.height;
            d.collidable = r.collidable != 0;
            d.isWindow = r.isWindow != 0;
            d.hitParticle = r.hitParticle ? r.hitParticle : "";
            d.punchSound = r.punchSound ? r.punchSound : "";
            d.doorSoundChange = r.doorSoundChange ? r.doorSoundChange : "";
            d.doorSoundOpen = r.doorSoundOpen ? r.doorSoundOpen : "";
            d.doorSoundClose = r.doorSoundClose ? r.doorSoundClose : "";
            d.doorSoundUnlock = r.doorSoundUnlock ? r.doorSoundUnlock : "";
            d.buttonSoundOn = r.buttonSoundOn ? r.buttonSoundOn : "";
            d.buttonSoundOff = r.buttonSoundOff ? r.buttonSoundOff : "";
            d.explodeSound = r.explodeSound ? r.explodeSound : "";
            d.bulletSound = r.bulletSound ? r.bulletSound : "";
            d.soundEnter = r.soundEnter ? r.soundEnter : "";
            d.buttonUseImg = r.buttonUseImg ? r.buttonUseImg : "";
            d.buttonOffImg = r.buttonOffImg ? r.buttonOffImg : "";
            d.buttonUseParticle = r.buttonUseParticle ? r.buttonUseParticle : "";
            d.hasSurface = r.hasSurface != 0;
            d.surfaceType = r.surfaceType == 2
                ? SurfaceTypeDef::Grass
                : (r.surfaceType == 1 ? SurfaceTypeDef::Sand : SurfaceTypeDef::Water);
            d.surfaceWaterColor = r.surfaceWaterColor;
            d.surfaceRippleColor = r.surfaceRippleColor;
            d.hasGore = r.hasGore != 0;
            d.goreFadeStart = r.goreFadeStart;
            d.goreFadeEnd = r.goreFadeEnd;
            d.goreFadePow = r.goreFadePow;
            d.goreFadeSpeed = r.goreFadeSpeed;
            d.goreHasTint = r.goreHasTint != 0;
            d.goreTint = r.goreTint;
            d.goreHasAlpha = r.goreHasAlpha != 0;
            d.goreAlpha = r.goreAlpha;
            d.goreHasWaterColor = r.goreHasWaterColor != 0;
            d.goreWaterColor = r.goreWaterColor;
            d.goreHasRippleColor = r.goreHasRippleColor != 0;
            d.goreRippleColor = r.goreRippleColor;
            d.imgFlicker = r.imgFlicker != 0;
            d.imgFlickerMin = r.imgFlickerMin;
            d.imgFlickerMax = r.imgFlickerMax;
            d.imgFlickerRate = r.imgFlickerRate;
            d.imgIgnoreAdjust = r.imgIgnoreAdjust != 0;
            d.hasLifetime = r.hasLifetime != 0;
            d.lifetimeMin = r.lifetimeMin;
            d.lifetimeMax = r.lifetimeMax;
            d.map.display = r.mapDisplay != 0;
            d.map.hasColor = r.mapHasColor != 0;
            d.map.color = r.mapColor;
            d.map.scale = r.mapScale;
            d.boundingCollider = toCollider(r.bounding);
            d.hasBounding = r.hasBounding != 0;
            for (int i = 0; i < r.layerCount; i++) {
                StructureLayerDef l;
                l.type = r.layers[i].type;
                l.pos = Vec2(r.layers[i].x, r.layers[i].y);
                l.ori = r.layers[i].ori;
                l.inheritOri = r.layers[i].inheritOri != 0;
                l.hasUnderground = r.layers[i].hasUnderground != 0;
                l.underground = r.layers[i].underground != 0;
                d.layers.push_back(l);
            }
            for (int i = 0; i < r.stairCount; i++) {
                StairDef s;
                s.collision = toCollider(r.stairs[i].col);
                s.downDir = Vec2(r.stairs[i].dx, r.stairs[i].dy);
                s.noCeilingReveal = r.stairs[i].noCeilingReveal != 0;
                s.lootOnly = r.stairs[i].lootOnly != 0;
                d.stairs.push_back(s);
            }
            for (int i = 0; i < r.maskCount; i++) d.mask.push_back(toCollider(r.mask[i]));
            for (int i = 0; i < r.shapeCount; i++) {
                MapShapeDef sh;
                sh.collider = toCollider(r.shapes[i].col);
                sh.scale = r.shapes[i].scale;
                sh.color = r.shapes[i].color;
                d.mapShapes.push_back(sh);
            }
            d.zIdx = r.zIdx;
            for (int i = 0; i < r.floorCount; i++) d.floorImgs.push_back(toBImg(r.floorImgs[i]));
            for (int i = 0; i < r.ceilingCount; i++) d.ceilingImgs.push_back(toBImg(r.ceilingImgs[i]));
            d.ceilingVision.dist = r.visionDist;
            d.ceilingVision.width = r.visionWidth;
            d.ceilingVision.linger = r.visionLinger;
            d.ceilingVision.fadeRate = r.visionFadeRate;
            d.ceilingDestroyResidue = r.destroyResidue ? r.destroyResidue : "";
            for (int i = 0; i < r.emitterCount; i++) {
                BuildingEmitterDef e;
                e.type = r.emitters[i].type;
                e.pos = Vec2(r.emitters[i].px, r.emitters[i].py);
                e.rot = r.emitters[i].rot;
                e.scale = r.emitters[i].scale;
                e.layer = r.emitters[i].layer;
                e.parentToCeiling = r.emitters[i].parentToCeiling != 0;
                e.dir = Vec2(r.emitters[i].dirx, r.emitters[i].diry);
                d.occupiedEmitters.push_back(e);
            }
            for (int i = 0; i < r.zoomCount; i++) d.ceilingZoomIn.push_back(toCollider(r.zoomIns[i]));
            for (int i = 0; i < r.surfaceCount; i++) {
                BuildingSurfaceDef s;
                s.type = r.surfaces[i].type ? r.surfaces[i].type : "";
                s.hasWaterColor = r.surfaces[i].hasWaterColor != 0;
                s.waterColor = r.surfaces[i].waterColor;
                s.hasRippleColor = r.surfaces[i].hasRippleColor != 0;
                s.rippleColor = r.surfaces[i].rippleColor;
                for (int j = 0; j < r.surfaces[i].count; j++) {
                    s.colliders.push_back(toCollider(r.surfaces[i].colliders[j]));
                }
                d.surfaces.push_back(std::move(s));
            }
            d.hasInteriorSound = r.hasInteriorSound != 0;
            d.interiorSound.sound = r.interiorSound ? r.interiorSound : "";
            d.interiorSound.soundAlt = r.interiorSoundAlt ? r.interiorSoundAlt : "";
            d.interiorSound.filter = r.interiorFilter ? r.interiorFilter : "";
            d.interiorSound.transitionTime = r.interiorTransitionTime;
            d.interiorSound.outsideMaxDist = r.interiorOutsideMaxDist;
            d.interiorSound.outsideVolume = r.interiorOutsideVolume;
            d.interiorSound.hasUndergroundVolume = r.hasInteriorUndergroundVolume != 0;
            d.interiorSound.undergroundVolume = r.interiorUndergroundVolume;
            d.puzzleSoundFail = r.puzzleSoundFail ? r.puzzleSoundFail : "";
            d.puzzleSoundComplete = r.puzzleSoundComplete ? r.puzzleSoundComplete : "";
            d.ceilingDestroySound = r.ceilingDestroySound ? r.ceilingDestroySound : "";
            d.ceilingDestroyParticle = r.ceilingDestroyParticle ? r.ceilingDestroyParticle : "";
            d.ceilingDestroyParticleCount = r.ceilingDestroyParticleCount;
            for (int i = 0; i < r.soundEmitterCount; i++) {
                BuildingSoundEmitterDef e;
                e.sound = r.soundEmitters[i].sound ? r.soundEmitters[i].sound : "";
                e.channel = r.soundEmitters[i].channel ? r.soundEmitters[i].channel : "";
                e.pos = Vec2(r.soundEmitters[i].x, r.soundEmitters[i].y);
                e.rangeMin = r.soundEmitters[i].rangeMin;
                e.rangeMax = r.soundEmitters[i].rangeMax;
                e.falloff = r.soundEmitters[i].falloff;
                e.volume = r.soundEmitters[i].volume;
                d.soundEmitters.push_back(e);
            }
            _mapObjs[d.type] = std::move(d);
        }
        for (const auto& r : kMapRenders) {
            MapRenderDef d;
            d.colors.background = r.background;
            d.colors.beach = r.beach;
            d.colors.grass = r.grass;
            d.colors.riverbank = r.riverbank;
            d.colors.water = r.water;
            d.colors.waterRipple = r.waterRipple;
            d.colors.lakeRiverbank = r.lakeRiverbank;
            d.colors.lakeWater = r.lakeWater;
            d.colors.lakeWaterRipple = r.lakeWaterRipple;
            d.colors.underground = r.underground;
            d.factionMode = r.faction != 0;
            d.potatoMode = r.potato != 0;
            d.perkMode = r.perk != 0;
            d.turkeyMode = r.turkey != 0;
            d.valueAdjust = r.valueAdjust;
            d.cameraEmitter = r.cameraEmitter ? r.cameraEmitter : "";
            for (int i = 0; i < r.atlasCount; i++) {
                d.atlases.push_back(r.atlases[i] ? r.atlases[i] : "");
            }
            d.ambienceMusic = r.ambMusic ? r.ambMusic : "";
            d.ambienceWind = r.ambWind ? r.ambWind : "";
            d.ambienceRiver = r.ambRiver ? r.ambRiver : "";
            d.ambienceWaves = r.ambWaves ? r.ambWaves : "";
            d.airdropPlaneImg = r.airdropPlaneImg ? r.airdropPlaneImg : "map-plane-01.img";
            d.airdropPlaneSound = r.airdropPlaneSound ? r.airdropPlaneSound : "plane_01";
            d.airdropChuteImg = r.airdropChuteImg ? r.airdropChuteImg : "map-chute-01.img";
            d.riverShoreSurface = static_cast<SurfaceTypeDef>(r.riverShoreSurface);
            for (int i = 0; i < r.perkModeRoleCount; i++) {
                d.perkModeRoles.push_back(r.perkModeRoles[i] ? r.perkModeRoles[i] : "");
            }
            _mapRenders[r.name] = d;
        }
        for (const auto& r : kGameObjs) {
            GameObjRenderDef d;
            d.type = r.type;
            d.category = r.category;
            d.img = toImg(r.img);
            d.hasImg = r.hasImg != 0;
            d.emitter = r.emitter ? r.emitter : "";
            d.auraSprite = r.auraSprite ? r.auraSprite : "";
            d.auraTint = r.auraTint;
            d.hasAura = r.hasAura != 0;
            d.hasAttack = r.hasAttack != 0;
            d.attackOffset = Vec2(r.attackOffX, r.attackOffY);
            d.attackRad = r.attackRad;
            d.cleave = r.cleave != 0;
            d.playerHit = r.playerHit ? r.playerHit : "";
            d.punchSound = r.punchSound ? r.punchSound : "";
            d.useThrowParticles = r.useThrowParticles != 0;
            d.bulletType = r.bulletType ? r.bulletType : "";
            d.barrelLength = r.barrelLength;
            d.mapIndicatorSprite = r.mapIndicatorSprite ? r.mapIndicatorSprite : "";
            d.mapIndicatorTint = r.mapIndicatorTint;
            d.mapIndicatorPulse = r.mapIndicatorPulse != 0;
            d.mapIndicatorPulseTint = r.mapIndicatorPulseTint;
            d.soundReload = r.soundReload ? r.soundReload : "";
            d.soundReloadAlt = r.soundReloadAlt ? r.soundReloadAlt : "";
            d.soundDeploy = r.soundDeploy ? r.soundDeploy : "";
            d.soundUse = r.soundUse ? r.soundUse : "";
            d.soundPickup = r.soundPickup ? r.soundPickup : "";
            d.lootBorder = r.lootBorder ? r.lootBorder : "";
            d.lootBorderTint = r.lootBorderTint;
            d.lootInnerScale = r.lootInnerScale;
            d.lootRot = r.lootRot;
            d.lootMirror = r.lootMirror != 0;
            d.explosionEffectType = r.explosionEffectType ? r.explosionEffectType : "";
            d.hasTrail = r.hasTrail != 0;
            d.trailMaxLength = r.trailMaxLength;
            d.trailWidth = r.trailWidth;
            d.trailAlpha = r.trailAlpha;
            d.trailTint = r.trailTint;
            d.throwSpeed = r.throwSpeed;
            d.spinVel = r.spinVel;
            d.spinDrag = r.spinDrag;
            d.randomizeSpinDir = r.randomizeSpinDir != 0;
            d.alwaysRenderOntop = r.alwaysRenderOntop != 0;
            d.projectileRad = r.projectileRad;
            d.casing = r.casing ? r.casing : "";
            d.casingOnReload = r.casingOnReload != 0;
            d.maxReload = r.maxReload;
            d.shellScale = r.shellScale;
            d.shellOffset = r.shellOffset;
            d.shellOffsetY = r.shellOffsetY;
            d.shellForward = r.shellForward;
            d.shellReverse = r.shellReverse != 0;
            d.soundShoot = r.soundShoot ? r.soundShoot : "";
            d.soundShootLast = r.soundShootLast ? r.soundShootLast : "";
            d.soundShootAlt = r.soundShootAlt ? r.soundShootAlt : "";
            d.soundShootTeamRed = r.soundShootTeamRed ? r.soundShootTeamRed : "";
            d.soundShootTeamBlue = r.soundShootTeamBlue ? r.soundShootTeamBlue : "";
            d.cycleSound = r.cycleSound ? r.cycleSound : "";
            d.pullSound = r.pullSound ? r.pullSound : "";
            d.soundFallOff = r.soundFallOff;
            d.ignoreDetune = r.ignoreDetune != 0;
            d.fireModeSingle = r.fireModeSingle != 0;
            d.pullDelay = r.pullDelay;
            d.hasPullDelay = r.hasPullDelay != 0;
            d.explodeOnImpact = r.explodeOnImpact != 0;
            d.soundEmpty = r.soundEmpty ? r.soundEmpty : "";
            d.ammoInfinite = r.ammoInfinite != 0;
            _gameObjs[d.type] = d;
        }
${playerEntries}${poseEntries}
${
    Object.entries(MapDefs).map(([name, d]) =>
        `        _mapRenders.at(${str(name)}).colors.playerGhillie=${n(d.biome?.colors?.playerGhillie, 0x5a9e4c)};`
    ).join("\n")
}
        for (const auto& r : kParticles) {
            ParticleDef d;
            d.images.reserve(static_cast<size_t>(r.imageCount));
            for (int i = 0; i < r.imageCount; i++) d.images.push_back(r.images[i]);
            d.zOrd = r.zOrd;
            d.life = toRange(r.life);
            d.drag = toRange(r.drag);
            d.rotVel = toRange(r.rotVel);
            d.scaleStart = toRange(r.scaleStart);
            d.scaleEnd = toRange(r.scaleEnd);
            d.scaleLerp = toRange(r.scaleLerp);
            d.scaleUseExp = r.scaleUseExp != 0;
            d.scaleExp = r.scaleExp;
            d.alphaStart = r.alphaStart;
            d.alphaEnd = r.alphaEnd;
            d.alphaLerp = toRange(r.alphaLerp);
            d.alphaUseExp = r.alphaUseExp != 0;
            d.alphaExp = r.alphaExp;
            d.hasAlphaIn = r.hasAlphaIn != 0;
            d.alphaInStart = r.alphaInStart;
            d.alphaInEnd = r.alphaInEnd;
            d.alphaInLerp = toRange(r.alphaInLerp);
            d.hasColor = r.hasColor != 0;
            d.color = r.color;
            d.ignoreValueAdjust = r.ignoreValueAdjust != 0;
            d.colorVariance = r.colorVariance != 0;
            d.colorHueMin = r.colorHMin;
            d.colorHueMax = r.colorHMax;
            d.colorSatMin = r.colorSMin;
            d.colorSatMax = r.colorSMax;
            d.colorValMin = r.colorVMin;
            d.colorValMax = r.colorVMax;
            _particles[r.name] = std::move(d);
        }
        for (const auto& r : kEmitters) {
            EmitterDef d;
            d.particle = r.particle;
            d.rate = toRange(r.rate);
            d.radius = r.radius;
            d.speed = toRange(r.speed);
            d.angle = r.angle;
            d.hasRot = r.hasRot != 0;
            d.rot = toRange(r.rot);
            d.maxCount = r.maxCount;
            d.hasMaxRate = r.hasMaxRate != 0;
            d.maxRate = toRange(r.maxRate);
            d.maxElapsed = r.maxElapsed;
            d.hasZOrd = r.hasZOrd != 0;
            d.zOrd = r.zOrd;
            _emitters[r.name] = std::move(d);
        }
        for (const auto& r : kEmotes) {
            EmoteRenderDef d;
            d.texture = r.texture ? r.texture : "";
            d.sound = r.sound ? r.sound : "";
            d.channel = r.channel ? r.channel : "";
            d.name = r.name ? r.name : "";
            d.teamOnly = r.teamOnly != 0;
            d.category = r.category;
            _emotes[r.type] = std::move(d);
        }
        for (const auto& r : kPings) {
            PingRenderDef d;
            d.texture = r.texture ? r.texture : "";
            d.mapTexture = r.mapTexture ? r.mapTexture : "";
            d.sound = r.sound ? r.sound : "";
            d.soundLeader = r.soundLeader ? r.soundLeader : "";
            d.pingMap = r.pingMap != 0;
            d.pingLife = r.pingLife;
            d.mapLife = r.mapLife;
            d.worldDisplay = r.worldDisplay != 0;
            d.mapEvent = r.mapEvent != 0;
            d.tint = r.tint;
            _pings[r.type] = std::move(d);
        }
        for (const auto& r : kRoles) {
            RoleRenderDef d;
            d.mapIconAlive = r.mapIconAlive ? r.mapIconAlive : "";
            d.mapIconDead = r.mapIconDead ? r.mapIconDead : "";
            d.soundAssign = r.soundAssign ? r.soundAssign : "";
            d.soundDead = r.soundDead ? r.soundDead : "";
            d.name = r.name ? r.name : "";
            d.nameRed = r.nameRed ? r.nameRed : "";
            d.nameBlue = r.nameBlue ? r.nameBlue : "";
            d.killFeedAssign = r.killFeedAssign != 0;
            d.killFeedDead = r.killFeedDead != 0;
            d.hasKillFeedColor = r.hasKillFeedColor != 0;
            d.killFeedColor = r.killFeedColor;
            d.guiImg = r.guiImg ? r.guiImg : "";
            d.hasColor = r.hasColor != 0;
            d.color = r.color;
            for (int i = 0; i < r.perkCount; i++) {
                d.perks.push_back(r.perks[i] ? r.perks[i] : "");
                d.perkNames.push_back(r.perkNames[i] ? r.perkNames[i] : "");
            }
            _roles[r.type] = std::move(d);
        }
        _loadoutItems.reserve(sizeof(kLoadoutItems) / sizeof(kLoadoutItems[0]));
        for (const auto& r : kLoadoutItems) {
            LoadoutItemDef d;
            d.type = r.type ? r.type : "";
            d.category = r.category ? r.category : "";
            d.name = r.name ? r.name : "";
            d.lore = r.lore ? r.lore : "";
            d.sprite = r.sprite ? r.sprite : "";
            d.subcat = r.subcat ? r.subcat : "";
            d.rarity = r.rarity;
            d.tint = r.tint;
            _loadoutItems.push_back(std::move(d));
        }
        _loadoutSources.reserve(sizeof(kLoadoutSources) / sizeof(kLoadoutSources[0]));
        for (const auto& r : kLoadoutSources) {
            LoadoutSourceDef d;
            d.type = r.type ? r.type : "";
            d.name = r.name ? r.name : "";
            _loadoutSources.push_back(std::move(d));
        }
        _planes.reserve(2);
        for (const auto& r : kPlanes) _planes.push_back(toPlane(r));
        for (int i = 0; i < ${tracerCount}; ++i) {
            _tracers[kTracerColors_names[i]] = toTracer(kTracerColors[i]);
        }
        for (int i = 0; i < ${bulletCount}; ++i) {
            _bullets[kBulletRender_names[i]] = toBulletRender(kBulletRender[i]);
        }
        for (int i = 0; i < ${explosionEffectCount}; ++i) {
            _explosionEffects[kExplosionEffect_names[i]] = toExplosionEffect(kExplosionEffects[i]);
        }
    }

    const MapObjectDef* mapObject(const std::string& type) const override {
        auto it = _mapObjs.find(type);
        return it == _mapObjs.end() ? nullptr : &it->second;
    }
    const MapRenderDef* mapRender(const std::string& name) const override {
        auto it = _mapRenders.find(name);
        return it == _mapRenders.end() ? nullptr : &it->second;
    }
    const GameObjRenderDef* gameObject(const std::string& type) const override {
        auto it = _gameObjs.find(type);
        return it == _gameObjs.end() ? nullptr : &it->second;
    }
    const ParticleDef* particle(const std::string& type) const override {
        auto it = _particles.find(type);
        return it == _particles.end() ? nullptr : &it->second;
    }
    const EmitterDef* emitter(const std::string& type) const override {
        auto it = _emitters.find(type);
        return it == _emitters.end() ? nullptr : &it->second;
    }
    const PlayerPose* playerPose(const std::string& type) const override {
        auto it = _poses.find(type);
        return it == _poses.end() ? nullptr : &it->second;
    }
    const PlayerAnimationDef* playerAnimation(const std::string& type) const override {
        auto it = _animations.find(type);
        return it == _animations.end() ? nullptr : &it->second;
    }
    const PlaneDef* plane(int type) const override {
        if (type < 0 || type >= static_cast<int>(_planes.size())) return nullptr;
        return &_planes[static_cast<size_t>(type)];
    }
    const TracerColorDef* tracerColor(const std::string& name) const override {
        auto it = _tracers.find(name);
        return it == _tracers.end() ? nullptr : &it->second;
    }
    const BulletRenderDef* bullet(const std::string& type) const override {
        auto it = _bullets.find(type);
        return it == _bullets.end() ? nullptr : &it->second;
    }
    const ExplosionEffectRenderDef* explosionEffect(const std::string& type) const override {
        auto it = _explosionEffects.find(type);
        return it == _explosionEffects.end() ? nullptr : &it->second;
    }
    const EmoteRenderDef* emote(const std::string& type) const override {
        auto it = _emotes.find(type);
        return it == _emotes.end() ? nullptr : &it->second;
    }
    const PingRenderDef* ping(const std::string& type) const override {
        auto it = _pings.find(type);
        return it == _pings.end() ? nullptr : &it->second;
    }
    const RoleRenderDef* role(const std::string& type) const override {
        auto it = _roles.find(type);
        return it == _roles.end() ? nullptr : &it->second;
    }
    const std::vector<LoadoutItemDef>* loadoutItems() const override {
        return &_loadoutItems;
    }
    const std::vector<LoadoutSourceDef>* loadoutSources() const override {
        return &_loadoutSources;
    }

private:
    std::unordered_map<std::string, MapObjectDef> _mapObjs;
    std::unordered_map<std::string, MapRenderDef> _mapRenders;
    std::unordered_map<std::string, GameObjRenderDef> _gameObjs;
    std::unordered_map<std::string, ParticleDef> _particles;
    std::unordered_map<std::string, TracerColorDef> _tracers;
    std::unordered_map<std::string, BulletRenderDef> _bullets;
    std::unordered_map<std::string, ExplosionEffectRenderDef> _explosionEffects;
    std::unordered_map<std::string, EmoteRenderDef> _emotes;
    std::unordered_map<std::string, PingRenderDef> _pings;
    std::unordered_map<std::string, RoleRenderDef> _roles;
    std::unordered_map<std::string, EmitterDef> _emitters;
    std::unordered_map<std::string, PlayerPose> _poses;
    std::unordered_map<std::string, PlayerAnimationDef> _animations;
    std::vector<LoadoutItemDef> _loadoutItems;
    std::vector<LoadoutSourceDef> _loadoutSources;
    std::vector<PlaneDef> _planes;
};

} // namespace

void installGeneratedDefs() {
    static GeneratedProvider provider;
    setDefProvider(&provider);
}

} // namespace surv
`;

const dest = path.join(here, "../src/render/GeneratedDefs.cpp");
fs.writeFileSync(dest, out.replace(/[ \t]+$/gm, ""));
console.log(`wrote ${dest}`);
console.log(
    `mapObjs=${mapTypes.length} layers arrays=${emittedLayers.length} maps=${
        Object.keys(MapDefs).length
    } gameObjs=${gameTypes.length} particles=${Object.keys(ParticleDefs).length} emitters=${
        Object.keys(EmitterDefs).length
    }`,
);
