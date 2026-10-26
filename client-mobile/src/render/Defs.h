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
    float ori = 0.0f; // obstacle.ts img.ori (degrees, folded into imgRot)
    std::string residue;
};

struct MapDisplayDef {
    bool display = true;
    bool hasColor = false;
    uint32_t color = 0;
    float scale = 1.0f;
};

// shared/defs/mapObjectsTyping.ts `SurfaceType`. Drives player submersion and
// the `footstep_${surface.type}` sound group, so it must cover every floor
// material the maps use (map.ts getGroundSurface).
enum class SurfaceType {
    Water,
    Sand,
    Grass,
    Tile,
    Asphalt,
    Brick,
    Stone,
    Carpet,
    Bunker,
    Container,
    Warehouse,
    House,
    Shack,
    Snow,
};

// The web client stores the type as a lowercase string and passes it straight
// into `playGroup("footstep_" + type)`; keep that spelling.
inline const char* surfaceTypeName(SurfaceType type) {
    switch (type) {
        case SurfaceType::Water: return "water";
        case SurfaceType::Sand: return "sand";
        case SurfaceType::Grass: return "grass";
        case SurfaceType::Tile: return "tile";
        case SurfaceType::Asphalt: return "asphalt";
        case SurfaceType::Brick: return "brick";
        case SurfaceType::Stone: return "stone";
        case SurfaceType::Carpet: return "carpet";
        case SurfaceType::Bunker: return "bunker";
        case SurfaceType::Container: return "container";
        case SurfaceType::Warehouse: return "warehouse";
        case SurfaceType::House: return "house";
        case SurfaceType::Shack: return "shack";
        case SurfaceType::Snow: return "snow";
    }
    return "grass";
}

inline SurfaceType surfaceTypeFromName(const std::string& name) {
    if (name == "water") return SurfaceType::Water;
    if (name == "sand") return SurfaceType::Sand;
    if (name == "tile") return SurfaceType::Tile;
    if (name == "asphalt") return SurfaceType::Asphalt;
    if (name == "brick") return SurfaceType::Brick;
    if (name == "stone") return SurfaceType::Stone;
    if (name == "carpet") return SurfaceType::Carpet;
    if (name == "bunker") return SurfaceType::Bunker;
    if (name == "container") return SurfaceType::Container;
    if (name == "warehouse") return SurfaceType::Warehouse;
    if (name == "house") return SurfaceType::House;
    if (name == "shack") return SurfaceType::Shack;
    if (name == "snow") return SurfaceType::Snow;
    return SurfaceType::Grass;
}

// decal.ts surface defs only use water/sand/grass; keep the old name for the
// generated table and the decal code.
using SurfaceTypeDef = SurfaceType;

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
    // structure.ts: `underground` defaults to `i == 1` when the def omits it,
    // so the "defined" flag must be preserved.
    bool hasUnderground = false;
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

// building.ts `def.floor.surfaces[]` (building.ts `surfaces`): the floor
// material under a building, used by map.ts getGroundSurface. The colliders
// are authored in building-local space and transformed in Building::m_updateData.
struct BuildingSurfaceDef {
    std::string type;
    std::vector<Collider> colliders;
    bool hasWaterColor = false;
    uint32_t waterColor = 0;
    bool hasRippleColor = false;
    uint32_t rippleColor = 0;
};

// structureDefs.ts `interiorSound`: the ambient track played inside a
// structure (structure.ts updateInteriorSounds drives the two Ambiance
// immediate-mode tracks from these fields).
struct InteriorSoundDef {
    std::string sound;
    std::string soundAlt;
    std::string filter;
    float transitionTime = 1.0f;
    float outsideMaxDist = 10.0f;
    float outsideVolume = 0.0f;
    bool hasUndergroundVolume = false;
    float undergroundVolume = 1.0f;
};

