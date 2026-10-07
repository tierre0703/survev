#pragma once
// Render-facing definition tables. The web client reads these from the large
// TypeScript defs modules; here they are exposed through a small provider
// interface so the ported map/objects stay decoupled from codegen (and the
// host tests can run without any generated data). `tools/codegen_render_defs.mjs`
// can emit a concrete provider for the app build.
#include "../core/Collider.h"
#include "../core/Vec2.h"
#include <cstdint>
#include <array>
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

// One floor/ceiling image (building.ts createSpriteFromDef). `pos`/`rot` are
// the per-image offsets; `removeOnDamaged` is ceiling-only.
struct BuildingImageDef {
    std::string sprite;
    float scale = 1.0f;
    float alpha = 1.0f;
    uint32_t tint = 0xffffff;
    Vec2 pos;
    float rot = 0.0f;
    bool mirrorX = false;
    bool mirrorY = false;
    bool removeOnDamaged = false;
};

// building.ts `ceiling.vision` (defaults applied in the def loader).
struct CeilingVisionDef {
    float dist = 5.5f;
    float width = 2.75f;
    float linger = 0.0f;
    float fadeRate = 12.0f;
};

// building.ts `occupiedEmitters` entry.
struct BuildingEmitterDef {
    std::string type;
    Vec2 pos;
    float rot = 0.0f;
    float scale = 1.0f;
    int layer = 0;
    bool parentToCeiling = false;
    Vec2 dir{1.0f, 0.0f};
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

    // buildings
    int zIdx = 0;
    std::vector<BuildingImageDef> floorImgs;
    std::vector<BuildingImageDef> ceilingImgs;
    // ceiling.zoomRegions[].zoomIn (the reveal regions used for vision).
    std::vector<Collider> ceilingZoomIn;
    CeilingVisionDef ceilingVision;
    std::string ceilingDestroyResidue;
    std::vector<BuildingEmitterDef> occupiedEmitters;

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
    uint32_t playerGhillie = 0x5a9e4c;
};

struct MapRenderDef {
    BiomeColors colors;
    bool factionMode = false;
    bool potatoMode = false;
    bool perkMode = false;
    bool turkeyMode = false;
    // biome.valueAdjust (darkens sprite tints for night maps).
    float valueAdjust = 1.0f;
    // biome.particles.camera (map.ts cameraEmitter).
    std::string cameraEmitter;
    // mapDef.assets.atlases (which sprite atlases the map needs).
    std::vector<std::string> atlases;
    // biome.ambience (ambiance.ts tracks).
    std::string ambienceMusic;
    std::string ambienceWind;
    std::string ambienceRiver;
    std::string ambienceWaves;
};

// player.ts uses skinImg/worldImg, not the item's lootImg.
struct SkinRenderDef {
    std::string baseSprite, handL, handR, footSprite, backpackSprite;
    uint32_t baseTint = 0xffffff, handTint = 0xffffff, footTint = 0xffffff;
    uint32_t backpackTint = 0xffffff, baseTintRed = 0xffffff, baseTintBlue = 0xffffff;
    float spriteScale = 0.15f;
};

struct HeldImageDef {
    std::string sprite;
    Vec2 pos;
    Vec2 scale{1.0f, 1.0f};
    float rot = 0.0f;
    uint32_t tint = 0xffffff;
    bool renderOnHand = false, leftHandOnTop = false, handsBelow = false;
};

constexpr int PlayerBoneCount = 6; // HandL/R, FootL/R, MeleeL/R (animData.ts)
struct BonePose {
    Vec2 pivot;
    float rot = 0.0f;
    Vec2 pos;
};
using PlayerPose = std::array<BonePose, PlayerBoneCount>;
enum class PoseEasing { Linear, InSine, OutSine, InOutSine, OutQuart, OutBounce, OutQuad };
struct PoseKeyframe {
    float time = 0.0f;
    unsigned mask = 0;
    PlayerPose bones{};
    PoseEasing easing = PoseEasing::Linear;
};
struct PlayerAnimationDef {
    std::vector<PoseKeyframe> keyframes;
};

// Game-object (item/player) render fields.
struct GameObjRenderDef {
    std::string type;
    std::string category; // "gun" | "melee" | "heal" | "boost" | "outfit" | ...
    ImgDef img;
    bool hasImg = false;
    // loot.ts: `itemDef.type == "xp" && itemDef.emitter`.
    std::string emitter;
    SkinRenderDef skin, visor;
    HeldImageDef worldImg, hipImg;
    std::array<std::array<HeldImageDef, 2>, 3> handImgs{}; // equip/cook/throwing, L/R
    bool ghillie = false, isDual = false;
    int level = 0;
    Vec2 gunOffset, leftHandOffset;
    std::string magSprite;
    Vec2 magPos;
    bool magTop = false;
    float recoil = 0.0f;
    std::string idlePose = "fists";
    std::vector<std::string> attackAnims, deployAnims, idleAnims;
};

// A [min,max] range or a constant (both stored as min==max for constants).
struct RangeDef {
    float min = 0.0f;
    float max = 0.0f;
    bool isConstant = true;
    float random() const;
    float value() const { return min; }
};

// particles.ts ParticleDef. Ranges are stored flattened (particles.ts wraps
// some fields in a Range class; the codegen flattens both forms).
struct ParticleDef {
    std::vector<std::string> images;
    int zOrd = 20;

    RangeDef life;
    RangeDef drag;
    RangeDef rotVel;

    RangeDef scaleStart;
    RangeDef scaleEnd;
    RangeDef scaleLerp;
    bool scaleUseExp = false;
    float scaleExp = 0.0f;

    float alphaStart = 1.0f;
    float alphaEnd = 0.0f;
    RangeDef alphaLerp;
    bool alphaUseExp = false;
    float alphaExp = 0.0f;

    bool hasAlphaIn = false;
    float alphaInStart = 0.0f;
    float alphaInEnd = 0.0f;
    RangeDef alphaInLerp;

    bool hasColor = false;
    uint32_t color = 0xffffff;
    bool ignoreValueAdjust = false;
};

// particles.ts EmitterDef.
struct EmitterDef {
    std::string particle;
    RangeDef rate;
    float radius = 0.0f;
    RangeDef speed;
    float angle = 0.0f;
    bool hasRot = false;
    RangeDef rot;
    float maxCount = 3.402823466e+38f;
    bool hasMaxRate = false;
    RangeDef maxRate;
    float maxElapsed = 0.0f;
    bool hasZOrd = false;
    int zOrd = 0;
};

class DefProvider {
public:
    virtual ~DefProvider() = default;
    virtual const MapObjectDef* mapObject(const std::string& type) const = 0;
    virtual const MapRenderDef* mapRender(const std::string& mapName) const = 0;
    virtual const GameObjRenderDef* gameObject(const std::string& type) const = 0;
    // Particle/emitter tables. Default to null so providers that don't emit
    // them (e.g. host-test fakes) keep compiling.
    virtual const ParticleDef* particle(const std::string&) const { return nullptr; }
    virtual const EmitterDef* emitter(const std::string&) const { return nullptr; }
    virtual const PlayerPose* playerPose(const std::string&) const { return nullptr; }
    virtual const PlayerAnimationDef* playerAnimation(const std::string&) const { return nullptr; }
};

// Process-wide provider, installed by the app/generated code. May be null.
const DefProvider* getDefProvider();
void setDefProvider(const DefProvider* provider);

} // namespace surv
