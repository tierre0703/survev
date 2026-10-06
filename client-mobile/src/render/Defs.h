#pragma once
// Render-facing definition tables. The web client reads these from the large
// TypeScript defs modules; here they are exposed through a small provider
// interface so the ported map/objects stay decoupled from codegen (and the
// host tests can run without any generated data). `tools/codegen_defs.mjs`
// can emit a concrete provider for the app build.
#include "../core/Collider.h"
#include "../core/Vec2.h"
#include <cstdint>
#include <string>
#include <vector>

namespace surv {

// Port of the `img` block shared by obstacle/building/structure defs.
struct ImgDef {
    std::string sprite;
    float scale = 1.0f;
    float alpha = 1.0f;
    uint32_t tint = 0xffffff;
    int zIdx = 0;
    bool mirrorX = false;
    bool mirrorY = false;
    std::string residue;
};

struct MapDisplayDef {
    bool display = true;
    bool hasColor = false;
    uint32_t color = 0;
    float scale = 1.0f;
};

// One minimap shape (def.map.shapes entries and generated fallbacks).
struct MapShapeDef {
    Collider collider;
    float scale = 1.0f;
    uint32_t color = 0;
};

struct StructureLayerDef {
    std::string type;
    Vec2 pos;
    int ori = 0;
    bool inheritOri = true;
    bool underground = false;
};

struct StairDef {
    Collider collision;
    Vec2 downDir;
    bool noCeilingReveal = false;
    bool lootOnly = false;
};

// Union of the render-relevant fields of ObstacleDef/BuildingDef/StructureDef.
struct MapObjectDef {
    std::string type; // "obstacle" | "building" | "structure" | "decal" | "loot_spawner"
    ImgDef img;
    MapDisplayDef map;
    Collider collision; // obstacles
    bool hasCollision = false;
    bool isDoor = false;
    bool isButton = false;
    bool isTree = false;
    bool isWall = false;

    std::vector<MapShapeDef> mapShapes;

    // structures
    std::vector<StructureLayerDef> layers;
    std::vector<StairDef> stairs;
    std::vector<Collider> mask;

    // pre-computed bounding collider (mapHelpers.getBoundingCollider)
    Collider boundingCollider;
    bool hasBounding = false;
};

struct BiomeColors {
    uint32_t background = 0x2b2b2b;
    uint32_t beach = 0xd9c38a;
    uint32_t grass = 0x5a9e4c;
    uint32_t riverbank = 0xc2b280;
    uint32_t water = 0x3a6ea5;
    uint32_t waterRipple = 0x5a8ec5;
    uint32_t lakeRiverbank = 0xc2b280;
    uint32_t lakeWater = 0x3a6ea5;
    uint32_t lakeWaterRipple = 0x5a8ec5;
    uint32_t underground = 0x1b0e0b;
};

struct MapRenderDef {
    BiomeColors colors;
    bool factionMode = false;
    bool potatoMode = false;
    bool perkMode = false;
    bool turkeyMode = false;
};

// Game-object (item/player) render fields used by loot/dead bodies.
struct GameObjRenderDef {
    std::string type;
    std::string category; // "gun" | "melee" | "heal" | "boost" | "outfit" | ...
    ImgDef img;
    bool hasImg = false;
};

class DefProvider {
public:
    virtual ~DefProvider() = default;
    virtual const MapObjectDef* mapObject(const std::string& type) const = 0;
    virtual const MapRenderDef* mapRender(const std::string& mapName) const = 0;
    virtual const GameObjRenderDef* gameObject(const std::string& type) const = 0;
};

// Process-wide provider, installed by the app/generated code. May be null.
const DefProvider* getDefProvider();
void setDefProvider(const DefProvider* provider);

} // namespace surv