// buildingDefs.ts `soundEmitters`: looping positional ambience baked into a
// building (bathhouse steam, potato-mode waves).
struct BuildingSoundEmitterDef {
    std::string sound;
    std::string channel;
    Vec2 pos;
    float rangeMin = 0.0f;
    float rangeMax = 0.0f;
    float falloff = 1.0f;
    float volume = 1.0f;
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
    // obstacle.ts `isBush`: player player.ts bush enter/exit rustle FX.
    bool isBush = false;
    bool isWall = false;
    // obstacle.ts `randomRotation`: rotate the obstacle image by __id % 360.
    bool randomRotation = false;
    // obstacle.ts `explosion` particle (hasExplosion gates the health smoke).
    bool hasExplosion = false;
    std::string explosionParticle;
    // obstacle.ts `explodeParticle` (destroy FX; string or 2-item random pick).
    std::string explodeParticle;
    std::string explodeParticleAlt;
    // obstacle.ts door.slideOffset (used to compute the open door position).
    float doorSlideOffset = 0.0f;
    // obstacle.ts door.openSpeed (position/rotation interpolation speed).
    float doorOpenSpeed = 2.0f;
    // obstacle.ts def.door.casingImg (rendered behind/next to the door).
    std::string doorCasingSprite;
    Vec2 doorCasingPos;
    float doorCasingScale = 1.0f;
    uint32_t doorCasingTint = 0xffffff;
    float doorCasingAlpha = 1.0f;
    // obstacle.ts door.spriteAnchor (image pivot; 0.5 for every shipped door).
    Vec2 doorSpriteAnchor{0.5f, 0.5f};

    // obstacle.ts/obstacleDefs: `height` gates melee/bullet collision, `punch`
    // is the obstacle hit sound (played via the punch group), and `hitParticle`
    // is the impact spray. `collidable`/`isWindow` mirror the def booleans used
    // by collisionHelpers.intersectSegmentObstacle.
    float height = 0.0f;
    bool collidable = false;
    bool isWindow = false;
    std::string hitParticle;
    std::string punchSound;
    // obstacle.ts door/button/explosion sounds (played on state changes).
    std::string doorSoundChange;
    std::string doorSoundOpen;
    std::string doorSoundClose;
    std::string doorSoundUnlock;
    std::string buttonSoundOn;
    std::string buttonSoundOff;
    std::string explodeSound;
    // obstacle.ts def.sound.bullet (bullet impact hit group).
    std::string bulletSound;
    // obstacle.ts def.sound.enter (bush rustle on enter/exit).
    std::string soundEnter;
    // obstacle.ts button useImg/offImg sprite swap + useParticle on seq change.
    std::string buttonUseImg;
    std::string buttonOffImg;
    std::string buttonUseParticle;

    // decalDefs (decal.ts): `surface` recolours getGroundSurface for decals
    // painted over water; `gore` fades that water colour with the gore level.
    bool hasSurface = false;
    SurfaceTypeDef surfaceType = SurfaceTypeDef::Water;
    uint32_t surfaceWaterColor = 0;
    uint32_t surfaceRippleColor = 0;
    bool hasGore = false;
    float goreFadeStart = 0.0f;
    float goreFadeEnd = 1.0f;
    float goreFadePow = 1.0f;
    float goreFadeSpeed = 1.0f;
    bool goreHasTint = false;
    uint32_t goreTint = 0xffffff;
    bool goreHasAlpha = false;
    float goreAlpha = 1.0f;
    bool goreHasWaterColor = false;
    uint32_t goreWaterColor = 0;
    bool goreHasRippleColor = false;
    uint32_t goreRippleColor = 0;
    // decalDefs img extras: flicker (map lights) and ignoreAdjust.
    bool imgFlicker = false;
    float imgFlickerMin = 1.0f;
    float imgFlickerMax = 1.0f;
    float imgFlickerRate = 1.0f;
    bool imgIgnoreAdjust = false;
    bool hasLifetime = false;
    float lifetimeMin = 0.0f;
    float lifetimeMax = 0.0f;

    std::vector<MapShapeDef> mapShapes;

