// Generates src/render/GeneratedDefs.cpp from the real shared/ TS defs. Emits a
// concrete DefProvider (biome colors, map-object images/colliders, structure
// layers/stairs/mask, building floor/ceiling images + vision, loot images, and
// the particles.ts particle/emitter tables) used by the M4 rendering port.
//
// Usage: node --experimental-transform-types tools/codegen_render_defs.mjs
import { fileURLToPath } from "node:url";
import * as path from "node:path";
import * as fs from "node:fs";

const here = path.dirname(fileURLToPath(import.meta.url));
const sharedDir = path.resolve(here, "../../shared");
const clientDir = path.resolve(here, "../../client");

function pathToFile(dir, rel) {
    return new URL(`file:///${path.join(dir, rel).replace(/\\/g, "/")}`);
}

const { MapObjectDefs, GameObjectDefs } = await import(pathToFile(sharedDir, "defs/register.ts"));
const { MapDefs } = await import(pathToFile(sharedDir, "defs/mapDefs.ts"));
const { util } = await import(pathToFile(sharedDir, "utils/util.ts"));
const { math } = await import(pathToFile(sharedDir, "utils/math.ts"));
const { v2 } = await import(pathToFile(sharedDir, "utils/v2.ts"));

const mapTypes = MapObjectDefs.getAllTypes();
const gameTypes = GameObjectDefs.getAllTypes();

