// Generates src/render/GeneratedDefs.cpp from the real shared/ TS defs. Emits a
// concrete DefProvider (biome colors, map-object images/colliders, structure
// layers/stairs/mask, loot images) used by the M4 rendering port.
//
// Usage: node --experimental-transform-types tools/codegen_render_defs.mjs
import { fileURLToPath } from "node:url";
import * as path from "node:path";
import * as fs from "node:fs";

const here = path.dirname(fileURLToPath(import.meta.url));
const sharedDir = path.resolve(here, "../../shared");

function pathToFile(dir, rel) {
    return new URL(`file:///${path.join(dir, rel).replace(/\\/g, "/")}`);
}

const { MapObjectDefs, GameObjectDefs } = await import(pathToFile(sharedDir, "defs/register.ts"));
const { MapDefs } = await import(pathToFile(sharedDir, "defs/mapDefs.ts"));

const mapTypes = MapObjectDefs.getAllTypes();
const gameTypes = GameObjectDefs.getAllTypes();

const n = (x, d = 0) => (Number.isFinite(Number(x)) ? Number(x) : d);
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

    const refs = { layers: "nullptr", stairs: "nullptr", mask: "nullptr", shapes: "nullptr" };
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

    entries += `    {${str(type)},${imgInit(img)},${hasImg ? 1 : 0},${colInit(collision)},${
        hasCollision ? 1 : 0
    },${def.isDoor ? 1 : 0},${def.isButton ? 1 : 0},${def.isTree ? 1 : 0},${def.isWall ? 1 : 0},${
        map && map.display === false ? 0 : 1
    },${map && map.color !== undefined ? 1 : 0},${n(map?.color, 0)},${n(map?.scale, 1)},${colInit(
        bounds,
    )},${bounds ? 1 : 0},${refs.layers},${layers ? layers.length : 0},${refs.stairs},${
        stairs ? stairs.length : 0
    },${refs.mask},${mask ? mask.length : 0},${refs.shapes},${shapes.length}},\n`;
}

// Map render defs.
let mapRenderEntries = "";
for (const [name, def] of Object.entries(MapDefs)) {
    const c = def?.biome?.colors || {};
    const gm = def?.gameMode || {};
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
    },${gm.turkeyMode ? 1 : 0}},\n`;
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
    gameEntries += `    {${str(type)},${str(def.type || "")},${imgInit(img)},1},\n`;
}

const out = `// GENERATED FILE - do not edit. Run tools/codegen_render_defs.mjs.
#include "Defs.h"
#include <string>
#include <unordered_map>

namespace surv {
namespace {

struct RawCollider { int type; float a, b, c, d; };
struct RawImg { const char* sprite; float scale; float alpha; unsigned tint; int zIdx; int mirrorX; int mirrorY; };
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
};
struct RawMapRender {
    const char* name;
    unsigned background, beach, grass, riverbank, water, waterRipple, lakeRiverbank, lakeWater, lakeWaterRipple, underground;
    int faction, potato, perk, turkey;
};
struct RawGameObj { const char* type; const char* category; RawImg img; int hasImg; };

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

${arrays}
static const RawMapObj kMapObjs[] = {
${entries}};
static const RawMapRender kMapRenders[] = {
${mapRenderEntries}};
static const RawGameObj kGameObjs[] = {
${gameEntries}};

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
            _mapRenders[r.name] = d;
        }
        for (const auto& r : kGameObjs) {
            GameObjRenderDef d;
            d.type = r.type;
            d.category = r.category;
            d.img = toImg(r.img);
            d.hasImg = r.hasImg != 0;
            _gameObjs[d.type] = d;
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

private:
    std::unordered_map<std::string, MapObjectDef> _mapObjs;
    std::unordered_map<std::string, MapRenderDef> _mapRenders;
    std::unordered_map<std::string, GameObjRenderDef> _gameObjs;
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
    } gameObjs=${gameTypes.length}`,
);