    // structures
    std::vector<StructureLayerDef> layers;
    std::vector<StairDef> stairs;
    std::vector<Collider> mask;

    // buildings
    int zIdx = 0;
    std::vector<BuildingImageDef> floorImgs;
    std::vector<BuildingImageDef> ceilingImgs;
    std::vector<BuildingSoundEmitterDef> soundEmitters;
    // ceiling.zoomRegions[].zoomIn (the reveal regions used for vision).
    std::vector<Collider> ceilingZoomIn;
    std::vector<BuildingSurfaceDef> surfaces;
    // structures
    bool hasInteriorSound = false;
    InteriorSoundDef interiorSound;
    // building.ts puzzle/ceiling-destroy sounds.
    std::string puzzleSoundFail;
    std::string puzzleSoundComplete;
    std::string ceilingDestroySound;
    CeilingVisionDef ceilingVision;
    std::string ceilingDestroyResidue;
    // building.ts ceiling.destroy: particle count + sprite (destroyCeilingFx).
    std::string ceilingDestroyParticle;
    int ceilingDestroyParticleCount = 0;
    std::vector<BuildingEmitterDef> occupiedEmitters;

    // pre-computed bounding collider (mapHelpers.getBoundingCollider)
    Collider boundingCollider;
    bool hasBounding = false;
};

// shared/defs/gameObjects/emoteDefs.ts. Used by the world emote sprites
// (EmoteManager) and the emote wheel, which both need the atlas frame and the
// sound/channel the web client takes from EmotesDefs.
struct EmoteRenderDef {
    std::string texture; // atlas frame, e.g. "face-happy.img"
    std::string sound;   // e.g. "emote_01"
    std::string channel; // "ui"
    std::string name;    // display name, e.g. "Happy Face"
    bool teamOnly = false;
    int category = 0;
};

// shared/defs/gameObjects/pingDefs.ts. Used by the world ping indicators
// (EmoteManager), the team-ping wheel and the minimap ping sprites.
struct PingRenderDef {
    std::string texture;    // world ping sprite, e.g. "ping-team-danger.img"
    std::string mapTexture; // minimap sprite, e.g. "ping-map-danger.img"
    std::string sound;      // e.g. "ping_danger_01"
    std::string soundLeader;
    bool pingMap = true;
    float pingLife = 4.0f;
    float mapLife = 4.0f;
    bool worldDisplay = true;
    bool mapEvent = false;
    uint32_t tint = 0xffffff;
};