const n = (x, d = 0) => (Number.isFinite(Number(x)) ? Number(x) : d);
// Float literal helper: clamps out-of-float-range constants (Number.MAX_VALUE)
// so the generated braced initializers don't trip -Wc++11-narrowing.
const f = (x, d = 0) => {
    let v = Number.isFinite(Number(x)) ? Number(x) : d;
    if (v > 3.402823466e38) v = 3.402823466e38;
    if (v < -3.402823466e38) v = -3.402823466e38;
    return v;
};
const esc = (s) => String(s ?? "").replace(/\\/g, "\\\\").replace(/"/g, '\\"');
const str = (s) => `"${esc(s)}"`;

function colInit(c) {
    if (!c || typeof c !== "object") return "{1,0,0,0,0}";
    if (c.type === 0) return `{0,${n(c.pos.x)},${n(c.pos.y)},${n(c.rad)},0}`;
    return `{1,${n(c.min.x)},${n(c.min.y)},${n(c.max.x)},${n(c.max.y)}}`;
}
function imgInit(img) {
    const i = img || {};
    return `{${str(i.sprite)},${n(i.scale, 1)},${n(i.alpha, 1)},${n(i.tint, 0xffffff)},${n(
        i.zIdx,
        0,
    )},${i.mirrorX ? 1 : 0},${i.mirrorY ? 1 : 0}}`;
}
// building FloorImage -> RawBImg.
function bimgInit(img) {
    const i = img || {};
    return `{${str(i.sprite)},${n(i.scale, 1)},${n(i.alpha, 1)},${n(i.tint, 0xffffff)},${n(
        i.pos?.x,
        0,
    )},${n(i.pos?.y, 0)},${n(i.rot, 0)},${i.mirrorX ? 1 : 0},${i.mirrorY ? 1 : 0},${
        i.removeOnDamaged ? 1 : 0
    }}`;
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
    if (kind === "structure") {
        layers = def.layers || [];
        stairs = def.stairs || [];
        mask = def.mask || [];
    }

    let bounds = null;
    if (collision) {
        bounds = collision;
    } else if (Array.isArray(def.mapObstacleBounds) && def.mapObstacleBounds.length) {
        bounds = unionBounds(def.mapObstacleBounds);
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
    };
    if (layers && layers.length) {
        const name = `kLayers_${i}`;
        arrays += `static const RawLayer ${name}[] = {\n`;
        for (const l of layers) {
            arrays += `    {${str(l.type)},${n(l.pos.x)},${n(l.pos.y)},${n(l.ori, 0)},${
                l.inheritOri === undefined || l.inheritOri ? 1 : 0
            },${l.underground ? 1 : 0}},\n`;
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
    if (kind === "building") {
        zIdx = n(def.zIdx, 0);
        floorImgs = (def.floor && def.floor.imgs) || [];
        ceilingImgs = (def.ceiling && def.ceiling.imgs) || [];
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
            arrays += `    {${str(e.type)},${n(e.pos?.x)},${n(e.pos?.y)},${n(e.rot, 0)},${n(
                e.scale,
                1,
            )},${n(e.layer, 0)},${e.parentToCeiling ? 1 : 0},${n(e.dir?.x, 1)},${n(e.dir?.y, 0)}},\n`;
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

    entries += `    {${str(type)},${imgInit(img)},${hasImg ? 1 : 0},${colInit(collision)},${
        hasCollision ? 1 : 0
    },${def.isDoor ? 1 : 0},${def.isButton ? 1 : 0},${def.isTree ? 1 : 0},${def.isWall ? 1 : 0},${
        map && map.display === false ? 0 : 1
    },${map && map.color !== undefined ? 1 : 0},${n(map?.color, 0)},${n(map?.scale, 1)},${colInit(
        bounds,
    )},${bounds ? 1 : 0},${refs.layers},${layers ? layers.length : 0},${refs.stairs},${
        stairs ? stairs.length : 0
    },${refs.mask},${mask ? mask.length : 0},${refs.shapes},${shapes.length},${zIdx},${
        refs.floorImgs
    },${floorImgs.length},${refs.ceilingImgs},${ceilingImgs.length},${n(vision.dist, 5.5)},${n(
        vision.width,
        2.75,
    )},${n(vision.linger, 0)},${n(vision.fadeRate, 12)},${str(destroyResidue)},${refs.emitters},${
        emitters.length
    },${refs.zoomIns},${zoomIns.length}},\n`;
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
    mapRenderEntries += `    {${str(name)},${n(c.background, 0x2b2b2b)},${n(
        c.beach,
        0xd9c38a,
    )},${n(c.grass, 0x5a9e4c)},${n(c.riverbank, 0xc2b280)},${n(c.water, 0x3a6ea5)},${n(
        c.waterRipple,
        0x5a8ec5,
    )},${n(c.lakeRiverbank, n(c.riverbank, 0xc2b280))},${n(c.lakeWater, n(c.water, 0x3a6ea5))},${n(
        c.lakeWaterRipple,
        n(c.waterRipple, 0x5a8ec5),
    )},${n(c.underground, 0x1b0e0b)},${gm.factionMode ? 1 : 0},${gm.potatoMode ? 1 : 0},${
        gm.perkMode ? 1 : 0
    },${gm.turkeyMode ? 1 : 0},${n(def?.biome?.valueAdjust, 1)},${str(
        def?.biome?.particles?.camera || "",
    )},${atlasArr},${atlases.length},${str(amb.music || "")},${str(amb.wind || "")},${str(
        amb.river || "",
    )},${str(amb.waves || "")}},\n`;
}

// Game object render defs (loot images).
let gameEntries = "";
for (const type of gameTypes) {
    let def;
    try {
        def = GameObjectDefs.typeToDef(type);
    } catch {
        def = null;
    }
    if (!def) continue;
    const img = def.lootImg || def.img || null;
    if (!img || !img.sprite) continue;
    gameEntries += `    {${str(type)},${str(def.type || "")},${imgInit(img)},1,${str(def.emitter || "")}},\n`;
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
    if (color === undefined) return { has: 0, value: 0xffffff };
    if (typeof color === "function") {
        // Average several samples so random color closures land near their mean.
        let r = 0;
        let g = 0;
        let b = 0;
        const samples = 16;
        for (let i = 0; i < samples; i++) {
            const c = color() & 0xffffff;
            r += (c >> 16) & 0xff;
            g += (c >> 8) & 0xff;
            b += c & 0xff;
        }
        r = Math.round(r / samples);
        g = Math.round(g / samples);
        b = Math.round(b / samples);
        return { has: 1, value: (r << 16) | (g << 8) | b };
    }
    return { has: 1, value: n(color) & 0xffffff };
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
    particleEntries += `    {${str(name)},${arrName},${images.length},${n(def.zOrd, 20)},${rangeInit(
        life,
    )},${rangeInit(drag)},${rangeInit(rotVel)},${rangeInit(scaleStart)},${rangeInit(
        scaleEnd,
    )},${rangeInit(scaleLerp)},${scaleUseExp},${n(def.scale?.exp)},${n(def.alpha?.start, 1)},${n(
        def.alpha?.end,
        0,
    )},${rangeInit(alphaLerp)},${alphaUseExp},${n(def.alpha?.exp)},${hasAlphaIn},${n(
        def.alphaIn?.start,
        0,
    )},${n(def.alphaIn?.end, 0)},${rangeInit(alphaInLerp)},${color.has},${color.value},${
        def.ignoreValueAdjust ? 1 : 0
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
    emitterEntries += `    {${str(name)},${str(def.particle)},${rangeInit(rate)},${n(
        def.radius,
    )},${rangeInit(speed)},${n(def.angle)},${hasRot},${rangeInit(rot)},${f(
        def.maxCount,
        Number.MAX_VALUE,
    )},${hasMaxRate},${rangeInit(maxRate)},${n(def.maxElapsed)},${hasZOrd},${n(
        def.zOrd,
    )}},\n`;
}

const out = `// GENERATED FILE - do not edit. Run tools/codegen_render_defs.mjs.
#include "Defs.h"
#include <string>
#include <unordered_map>

namespace surv {
namespace {

struct RawCollider { int type; float a, b, c, d; };
struct RawImg { const char* sprite; float scale; float alpha; unsigned tint; int zIdx; int mirrorX; int mirrorY; };
struct RawBImg { const char* sprite; float scale; float alpha; unsigned tint; float px, py, rot; int mirrorX, mirrorY, removeOnDamaged; };
struct RawBEmitter { const char* type; float px, py, rot, scale; int layer, parentToCeiling; float dirx, diry; };
struct RawLayer { const char* type; float x, y; int ori; int inheritOri; int underground; };
struct RawStair { RawCollider col; float dx, dy; int noCeilingReveal; int lootOnly; };
struct RawShape { RawCollider col; float scale; unsigned color; };
struct RawMapObj {
    const char* type; RawImg img; int hasImg; RawCollider collision; int hasCollision;
    int isDoor, isButton, isTree, isWall; int mapDisplay, mapHasColor; unsigned mapColor; float mapScale;
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
};
struct RawMapRender {
    const char* name;
    unsigned background, beach, grass, riverbank, water, waterRipple, lakeRiverbank, lakeWater, lakeWaterRipple, underground;
    int faction, potato, perk, turkey; float valueAdjust; const char* cameraEmitter;
    const char* const* atlases; int atlasCount;
    const char* ambMusic; const char* ambWind; const char* ambRiver; const char* ambWaves;
};
struct RawGameObj { const char* type; const char* category; RawImg img; int hasImg; const char* emitter; };
struct RawRange { float min; float max; int isConstant; };
struct RawParticle {
    const char* name; const char* const* images; int imageCount; int zOrd;
    RawRange life, drag, rotVel, scaleStart, scaleEnd, scaleLerp;
    int scaleUseExp; float scaleExp;
    float alphaStart, alphaEnd; RawRange alphaLerp; int alphaUseExp; float alphaExp;
    int hasAlphaIn; float alphaInStart, alphaInEnd; RawRange alphaInLerp;
    int hasColor; unsigned color; int ignoreValueAdjust;
};
struct RawEmitter {
    const char* name; const char* particle; RawRange rate; float radius; RawRange speed; float angle;
    int hasRot; RawRange rot; float maxCount; int hasMaxRate; RawRange maxRate; float maxElapsed;
    int hasZOrd; int zOrd;
};

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

${arrays}${particleArrays}
static const RawMapObj kMapObjs[] = {
${entries}};
${mapAtlasArrays}static const RawMapRender kMapRenders[] = {
${mapRenderEntries}};
static const RawGameObj kGameObjs[] = {
${gameEntries}};
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
            d.isWall = r.isWall != 0;
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
            _mapRenders[r.name] = d;
        }
        for (const auto& r : kGameObjs) {
            GameObjRenderDef d;
            d.type = r.type;
            d.category = r.category;
            d.img = toImg(r.img);
            d.hasImg = r.hasImg != 0;
            d.emitter = r.emitter ? r.emitter : "";
            _gameObjs[d.type] = d;
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

private:
    std::unordered_map<std::string, MapObjectDef> _mapObjs;
    std::unordered_map<std::string, MapRenderDef> _mapRenders;
    std::unordered_map<std::string, GameObjRenderDef> _gameObjs;
    std::unordered_map<std::string, ParticleDef> _particles;
    std::unordered_map<std::string, EmitterDef> _emitters;
};

} // namespace

void installGeneratedDefs() {
    static GeneratedProvider provider;
    setDefProvider(&provider);
}

} // namespace surv
`;

const dest = path.join(here, "../src/render/GeneratedDefs.cpp");
fs.writeFileSync(dest, out);
console.log(`wrote ${dest}`);
console.log(
    `mapObjs=${mapTypes.length} layers arrays=${emittedLayers.length} maps=${
        Object.keys(MapDefs).length
    } gameObjs=${gameTypes.length} particles=${Object.keys(ParticleDefs).length} emitters=${
        Object.keys(EmitterDefs).length
    }`,
);