// shared/defs/gameObjects/roleDefs.ts. Role-specific minimap icons (leader,
// captain, medic, last man) used by the player/team dots.
struct RoleRenderDef {
    std::string mapIconAlive; // e.g. "player-star.img"
    std::string mapIconDead;  // e.g. "skull-leader.img" (optional)
    std::string soundAssign;  // game.ts RoleAnnouncement assigned sound
    std::string soundDead;    // ... dead sound
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
    // mapDef.biome.sound.riverShore: footstep surface on river banks
    // ("sand" everywhere except main_spring/woods_spring -> "stone").
    SurfaceTypeDef riverShoreSurface = SurfaceTypeDef::Sand;
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
    // biome.airdrop (plane.ts / airdrop.ts sprite + sound selection).
    std::string airdropPlaneImg = "map-plane-01.img";
    std::string airdropPlaneSound = "plane_01";
    std::string airdropChuteImg = "map-chute-01.img";
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
    unsigned noMask = 0; // bones missing from this frame that must not interpolate
};
// animData.ts animation effect kinds (animPlaySound, animMeleeCollision, ...).
enum class AnimEffectFn { PlaySound, MeleeCollision, ThrowableParticles };
struct AnimEffect {
    float time = 0.0f;
    AnimEffectFn fn = AnimEffectFn::PlaySound;
    std::string sound;       // PlaySound: EmitterDef sound key
    std::string playerHit;   // MeleeCollision: MeleeDef sound.playerHit key
};
struct PlayerAnimationDef {
    std::vector<PoseKeyframe> keyframes;
    std::vector<AnimEffect> effects;
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
    // meleeDefs (player.ts getMeleeCollider + animMeleeCollision): attack
    // capsule offset/radius, whether the attack cleaves through walls, and the
    // impact sounds. Obstacles' punch groups reuse punchSound.
    bool hasAttack = false;
    Vec2 attackOffset;
    float attackRad = 0.0f;
    bool cleave = false;
    std::string playerHit;  // meleeDefs sound.playerHit
    std::string punchSound; // meleeDefs/obstacleDefs sound.punch
    // Item action sounds (player.ts playActionStartEffect / deploy / pickup).
    std::string soundReload;
    std::string soundReloadAlt;
    std::string soundDeploy;
    std::string soundUse;
    std::string soundPickup;
    // loot.ts lootImg border/inner art (the outer ring behind the item art).
    std::string lootBorder;
    uint32_t lootBorderTint = 0;
    float lootInnerScale = 0.8f;
    float lootRot = 0.0f;
    bool lootMirror = false;
    // gameObjects/explosionsDefs.ts explosionEffectType (frag/barrel/...).
    std::string explosionEffectType;
    // throwableDefs: projectile trail/physics (projectile.ts).
    bool hasTrail = false;
    float trailMaxLength = 0.0f;
    float trailWidth = 0.0f;
    float trailAlpha = 1.0f;
    uint32_t trailTint = 0xffffff;
    float throwSpeed = 20.0f;
    float spinVel = 0.0f;
    float spinDrag = 1.0f;
    bool randomizeSpinDir = false;
    bool alwaysRenderOntop = false;
    float projectileRad = 1.0f;
    // gunDefs.bulletType + barrelLength (touch.ts LineSprites aim-line range).
    std::string bulletType;
    float barrelLength = 0.0f;
    // player.ts action aura (heal/boost defs) and frozen patch sprites.
    std::string auraSprite;
    uint32_t auraTint = 0xff00ff;
    bool hasAura = false;
    bool useThrowParticles = false;
    std::vector<std::string> frozenSprites;
    std::vector<std::string> unlocks; // unlock_default and future account unlocks
    std::string ammoType; // gun reserve-ammo inventory key
    // shot.ts createCasingParticle: custom casing particle type (gunDefs
    // particle.casing; empty = ammoType) and caseTiming == "reload" (revolvers/
    // pistols eject when reloading, not on every shot).
    std::string casing;
    bool casingOnReload = false;
    // shot.ts createCasingParticle: shell placement/scale/forward/reverse and
    // the reload-shot count (revolvers eject maxReload casings).
    int maxReload = 1;
    float shellScale = 1.0f;
    float shellOffset = 0.375f;
    float shellOffsetY = 0.0f;
    float shellForward = 0.0f;
    bool shellReverse = false;

    // Map indicator (item/role minimap marker) fields.
    std::string mapIndicatorSprite;
    uint32_t mapIndicatorTint = 0xffffff;
    bool mapIndicatorPulse = false;
    uint32_t mapIndicatorPulseTint = 0xffffff;
};

struct PlaneDef {
    int type = 0; // GameConfig.Plane
    float planeVel = 48.0f;
    float planeRad = 150.0f;
    float soundRangeMult = 2.5f;
    float soundRangeDelta = 0.25f;
    float soundRangeMax = 92.0f;
    float fallOff = 0.0f;
};

struct TracerColorDef {
    uint32_t regular = 0xffffff;
    uint32_t saturated = 0xffffff;
    uint32_t chambered = 0xffffff;
    uint32_t apSaturated = 0xffffff;
    float alphaRate = 0.0f;
    float alphaMin = 0.0f;
};

struct BulletRenderDef {
    float distance = 0.0f;
    float speed = 0.0f;
    float variance = 0.0f;
    std::string tracerColor;
    float tracerWidth = 1.0f;
    float tracerLength = 0.0f;
    uint32_t flareColor = 0xffffff;
    float maxFlareScale = 1.0f;
    bool suppressed = false;
};

// client/src/objects/explosion.ts ExplosionEffectDefs (moved to
// shared/defs/explosionEffectDefs.ts for the native codegen).
struct ExplosionEffectRenderDef {
    std::string burstParticle;
    float burstScale = 1.0f;
    std::string grassSound;
    std::string waterSound;
    float detune = 0.0f;
    float volume = 1.0f;
    std::string scatterParticle;
    int scatterCount = 0;
    float scatterSpeedMin = 0.0f;
    float scatterSpeedMax = 0.0f;
    int rippleCount = 0;
    float shakeStr = 0.0f;
    float shakeDur = 0.0f;
    float lifetime = 1.0f;
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
    // particles.ts color closures: HSV ranges sampled from the TS function.
    // colorVariance picks a fresh hue/sat/value per particle at runtime.
    bool colorVariance = false;
    float colorHueMin = 0.0f;
    float colorHueMax = 0.0f;
    float colorSatMin = 1.0f;
    float colorSatMax = 1.0f;
    float colorValMin = 1.0f;
    float colorValMax = 1.0f;
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

// Shared loadout item metadata (shared/defs BaseLoadoutItem): outfits, melee,
// emotes, heal/boost effects and crosshairs. Ownership is resolved separately
// by ui::Loadout (unlock_default + account items); this table only carries the
// display data the native customize screen needs.
struct LoadoutItemDef {
    std::string type;
    std::string category; // "outfit" | "melee" | "emote" | "heal_effect" | "boost_effect" | "crosshair"
    std::string name;
    std::string lore;
    std::string sprite; // loot/emote/crosshair texture name
    int rarity = 0;
    // Outfit loot art is a white silhouette tinted by lootImg.tint (the web
    // applies an SVG multiply filter). The native loadout tile applies the same
    // multiply with a ColorMatrix.
    uint32_t tint = 0xffffff;
};

class DefProvider {
public:
    virtual ~DefProvider() = default;
    virtual const MapObjectDef* mapObject(const std::string& type) const = 0;
    virtual const MapRenderDef* mapRender(const std::string& mapName) const = 0;
    virtual const GameObjRenderDef* gameObject(const std::string& type) const = 0;
    // Loadout metadata for the Java customize screen (null in fakes).
    virtual const std::vector<LoadoutItemDef>* loadoutItems() const { return nullptr; }
    // Particle/emitter tables. Default to null so providers that don't emit
    // them (e.g. host-test fakes) keep compiling.
    virtual const ParticleDef* particle(const std::string&) const { return nullptr; }
    virtual const EmitterDef* emitter(const std::string&) const { return nullptr; }
    virtual const PlayerPose* playerPose(const std::string&) const { return nullptr; }
    virtual const PlayerAnimationDef* playerAnimation(const std::string&) const { return nullptr; }
    // GameConfig plane tuning (plane.ts) and bullet tracer colours (shot/flare).
    virtual const PlaneDef* plane(int /*type*/) const { return nullptr; }
    virtual const TracerColorDef* tracerColor(const std::string&) const { return nullptr; }
    virtual const BulletRenderDef* bullet(const std::string&) const { return nullptr; }
    // client/src/objects/explosion.ts ExplosionEffectDefs.
    virtual const ExplosionEffectRenderDef* explosionEffect(const std::string&) const { return nullptr; }
    // shared/defs/gameObjects/emoteDefs.ts (world emotes + emote wheel).
    virtual const EmoteRenderDef* emote(const std::string& /*type*/) const { return nullptr; }
    // shared/defs/gameObjects/pingDefs.ts (team pings + minimap markers).
    virtual const PingRenderDef* ping(const std::string& /*type*/) const { return nullptr; }
    // shared/defs/gameObjects/roleDefs.ts (role map icons).
    virtual const RoleRenderDef* role(const std::string& /*type*/) const { return nullptr; }
};

// Process-wide provider, installed by the app/generated code. May be null.
const DefProvider* getDefProvider();
void setDefProvider(const DefProvider* provider);

} // namespace surv
