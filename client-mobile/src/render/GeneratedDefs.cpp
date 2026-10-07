// GENERATED FILE - do not edit. Run tools/codegen_render_defs.mjs.
#include "Defs.h"
#include <string>
#include <unordered_map>

namespace surv {
namespace {

struct RawCollider { int type; float a, b, c, d; };
struct RawImg { const char* sprite; float scale; float alpha; unsigned tint; int zIdx; int mirrorX; int mirrorY; float ori; };
struct RawBImg { const char* sprite; float scale; float alpha; unsigned tint; float px, py, rot; int mirrorX, mirrorY, removeOnDamaged; };
struct RawBEmitter { const char* type; float px, py, rot, scale; int layer, parentToCeiling; float dirx, diry; };
struct RawLayer { const char* type; float x, y; int ori; int inheritOri; int underground; };
struct RawStair { RawCollider col; float dx, dy; int noCeilingReveal; int lootOnly; };
struct RawShape { RawCollider col; float scale; unsigned color; };
struct RawMapObj {
    const char* type; RawImg img; int hasImg; RawCollider collision; int hasCollision;
    int isDoor, isButton, isTree, isWall; int randomRotation, hasExplosion; const char* explosionParticle;
    float doorSlideOffset;
    const char* doorCasingSprite; float doorCasingPx, doorCasingPy, doorCasingScale; unsigned doorCasingTint; float doorCasingAlpha;
    float doorSpriteAnchorX, doorSpriteAnchorY;
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
};
struct RawMapRender {
    const char* name;
    unsigned background, beach, grass, riverbank, water, waterRipple, lakeRiverbank, lakeWater, lakeWaterRipple, underground;
    int faction, potato, perk, turkey; float valueAdjust; const char* cameraEmitter;
    const char* const* atlases; int atlasCount;
    const char* ambMusic; const char* ambWind; const char* ambRiver; const char* ambWaves;
};
struct RawGameObj { const char* type; const char* category; RawImg img; int hasImg; const char* emitter; const char* auraSprite; unsigned auraTint; int hasAura; };
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

static const RawShape kShapes_741[] = {
    {{1,-26.75,-4,-5.25,18},1,7820585},
    {{1,-5.5,-18.25,17.5,18.25},1,9989427},
    {{1,17.5,-3.5,26.5,11.5},1,7820585},
};
static const RawBImg kFloorImgs_741[] = {
    {"map-building-bank-floor-01.img",0.5,1,16777215,0,6.96,0,0,0,0},
    {"map-building-bank-floor-02.img",0.5,1,16777215,9.5,-12.5,0,0,0,0},
};
static const RawBImg kCeilImgs_741[] = {
    {"map-building-bank-ceiling-01.img",0.667,1,16777215,-16,7,0,0,0,0},
    {"map-building-bank-ceiling-02.img",0.667,1,16777215,6,0,0,0,0,0},
    {"map-building-bank-ceiling-03.img",0.667,1,16777215,22,8,0,0,0,0},
};
static const RawCollider kZoomIns_741[] = {
    {1,-5.25,-19.25,17.25,17.25},
    {1,16.75,-3.25,26.25,11.25},
    {1,-25.75,-5,-4.25,17},
};
static const RawBImg kFloorImgs_742[] = {
    {"map-building-barn-basement-stairs.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_743[] = {
    {"map-building-barn-basement-floor-01.img",0.5,1,16777215,5.5,-0.5,0,0,0,0},
};
static const RawBImg kCeilImgs_743[] = {
    {"map-building-barn-basement-ceiling-01.img",1,1,6182731,5,0,0,0,0,0},
};
static const RawCollider kZoomIns_743[] = {
    {1,-4,-7,8,7},
    {1,7.5,-5.5,14.5,-1.5},
};
static const RawBImg kFloorImgs_744[] = {
    {"map-building-barn-basement-floor-02.img",0.5,1,16777215,-2,-0.5,0,0,0,0},
};
static const RawBImg kCeilImgs_744[] = {
    {"map-building-barn-basement-ceiling-02.img",1,1,6182731,-1.4,0,0,0,0,0},
};
static const RawCollider kZoomIns_744[] = {
    {1,-6,-7,4,5},
};
static const RawShape kShapes_745[] = {
    {{1,-5,10,5,14},1,12300935},
    {{1,-24.5,-14.8,24.5,10.8},1,3816739},
};
static const RawBImg kFloorImgs_745[] = {
    {"map-building-barn-floor-01.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_745[] = {
    {"map-building-barn-ceiling-01.img",1,1,16777215,0,-2,0,0,0,0},
    {"map-building-barn-ceiling-02.img",0.667,1,16777215,0,13.2,0,0,0,0},
};
static const RawCollider kZoomIns_745[] = {
    {1,-24.5,-14.8,24.5,10.8},
    {1,-5.5,9.5,5.5,14.5},
};
static const RawShape kShapes_746[] = {
    {{1,-5,10,5,14},1,12300935},
    {{1,-24.5,-14.8,24.5,10.8},1,3816739},
};
static const RawBImg kFloorImgs_746[] = {
    {"map-building-barn-floor-01.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_746[] = {
    {"map-building-barn-ceiling-01.img",1,1,16777215,0,-2,0,0,0,0},
    {"map-building-barn-ceiling-02.img",0.667,1,16777215,0,13.2,0,0,0,0},
};
static const RawCollider kZoomIns_746[] = {
    {1,-24.5,-14.8,24.5,10.8},
    {1,-5.5,9.5,5.5,14.5},
};
static const RawBImg kFloorImgs_747[] = {
    {"map-building-club-gradient-01.img",4,1,16777215,-3.5,-13.5,0,0,0,0},
    {"map-building-bathhouse-basement-01a.img",0.5,1,16777215,-33.5,-26,0,0,0,0},
    {"map-building-bathhouse-basement-01b.img",0.5,1,16777215,-10,-26.5,0,0,0,0},
    {"map-building-bathhouse-basement-01c.img",0.5,1,16777215,18.5,-35.5,0,0,0,0},
    {"map-building-bathhouse-basement-01d.img",0.5,1,16777215,23.02,-27.5,0,0,0,0},
    {"map-building-bathhouse-basement-01e.img",0.5,1,16777215,2,9,0,0,0,0},
};
static const RawBEmitter kEmitters_747[] = {
    {"bathhouse_steam",30,0.5,0,1,1,0,-1,0},
    {"bathhouse_steam",-26,16.5,0,1,1,0,1,0},
};
static const RawCollider kZoomIns_747[] = {
    {1,-18,-12.5,22,31.5},
    {1,-26,-40.5,26,55.5},
    {1,-30,-29,-22,-23},
    {1,22.5,-6,37.5,7},
    {1,-33.5,10,-18.5,23},
};
static const RawBImg kFloorImgs_748[] = {
    {"map-building-bathhouse-sideroom-01.img",0.5,1,16777215,-1,0,0,0,0,0},
};
static const RawBImg kCeilImgs_748[] = {
    {"map-building-bathhouse-sideroom-ceiling-01.img",1,1,16777215,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_748[] = {
    {1,-7.5,-6.5,7.5,6.5},
};
static const RawBImg kFloorImgs_749[] = {
    {"map-building-bathhouse-sideroom-02.img",0.5,1,16777215,0,0.5,0,0,0,0},
};
static const RawBImg kCeilImgs_749[] = {
    {"map-building-bathhouse-sideroom-ceiling-02.img",1,1,4931116,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_749[] = {
    {1,-14,-9.5,14,9.5},
};
static const RawShape kShapes_750[] = {
    {{1,-31.5,-8,31.5,8},1,5197647},
    {{1,-16.5,-11,-11.5,-8},1,3618615},
    {{1,11.5,-11,16.5,-8},1,3618615},
    {{1,-16.5,8,-11.5,11},1,3618615},
    {{1,11.5,8,16.5,11},1,3618615},
};
static const RawBImg kFloorImgs_750[] = {
    {"map-building-bridge-lg-floor.img",0.5,1,16777215,-15.75,0,0,0,0,0},
    {"map-building-bridge-lg-floor.img",0.5,1,16777215,15.75,0,2,0,1,0},
};
static const RawBImg kCeilImgs_750[] = {
    {"map-building-bridge-lg-ceiling.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_750[] = {
    {1,-16.5,-7,16.5,7},
};
static const RawShape kShapes_752[] = {
    {{1,-38.5,-12,38.5,12},1,2894124},
    {{1,-19,-14.5,-13,-11.5},1,3618615},
    {{1,13,-14.5,19,-11.5},1,3618615},
    {{1,-19,11.5,-13,14.5},1,3618615},
    {{1,13,11.5,19,14.5},1,3618615},
};
static const RawBImg kFloorImgs_752[] = {
    {"map-building-bridge-xlg-floor.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawShape kShapes_754[] = {
    {{1,-14,-3.5,14,3.5},1,9322264},
};
static const RawBImg kFloorImgs_754[] = {
    {"map-building-bridge-md-floor.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawShape kShapes_756[] = {
    {{1,-18,-11.5,18,12.5},1,3823128},
    {{1,-17,-15,17,-11},1,6368528},
};
static const RawBImg kFloorImgs_756[] = {
    {"map-building-cabin-floor.img",0.5,1,16777215,0,-1,0,0,0,0},
};
static const RawBImg kCeilImgs_756[] = {
    {"map-building-cabin-ceiling-01a.img",0.667,1,16777215,0,0.5,0,0,0,0},
    {"map-building-cabin-ceiling-01b.img",0.667,1,16777215,4,-13,0,0,0,0},
    {"map-chimney-01.img",0.5,1,16777215,13,2,0,0,0,1},
};
static const RawBEmitter kEmitters_756[] = {
    {"cabin_smoke_parent",0,0,0,1,0,1,1,0},
};
static const RawCollider kZoomIns_756[] = {
    {1,-19,-11.5,19,12.5},
    {1,1,-15,7,-11},
};
static const RawShape kShapes_757[] = {
    {{1,-32.5,-11,-26,-6},1,13022098},
    {{1,-26,-21.75,18,4.75},1,5900046},
    {{1,-6,9.75,18,26.25},1,5900046},
    {{1,14,10,18,23},1,5900046},
    {{1,4,4,9,10},1,5900046},
    {{1,17.75,-14.5,29.25,-0.5},1,5900046},
    {{1,-6.5,-28,9.5,-21},1,5900046},
    {{1,-6,26.25,3,35.25},1,5900046},
    {{1,-24,4,-19,10},1,5900046},
};
static const RawBImg kFloorImgs_757[] = {
    {"map-building-club-floor-01a.img",0.5,1,16777215,-30,-8.5,0,0,0,0},
    {"map-building-club-floor-01b.img",0.5,1,16777215,-21.5,8,0,0,0,0},
    {"map-building-club-floor-01c.img",0.5,1,16777215,-4,-8.5,0,0,0,0},
    {"map-building-club-floor-01d.img",0.5,1,16777215,1.5,-25,0,0,0,0},
    {"map-building-club-floor-01e.img",0.5,1,16777215,24,-7.5,0,0,0,0},
    {"map-building-club-floor-01f.img",0.5,1,16777215,6.5,7,0,0,0,0},
    {"map-building-club-floor-01g.img",0.5,1,16777215,6,18,0,0,0,0},
    {"map-building-club-floor-01h.img",0.5,1,16777215,-1.5,31.5,0,0,0,0},
};
static const RawBImg kCeilImgs_757[] = {
    {"map-building-club-ceiling-01a.img",1,1,16777215,-4.5,-8.5,0,0,0,0},
    {"map-building-club-ceiling-01b.img",1,1,16777215,24,-7.5,0,0,0,0},
    {"map-building-club-ceiling-01c.img",1,1,16777215,6,22.5,0,0,0,0},
};
static const RawCollider kZoomIns_757[] = {
    {1,-26,-21.75,18,4.75},
    {1,-6,9.75,15,26.25},
    {1,14,9.75,18,23.25},
    {1,4,4,9,10},
    {1,-4.5,26,1.5,34},
    {1,17.75,-5.5,29.25,-0.5},
    {1,24,-14.5,29,-0.5},
    {1,-3.5,-27,6.5,-21},
    {1,-24,4,-19,10},
};
static const RawBImg kFloorImgs_758[] = {
    {"",0.5,1,6250335,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_758[] = {
    {"map-building-club-vault-ceiling.img",1,1,6250335,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_758[] = {
    {1,-3,-4,3,4},
};
static const RawBImg kFloorImgs_760[] = {
    {"map-building-container-floor-01.img",0.5,1,2703694,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_760[] = {
    {"map-building-container-ceiling-01.img",0.5,1,2703694,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_760[] = {
    {1,-2.5,-3.25,2.5,7.75},
};
static const RawBImg kFloorImgs_761[] = {
    {"map-building-container-floor-01.img",0.5,1,2703694,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_761[] = {
    {"map-building-container-ceiling-02.img",0.5,1,2703694,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_761[] = {
    {1,-2.5,-3.25,2.5,7.75},
};
static const RawBImg kFloorImgs_762[] = {
    {"map-building-container-floor-01.img",0.5,1,2703694,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_762[] = {
    {"map-building-container-ceiling-03.img",0.5,1,2703694,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_762[] = {
    {1,-2.5,-3.25,2.5,7.75},
};
static const RawBImg kFloorImgs_763[] = {
    {"map-building-container-open-floor.img",0.5,1,3560807,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_763[] = {
    {"map-building-container-open-ceiling-01.img",0.5,1,3560807,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_763[] = {
    {1,-2.5,-5.75,2.5,5.75},
};
static const RawBImg kFloorImgs_764[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_764[] = {
    {"map-building-container-ceiling-05.img",0.5,1,11485762,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_764[] = {
    {1,-2.5,-3.35,2.5,8.15},
};
static const RawBImg kFloorImgs_765[] = {
    {"map-building-container-floor-01.img",0.5,1,12227840,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_765[] = {
    {"map-building-container-ceiling-01.img",0.5,1,12227840,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_765[] = {
    {1,-2.5,-3.25,2.5,7.75},
};
static const RawShape kShapes_766[] = {
    {{1,0.10000000000000009,-10.25,4.9,10.25},1,8862486},
    {{1,-5.050000000000001,5.25,0.1499999999999999,10.25},1,8862486},
};
static const RawBImg kFloorImgs_766[] = {
    {"map-building-dock-floor-01a.img",0.5,1,16777215,-2.5,7.85,0,0,0,0},
    {"map-building-dock-floor-01b.img",0.5,1,16777215,2.5,0,0,0,0,0},
};
static const RawBImg kFloorImgs_767[] = {
    {"map-building-greenhouse-floor-01.img",0.5,1,16777215,0,10,2,0,0,0},
    {"map-building-greenhouse-floor-01.img",0.5,1,16777215,0,-10,0,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,0,21,0,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,0,-21,2,0,0,0},
};
static const RawBImg kCeilImgs_767[] = {
    {"map-building-greenhouse-ceiling-01.img",1,1,16777215,0,-9.85,0,0,0,0},
    {"map-building-greenhouse-ceiling-01.img",1,1,16777215,0,9.85,0,0,1,0},
};
static const RawCollider kZoomIns_767[] = {
    {1,-12.5,-19.5,12.5,19.5},
};
static const RawBImg kFloorImgs_768[] = {
    {"map-building-greenhouse-floor-02.img",0.5,1,16777215,0,10,2,0,0,0},
    {"map-building-greenhouse-floor-02.img",0.5,1,16777215,0,-10,0,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,0,21,0,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,0,-21,2,0,0,0},
};
static const RawBImg kCeilImgs_768[] = {
    {"map-building-greenhouse-ceiling-02.img",1,1,16777215,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_768[] = {
    {1,-12.5,-19.5,12.5,19.5},
};
static const RawBImg kFloorImgs_769[] = {
    {"map-hedgehog-01.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawShape kShapes_770[] = {
    {{1,-7,-7,7,7},1,15181895},
    {{1,-2,-30.9,2,-6.899999999999999},1,6171907},
};
static const RawBImg kFloorImgs_770[] = {
    {"map-building-hut-floor-01.img",0.5,1,16777215,0,0,0,0,0,0},
    {"map-building-hut-floor-02.img",0.5,1,16777215,0,-18.9,0,0,0,0},
};
static const RawBImg kCeilImgs_770[] = {
    {"map-building-hut-ceiling-01.img",0.667,1,16777215,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_770[] = {
    {1,-6,-6,6,6},
};
static const RawShape kShapes_771[] = {
    {{1,-7,-7,7,7},1,15181895},
    {{1,-2,-30.9,2,-6.899999999999999},1,6171907},
};
static const RawBImg kFloorImgs_771[] = {
    {"map-building-hut-floor-01.img",0.5,1,16777215,0,0,0,0,0,0},
    {"map-building-hut-floor-02.img",0.5,1,16777215,0,-18.9,0,0,0,0},
};
static const RawBImg kCeilImgs_771[] = {
    {"map-building-hut-ceiling-02.img",0.667,1,16777215,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_771[] = {
    {1,-6,-6,6,6},
};
static const RawShape kShapes_772[] = {
    {{1,-7,-7,7,7},1,7771201},
    {{1,-2,-30.9,2,-6.899999999999999},1,6171907},
};
static const RawBImg kFloorImgs_772[] = {
    {"map-building-hut-floor-01.img",0.5,1,16777215,0,0,0,0,0,0},
    {"map-building-hut-floor-02.img",0.5,1,16777215,0,-18.9,0,0,0,0},
};
static const RawBImg kCeilImgs_772[] = {
    {"map-building-hut-ceiling-03.img",0.667,1,16777215,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_772[] = {
    {1,-6,-6,6,6},
};
static const RawBImg kFloorImgs_773[] = {
    {"map-building-house-floor-01.img",0.5,1,16777215,0,0,0,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,-1,14.5,0,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,0,-14.5,2,0,0,0},
};
static const RawBImg kCeilImgs_773[] = {
    {"map-building-house-ceiling.img",0.667,1,16777215,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_773[] = {
    {1,-14.5,-13,14.5,13},
};
static const RawBImg kFloorImgs_774[] = {
    {"map-building-house-floor-02.img",0.5,1,16777215,0,0,0,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,10,14.5,0,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,0,-14.5,2,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,2.6,-14.5,2,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,5.2,-14.5,2,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,7.8,-14.5,2,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,0,-16.25,2,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,2.6,-16.25,2,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,5.2,-16.25,2,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,7.8,-16.25,2,0,0,0},
};
static const RawBImg kCeilImgs_774[] = {
    {"map-building-house-ceiling.img",0.667,1,13619151,0,0,2,0,0,0},
};
static const RawCollider kZoomIns_774[] = {
    {1,-14.5,-13,14.5,13},
};
static const RawShape kShapes_775[] = {
    {{1,-14,16,11,25},1,8671554},
    {{1,-5,-25.5,1,-20.5},1,8671554},
    {{1,-30.5,-24.5,-10.5,-20.5},1,7750457},
    {{1,24.25,-1.5,31.75,4.5},1,7237230},
    {{1,-31.5,-20.5,24.5,16.5},1,6175023},
};
static const RawBImg kFloorImgs_775[] = {
    {"map-building-mansion-floor-01a.img",0.5,1,16777215,-1.5,22,0,0,0,0},
    {"map-building-mansion-floor-01b.img",0.5,1,16777215,-3.5,-2,0,0,0,0},
    {"map-building-mansion-floor-01c.img",0.5,1,16777215,28.5,1.5,0,0,0,0},
    {"map-building-mansion-floor-01d.img",0.5,1,16777215,-15,-24,0,0,0,0},
};
static const RawBImg kCeilImgs_775[] = {
    {"map-building-mansion-ceiling.img",1,1,16777215,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_775[] = {
    {1,-32,-24.599999999999998,2,-20.2},
    {1,-31.5,-20.5,24.5,16.5},
    {1,-13.5,16.400000000000002,10.5,24.8},
};
static const RawBImg kFloorImgs_776[] = {
    {"map-building-mansion-gradient-01.img",4,1,16777215,-3.75,0.25,0,0,0,0},
    {"map-building-mansion-cellar-01a.img",0.5,1,16777215,11.5,5.5,0,0,0,0},
    {"map-building-mansion-cellar-01b.img",0.5,1,16777215,28.5,1.5,0,0,0,0},
    {"map-building-mansion-cellar-01c.img",0.5,1,16777215,11.5,-9,0,0,0,0},
};
static const RawCollider kZoomIns_776[] = {
    {1,11,-10,25,16},
    {1,-1,-10.5,11,13.5},
};
static const RawBImg kFloorImgs_777[] = {
    {"map-building-outhouse-floor.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_777[] = {
    {"map-building-outhouse-ceiling.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_777[] = {
    {1,-3.6,-1.7500000000000002,3.6,4.65},
};
static const RawBImg kFloorImgs_778[] = {
    {"map-building-panicroom-floor.img",0.5,1,6250335,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_778[] = {
    {"map-building-panicroom-ceiling.img",0.5,1,6250335,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_778[] = {
    {1,-4.5,-6,4.5,6},
};
static const RawShape kShapes_779[] = {
    {{1,-42.25,-22,0.25,6},1,5855577},
    {{1,-42.25,-1.25,-6.75,18.25},1,3355970},
    {{1,-7,5.75,0,18.25},1,4278620},
    {{1,-0.15000000000000036,-22,20.85,22},1,3355970},
    {{1,20.5,3,42,22},1,3355970},
    {{1,-5.75,0.25,-1.25,4.75},1,6310464},
    {{0,-30.5,-18,1.5,0},1,8026746},
    {{0,-20.5,-10.5,1.5,0},1,8026746},
    {{1,-39.9,-10.1,-37.1,-3.9},1,13278307},
    {{1,-10.6,-20.9,-4.4,-18.1},1,13278307},
};
static const RawBImg kFloorImgs_779[] = {
    {"map-building-police-floor-01.img",0.5,1,16777215,-9.5,0,0,0,0,0},
    {"map-building-police-floor-02.img",0.5,1,16777215,33,0,0,0,0,0},
};
static const RawBImg kCeilImgs_779[] = {
    {"map-building-police-ceiling-01.img",0.667,1,16777215,-21.5,8.5,0,0,0,0},
    {"map-building-police-ceiling-02.img",0.667,1,16777215,10.5,0,0,0,0,0},
    {"map-building-police-ceiling-03.img",0.667,1,16777215,31.96,12.5,0,0,0,0},
};
static const RawCollider kZoomIns_779[] = {
    {1,-42.25,-1.25,-6.75,18.25},
    {1,-7,5.75,0,18.25},
    {1,-0.15000000000000036,-22,20.85,22},
    {1,20.5,3,42,22},
};
static const RawBImg kFloorImgs_780[] = {
    {"map-building-saferoom-floor.img",0.5,1,6250335,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_780[] = {
    {"map-building-saferoom-ceiling.img",0.5,1,6250335,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_780[] = {
    {1,-5,-3,5,3},
};
static const RawBImg kFloorImgs_781[] = {
    {"map-building-shack-floor-01.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_781[] = {
    {"map-building-shack-ceiling-01.img",0.667,1,16777215,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_781[] = {
    {1,-5.6,-2.6,5.6,4.4},
};
static const RawBImg kFloorImgs_782[] = {
    {"map-building-shack-floor-02.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_782[] = {
    {"map-building-shack-ceiling-02.img",0.667,1,16777215,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_782[] = {
    {1,-4.75,-2.75,4.75,4.75},
};
static const RawShape kShapes_783[] = {
    {{1,-8.75,1,-6.75,5},1,6171907},
    {{1,3,-5.75,7,-3.75},1,6171907},
    {{1,-7,-4,9,7},1,3754050},
    {{1,-12.65,-5,-8.65,19},1,6171907},
};
static const RawBImg kFloorImgs_783[] = {
    {"map-building-shack-floor-03.img",0.5,1,16777215,0,0,0,0,0,0},
    {"map-building-hut-floor-02.img",0.5,1,16777215,-10.65,7,0,0,0,0},
};
static const RawBImg kCeilImgs_783[] = {
    {"map-building-shack-ceiling-03.img",0.667,1,10461087,0.5,0.5,0,0,0,0},
};
static const RawCollider kZoomIns_783[] = {
    {1,-6.75,-3.75,8.75,6.75},
};
static const RawShape kShapes_784[] = {
    {{1,-8.75,1,-6.75,5},1,6171907},
    {{1,3,-5.75,7,-3.75},1,6171907},
    {{1,-7,-4,9,7},1,5730406},
    {{1,-12.65,-3,-8.65,21},1,6171907},
};
static const RawBImg kFloorImgs_784[] = {
    {"map-building-shack-floor-03.img",0.5,1,16777215,0,0,0,0,0,0},
    {"map-building-hut-floor-02.img",0.5,1,16777215,-10.65,9,0,0,0,0},
};
static const RawBImg kCeilImgs_784[] = {
    {"map-building-shack-ceiling-03.img",0.667,1,16777215,0.5,0.5,0,0,0,0},
};
static const RawCollider kZoomIns_784[] = {
    {1,-6.75,-3.75,8.75,6.75},
};
static const RawShape kShapes_785[] = {
    {{1,-14,-9,14,9},1,4608356},
    {{1,-7.5,-3.75,7.5,3.75},1,5793921},
    {{1,7,-11.65,11,-8.65},1,7354635},
    {{1,-11,8.65,-7,11.65},1,7354635},
};
static const RawBImg kFloorImgs_785[] = {
    {"map-building-teahouse-floor-01.img",0.5,1,16777215,0,0,0,0,0,0},
    {"map-building-teahouse-floor-02.img",0.5,1,16777215,9,-10.25,0,0,0,0},
    {"map-building-teahouse-floor-02.img",0.5,1,16777215,-9,10.25,2,0,0,0},
};
static const RawBImg kCeilImgs_785[] = {
    {"map-building-teahouse-ceiling-01.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_785[] = {
    {1,-12,-7,12,7},
};
static const RawShape kShapes_787[] = {
    {{1,24,-12.25,30,12.25},1,10066329},
    {{1,-30,-12.25,-24,12.25},1,10066329},
    {{1,-24.5,-12.25,24.5,12.25},1,5915450},
};
static const RawBImg kFloorImgs_787[] = {
    {"map-building-warehouse-floor-01.img",0.5,1,16777215,-15.615,0,0,0,0,0},
    {"map-building-warehouse-floor-01.img",0.5,1,16777215,15.615,0,2,0,0,0},
};
static const RawBImg kCeilImgs_787[] = {
    {"map-building-warehouse-ceiling-01.img",1,1,16777215,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_787[] = {
    {1,-24.5,-12.25,24.5,12.25},
};
static const RawShape kShapes_788[] = {
    {{1,22,-12.25,28,12.25},1,10066329},
    {{1,-28,-12.25,-22,12.25},1,10066329},
    {{1,-22.5,-12.25,22.5,12.25},1,2240064},
};
static const RawBImg kFloorImgs_788[] = {
    {"map-building-warehouse-floor-02.img",0.5,1,16777215,-13.72,0,0,0,0,0},
    {"map-building-warehouse-floor-02.img",0.5,1,16777215,13.72,0,2,0,0,0},
};
static const RawBImg kCeilImgs_788[] = {
    {"map-building-warehouse-ceiling-02.img",1,1,16777215,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_788[] = {
    {1,-22,-12.25,22,12.25},
};
static const RawShape kShapes_789[] = {
    {{1,24,-12.25,30,12.25},1,10066329},
    {{1,-30,-12.25,-24,12.25},1,10066329},
    {{1,-24.5,-12.25,24.5,12.25},1,5915450},
};
static const RawBImg kFloorImgs_789[] = {
    {"map-building-warehouse-floor-03.img",0.5,1,16777215,-15.615,0,0,0,0,0},
    {"map-building-warehouse-floor-01.img",0.5,1,16777215,15.615,0,2,0,0,0},
};
static const RawBImg kCeilImgs_789[] = {
    {"map-building-warehouse-ceiling-01.img",1,1,16777215,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_789[] = {
    {1,-24.5,-12.25,24.5,12.25},
};
static const RawShape kShapes_790[] = {
    {{1,-21,63,73,78},1,5855577},
    {{1,-42,42,73,63},1,5855577},
    {{1,-42,-20,108,42},1,5855577},
    {{1,-42,-40,52,-20},1,5855577},
    {{1,-41.75,-39.75,-37.75,62.25},1,16109568},
    {{0,-39,55,1.25,0},1,6310464},
    {{0,-39,20.5,1.25,0},1,6310464},
    {{0,-39,2,1.25,0},1,6310464},
    {{0,-39,-31.5,1.25,0},1,6310464},
    {{1,-30,-32,-26,-28},1,6697728},
    {{1,-25,-35,-21,-31},1,6697728},
    {{1,5,68,9,72},1,6697728},
    {{1,10,70,14,74},1,6697728},
    {{0,-26.5,54.75,1.75,0},1,8026746},
    {{0,-23.5,57,1.75,0},1,8026746},
    {{0,84,-15.5,1.75,0},1,8026746},
    {{0,40,-35,1.5,0},1,8026746},
    {{0,65,61,1.5,0},1,8026746},
    {{1,43.1,-28.1,45.9,-21.9},1,13278307},
    {{1,56.6,44.4,59.4,50.6},1,13278307},
};
static const RawBImg kFloorImgs_790[] = {
    {"map-complex-warehouse-floor-01.img",1,1,16777215,-39.2,55,0,0,0,0},
    {"map-complex-warehouse-floor-02.img",1,1,16777215,-39.2,11.5,0,0,0,0},
    {"map-complex-warehouse-floor-03.img",1,1,16777215,-39.2,-32,0,0,0,0},
};
static const RawBImg kFloorImgs_791[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_791[] = {
    {"map-building-vault-ceiling.img",1,1,6250335,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_791[] = {
    {1,-12.75,-10.5,5.75,10.5},
};
static const RawShape kShapes_792[] = {
    {{1,-7,-7,7,7},1,15181895},
    {{1,-2,-30.9,2,-6.899999999999999},1,6171907},
};
static const RawBImg kFloorImgs_792[] = {
    {"map-building-hut-floor-01.img",0.5,1,16777215,0,0,0,0,0,0},
    {"map-building-hut-floor-02.img",0.5,1,16777215,0,-18.9,0,0,0,0},
};
static const RawBImg kCeilImgs_792[] = {
    {"map-building-hut-ceiling-01.img",0.667,1,16777215,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_792[] = {
    {1,-6,-6,6,6},
};
static const RawShape kShapes_793[] = {
    {{1,-15.25,-6.25,9.75,10.75},1,15181895},
    {{1,-13.25,-10.75,9.75,-6.25},1,6171907},
    {{1,9.7,-10.75,15.3,8.75},1,6171907},
    {{1,-2,-34.5,2,-10.5},1,6171907},
};
static const RawBImg kFloorImgs_793[] = {
    {"map-building-hut-floor-03.img",0.5,1,16777215,0,0,0,0,0,0},
    {"map-building-hut-floor-02.img",0.5,1,16777215,0,-22.75,0,0,0,0},
};
static const RawBImg kCeilImgs_793[] = {
    {"map-building-hut-ceiling-04.img",0.5,1,16777215,-2,2,0,0,0,0},
};
static const RawCollider kZoomIns_793[] = {
    {1,-14.25,-5.25,8.75,9.75},
};
static const RawShape kShapes_794[] = {
    {{1,-14,16,11,25},1,8671554},
    {{1,-5,-25.5,1,-20.5},1,8671554},
    {{1,-30.5,-24.5,-10.5,-20.5},1,7750457},
    {{1,24.25,-1.5,31.75,4.5},1,7237230},
    {{1,-31.5,-20.5,24.5,16.5},1,6175023},
};
static const RawBImg kFloorImgs_794[] = {
    {"map-building-mansion-floor-01a.img",0.5,1,16777215,-1.5,22,0,0,0,0},
    {"map-building-mansion-floor-01b.img",0.5,1,16777215,-3.5,-2,0,0,0,0},
    {"map-building-mansion-floor-01c.img",0.5,1,16777215,28.5,1.5,0,0,0,0},
    {"map-building-mansion-floor-01d.img",0.5,1,16777215,-15,-24,0,0,0,0},
};
static const RawBImg kCeilImgs_794[] = {
    {"map-building-mansion-ceiling.img",1,1,16777215,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_794[] = {
    {1,-32,-24.599999999999998,2,-20.2},
    {1,-31.5,-20.5,24.5,16.5},
    {1,-13.5,16.400000000000002,10.5,24.8},
};
static const RawBImg kFloorImgs_795[] = {
    {"map-building-mansion-gradient-01.img",4,1,16777215,-3.75,0.25,0,0,0,0},
    {"map-building-mansion-cellar-01a.img",0.5,1,16777215,11.5,5.5,0,0,0,0},
    {"map-building-mansion-cellar-01b.img",0.5,1,16777215,28.5,1.5,0,0,0,0},
    {"map-building-mansion-cellar-01c.img",0.5,1,16777215,11.5,-9,0,0,0,0},
};
static const RawCollider kZoomIns_795[] = {
    {1,11,-10,25,16},
    {1,-1,-10.5,11,13.5},
};
static const RawBImg kCeilImgs_797[] = {
    {"map-building-archway-ceiling-01.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawShape kShapes_798[] = {
    {{1,-26.75,-4,-5.25,18},1,7820585},
    {{1,-5.5,-18.25,17.5,18.25},1,9989427},
    {{1,17.5,-3.5,26.5,11.5},1,7820585},
};
static const RawBImg kFloorImgs_798[] = {
    {"map-building-bank-floor-01.img",0.5,1,16777215,0,6.96,0,0,0,0},
    {"map-building-bank-floor-02.img",0.5,1,16777215,9.5,-12.5,0,0,0,0},
};
static const RawBImg kCeilImgs_798[] = {
    {"map-building-bank-ceiling-01.img",0.667,1,16777215,-16,7,0,0,0,0},
    {"map-building-bank-ceiling-02.img",0.667,1,16777215,6,0,0,0,0,0},
    {"map-building-bank-ceiling-03.img",0.667,1,16777215,22,8,0,0,0,0},
};
static const RawCollider kZoomIns_798[] = {
    {1,-5.25,-19.25,17.25,17.25},
    {1,16.75,-3.25,26.25,11.25},
    {1,-25.75,-5,-4.25,17},
};
static const RawBImg kFloorImgs_799[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_799[] = {
    {"map-building-vault-ceiling.img",1,1,6250335,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_799[] = {
    {1,-12.75,-10.5,5.75,10.5},
};
static const RawBImg kFloorImgs_800[] = {
    {"map-building-barn-basement-floor-01.img",0.5,1,16777215,5.5,-0.5,0,0,0,0},
};
static const RawBImg kCeilImgs_800[] = {
    {"map-building-barn-basement-ceiling-01.img",1,1,6182731,5,0,0,0,0,0},
};
static const RawCollider kZoomIns_800[] = {
    {1,-4,-7,8,7},
    {1,7.5,-5.5,14.5,-1.5},
};
static const RawBImg kFloorImgs_801[] = {
    {"map-building-barn-basement-floor-02.img",0.5,1,16777215,-2,-0.5,0,0,0,0},
};
static const RawBImg kCeilImgs_801[] = {
    {"map-building-barn-basement-ceiling-02.img",1,1,6182731,-1.4,0,0,0,0,0},
};
static const RawCollider kZoomIns_801[] = {
    {1,-6,-7,4,5},
};
static const RawShape kShapes_802[] = {
    {{1,-5,10,5,14},1,12300935},
    {{1,-24.5,-14.8,24.5,10.8},1,3816739},
};
static const RawBImg kFloorImgs_802[] = {
    {"map-building-barn-floor-01.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_802[] = {
    {"map-building-barn-ceiling-01.img",1,1,16777215,0,-2,0,0,0,0},
    {"map-building-barn-ceiling-02.img",0.667,1,16777215,0,13.2,0,0,0,0},
};
static const RawCollider kZoomIns_802[] = {
    {1,-24.5,-14.8,24.5,10.8},
    {1,-5.5,9.5,5.5,14.5},
};
static const RawBImg kFloorImgs_806[] = {
    {"map-complex-warehouse-floor-05.img",1,1,16777215,81,10,0,0,0,0},
};
static const RawShape kShapes_807[] = {
    {{1,-30,-18.5,30,23.5},1,2499104},
    {{1,-58,-23.5,-30,26.5},1,2499104},
    {{1,-30,22.5,-15,26.5},1,2499104},
    {{1,30,-23.5,59,23.5},1,2499104},
    {{1,-19,-36.5,19,-18.5},1,4337194},
    {{1,39,23.5,59,27.5},1,4337194},
};
static const RawBImg kFloorImgs_807[] = {
    {"map-building-reserve-floor-04.img",0.5,1,16777215,0,-25.5,0,0,0,0},
    {"map-building-reserve-floor-01.img",0.5,1,16777215,-37,2,0,0,0,0},
    {"map-building-reserve-floor-02.img",0.5,1,16777215,7,2.5,0,0,0,0},
    {"map-building-reserve-floor-03.img",0.5,1,16777215,44,2,0,0,0,0},
};
static const RawBImg kCeilImgs_807[] = {
    {"map-building-reserve-ceiling-01.img",1,1,16777215,-44,2,0,0,0,0},
    {"map-building-reserve-ceiling-02.img",1,1,16777215,0,4.5,0,0,0,0},
    {"map-building-reserve-ceiling-03.img",1,1,16777215,44,2.5,0,0,0,0},
    {"map-building-reserve-ceiling-04.img",1,1,16777215,0,-26.5,0,0,0,0},
    {"map-chimney-01.img",0.5,1,16777215,-34.5,11,0,0,0,1},
};
static const RawBEmitter kEmitters_807[] = {
    {"cabin_smoke_parent",0,0,0,1,0,1,1,0},
};
static const RawCollider kZoomIns_807[] = {
    {1,-58,-22.5,-30,-4.5},
    {1,-58,-4.5,-16,26.5},
    {1,-29,-17.5,-16,-4.5},
    {1,-16,-17.5,30,22.5},
    {1,30,-22.5,58,22.5},
    {1,33,22.5,59,30.5},
    {1,-18,-33.5,18,-17.5},
};
static const RawShape kShapes_808[] = {
    {{1,-40,-30,40,30},1,7820585},
};
static const RawBImg kFloorImgs_808[] = {
    {"map-building-reserve-basement-floor-02.img",0.5,1,16777215,2,1,0,0,0,0},
    {"map-building-reserve-basement-floor-01.img",0.5,1,16777215,-21,26.5,0,0,0,0},
    {"map-building-reserve-basement-floor-03.img",0.5,1,16777215,-21,-2.5,0,0,0,0},
    {"map-building-reserve-basement-floor-04.img",0.5,1,16777215,40.5,-8,0,0,0,0},
    {"map-building-reserve-basement-floor-05.img",0.5,1,16777215,37,11.5,0,0,0,0},
};
static const RawCollider kZoomIns_808[] = {
    {1,-60,-35,60,35},
    {1,-39.5,23,-4.5,31},
    {1,30.5,4.5,43.5,18.5},
    {1,16.5,-20.5,30.5,-8.5},
    {1,30.5,-20.5,56.5,4.5},
    {1,-26.5,-21,3,23},
    {1,3,-21,16.5,-3.5},
};
static const RawBImg kFloorImgs_809[] = {
    {"map-building-reserve-sideroom-01.img",0.5,1,16777215,0,-0.5,0,0,0,0},
};
static const RawBImg kCeilImgs_809[] = {
    {"map-building-reserve-sideroom-ceiling-01.img",1,1,16777215,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_809[] = {
    {1,-8.5,-13,8.5,13},
};
static const RawBImg kFloorImgs_810[] = {
    {"map-building-reserve-sideroom-02.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_810[] = {
    {"map-building-reserve-sideroom-ceiling-02.img",1,1,16777215,0,0.5,0,0,0,0},
};
static const RawCollider kZoomIns_810[] = {
    {1,-14,-4.5,14,5.5},
};
static const RawBImg kFloorImgs_811[] = {
    {"map-building-reserve-vault-01.img",0.5,1,16777215,-1,0,0,0,0,0},
};
static const RawBImg kCeilImgs_811[] = {
    {"map-building-reserve-vault-ceiling-01.img",1,1,6250335,-0.5,0,0,0,0,0},
};
static const RawCollider kZoomIns_811[] = {
    {1,-14,-13,14,13},
};
static const RawShape kShapes_812[] = {
    {{1,-20.5,-20.5,20.5,20.5},1,5252110},
    {{1,-20,-18,18,20},1,4337194},
    {{1,-20,-14,14,20},1,2499104},
    {{1,-26.5,-1,-20.5,3},1,3485483},
};
static const RawBImg kFloorImgs_812[] = {
    {"map-building-saloon-floor-01.img",0.5,1,16777215,0,0,0,0,0,0},
    {"map-building-saloon-ceiling-02.img",0.5,1,16777215,-23.5,1,0,0,0,0},
};
static const RawBImg kCeilImgs_812[] = {
    {"map-building-saloon-ceiling-01.img",1,1,16777215,0,0,0,0,0,0},
    {"map-building-saloon-ceiling-02.img",0.5,1,16777215,-23.5,1,0,0,0,0},
    {"map-chimney-01.img",0.5,1,16777215,-3,3,0,0,0,1},
};
static const RawBEmitter kEmitters_812[] = {
    {"cabin_smoke_parent",0,0,0,1,0,1,1,0},
};
static const RawCollider kZoomIns_812[] = {
    {1,-20,-18,18,20},
};
static const RawBImg kFloorImgs_813[] = {
    {"map-building-saloon-cellar-01.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_813[] = {
    {"",1,1,6250335,-2,3.5,0,0,0,0},
};
static const RawCollider kZoomIns_813[] = {
    {1,-15,-9,15,9},
};
static const RawBImg kFloorImgs_814[] = {
    {"map-bunker-generic-floor-01.img",0.5,1,16777215,0,0,3,0,0,0},
};
static const RawBImg kFloorImgs_815[] = {
    {"map-bunker-statue-chamber-floor-01.img",0.5,1,16777215,3.5,0,3,0,0,0},
};
static const RawBImg kCeilImgs_815[] = {
    {"",1,1,16777215,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_815[] = {
    {1,2.5,-3,10.5,3},
};
static const RawBImg kFloorImgs_816[] = {
    {"map-bunker-generic-floor-01.img",0.5,1,16777215,0,0,3,0,0,0},
};
static const RawBImg kFloorImgs_817[] = {
    {"map-bunker-statue-chamber-floor-01.img",0.5,1,16777215,3.5,0,3,0,0,0},
};
static const RawBImg kCeilImgs_817[] = {
    {"",1,1,16777215,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_817[] = {
    {1,2.5,-3,10.5,3},
};
static const RawShape kShapes_818[] = {
    {{1,40.75,-54,100.75,55},1,3815994},
    {{1,54.5,54,100.5,74},1,3815994},
    {{1,100.5,-54,121.5,-5},1,3815994},
    {{1,45.6,-4.4,54.4,4.4},1,5723991},
};
static const RawBImg kFloorImgs_818[] = {
    {"map-complex-warehouse-floor-04.img",1,1,16777215,81,10,0,0,0,0},
};
static const RawBImg kFloorImgs_819[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_820[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawShape kShapes_821[] = {
    {{1,24,-12.25,30,12.25},1,10066329},
    {{1,-30,-12.25,-24,12.25},1,10066329},
    {{1,-24.5,-12.25,24.5,12.25},1,5915450},
};
static const RawBImg kFloorImgs_821[] = {
    {"map-building-warehouse-floor-01.img",0.5,1,16777215,-15.615,0,0,0,0,0},
    {"map-building-warehouse-floor-01.img",0.5,1,16777215,15.615,0,2,0,0,0},
};
static const RawBImg kCeilImgs_821[] = {
    {"map-building-warehouse-ceiling-01.img",1,1,16777215,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_821[] = {
    {1,-24.5,-12.25,24.5,12.25},
};
static const RawShape kShapes_822[] = {
    {{1,-5,10,5,14},1,12300935},
    {{1,-24.5,-14.8,24.5,10.8},1,3816739},
};
static const RawBImg kFloorImgs_822[] = {
    {"map-building-barn-floor-01.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_822[] = {
    {"map-building-barn-ceiling-01.img",1,1,16777215,0,-2,0,0,0,0},
    {"map-building-barn-ceiling-02.img",0.667,1,16777215,0,13.2,0,0,0,0},
};
static const RawCollider kZoomIns_822[] = {
    {1,-24.5,-14.8,24.5,10.8},
    {1,-5.5,9.5,5.5,14.5},
};
static const RawShape kShapes_823[] = {
    {{1,24,-12.25,30,12.25},1,10066329},
    {{1,-30,-12.25,-24,12.25},1,10066329},
    {{1,-24.5,-12.25,24.5,12.25},1,5915450},
};
static const RawBImg kFloorImgs_823[] = {
    {"map-building-warehouse-floor-01.img",0.5,1,16777215,-15.615,0,0,0,0,0},
    {"map-building-warehouse-floor-01.img",0.5,1,16777215,15.615,0,2,0,0,0},
};
static const RawBImg kCeilImgs_823[] = {
    {"map-building-warehouse-ceiling-01.img",1,1,16777215,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_823[] = {
    {1,-24.5,-12.25,24.5,12.25},
};
static const RawBImg kFloorImgs_825[] = {
    {"map-building-house-floor-01.img",0.5,1,16777215,0,0,0,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,-1,14.5,0,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,0,-14.5,2,0,0,0},
};
static const RawBImg kCeilImgs_825[] = {
    {"map-building-house-ceiling.img",0.667,1,16777215,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_825[] = {
    {1,-14.5,-13,14.5,13},
};
static const RawBImg kFloorImgs_826[] = {
    {"map-building-house-floor-02.img",0.5,1,16777215,0,0,0,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,10,14.5,0,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,0,-14.5,2,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,2.6,-14.5,2,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,5.2,-14.5,2,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,7.8,-14.5,2,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,0,-16.25,2,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,2.6,-16.25,2,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,5.2,-16.25,2,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,7.8,-16.25,2,0,0,0},
};
static const RawBImg kCeilImgs_826[] = {
    {"map-building-house-ceiling.img",0.667,1,13619151,0,0,2,0,0,0},
};
static const RawCollider kZoomIns_826[] = {
    {1,-14.5,-13,14.5,13},
};
static const RawShape kShapes_827[] = {
    {{1,-18,-11.5,18,12.5},1,3823128},
    {{1,-17,-15,17,-11},1,6368528},
};
static const RawBImg kFloorImgs_827[] = {
    {"map-building-cabin-floor.img",0.5,1,16777215,0,-1,0,0,0,0},
};
static const RawBImg kCeilImgs_827[] = {
    {"map-building-cabin-ceiling-01a.img",0.667,1,16777215,0,0.5,0,0,0,0},
    {"map-building-cabin-ceiling-01b.img",0.667,1,16777215,4,-13,0,0,0,0},
    {"map-chimney-01.img",0.5,1,16777215,13,2,0,0,0,1},
};
static const RawBEmitter kEmitters_827[] = {
    {"cabin_smoke_parent",0,0,0,1,0,1,1,0},
};
static const RawCollider kZoomIns_827[] = {
    {1,-19,-11.5,19,12.5},
    {1,1,-15,7,-11},
};
static const RawShape kShapes_828[] = {
    {{1,-14,16,11,25},1,8671554},
    {{1,-5,-25.5,1,-20.5},1,8671554},
    {{1,-30.5,-24.5,-10.5,-20.5},1,7750457},
    {{1,24.25,-1.5,31.75,4.5},1,7237230},
    {{1,-31.5,-20.5,24.5,16.5},1,6175023},
};
static const RawBImg kFloorImgs_828[] = {
    {"map-building-mansion-floor-01a.img",0.5,1,16777215,-1.5,22,0,0,0,0},
    {"map-building-mansion-floor-01b.img",0.5,1,16777215,-3.5,-2,0,0,0,0},
    {"map-building-mansion-floor-01c.img",0.5,1,16777215,28.5,1.5,0,0,0,0},
    {"map-building-mansion-floor-01d.img",0.5,1,16777215,-15,-24,0,0,0,0},
};
static const RawBImg kCeilImgs_828[] = {
    {"map-building-mansion-ceiling.img",1,1,16777215,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_828[] = {
    {1,-32,-24.599999999999998,2,-20.2},
    {1,-31.5,-20.5,24.5,16.5},
    {1,-13.5,16.400000000000002,10.5,24.8},
};
static const RawBImg kFloorImgs_829[] = {
    {"map-building-mansion-gradient-01.img",4,1,16777215,-3.75,0.25,0,0,0,0},
    {"map-building-mansion-cellar-01a.img",0.5,1,16777215,11.5,5.5,0,0,0,0},
    {"map-building-mansion-cellar-01b.img",0.5,1,16777215,28.5,1.5,0,0,0,0},
    {"map-building-mansion-cellar-01c.img",0.5,1,16777215,11.5,-9,0,0,0,0},
};
static const RawCollider kZoomIns_829[] = {
    {1,11,-10,25,16},
    {1,-1,-10.5,11,13.5},
};
static const RawBImg kFloorImgs_830[] = {
    {"map-building-shilo-floor-01.img",0.5,1,16777215,0,0,0,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,0,-13,2,0,0,0},
};
static const RawBImg kCeilImgs_830[] = {
    {"map-building-shilo-ceiling-01.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_830[] = {
    {1,-14.5,-11.5,14.5,11.5},
};
static const RawShape kShapes_831[] = {
    {{1,24,-12.25,30,12.25},1,10066329},
    {{1,-30,-12.25,-24,12.25},1,10066329},
    {{1,-24.5,-12.25,24.5,12.25},1,5915450},
};
static const RawBImg kFloorImgs_831[] = {
    {"map-building-warehouse-floor-03.img",0.5,1,16777215,-15.615,0,0,0,0,0},
    {"map-building-warehouse-floor-01.img",0.5,1,16777215,15.615,0,2,0,0,0},
};
static const RawBImg kCeilImgs_831[] = {
    {"map-building-warehouse-ceiling-01.img",1,1,16777215,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_831[] = {
    {1,-24.5,-12.25,24.5,12.25},
};
static const RawBImg kFloorImgs_842[] = {
    {"map-building-perch-floor.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_842[] = {
    {"map-building-perch-ceiling.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawShape kShapes_844[] = {
    {{1,-7,-7,7,7},1,15181895},
    {{1,-2,-30.9,2,-6.899999999999999},1,6171907},
};
static const RawBImg kFloorImgs_844[] = {
    {"map-building-hut-floor-01.img",0.5,1,16777215,0,0,0,0,0,0},
    {"map-building-hut-floor-02.img",0.5,1,16777215,0,-18.9,0,0,0,0},
};
static const RawBImg kCeilImgs_844[] = {
    {"map-building-hut-ceiling-01.img",0.667,1,16777215,0,0,0,0,0,0},
    {"map-snow-04.img",0.667,1,16777215,4.5,0.5,0,0,0,0},
    {"map-snow-05.img",0.667,1,16777215,-0.5,5,1,0,0,0},
};
static const RawCollider kZoomIns_844[] = {
    {1,-6,-6,6,6},
};
static const RawShape kShapes_845[] = {
    {{1,-7,-7,7,7},1,15181895},
    {{1,-2,-30.9,2,-6.899999999999999},1,6171907},
};
static const RawBImg kFloorImgs_845[] = {
    {"map-building-hut-floor-01.img",0.5,1,16777215,0,0,0,0,0,0},
    {"map-building-hut-floor-02.img",0.5,1,16777215,0,-18.9,0,0,0,0},
};
static const RawBImg kCeilImgs_845[] = {
    {"map-building-hut-ceiling-02.img",0.667,1,16777215,0,0,0,0,0,0},
    {"map-snow-04.img",0.667,1,16777215,4.5,0.5,0,0,0,0},
    {"map-snow-05.img",0.667,1,16777215,0.5,-4.5,3,0,0,0},
};
static const RawCollider kZoomIns_845[] = {
    {1,-6,-6,6,6},
};
static const RawShape kShapes_846[] = {
    {{1,24,-12.25,30,12.25},1,10066329},
    {{1,-30,-12.25,-24,12.25},1,10066329},
    {{1,-24.5,-12.25,24.5,12.25},1,5915450},
};
static const RawBImg kFloorImgs_846[] = {
    {"map-building-warehouse-floor-01.img",0.5,1,16777215,-15.615,0,0,0,0,0},
    {"map-building-warehouse-floor-01.img",0.5,1,16777215,15.615,0,2,0,0,0},
};
static const RawBImg kCeilImgs_846[] = {
    {"map-building-warehouse-ceiling-01.img",1,1,16777215,0,0,0,0,0,0},
    {"map-snow-04.img",0.9,1,16777215,7.5,5,1,0,0,0},
    {"map-snow-05.img",0.9,1,16777215,-8.5,4,2,0,0,0},
    {"map-snow-06.img",0.75,1,16777215,22.25,11.25,0,0,0,0},
    {"map-snow-06.img",0.75,1,16777215,-22.25,-11.25,2,0,0,0},
};
static const RawCollider kZoomIns_846[] = {
    {1,-24.5,-12.25,24.5,12.25},
};
static const RawShape kShapes_847[] = {
    {{1,22,-12.25,28,12.25},1,10066329},
    {{1,-28,-12.25,-22,12.25},1,10066329},
    {{1,-22.5,-12.25,22.5,12.25},1,2240064},
};
static const RawBImg kFloorImgs_847[] = {
    {"map-building-warehouse-floor-02.img",0.5,1,16777215,-13.72,0,0,0,0,0},
    {"map-building-warehouse-floor-02.img",0.5,1,16777215,13.72,0,2,0,0,0},
};
static const RawBImg kCeilImgs_847[] = {
    {"map-building-warehouse-ceiling-02.img",1,1,16777215,0,0,0,0,0,0},
    {"map-snow-04.img",1,1,16777215,0,4,0,0,0,0},
    {"map-snow-06.img",0.75,1,16777215,20.25,-9.75,1,0,0,0},
    {"map-snow-06.img",0.75,1,16777215,-20.25,9.75,3,0,0,0},
};
static const RawCollider kZoomIns_847[] = {
    {1,-22,-12.25,22,12.25},
};
static const RawShape kShapes_848[] = {
    {{1,24,-12.25,30,12.25},1,10066329},
    {{1,-30,-12.25,-24,12.25},1,10066329},
    {{1,-24.5,-12.25,24.5,12.25},1,5915450},
};
static const RawBImg kFloorImgs_848[] = {
    {"map-building-warehouse-floor-03.img",0.5,1,16777215,-15.615,0,0,0,0,0},
    {"map-building-warehouse-floor-01.img",0.5,1,16777215,15.615,0,2,0,0,0},
};
static const RawBImg kCeilImgs_848[] = {
    {"map-building-warehouse-ceiling-01.img",1,1,16777215,0,0,0,0,0,0},
    {"map-snow-04.img",0.9,1,16777215,7.5,5,1,0,0,0},
    {"map-snow-05.img",0.9,1,16777215,-8.5,4,2,0,0,0},
    {"map-snow-06.img",0.75,1,16777215,22.25,11.25,0,0,0,0},
    {"map-snow-06.img",0.75,1,16777215,-22.25,-11.25,2,0,0,0},
};
static const RawCollider kZoomIns_848[] = {
    {1,-24.5,-12.25,24.5,12.25},
};
static const RawBImg kFloorImgs_849[] = {
    {"map-building-shack-floor-01.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_849[] = {
    {"map-building-shack-ceiling-01.img",0.667,1,16777215,0,0,0,0,0,0},
    {"map-snow-05.img",0.667,1,16777215,-4,2.5,0,0,0,0},
    {"map-snow-04.img",0.667,1,16777215,3.5,-0.5,0,0,0,0},
};
static const RawCollider kZoomIns_849[] = {
    {1,-5.6,-2.6,5.6,4.4},
};
static const RawBImg kFloorImgs_850[] = {
    {"map-building-shack-floor-02.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_850[] = {
    {"map-building-shack-ceiling-02.img",0.667,1,16777215,0,0,0,0,0,0},
    {"map-snow-05.img",0.667,1,16777215,-2,1,0,0,0,0},
};
static const RawCollider kZoomIns_850[] = {
    {1,-4.75,-2.75,4.75,4.75},
};
static const RawShape kShapes_851[] = {
    {{1,-8.75,1,-6.75,5},1,6171907},
    {{1,3,-5.75,7,-3.75},1,6171907},
    {{1,-7,-4,9,7},1,3754050},
    {{1,-12.65,-5,-8.65,19},1,6171907},
};
static const RawBImg kFloorImgs_851[] = {
    {"map-building-shack-floor-03.img",0.5,1,16777215,0,0,0,0,0,0},
    {"map-building-hut-floor-02.img",0.5,1,16777215,-10.65,7,0,0,0,0},
};
static const RawBImg kCeilImgs_851[] = {
    {"map-building-shack-ceiling-03.img",0.667,1,10461087,0.5,0.5,0,0,0,0},
    {"map-snow-01.img",0.5,1,16777215,3.75,1.75,1,0,0,0},
};
static const RawCollider kZoomIns_851[] = {
    {1,-6.75,-3.75,8.75,6.75},
};
static const RawBImg kFloorImgs_852[] = {
    {"map-building-outhouse-floor.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_852[] = {
    {"map-building-outhouse-ceiling.img",0.5,1,16777215,0,0,0,0,0,0},
    {"map-snow-04.img",0.5,1,16777215,2.25,0,0,0,0,0},
};
static const RawCollider kZoomIns_852[] = {
    {1,-3.6,-1.7500000000000002,3.6,4.65},
};
static const RawBImg kFloorImgs_853[] = {
    {"map-building-outhouse-floor.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_853[] = {
    {"map-building-outhouse-ceiling.img",0.5,1,13735576,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_853[] = {
    {1,-3.6,-1.7500000000000002,3.6,4.65},
};
static const RawShape kShapes_854[] = {
    {{1,-5,10,5,14},1,12300935},
    {{1,-24.5,-14.8,24.5,10.8},1,3816739},
};
static const RawBImg kFloorImgs_854[] = {
    {"map-building-barn-floor-01.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_854[] = {
    {"map-building-barn-ceiling-01.img",1,1,16777215,0,-2,0,0,0,0},
    {"map-building-barn-ceiling-02.img",0.667,1,16777215,0,13.2,0,0,0,0},
    {"map-snow-01.img",0.5,1,16777215,-14.5,5.5,0,0,0,0},
    {"map-snow-02.img",0.5,1,16777215,-0.5,-9,0,0,0,0},
    {"map-snow-03.img",0.5,1,16777215,14.5,5.5,0,0,0,0},
};
static const RawCollider kZoomIns_854[] = {
    {1,-24.5,-14.8,24.5,10.8},
    {1,-5.5,9.5,5.5,14.5},
};
static const RawShape kShapes_855[] = {
    {{1,-5,10,5,14},1,12300935},
    {{1,-24.5,-14.8,24.5,10.8},1,3816739},
};
static const RawBImg kFloorImgs_855[] = {
    {"map-building-barn-floor-01.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_855[] = {
    {"map-building-barn-ceiling-01.img",1,1,16777215,0,-2,0,0,0,0},
    {"map-building-barn-ceiling-02.img",0.667,1,16777215,0,13.2,0,0,0,0},
    {"map-snow-01.img",0.5,1,16777215,-14.5,5.5,0,0,0,0},
    {"map-snow-02.img",0.5,1,16777215,-0.5,-9,0,0,0,0},
    {"map-snow-03.img",0.5,1,16777215,14.5,5.5,0,0,0,0},
};
static const RawCollider kZoomIns_855[] = {
    {1,-24.5,-14.8,24.5,10.8},
    {1,-5.5,9.5,5.5,14.5},
};
static const RawShape kShapes_856[] = {
    {{1,-26.75,-4,-5.25,18},1,7820585},
    {{1,-5.5,-18.25,17.5,18.25},1,9989427},
    {{1,17.5,-3.5,26.5,11.5},1,7820585},
};
static const RawBImg kFloorImgs_856[] = {
    {"map-building-bank-floor-01.img",0.5,1,16777215,0,6.96,0,0,0,0},
    {"map-building-bank-floor-02.img",0.5,1,16777215,9.5,-12.5,0,0,0,0},
};
static const RawBImg kCeilImgs_856[] = {
    {"map-building-bank-ceiling-01.img",0.667,1,16777215,-16,7,0,0,0,0},
    {"map-building-bank-ceiling-02.img",0.667,1,16777215,6,0,0,0,0,0},
    {"map-building-bank-ceiling-03.img",0.667,1,16777215,22,8,0,0,0,0},
    {"map-snow-02.img",0.5,1,16777215,-13,0,1,0,0,0},
    {"map-snow-04.img",1,1,16777215,1.25,9.25,2,0,0,0},
    {"map-snow-06.img",0.75,1,16777215,13.75,15.25,0,0,0,0},
    {"map-snow-06.img",0.75,1,16777215,15.25,-15.75,1,0,0,0},
};
static const RawCollider kZoomIns_856[] = {
    {1,-5.25,-19.25,17.25,17.25},
    {1,16.75,-3.25,26.25,11.25},
    {1,-25.75,-5,-4.25,17},
};
static const RawShape kShapes_857[] = {
    {{1,-42.25,-22,0.25,6},1,5855577},
    {{1,-42.25,-1.25,-6.75,18.25},1,3355970},
    {{1,-7,5.75,0,18.25},1,4278620},
    {{1,-0.15000000000000036,-22,20.85,22},1,3355970},
    {{1,20.5,3,42,22},1,3355970},
    {{1,-5.75,0.25,-1.25,4.75},1,6310464},
    {{0,-30.5,-18,1.5,0},1,8026746},
    {{0,-20.5,-10.5,1.5,0},1,8026746},
    {{1,-39.9,-10.1,-37.1,-3.9},1,13278307},
    {{1,-10.6,-20.9,-4.4,-18.1},1,13278307},
};
static const RawBImg kFloorImgs_857[] = {
    {"map-building-police-floor-01.img",0.5,1,16777215,-9.5,0,0,0,0,0},
    {"map-building-police-floor-02.img",0.5,1,16777215,33,0,0,0,0,0},
};
static const RawBImg kCeilImgs_857[] = {
    {"map-building-police-ceiling-01.img",0.667,1,16777215,-21.5,8.5,0,0,0,0},
    {"map-building-police-ceiling-02.img",0.667,1,16777215,10.5,0,0,0,0,0},
    {"map-building-police-ceiling-03.img",0.667,1,16777215,31.96,12.5,0,0,0,0},
    {"map-snow-01.img",0.5,1,16777215,13,17.5,3,0,0,0},
    {"map-snow-02.img",0.5,1,16777215,-21,14,0,0,0,0},
    {"map-snow-03.img",0.5,1,16777215,30.25,6.25,2,0,0,0},
    {"map-snow-07.img",0.6,1,16777215,4.5,-3.25,1,0,0,0},
    {"map-snow-06.img",0.75,1,16777215,-40.25,14.75,3,0,0,0},
    {"map-snow-06.img",0.75,1,16777215,-38.75,0.75,2,0,0,0},
};
static const RawCollider kZoomIns_857[] = {
    {1,-42.25,-1.25,-6.75,18.25},
    {1,-7,5.75,0,18.25},
    {1,-0.15000000000000036,-22,20.85,22},
    {1,20.5,3,42,22},
};
static const RawBImg kFloorImgs_858[] = {
    {"map-building-house-floor-01.img",0.5,1,16777215,0,0,0,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,-1,14.5,0,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,0,-14.5,2,0,0,0},
};
static const RawBImg kCeilImgs_858[] = {
    {"map-building-house-ceiling.img",0.667,1,16777215,0,0,0,0,0,0},
    {"map-snow-01.img",0.5,1,16777215,-5.5,8.5,0,0,0,0},
    {"map-snow-02.img",0.5,1,16777215,4.5,-7,0,0,0,0},
};
static const RawCollider kZoomIns_858[] = {
    {1,-14.5,-13,14.5,13},
};
static const RawBImg kFloorImgs_859[] = {
    {"map-building-house-floor-02.img",0.5,1,16777215,0,0,0,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,10,14.5,0,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,0,-14.5,2,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,2.6,-14.5,2,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,5.2,-14.5,2,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,7.8,-14.5,2,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,0,-16.25,2,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,2.6,-16.25,2,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,5.2,-16.25,2,0,0,0},
    {"map-building-porch-01.img",0.5,1,16777215,7.8,-16.25,2,0,0,0},
};
static const RawBImg kCeilImgs_859[] = {
    {"map-building-house-ceiling.img",0.667,1,13619151,0,0,2,0,0,0},
    {"map-snow-02.img",0.5,1,16777215,3.5,6,2,0,0,0},
    {"map-snow-01.img",0.5,1,16777215,-4.5,-8,3,0,0,0},
};
static const RawCollider kZoomIns_859[] = {
    {1,-14.5,-13,14.5,13},
};
static const RawShape kShapes_860[] = {
    {{1,-18,-11.5,18,12.5},1,3823128},
    {{1,-17,-15,17,-11},1,6368528},
};
static const RawBImg kFloorImgs_860[] = {
    {"map-building-cabin-floor.img",0.5,1,16777215,0,-1,0,0,0,0},
};
static const RawBImg kCeilImgs_860[] = {
    {"map-building-cabin-ceiling-01a.img",0.667,1,16777215,0,0.5,0,0,0,0},
    {"map-building-cabin-ceiling-01b.img",0.667,1,16777215,4,-13,0,0,0,0},
    {"map-snow-01.img",0.5,1,16777215,-13,6,1,0,0,0},
    {"map-snow-02.img",0.5,1,16777215,-3.5,-6.25,1,0,0,0},
    {"map-snow-03.img",0.5,1,16777215,10.75,8.25,0,0,0,0},
    {"map-chimney-01.img",0.5,1,16777215,13,2,0,0,0,1},
};
static const RawBEmitter kEmitters_860[] = {
    {"cabin_smoke_parent",0,0,0,1,0,1,1,0},
};
static const RawCollider kZoomIns_860[] = {
    {1,-19,-11.5,19,12.5},
    {1,1,-15,7,-11},
};
static const RawShape kShapes_861[] = {
    {{1,-14,16,11,25},1,8671554},
    {{1,-5,-25.5,1,-20.5},1,8671554},
    {{1,-30.5,-24.5,-10.5,-20.5},1,7750457},
    {{1,24.25,-1.5,31.75,4.5},1,7237230},
    {{1,-31.5,-20.5,24.5,16.5},1,6175023},
};
static const RawBImg kFloorImgs_861[] = {
    {"map-building-mansion-floor-01a.img",0.5,1,16777215,-1.5,22,0,0,0,0},
    {"map-building-mansion-floor-01b.img",0.5,1,16777215,-3.5,-2,0,0,0,0},
    {"map-building-mansion-floor-01c.img",0.5,1,16777215,28.5,1.5,0,0,0,0},
    {"map-building-mansion-floor-01d.img",0.5,1,16777215,-15,-24,0,0,0,0},
};
static const RawBImg kCeilImgs_861[] = {
    {"map-building-mansion-ceiling.img",1,1,16777215,0,0,0,0,0,0},
    {"map-snow-01.img",0.5,1,16777215,6,19.5,1,0,0,0},
    {"map-snow-02.img",0.5,1,16777215,-16,8,2,0,0,0},
    {"map-snow-03.img",0.5,1,16777215,20.25,-1.75,1,0,0,0},
    {"map-snow-04.img",1,1,16777215,10.25,-13.25,0,0,0,0},
    {"map-snow-05.img",1,1,16777215,10.25,6.25,0,0,0,0},
    {"map-snow-07.img",0.5,1,16777215,-21.25,-20.25,2,0,0,0},
    {"map-snow-06.img",0.75,1,16777215,-29.75,13.25,3,0,0,0},
};
static const RawCollider kZoomIns_861[] = {
    {1,-32,-24.599999999999998,2,-20.2},
    {1,-31.5,-20.5,24.5,16.5},
    {1,-13.5,16.400000000000002,10.5,24.8},
};
static const RawShape kShapes_862[] = {
    {{1,-14,-9,14,9},1,4608356},
    {{1,-7.5,-3.75,7.5,3.75},1,5793921},
    {{1,7,-11.65,11,-8.65},1,7354635},
    {{1,-11,8.65,-7,11.65},1,7354635},
};
static const RawBImg kFloorImgs_862[] = {
    {"map-building-teahouse-floor-01.img",0.5,1,16777215,0,0,0,0,0,0},
    {"map-building-teahouse-floor-02.img",0.5,1,16777215,9,-10.25,0,0,0,0},
    {"map-building-teahouse-floor-02.img",0.5,1,16777215,-9,10.25,2,0,0,0},
};
static const RawBImg kCeilImgs_862[] = {
    {"map-building-teahouse-ceiling-01.img",0.5,1,16777215,0,0,0,0,0,0},
    {"map-snow-04.img",1,1,16777215,4,0.5,0,0,0,0},
    {"map-snow-06.img",0.75,1,16777215,11.75,-5,1,0,0,0},
    {"map-snow-06.img",0.75,1,16777215,-11.75,5,3,0,0,0},
};
static const RawCollider kZoomIns_862[] = {
    {1,-12,-7,12,7},
};
static const RawShape kShapes_864[] = {
    {{1,-31.5,-8,31.5,8},1,5197647},
    {{1,-16.5,-11,-11.5,-8},1,3618615},
    {{1,11.5,-11,16.5,-8},1,3618615},
    {{1,-16.5,8,-11.5,11},1,3618615},
    {{1,11.5,8,16.5,11},1,3618615},
};
static const RawBImg kFloorImgs_864[] = {
    {"map-building-bridge-lg-floor.img",0.5,1,16777215,-15.75,0,0,0,0,0},
    {"map-building-bridge-lg-floor.img",0.5,1,16777215,15.75,0,2,0,1,0},
};
static const RawBImg kCeilImgs_864[] = {
    {"map-building-bridge-lg-ceiling.img",0.5,1,16777215,0,0,0,0,0,0},
    {"map-snow-03.img",0.4,1,16777215,-10,-4,0,0,0,0},
    {"map-snow-07.img",0.4,1,16777215,8,4,0,0,0,0},
    {"map-snow-06.img",0.667,1,16777215,15,-5.25,1,0,0,0},
    {"map-snow-06.img",0.667,1,16777215,-15,5.25,3,0,0,0},
};
static const RawCollider kZoomIns_864[] = {
    {1,-16.5,-7,16.5,7},
};
static const RawBImg kFloorImgs_865[] = {
    {"map-building-container-floor-01.img",0.5,1,2703694,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_865[] = {
    {"map-building-container-ceiling-01.img",0.5,1,2703694,0,0,0,0,0,0},
    {"map-snow-05.img",0.6,1,16777215,0,3,0,0,0,0},
};
static const RawCollider kZoomIns_865[] = {
    {1,-2.5,-3.25,2.5,7.75},
};
static const RawShape kShapes_866[] = {
    {{1,-8.5,-20.5,24.5,20.5},1,1130539},
    {{1,-3.5,20.5,20.5,26.5},1,10066329},
    {{1,-3.5,-26.5,20.5,-20.5},1,10066329},
    {{1,-25.5,-11.5,-8.5,20.5},1,5388583},
};
static const RawBImg kFloorImgs_866[] = {
    {"map-building-workshop-floor-01.img",0.5,1,16777215,8,0,0,0,0,0},
    {"map-building-workshop-floor-02.img",0.5,1,16777215,-17,4.5,0,0,0,0},
};
static const RawBImg kCeilImgs_866[] = {
    {"map-building-workshop-ceiling-02.img",0.5,1,16777215,-16.5,4.5,0,0,0,0},
    {"map-building-workshop-ceiling-01.img",0.5,1,16777215,8,0,0,0,0,0},
};
static const RawCollider kZoomIns_866[] = {
    {1,-8,-20,24,20},
    {1,-25,-11,-8,20},
};
static const RawBEmitter kEmitters_871[] = {
    {"campfire_smoke",0,0,0,1,0,1,1,0},
};
static const RawCollider kZoomIns_871[] = {
    {1,-15,-15,15,15},
};
static const RawShape kShapes_872[] = {
    {{1,-9,-9,9,9},1,10555920},
    {{1,-3.5,-3.5,3.5,3.5},1,16727611},
    {{1,-2,-11.65,2,-8.65},1,7354635},
};
static const RawBImg kFloorImgs_872[] = {
    {"map-building-pavilion-floor-01.img",0.5,1,16777215,0,0,0,0,0,0},
    {"map-building-teahouse-floor-02.img",0.5,1,16777215,0,-10.25,0,0,0,0},
};
static const RawBImg kCeilImgs_872[] = {
    {"map-building-pavilion-ceiling-01.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_872[] = {
    {1,-7,-7,7,7},
};
static const RawShape kShapes_877[] = {
    {{1,-8.5,-20.5,24.5,20.5},1,1130539},
    {{1,-3.5,20.5,20.5,26.5},1,10066329},
    {{1,-3.5,-26.5,20.5,-20.5},1,10066329},
    {{1,-25.5,-11.5,-8.5,20.5},1,5388583},
};
static const RawBImg kFloorImgs_877[] = {
    {"map-building-workshop-floor-01.img",0.5,1,16777215,8,0,0,0,0,0},
    {"map-building-workshop-floor-02.img",0.5,1,16777215,-17,4.5,0,0,0,0},
};
static const RawBImg kCeilImgs_877[] = {
    {"map-building-workshop-ceiling-02.img",0.5,1,16777215,-16.5,4.5,0,0,0,0},
    {"map-building-workshop-ceiling-01.img",0.5,1,16777215,8,0,0,0,0,0},
    {"map-snow-01.img",0.667,1,16777215,1,2,3,0,0,0},
    {"map-snow-02.img",0.667,1,16777215,17.5,16,0,0,0,0},
    {"map-snow-05.img",1,1,16777215,-12,-7,2,0,0,0},
    {"map-snow-06.img",1,1,16777215,21.5,-17.15,1,0,0,0},
    {"map-snow-06.img",0.925,1,16777215,-22.75,15.9,3,0,0,0},
};
static const RawCollider kZoomIns_877[] = {
    {1,-8,-20,24,20},
    {1,-25,-11,-8,20},
};
static const RawBEmitter kEmitters_881[] = {
    {"campfire_smoke",0,0,0,1,0,1,1,0},
};
static const RawCollider kZoomIns_881[] = {
    {1,-15,-15,15,15},
};
static const RawShape kShapes_882[] = {
    {{1,-9,-9,9,9},1,10555920},
    {{1,-3.5,-3.5,3.5,3.5},1,16727611},
    {{1,-2,-11.65,2,-8.65},1,7354635},
};
static const RawBImg kFloorImgs_882[] = {
    {"map-building-pavilion-floor-01.img",0.5,1,16777215,0,0,0,0,0,0},
    {"map-building-teahouse-floor-02.img",0.5,1,16777215,0,-10.25,0,0,0,0},
};
static const RawBImg kCeilImgs_882[] = {
    {"map-building-pavilion-ceiling-01.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_882[] = {
    {1,-7,-7,7,7},
};
static const RawShape kShapes_887[] = {
    {{1,-3.6,4.2,3.6,15.8},1,6707790},
};
static const RawBImg kFloorImgs_887[] = {
    {"map-bunker-generic-floor-03.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_887[] = {
    {"map-bunker-generic-ceiling-02.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_887[] = {
    {1,-1.5,-3.5,1.5,3},
};
static const RawBImg kFloorImgs_888[] = {
    {"map-bunker-chrys-chamber-floor-01a.img",0.5,1,16777215,0,1.85,0,0,0,0},
    {"map-bunker-chrys-chamber-floor-01b.img",0.5,1,16777215,11,-10.75,0,0,0,0},
};
static const RawBImg kCeilImgs_888[] = {
    {"map-bunker-chrys-chamber-ceiling-01.img",1,1,6182731,11.5,-11,0,0,0,0},
};
static const RawCollider kZoomIns_888[] = {
    {1,-3.5,-21,25.5,-3},
};
static const RawBImg kFloorImgs_889[] = {
    {"map-bunker-chrys-chamber-floor-01a.img",0.5,1,16777215,0,1.85,0,0,0,0},
    {"map-bunker-chrys-chamber-floor-01b.img",0.5,1,16777215,11,-10.75,0,0,0,0},
};
static const RawBImg kCeilImgs_889[] = {
    {"map-bunker-chrys-chamber-ceiling-01.img",1,1,6182731,11.5,-11,0,0,0,0},
};
static const RawCollider kZoomIns_889[] = {
    {1,-3.5,-21,25.5,-3},
};
static const RawBImg kFloorImgs_890[] = {
    {"map-bunker-chrys-compartment-floor-01a.img",0.5,1,16777215,-12.5,-4.5,0,0,0,0},
    {"map-bunker-chrys-compartment-floor-01d.img",0.5,1,16777215,3.5,2,0,0,0,0},
};
static const RawBImg kCeilImgs_890[] = {
    {"map-bunker-chrys-compartment-ceiling-01a.img",1,1,6182731,-10.5,-2.5,0,0,0,0},
    {"map-bunker-chrys-compartment-ceiling-01b.img",1,1,6182731,4,3,0,0,0,0},
};
static const RawCollider kZoomIns_890[] = {
    {1,-14,-11,14,15},
};
static const RawBImg kFloorImgs_891[] = {
    {"map-bunker-chrys-compartment-floor-01a.img",0.5,1,16777215,-12.5,-4.5,0,0,0,0},
    {"map-bunker-chrys-compartment-floor-01c.img",0.5,1,16777215,3.5,2,0,0,0,0},
};
static const RawBImg kCeilImgs_891[] = {
    {"map-bunker-chrys-compartment-ceiling-01a.img",1,1,6182731,-10.5,-2.5,0,0,0,0},
    {"map-bunker-chrys-compartment-ceiling-01b.img",1,1,6182731,4,3,0,0,0,0},
};
static const RawCollider kZoomIns_891[] = {
    {1,-14,-11,14,15},
};
static const RawBImg kFloorImgs_892[] = {
    {"map-bunker-chrys-compartment-floor-02a.img",0.5,1,16777215,0,-2.75,0,0,0,0},
    {"map-bunker-chrys-compartment-floor-02b.img",0.5,1,16777215,0,9.75,0,0,0,0},
};
static const RawBImg kCeilImgs_892[] = {
    {"map-bunker-chrys-compartment-ceiling-02a.img",1,1,6182731,0,8.5,0,0,0,0},
    {"map-bunker-chrys-compartment-ceiling-02b.img",1,1,6182731,0,-2.5,0,0,0,0},
};
static const RawCollider kZoomIns_892[] = {
    {1,-10,-11,10,11},
};
static const RawBImg kFloorImgs_893[] = {
    {"map-bunker-chrys-compartment-floor-02a.img",0.5,1,16777215,0,-2.75,0,0,0,0},
    {"map-bunker-chrys-compartment-floor-02c.img",0.5,1,16777215,0,9.75,0,0,0,0},
};
static const RawBImg kCeilImgs_893[] = {
    {"map-bunker-chrys-compartment-ceiling-02a.img",1,1,6182731,0,8.5,0,0,0,0},
    {"map-bunker-chrys-compartment-ceiling-02b.img",1,1,6182731,0,-2.5,0,0,0,0},
};
static const RawCollider kZoomIns_893[] = {
    {1,-10,-11,10,11},
};
static const RawBImg kFloorImgs_894[] = {
    {"map-bunker-chrys-compartment-floor-03a.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_894[] = {
    {"map-bunker-chrys-compartment-ceiling-03a.img",1,1,6182731,0,-9.5,0,0,0,0},
    {"map-bunker-chrys-compartment-ceiling-03b.img",1,1,6182731,0,3,0,0,0,0},
};
static const RawCollider kZoomIns_894[] = {
    {1,-10,-13,10,13},
};
static const RawBImg kFloorImgs_895[] = {
    {"map-bunker-chrys-compartment-floor-03a.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_895[] = {
    {"map-bunker-chrys-compartment-ceiling-03a.img",1,1,6182731,0,-9.5,0,0,0,0},
    {"map-bunker-chrys-compartment-ceiling-03b.img",1,1,6182731,0,3,0,0,0,0},
};
static const RawCollider kZoomIns_895[] = {
    {1,-10,-13,10,13},
};
static const RawBImg kFloorImgs_896[] = {
    {"map-bunker-generic-floor-01.img",0.5,1,16777215,0,0,0,0,0,0},
    {"map-bunker-generic-floor-01.img",0.5,1,16777215,-28.5,-77,2,0,0,0},
};
static const RawBImg kFloorImgs_897[] = {
    {"map-bunker-cloud-floor-01.img",0.5,1,16777215,-4,-10,0,0,0,0},
    {"map-bunker-cloud-floor-02.img",0.5,1,16777215,-1.5,33.5,0,0,0,0},
    {"map-bunker-cloud-floor-03.img",0.5,1,16777215,35.5,-8,0,0,0,0},
};
static const RawBImg kCeilImgs_897[] = {
    {"map-bunker-cloud-ceiling-03.img",1,1,6250335,-1.5,18.5,0,0,0,0},
    {"map-bunker-cloud-ceiling-04.img",1,1,6250335,-1.5,29.5,0,0,0,0},
    {"map-bunker-cloud-ceiling-05.img",1,1,6250335,28.25,-9.5,0,0,0,0},
    {"map-bunker-cloud-ceiling-06.img",1,1,6250335,-31.75,-8,0,0,0,0},
};
static const RawCollider kZoomIns_897[] = {
    {1,-33,15.5,30,36.5},
    {1,-38,-31,-26,15.5},
    {1,23,-35.5,38,15.5},
    {1,19,-35.5,23,-29.5},
};
static const RawBImg kCeilImgs_898[] = {
    {"map-bunker-cloud-ceiling-01.img",1,1,6250335,-1.5,-6,0,0,0,0},
    {"map-bunker-cloud-ceiling-02.img",1,1,6250335,-0.25,-34,0,0,0,0},
};
static const RawCollider kZoomIns_898[] = {
    {1,-25,-26.5,22,14.5},
    {1,-19,-42.5,18,-26.5},
};
static const RawCollider kZoomIns_899[] = {
    {1,-2.5,-2.25,2.5,2.25},
};
static const RawBImg kFloorImgs_900[] = {
    {"map-bunker-generic-floor-01.img",0.5,1,16777215,0,7.5,0,0,0,0},
};
static const RawBImg kFloorImgs_901[] = {
    {"map-bunker-egg-chamber-floor-01a.img",0.5,1,16777215,-0.15,-4.6,0,0,0,0},
    {"map-bunker-egg-chamber-floor-01b.img",0.5,1,16777215,0,9.24,0,0,0,0},
};
static const RawBImg kCeilImgs_901[] = {
    {"map-bunker-egg-chamber-ceiling-01.img",1,1,16777215,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_901[] = {
    {1,-10,-13.5,10,4.5},
};
static const RawBImg kFloorImgs_902[] = {
    {"map-bunker-egg-chamber-floor-01a.img",0.5,1,16777215,-0.15,-4.6,0,0,0,0},
    {"map-bunker-egg-chamber-floor-01b.img",0.5,1,16777215,0,9.25,0,0,0,0},
};
static const RawBImg kCeilImgs_902[] = {
    {"map-bunker-egg-chamber-ceiling-01.img",1,1,16777215,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_902[] = {
    {1,-10,-13.5,10,4.5},
};
static const RawBImg kFloorImgs_903[] = {
    {"map-bunker-egg-chamber-floor-01a.img",0.5,1,16777215,-0.15,-4.6,0,0,0,0},
    {"map-bunker-egg-chamber-floor-01b.img",0.5,1,16777215,0,9.25,0,0,0,0},
};
static const RawBImg kCeilImgs_903[] = {
    {"map-bunker-egg-chamber-ceiling-01.img",1,1,16777215,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_903[] = {
    {1,-10,-13.5,10,4.5},
};
static const RawShape kShapes_904[] = {
    {{1,14,-2,26.5,9},1,2894892},
    {{1,25.5,-5.75,39,12.75},1,3815994},
};
static const RawBImg kFloorImgs_904[] = {
    {"map-bunker-hydra-floor-01.img",0.5,1,16777215,25.75,3.5,0,0,0,0},
    {"map-bunker-generic-floor-01.img",0.5,1,16777215,-16.5,-90,2,0,0,0},
    {"map-bunker-generic-floor-01.img",0.5,1,16777215,40,-51,0,0,0,0},
};
static const RawBImg kCeilImgs_904[] = {
    {"map-bunker-hydra-ceiling-01.img",1,1,16777215,25.75,3.5,0,0,0,0},
};
static const RawCollider kZoomIns_904[] = {
    {1,13,-2,25.5,9},
    {1,25.5,-5.75,39,12.75},
};
static const RawBImg kFloorImgs_905[] = {
    {"map-bunker-hydra-chamber-floor-01a.img",0.5,1,16777215,17.5,3.5,0,0,0,0},
    {"map-bunker-hydra-chamber-floor-01b.img",0.5,1,16777215,3.5,2.5,0,0,0,0},
    {"map-bunker-hydra-chamber-floor-02.img",0.5,1,16777215,-15.5,-83,0,0,0,0},
    {"map-bunker-hydra-chamber-floor-03.img",0.5,1,16777215,40.5,-58.5,0,0,0,0},
};
static const RawBImg kCeilImgs_905[] = {
    {"map-bunker-hydra-chamber-ceiling-01.img",1,1,6250335,7,2,0,0,0,0},
    {"map-bunker-hydra-chamber-ceiling-02.img",1,1,6250335,-13.5,-76.5,0,0,0,0},
    {"map-bunker-hydra-chamber-ceiling-03.img",1,1,6250335,38,-62,0,0,0,0},
};
static const RawCollider kZoomIns_905[] = {
    {1,-6.5,-7.75,13.5,12.25},
    {1,-20.5,-87.5,-9.5,-66.5},
    {1,26.5,-70,49.5,-54},
};
static const RawBImg kFloorImgs_906[] = {
    {"map-bunker-hydra-compartment-floor-01.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_906[] = {
    {"map-bunker-hydra-compartment-ceiling-01.img",1,1,6250335,0,1.25,0,0,0,0},
};
static const RawCollider kZoomIns_906[] = {
    {1,-10,-8.75,10,11.25},
};
static const RawBImg kFloorImgs_907[] = {
    {"map-bunker-hydra-compartment-floor-02.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_907[] = {
    {"map-bunker-hydra-compartment-ceiling-02.img",1,1,6250335,0,1,0,0,0,0},
};
static const RawCollider kZoomIns_907[] = {
    {1,-24.5,-16.5,20.5,22.5},
};
static const RawBImg kFloorImgs_908[] = {
    {"map-bunker-hydra-compartment-floor-03.img",0.5,1,16777215,0,-0.5,0,0,0,0},
};
static const RawBImg kCeilImgs_908[] = {
    {"map-bunker-hydra-compartment-ceiling-03.img",1,1,6250335,0,1,0,0,0,0},
};
static const RawCollider kZoomIns_908[] = {
    {1,-10,-7,10,8.5},
};
static const RawShape kShapes_909[] = {
    {{1,-3.6,4.2,3.6,15.8},1,6707790},
};
static const RawBImg kFloorImgs_909[] = {
    {"map-bunker-storm-floor-02.img",0.5,1,16777215,1.25,10,0,0,0,0},
};
static const RawBImg kCeilImgs_909[] = {
    {"map-building-shack-ceiling-01.img",0.667,1,16777215,-1,10,1,0,0,0},
};
static const RawCollider kZoomIns_909[] = {
    {1,-3.5,4.4,3.5,15.6},
};
static const RawBImg kFloorImgs_910[] = {
    {"map-bunker-storm-chamber-floor-01a.img",0.5,1,16777215,8.5,-4.5,0,0,0,0},
    {"map-bunker-storm-chamber-floor-01b.img",0.5,1,16777215,0,9.25,0,0,0,0},
};
static const RawBImg kCeilImgs_910[] = {
    {"map-bunker-storm-chamber-ceiling-01.img",1,1,16777215,8.5,-1,0,0,0,0},
};
static const RawCollider kZoomIns_910[] = {
    {1,-9.5,-14,26.5,5},
};
static const RawShape kShapes_911[] = {
    {{1,14.5,0.8500000000000001,25.5,5.85},1,2703694},
    {{1,41,-35.05,52,-30.049999999999997},1,2703694},
};
static const RawBImg kFloorImgs_911[] = {
    {"map-bunker-conch-floor-01.img",0.5,1,16777215,20.75,3.45,0,0,0,0},
    {"map-bunker-conch-floor-01.img",0.5,1,16777215,48.75,-32.45,0,0,0,0},
};
static const RawBImg kCeilImgs_911[] = {
    {"map-bunker-conch-ceiling-01.img",0.5,1,16777215,19.25,3.35,0,0,0,0},
    {"map-bunker-conch-ceiling-01.img",0.5,1,16777215,47.25,-32.55,0,0,0,0},
};
static const RawCollider kZoomIns_911[] = {
    {1,13.5,0.8500000000000001,24.5,5.85},
    {1,42,-35.05,53,-30.049999999999997},
};
static const RawBImg kFloorImgs_912[] = {
    {"map-bunker-conch-chamber-floor-01.img",0.5,1,16777215,4,5,0,0,0,0},
    {"map-bunker-conch-chamber-floor-02.img",0.5,1,16777215,34.86,-29.9,0,0,0,0},
};
static const RawBImg kCeilImgs_912[] = {
    {"map-bunker-conch-chamber-ceiling-01.img",1,1,6250335,-2,3.5,0,0,0,0},
    {"map-bunker-conch-chamber-ceiling-02.img",1,1,6250335,26.25,-29.9,0,0,0,0},
};
static const RawBEmitter kEmitters_912[] = {
    {"bunker_bubbles_01",-2,-13.5,0,0.5,0,0,1,0},
};
static const RawCollider kZoomIns_912[] = {
    {1,-11.5,-1.5,13.5,8.5},
    {1,11.5,-34.5,42,-25.5},
};
static const RawBImg kFloorImgs_913[] = {
    {"map-bunker-conch-compartment-floor-01a.img",0.5,1,16777215,-3,-0.75,0,0,0,0},
    {"map-bunker-conch-compartment-floor-01b.img",0.5,1,16777215,9.75,-17.5,0,0,0,0},
};
static const RawBImg kCeilImgs_913[] = {
    {"map-bunker-conch-compartment-ceiling-01.img",1,1,6250335,-0.75,-5.5,0,0,0,0},
};
static const RawBEmitter kEmitters_913[] = {
    {"bunker_bubbles_01",-0.5,-1,0,0.5,0,0,1,0},
};
static const RawCollider kZoomIns_913[] = {
    {1,-14,-13,11,11},
    {1,5.5,-17,13.5,-12},
};
static const RawShape kShapes_914[] = {
    {{1,-2,-2.25,2,4.25},1,3815994},
};
static const RawBImg kFloorImgs_914[] = {
    {"map-bunker-generic-floor-02.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_914[] = {
    {"map-bunker-generic-ceiling-01.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_914[] = {
    {1,-2,-2.5,2,4},
};
static const RawShape kShapes_915[] = {
    {{1,-2,-2.25,2,4.25},1,3815994},
};
static const RawBImg kFloorImgs_915[] = {
    {"map-bunker-generic-floor-02.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_915[] = {
    {"map-bunker-crossing-ceiling-01.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_915[] = {
    {1,-2,-2.5,2,4},
};
static const RawShape kShapes_916[] = {
    {{1,-5,-5,5,5},1,1984867},
};
static const RawBImg kFloorImgs_916[] = {
    {"map-bunker-crossing-floor-01.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_917[] = {
    {"map-bunker-crossing-chamber-floor-01a.img",0.5,1,16777215,-11.44,27,2,0,0,0},
    {"map-bunker-crossing-chamber-floor-01b.img",0.5,1,16777215,-9.38,18.5,2,0,0,0},
    {"map-bunker-crossing-chamber-floor-01c.img",0.5,1,16777215,-36.44,18.5,2,0,0,0},
    {"map-bunker-crossing-chamber-floor-03.img",0.5,1,16777215,28.5,23.5,2,0,0,0},
    {"map-bunker-crossing-chamber-floor-02.img",0.5,1,16777215,-28.5,-17.5,0,0,0,0},
    {"map-bunker-crossing-chamber-floor-01a.img",0.5,1,16777215,11.45,-21,0,0,0,0},
    {"map-bunker-crossing-chamber-floor-01b.img",0.5,1,16777215,9.39,-12.5,0,0,0,0},
    {"map-bunker-crossing-chamber-floor-01c.img",0.5,1,16777215,36.45,-12.5,0,0,0,0},
};
static const RawBImg kCeilImgs_917[] = {
    {"map-bunker-crossing-chamber-ceiling-01.img",1,1,6250335,-3.5,24,0,0,0,0},
    {"map-bunker-crossing-chamber-ceiling-01.img",1,1,6250335,3.5,-18,2,0,0,0},
};
static const RawCollider kZoomIns_917[] = {
    {1,-38.1,22.5,32.1,32.5},
    {1,-32.1,-26.5,38.1,-16.5},
    {1,-7,17.5,-1,23.5},
    {1,1,-17.5,7,-11.5},
};
static const RawBImg kFloorImgs_918[] = {
    {"",0.5,1,6250335,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_918[] = {
    {"map-building-crossing-bathroom-ceiling.img",0.5,1,6250335,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_918[] = {
    {1,-3.75,-2,3.75,2},
};
static const RawBImg kFloorImgs_919[] = {
    {"map-bunker-crossing-compartment-floor-02.img",0.5,1,16777215,-22.5,-10,0,0,0,0},
    {"map-bunker-crossing-compartment-floor-01.img",0.5,1,16777215,4,3,0,0,0,0},
};
static const RawBImg kCeilImgs_919[] = {
    {"map-bunker-crossing-compartment-ceiling-01a.img",1,1,6250335,-22.475,-11,0,0,0,0},
    {"map-bunker-crossing-compartment-ceiling-01b.img",1,1,6250335,3.975,3,0,0,0,0},
};
static const RawBEmitter kEmitters_919[] = {
    {"bunker_bubbles_02",-1.5,0,0,0.5,0,0,1,0},
};
static const RawCollider kZoomIns_919[] = {
    {1,-18,-11.5,26,17.5},
    {1,-26.5,-20,-17.5,-2},
};
static const RawShape kShapes_920[] = {
    {{1,-3.6,4.2,3.6,15.8},1,6707790},
};
static const RawBImg kFloorImgs_920[] = {
    {"map-bunker-storm-floor-02.img",0.5,1,16777215,1.25,10,0,0,0,0},
};
static const RawBImg kCeilImgs_920[] = {
    {"map-building-shack-ceiling-01.img",0.667,1,16777215,-1,10,1,0,0,0},
};
static const RawCollider kZoomIns_920[] = {
    {1,-3.5,4.4,3.5,15.6},
};
static const RawBImg kFloorImgs_921[] = {
    {"map-bunker-hatchet-chamber-floor-01a.img",0.5,1,16777215,0,-4.5,0,0,0,0},
    {"map-bunker-hatchet-chamber-floor-01b.img",0.5,1,16777215,0,9.25,0,0,0,0},
    {"map-bunker-hatchet-chamber-floor-01c.img",0.5,1,16777215,-15,-9.475,0,0,0,0},
};
static const RawBImg kCeilImgs_921[] = {
    {"map-bunker-hatchet-chamber-ceiling-01.img",1,1,6250335,-3,-4.5,0,0,0,0},
};
static const RawCollider kZoomIns_921[] = {
    {1,-16,-13.65,10,4.85},
};
static const RawBImg kFloorImgs_922[] = {
    {"map-bunker-hatchet-compartment-floor-01.img",0.5,1,16777215,0,0.5,0,0,0,0},
};
static const RawBImg kCeilImgs_922[] = {
    {"map-bunker-hatchet-compartment-ceiling-01.img",1,1,6250335,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_922[] = {
    {1,-16,-12.5,16,12.5},
};
static const RawBImg kFloorImgs_923[] = {
    {"map-bunker-hatchet-compartment-floor-02a.img",0.5,1,16777215,4,-8.25,0,0,0,0},
    {"map-bunker-hatchet-compartment-floor-02b.img",0.5,1,16777215,0.75,6,0,0,0,0},
    {"map-bunker-hatchet-compartment-floor-02c.img",0.5,1,16777215,-14,0.5,0,0,0,0},
    {"map-bunker-hatchet-compartment-floor-02d.img",0.5,1,16777215,-6.27,14.25,0,0,0,0},
};
static const RawBImg kCeilImgs_923[] = {
    {"map-bunker-hatchet-compartment-ceiling-02.img",1,1,6250335,-0.5,-0.5,0,0,0,0},
};
static const RawCollider kZoomIns_923[] = {
    {1,-16.5,-15,15.5,15},
};
static const RawBImg kFloorImgs_924[] = {
    {"map-bunker-hatchet-compartment-floor-03a.img",0.5,1,16777215,-14.5,-8.5,0,0,0,0},
    {"map-bunker-hatchet-compartment-floor-03b.img",0.5,1,16777215,-9,3,0,0,0,0},
    {"map-bunker-hatchet-compartment-floor-03c.img",0.5,1,16777215,5.5,-0.25,0,0,0,0},
    {"map-bunker-hatchet-compartment-floor-03d.img",0.5,1,16777215,14.5,-3.75,0,0,0,0},
};
static const RawBImg kCeilImgs_924[] = {
    {"map-bunker-hatchet-compartment-ceiling-03.img",1,1,6250335,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_924[] = {
    {1,-19,-9.5,19,9.5},
};
static const RawShape kShapes_925[] = {
    {{1,-2,4.25,2,10.75},1,6946816},
};
static const RawBImg kFloorImgs_925[] = {
    {"map-bunker-generic-floor-01.img",0.5,1,16777215,0,7.5,0,0,0,0},
};
static const RawBImg kCeilImgs_925[] = {
    {"map-bunker-generic-ceiling-01.img",0.5,1,16777215,0,7.5,0,0,0,0},
};
static const RawCollider kZoomIns_925[] = {
    {1,-2,5,2,11.5},
};
static const RawBImg kFloorImgs_926[] = {
    {"map-bunker-eye-chamber-floor-01a.img",0.5,1,16777215,0,-8.5,0,0,0,0},
    {"map-bunker-eye-chamber-floor-01b.img",0.5,1,16777215,13,-23,0,0,0,0},
};
static const RawBImg kCeilImgs_926[] = {
    {"map-bunker-eye-chamber-ceiling-01.img",1,1,6250335,0,-12,0,0,0,0},
};
static const RawCollider kZoomIns_926[] = {
    {1,-14,-29,14,5},
};
static const RawBImg kFloorImgs_927[] = {
    {"map-bunker-eye-compartment-floor-01.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_927[] = {
    {"map-bunker-eye-compartment-ceiling-01.img",1,1,6250335,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_927[] = {
    {1,-10,-10,10,10},
};
static const RawShape kShapes_928[] = {
    {{1,-2,-2.25,2,4.25},1,10244368},
};
static const RawBImg kFloorImgs_928[] = {
    {"map-bunker-generic-floor-02.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_928[] = {
    {"map-bunker-twins-ceiling-01.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_928[] = {
    {1,-2,-2.5,2,4},
};
static const RawBImg kFloorImgs_929[] = {
    {"map-bunker-vent-02.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_930[] = {
    {"map-bunker-twins-chamber-floor-01.img",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kCeilImgs_930[] = {
    {"map-bunker-twins-chamber-ceiling-01.img",1,1,6250335,0,0,0,0,0,0},
};
static const RawCollider kZoomIns_930[] = {
    {1,-15.5,-10.5,15.5,10.5},
};
static const RawBImg kFloorImgs_931[] = {
    {"map-bunker-twins-compartment-floor-01.img",0.5,1,16777215,0,-2,0,0,0,0},
};
static const RawBImg kCeilImgs_931[] = {
    {"map-bunker-hydra-compartment-ceiling-03.img",1,1,6250335,0,0,0,1,0,0},
};
static const RawCollider kZoomIns_931[] = {
    {1,-10,-7,10,8.5},
};
static const RawBImg kFloorImgs_932[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_933[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_934[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_935[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_936[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_937[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_938[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_939[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_940[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_941[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_942[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_943[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_944[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_945[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_946[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_947[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_948[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_949[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_950[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_951[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_952[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_953[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_954[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_955[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_956[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_957[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_958[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_959[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_960[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_961[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_962[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_963[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_964[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_965[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_966[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_967[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawBImg kFloorImgs_968[] = {
    {"",0.5,1,16777215,0,0,0,0,0,0},
};
static const RawLayer kLayers_969[] = {
    {"bridge_lg_01",0,0,0,1,0},
    {"bridge_lg_under_01",0,0,0,1,0},
};
static const RawStair kStairs_969[] = {
    {{1,-11.5,-11,11.5,-8},0,1,0,1},
    {{1,-11.5,8,11.5,11},0,-1,0,1},
};
static const RawCollider kMask_969[] = {
    {1,-12,-8,12,8},
};
static const RawLayer kLayers_970[] = {
    {"bridge_xlg_01",0,0,0,1,0},
    {"bridge_xlg_under_01",0,0,0,1,0},
};
static const RawStair kStairs_970[] = {
    {{1,-11.5,-15,11.5,-12},0,1,0,1},
    {{1,-11.5,12,11.5,15},0,-1,0,1},
};
static const RawCollider kMask_970[] = {
    {1,-12,-12,12,12},
};
static const RawLayer kLayers_971[] = {
    {"bridge_md_01",0,0,0,1,0},
    {"bridge_md_under_01",0,0,0,1,0},
};
static const RawStair kStairs_971[] = {
    {{1,-5.5,-6,5.5,-3.5},0,1,0,1},
    {{1,-5.5,3.5,5.5,6},0,-1,0,1},
};
static const RawCollider kMask_971[] = {
    {1,-6.5,-3.6,6.5,3.6},
};
static const RawLayer kLayers_972[] = {
    {"statue_building_03",0,0,0,1,0},
    {"statue_underground_03",0,0,0,1,0},
};
static const RawStair kStairs_972[] = {
    {{1,-3.6,-2,1.6,2},1,0,0,0},
};
static const RawCollider kMask_972[] = {
    {1,1.7000000000000002,-4,9.7,4},
};
static const RawLayer kLayers_973[] = {
    {"statue_building_04",0,0,0,1,0},
    {"statue_underground_04",0,0,0,1,0},
};
static const RawStair kStairs_973[] = {
    {{1,-3.6,-2,1.6,2},1,0,0,0},
};
static const RawCollider kMask_973[] = {
    {1,1.7000000000000002,-4,9.7,4},
};
static const RawLayer kLayers_974[] = {
    {"barn_basement_stairs_01",0,0,0,1,0},
    {"barn_basement_floor_01",-10,-0.5,0,1,0},
};
static const RawStair kStairs_974[] = {
    {{1,-0.5,-2,3.5,5},0,-1,0,0},
};
static const RawCollider kMask_974[] = {
    {1,-24.5,-9.5,-0.5,7.5},
    {1,-0.4900000000000002,-10,7.51,-2},
};
static const RawLayer kLayers_975[] = {
    {"barn_basement_stairs_01",0,0,0,1,0},
    {"barn_basement_floor_01d",-10,-0.5,0,1,0},
};
static const RawStair kStairs_975[] = {
    {{1,-0.5,-2,3.5,5},0,-1,0,0},
};
static const RawCollider kMask_975[] = {
    {1,-24.5,-9.5,-0.5,7.5},
    {1,-0.4900000000000002,-10,7.51,-2},
};
static const RawLayer kLayers_976[] = {
    {"barn_basement_stairs_01",0,0,0,1,0},
    {"barn_basement_floor_01",-10,-0.5,0,1,0},
};
static const RawStair kStairs_976[] = {
    {{1,-0.5,-2,3.5,5},0,-1,0,0},
};
static const RawCollider kMask_976[] = {
    {1,-24.5,-9.5,-0.5,7.5},
    {1,-0.4900000000000002,-10,7.51,-2},
};
static const RawLayer kLayers_977[] = {
    {"mansion_01",0,0,0,1,0},
    {"mansion_cellar_01",0,0,0,1,0},
};
static const RawStair kStairs_977[] = {
    {{1,25,-1.0499999999999998,31,4.05},-1,0,1,0},
    {{1,-1,10,3,17},0,-1,0,0},
};
static const RawCollider kMask_977[] = {
    {1,-5,-10.2,25,10},
    {1,10.01,10.01,24.990000000000002,16.990000000000002},
};
static const RawLayer kLayers_978[] = {
    {"mansion_01x",0,0,0,1,0},
    {"mansion_cellar_01",0,0,0,1,0},
};
static const RawStair kStairs_978[] = {
    {{1,25,-1.0499999999999998,31,4.05},-1,0,1,0},
    {{1,-1,10,3,17},0,-1,0,0},
};
static const RawCollider kMask_978[] = {
    {1,-5,-10.2,25,10},
    {1,10.01,10.01,24.990000000000002,16.990000000000002},
};
static const RawLayer kLayers_979[] = {
    {"mansion_02",0,0,0,1,0},
    {"mansion_cellar_02",0,0,0,1,0},
};
static const RawStair kStairs_979[] = {
    {{1,25,-1.0499999999999998,31,4.05},-1,0,1,0},
    {{1,-1,10,3,17},0,-1,0,0},
};
static const RawCollider kMask_979[] = {
    {1,-5,-10.2,25,10},
    {1,10.01,10.01,24.990000000000002,16.990000000000002},
};
static const RawLayer kLayers_980[] = {
    {"mansion_03",0,0,0,1,0},
    {"mansion_cellar_03",0,0,0,1,0},
};
static const RawStair kStairs_980[] = {
    {{1,25,-1.0499999999999998,31,4.05},-1,0,1,0},
    {{1,-1,10,3,17},0,-1,0,0},
};
static const RawCollider kMask_980[] = {
    {1,-5,-10.2,25,10},
    {1,10.01,10.01,24.990000000000002,16.990000000000002},
};
static const RawLayer kLayers_981[] = {
    {"reserve_01",0,0,0,1,0},
    {"reserve_basement_01",14.5,-2,0,1,0},
};
static const RawStair kStairs_981[] = {
    {{1,45,5.5,58,16.5},0,-1,0,0},
    {{1,-25,21.5,-18,25.5},1,0,0,0},
};
static const RawCollider kMask_981[] = {
    {1,-45,-34.5,75,5.5},
    {1,-18,5.5,38,33.5},
    {1,-45,5.5,-18,13.5},
};
static const RawLayer kLayers_982[] = {
    {"saloon_01",0,0,0,1,0},
    {"saloon_cellar_01",-19,-6,0,1,0},
};
static const RawStair kStairs_982[] = {
    {{1,-21,-1.25,-18,2.75},-1,0,0,0},
};
static const RawCollider kMask_982[] = {
    {1,-40,-4.25,-20,5.75},
};
static const RawLayer kLayers_983[] = {
    {"club_01",-3.5,-17.5,0,1,0},
    {"bathhouse_01",0,0,0,1,0},
};
static const RawStair kStairs_983[] = {
    {{1,-36,-28.55,-30,-23.45},1,0,1,0},
    {{1,21,-31.5,25,-25.5},0,-1,0,0},
};
static const RawCollider kMask_983[] = {
    {1,-30,-42,20,58},
    {1,20.01,-39.5,26.01,-31.5},
};
static const RawLayer kLayers_984[] = {
    {"bunker_egg_01",0,0,0,1,0},
    {"bunker_egg_sublevel_01",0,0,0,1,0},
};
static const RawStair kStairs_984[] = {
    {{1,-2,5.800000000000001,2,11},0,-1,0,0},
};
static const RawCollider kMask_984[] = {
    {1,-10,-13.2,10,5.8},
};
static const RawLayer kLayers_985[] = {
    {"bunker_egg_01",0,0,0,1,0},
    {"bunker_egg_sublevel_02",0,0,0,1,0},
};
static const RawStair kStairs_985[] = {
    {{1,-2,5.800000000000001,2,11},0,-1,0,0},
};
static const RawCollider kMask_985[] = {
    {1,-10,-13.2,10,5.8},
};
static const RawLayer kLayers_986[] = {
    {"bunker_egg_01",0,0,0,1,0},
    {"bunker_egg_sublevel_01sv",0,0,0,1,0},
};
static const RawStair kStairs_986[] = {
    {{1,-2,5.800000000000001,2,11},0,-1,0,0},
};
static const RawCollider kMask_986[] = {
    {1,-10,-13.2,10,5.8},
};
static const RawLayer kLayers_987[] = {
    {"bunker_hydra_01",0,0,0,1,0},
    {"bunker_hydra_sublevel_01",0,0,0,1,0},
};
static const RawStair kStairs_987[] = {
    {{1,13.799999999999999,1.5,19,5.5},-1,0,0,0},
    {{1,-18.5,-93.25,-14.5,-88.25},0,1,0,0},
    {{1,38,-52.85,42,-47.85},0,-1,0,0},
};
static const RawCollider kMask_987[] = {
    {1,-7.25,-27.2,14.25,12.8},
    {1,-20,-88.25,-10,-71.25},
    {1,27,-70.85,51,-52.85},
    {1,-19.99,-71.19,26.99,-27.210000000000004},
    {1,0.5,-82.2,20.5,-71.2},
};
static const RawLayer kLayers_988[] = {
    {"bunker_storm_01",0,0,0,1,0},
    {"bunker_storm_sublevel_01",0,0,0,1,0},
};
static const RawStair kStairs_988[] = {
    {{1,-2,5.800000000000001,2,11},0,-1,0,0},
};
static const RawCollider kMask_988[] = {
    {1,-9.5,-13.2,26.5,5.8},
};
static const RawLayer kLayers_989[] = {
    {"bunker_conch_01",0,0,0,1,0},
    {"bunker_conch_sublevel_01",0,0,0,1,0},
};
static const RawStair kStairs_989[] = {
    {{1,14.299999999999999,1.5,19.5,5.5},-1,0,0,0},
    {{1,42.3,-34.5,47.5,-30.5},-1,0,0,0},
};
static const RawCollider kMask_989[] = {
    {1,-17.2,-31.2,14.2,12.8},
    {1,14.25,-40,42.25,-24},
};
static const RawLayer kLayers_990[] = {
    {"bunker_crossing_01",0,0,0,1,0},
    {"bunker_crossing_sublevel_01",0,0,0,1,0},
};
static const RawStair kStairs_990[] = {
    {{1,33,26.5,38.2,30.5},-1,0,0,0},
    {{1,-38,16.4,-34,21.6},0,1,0,0},
    {{1,34,-15.6,38,-10.4},0,-1,0,0},
    {{1,-38.1,-24.5,-32.9,-20.5},1,0,0,0},
};
static const RawCollider kMask_990[] = {
    {1,-40.2,22,32.8,32},
    {1,-32.8,-26,40.2,-16},
    {1,-30,-15.95,30,21.95},
};
static const RawLayer kLayers_991[] = {
    {"bunker_hatchet_01",0,0,0,1,0},
    {"bunker_hatchet_sublevel_01",0,0,0,1,0},
};
static const RawStair kStairs_991[] = {
    {{1,-2,5.800000000000001,2,11},0,-1,0,0},
};
static const RawCollider kMask_991[] = {
    {1,-16,-13.2,10,5.8},
    {1,-80.025,-18.95,-16.025,30.95},
};
static const RawLayer kLayers_992[] = {
    {"bunker_eye_01",0,0,0,1,0},
    {"bunker_eye_sublevel_01",0,0,0,1,0},
};
static const RawStair kStairs_992[] = {
    {{1,-2,5.800000000000001,2,11},0,-1,0,0},
};
static const RawCollider kMask_992[] = {
    {1,-13.5,-50.2,13.5,5.800000000000001},
};
static const RawLayer kLayers_993[] = {
    {"bunker_chrys_01",0,0,0,1,0},
    {"bunker_chrys_sublevel_01",0,0,0,1,0},
};
static const RawStair kStairs_993[] = {
    {{1,-1.5,-2.6,1.5,2.6},0,-1,0,0},
};
static const RawCollider kMask_993[] = {
    {1,-4.5,-21.75,25.5,-2.75},
    {1,25.55,-15,54.45,55},
};
static const RawLayer kLayers_994[] = {
    {"bunker_chrys_01",0,0,0,1,0},
    {"bunker_chrys_sublevel_01b",0,0,0,1,0},
};
static const RawStair kStairs_994[] = {
    {{1,-1.5,-2.6,1.5,2.6},0,-1,0,0},
};
static const RawCollider kMask_994[] = {
    {1,-4.5,-21.75,25.5,-2.75},
    {1,25.55,-15,54.45,55},
};
static const RawLayer kLayers_995[] = {
    {"bunker_twins_01",0,0,0,1,0},
    {"bunker_twins_sublevel_01",0,0,0,1,0},
};
static const RawStair kStairs_995[] = {
    {{1,-1,11.8,3,17},0,-1,0,0},
    {{1,-3,-17,1,-11.8},0,1,0,0},
    {{1,16.9,-2,22.1,2},-1,0,0,0},
    {{1,-22.1,-2,-16.9,2},1,0,0,0},
};
static const RawCollider kMask_995[] = {
    {1,-16.75,-11.75,16.75,11.75},
};
static const RawLayer kLayers_996[] = {
    {"bunker_cloud_01",0,0,0,1,0},
    {"bunker_cloud_sublevel_01",1.5,-39,0,1,0},
};
static const RawStair kStairs_996[] = {
    {{1,-2,-1.7000000000000002,2,3.5},0,-1,0,0},
    {{1,-30.5,-80.5,-26.5,-75.30000000000001},0,1,0,0},
};
static const RawCollider kMask_996[] = {
    {1,-40,-75.3,40,-1.7000000000000028},
    {1,-19,-85.31,39,-75.31},
};
static const char* const kParticleImgs_0[] = {"part-panel-01.img"};
static const char* const kParticleImgs_169[] = {"part-splat-01.img","part-splat-02.img","part-splat-03.img"};
static const char* const kParticleImgs_325[] = {"part-plank-01.img"};
static const char* const kParticleImgs_510[] = {"part-spark-02.img"};
static const char* const kParticleImgs_667[] = {"part-spark-02.img"};
static const char* const kParticleImgs_820[] = {"part-woodchip-01.img"};
static const char* const kParticleImgs_1002[] = {"part-spark-02.img"};
static const char* const kParticleImgs_1155[] = {"part-book-01.img"};
static const char* const kParticleImgs_1334[] = {"part-spark-02.img"};
static const char* const kParticleImgs_1528[] = {"part-spark-02.img"};
static const char* const kParticleImgs_1724[] = {"part-spark-02.img"};
static const char* const kParticleImgs_1915[] = {"part-spark-02.img"};
static const char* const kParticleImgs_2113[] = {"part-spark-02.img"};
static const char* const kParticleImgs_2311[] = {"part-spark-02.img"};
static const char* const kParticleImgs_2504[] = {"part-spark-02.img"};
static const char* const kParticleImgs_2661[] = {"part-cloth-01.img"};
static const char* const kParticleImgs_2815[] = {"part-cloth-01.img"};
static const char* const kParticleImgs_2972[] = {"part-plate-01.img"};
static const char* const kParticleImgs_3149[] = {"part-plate-01.img"};
static const char* const kParticleImgs_3327[] = {"part-plate-01.img"};
static const char* const kParticleImgs_3507[] = {"part-spark-02.img"};
static const char* const kParticleImgs_3695[] = {"part-plank-01.img"};
static const char* const kParticleImgs_3877[] = {"part-spark-02.img"};
static const char* const kParticleImgs_4034[] = {"part-spark-02.img"};
static const char* const kParticleImgs_4217[] = {"part-spark-02.img"};
static const char* const kParticleImgs_4401[] = {"part-spark-02.img"};
static const char* const kParticleImgs_4586[] = {"part-spark-02.img"};
static const char* const kParticleImgs_4743[] = {"part-plank-01.img"};
static const char* const kParticleImgs_4926[] = {"part-spark-02.img","part-plate-01.img","part-panel-01.img"};
static const char* const kParticleImgs_5122[] = {"part-panel-01.img"};
static const char* const kParticleImgs_5292[] = {"part-leaf-01.img"};
static const char* const kParticleImgs_5470[] = {"part-leaf-01.img"};
static const char* const kParticleImgs_5656[] = {"part-leaf-01sv.img"};
static const char* const kParticleImgs_5842[] = {"part-leaf-02.img"};
static const char* const kParticleImgs_6026[] = {"part-plate-01.img"};
static const char* const kParticleImgs_6195[] = {"part-woodchip-01.img"};
static const char* const kParticleImgs_6380[] = {"part-woodchip-01.img"};
static const char* const kParticleImgs_6566[] = {"part-panel-01.img"};
static const char* const kParticleImgs_6740[] = {"part-plank-01.img"};
static const char* const kParticleImgs_6925[] = {"part-spark-02.img"};
static const char* const kParticleImgs_7081[] = {"part-pot-01.img"};
static const char* const kParticleImgs_7233[] = {"part-spark-02.img"};
static const char* const kParticleImgs_7391[] = {"part-pumpkin-01.img"};
static const char* const kParticleImgs_7545[] = {"part-spark-02.img"};
static const char* const kParticleImgs_7707[] = {"part-spark-02.img"};
static const char* const kParticleImgs_7868[] = {"part-pumpkin-01.img"};
static const char* const kParticleImgs_8026[] = {"part-pumpkin-01.img"};
static const char* const kParticleImgs_8183[] = {"part-spark-02.img"};
static const char* const kParticleImgs_8343[] = {"part-pumpkin-01.img"};
static const char* const kParticleImgs_8499[] = {"part-spark-02.img"};
static const char* const kParticleImgs_8657[] = {"part-pumpkin-01.img"};
static const char* const kParticleImgs_8811[] = {"part-spark-02.img"};
static const char* const kParticleImgs_8966[] = {"part-spark-02.img"};
static const char* const kParticleImgs_9117[] = {"part-plank-01.img"};
static const char* const kParticleImgs_9297[] = {"map-stone-01.img"};
static const char* const kParticleImgs_9454[] = {"map-stone-01.img"};
static const char* const kParticleImgs_9607[] = {"map-stone-01.img"};
static const char* const kParticleImgs_9766[] = {"map-stone-01.img"};
static const char* const kParticleImgs_9922[] = {"part-panel-01.img"};
static const char* const kParticleImgs_10093[] = {"part-panel-01.img"};
static const char* const kParticleImgs_10270[] = {"part-woodchip-01.img"};
static const char* const kParticleImgs_10452[] = {"part-panel-01.img"};
static const char* const kParticleImgs_10627[] = {"part-panel-01.img"};
static const char* const kParticleImgs_10806[] = {"part-spark-02.img"};
static const char* const kParticleImgs_10962[] = {"part-spark-02.img"};
static const char* const kParticleImgs_11126[] = {"part-spark-02.img"};
static const char* const kParticleImgs_11286[] = {"part-spark-02.img"};
static const char* const kParticleImgs_11446[] = {"part-feather-01.img","part-feather-02.img"};
static const char* const kParticleImgs_11624[] = {"part-feather-01.img","part-feather-02.img"};
static const char* const kParticleImgs_11805[] = {"part-spark-02.img"};
static const char* const kParticleImgs_11964[] = {"part-plank-01.img"};
static const char* const kParticleImgs_12148[] = {"part-spark-02.img"};
static const char* const kParticleImgs_12340[] = {"part-woodchip-01.img"};
static const char* const kParticleImgs_12523[] = {"part-log-01.img"};
static const char* const kParticleImgs_12703[] = {"part-plank-01.img"};
static const char* const kParticleImgs_12885[] = {"part-spark-02.img"};
static const char* const kParticleImgs_13068[] = {"part-shell-01.img"};
static const char* const kParticleImgs_13259[] = {"part-shell-01.img"};
static const char* const kParticleImgs_13457[] = {"part-shell-02.img"};
static const char* const kParticleImgs_13651[] = {"part-shell-04.img"};
static const char* const kParticleImgs_13845[] = {"part-shell-03.img"};
static const char* const kParticleImgs_14030[] = {"part-shell-01.img"};
static const char* const kParticleImgs_14222[] = {"part-shell-06.img"};
static const char* const kParticleImgs_14415[] = {"part-shell-05.img"};
static const char* const kParticleImgs_14609[] = {"part-shell-03.img"};
static const char* const kParticleImgs_14792[] = {"part-shell-01.img"};
static const char* const kParticleImgs_14977[] = {"part-wedge-01.img"};
static const char* const kParticleImgs_15168[] = {"part-note-02.img"};
static const char* const kParticleImgs_15357[] = {"part-frag-pin-01.img"};
static const char* const kParticleImgs_15514[] = {"part-frag-lever-01.img"};
static const char* const kParticleImgs_15707[] = {"part-frag-burst-01.img"};
static const char* const kParticleImgs_15858[] = {"part-frag-burst-01.img"};
static const char* const kParticleImgs_16008[] = {"part-smoke-01.img"};
static const char* const kParticleImgs_16199[] = {"part-frag-burst-01.img"};
static const char* const kParticleImgs_16349[] = {"part-frag-burst-03.img"};
static const char* const kParticleImgs_16501[] = {"part-frag-burst-02.img"};
static const char* const kParticleImgs_16651[] = {"part-frag-burst-01.img"};
static const char* const kParticleImgs_16803[] = {"part-frag-burst-01.img"};
static const char* const kParticleImgs_16958[] = {"part-smoke-02.img","part-smoke-03.img"};
static const char* const kParticleImgs_17151[] = {"part-airdrop-01.img"};
static const char* const kParticleImgs_17346[] = {"part-airdrop-01h.img"};
static const char* const kParticleImgs_17542[] = {"part-airdrop-01x.img"};
static const char* const kParticleImgs_17738[] = {"part-airdrop-02.img"};
static const char* const kParticleImgs_17920[] = {"part-airdrop-02h.img"};
static const char* const kParticleImgs_18103[] = {"part-airdrop-02x.img"};
static const char* const kParticleImgs_18286[] = {"part-airdrop-03.img"};
static const char* const kParticleImgs_18481[] = {"part-airdrop-04.img"};
static const char* const kParticleImgs_18663[] = {"part-class-shell-01a.img"};
static const char* const kParticleImgs_18857[] = {"part-class-shell-01b.img"};
static const char* const kParticleImgs_19038[] = {"part-class-shell-02a.img"};
static const char* const kParticleImgs_19232[] = {"part-class-shell-02b.img"};
static const char* const kParticleImgs_19413[] = {"part-class-shell-03a.img"};
static const char* const kParticleImgs_19607[] = {"part-class-shell-03b.img"};
static const char* const kParticleImgs_19788[] = {"part-smoke-02.img","part-smoke-03.img"};
static const char* const kParticleImgs_19988[] = {"part-smoke-02.img","part-smoke-03.img"};
static const char* const kParticleImgs_20192[] = {"player-ripple-01.img"};
static const char* const kParticleImgs_20395[] = {"player-ripple-01.img"};
static const char* const kParticleImgs_20551[] = {"part-leaf-03.img","part-leaf-04.img","part-leaf-05.img","part-leaf-06.img"};
static const char* const kParticleImgs_20744[] = {"part-leaf-03.img","part-leaf-04.img","part-leaf-05.img","part-leaf-06.img"};
static const char* const kParticleImgs_20939[] = {"part-blossom-01.img","part-blossom-02.img","part-blossom-03.img","part-blossom-04.img"};
static const char* const kParticleImgs_21132[] = {"part-leaf-06.img"};
static const char* const kParticleImgs_21325[] = {"part-blossom-01.img","part-blossom-02.img","part-blossom-03.img","part-blossom-04.img","part-potato-02.img"};
static const char* const kParticleImgs_21518[] = {"part-potato-02.img"};
static const char* const kParticleImgs_21707[] = {"part-potato-02.img","part-tomato-02.img"};
static const char* const kParticleImgs_21905[] = {"part-snow-01.img"};
static const char* const kParticleImgs_22091[] = {"part-snow-01.img"};
static const char* const kParticleImgs_22286[] = {"part-potato-01.img"};
static const char* const kParticleImgs_22479[] = {"part-potato-01.img"};
static const char* const kParticleImgs_22676[] = {"part-coconut-01.img","part-coconut-02.img","part-coconut-03.img"};
static const char* const kParticleImgs_22870[] = {"part-tomato-01.img"};
static const char* const kParticleImgs_23063[] = {"part-heal-basic.img"};
static const char* const kParticleImgs_23228[] = {"part-heal-heart.img"};
static const char* const kParticleImgs_23393[] = {"part-heal-moon.img"};
static const char* const kParticleImgs_23591[] = {"part-heal-tomoe.img"};
static const char* const kParticleImgs_23789[] = {"part-heal-diamond.img"};
static const char* const kParticleImgs_23956[] = {"part-heal-ankh.img"};
static const char* const kParticleImgs_24153[] = {"part-heal-menacing.img"};
static const char* const kParticleImgs_24321[] = {"part-boost-basic.img"};
static const char* const kParticleImgs_24515[] = {"part-boost-star.img"};
static const char* const kParticleImgs_24708[] = {"part-boost-naturalize.img"};
static const char* const kParticleImgs_24906[] = {"part-boost-shuriken.img"};
static const char* const kParticleImgs_25101[] = {"part-boost-club.img"};
static const char* const kParticleImgs_25294[] = {"part-boost-lightning.img"};
static const char* const kParticleImgs_25492[] = {"part-boost-hermes.img"};
static const char* const kParticleImgs_25687[] = {"part-boost-gearshift-01.img"};
static const char* const kParticleImgs_25854[] = {"part-boost-gearshift-02.img"};
static const char* const kParticleImgs_26053[] = {"part-heal-basic.img"};
static const char* const kParticleImgs_26220[] = {"part-blossom-01.img","part-blossom-02.img","part-blossom-03.img","part-blossom-04.img"};
static const char* const kParticleImgs_26406[] = {"part-takedown-01.img"};
static const char* const kParticleImgs_26599[] = {"part-note-01.img"};
static const char* const kParticleImgs_26791[] = {"part-boost-basic.img"};
static const char* const kParticleImgs_26984[] = {"part-boost-basic.img"};
static const char* const kParticleImgs_27175[] = {"part-boost-basic.img"};

static const RawMapObj kMapObjs[] = {
    {"house_door_01",{"map-door-01.img",0.5,1,14671839,15,0,0,0},1,{1,-0.3,0,0.3,4},1,0,0,0,0,0,0,"",3.5,"",0,0,1,16777215,1,0.5,1,1,0,0,1,{1,-0.3,0,0.3,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"house_door_02",{"map-door-01.img",0.5,1,4934475,15,0,0,0},1,{1,-0.3,0,0.3,4},1,0,0,0,0,0,0,"",3.5,"",0,0,1,16777215,1,0.5,1,1,0,0,1,{1,-0.3,0,0.3,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"house_door_03",{"map-door-03.img",0.5,1,14671839,15,0,0,0},1,{1,-0.5,0.25,0.5,3.75},1,0,0,0,0,0,0,"",3.5,"",0,0,1,16777215,1,0.5,1,1,0,0,1,{1,-0.5,0.25,0.5,3.75},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"house_door_05",{"map-door-05.img",0.5,1,14671839,15,0,0,0},1,{1,-0.3,0,0.3,4},1,0,0,0,0,0,0,"",3.5,"",0,0,1,16777215,1,0.5,1,1,0,0,1,{1,-0.3,0,0.3,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crossing_door_01",{"map-door-01.img",0.5,1,3159362,15,0,0,0},1,{1,-0.3,0,0.3,4},1,0,0,0,0,0,0,"",3.5,"",0,0,1,16777215,1,0.5,1,1,0,0,1,{1,-0.3,0,0.3,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cell_door_01",{"map-door-01.img",0.5,1,1776411,15,0,0,0},1,{1,-0.3,0,0.3,4},1,0,0,0,0,0,0,"",3.5,"",0,0,1,16777215,1,0.5,1,1,0,0,1,{1,-0.3,0,0.3,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"eye_door_01",{"map-door-01.img",0.5,1,921102,15,0,0,0},1,{1,-0.3,0,0.3,4},1,0,0,0,0,0,0,"",3.5,"",0,0,1,16777215,1,0.5,1,1,0,0,1,{1,-0.3,0,0.3,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"lab_door_01",{"map-door-01.img",0.5,1,5373952,15,0,0,0},1,{1,-0.3,0,0.3,4},1,0,0,0,0,0,0,"",3.75,"map-door-slot-01.img",-2,0,0.5,1316379,1,0.5,1,1,0,0,1,{1,-0.3,0,0.3,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"lab_door_02",{"map-door-01.img",0.5,1,5373952,15,0,0,0},1,{1,-0.3,0,0.3,4},1,0,0,0,0,0,0,"",-3.75,"map-door-slot-01.img",6,0,0.5,1316379,1,0.5,1,1,0,0,1,{1,-0.3,0,0.3,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"lab_door_03",{"map-door-01.img",0.5,1,5373952,15,0,0,0},1,{1,-0.3,0,0.3,4},1,0,0,0,0,0,0,"",3.75,"map-door-slot-01.img",-2,0,0.5,1316379,1,0.5,1,1,0,0,1,{1,-0.3,0,0.3,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"lab_door_locked_01",{"map-door-01.img",0.5,1,5373952,15,0,0,0},1,{1,-0.3,0,0.3,4},1,0,0,0,0,0,0,"",3.75,"map-door-slot-01.img",-2,0,0.5,1316379,1,0.5,1,1,0,0,1,{1,-0.3,0,0.3,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"lab_door_chrys",{"map-door-01.img",0.5,1,5373952,15,0,0,0},1,{1,-0.3,0,0.3,4},1,0,0,0,0,0,0,"",3.75,"map-door-slot-01.img",-2,0,0.5,1316379,1,0.5,1,1,0,0,1,{1,-0.3,0,0.3,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"vault_door_main",{"map-door-02.img",0.5,1,14671839,15,0,0,0},1,{1,0,0,2,7},1,0,0,0,0,0,0,"",3.5,"",0,0,1,16777215,1,0.2,1,1,0,0,1,{1,0,0,2,7},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"vault_door_chrys_01",{"map-door-02.img",0.5,1,14671839,15,0,0,0},1,{1,0,0,2,7},1,0,0,0,0,0,0,"",3.5,"",0,0,1,16777215,1,0.2,1,1,0,0,1,{1,0,0,2,7},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"vault_door_chrys_02",{"map-door-02.img",0.5,1,14671839,15,0,0,0},1,{1,0,0,2,7},1,0,0,0,0,0,0,"",3.5,"",0,0,1,16777215,1,0.2,1,1,0,0,1,{1,0,0,2,7},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"vault_door_reserve",{"map-door-06.img",0.5,1,14671839,15,0,0,0},1,{1,0,0,2,10},1,0,0,0,0,0,0,"",3.5,"",0,0,1,16777215,1,0.2,1,1,0,0,1,{1,0,0,2,10},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"vault_door_eye",{"map-door-02.img",0.5,1,14671839,15,0,0,0},1,{1,0,0,2,7},1,0,0,0,0,0,0,"",3.5,"",0,0,1,16777215,1,0.2,1,1,0,0,1,{1,0,0,2,7},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"saloon_door_secret",{"map-door-04.img",0.5,1,16777215,9,0,0,0},1,{1,-0.75,0,0.75,4},1,0,0,0,0,0,0,"",4.5,"",0,0,1,16777215,1,0.5,1,1,0,0,1,{1,-0.75,0,0.75,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"teahouse_door_01",{"map-door-01.img",0.5,0.95,14537141,15,0,0,0},1,{1,-0.3,0,0.3,4},1,0,0,0,0,0,0,"",3.75,"map-door-slot-02.img",-2,0,0.5,3211264,1,0.5,1,1,0,0,1,{1,-0.3,0,0.3,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"secret_door_club",{"map-door-01.img",0.5,1,5373952,15,0,0,0},1,{1,-0.3,0,0.3,4},1,0,0,0,0,0,0,"",3.75,"map-door-slot-01.img",-2,0,0.5,1316379,1,0.5,1,1,0,0,1,{1,-0.3,0,0.3,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"vault_door_bathhouse",{"map-door-01.img",0.5,1,4934475,15,0,0,0},1,{1,-0.3,0,0.3,4},1,0,0,0,0,0,0,"",3.75,"map-door-slot-01.img",-2,0,0.5,1316379,1,0.5,1,1,0,0,1,{1,-0.3,0,0.3,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"house_window_01",{"map-building-house-window-01.img",0.5,1,16777215,10,0,0,0},1,{1,-0.4,-2,0.4,2},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-0.4,-2,0.4,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"house_window_broken_01",{"map-building-house-window-res-01.img",0.5,1,4456448,10,0,0,0},1,{1,-0.4,-2,0.4,2},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-0.4,-2,0.4,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"lab_window_01",{"map-building-house-window-01.img",0.5,1,16777215,10,0,0,0},1,{1,-0.4,-2,0.4,2},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-0.4,-2,0.4,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"lab_window_broken_01",{"map-building-house-window-res-01.img",0.5,1,1316379,10,0,0,0},1,{1,-0.4,-2,0.4,2},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-0.4,-2,0.4,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stairs_01",{"map-stairs-broken-01.img",0.5,1,16777215,60,0,0,0},1,{1,-2.5,-2,2.5,2},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.5,-2,2.5,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stairs_02",{"map-stairs-broken-02.img",0.5,1,16777215,60,0,0,0},1,{1,-2.5,-4,2.5,4},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.5,-4,2.5,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stairs_03",{"map-stairs-broken-03.img",0.5,1,16777215,60,0,0,0},1,{1,-2.5,-2,2.5,2},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.5,-2,2.5,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"club_window_01",{"map-building-boarded-window-01.img",0.5,1,16777215,10,0,0,0},1,{1,-0.4,-2,0.4,2},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-0.4,-2,0.4,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"club_window_broken_01",{"map-building-house-window-res-01.img",0.5,1,7886127,10,0,0,0},1,{1,-0.4,-2,0.4,2},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-0.4,-2,0.4,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bank_window_01",{"map-building-bank-window-01.img",0.5,1,16777215,10,0,0,0},1,{1,-0.4,-2,0.4,2},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-0.4,-2,0.4,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_window_01",{"map-building-reserve-window-01.img",0.5,1,16777215,10,0,0,0},1,{1,-0.4,-3.5,0.4,3.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-0.4,-3.5,0.4,3.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_window_broken_01",{"map-building-reserve-window-res-01.img",0.5,1,1316379,10,0,0,0},1,{1,-0.4,-4,0.4,4},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-0.4,-4,0.4,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"container_05_collider",{"",1,1,16777215,0,0,0,0},0,{1,-2.75,-6,2.75,6},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-2.75,-6,2.75,6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"hedgehog_wall",{"",1,1,16777215,0,0,0,0},0,{1,-3,-0.5,3,0.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,5854285,1,{1,-3,-0.5,3,0.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"hut_wall_int_4",{"map-wall-04.img",0.5,1,4608000,10,0,0,0},1,{1,-0.5,-2,0.5,2},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-2,0.5,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"hut_wall_int_5",{"map-wall-05.img",0.5,1,4608000,10,0,0,0},1,{1,-0.5,-2.5,0.5,2.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-2.5,0.5,2.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"hut_wall_int_6",{"map-wall-06.img",0.5,1,4608000,10,0,0,0},1,{1,-0.5,-3,0.5,3},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-3,0.5,3},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"hut_wall_int_7",{"map-wall-07.img",0.5,1,4608000,10,0,0,0},1,{1,-0.5,-3.5,0.5,3.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-3.5,0.5,3.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"hut_wall_int_10",{"map-wall-10.img",0.5,1,4608000,10,0,0,0},1,{1,-0.5,-5,0.5,5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-5,0.5,5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"hut_wall_int_12",{"map-wall-12.img",0.5,1,4608000,10,0,0,0},1,{1,-0.5,-6,0.5,6},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-6,0.5,6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"hut_wall_int_14",{"map-wall-14.img",0.5,1,4608000,10,0,0,0},1,{1,-0.5,-7,0.5,7},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-7,0.5,7},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"hut_window_open_01",{"map-building-house-window-res-01.img",0.5,1,7681026,10,0,0,0},1,{1,-0.4,-2,0.4,2},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-0.4,-2,0.4,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"warehouse_wall_side",{"",1,1,16777215,0,0,0,0},0,{1,-25,-0.6,25,0.6},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-25,-0.6,25,0.6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"warehouse_wall_edge",{"",1,1,16777215,0,0,0,0},0,{1,-0.6,-3.2,0.6,3.2},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.6,-3.2,0.6,3.2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"warehouse_wall_edge_2",{"",1,1,16777215,0,0,0,0},0,{1,-0.6,-6.5,0.6,6.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.6,-6.5,0.6,6.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"warehouse_wall_int",{"",1,1,16777215,0,0,0,0},0,{1,-0.6,-1,0.6,1},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.6,-1,0.6,1},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"warehouse_column",{"",1,1,16777215,0,0,0,0},0,{1,-0.6,-0.6,0.6,0.6},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.6,-0.6,0.6,0.6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"workshop_wall_right",{"",1,1,16777215,0,0,0,0},0,{1,-20,-0.5,20,0.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-20,-0.5,20,0.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"workshop_wall_edge",{"",1,1,16777215,0,0,0,0},0,{1,-4,-0.5,4,0.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-4,-0.5,4,0.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"workshop_wall_mid_1",{"",1,1,16777215,0,0,0,0},0,{1,-7.25,-0.5,7.25,0.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-7.25,-0.5,7.25,0.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"workshop_wall_mid_2",{"",1,1,16777215,0,0,0,0},0,{1,-8,-0.5,8,0.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-8,-0.5,8,0.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"workshop_wall_mid_3",{"",1,1,16777215,0,0,0,0},0,{1,-1.25,-0.5,1.25,0.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.25,-0.5,1.25,0.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"workshop_wall_bot",{"",1,1,16777215,0,0,0,0},0,{1,-8.75,-0.5,8.75,0.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-8.75,-0.5,8.75,0.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"workshop_wall_room_1",{"",1,1,16777215,0,0,0,0},0,{1,-4.25,-0.5,4.25,0.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-4.25,-0.5,4.25,0.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"workshop_wall_room_2",{"",1,1,16777215,0,0,0,0},0,{1,-2.25,-0.5,2.25,0.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-2.25,-0.5,2.25,0.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"workshop_wall_room_3",{"",1,1,16777215,0,0,0,0},0,{1,-4.5,-0.5,4.5,0.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-4.5,-0.5,4.5,0.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"workshop_wall_room_4",{"",1,1,16777215,0,0,0,0},0,{1,-2.75,-0.5,2.75,0.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-2.75,-0.5,2.75,0.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"workshop_wall_left",{"",1,1,16777215,0,0,0,0},0,{1,-15.5,-0.5,15.5,0.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-15.5,-0.5,15.5,0.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cobalt_wall_int_4",{"map-wall-04-cobalt.img",0.5,1,16777215,10,0,0,0},1,{1,-0.6,-2,0.6,2},1,0,0,0,1,0,1,"explosion_cobalt",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.6,-2,0.6,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"archway_column_1",{"map-column-01.img",0.5,1,7290644,10,0,0,0},1,{1,-1,-1,1,1},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1,-1,1,1},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"shack_wall_top",{"map-wall-shack-top.img",0.5,1,16777215,10,0,0,0},1,{1,-5.6,-0.35,5.6,0.35},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-5.6,-0.35,5.6,0.35},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"shack_wall_side_left",{"map-wall-shack-left.img",0.5,1,16777215,10,0,0,0},1,{1,-0.35,-3.43,0.35,3.43},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.35,-3.43,0.35,3.43},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"shack_wall_side_right",{"map-wall-shack-right.img",0.5,1,16777215,10,0,0,0},1,{1,-0.35,-3.8,0.35,3.8},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.35,-3.8,0.35,3.8},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"shack_wall_bot",{"map-wall-shack-bot.img",0.5,1,16777215,10,0,0,0},1,{1,-3.75,-0.35,3.75,0.35},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-3.75,-0.35,3.75,0.35},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"shack_wall_ext_2",{"map-wall-02.img",0.5,1,12556639,10,0,0,0},1,{1,-0.5,-1,0.5,1},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-1,0.5,1},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"shack_wall_ext_5",{"map-wall-05.img",0.5,1,12556639,10,0,0,0},1,{1,-0.5,-2.5,0.5,2.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-2.5,0.5,2.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"shack_wall_ext_9",{"map-wall-09.img",0.5,1,12556639,10,0,0,0},1,{1,-0.5,-4.5,0.5,4.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-4.5,0.5,4.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"shack_wall_ext_10",{"map-wall-10.img",0.5,1,12556639,10,0,0,0},1,{1,-0.5,-5,0.5,5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-5,0.5,5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"shack_wall_ext_14",{"map-wall-14.img",0.5,1,12556639,10,0,0,0},1,{1,-0.5,-7,0.5,7},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-7,0.5,7},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"outhouse_wall_top",{"map-wall-outhouse-top.img",0.5,1,16777215,10,0,0,0},1,{1,-3.2,-0.35,3.2,0.35},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-3.2,-0.35,3.2,0.35},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"outhouse_wall_side",{"map-wall-outhouse-side.img",0.5,1,16777215,10,0,0,0},1,{1,-0.35,-3.1,0.35,3.1},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.35,-3.1,0.35,3.1},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"outhouse_wall_bot",{"map-wall-outhouse-bot.img",0.5,1,16777215,10,0,0,0},1,{1,-1.15,-0.35,1.15,0.35},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.15,-0.35,1.15,0.35},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_1",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-0.5,0.5,0.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-0.5,0.5,0.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_2",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-1,0.5,1},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-1,0.5,1},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_3",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-1.5,0.5,1.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-1.5,0.5,1.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_4",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-2,0.5,2},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-2,0.5,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_5",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-2.5,0.5,2.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-2.5,0.5,2.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_6",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-3,0.5,3},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-3,0.5,3},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_7",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-3.5,0.5,3.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-3.5,0.5,3.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_8",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-4,0.5,4},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-4,0.5,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_9",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-4.5,0.5,4.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-4.5,0.5,4.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_10",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-5,0.5,5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-5,0.5,5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_11",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-5.5,0.5,5.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-5.5,0.5,5.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_12",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-6,0.5,6},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-6,0.5,6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_12_5",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-6.25,0.5,6.25},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-6.25,0.5,6.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_13",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-6.5,0.5,6.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-6.5,0.5,6.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_14",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-7,0.5,7},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-7,0.5,7},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_15",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-7.5,0.5,7.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-7.5,0.5,7.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_16",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-8,0.5,8},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-8,0.5,8},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_17",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-8.5,0.5,8.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-8.5,0.5,8.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_18",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-9,0.5,9},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-9,0.5,9},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_19",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-9.5,0.5,9.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-9.5,0.5,9.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_20",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-10,0.5,10},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-10,0.5,10},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_21",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-10.5,0.5,10.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-10.5,0.5,10.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_23",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-11.5,0.5,11.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-11.5,0.5,11.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_33",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-16.5,0.5,16.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-16.5,0.5,16.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_41",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-20.5,0.5,20.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-20.5,0.5,20.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_short_7",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-3.5,0.5,3.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-3.5,0.5,3.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_thicker_4",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-2,1.5,2},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-2,1.5,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_thicker_5",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-2.5,1.5,2.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-2.5,1.5,2.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_thicker_6",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-3,1.5,3},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-3,1.5,3},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_thicker_7",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-3.5,1.5,3.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-3.5,1.5,3.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_thicker_8",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-4,1.5,4},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-4,1.5,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_thicker_9",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-4.5,1.5,4.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-4.5,1.5,4.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_thicker_15",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-7.5,1.5,7.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-7.5,1.5,7.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_thicker_16",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-8,1.5,8},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-8,1.5,8},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_thicker_24",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-12,1.5,12},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-12,1.5,12},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_thin_6",{"",1,1,16777215,0,0,0,0},0,{1,-0.375,-3,0.375,3},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.375,-3,0.375,3},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_1_5",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-0.75,0.5,0.75},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-0.75,0.5,0.75},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_2",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-1,0.5,1},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-1,0.5,1},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_3",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-1.5,0.5,1.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-1.5,0.5,1.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_4",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-2,0.5,2},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-2,0.5,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_5",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-2.5,0.5,2.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-2.5,0.5,2.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_6",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-3,0.5,3},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-3,0.5,3},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_7",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-3.5,0.5,3.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-3.5,0.5,3.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_8",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-4,0.5,4},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-4,0.5,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_9",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-4.5,0.5,4.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-4.5,0.5,4.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_9_5",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-4.75,0.5,4.75},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-4.75,0.5,4.75},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_10_5",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-5.25,0.5,5.25},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-5.25,0.5,5.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_11",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-5.5,0.5,5.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-5.5,0.5,5.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_11_5",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-5.75,0.5,5.75},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-5.75,0.5,5.75},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_13",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-6.5,0.5,6.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-6.5,0.5,6.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_14",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-7,0.5,7},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-7,0.5,7},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_15",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-7.5,0.5,7.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-7.5,0.5,7.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_16",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-8,0.5,8},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-8,0.5,8},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_17",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-8.5,0.5,8.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-8.5,0.5,8.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_23",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-11.5,0.5,11.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-11.5,0.5,11.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_24",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-12,0.5,12},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-12,0.5,12},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_25",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-12.5,0.5,12.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-12.5,0.5,12.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_column_2x8",{"",1,1,16777215,0,0,0,0},0,{1,-1,-4,1,4},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1,-4,1,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_column_4x8",{"",1,1,16777215,0,0,0,0},0,{1,-2,-4,2,4},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-2,-4,2,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_column_4x9",{"",1,1,16777215,0,0,0,0},0,{1,-2,-4.5,2,4.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-2,-4.5,2,4.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_column_4x24",{"",1,1,16777215,0,0,0,0},0,{1,-2,-12,2,12},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-2,-12,2,12},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_column_5x10",{"",1,1,16777215,0,0,0,0},0,{1,-2.5,-5,2.5,5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-2.5,-5,2.5,5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_column_7x10",{"",1,1,16777215,0,0,0,0},0,{1,-3.5,-5,3.5,5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-3.5,-5,3.5,5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_column_8x3",{"",1,1,16777215,0,0,0,0},0,{1,-4,-1.5,4,1.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-4,-1.5,4,1.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_thick_11",{"",1,1,16777215,0,0,0,0},0,{1,-1,-5.5,1,5.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1,-5.5,1,5.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_thicker_4",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-2,1.5,2},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-2,1.5,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_thicker_5",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-2.5,1.5,2.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-2.5,1.5,2.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_thicker_6",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-3,1.5,3},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-3,1.5,3},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_thicker_8",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-4,1.5,4},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-4,1.5,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_thicker_9",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-4.5,1.5,4.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-4.5,1.5,4.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_thicker_10",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-5,1.5,5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-5,1.5,5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_thicker_11",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-5.5,1.5,5.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-5.5,1.5,5.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_thicker_12",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-6,1.5,6},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-6,1.5,6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_thicker_13",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-6.5,1.5,6.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-6.5,1.5,6.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_thicker_14",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-7,1.5,7},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-7,1.5,7},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_thicker_15",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-7.5,1.5,7.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-7.5,1.5,7.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_thicker_17",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-8.5,1.5,8.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-8.5,1.5,8.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_thicker_19",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-9.5,1.5,9.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-9.5,1.5,9.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_thicker_21",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-10.5,1.5,10.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-10.5,1.5,10.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_thicker_22",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-11,1.5,11},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-11,1.5,11},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_thicker_27",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-13.5,1.5,13.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-13.5,1.5,13.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_thicker_30",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-15,1.5,15},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-15,1.5,15},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_thicker_31",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-15.5,1.5,15.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-15.5,1.5,15.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_thicker_42",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-21,1.5,21},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-21,1.5,21},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_ext_thicker_54",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-27,1.5,27},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-27,1.5,27},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_2x2",{"",1,1,16777215,0,0,0,0},0,{1,-1,-1,1,1},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1,-1,1,1},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_2",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-1,0.5,1},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-1,0.5,1},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_3",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-1.5,0.5,1.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-1.5,0.5,1.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_4",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-2,0.5,2},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-2,0.5,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_5",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-2.5,0.5,2.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-2.5,0.5,2.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_6",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-3,0.5,3},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-3,0.5,3},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_7",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-3.5,0.5,3.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-3.5,0.5,3.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_8",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-4,0.5,4},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-4,0.5,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_9",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-4.5,0.5,4.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-4.5,0.5,4.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_10",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-5,0.5,5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-5,0.5,5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_12",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-6,0.5,6},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-6,0.5,6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_12_5",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-6.25,0.5,6.25},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-6.25,0.5,6.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_13",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-6.5,0.5,6.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-6.5,0.5,6.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_1x15",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-7.5,0.5,7.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-7.5,0.5,7.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_16",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-8,0.5,8},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-8,0.5,8},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_18",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-9,0.5,9},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-9,0.5,9},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_23",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-11.5,0.5,11.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-11.5,0.5,11.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_43",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-21.5,0.5,21.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-21.5,0.5,21.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_short_6",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-3,0.5,3},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-3,0.5,3},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_short_7",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-3.5,0.5,3.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-3.5,0.5,3.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thick_5",{"",1,1,16777215,0,0,0,0},0,{1,-1,-2.5,1,2.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1,-2.5,1,2.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_2x5_5",{"",1,1,16777215,0,0,0,0},0,{1,-1,-2.75,1,2.75},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1,-2.75,1,2.75},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thick_6",{"",1,1,16777215,0,0,0,0},0,{1,-1,-3,1,3},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1,-3,1,3},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thick_8",{"",1,1,16777215,0,0,0,0},0,{1,-1,-4,1,4},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1,-4,1,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thick_12",{"",1,1,16777215,0,0,0,0},0,{1,-1,-6,1,6},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1,-6,1,6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thick_16",{"",1,1,16777215,0,0,0,0},0,{1,-1,-8,1,8},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1,-8,1,8},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thick_20",{"",1,1,16777215,0,0,0,0},0,{1,-1,-10,1,10},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1,-10,1,10},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thicker_4",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-2,1.5,2},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-2,1.5,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thick_23",{"",1,1,16777215,0,0,0,0},0,{1,-1,-11.5,1,11.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1,-11.5,1,11.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thick_28",{"",1,1,16777215,0,0,0,0},0,{1,-1,-14,1,14},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1,-14,1,14},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thicker_1_5",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-0.75,1.5,0.75},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-0.75,1.5,0.75},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thicker_5",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-2.5,1.5,2.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-2.5,1.5,2.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thicker_6",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-3,1.5,3},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-3,1.5,3},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thicker_7",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-3.5,1.5,3.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-3.5,1.5,3.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thicker_8",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-4,1.5,4},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-4,1.5,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thicker_9",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-4.5,1.5,4.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-4.5,1.5,4.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thicker_10",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-5,1.5,5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-5,1.5,5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thicker_11",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-5.5,1.5,5.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-5.5,1.5,5.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thicker_12",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-6,1.5,6},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-6,1.5,6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thicker_13",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-6.5,1.5,6.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-6.5,1.5,6.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thicker_14",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-7,1.5,7},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-7,1.5,7},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thicker_15",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-7.5,1.5,7.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-7.5,1.5,7.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thicker_16",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-8,1.5,8},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-8,1.5,8},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thicker_17",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-8.5,1.5,8.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-8.5,1.5,8.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thicker_18",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-9,1.5,9},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-9,1.5,9},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thicker_19",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-9.5,1.5,9.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-9.5,1.5,9.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thicker_20",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-10,1.5,10},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-10,1.5,10},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thicker_21",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-10.5,1.5,10.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-10.5,1.5,10.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thicker_22",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-11,1.5,11},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-11,1.5,11},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thicker_23",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-11.5,1.5,11.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-11.5,1.5,11.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thicker_24",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-12,1.5,12},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-12,1.5,12},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thicker_25",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-12.5,1.5,12.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-12.5,1.5,12.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thicker_26",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-13,1.5,13},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-13,1.5,13},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thicker_27",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-13.5,1.5,13.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-13.5,1.5,13.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thicker_28",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-14,1.5,14},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-14,1.5,14},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thicker_29",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-14.5,1.5,14.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-14.5,1.5,14.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thicker_30",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-15,1.5,15},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-15,1.5,15},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thicker_32",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-16,1.5,16},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-16,1.5,16},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thicker_34",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-17,1.5,17},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-17,1.5,17},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thicker_35",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-17.5,1.5,17.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-17.5,1.5,17.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thicker_42",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-21,1.5,21},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-21,1.5,21},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thicker_48",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-24,1.5,24},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-24,1.5,24},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_ext_thicker_49",{"",1,1,16777215,0,0,0,0},0,{1,-2.5,-7,2.5,7},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-2.5,-7,2.5,7},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_5x6",{"",1,1,16777215,0,0,0,0},0,{1,-2.5,-3,2.5,3},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-2.5,-3,2.5,3},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_5x10",{"",1,1,16777215,0,0,0,0},0,{1,-2.5,-5,2.5,5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-2.5,-5,2.5,5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_5x13",{"",1,1,16777215,0,0,0,0},0,{1,-2.5,-6.5,2.5,6.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-2.5,-6.5,2.5,6.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_5x22_5",{"",1,1,16777215,0,0,0,0},0,{1,-2.5,-11.25,2.5,11.25},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-2.5,-11.25,2.5,11.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_5x23",{"",1,1,16777215,0,0,0,0},0,{1,-2.5,-11.5,2.5,11.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-2.5,-11.5,2.5,11.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_5x26",{"",1,1,16777215,0,0,0,0},0,{1,-2.5,-13,2.5,13},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-2.5,-13,2.5,13},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_6x8",{"",1,1,16777215,0,0,0,0},0,{1,-3,-4,3,4},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-3,-4,3,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"glass_wall_9",{"map-wall-glass-9.img",0.5,1,16777215,10,0,0,0},1,{1,-0.5,-4.5,0.5,4.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-4.5,0.5,4.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"glass_wall_10",{"map-wall-glass-10.img",0.5,1,16777215,10,0,0,0},1,{1,-0.5,-5,0.5,5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-5,0.5,5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"glass_wall_12",{"map-wall-glass-12.img",0.5,1,16777215,10,0,0,0},1,{1,-0.5,-6,0.5,6},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-6,0.5,6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"glass_wall_12_2",{"map-wall-glass-12-2.img",0.5,1,16777215,10,0,0,0},1,{1,-1,-6,1,6},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1,-6,1,6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"glass_wall_13",{"map-wall-glass-13.img",0.5,1,16777215,10,0,0,0},1,{1,-0.5,-6.5,0.5,6.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-6.5,0.5,6.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"glass_wall_1x19",{"map-wall-glass-1x19.img",0.5,1,16777215,10,0,0,0},1,{1,-0.5,-9.5,0.5,9.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-9.5,0.5,9.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"glass_wall_1x23",{"map-wall-glass-1x23.img",0.5,1,16777215,10,0,0,0},1,{1,-0.5,-11.5,0.5,11.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-11.5,0.5,11.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"barn_wall_int_2",{"map-wall-02-rounded.img",0.5,1,7173701,10,0,0,0},1,{1,-0.5,-1,0.5,1},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-1,0.5,1},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"barn_wall_int_2_5",{"map-wall-02-5-rounded.img",0.5,1,7173701,10,0,0,0},1,{1,-0.5,-1.25,0.5,1.25},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-1.25,0.5,1.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"barn_wall_int_4",{"map-wall-04-rounded.img",0.5,1,7173701,10,0,0,0},1,{1,-0.5,-2,0.5,2},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-2,0.5,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"barn_wall_int_5",{"map-wall-05-rounded.img",0.5,1,7173701,10,0,0,0},1,{1,-0.5,-2.5,0.5,2.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-2.5,0.5,2.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"barn_wall_int_6",{"map-wall-06-rounded.img",0.5,1,7173701,10,0,0,0},1,{1,-0.5,-3,0.5,3},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-3,0.5,3},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"barn_wall_int_7",{"map-wall-07-rounded.img",0.5,1,7173701,10,0,0,0},1,{1,-0.5,-3.5,0.5,3.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-3.5,0.5,3.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"barn_wall_int_8",{"map-wall-08-rounded.img",0.5,1,7173701,10,0,0,0},1,{1,-0.5,-4,0.5,4},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-4,0.5,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"barn_wall_int_11",{"map-wall-11-rounded.img",0.5,1,7173701,10,0,0,0},1,{1,-0.5,-5.5,0.5,5.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-5.5,0.5,5.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"barn_wall_int_13",{"map-wall-13-rounded.img",0.5,1,7173701,10,0,0,0},1,{1,-0.5,-6.5,0.5,6.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-6.5,0.5,6.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"barn_column_1",{"map-column-01.img",0.5,1,2764060,10,0,0,0},1,{1,-1,-1,1,1},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1,-1,1,1},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bank_wall_int_3",{"map-wall-03-rounded.img",0.5,1,7951934,10,0,0,0},1,{1,-0.5,-1.5,0.5,1.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-1.5,0.5,1.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bank_wall_int_4",{"map-wall-04-rounded.img",0.5,1,7951934,10,0,0,0},1,{1,-0.5,-2,0.5,2},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-2,0.5,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bank_wall_int_5",{"map-wall-05-rounded.img",0.5,1,7951934,10,0,0,0},1,{1,-0.5,-2.5,0.5,2.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-2.5,0.5,2.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bank_wall_int_8",{"map-wall-08-rounded.img",0.5,1,7951934,10,0,0,0},1,{1,-0.5,-4,0.5,4},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-4,0.5,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_perm_wall_ext_1",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-0.5,0.5,0.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-0.5,0.5,0.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_perm_wall_ext_2",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-1,0.5,1},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-1,0.5,1},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_perm_wall_ext_4",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-2,0.5,2},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-2,0.5,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_perm_wall_ext_6",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-3,0.5,3},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-3,0.5,3},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_perm_wall_ext_7",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-3.5,0.5,3.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-3.5,0.5,3.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_perm_wall_ext_8",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-4,0.5,4},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-4,0.5,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_perm_wall_ext_10",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-5,0.5,5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-5,0.5,5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_perm_wall_ext_11",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-5.5,0.5,5.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-5.5,0.5,5.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_perm_wall_ext_12",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-6,0.5,6},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-6,0.5,6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_perm_wall_ext_14",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-7,0.5,7},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-7,0.5,7},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_perm_wall_ext_18",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-9,0.5,9},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-9,0.5,9},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_perm_wall_ext_20",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-10,0.5,10},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-10,0.5,10},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_perm_wall_ext_22",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-11,0.5,11},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-11,0.5,11},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_perm_wall_ext_23",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-11.5,0.5,11.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-11.5,0.5,11.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_perm_wall_ext_25",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-12.5,0.5,12.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-12.5,0.5,12.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_perm_wall_ext_38",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-19,0.5,19},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-19,0.5,19},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_perm_wall_ext_2x11",{"",1,1,16777215,0,0,0,0},0,{1,-1,-5.5,1,5.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1,-5.5,1,5.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_perm_wall_ext_2x10",{"",1,1,16777215,0,0,0,0},0,{1,-1,-5,1,5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1,-5,1,5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_perm_wall_ext_3x4",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-2,1.5,2},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-2,1.5,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_perm_wall_ext_3x13",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-6.5,1.5,6.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-6.5,1.5,6.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_wall_int_3",{"map-wall-03-rounded.img",0.5,1,5186573,10,0,0,0},1,{1,-0.5,-1.5,0.5,1.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-1.5,0.5,1.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_wall_int_4",{"map-wall-04-rounded.img",0.5,1,5186573,10,0,0,0},1,{1,-0.5,-2,0.5,2},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-2,0.5,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_wall_int_5",{"map-wall-05-rounded.img",0.5,1,5186573,10,0,0,0},1,{1,-0.5,-2.5,0.5,2.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-2.5,0.5,2.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_wall_int_6",{"map-wall-06-rounded.img",0.5,1,5186573,10,0,0,0},1,{1,-0.5,-3,0.5,3},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-3,0.5,3},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_wall_int_8",{"map-wall-08-rounded.img",0.5,1,5186573,10,0,0,0},1,{1,-0.5,-4,0.5,4},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-4,0.5,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_wall_int_9",{"map-wall-09-rounded.img",0.5,1,5186573,10,0,0,0},1,{1,-0.5,-4.5,0.5,4.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-4.5,0.5,4.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_wall_int_10",{"map-wall-10-rounded.img",0.5,1,5186573,10,0,0,0},1,{1,-0.5,-5,0.5,5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-5,0.5,5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_wall_int_12",{"map-wall-12-rounded.img",0.5,1,5186573,10,0,0,0},1,{1,-0.5,-6,0.5,6},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-6,0.5,6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_wall_int_13",{"map-wall-13-rounded.img",0.5,1,5186573,10,0,0,0},1,{1,-0.5,-6.5,0.5,6.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-6.5,0.5,6.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_wall_int_16",{"map-wall-16-rounded.img",0.5,1,5186573,10,0,0,0},1,{1,-0.5,-8,0.5,8},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-8,0.5,8},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_bar_small",{"",0.5,1,16777215,10,0,0,0},0,{1,-1.5,-4,1.5,4},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-1.5,-4,1.5,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_bar_large",{"map-reserve-bar-large.img",0.5,1,16777215,10,0,0,0},1,{1,-1.5,-7.5,1.5,7.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-1.5,-7.5,1.5,7.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_bar_back",{"map-reserve-bar-01.img",0.5,1,16777215,10,0,0,0},1,{1,-0.75,-6.5,0.75,6.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-0.75,-6.5,0.75,6.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"police_wall_int_2",{"map-wall-02-rounded.img",0.5,1,1777447,10,0,0,0},1,{1,-0.5,-1,0.5,1},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-1,0.5,1},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"police_wall_int_3",{"map-wall-03-rounded.img",0.5,1,1777447,10,0,0,0},1,{1,-0.5,-1.5,0.5,1.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-1.5,0.5,1.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"police_wall_int_4",{"map-wall-04-rounded.img",0.5,1,1777447,10,0,0,0},1,{1,-0.5,-2,0.5,2},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-2,0.5,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"police_wall_int_6",{"map-wall-06-rounded.img",0.5,1,1777447,10,0,0,0},1,{1,-0.5,-3,0.5,3},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-3,0.5,3},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"police_wall_int_7",{"map-wall-07-rounded.img",0.5,1,1777447,10,0,0,0},1,{1,-0.5,-3.5,0.5,3.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-3.5,0.5,3.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"police_wall_int_8",{"map-wall-08-rounded.img",0.5,1,1777447,10,0,0,0},1,{1,-0.5,-4,0.5,4},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-4,0.5,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"police_wall_int_10",{"map-wall-10-rounded.img",0.5,1,1777447,10,0,0,0},1,{1,-0.5,-5,0.5,5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-5,0.5,5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"house_wall_int_4",{"map-wall-04-rounded.img",0.5,1,10584424,10,0,0,0},1,{1,-0.5,-2,0.5,2},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-2,0.5,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"house_wall_int_5",{"map-wall-05-rounded.img",0.5,1,10584424,10,0,0,0},1,{1,-0.5,-2.5,0.5,2.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-2.5,0.5,2.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"house_wall_int_8",{"map-wall-08-rounded.img",0.5,1,10584424,10,0,0,0},1,{1,-0.5,-4,0.5,4},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-4,0.5,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"house_wall_int_9",{"map-wall-09-rounded.img",0.5,1,10584424,10,0,0,0},1,{1,-0.5,-4.5,0.5,4.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-4.5,0.5,4.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"house_wall_int_11",{"map-wall-11-rounded.img",0.5,1,10584424,10,0,0,0},1,{1,-0.5,-5.5,0.5,5.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-5.5,0.5,5.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"house_wall_int_14",{"map-wall-14-rounded.img",0.5,1,10584424,10,0,0,0},1,{1,-0.5,-7,0.5,7},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-7,0.5,7},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"house_column_1",{"map-column-01.img",0.5,1,5587506,10,0,0,0},1,{1,-1,-1,1,1},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1,-1,1,1},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cabin_wall_int_5",{"map-wall-05-rounded.img",0.5,1,10584424,10,0,0,0},1,{1,-0.5,-2.5,0.5,2.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-2.5,0.5,2.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cabin_wall_int_10",{"map-wall-10-rounded.img",0.5,1,10584424,10,0,0,0},1,{1,-0.5,-5,0.5,5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-5,0.5,5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cabin_wall_int_13",{"map-wall-13-rounded.img",0.5,1,10584424,10,0,0,0},1,{1,-0.5,-6.5,0.5,6.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-6.5,0.5,6.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"mansion_wall_int_1",{"map-wall-01-rounded.img",0.5,1,16768917,10,0,0,0},1,{1,-0.5,-0.5,0.5,0.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-0.5,0.5,0.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"mansion_wall_int_5",{"map-wall-05-rounded.img",0.5,1,16768917,10,0,0,0},1,{1,-0.5,-2.5,0.5,2.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-2.5,0.5,2.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"mansion_wall_int_6",{"map-wall-06-rounded.img",0.5,1,16768917,10,0,0,0},1,{1,-0.5,-3,0.5,3},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-3,0.5,3},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"mansion_wall_int_7",{"map-wall-07-rounded.img",0.5,1,16768917,10,0,0,0},1,{1,-0.5,-3.5,0.5,3.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-3.5,0.5,3.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"mansion_wall_int_8",{"map-wall-08-rounded.img",0.5,1,16768917,10,0,0,0},1,{1,-0.5,-4,0.5,4},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-4,0.5,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"mansion_wall_int_9",{"map-wall-09-rounded.img",0.5,1,16768917,10,0,0,0},1,{1,-0.5,-4.5,0.5,4.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-4.5,0.5,4.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"mansion_wall_int_10",{"map-wall-10-rounded.img",0.5,1,16768917,10,0,0,0},1,{1,-0.5,-5,0.5,5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-5,0.5,5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"mansion_wall_int_11",{"map-wall-11-rounded.img",0.5,1,16768917,10,0,0,0},1,{1,-0.5,-5.5,0.5,5.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-5.5,0.5,5.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"mansion_wall_int_12",{"map-wall-12-rounded.img",0.5,1,16768917,10,0,0,0},1,{1,-0.5,-6,0.5,6},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-6,0.5,6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"mansion_wall_int_13",{"map-wall-13-rounded.img",0.5,1,16768917,10,0,0,0},1,{1,-0.5,-6.5,0.5,6.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-6.5,0.5,6.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"mansion_column_1",{"map-column-01.img",0.5,1,7432016,10,0,0,0},1,{1,-1,-1,1,1},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1,-1,1,1},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"saloon_column_1",{"map-column-01.img",0.5,1,1710618,10,0,0,0},1,{1,-1,-1,1,1},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1,-1,1,1},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"saloon_bar_small",{"",0.5,1,4456448,10,0,0,0},0,{1,-1.5,-5,1.5,5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-1.5,-5,1.5,5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"saloon_bar_large",{"",0.5,1,4456448,10,0,0,0},0,{1,-1.5,-7.5,1.5,7.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-1.5,-7.5,1.5,7.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"saloon_bar_back_large",{"map-saloon-bar-01.img",0.5,1,16777215,10,0,0,0},1,{1,-0.75,-5,0.75,5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-0.75,-5,0.75,5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"saloon_bar_back_small",{"map-saloon-bar-02.img",0.5,1,16777215,10,0,0,0},1,{1,-0.75,-1.5,0.75,1.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-0.75,-1.5,0.75,1.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stone_wall_int_4",{"map-wall-04-stone.img",0.5,1,16777215,10,0,0,0},1,{1,-0.6,-2,0.6,2},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.6,-2,0.6,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_column_4x8",{"",1,1,16777215,0,0,0,0},0,{1,-2,-4,2,4},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-2,-4,2,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"metal_wall_column_5x12",{"",1,1,16777215,0,0,0,0},0,{1,-2.5,-6,2.5,6},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-2.5,-6,2.5,6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"wood_perm_wall_ext_5",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-2.5,0.5,2.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-2.5,0.5,2.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"wood_perm_wall_ext_6",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-3,0.5,3},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-3,0.5,3},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"wood_perm_wall_ext_7",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-3.5,0.5,3.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-3.5,0.5,3.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"wood_perm_wall_ext_14",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-7,0.5,7},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-7,0.5,7},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"wood_perm_wall_ext_17",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-8.5,0.5,8.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-8.5,0.5,8.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"wood_perm_wall_ext_35",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-17.5,0.5,17.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-17.5,0.5,17.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"wood_perm_wall_ext_thicker_6",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-3,1.5,3},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-3,1.5,3},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"wood_perm_wall_ext_thicker_7",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-3.5,1.5,3.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-3.5,1.5,3.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"wood_perm_wall_ext_thicker_8",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-4,1.5,4},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-4,1.5,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"wood_perm_wall_ext_thicker_10",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-5,1.5,5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-5,1.5,5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"wood_perm_wall_ext_thicker_12",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-6,1.5,6},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-6,1.5,6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"wood_perm_wall_ext_thicker_13",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-6.5,1.5,6.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-6.5,1.5,6.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"wood_perm_wall_ext_thicker_18",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-9,1.5,9},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-9,1.5,9},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"wood_perm_wall_ext_thicker_21",{"",1,1,16777215,0,0,0,0},0,{1,-1.5,-10.5,1.5,10.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.5,-10.5,1.5,10.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"teahouse_wall_int_3",{"map-wall-03.img",0.5,0.95,5505024,10,0,0,0},1,{1,-0.5,-1.5,0.5,1.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-1.5,0.5,1.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"teahouse_wall_int_4",{"map-wall-04.img",0.5,0.95,5505024,10,0,0,0},1,{1,-0.5,-2,0.5,2},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-2,0.5,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"teahouse_wall_int_5",{"map-wall-05.img",0.5,0.95,5505024,10,0,0,0},1,{1,-0.5,-2.5,0.5,2.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-2.5,0.5,2.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"teahouse_wall_int_7",{"map-wall-07.img",0.5,0.95,5505024,10,0,0,0},1,{1,-0.5,-3.5,0.5,3.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-3.5,0.5,3.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"teahouse_wall_int_12",{"map-wall-12.img",0.5,0.95,5505024,10,0,0,0},1,{1,-0.5,-6,0.5,6},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-6,0.5,6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"teahouse_wall_int_13",{"map-wall-13.img",0.5,0.95,5505024,10,0,0,0},1,{1,-0.5,-6.5,0.5,6.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-6.5,0.5,6.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"teahouse_wall_int_14",{"map-wall-14.img",0.5,0.95,5505024,10,0,0,0},1,{1,-0.5,-7,0.5,7},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-7,0.5,7},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"teahouse_wall_int_18",{"map-wall-18.img",0.5,0.95,5505024,10,0,0,0},1,{1,-0.5,-9,0.5,9},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-9,0.5,9},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"teahouse_window_open_01",{"map-building-house-window-res-01.img",0.5,1,12216619,10,0,0,0},1,{1,-0.4,-2,0.4,2},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-0.4,-2,0.4,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"grassy_wall_3",{"map-wall-03-grassy.img",0.5,1,16777215,10,0,0,0},1,{1,-0.375,-1.5,0.375,1.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,7282176,1,{1,-0.375,-1.5,0.375,1.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"grassy_wall_8",{"map-wall-08-grassy.img",0.5,1,16777215,10,0,0,0},1,{1,-0.375,-4,0.375,4},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,7282176,1,{1,-0.375,-4,0.375,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"club_wall_int_6",{"map-wall-06-rounded.img",0.5,1,10584424,10,0,0,0},1,{1,-0.5,-3,0.5,3},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-3,0.5,3},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"club_wall_int_10",{"map-wall-10-rounded.img",0.5,1,7218988,10,0,0,0},1,{1,-0.5,-5,0.5,5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-5,0.5,5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"club_bar_small",{"",0.5,1,4456448,10,0,0,0},0,{1,-1.5,-4.5,1.5,4.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-1.5,-4.5,1.5,4.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"club_bar_large",{"",0.5,1,4456448,10,0,0,0},0,{1,-1.5,-7,1.5,7},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-1.5,-7,1.5,7},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"club_bar_back_large",{"map-club-bar-01.img",0.5,1,16777215,10,0,0,0},1,{1,-0.75,-7.5,0.75,7.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-0.75,-7.5,0.75,7.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bathhouse_column_1",{"map-bathhouse-column-01.img",0.5,1,13481337,10,0,0,0},1,{1,-2,-2,2,2},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-2,-2,2,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bathhouse_column_2",{"map-bathhouse-column-02.img",0.5,1,13481337,10,0,0,0},1,{1,-1,-1,1,1},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1,-1,1,1},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bridge_lg_under_column",{"",1,1,16777215,0,0,0,0},0,{1,-2.5,-10,2.5,10},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-2.5,-10,2.5,10},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_column_5x4",{"",1,1,16777215,0,0,0,0},0,{1,-2.5,-2,2.5,2},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-2.5,-2,2.5,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bridge_rail_3",{"",0.5,1,4456448,10,0,0,0},0,{1,-0.4,-2,0.4,2},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-0.4,-2,0.4,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"rail_4",{"",0.5,1,4456448,10,0,0,0},0,{1,-0.4,-2.5,0.4,2.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-0.4,-2.5,0.4,2.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bridge_rail_12",{"",0.5,1,4456448,10,0,0,0},0,{1,-0.4,-6.5,0.4,6.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-0.4,-6.5,0.4,6.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bridge_xlg_under_column",{"",1,1,16777215,0,0,0,0},0,{1,-2.5,-14,2.5,14},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-2.5,-14,2.5,14},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"concrete_wall_column_9x4",{"",1,1,16777215,0,0,0,0},0,{1,-4.5,-2,4.5,2},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-4.5,-2,4.5,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bridge_rail_20",{"",0.5,1,4456448,10,0,0,0},0,{1,-0.4,-10,0.4,10},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-0.4,-10,0.4,10},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bridge_rail_28",{"",0.5,1,4456448,10,0,0,0},0,{1,-0.4,-14,0.4,14},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-0.4,-14,0.4,14},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_3_0_low",{"",0.5,1,4456448,10,0,0,0},0,{1,-0.5,-1.5,0.5,1.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-0.5,-1.5,0.5,1.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brick_wall_ext_11_5",{"",1,1,16777215,0,0,0,0},0,{1,-0.5,-5.75,0.5,5.75},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.5,-5.75,0.5,5.75},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"container_wall_top",{"",1,1,16777215,0,0,0,0},0,{1,-2.75,-0.4,2.75,0.4},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-2.75,-0.4,2.75,0.4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"container_wall_side",{"",1,1,16777215,0,0,0,0},0,{1,-0.4,-5.5,0.4,5.5},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.4,-5.5,0.4,5.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"container_wall_side_open",{"",1,1,16777215,0,0,0,0},0,{1,-0.4,-6,0.4,6},1,0,0,0,1,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.4,-6,0.4,6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"case_01",{"map-case-deagle-01.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-1.6,2.25,1.6},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.25,-1.6,2.25,1.6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"case_02",{"map-case-deagle-02.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-1.6,2.25,1.6},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.25,-1.6,2.25,1.6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"case_03",{"map-case-hatchet-01.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-1.6,2.25,1.6},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.25,-1.6,2.25,1.6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"case_04",{"map-case-flare-01.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-1.6,2.25,1.6},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,7025920,0.85,{1,-2.25,-1.6,2.25,1.6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"case_05",{"map-case-meteor-01.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-1.6,2.25,1.6},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,7025920,0.85,{1,-2.25,-1.6,2.25,1.6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"case_06",{"map-case-chrys-01.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-1.6,2.25,1.6},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,7025920,0.85,{1,-2.25,-1.6,2.25,1.6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"case_07",{"map-case-ring-01.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-1.6,2.25,1.6},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,7025920,0.85,{1,-2.25,-1.6,2.25,1.6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"case_07de",{"map-case-ring-02.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-1.6,2.25,1.6},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,7025920,0.85,{1,-2.25,-1.6,2.25,1.6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"case_08",{"map-case-crow-01.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-1.6,2.25,1.6},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,7025920,0.85,{1,-2.25,-1.6,2.25,1.6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"case_08sv",{"map-case-crow-01.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-1.6,2.25,1.6},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,7025920,0.85,{1,-2.25,-1.6,2.25,1.6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"case_09",{"map-case-twins-01.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-1.6,2.25,1.6},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.25,-1.6,2.25,1.6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"case_10",{"map-case-cloud-01.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-1.6,2.25,1.6},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.25,-1.6,2.25,1.6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"chest_01",{"map-chest-01.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-1.6,2.25,1.6},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.25,-1.6,2.25,1.6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"chest_01cb",{"map-chest-01.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-1.6,2.25,1.6},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.25,-1.6,2.25,1.6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"chest_02",{"map-chest-02.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-1.6,2.25,1.6},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,7025920,0.85,{1,-2.25,-1.6,2.25,1.6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"chest_03",{"map-chest-03.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,0,2.25,1.6},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.25,0,2.25,1.6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"chest_03cb",{"map-chest-03cb.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,0,2.25,1.6},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.25,0,2.25,1.6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"chest_03d",{"map-chest-03d.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,0,2.25,1.6},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.25,0,2.25,1.6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"chest_03f",{"map-chest-03f.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,0,2.25,1.6},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.25,0,2.25,1.6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"chest_03sv",{"map-chest-03sv.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,0,2.25,1.6},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.25,0,2.25,1.6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"chest_03x",{"map-chest-03x.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,0,2.25,1.6},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.25,0,2.25,1.6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"chest_03tr",{"map-chest-03tr.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,0,2.25,1.6},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.25,0,2.25,1.6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"chest_04",{"map-case-basement-01.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-1.6,2.25,1.6},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,7025920,0.85,{1,-2.25,-1.6,2.25,1.6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"chest_04d",{"map-case-basement-01.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-1.6,2.25,1.6},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,7025920,0.85,{1,-2.25,-1.6,2.25,1.6},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_01",{"map-crate-01.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-2.25,2.25,2.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,0.875,{1,-2.25,-2.25,2.25,2.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_01x",{"map-crate-01x.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-2.25,2.25,2.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,0.875,{1,-2.25,-2.25,2.25,2.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_02",{"map-crate-02.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-2.25,2.25,2.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.25,-2.25,2.25,2.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_02sv",{"map-crate-02sv.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-2.25,2.25,2.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,16760832,0.875,{1,-2.25,-2.25,2.25,2.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_02sv_lake",{"map-crate-02sv.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-2.25,2.25,2.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,16760832,0.875,{1,-2.25,-2.25,2.25,2.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_02x",{"map-crate-02x.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-2.25,2.25,2.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.25,-2.25,2.25,2.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_02f",{"map-crate-02f.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-2.25,2.25,2.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,13369344,0.875,{1,-2.25,-2.25,2.25,2.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_02d",{"map-crate-02f.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-2.25,2.25,2.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,13369344,0.875,{1,-2.25,-2.25,2.25,2.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_03",{"map-crate-03.img",0.35,1,16777215,10,0,0,0},1,{1,-1.575,-1.575,1.575,1.575},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,5066014,0.875,{1,-1.575,-1.575,1.575,1.575},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_03x",{"map-crate-03x.img",0.35,1,16777215,10,0,0,0},1,{1,-1.575,-1.575,1.575,1.575},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,31863,0.875,{1,-1.575,-1.575,1.575,1.575},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_04",{"map-crate-04.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-2.25,2.25,2.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,5468244,0.875,{1,-2.25,-2.25,2.25,2.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_05",{"map-crate-05.img",0.5,1,16777215,10,0,0,0},1,{1,-2,-2,2,2},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2,-2,2,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_06",{"map-crate-06.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-1.1,2.25,1.1},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,0.875,{1,-2.25,-1.1,2.25,1.1},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_07",{"map-crate-07.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-2.25,2.25,2.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,0.875,{1,-2.25,-2.25,2.25,2.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_07b",{"map-crate-07.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-2.25,2.25,2.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,0.875,{1,-2.25,-2.25,2.25,2.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_07sv",{"map-crate-07.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-2.25,2.25,2.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,0.875,{1,-2.25,-2.25,2.25,2.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_08",{"map-crate-08.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-2.25,2.25,2.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.25,-2.25,2.25,2.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_09",{"map-crate-09.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-2.25,2.25,2.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.25,-2.25,2.25,2.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_09bh",{"map-crate-09.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-2.25,2.25,2.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.25,-2.25,2.25,2.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_09de",{"map-crate-09.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-2.25,2.25,2.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.25,-2.25,2.25,2.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_10",{"map-crate-10.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-2.25,2.25,2.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.25,-2.25,2.25,2.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_11",{"map-crate-11.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-2.25,2.25,2.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.25,-2.25,2.25,2.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_11h",{"map-crate-11h.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,2.25,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{0,0,0,2.25,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_10sv",{"map-crate-10.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-2.25,2.25,2.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.25,-2.25,2.25,2.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_11sv",{"map-crate-11.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-2.25,2.25,2.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.25,-2.25,2.25,2.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_11de",{"map-crate-11.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-2.25,2.25,2.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.25,-2.25,2.25,2.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_11tr",{"map-crate-11.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-2.25,2.25,2.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.25,-2.25,2.25,2.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_12",{"map-crate-12.img",0.5,1,16777215,10,0,0,0},1,{1,-3.5,-3.5,3.5,3.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-3.5,-3.5,3.5,3.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_12po",{"map-crate-12.img",0.5,1,16777215,10,0,0,0},1,{1,-3.5,-3.5,3.5,3.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-3.5,-3.5,3.5,3.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_12dev",{"map-crate-13.img",0.5,1,16777215,10,0,0,0},1,{1,-3.5,-3.5,3.5,3.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-3.5,-3.5,3.5,3.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_13",{"map-crate-13.img",0.5,1,16777215,10,0,0,0},1,{1,-3.5,-3.5,3.5,3.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-3.5,-3.5,3.5,3.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_13po",{"map-crate-13.img",0.5,1,16777215,10,0,0,0},1,{1,-3.5,-3.5,3.5,3.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-3.5,-3.5,3.5,3.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_14",{"map-crate-14.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-2.25,2.25,2.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,0.875,{1,-2.25,-2.25,2.25,2.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_14a",{"map-crate-14a.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-2.25,2.25,2.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,0.875,{1,-2.25,-2.25,2.25,2.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_15",{"map-crate-14.img",0.5,1,16777215,10,0,0,0},1,{1,-2.7,-1.25,2.7,1.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.7,-1.25,2.7,1.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_16",{"map-crate-14.img",0.5,1,16777215,10,0,0,0},1,{1,-2.7,-1.25,2.7,1.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.7,-1.25,2.7,1.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_17",{"map-crate-17.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-2.25,2.25,2.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.25,-2.25,2.25,2.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_18",{"map-crate-18.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-2.25,2.25,2.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,12867840,0.875,{1,-2.25,-2.25,2.25,2.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_19",{"map-crate-19.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-2.25,2.25,2.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,4500224,0.875,{1,-2.25,-2.25,2.25,2.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_20",{"map-crate-20.img",0.5,1,16777215,10,0,0,0},1,{1,-1.7,-1.7,1.7,1.7},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,3884335,1,{1,-1.7,-1.7,1.7,1.7},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_21",{"map-crate-21.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-2.25,2.25,2.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,18799,0.875,{1,-2.25,-2.25,2.25,2.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_21b",{"map-crate-21.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-2.25,2.25,2.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,18799,0.875,{1,-2.25,-2.25,2.25,2.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_22",{"map-crate-22.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-2.25,2.25,2.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,32511,0.875,{1,-2.25,-2.25,2.25,2.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"crate_22d",{"map-crate-22.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-2.25,2.25,2.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,32511,0.875,{1,-2.25,-2.25,2.25,2.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"airdrop_crate_01",{"map-airdrop-01.img",0.5,1,16777215,10,0,0,0},1,{1,-2.5,-2.5,2.5,2.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.5,-2.5,2.5,2.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"airdrop_crate_02",{"map-airdrop-01.img",0.5,1,16777215,10,0,0,0},1,{1,-2.5,-2.5,2.5,2.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.5,-2.5,2.5,2.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"airdrop_crate_03",{"map-airdrop-03.img",0.5,1,16777215,10,0,0,0},1,{1,-4,-4,4,4},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-4,-4,4,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"airdrop_crate_03po",{"map-airdrop-03.img",0.5,1,16777215,10,0,0,0},1,{1,-4,-4,4,4},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-4,-4,4,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"airdrop_crate_03dev",{"map-airdrop-03.img",0.5,1,16777215,10,0,0,0},1,{1,-4,-4,4,4},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-4,-4,4,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"airdrop_crate_04",{"map-airdrop-03.img",0.5,1,16777215,10,0,0,0},1,{1,-4,-4,4,4},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-4,-4,4,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"airdrop_crate_04po",{"map-airdrop-03.img",0.5,1,16777215,10,0,0,0},1,{1,-4,-4,4,4},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-4,-4,4,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"airdrop_crate_05",{"map-airdrop-01.img",0.5,1,16777215,10,0,0,0},1,{1,-2.5,-2.5,2.5,2.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.5,-2.5,2.5,2.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"airdrop_crate_01sv",{"map-airdrop-01.img",0.5,1,16777215,10,0,0,0},1,{1,-2.5,-2.5,2.5,2.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.5,-2.5,2.5,2.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"airdrop_crate_02sv",{"map-airdrop-01.img",0.5,1,16777215,10,0,0,0},1,{1,-2.5,-2.5,2.5,2.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.5,-2.5,2.5,2.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"airdrop_crate_02de",{"map-airdrop-01.img",0.5,1,16777215,10,0,0,0},1,{1,-2.5,-2.5,2.5,2.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.5,-2.5,2.5,2.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"airdrop_crate_02h",{"map-airdrop-01h.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,2.5,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{0,0,0,2.5,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"airdrop_crate_02tr",{"map-airdrop-01.img",0.5,1,16777215,10,0,0,0},1,{1,-2.5,-2.5,2.5,2.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.5,-2.5,2.5,2.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"airdrop_crate_01x",{"map-airdrop-01x.img",0.5,1,16777215,10,0,0,0},1,{1,-2.5,-2.5,2.5,2.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.5,-2.5,2.5,2.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"airdrop_crate_02x",{"map-airdrop-01x.img",0.5,1,16777215,10,0,0,0},1,{1,-2.5,-2.5,2.5,2.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.5,-2.5,2.5,2.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"class_shell_01",{"map-class-shell-01a.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,2.25,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{0,0,0,2.25,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"class_shell_02",{"map-class-shell-02a.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,2.25,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{0,0,0,2.25,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"class_shell_03",{"map-class-shell-03a.img",0.5,1,16777215,20,0,0,0},1,{0,0,0,2.25,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{0,0,0,2.25,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"class_crate_common_scout",{"map-class-crate-scout.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,2.1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{0,0,0,2.1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"class_crate_common_sniper",{"map-class-crate-sniper.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,2.1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{0,0,0,2.1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"class_crate_common_healer",{"map-class-crate-healer.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,2.1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{0,0,0,2.1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"class_crate_common_demo",{"map-class-crate-demo.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,2.1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{0,0,0,2.1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"class_crate_common_assault",{"map-class-crate-assault.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,2.1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{0,0,0,2.1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"class_crate_common_tank",{"map-class-crate-tank.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,2.1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{0,0,0,2.1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"class_crate_common_classless",{"map-class-crate-classless.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,2.1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{0,0,0,2.1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"class_crate_rare_scout",{"map-class-crate-scout.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,2.1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{0,0,0,2.1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"class_crate_rare_sniper",{"map-class-crate-sniper.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,2.1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{0,0,0,2.1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"class_crate_rare_healer",{"map-class-crate-healer.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,2.1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{0,0,0,2.1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"class_crate_rare_demo",{"map-class-crate-demo.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,2.1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{0,0,0,2.1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"class_crate_rare_assault",{"map-class-crate-assault.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,2.1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{0,0,0,2.1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"class_crate_rare_tank",{"map-class-crate-tank.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,2.1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{0,0,0,2.1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"class_crate_rare_classless",{"map-class-crate-classless.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,2.1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{0,0,0,2.1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"class_crate_mythic",{"map-class-crate-03.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,2.1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{0,0,0,2.1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"mil_crate_01",{"map-crate-mil-01.img",0.5,1,16777215,10,0,0,0},1,{1,-2.7,-1.25,2.7,1.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.7,-1.25,2.7,1.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"mil_crate_02",{"map-crate-mil-02.img",0.5,1,16777215,10,0,0,0},1,{1,-2.7,-1.25,2.7,1.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.7,-1.25,2.7,1.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"mil_crate_03",{"map-crate-mil-03.img",0.5,1,16777215,10,0,0,0},1,{1,-2.7,-1.25,2.7,1.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.7,-1.25,2.7,1.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"mil_crate_04",{"map-crate-mil-04.img",0.5,1,16777215,10,0,0,0},1,{1,-2.7,-1.25,2.7,1.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.7,-1.25,2.7,1.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"mil_crate_05",{"map-crate-mil-05.img",0.5,1,16777215,10,0,0,0},1,{1,-2.7,-1.25,2.7,1.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,3622438,0.875,{1,-2.7,-1.25,2.7,1.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"barrel_02",{"map-barrel-02.img",0.4,1,16777215,10,0,0,0},1,{0,0,0,1.75,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,11235106,1,{0,0,0,1.75,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"barrel_03",{"map-barrel-03.img",0.45,1,16777215,10,0,0,0},1,{1,-1.25,-0.5,1.25,0.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,11235106,1,{1,-1.25,-0.5,1.25,0.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"barrel_04",{"map-barrel-04.img",0.45,1,16777215,10,0,0,0},1,{1,-1.25,-0.5,1.25,0.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,11235106,1,{1,-1.25,-0.5,1.25,0.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"barrel_05",{"map-barrel-05.img",0.4,1,16777215,10,0,0,0},1,{0,0,0,1.75,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,11235106,1,{0,0,0,1.75,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bathhouse_rocks_01",{"map-bathrocks-01.img",0.5,1,16777215,10,0,0,0},1,{1,-1.55,-1.55,1.55,1.55},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.55,-1.55,1.55,1.55},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bed_sm_01",{"map-bed-01.img",0.5,1,16777215,10,0,0,0},1,{1,-1.4,-3.4,1.4,3.4},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,0.875,{1,-1.4,-3.4,1.4,3.4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bed_lg_01",{"map-bed-02.img",0.5,1,16777215,10,0,0,0},1,{1,-2.8,-3.4,2.8,3.4},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,0.875,{1,-2.8,-3.4,2.8,3.4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bookshelf_01",{"map-bookshelf-01.img",0.5,1,16777215,10,0,0,0},1,{1,-3.5,-1,3.5,1},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-3.5,-1,3.5,1},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bookshelf_02",{"map-bookshelf-02.img",0.5,1,16777215,10,0,0,0},1,{1,-3.5,-1,3.5,1},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-3.5,-1,3.5,1},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"chair_01",{"map-chair-01.img",0.5,1,16777215,5,0,0,0},1,{1,-1,-1.25,1,1.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-1,-1.25,1,1.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"chair_02",{"map-chair-02.img",0.5,1,16777215,5,0,0,0},1,{0,0,0,1.25,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{0,0,0,1.25,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"couch_01",{"map-couch-01.img",0.5,1,16777215,10,0,0,0},1,{1,-4.5,-1.5,4.5,1.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-4.5,-1.5,4.5,1.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"couch_02",{"map-couch-02.img",0.5,1,16777215,10,0,0,0},1,{1,-3,-1.5,3,1.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-3,-1.5,3,1.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"couch_02b",{"map-couch-02.img",0.5,1,16777215,10,0,1,0},1,{1,-3,-1.5,3,1.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-3,-1.5,3,1.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"couch_03",{"map-couch-03.img",0.5,1,16777215,10,0,0,0},1,{1,-1.5,-1.5,1.5,1.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-1.5,-1.5,1.5,1.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bottle_01",{"map-bottle-01.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,0.5,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,1,{0,0,0,0.5,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bottle_02",{"map-bottle-02.img",0.5,1,16777215,10,0,0,0},1,{1,-0.5,-0.5,0.5,0.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,1,{1,-0.5,-0.5,0.5,0.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bottle_04",{"map-bottle-04.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,0.5,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,1,{0,0,0,0.5,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bottle_05",{"map-bottle-05.img",0.5,1,16777215,10,0,0,0},1,{1,-0.5,-0.5,0.5,0.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,1,{1,-0.5,-0.5,0.5,0.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"candle_01",{"map-candle-01.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,0.5,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,16777215,1,{0,0,0,0.5,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"deposit_box_01",{"map-deposit-box-01.img",0.5,1,16777215,10,0,0,0},1,{1,-2.5,-0.85,2.5,1.15},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.5,-0.85,2.5,1.15},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"deposit_box_02",{"map-deposit-box-02.img",0.5,1,16777215,10,0,0,0},1,{1,-2.5,-0.85,2.5,1.15},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.5,-0.85,2.5,1.15},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"deposit_box_03",{"map-deposit-box-03.img",0.5,1,16777215,10,0,0,0},1,{1,-2.5,-0.85,2.5,1.15},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.5,-0.85,2.5,1.15},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"drawers_01",{"map-drawers-01.img",0.5,1,16777215,10,0,0,0},1,{1,-2.5,-1.1,2.5,1.4},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.5,-1.1,2.5,1.4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"drawers_02",{"map-drawers-02.img",0.5,1,16777215,10,0,0,0},1,{1,-2.5,-1.1,2.5,1.4},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.5,-1.1,2.5,1.4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"fire_ext_01",{"map-fire-ext-01.img",0.5,1,16777215,10,0,0,0},1,{0,0.35,0,1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{0,0.35,0,1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"grill_01",{"map-grill-01.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,1.55,0},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,0,1,14935011,0.875,{0,0,0,1.55,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"gun_mount_empty",{"map-gun-mount-empty.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-0.49999999999999994,2.25,0.8999999999999999},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.25,-0.49999999999999994,2.25,0.8999999999999999},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"gun_mount_01",{"map-gun-mount-01.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-0.49999999999999994,2.25,0.8999999999999999},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.25,-0.49999999999999994,2.25,0.8999999999999999},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"gun_mount_02",{"map-gun-mount-02.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-0.49999999999999994,2.25,0.8999999999999999},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.25,-0.49999999999999994,2.25,0.8999999999999999},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"gun_mount_03",{"map-gun-mount-03.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-0.49999999999999994,2.25,0.8999999999999999},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.25,-0.49999999999999994,2.25,0.8999999999999999},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"gun_mount_04",{"map-gun-mount-04.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-0.49999999999999994,2.25,0.8999999999999999},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.25,-0.49999999999999994,2.25,0.8999999999999999},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"gun_mount_05",{"map-gun-mount-05.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-0.49999999999999994,2.25,0.8999999999999999},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.25,-0.49999999999999994,2.25,0.8999999999999999},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"gun_mount_06",{"map-gun-mount-06.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-0.49999999999999994,2.25,0.8999999999999999},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.25,-0.49999999999999994,2.25,0.8999999999999999},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"gun_mount_07",{"map-gun-mount-07.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-0.49999999999999994,2.25,0.8999999999999999},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.25,-0.49999999999999994,2.25,0.8999999999999999},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"locker_01",{"map-locker-01.img",0.5,1,16777215,10,0,0,0},1,{1,-1.5,-0.44999999999999996,1.5,0.75},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-1.5,-0.44999999999999996,1.5,0.75},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"locker_02",{"map-locker-02.img",0.5,1,16777215,10,0,0,0},1,{1,-1.5,-0.44999999999999996,1.5,0.75},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-1.5,-0.44999999999999996,1.5,0.75},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"locker_03",{"map-locker-03.img",0.5,1,16777215,10,0,0,0},1,{1,-1.5,-0.44999999999999996,1.5,0.75},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-1.5,-0.44999999999999996,1.5,0.75},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"oven_01",{"map-oven-01.img",0.5,1,16777215,10,0,0,0},1,{1,-1.7,-1.1500000000000001,1.7,1.45},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,0,1,14935011,0.875,{1,-1.7,-1.1500000000000001,1.7,1.45},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"piano_01",{"map-piano-01.img",0.5,1,16777215,10,0,0,0},1,{1,-3.75,-1,3.75,1},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-3.75,-1,3.75,1},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"planter_01",{"map-planter-01.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-4.25,2.25,4.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,0.875,{1,-2.25,-4.25,2.25,4.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"planter_02",{"map-planter-02.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-4.25,2.25,4.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,0.875,{1,-2.25,-4.25,2.25,4.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"planter_03",{"map-planter-03.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-4.25,2.25,4.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,0.875,{1,-2.25,-4.25,2.25,4.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"planter_04",{"map-planter-04.img",0.5,1,16777215,10,0,0,0},1,{1,-1.5,-1.5,1.5,1.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,0.875,{1,-1.5,-1.5,1.5,1.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"planter_06",{"map-planter-06.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-4.25,2.25,4.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,0.875,{1,-2.25,-4.25,2.25,4.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"planter_07",{"map-planter-07.img",0.5,1,16777215,10,0,0,0},1,{1,-1.5,-1.5,1.5,1.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,0.875,{1,-1.5,-1.5,1.5,1.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"pot_01",{"map-pot-01.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,1.5,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,1,{0,0,0,1.5,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"pot_02",{"map-pot-02.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,1.5,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,1,{0,0,0,1.5,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"pot_03",{"map-pot-03.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,1.5,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,1,{0,0,0,1.5,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"pot_03b",{"map-pot-03.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,1.5,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,1,{0,0,0,1.5,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"pot_03c",{"map-pot-03.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,1.5,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,1,{0,0,0,1.5,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"pot_04",{"map-pot-04.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,1.5,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,1,{0,0,0,1.5,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"pot_05",{"map-pot-05.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,1.5,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,1,{0,0,0,1.5,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"rack_01",{"map-rack-01.img",0.5,1,16777215,10,0,0,0},1,{1,-2,-1.05,2,1.45},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2,-1.05,2,1.45},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"refrigerator_01",{"map-refrigerator-01.img",0.5,1,16777215,10,0,0,0},1,{1,-1.7,-1.1,1.7,1.4},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,7733259,0.875,{1,-1.7,-1.1,1.7,1.4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"refrigerator_01b",{"map-refrigerator-01.img",0.5,1,16777215,10,0,0,0},1,{1,-1.7,-1.1,1.7,1.4},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,7733259,0.875,{1,-1.7,-1.1,1.7,1.4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"safe_01",{"map-safe-01.img",0.4,1,16777215,10,0,0,0},1,{1,-1.25,-1.15,1.25,1.35},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,1512466,1,{1,-1.25,-1.15,1.25,1.35},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"safe_01de",{"map-safe-01.img",0.4,1,16777215,10,0,0,0},1,{1,-1.25,-1.15,1.25,1.35},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,1512466,1,{1,-1.25,-1.15,1.25,1.35},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"screen_01",{"map-screen-01.img",0.5,1,16777215,10,0,0,0},1,{1,-4,-0.15000000000000002,4,0.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-4,-0.15000000000000002,4,0.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"sink_01",{"map-sink-01.img",0.35,1,16777215,10,0,0,0},1,{1,-2,-1.5,2,1.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,11776947,1,{1,-2,-1.5,2,1.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stand_01",{"map-stand-01.img",0.5,1,16777215,10,0,0,0},1,{1,-1.25,-1.1,1.25,1.4},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-1.25,-1.1,1.25,1.4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"power_box_01",{"map-power-box-01.img",0.5,1,16777215,10,0,0,0},1,{1,-1,-1,1,1},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1,-1,1,1},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stove_01",{"map-stove-01.img",0.5,1,16777215,10,0,0,0},1,{1,-3,-2.25,3,2.25},1,0,0,0,0,0,1,"explosion_stove",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-3,-2.25,3,2.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stove_02",{"map-stove-02.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,1.5,0},1,0,0,0,0,0,1,"explosion_stove",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{0,0,0,1.5,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"table_01",{"map-table-01.img",0.5,1,16777215,60,0,0,0},1,{1,-2.5,-2,2.5,2},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.5,-2,2.5,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"table_01x",{"map-table-01x.img",0.5,1,16777215,60,0,0,0},1,{1,-2.5,-2,2.5,2},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.5,-2,2.5,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"table_01d",{"map-table-01d.img",0.5,1,16777215,60,0,0,0},1,{1,-2.5,-2.5,2.5,1.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-2.5,-2.5,2.5,1.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"table_02",{"map-table-02.img",0.5,1,16777215,60,0,0,0},1,{1,-4.5,-2.5,4.5,2.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-4.5,-2.5,4.5,2.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"table_02x",{"map-table-02x.img",0.5,1,16777215,60,0,0,0},1,{1,-4.5,-2.5,4.5,2.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-4.5,-2.5,4.5,2.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"table_03",{"map-table-03.img",0.5,1,16777215,60,0,0,0},1,{0,0,0,2.5,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{0,0,0,2.5,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"table_03x",{"map-table-03x.img",0.5,1,16777215,60,0,0,0},1,{0,0,0,2.5,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{0,0,0,2.5,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"table_04",{"map-table-04.img",0.5,1,16777215,60,0,0,0},1,{1,-4.5,-2,4.5,2},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-4.5,-2,4.5,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"table_05",{"map-table-05.img",0.5,1,16777215,60,0,0,0},1,{1,-9,-2.75,9,2.75},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-9,-2.75,9,2.75},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"table_06",{"map-table-06.img",0.5,1,16777215,10,0,0,0},1,{1,-4,-2,4,2},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-4,-2,4,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"table_07",{"map-table-07.img",0.5,1,16777215,10,0,0,0},1,{1,-3.3000000000000003,-1.35,3.4,1.15},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-3.3000000000000003,-1.35,3.4,1.15},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"table_08",{"map-table-08.img",0.5,1,16777215,10,0,0,0},1,{1,-4,-1.55,4,1.45},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-4,-1.55,4,1.45},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"table_09",{"map-table-09.img",0.5,1,16777215,10,0,0,0},1,{1,-3,-2.05,3,1.95},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-3,-2.05,3,1.95},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"toilet_01",{"map-toilet-01.img",0.5,1,16777215,10,0,0,0},1,{0,0,0.25,1.18,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,11776947,1,{0,0,0.25,1.18,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"toilet_02",{"map-toilet-02.img",0.5,1,16777215,10,0,0,0},1,{0,0,0.25,1.18,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,11776947,1,{0,0,0.25,1.18,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"toilet_02b",{"map-toilet-02.img",0.5,1,11842740,10,0,0,0},1,{0,0,0.25,1.18,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,11776947,1,{0,0,0.25,1.18,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"toilet_03",{"map-toilet-03.img",0.5,1,16777215,10,0,0,0},1,{0,0,0.25,1.18,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,11776947,1,{0,0,0.25,1.18,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"toilet_04",{"map-toilet-04.img",0.5,1,16777215,10,0,0,0},1,{0,0,0.25,1.18,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,11776947,1,{0,0,0.25,1.18,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"toilet_05",{"map-toilet-05.img",0.66,1,16777215,10,0,0,0},1,{0,0,0.25,1.56,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,11776947,1,{0,0,0.25,1.56,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"towelrack_01",{"map-towelrack-01.img",0.5,1,16777215,10,0,0,0},1,{1,-3,-1,3,1},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{1,-3,-1,3,1},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"vat_01",{"map-vat-01.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,2,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,11776947,1,{0,0,0,2,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"vat_02",{"map-vat-02.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,3.1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,11776947,1,{0,0,0,3.1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"vat_03",{"map-vat-03.img",0.5,1,16777215,50,0,0,0},1,{0,0,0,1.75,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,11776947,1,{0,0,0,1.75,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"vat_04",{"map-vat-04.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,2,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,11776947,1,{0,0,0,2,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"vat_05",{"map-vat-05.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,2,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,11776947,1,{0,0,0,2,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"vending_01",{"map-vending-soda-01.img",0.5,1,16777215,10,0,0,0},1,{1,-1.7,-1.1,1.7,1.4},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,10925,0.875,{1,-1.7,-1.1,1.7,1.4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"wheel_01",{"map-wheel-01.img",0.5,1,16777215,10,0,0,0},1,{0,0,2.3,4.6,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6310464,1,{0,0,2.3,4.6,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"wheel_02",{"map-wheel-02.img",0.5,1,16777215,10,0,0,0},1,{0,0,2.3,4.6,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6310464,1,{0,0,2.3,4.6,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"wheel_03",{"map-wheel-03.img",0.5,1,16777215,10,0,0,0},1,{0,0,2.3,4.6,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6310464,1,{0,0,2.3,4.6,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"control_panel_01",{"map-control-panel-01.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-1.7,2.25,1.7},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-2.25,-1.7,2.25,1.7},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"control_panel_02",{"map-control-panel-02.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-1.7,2.25,1.7},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-2.25,-1.7,2.25,1.7},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"control_panel_02b",{"map-control-panel-02.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-1.7,2.25,1.7},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-2.25,-1.7,2.25,1.7},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"control_panel_03",{"map-control-panel-03.img",0.5,1,16777215,10,0,0,0},1,{1,-1.25,-1.2,1.25,1.2},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-1.25,-1.2,1.25,1.2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"control_panel_04",{"map-control-panel-04.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-1.7,2.25,1.7},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-2.25,-1.7,2.25,1.7},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"control_panel_06",{"map-control-panel-06.img",0.5,1,16777215,10,0,0,0},1,{1,-3,-1.4,3,1.4},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-3,-1.4,3,1.4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"control_panel_07de",{"map-control-panel-01.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-1.7,2.25,1.7},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-2.25,-1.7,2.25,1.7},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"control_panel_07sv",{"map-control-panel-01.img",0.5,1,16777215,10,0,0,0},1,{1,-2.25,-1.7,2.25,1.7},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-2.25,-1.7,2.25,1.7},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"switch_01",{"map-switch-01.img",0.5,1,16777215,10,0,0,0},1,{1,-0.45,-0.55,0.45,0.55},1,0,0,0,0,0,1,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.45,-0.55,0.45,0.55},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"switch_01o",{"map-switch-01o.img",0.5,1,16777215,10,0,0,0},1,{1,-0.45,-0.55,0.45,0.55},1,0,0,0,0,0,1,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.45,-0.55,0.45,0.55},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"switch_01p",{"map-switch-01p.img",0.5,1,16777215,10,0,0,0},1,{1,-0.45,-0.55,0.45,0.55},1,0,0,0,0,0,1,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.45,-0.55,0.45,0.55},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"switch_01y",{"map-switch-01y.img",0.5,1,16777215,10,0,0,0},1,{1,-0.45,-0.55,0.45,0.55},1,0,0,0,0,0,1,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.45,-0.55,0.45,0.55},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"switch_02",{"map-switch-02.img",0.5,1,16777215,10,0,0,0},1,{1,-0.45,-0.55,0.45,0.55},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.45,-0.55,0.45,0.55},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"switch_03",{"map-switch-01.img",0.5,1,16777215,10,0,0,0},1,{1,-0.45,-0.55,0.45,0.55},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.45,-0.55,0.45,0.55},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bottle_02r",{"map-bottle-02.img",0.5,1,13172736,10,0,0,0},1,{1,-0.5,-0.5,0.5,0.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,1,{1,-0.5,-0.5,0.5,0.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bottle_02o",{"map-bottle-02.img",0.5,1,16734720,10,0,0,0},1,{1,-0.5,-0.5,0.5,0.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,1,{1,-0.5,-0.5,0.5,0.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bottle_02y",{"map-bottle-02.img",0.5,1,16776960,10,0,0,0},1,{1,-0.5,-0.5,0.5,0.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,1,{1,-0.5,-0.5,0.5,0.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bottle_02g",{"map-bottle-02.img",0.5,1,32768,10,0,0,0},1,{1,-0.5,-0.5,0.5,0.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,1,{1,-0.5,-0.5,0.5,0.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bottle_02b",{"map-bottle-02.img",0.5,1,27903,10,0,0,0},1,{1,-0.5,-0.5,0.5,0.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,1,{1,-0.5,-0.5,0.5,0.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bottle_02i",{"map-bottle-02.img",0.5,1,4915330,10,0,0,0},1,{1,-0.5,-0.5,0.5,0.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,1,{1,-0.5,-0.5,0.5,0.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bottle_02v",{"map-bottle-02.img",0.5,1,15631086,10,0,0,0},1,{1,-0.5,-0.5,0.5,0.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,1,{1,-0.5,-0.5,0.5,0.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"button_01",{"map-button-01.img",0.5,1,16777215,10,0,0,0},1,{1,-0.5,-0.5,0.5,0.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,16777215,1,{1,-0.5,-0.5,0.5,0.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"button_01g",{"map-button-01g.img",0.5,1,16777215,10,0,0,0},1,{1,-0.5,-0.5,0.5,0.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,16777215,1,{1,-0.5,-0.5,0.5,0.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"button_01b",{"map-button-01b.img",0.5,1,16777215,10,0,0,0},1,{1,-0.5,-0.5,0.5,0.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,16777215,1,{1,-0.5,-0.5,0.5,0.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"recorder_01",{"map-recorder-01.img",0.5,1,16777215,9,0,0,0},1,{1,-0.9,-1.5,0.9,1.5},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.9,-1.5,0.9,1.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"recorder_02",{"map-recorder-01.img",0.5,1,16777215,9,0,0,0},1,{1,-0.9,-1.5,0.9,1.5},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.9,-1.5,0.9,1.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"recorder_03",{"map-recorder-01.img",0.5,1,16777215,9,0,0,0},1,{1,-0.9,-1.5,0.9,1.5},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.9,-1.5,0.9,1.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"recorder_04",{"map-recorder-01.img",0.5,1,16777215,9,0,0,0},1,{1,-0.9,-1.5,0.9,1.5},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.9,-1.5,0.9,1.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"recorder_05",{"map-recorder-01.img",0.5,1,16777215,9,0,0,0},1,{1,-0.9,-1.5,0.9,1.5},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.9,-1.5,0.9,1.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"recorder_06",{"map-recorder-01.img",0.5,1,16777215,9,0,0,0},1,{1,-0.9,-1.5,0.9,1.5},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.9,-1.5,0.9,1.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"recorder_07",{"map-recorder-01.img",0.5,1,16777215,9,0,0,0},1,{1,-0.9,-1.5,0.9,1.5},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.9,-1.5,0.9,1.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"recorder_08",{"map-recorder-01.img",0.5,1,16777215,9,0,0,0},1,{1,-0.9,-1.5,0.9,1.5},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.9,-1.5,0.9,1.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"recorder_09",{"map-recorder-01.img",0.5,1,16777215,9,0,0,0},1,{1,-0.9,-1.5,0.9,1.5},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.9,-1.5,0.9,1.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"recorder_10",{"map-recorder-01.img",0.5,1,16777215,9,0,0,0},1,{1,-0.9,-1.5,0.9,1.5},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.9,-1.5,0.9,1.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"recorder_11",{"map-recorder-03.img",0.5,1,16777215,9,0,0,0},1,{1,-0.75,-1.25,0.75,1.25},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.75,-1.25,0.75,1.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"recorder_12",{"map-recorder-03.img",0.5,1,16777215,9,0,0,0},1,{1,-0.75,-1.25,0.75,1.25},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.75,-1.25,0.75,1.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"recorder_13",{"map-recorder-03.img",0.5,1,16777215,9,0,0,0},1,{1,-0.75,-1.25,0.75,1.25},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.75,-1.25,0.75,1.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"recorder_14",{"map-recorder-03.img",0.5,1,16777215,9,0,0,0},1,{1,-0.75,-1.25,0.75,1.25},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,-0.75,-1.25,0.75,1.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_switch_01",{"map-tree-switch-01.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,1.6,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,8602624,1,{0,0,0,1.6,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_switch_02",{"map-tree-switch-02.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,1.6,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,8602624,1,{0,0,0,1.6,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_switch_03",{"map-tree-switch-03.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,1.6,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,8602624,1,{0,0,0,1.6,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"barrel_01",{"map-barrel-01.img",0.4,1,16777215,10,0,0,0},1,{0,0,0,1.75,0},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6447714,1,{0,0,0,1.75,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"barrel_01b",{"map-barrel-01.img",0.4,1,13224393,10,0,0,0},1,{0,0,0,1.75,0},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6447714,1,{0,0,0,1.75,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"barrel_01w",{"map-barrel-01.img",0.4,1,13224393,10,0,0,0},1,{0,0,0,1.75,0},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6447714,1,{0,0,0,1.75,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"barrel_01bh",{"map-barrel-01.img",0.4,1,13224393,10,0,0,0},1,{0,0,0,1.75,0},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6447714,1,{0,0,0,1.75,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"barrel_01f",{"map-barrel-01.img",0.4,1,13224393,10,0,0,0},1,{0,0,0,1.75,0},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6447714,1,{0,0,0,1.75,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"barrel_01bd",{"map-barrel-01.img",0.4,1,16777215,10,0,0,0},1,{0,0,0,1.75,0},1,0,0,0,0,0,1,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6447714,1,{0,0,0,1.75,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"propane_01",{"map-propane-01.img",0.4,1,16777215,10,0,0,0},1,{0,0,0,1.25,0},1,0,0,0,0,0,1,"explosion_barrel",0,"",0,0,1,16777215,1,0.5,0.5,1,1,24516,1,{0,0,0,1.25,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bollard_01",{"map-bollard-01.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,1.25,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6310464,1,{0,0,0,1.25,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bush_01",{"map-bush-01.img",0.5,0.97,16777215,60,0,0,0},1,{0,0,0,1.4,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,24320,1.5,{0,0,0,1.4,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bush_01b",{"map-bush-01.img",0.5,1,16777215,60,0,0,0},1,{0,0,0,1.4,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,24320,1.5,{0,0,0,1.4,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bush_01cb",{"map-bush-01cb.img",0.5,0.97,16777215,60,0,0,0},1,{0,0,0,1.4,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,2518873,1.5,{0,0,0,1.4,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bush_01f",{"map-bush-01f.img",0.5,0.97,16777215,60,0,0,0},1,{0,0,0,1.4,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,1793032,1.5,{0,0,0,1.4,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bush_01sv",{"map-bush-01sv.img",0.5,0.97,16777215,60,0,0,0},1,{0,0,0,1.4,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,7569455,1.5,{0,0,0,1.4,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brush_01sv",{"map-brush-01sv.img",0.5,0.97,16777215,60,0,0,0},1,{1,-1.75,-1.75,1.75,1.75},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,5207588,1.5,{1,-1.75,-1.75,1.75,1.75},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brush_02sv",{"map-brush-02sv.img",0.5,0.97,16777215,60,0,0,0},1,{1,-1.75,-1.75,1.75,1.75},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,5207588,1.5,{1,-1.75,-1.75,1.75,1.75},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bush_01x",{"map-bush-01x.img",0.5,0.97,16777215,60,0,0,0},1,{0,0,0,1.4,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,4545840,1.5,{0,0,0,1.4,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bush_02",{"map-bush-01.img",0.5,0.97,16777215,60,0,0,0},1,{0,0,0,1.4,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,24320,1.5,{0,0,0,1.4,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bush_03",{"map-bush-03.img",0.5,1,16777215,60,0,0,0},1,{0,0,0,1.4,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,24320,1.5,{0,0,0,1.4,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bush_04",{"map-bush-04.img",0.5,1,16777215,60,0,0,0},1,{0,0,0,1.4,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,24320,1.5,{0,0,0,1.4,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bush_04cb",{"map-bush-04cb.img",0.5,1,16777215,60,0,0,0},1,{0,0,0,1.4,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,2784099,1.5,{0,0,0,1.4,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bush_05",{"map-bush-05.img",0.5,0.97,16777215,60,0,0,0},1,{0,0,0,1.4,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6971965,1.5,{0,0,0,1.4,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bush_06",{"map-bush-06.img",0.5,0.97,16777215,60,0,0,0},1,{0,0,0,1.75,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,16489473,1.5,{0,0,0,1.75,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bush_06tr",{"map-bush-06tr.img",0.5,0.97,16777215,60,0,0,0},1,{0,0,0,2.5,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,14853402,1,{0,0,0,2.5,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bush_06b",{"map-bush-06.img",0.5,1,16777215,60,0,0,0},1,{0,0,0,1.75,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,14041344,1.5,{0,0,0,1.75,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bush_07",{"map-bush-07.img",0.5,1,16777215,60,0,0,0},1,{0,0,0,1.4,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,24320,1.5,{0,0,0,1.4,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bush_07sp",{"map-bush-07sp.img",0.5,1,16777215,60,0,0,0},1,{0,0,0,1.4,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,671242,1.5,{0,0,0,1.4,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bush_07x",{"map-bush-07x.img",0.5,0.97,16777215,60,0,0,0},1,{0,0,0,1.4,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,24320,1.5,{0,0,0,1.4,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bush_07cb",{"map-bush-07cb.img",0.5,0.97,16777215,60,0,0,0},1,{0,0,0,1.4,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,24320,1.5,{0,0,0,1.4,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"campfire_01",{"map-campfire-01.img",0.375,1,16777215,10,0,0,0},1,{0,0,0,2.75,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6447714,1,{0,0,0,2.75,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"potato_01",{"map-potato-01.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,1.1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,9466197,1,{0,0,0,1.1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"potato_01f",{"map-potato-01.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,1.1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,9466197,1,{0,0,0,1.1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"potato_02",{"map-potato-02.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,1.1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,9466197,1,{0,0,0,1.1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"potato_02f",{"map-potato-02.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,1.1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,9466197,1,{0,0,0,1.1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"potato_03",{"map-potato-03.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,1.1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,9466197,1,{0,0,0,1.1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"potato_03f",{"map-potato-03.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,1.1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,9466197,1,{0,0,0,1.1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tomato_01",{"map-tomato-01.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,1.1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,9466197,1,{0,0,0,1.1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tomato_02",{"map-tomato-02.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,1.1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,9466197,1,{0,0,0,1.1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tomato_03",{"map-tomato-03.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,1.1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,9466197,1,{0,0,0,1.1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"egg_01",{"map-egg-01.img",0.35,1,16777215,10,0,0,0},1,{0,0,0,1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{0,0,0,1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"egg_02",{"map-egg-02.img",0.35,1,16777215,10,0,0,0},1,{0,0,0,1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{0,0,0,1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"egg_03",{"map-egg-03.img",0.35,1,16777215,10,0,0,0},1,{0,0,0,1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{0,0,0,1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"egg_04",{"map-egg-04.img",0.35,1,16777215,10,0,0,0},1,{0,0,0,1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6697728,0.875,{0,0,0,1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"pumpkin_01",{"map-pumpkin-01.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,1.9,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,15889667,1,{0,0,0,1.9,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"pumpkin_02",{"map-pumpkin-02.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,1.9,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,15889667,1,{0,0,0,1.9,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"pumpkin_03",{"map-pumpkin-04.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,1.25,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,15889667,1,{0,0,0,1.25,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"squash_01",{"map-squash-03.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6452036,1.25,{0,0,0,1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"squash_02",{"map-squash-02.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,1.5,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,16569521,1.25,{0,0,0,1.5,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"sandbags_01",{"map-sandbags-01.img",0.5,1,16777215,10,0,0,0},1,{1,-3.1,-1.4,3.1,1.4},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,13278307,1,{1,-3.1,-1.4,3.1,1.4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"sandbags_02",{"map-sandbags-02.img",0.5,1,16777215,10,0,0,0},1,{1,-1.1,-1.4,1.1,1.4},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,13278307,1,{1,-1.1,-1.4,1.1,1.4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"silo_01",{"map-silo-01.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,7.75,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,4079166,1,{0,0,0,7.75,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"silo_01po",{"map-silo-01.img",0.5,1,16749645,10,0,0,0},1,{0,0,0,7.75,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,4079166,1,{0,0,0,7.75,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"statue_01",{"map-statue-01.img",0.5,1,16777215,10,0,0,0},1,{1,-4.4,-4.4,4.4,4.4},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,5723991,1,{1,-4.4,-4.4,4.4,4.4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"statue_03",{"map-statue-03.img",0.5,1,16777215,10,0,0,0},1,{1,-4.4,-4.4,4.4,4.4},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,5723991,1,{1,-4.4,-4.4,4.4,4.4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"statue_04",{"map-statue-04.img",0.5,1,16777215,10,0,0,0},1,{1,-4.4,-4.4,4.4,4.4},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,5723991,1,{1,-4.4,-4.4,4.4,4.4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"statue_top_01",{"map-statue-top-01.img",0.5,1,16777215,60,0,0,0},1,{0,0,0,2.45,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,5723991,1,{0,0,0,2.45,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"statue_top_02",{"map-statue-top-02.img",0.5,1,16777215,60,0,0,0},1,{0,0,0,2.45,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,5723991,1,{0,0,0,2.45,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stone_01",{"map-stone-01.img",0.4,1,16777215,10,0,0,0},1,{0,0,0,1.6,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,11776947,1,{0,0,0,1.6,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stone_01b",{"map-stone-01.img",0.4,1,16777215,10,0,0,0},1,{0,0,0,1.6,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,11776947,1,{0,0,0,1.6,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stone_01cb",{"map-stone-01cb.img",0.4,1,16777215,10,0,0,0},1,{0,0,0,1.6,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,10265256,1,{0,0,0,1.6,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stone_01f",{"map-stone-01.img",0.4,1,16777215,10,0,0,0},1,{0,0,0,1.6,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,8224125,1,{0,0,0,1.6,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stone_01sv",{"map-stone-01.img",0.4,1,16777215,10,0,0,0},1,{0,0,0,1.6,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,11776947,1,{0,0,0,1.6,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stone_01x",{"map-stone-01x.img",0.4,1,16777215,10,0,0,0},1,{0,0,0,1.6,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6052956,1,{0,0,0,1.6,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stone_02",{"map-stone-01.img",0.4,1,15066597,10,0,0,0},1,{0,0,0,1.6,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,11776947,1,{0,0,0,1.6,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stone_02sv",{"map-stone-01.img",0.4,1,15066597,10,0,0,0},1,{0,0,0,1.6,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,11776947,1,{0,0,0,1.6,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stone_02cb",{"map-stone-01cb.img",0.4,1,15066597,10,0,0,0},1,{0,0,0,1.6,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,10265256,1,{0,0,0,1.6,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stone_02w",{"map-stone-01.img",0.4,1,15066597,10,0,0,0},1,{0,0,0,1.6,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,11776947,1,{0,0,0,1.6,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stone_02x",{"map-stone-01x.img",0.4,1,15066597,10,0,0,0},1,{0,0,0,1.6,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,10265256,1,{0,0,0,1.6,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stone_02bh",{"map-stone-01.img",0.4,1,15066597,10,0,0,0},1,{0,0,0,1.6,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,11776947,1,{0,0,0,1.6,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stone_02f",{"map-stone-01.img",0.4,1,15066597,10,0,0,0},1,{0,0,0,1.6,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,11776947,1,{0,0,0,1.6,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stone_03",{"map-stone-03.img",0.4,1,16777215,10,0,0,0},1,{0,0,0,2.9,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,5197647,1,{0,0,0,2.9,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stone_03b",{"map-stone-03b.img",0.4,1,16777215,10,0,0,0},1,{0,0,0,2.9,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,5197647,1,{0,0,0,2.9,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stone_03cb",{"map-stone-03cb.img",0.4,1,16777215,10,0,0,0},1,{0,0,0,2.9,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,5197647,1,{0,0,0,2.9,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stone_03f",{"map-stone-03f.img",0.4,1,16777215,10,0,0,0},1,{0,0,0,2.9,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,5197647,1,{0,0,0,2.9,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stone_03sv",{"map-stone-03sv.img",0.4,1,16777215,10,0,0,0},1,{0,0,0,2.9,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,5197647,1,{0,0,0,2.9,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stone_03x",{"map-stone-03x.img",0.4,1,16777215,10,0,0,0},1,{0,0,0,2.9,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,5197647,1,{0,0,0,2.9,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stone_03tr",{"map-stone-03tr.img",0.4,1,16777215,10,0,0,0},1,{0,0,0,2.9,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,5197647,1,{0,0,0,2.9,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stone_03bh",{"map-stone-03bh.img",0.4,1,16777215,10,0,0,0},1,{0,0,0,2.9,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,5197647,1,{0,0,0,2.9,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stone_04",{"map-stone-04.img",0.4,1,16777215,10,0,0,0},1,{1,-1.8,-1.8,1.8,1.8},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,1512466,1,{1,-1.8,-1.8,1.8,1.8},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stone_04x",{"map-stone-04x.img",0.4,1,16777215,10,0,0,0},1,{1,-1.8,-1.8,1.8,1.8},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,11725567,1,{1,-1.8,-1.8,1.8,1.8},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stone_05",{"map-stone-05.img",0.4,1,16777215,10,0,0,0},1,{0,0,0,1.7,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,1512466,1,{0,0,0,1.7,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stone_06",{"map-stone-06.img",0.5,1,16777215,10,0,0,0},1,{1,-4.5,-2,4.5,2},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,3618615,1,{1,-4.5,-2,4.5,2},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stone_07",{"map-stone-07.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,7.75,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,9931908,1,{0,0,0,7.75,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stone_08",{"map-stone-03.img",0.4,1,15132390,10,0,0,0},1,{0,0,0,2.9,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,5197647,1,{0,0,0,2.9,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stone_08x",{"map-stone-03x.img",0.4,1,15132390,10,0,0,0},1,{0,0,0,2.9,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,5197647,1,{0,0,0,2.9,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"stone_08cb",{"map-stone-03cb.img",0.4,1,15132390,10,0,0,0},1,{0,0,0,2.9,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,5197647,1,{0,0,0,2.9,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_01",{"map-tree-03.img",0.7,1,16777215,800,0,0,0},1,{0,0,0,1.55,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,4083758,2.5,{0,0,0,1.55,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_01cb",{"map-tree-03cb.img",0.7,1,16777215,800,0,0,0},1,{0,0,0,1.2,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,2900834,2.5,{0,0,0,1.2,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_01sv",{"map-tree-03sv.img",0.7,1,16777215,800,0,0,0},1,{0,0,0,1.55,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,4411673,2.5,{0,0,0,1.55,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_interior_01",{"map-tree-03.img",0.7,1,16777215,200,0,0,0},1,{0,0,0,1.55,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,4083758,2.5,{0,0,0,1.55,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_interior_01bh",{"map-tree-13.img",0.35,1,16777215,200,0,0,0},1,{0,0,0,1.55,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,4083758,2.5,{0,0,0,1.55,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_interior_01de",{"map-tree-14.img",0.35,1,16777215,200,0,0,0},1,{0,0,0,1.55,0},1,0,0,1,0,1,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,4083758,2.5,{0,0,0,1.55,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_01x",{"map-tree-01x.img",0.35,1,16777215,800,0,0,0},1,{0,0,0,1.55,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,4083758,2.5,{0,0,0,1.55,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_02",{"map-tree-04.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,1.6,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,4083758,2.5,{0,0,0,1.6,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_02h",{"map-tree-04h.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,1.6,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,4083758,2.5,{0,0,0,1.6,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_03",{"map-tree-03.img",0.7,1,11645361,800,0,0,0},1,{0,0,0,1.55,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,4083758,2.5,{0,0,0,1.55,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_03su",{"map-tree-07su.img",0.7,1,11645361,800,0,0,0},1,{0,0,0,1.55,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,2185478,2.5,{0,0,0,1.55,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_03sp",{"map-tree-07sp.img",0.7,1,11645361,800,0,0,0},1,{0,0,0,1.55,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,16697057,2.5,{0,0,0,1.55,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_03x",{"map-tree-10.img",0.7,1,13158600,800,0,0,0},1,{0,0,0,1.55,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,4083758,2.5,{0,0,0,1.55,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_03sv",{"map-tree-03sv.img",0.7,1,11645361,800,0,0,0},1,{0,0,0,1.55,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,4411673,2.5,{0,0,0,1.55,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_03d",{"map-tree-06.img",0.7,1,11645361,800,0,0,0},1,{0,0,0,1.55,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,7700520,2.5,{0,0,0,1.55,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_03f",{"map-tree-08f.img",0.35,1,11645361,801,0,0,0},1,{0,0,0,1.55,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,995844,3,{0,0,0,1.55,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_03w",{"map-tree-07.img",0.7,1,11645361,800,0,0,0},1,{0,0,0,1.55,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,5199637,2.5,{0,0,0,1.55,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_03h",{"map-tree-07.img",0.7,1,11645361,800,0,0,0},1,{0,0,0,1.55,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,5199637,2.5,{0,0,0,1.55,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_03cb",{"map-tree-03cb.img",0.7,1,11645361,800,0,0,0},1,{0,0,0,1.2,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,2900834,2.5,{0,0,0,1.2,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_03bh",{"map-tree-13.img",0.35,1,11645361,800,0,0,0},1,{0,0,0,1.1,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,4083758,2.5,{0,0,0,1.1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_05",{"map-tree-05.img",0.7,1,16777215,801,0,0,0},1,{0,0,0,2.3,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,5911831,3,{0,0,0,2.3,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_05b",{"map-tree-05.img",0.7,1,16777215,801,0,0,0},1,{0,0,0,2.3,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,5911831,3,{0,0,0,2.3,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_05c",{"map-tree-05c.img",0.35,1,16777215,801,0,0,0},1,{0,0,0,1.05,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,9064739,3,{0,0,0,1.05,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_06",{"map-tree-06.img",0.7,1,16777215,800,0,0,0},1,{0,0,0,1.55,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,7700520,2.5,{0,0,0,1.55,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_07",{"map-tree-07.img",0.7,1,16777215,800,0,0,0},1,{0,0,0,1.55,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,5199637,2.5,{0,0,0,1.55,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_07sp",{"map-tree-07sp.img",0.7,1,16777215,800,0,0,0},1,{0,0,0,1.55,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,16697057,2.5,{0,0,0,1.55,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_07spr",{"map-tree-07sp.img",0.7,1,16777215,800,0,0,0},1,{0,0,0,1.55,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,16697057,2.5,{0,0,0,1.55,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_07su",{"map-tree-07su.img",0.7,1,16777215,800,0,0,0},1,{0,0,0,1.55,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,2185478,2.5,{0,0,0,1.55,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_08",{"map-tree-08.img",0.35,1,16777215,800,0,0,0},1,{0,0,0,1.55,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,11033868,2.5,{0,0,0,1.55,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_08b",{"map-tree-08.img",0.35,1,14383224,801,0,0,0},1,{0,0,0,1.55,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,9647632,3,{0,0,0,1.55,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_08c",{"map-tree-08.img",0.35,1,11645361,801,0,0,0},1,{0,0,0,1.55,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,7817749,3,{0,0,0,1.55,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_08f",{"map-tree-08f.img",0.35,1,16777215,801,0,0,0},1,{0,0,0,1.55,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,995844,3,{0,0,0,1.55,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_08sp",{"map-tree-08sp.img",0.35,1,16777215,800,0,0,0},1,{0,0,0,1.55,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,16746936,2.5,{0,0,0,1.55,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_08spb",{"map-tree-08sp.img",0.35,1,14383224,801,0,0,0},1,{0,0,0,1.55,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,16734619,3,{0,0,0,1.55,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_08spc",{"map-tree-08sp.img",0.35,1,11645361,801,0,0,0},1,{0,0,0,1.55,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,8268107,3,{0,0,0,1.55,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_08spr",{"map-tree-08sp.img",0.35,1,16777215,800,0,0,0},1,{0,0,0,1.55,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,16746936,2.5,{0,0,0,1.55,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_08su",{"map-tree-08su.img",0.35,1,16777215,801,0,0,0},1,{0,0,0,1.55,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,2183181,2.5,{0,0,0,1.55,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_08sub",{"map-tree-08su.img",0.35,1,9211210,801,0,0,0},1,{0,0,0,1.55,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,1785864,3,{0,0,0,1.55,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_09",{"map-tree-09.img",0.5,1,16777215,10,0,0,0},1,{0,0,0,1.6,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,8602624,1,{0,0,0,1.6,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_10",{"map-tree-10.img",0.7,1,16777215,800,0,0,0},1,{0,0,0,1.25,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,7571807,2.5,{0,0,0,1.25,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_11",{"map-tree-11.img",0.75,0.92,16777215,201,0,0,0},1,{0,0,0,1.25,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,4083758,2.5,{0,0,0,1.25,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_interior_11",{"map-tree-11.img",0.5,0.92,16777215,200,0,0,0},1,{0,0,0,1.25,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,4083758,2.5,{0,0,0,1.25,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_12",{"map-tree-12.img",0.7,1,16777215,801,0,0,0},1,{0,0,0,1.55,0},1,0,0,1,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,8032292,7,{0,0,0,1.55,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_13",{"map-tree-13.img",0.35,1,16777215,801,0,0,0},1,{0,0,0,1,0},1,0,0,1,0,1,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,4083758,2.5,{0,0,0,1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_13bh",{"map-tree-13.img",0.35,1,16777215,801,0,0,0},1,{0,0,0,1,0},1,0,0,1,0,1,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,4083758,2.5,{0,0,0,1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_13x",{"map-tree-13x.img",0.35,1,16777215,801,0,0,0},1,{0,0,0,1,0},1,0,0,1,0,1,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,4083758,2.5,{0,0,0,1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_14",{"map-tree-14.img",0.35,1,16777215,801,0,0,0},1,{0,0,0,1,0},1,0,0,1,0,1,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,4083758,2.5,{0,0,0,1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_14d",{"map-tree-14.img",0.35,1,16777215,801,0,0,0},1,{0,0,0,1,0},1,0,0,1,0,1,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,4083758,2.5,{0,0,0,1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"tree_14x",{"map-tree-14x.img",0.35,1,16777215,801,0,0,0},1,{0,0,0,1,0},1,0,0,1,0,1,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,4083758,2.5,{0,0,0,1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"woodpile_01",{"map-woodpile-01.img",0.5,1,16777215,10,0,0,0},1,{1,-1.5,-1.5,1.5,1.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,9455616,0.875,{1,-1.5,-1.5,1.5,1.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"woodpile_02",{"map-woodpile-02.img",0.5,1,16777215,10,0,0,0},1,{1,-6,-3,6,3},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,0.8,{1,-6,-3,6,3},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"woodpile_03",{"map-woodpile-03.img",0.5,1,16777215,10,0,0,0},1,{1,-3,-1.75,3,1.75},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6697728,0.8,{1,-3,-1.75,3,1.75},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bank_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_741,3,1,kFloorImgs_741,2,kCeilImgs_741,3,5.5,2.75,0.5,6,"",nullptr,0,kZoomIns_741,3},
    {"barn_basement_stairs_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,2,kFloorImgs_742,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"barn_basement_floor_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_743,1,kCeilImgs_743,1,5.5,2.75,0,12,"",nullptr,0,kZoomIns_743,2},
    {"barn_basement_floor_02",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_744,1,kCeilImgs_744,1,5.5,2.75,0,12,"",nullptr,0,kZoomIns_744,1},
    {"barn_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-28,-18.5,28,19},1,nullptr,0,nullptr,0,nullptr,0,kShapes_745,2,1,kFloorImgs_745,1,kCeilImgs_745,2,5.5,2.75,0.5,6,"",nullptr,0,kZoomIns_745,2},
    {"barn_02",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-28,-18.5,28,19},1,nullptr,0,nullptr,0,nullptr,0,kShapes_746,2,1,kFloorImgs_746,1,kCeilImgs_746,2,5.5,2.75,0.5,6,"",nullptr,0,kZoomIns_746,2},
    {"bathhouse_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_747,6,nullptr,0,5.5,2.75,0.5,6,"",kEmitters_747,2,kZoomIns_747,5},
    {"bathhouse_sideroom_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,1,kFloorImgs_748,1,kCeilImgs_748,1,5.5,3.25,0.5,6,"",nullptr,0,kZoomIns_748,1},
    {"bathhouse_sideroom_02",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,1,kFloorImgs_749,1,kCeilImgs_749,1,5.5,2.75,0.5,6,"",nullptr,0,kZoomIns_749,1},
    {"bridge_lg_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_750,5,1,kFloorImgs_750,2,kCeilImgs_750,1,10,2.75,0,12,"",nullptr,0,kZoomIns_750,1},
    {"bridge_lg_under_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0.5,6,"",nullptr,0,nullptr,0},
    {"bridge_xlg_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_752,5,1,kFloorImgs_752,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bridge_xlg_under_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0.5,6,"",nullptr,0,nullptr,0},
    {"bridge_md_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_754,1,1,kFloorImgs_754,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bridge_md_under_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0.5,6,"",nullptr,0,nullptr,0},
    {"cabin_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_756,2,1,kFloorImgs_756,1,kCeilImgs_756,3,5.5,2.75,0.5,6,"",kEmitters_756,1,kZoomIns_756,2},
    {"club_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_757,9,1,kFloorImgs_757,8,kCeilImgs_757,3,7.5,2.5,0.5,6,"",nullptr,0,kZoomIns_757,9},
    {"club_vault",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,2,kFloorImgs_758,1,kCeilImgs_758,1,5.5,2.75,0,12,"",nullptr,0,kZoomIns_758,1},
    {"club_complex_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-41,-52,31,21},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"container_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,2703694,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,1,kFloorImgs_760,1,kCeilImgs_760,1,5.5,2.75,0,12,"",nullptr,0,kZoomIns_760,1},
    {"container_02",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,2703694,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,1,kFloorImgs_761,1,kCeilImgs_761,1,5.5,2.75,0,12,"",nullptr,0,kZoomIns_761,1},
    {"container_03",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,2703694,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,1,kFloorImgs_762,1,kCeilImgs_762,1,5.5,2.75,0,12,"",nullptr,0,kZoomIns_762,1},
    {"container_04",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,2703694,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,1,kFloorImgs_763,1,kCeilImgs_763,1,5.5,2.75,0,12,"",nullptr,0,kZoomIns_763,1},
    {"container_05",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,11485762,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,1,kFloorImgs_764,1,kCeilImgs_764,1,5.5,2.75,0,12,"",nullptr,0,kZoomIns_764,1},
    {"container_06",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,2703694,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,1,kFloorImgs_765,1,kCeilImgs_765,1,5.5,2.75,0,12,"",nullptr,0,kZoomIns_765,1},
    {"dock_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_766,2,1,kFloorImgs_766,2,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"greenhouse_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,1995644,1,{1,-17.5,-25,22,25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,1,kFloorImgs_767,4,kCeilImgs_767,2,7.5,2.75,0.5,6,"",nullptr,0,kZoomIns_767,1},
    {"greenhouse_02",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,1995644,1,{1,-17.5,-25,22,25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,1,kFloorImgs_768,4,kCeilImgs_768,1,7.5,2.75,0.5,6,"",nullptr,0,kZoomIns_768,1},
    {"hedgehog_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_769,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"hut_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_770,2,0,kFloorImgs_770,2,kCeilImgs_770,1,5.5,4,0,12,"map-hut-res-01.img",nullptr,0,kZoomIns_770,1},
    {"hut_02",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_771,2,0,kFloorImgs_771,2,kCeilImgs_771,1,5.5,4,0,12,"map-hut-res-01.img",nullptr,0,kZoomIns_771,1},
    {"hut_03",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_772,2,0,kFloorImgs_772,2,kCeilImgs_772,1,5.5,4,0,12,"map-hut-res-01.img",nullptr,0,kZoomIns_772,1},
    {"house_red_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6429724,1,{1,-19,-17.5,19,17.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,1,kFloorImgs_773,3,kCeilImgs_773,1,5.5,2.75,0.5,6,"",nullptr,0,kZoomIns_773,1},
    {"house_red_02",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,4656911,1,{1,-19,-19.5,19,17.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,1,kFloorImgs_774,10,kCeilImgs_774,1,5.5,2.75,0.5,6,"",nullptr,0,kZoomIns_774,1},
    {"mansion_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_775,5,1,kFloorImgs_775,4,kCeilImgs_775,1,5.5,2.75,0.5,6,"",nullptr,0,kZoomIns_775,3},
    {"mansion_cellar_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_776,4,nullptr,0,5.5,2.75,0.5,6,"",nullptr,0,kZoomIns_776,2},
    {"outhouse_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,8145976,1,{1,-5.5,-5.1,5.5,7.9},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,1,kFloorImgs_777,1,kCeilImgs_777,1,5.5,2.75,0,12,"map-outhouse-res.img",nullptr,0,kZoomIns_777,1},
    {"panicroom_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,2,kFloorImgs_778,1,kCeilImgs_778,1,5.5,2.75,0,12,"",nullptr,0,kZoomIns_778,1},
    {"police_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_779,10,1,kFloorImgs_779,2,kCeilImgs_779,3,5.5,2.75,0.5,6,"",nullptr,0,kZoomIns_779,4},
    {"saferoom_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,2,kFloorImgs_780,1,kCeilImgs_780,1,5.5,2.75,0,12,"",nullptr,0,kZoomIns_780,1},
    {"shack_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_781,1,kCeilImgs_781,1,5.5,4,0,12,"map-shack-res-01.img",nullptr,0,kZoomIns_781,1},
    {"shack_02",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,4014894,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,1,kFloorImgs_782,1,kCeilImgs_782,1,5.5,4,0,12,"map-shack-res-02.img",nullptr,0,kZoomIns_782,1},
    {"shack_03a",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_783,4,1,kFloorImgs_783,2,kCeilImgs_783,1,5.5,4,0,12,"map-shack-res-03.img",nullptr,0,kZoomIns_783,1},
    {"shack_03b",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_784,4,1,kFloorImgs_784,2,kCeilImgs_784,1,5.5,4,0,12,"map-shack-res-03.img",nullptr,0,kZoomIns_784,1},
    {"teahouse_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_785,4,0,kFloorImgs_785,3,kCeilImgs_785,1,5.5,4,0,12,"map-building-teahouse-res-01.img",nullptr,0,kZoomIns_785,1},
    {"teahouse_complex_01s",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-24,-18,24,18},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"warehouse_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-35,-16,35,16},1,nullptr,0,nullptr,0,nullptr,0,kShapes_787,3,1,kFloorImgs_787,2,kCeilImgs_787,1,8,5,0,12,"",nullptr,0,kZoomIns_787,1},
    {"warehouse_02",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_788,3,1,kFloorImgs_788,2,kCeilImgs_788,1,8,5,0,12,"",nullptr,0,kZoomIns_788,1},
    {"warehouse_03",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-35,-16,35,16},1,nullptr,0,nullptr,0,nullptr,0,kShapes_789,3,1,kFloorImgs_789,2,kCeilImgs_789,1,8,5,0,12,"",nullptr,0,kZoomIns_789,1},
    {"warehouse_complex_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-42,-40,108,78},1,nullptr,0,nullptr,0,nullptr,0,kShapes_790,20,0,kFloorImgs_790,3,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"vault_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,2,kFloorImgs_791,1,kCeilImgs_791,1,7.25,2.75,0.5,6,"",nullptr,0,kZoomIns_791,1},
    {"hut_01bh",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_792,2,0,kFloorImgs_792,2,kCeilImgs_792,1,5.5,4,0,12,"map-hut-res-01.img",nullptr,0,kZoomIns_792,1},
    {"hut_04",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_793,4,0,kFloorImgs_793,2,kCeilImgs_793,1,5.5,4,0,12,"map-hut-res-02.img",nullptr,0,kZoomIns_793,1},
    {"mansion_03",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_794,5,1,kFloorImgs_794,4,kCeilImgs_794,1,5.5,2.75,0.5,6,"",nullptr,0,kZoomIns_794,3},
    {"mansion_cellar_03",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_795,4,nullptr,0,5.5,2.75,0.5,6,"",nullptr,0,kZoomIns_795,2},
    {"teahouse_complex_01cb",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-24,-18,24,18},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"archway_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,7813914,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,kCeilImgs_797,1,5.5,2.75,0,12,"map-archway-res-01.img",nullptr,0,nullptr,0},
    {"bank_01b",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_798,3,1,kFloorImgs_798,2,kCeilImgs_798,3,5.5,2.75,0.5,6,"",nullptr,0,kZoomIns_798,3},
    {"vault_01b",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,2,kFloorImgs_799,1,kCeilImgs_799,1,7.25,2.75,0.5,6,"",nullptr,0,kZoomIns_799,1},
    {"barn_basement_floor_01d",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_800,1,kCeilImgs_800,1,5.5,2.75,0,12,"",nullptr,0,kZoomIns_800,2},
    {"barn_basement_floor_02d",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_801,1,kCeilImgs_801,1,5.5,2.75,0,12,"",nullptr,0,kZoomIns_801,1},
    {"barn_02d",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-28,-18.5,28,19},1,nullptr,0,nullptr,0,nullptr,0,kShapes_802,2,1,kFloorImgs_802,1,kCeilImgs_802,2,5.5,2.75,0.5,6,"",nullptr,0,kZoomIns_802,2},
    {"desert_town_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-70,-120,65,120},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"desert_town_02",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-85,-89,85,57},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"oasis_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-43,-43,43,43},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"river_town_02",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-95,-50,85,36},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_806,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-68,-37,68,39},1,nullptr,0,nullptr,0,nullptr,0,kShapes_807,6,1,kFloorImgs_807,4,kCeilImgs_807,5,5.5,2.75,0.5,6,"",kEmitters_807,1,kZoomIns_807,7},
    {"reserve_basement_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-70,-35,70,35},1,nullptr,0,nullptr,0,nullptr,0,kShapes_808,1,1,kFloorImgs_808,5,nullptr,0,5.5,2.75,0.5,6,"",nullptr,0,kZoomIns_808,7},
    {"reserve_armory_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,1,kFloorImgs_809,1,kCeilImgs_809,1,5.5,3.25,0.5,6,"",nullptr,0,kZoomIns_809,1},
    {"reserve_security_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,1,kFloorImgs_810,1,kCeilImgs_810,1,5.5,3.25,0.5,6,"",nullptr,0,kZoomIns_810,1},
    {"reserve_vault_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,1,kFloorImgs_811,1,kCeilImgs_811,1,5.5,2.75,0.5,6,"",nullptr,0,kZoomIns_811,1},
    {"saloon_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-22.5,-22.5,22.5,22.5},1,nullptr,0,nullptr,0,nullptr,0,kShapes_812,4,1,kFloorImgs_812,2,kCeilImgs_812,3,5.5,2.75,0.5,6,"",kEmitters_812,1,kZoomIns_812,1},
    {"saloon_cellar_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,1,kFloorImgs_813,1,kCeilImgs_813,1,7,3,0,12,"",nullptr,0,kZoomIns_813,1},
    {"statue_building_04",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,2,kFloorImgs_814,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"statue_underground_04",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_815,1,kCeilImgs_815,1,5,3,0,12,"",nullptr,0,kZoomIns_815,1},
    {"statue_building_03",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,2,kFloorImgs_816,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"statue_underground_03",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_817,1,kCeilImgs_817,1,5,3,0,12,"",nullptr,0,kZoomIns_817,1},
    {"river_town_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-121,-56,122,75},1,nullptr,0,nullptr,0,nullptr,0,kShapes_818,4,0,kFloorImgs_818,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"statue_structure_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_819,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"statue_structure_02",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_820,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"warehouse_01f",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-35,-16,35,16},1,nullptr,0,nullptr,0,nullptr,0,kShapes_821,3,1,kFloorImgs_821,2,kCeilImgs_821,1,8,5,0,12,"",nullptr,0,kZoomIns_821,1},
    {"barn_01h",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-28,-18.5,28,19},1,nullptr,0,nullptr,0,nullptr,0,kShapes_822,2,1,kFloorImgs_822,1,kCeilImgs_822,2,5.5,2.75,0.5,6,"",nullptr,0,kZoomIns_822,2},
    {"warehouse_01h",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-35,-16,35,16},1,nullptr,0,nullptr,0,nullptr,0,kShapes_823,3,1,kFloorImgs_823,2,kCeilImgs_823,1,8,5,0,12,"",nullptr,0,kZoomIns_823,1},
    {"junkyard_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-37,-37,37,37},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"house_red_01h",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6429724,1,{1,-19,-17.5,19,17.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,1,kFloorImgs_825,3,kCeilImgs_825,1,5.5,2.75,0.5,6,"",nullptr,0,kZoomIns_825,1},
    {"house_red_02h",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,4656911,1,{1,-19,-19.5,19,17.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,1,kFloorImgs_826,10,kCeilImgs_826,1,5.5,2.75,0.5,6,"",nullptr,0,kZoomIns_826,1},
    {"cabin_02",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_827,2,1,kFloorImgs_827,1,kCeilImgs_827,3,5.5,2.75,0.5,6,"",kEmitters_827,1,kZoomIns_827,2},
    {"mansion_02",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_828,5,1,kFloorImgs_828,4,kCeilImgs_828,1,5.5,2.75,0.5,6,"",nullptr,0,kZoomIns_828,3},
    {"mansion_cellar_02",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_829,4,nullptr,0,5.5,2.75,0.5,6,"",nullptr,0,kZoomIns_829,2},
    {"shilo_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,3240224,1,{1,-17,-16,17,14},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_830,2,kCeilImgs_830,1,5.5,4,0,12,"",nullptr,0,kZoomIns_830,1},
    {"warehouse_03sv",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-35,-16,35,16},1,nullptr,0,nullptr,0,nullptr,0,kShapes_831,3,1,kFloorImgs_831,2,kCeilImgs_831,1,8,5,0,12,"",nullptr,0,kZoomIns_831,1},
    {"kopje_brush_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-18,-18,18,18},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"grassy_cover_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-10,-10,10,10},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"grassy_cover_02",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-10,-10,10,10},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"grassy_cover_03",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-10,-10,10,10},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"grassy_cover_complex_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-30,-10,30,10},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brush_clump_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-17,-17,17,17},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brush_clump_02",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-17,-17,17,17},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"brush_clump_03",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-17,-17,17,17},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"savannah_patch_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-20,-16,20,16},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"kopje_patch_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-45,-35,45,35},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"perch_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,1915136,1,{1,-7,-8,7,8},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,1,kFloorImgs_842,1,kCeilImgs_842,1,5.5,2.75,0,12,"map-perch-res-01.img",nullptr,0,nullptr,0},
    {"oasis_01sv",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-43,-43,43,43},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"hut_01x",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_844,2,0,kFloorImgs_844,2,kCeilImgs_844,3,5.5,4,0,12,"map-hut-res-01.img",nullptr,0,kZoomIns_844,1},
    {"hut_02x",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_845,2,0,kFloorImgs_845,2,kCeilImgs_845,3,5.5,4,0,12,"map-hut-res-01.img",nullptr,0,kZoomIns_845,1},
    {"warehouse_01x",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-35,-16,35,16},1,nullptr,0,nullptr,0,nullptr,0,kShapes_846,3,1,kFloorImgs_846,2,kCeilImgs_846,5,8,5,0,12,"",nullptr,0,kZoomIns_846,1},
    {"warehouse_02x",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_847,3,1,kFloorImgs_847,2,kCeilImgs_847,4,8,5,0,12,"",nullptr,0,kZoomIns_847,1},
    {"warehouse_03x",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-35,-16,35,16},1,nullptr,0,nullptr,0,nullptr,0,kShapes_848,3,1,kFloorImgs_848,2,kCeilImgs_848,5,8,5,0,12,"",nullptr,0,kZoomIns_848,1},
    {"shack_01x",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_849,1,kCeilImgs_849,3,5.5,4,0,12,"map-shack-res-01.img",nullptr,0,kZoomIns_849,1},
    {"shack_02x",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,4014894,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,1,kFloorImgs_850,1,kCeilImgs_850,2,5.5,4,0,12,"map-shack-res-02.img",nullptr,0,kZoomIns_850,1},
    {"shack_03x",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_851,4,1,kFloorImgs_851,2,kCeilImgs_851,2,5.5,4,0,12,"map-shack-res-03.img",nullptr,0,kZoomIns_851,1},
    {"outhouse_01x",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,8145976,1,{1,-5.5,-5.1,5.5,7.9},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,1,kFloorImgs_852,1,kCeilImgs_852,2,5.5,2.75,0,12,"map-outhouse-res.img",nullptr,0,kZoomIns_852,1},
    {"outhouse_02",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,10371350,1,{1,-5.5,-5.1,5.5,7.9},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,1,kFloorImgs_853,1,kCeilImgs_853,1,5.5,2.75,0,12,"map-outhouse-res.img",nullptr,0,kZoomIns_853,1},
    {"barn_01x",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-28,-18.5,28,19},1,nullptr,0,nullptr,0,nullptr,0,kShapes_854,2,1,kFloorImgs_854,1,kCeilImgs_854,5,5.5,2.75,0.5,6,"",nullptr,0,kZoomIns_854,2},
    {"barn_02x",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-28,-18.5,28,19},1,nullptr,0,nullptr,0,nullptr,0,kShapes_855,2,1,kFloorImgs_855,1,kCeilImgs_855,5,5.5,2.75,0.5,6,"",nullptr,0,kZoomIns_855,2},
    {"bank_01x",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_856,3,1,kFloorImgs_856,2,kCeilImgs_856,7,5.5,2.75,0.5,6,"",nullptr,0,kZoomIns_856,3},
    {"police_01x",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_857,10,1,kFloorImgs_857,2,kCeilImgs_857,9,5.5,2.75,0.5,6,"",nullptr,0,kZoomIns_857,4},
    {"house_red_01x",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,6429724,1,{1,-19,-17.5,19,17.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,1,kFloorImgs_858,3,kCeilImgs_858,3,5.5,2.75,0.5,6,"",nullptr,0,kZoomIns_858,1},
    {"house_red_02x",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,4656911,1,{1,-19,-19.5,19,17.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,1,kFloorImgs_859,10,kCeilImgs_859,3,5.5,2.75,0.5,6,"",nullptr,0,kZoomIns_859,1},
    {"cabin_01x",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_860,2,1,kFloorImgs_860,1,kCeilImgs_860,6,5.5,2.75,0.5,6,"",kEmitters_860,1,kZoomIns_860,2},
    {"mansion_01x",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_861,5,1,kFloorImgs_861,4,kCeilImgs_861,8,5.5,2.75,0.5,6,"",nullptr,0,kZoomIns_861,3},
    {"teahouse_01x",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_862,4,0,kFloorImgs_862,3,kCeilImgs_862,4,5.5,4,0,12,"map-building-teahouse-res-01.img",nullptr,0,kZoomIns_862,1},
    {"teahouse_complex_01x",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-24,-18,24,18},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bridge_lg_01x",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_864,5,1,kFloorImgs_864,2,kCeilImgs_864,5,10,2.75,0,12,"",nullptr,0,kZoomIns_864,1},
    {"container_01x",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,1,2703694,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,1,kFloorImgs_865,1,kCeilImgs_865,2,5.5,2.75,0,12,"",nullptr,0,kZoomIns_865,1},
    {"workshop_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-30.5,-32.5,34.5,32.5},1,nullptr,0,nullptr,0,nullptr,0,kShapes_866,4,1,kFloorImgs_866,2,kCeilImgs_866,2,8,5,0,12,"",nullptr,0,kZoomIns_866,2},
    {"workshop_complex_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-33,-35,37,35},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"logging_complex_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-55,-54,55,46},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"logging_complex_02",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-40,-40,40,40},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"logging_complex_03",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-32,-32,32,32},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"camp_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-22.5,-22.5,22.5,22.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",kEmitters_871,1,kZoomIns_871,1},
    {"teapavilion_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-11,-32,11,11},1,nullptr,0,nullptr,0,nullptr,0,kShapes_872,3,0,kFloorImgs_872,2,kCeilImgs_872,1,5.5,4,0,12,"map-building-pavilion-res-01.img",nullptr,0,kZoomIns_872,1},
    {"teapavilion_complex_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-14,-32,14,14},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"logging_complex_01sp",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-55,-54,55,46},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"logging_complex_02sp",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-40,-40,40,40},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"logging_complex_03sp",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-32,-32,32,32},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"workshop_01w",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-30.5,-32.5,34.5,32.5},1,nullptr,0,nullptr,0,nullptr,0,kShapes_877,4,1,kFloorImgs_877,2,kCeilImgs_877,7,8,5,0,12,"",nullptr,0,kZoomIns_877,2},
    {"workshop_complex_01w",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-33,-35,37,35},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"logging_complex_02x",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-40,-40,40,40},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"logging_complex_03x",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-32,-32,32,32},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"camp_01w",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-22.5,-22.5,22.5,22.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",kEmitters_881,1,kZoomIns_881,1},
    {"teapavilion_01w",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-11,-32,11,11},1,nullptr,0,nullptr,0,nullptr,0,kShapes_882,3,0,kFloorImgs_882,2,kCeilImgs_882,1,5.5,4,0,12,"map-building-pavilion-res-01.img",nullptr,0,kZoomIns_882,1},
    {"logging_complex_01su",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-55,-54,55,46},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"logging_complex_02su",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-40,-40,40,40},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"logging_complex_03su",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-32,-32,32,32},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"teahouse_complex_01su",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-24,-18,24,18},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bunker_chrys_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_887,1,2,kFloorImgs_887,1,kCeilImgs_887,1,5.5,2.75,0,12,"",nullptr,0,kZoomIns_887,1},
    {"bunker_chrys_sublevel_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,1,kFloorImgs_888,2,kCeilImgs_888,1,5.5,2.75,0,12,"",nullptr,0,kZoomIns_888,1},
    {"bunker_chrys_sublevel_01b",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,1,kFloorImgs_889,2,kCeilImgs_889,1,5.5,2.75,0,12,"",nullptr,0,kZoomIns_889,1},
    {"bunker_chrys_compartment_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,2,kFloorImgs_890,2,kCeilImgs_890,2,5.5,2.75,0,12,"",nullptr,0,kZoomIns_890,1},
    {"bunker_chrys_compartment_01b",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,2,kFloorImgs_891,2,kCeilImgs_891,2,5.5,2.75,0,12,"",nullptr,0,kZoomIns_891,1},
    {"bunker_chrys_compartment_02",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,2,kFloorImgs_892,2,kCeilImgs_892,2,5.5,2.75,0,12,"",nullptr,0,kZoomIns_892,1},
    {"bunker_chrys_compartment_02b",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,2,kFloorImgs_893,2,kCeilImgs_893,2,5.5,2.75,0,12,"",nullptr,0,kZoomIns_893,1},
    {"bunker_chrys_compartment_03",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,2,kFloorImgs_894,1,kCeilImgs_894,2,5.5,2.75,0,12,"",nullptr,0,kZoomIns_894,1},
    {"bunker_chrys_compartment_03b",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,2,kFloorImgs_895,1,kCeilImgs_895,2,5.5,2.75,0,12,"",nullptr,0,kZoomIns_895,1},
    {"bunker_cloud_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,2,kFloorImgs_896,2,nullptr,0,5,2.75,0.5,6,"",nullptr,0,nullptr,0},
    {"bunker_cloud_sublevel_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,-45,-45,45,45},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,2,kFloorImgs_897,3,kCeilImgs_897,4,7,3,0,12,"",nullptr,0,kZoomIns_897,4},
    {"bunker_cloud_compartment_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,2,nullptr,0,kCeilImgs_898,2,5.5,2.75,0,12,"",nullptr,0,kZoomIns_898,2},
    {"bunker_cloud_compartment_02",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,2,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,kZoomIns_899,1},
    {"bunker_egg_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,2,kFloorImgs_900,1,nullptr,0,5,2.75,0.5,6,"",nullptr,0,nullptr,0},
    {"bunker_egg_sublevel_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_901,2,kCeilImgs_901,1,5,3,0,12,"",nullptr,0,kZoomIns_901,1},
    {"bunker_egg_sublevel_02",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_902,2,kCeilImgs_902,1,5,3,0,12,"",nullptr,0,kZoomIns_902,1},
    {"bunker_egg_sublevel_01sv",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_903,2,kCeilImgs_903,1,5,3,0,12,"",nullptr,0,kZoomIns_903,1},
    {"bunker_hydra_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_904,2,0,kFloorImgs_904,3,kCeilImgs_904,1,5,2.75,0.5,6,"",nullptr,0,kZoomIns_904,2},
    {"bunker_hydra_sublevel_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,1,kFloorImgs_905,4,kCeilImgs_905,3,10,3,0,12,"",nullptr,0,kZoomIns_905,3},
    {"bunker_hydra_compartment_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,2,kFloorImgs_906,1,kCeilImgs_906,1,5.5,2.75,0,12,"",nullptr,0,kZoomIns_906,1},
    {"bunker_hydra_compartment_02",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_907,1,kCeilImgs_907,1,5.5,2.75,0,12,"",nullptr,0,kZoomIns_907,1},
    {"bunker_hydra_compartment_03",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,2,kFloorImgs_908,1,kCeilImgs_908,1,5.5,2.75,0,12,"",nullptr,0,kZoomIns_908,1},
    {"bunker_storm_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_909,1,0,kFloorImgs_909,1,kCeilImgs_909,1,5,2.75,0.5,6,"none",nullptr,0,kZoomIns_909,1},
    {"bunker_storm_sublevel_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,1,kFloorImgs_910,2,kCeilImgs_910,1,5,3,0,12,"",nullptr,0,kZoomIns_910,1},
    {"bunker_conch_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_911,2,0,kFloorImgs_911,2,kCeilImgs_911,2,5.5,2.75,0,12,"",nullptr,0,kZoomIns_911,2},
    {"bunker_conch_sublevel_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,1,kFloorImgs_912,2,kCeilImgs_912,2,7,3,0,12,"",kEmitters_912,1,kZoomIns_912,2},
    {"bunker_conch_compartment_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,2,kFloorImgs_913,2,kCeilImgs_913,1,7,3,0,12,"",kEmitters_913,1,kZoomIns_913,2},
    {"bunker_crossing_stairs_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_914,1,1,kFloorImgs_914,1,kCeilImgs_914,1,5.5,2.75,0,12,"",nullptr,0,kZoomIns_914,1},
    {"bunker_crossing_stairs_01b",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_915,1,1,kFloorImgs_915,1,kCeilImgs_915,1,5.5,2.75,0,12,"",nullptr,0,kZoomIns_915,1},
    {"bunker_crossing_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_916,1,0,kFloorImgs_916,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bunker_crossing_sublevel_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,1,kFloorImgs_917,8,kCeilImgs_917,2,7,3,0,12,"",nullptr,0,kZoomIns_917,4},
    {"bunker_crossing_bathroom",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,2,kFloorImgs_918,1,kCeilImgs_918,1,5.5,2.75,0,12,"",nullptr,0,kZoomIns_918,1},
    {"bunker_crossing_compartment_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,2,kFloorImgs_919,2,kCeilImgs_919,2,7,3,0,12,"",kEmitters_919,1,kZoomIns_919,2},
    {"bunker_hatchet_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_920,1,0,kFloorImgs_920,1,kCeilImgs_920,1,5,2.75,0.5,6,"none",nullptr,0,kZoomIns_920,1},
    {"bunker_hatchet_sublevel_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,1,kFloorImgs_921,3,kCeilImgs_921,1,5,3,0,12,"",nullptr,0,kZoomIns_921,1},
    {"bunker_hatchet_compartment_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,2,kFloorImgs_922,1,kCeilImgs_922,1,5.5,2.75,0,12,"",nullptr,0,kZoomIns_922,1},
    {"bunker_hatchet_compartment_02",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,2,kFloorImgs_923,4,kCeilImgs_923,1,5.5,2.75,0,12,"",nullptr,0,kZoomIns_923,1},
    {"bunker_hatchet_compartment_03",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,2,kFloorImgs_924,4,kCeilImgs_924,1,5.5,2.75,0,12,"",nullptr,0,kZoomIns_924,1},
    {"bunker_eye_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_925,1,1,kFloorImgs_925,1,kCeilImgs_925,1,5.5,2.75,0,12,"",nullptr,0,kZoomIns_925,1},
    {"bunker_eye_sublevel_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_926,2,kCeilImgs_926,1,5,3,0,12,"",nullptr,0,kZoomIns_926,1},
    {"bunker_eye_compartment_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,2,kFloorImgs_927,1,kCeilImgs_927,1,5.5,2.75,0,12,"",nullptr,0,kZoomIns_927,1},
    {"bunker_twins_stairs_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,kShapes_928,1,1,kFloorImgs_928,1,kCeilImgs_928,1,5.5,2.75,0,12,"",nullptr,0,kZoomIns_928,1},
    {"bunker_twins_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,2,kFloorImgs_929,1,nullptr,0,5,2.75,0.5,6,"",nullptr,0,nullptr,0},
    {"bunker_twins_sublevel_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_930,1,kCeilImgs_930,1,5.5,2.75,0,12,"",nullptr,0,kZoomIns_930,1},
    {"bunker_twins_compartment_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,0,1,6707790,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,2,kFloorImgs_931,1,kCeilImgs_931,1,5.5,2.75,0,12,"",nullptr,0,kZoomIns_931,1},
    {"cache_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_932,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cache_01x",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_933,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cache_01sv",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_934,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cache_01cb",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_935,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cache_01w",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_936,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cache_01bh",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_937,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cache_01f",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_938,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cache_02",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_939,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cache_02x",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_940,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cache_02sv",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_941,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cache_02w",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_942,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cache_02sp",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_943,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cache_02su",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_944,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cache_02cb",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_945,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cache_02d",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_946,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cache_02f",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_947,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cache_02h",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_948,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cache_02bh",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_949,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cache_03",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_950,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cache_03tr",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_951,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cache_04",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_952,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cache_04x",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_953,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cache_04cb",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_954,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cache_06",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_955,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cache_06bh",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_956,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cache_07",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_957,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cache_07w",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_958,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cache_06cb",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_959,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cache_07f",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_960,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cache_07bh",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_961,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cache_log_13",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_962,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cache_pumpkin_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_963,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cache_pumpkin_02",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_964,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cache_pumpkin_03",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_965,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"cache_pumpkin_airdrop_02",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_966,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"candle_lit_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_967,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"candle_lit_02",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,kFloorImgs_968,1,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bridge_lg_structure_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,kLayers_969,2,kStairs_969,2,kMask_969,1,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bridge_xlg_structure_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,kLayers_970,2,kStairs_970,2,kMask_970,1,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bridge_md_structure_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-23,-7,23,7},1,kLayers_971,2,kStairs_971,2,kMask_971,1,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"statue_structure_03",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-7.5,-7.5,7.5,17.5},1,kLayers_972,2,kStairs_972,1,kMask_972,1,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"statue_structure_04",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-7.5,-7.5,7.5,17.5},1,kLayers_973,2,kStairs_973,1,kMask_973,1,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"barn_basement_structure_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-30,-30,30,30},1,kLayers_974,2,kStairs_974,1,kMask_974,2,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"barn_basement_structure_01d",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-30,-30,30,30},1,kLayers_975,2,kStairs_975,1,kMask_975,2,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"barn_basement_structure_01x",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-30,-30,30,30},1,kLayers_976,2,kStairs_976,1,kMask_976,2,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"mansion_structure_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,kLayers_977,2,kStairs_977,2,kMask_977,2,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"mansion_structure_01x",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,kLayers_978,2,kStairs_978,2,kMask_978,2,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"mansion_structure_02",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,kLayers_979,2,kStairs_979,2,kMask_979,2,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"mansion_structure_03",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,kLayers_980,2,kStairs_980,2,kMask_980,2,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"reserve_structure_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-68,-37,68,39},1,kLayers_981,2,kStairs_981,2,kMask_981,3,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"saloon_structure_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,kLayers_982,2,kStairs_982,1,kMask_982,1,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"club_structure_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,kLayers_983,2,kStairs_983,2,kMask_983,2,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bunker_structure_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-7.5,-7.5,7.5,17.5},1,kLayers_984,2,kStairs_984,1,kMask_984,1,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bunker_structure_01b",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-7.5,-7.5,7.5,17.5},1,kLayers_985,2,kStairs_985,1,kMask_985,1,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bunker_structure_01sv",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-7.5,-7.5,7.5,17.5},1,kLayers_986,2,kStairs_986,1,kMask_986,1,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bunker_structure_02",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-23.5,-97,46.5,15},1,kLayers_987,2,kStairs_987,3,kMask_987,5,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bunker_structure_03",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-7,-10.5,7,22.5},1,kLayers_988,2,kStairs_988,1,kMask_988,1,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bunker_structure_04",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,11.5,-40.5,58,11.5},1,kLayers_989,2,kStairs_989,2,kMask_989,2,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bunker_structure_05",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-42,-28.5,42,34.5},1,kLayers_990,2,kStairs_990,4,kMask_990,3,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bunker_structure_06",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-6,-7.5,8,19.5},1,kLayers_991,2,kStairs_991,1,kMask_991,2,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bunker_structure_07",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-42,-72,42,25},1,kLayers_992,2,kStairs_992,1,kMask_992,1,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bunker_structure_08",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-10,-10,20,20},1,kLayers_993,2,kStairs_993,1,kMask_993,2,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bunker_structure_08b",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-10,-10,20,20},1,kLayers_994,2,kStairs_994,1,kMask_994,2,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bunker_structure_09",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-26.5,-21.4,26.5,21.4},1,kLayers_995,2,kStairs_995,4,kMask_995,1,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"bunker_structure_10",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-46.5,-99,20,22},1,kLayers_996,2,kStairs_996,2,kMask_996,2,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_barrel_explosion",{"map-barrel-res-01.img",0.24,1,0,9,0,0,0},1,{0,0,0,1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{0,0,0,1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_frag_explosion",{"map-barrel-res-01.img",0.2,0.8,0,11,0,0,0},1,{0,0,0,1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{0,0,0,1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_frag_small_explosion",{"map-barrel-res-01.img",0.12,0.8,2105376,11,0,0,0},1,{0,0,0,1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{0,0,0,1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_rounds_explosion",{"map-barrel-res-01.img",0.1,0.8,3150346,11,0,0,0},1,{0,0,0,1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{0,0,0,1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_bomb_iron_explosion",{"map-barrel-res-01.img",0.2,0.8,0,11,0,0,0},1,{0,0,0,1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{0,0,0,1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_smoke_explosion",{"map-smoke-res.img",0.2,0.5,16777215,11,0,0,0},1,{0,0,0,1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{0,0,0,1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_snowball_explosion",{"map-snowball-res.img",0.2,0.25,16777215,11,0,0,0},1,{0,0,0,1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{0,0,0,1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_potato_explosion",{"map-potato-res.img",0.2,0.25,16777215,11,0,0,0},1,{0,0,0,1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{0,0,0,1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_vent_01",{"map-bunker-vent-01.img",0.5,1,16777215,0,0,0,0},1,{0,0,0,2,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{0,0,0,2,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_vent_02",{"map-bunker-vent-02.img",0.5,1,16777215,0,0,0,0},1,{0,0,0,2,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{0,0,0,2,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_vent_03",{"map-bunker-vent-03.img",0.5,1,16777215,0,0,0,0},1,{0,0,0,2,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{0,0,0,2,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_hydra_01",{"map-bunker-hydra-floor-04.img",0.5,1,16777215,0,0,0,0},1,{0,0,0,3,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{0,0,0,3,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_camera_01",{"map-decal-camera-01.img",0.25,1,16777215,60,0,0,0},1,{1,-0.5,-0.5,0.5,0.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-0.5,-0.5,0.5,0.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_pipe_01",{"map-decal-pipe.img",0.5,1,16777215,0,0,0,0},1,{0,0,0,2,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{0,0,0,2,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_pipes_01",{"map-pipes-01.img",0.5,0.96,16777215,60,0,0,0},1,{1,-1,-4.5,1,4.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-1,-4.5,1,4.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_pipes_02",{"map-pipes-02.img",0.5,0.96,16777215,60,0,0,0},1,{1,-4,-3,4,3},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-4,-3,4,3},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_pipes_03",{"map-pipes-03.img",0.5,0.96,16777215,60,0,0,0},1,{1,-10.5,-4,10.5,4},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-10.5,-4,10.5,4},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_pipes_04",{"map-pipes-04.img",0.5,0.96,16777215,60,0,0,0},1,{1,-1,-5.5,1,5.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-1,-5.5,1,5.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_pipes_05",{"map-pipes-05.img",0.5,0.96,16777215,60,0,0,0},1,{1,-1,-3.5,1,3.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-1,-3.5,1,3.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_initiative_01",{"map-decal-initiative.img",0.5,1,16777215,0,0,0,0},1,{0,0,0,3,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{0,0,0,3,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_caduceus_01",{"map-decal-caduceus.img",0.5,1,16777215,0,0,0,0},1,{0,0,0,3,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{0,0,0,3,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_web_01",{"map-web-01.img",0.5,0.75,16777215,60,0,0,0},1,{1,-1.5,-1.5,1.5,1.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-1.5,-1.5,1.5,1.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_light_01",{"map-light-01.img",1,0.5,16751616,60,0,0,0},1,{1,-3.25,-3.25,3.25,3.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-3.25,-3.25,3.25,3.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_light_02",{"map-light-01.img",0.75,0.5,16760397,60,0,0,0},1,{1,-2.5,-2.5,2.5,2.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-2.5,-2.5,2.5,2.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_light_03",{"map-light-01.img",0.75,0.5,8585216,60,0,0,0},1,{1,-2.5,-2.5,2.5,2.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-2.5,-2.5,2.5,2.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_light_04",{"map-light-01.img",0.75,0.5,16734244,60,0,0,0},1,{1,-2.5,-2.5,2.5,2.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-2.5,-2.5,2.5,2.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_blood_01",{"part-splat-01.img",0.25,0.95,4001294,0,0,0,0},1,{1,-1.5,-1.5,1.5,1.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-1.5,-1.5,1.5,1.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_blood_02",{"part-splat-02.img",0.25,0.95,4001294,0,0,0,0},1,{1,-1.5,-1.5,1.5,1.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-1.5,-1.5,1.5,1.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_blood_03",{"part-splat-03.img",0.25,0.95,4001294,0,0,0,0},1,{1,-1.5,-1.5,1.5,1.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-1.5,-1.5,1.5,1.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_chrys_01",{"map-bunker-vent-01.img",0.5,1,16777215,3,0,0,0},1,{0,0,0,2,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{0,0,0,2,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_oil_01",{"map-decal-oil-01.img",0.5,1,16777215,0,0,0,0},1,{0,0,0,2,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{0,0,0,2,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_oil_02",{"map-decal-oil-02.img",0.5,1,16777215,0,0,0,0},1,{0,0,0,2,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{0,0,0,2,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_oil_03",{"map-decal-oil-03.img",0.5,1,16777215,0,0,0,0},1,{0,0,0,2,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{0,0,0,2,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_oil_04",{"map-decal-oil-04.img",0.5,1,16777215,0,0,0,0},1,{0,0,0,1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{0,0,0,1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_oil_05",{"map-decal-oil-05.img",0.5,1,16777215,0,0,0,0},1,{0,0,0,1,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{0,0,0,1,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_oil_06",{"map-decal-oil-06.img",0.5,1,16777215,0,0,0,0},1,{0,0,0,2,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{0,0,0,2,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_bathhouse_pool_01",{"map-bathhouse-pool-01.img",8,0.5,52721,5,0,0,0},1,{1,-9,-15,9,15},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-9,-15,9,15},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_club_01",{"map-decal-club-01.img",1,1,16777215,4,0,0,0},1,{0,0,0,4,0},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{0,0,0,4,0},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_club_02",{"map-decal-club-02.img",1,0,16777215,4,0,0,0},1,{1,-4,-10.5,4,10.5},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-4,-10.5,4,10.5},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_plank_01",{"part-plank-01.img",0.5,1,4327436,9,0,0,0},1,{1,-2.25,-2.25,2.25,2.25},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-2.25,-2.25,2.25,2.25},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"decal_flyer_01",{"map-decal-flyer-01.img",0.6,0.667,16777215,4,0,0,0},1,{1,-0.5,-1,0.5,1},1,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,-0.5,-1,0.5,1},1,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"loot_tier_1",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"loot_tier_2",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"loot_tier_beach",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"loot_tier_surviv",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"loot_tier_vault_floor",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"loot_tier_police_floor",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"loot_tier_mansion_floor",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"loot_tier_sv98",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"loot_tier_scopes_sniper",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"loot_tier_woodaxe",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"loot_tier_fireaxe",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"loot_tier_stonehammer",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"loot_tier_barn_melee",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"loot_tier_hatchet_melee",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"loot_tier_club_melee",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"loot_tier_leaf_pile",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"loot_tier_islander_outfit",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"loot_tier_verde_outfit",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"loot_tier_lumber_outfit",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"loot_tier_imperial_outfit",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"loot_tier_pineapple_outfit",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"loot_tier_tarkhany_outfit",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"loot_tier_spetsnaz_outfit",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"loot_tier_eye_02",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"loot_tier_saloon",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"loot_tier_chrys_01",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"loot_tier_chrys_02",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"loot_tier_chrys_03",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"loot_tier_airdrop_armor",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"loot_tier_perk_test",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"loot_tier_sniper_test",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"loot_tier_loot_test",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
    {"loot_tier_helmet_forest",{"",1,1,16777215,0,0,0,0},0,{1,0,0,0,0},0,0,0,0,0,0,0,"",0,"",0,0,1,16777215,1,0.5,0.5,1,0,0,1,{1,0,0,0,0},0,nullptr,0,nullptr,0,nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,5.5,2.75,0,12,"",nullptr,0,nullptr,0},
};
static const char* const kMapAtlases_0[] = {"loadout","shared","main"};
static const char* const kMapAtlases_199[] = {"loadout","shared","main"};
static const char* const kMapAtlases_426[] = {"loadout","shared","main"};
static const char* const kMapAtlases_636[] = {"loadout","shared","desert"};
static const char* const kMapAtlases_841[] = {"loadout","shared","faction"};
static const char* const kMapAtlases_1041[] = {"loadout","shared","faction","potato"};
static const char* const kMapAtlases_1260[] = {"loadout","shared","halloween"};
static const char* const kMapAtlases_1488[] = {"loadout","shared","main","potato"};
static const char* const kMapAtlases_1706[] = {"loadout","shared","main","potato"};
static const char* const kMapAtlases_1936[] = {"loadout","shared","snow"};
static const char* const kMapAtlases_2153[] = {"loadout","shared","woods"};
static const char* const kMapAtlases_2368[] = {"loadout","shared","woods"};
static const char* const kMapAtlases_2591[] = {"loadout","shared","woods"};
static const char* const kMapAtlases_2820[] = {"loadout","shared","woods"};
static const char* const kMapAtlases_3051[] = {"loadout","shared","savannah"};
static const char* const kMapAtlases_3258[] = {"loadout","shared","cobalt"};
static const char* const kMapAtlases_3456[] = {"loadout","shared","turkey"};
static const char* const kMapAtlases_3661[] = {"loadout","shared","main"};
static const char* const kMapAtlases_3866[] = {"loadout","shared","main","beach"};
static const char* const kMapAtlases_4071[] = {"loadout","shared","main"};
static const char* const kMapAtlases_4280[] = {"loadout","shared","main"};
static const RawMapRender kMapRenders[] = {
    {"main",2118510,13480795,8433481,9461284,3310251,11792639,9461284,3310251,11792639,1772803,0,0,0,0,1,"",kMapAtlases_0,3,"menu_music_01","ambient_wind_01","ambient_stream_01","ambient_waves_01"},
    {"main_spring",2118510,16035400,6066442,9079434,3310251,11792639,9079434,3310251,11792639,1772803,0,0,0,0,1,"falling_leaf_spring",kMapAtlases_199,3,"menu_music_01","ambient_wind_01","ambient_stream_01","ambient_waves_01"},
    {"main_summer",2118510,14458408,6460706,10711321,3310251,11792639,10711321,3310251,11792639,1772803,0,0,0,0,1,"",kMapAtlases_426,3,"menu_music_01","ambient_wind_01","ambient_stream_01","ambient_waves_01"},
    {"desert",6976835,13206586,14657367,11689508,9083726,13756037,9530919,4370618,11792639,4001027,0,0,0,0,1,"",kMapAtlases_636,3,"menu_music_01","ambient_wind_01","ambient_stream_01","ambient_waves_01"},
    {"faction",333348,9328178,5136680,6632211,465718,11792639,6632211,465718,11792639,1772803,1,0,0,0,1,"",kMapAtlases_841,3,"menu_music_01","ambient_wind_01","ambient_stream_01","ambient_waves_01"},
    {"faction_potato",333348,9328178,5136680,6632211,465718,11792639,6632211,465718,11792639,1772803,1,1,0,0,1,"falling_pvt",kMapAtlases_1041,4,"menu_music_01","ambient_wind_01","ambient_stream_01","ambient_waves_01"},
    {"halloween",1507328,6570254,2171908,3939077,2621440,1048833,3939077,2621440,1048833,1181697,0,0,0,0,0.3,"falling_leaf_halloween",kMapAtlases_1260,3,"menu_music_02","ambient_wind_01","ambient_stream_01","ambient_waves_01"},
    {"potato",2118510,13480795,8433481,9461284,3310251,11792639,9461284,3310251,11792639,1772803,0,1,0,0,1,"falling_potato",kMapAtlases_1488,4,"menu_music_01","ambient_wind_01","ambient_stream_01","ambient_waves_01"},
    {"potato_spring",2118510,16035400,6066442,9079434,3310251,11792639,9079434,3310251,11792639,1772803,0,1,0,0,1,"falling_leaf_potato",kMapAtlases_1706,4,"menu_music_01","ambient_wind_01","ambient_stream_01","ambient_waves_01"},
    {"snow",603705,13480795,12434877,9461284,806225,11792639,9461284,806225,11792639,1772803,0,0,0,0,1,"falling_snow_fast",kMapAtlases_1936,3,"menu_music_01","ambient_wind_01","ambient_stream_01","ambient_waves_01"},
    {"woods",2118510,15709019,9339690,7812619,3310251,11792639,7812619,3310251,11792639,1772803,0,0,0,0,1,"falling_leaf",kMapAtlases_2153,3,"menu_music_01","ambient_wind_01","ambient_stream_01","ambient_waves_01"},
    {"woods_snow",603705,13480795,12434877,9461284,806225,11792639,9461284,806225,11792639,1772803,0,0,0,0,1,"falling_snow_slow",kMapAtlases_2368,3,"menu_music_01","ambient_wind_01","ambient_stream_01","ambient_waves_01"},
    {"woods_spring",2118510,15709019,4351497,9079434,3310251,11792639,9079434,3310251,11792639,1772803,0,0,0,0,1,"falling_leaf_spring",kMapAtlases_2591,3,"menu_music_01","ambient_wind_01","ambient_stream_01","ambient_waves_01"},
    {"woods_summer",2118510,14458408,6460706,10711321,3310251,11792639,10711321,3310251,11792639,1772803,0,0,0,0,1,"falling_leaf_summer",kMapAtlases_2820,3,"menu_music_01","ambient_wind_01","ambient_stream_01","ambient_waves_01"},
    {"savannah",1858399,13332786,11841582,11689508,4301994,9892086,11689508,4301994,9892086,4001027,0,0,0,0,1,"",kMapAtlases_3051,3,"menu_music_01","ambient_wind_01","ambient_stream_01","ambient_waves_01"},
    {"cobalt",134680,6834230,5069416,4472122,13681,11792639,4472122,13681,11792639,1772803,0,0,1,0,1,"",kMapAtlases_3258,3,"menu_music_01","ambient_wind_01","ambient_stream_01","ambient_waves_01"},
    {"turkey",4672842,13408091,10521391,12409889,7371638,8292740,12409889,7371638,8292740,1772803,0,0,0,1,1,"",kMapAtlases_3456,3,"menu_music_01","ambient_wind_01","ambient_stream_01","ambient_waves_01"},
    {"birthday",2118510,7444036,8433481,9461284,8433481,11792639,9461284,8433481,11792639,1772803,0,0,0,0,1,"",kMapAtlases_3661,3,"menu_music_01","ambient_wind_01","ambient_stream_01","ambient_waves_01"},
    {"beach",2118510,16771002,8104037,10711321,4370618,11792639,10711321,4370618,11792639,1772803,0,0,0,0,1,"",kMapAtlases_3866,4,"menu_music_01","ambient_wind_01","ambient_stream_01","ambient_waves_01"},
    {"test_normal",2118510,13480795,8433481,9461284,3310251,11792639,9461284,3310251,11792639,1772803,0,0,0,0,1,"",kMapAtlases_4071,3,"menu_music_01","ambient_wind_01","ambient_stream_01","ambient_waves_01"},
    {"test_faction",2118510,13480795,8433481,9461284,3310251,11792639,9461284,3310251,11792639,1772803,1,0,0,0,1,"",kMapAtlases_4280,3,"menu_music_01","ambient_wind_01","ambient_stream_01","ambient_waves_01"},
};
static const RawGameObj kGameObjs[] = {
    {"bullet_mp5","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_ak47","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_scar","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_an94","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_groza","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_grozas","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_model94","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_blr","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_mosin","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_sv98","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_awc","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_scarssr","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_m39","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_svd","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_garand","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_buckshot","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_flechette","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_frag","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_slug","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_birdshot","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_m9","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_m9_cursed","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_m93r","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_p30l","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_ot38","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_ots38","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_colt45","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_m1911","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_m1a1","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_mkg45","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_deagle","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_barrett","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_sw500","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_ash12","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_mac10","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_ump9","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_vector","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_vector45","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_scorpion","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_vss","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_dp28","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_bar","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_imbel","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_pkp","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_glock","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_famas","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_hk416","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_m4a1","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_mk12","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_l86","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_m249","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_qbb97","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_scout","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_flare","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bullet_invis","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"shrapnel_barrel","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"shrapnel_stove","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"shrapnel_frag","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"shrapnel_strobe","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"shrapnel_usas","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"shrapnel_mirv_mini","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"shrapnel_bomb_iron","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"shrapnel_cobalt","bullet",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"crosshair_default","crosshair",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"crosshair_001","crosshair",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"crosshair_005","crosshair",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"crosshair_007","crosshair",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"crosshair_010","crosshair",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"crosshair_022","crosshair",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"crosshair_027","crosshair",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"crosshair_038","crosshair",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"crosshair_040","crosshair",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"crosshair_045","crosshair",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"crosshair_051","crosshair",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"crosshair_064","crosshair",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"crosshair_080","crosshair",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"crosshair_086","crosshair",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"crosshair_094","crosshair",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"crosshair_098","crosshair",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"crosshair_101","crosshair",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"crosshair_102","crosshair",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"crosshair_109","crosshair",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"crosshair_118","crosshair",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"crosshair_124","crosshair",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"crosshair_125","crosshair",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"crosshair_136","crosshair",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"crosshair_158","crosshair",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"crosshair_160","crosshair",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"crosshair_173","crosshair",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"crosshair_176","crosshair",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"crosshair_177","crosshair",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"crosshair_181","crosshair",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"crosshair_184","crosshair",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"heal_basic","heal_effect",{"",1,1,16777215,0,0,0,0},0,"heal_basic","",16711935,0},
    {"heal_heart","heal_effect",{"",1,1,16777215,0,0,0,0},0,"heal_heart","",16711935,0},
    {"heal_moon","heal_effect",{"",1,1,16777215,0,0,0,0},0,"heal_moon","",16711935,0},
    {"heal_tomoe","heal_effect",{"",1,1,16777215,0,0,0,0},0,"heal_tomoe","",16711935,0},
    {"heal_diamond","heal_effect",{"",1,1,16777215,0,0,0,0},0,"heal_diamond","",16711935,0},
    {"heal_ankh","heal_effect",{"",1,1,16777215,0,0,0,0},0,"heal_ankh","",16711935,0},
    {"heal_menacing","heal_effect",{"",1,1,16777215,0,0,0,0},0,"heal_menacing","",16711935,0},
    {"boost_basic","boost_effect",{"",1,1,16777215,0,0,0,0},0,"boost_basic","",16711935,0},
    {"boost_star","boost_effect",{"",1,1,16777215,0,0,0,0},0,"boost_star","",16711935,0},
    {"boost_naturalize","boost_effect",{"",1,1,16777215,0,0,0,0},0,"boost_naturalize","",16711935,0},
    {"boost_shuriken","boost_effect",{"",1,1,16777215,0,0,0,0},0,"boost_shuriken","",16711935,0},
    {"boost_club","boost_effect",{"",1,1,16777215,0,0,0,0},0,"boost_club","",16711935,0},
    {"boost_hermes","boost_effect",{"",1,1,16777215,0,0,0,0},0,"boost_hermes","",16711935,0},
    {"boost_lightning","boost_effect",{"",1,1,16777215,0,0,0,0},0,"boost_lightning","",16711935,0},
    {"boost_gearshift","boost_effect",{"",1,1,16777215,0,0,0,0},0,"boost_gearshift_01,boost_gearshift_02","",16711935,0},
    {"emote_medical","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_ammo","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_ammo9mm","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_ammo12gauge","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_ammo762mm","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_ammo556mm","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_ammo50ae","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_ammo308sub","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_ammoflare","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_ammo45acp","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_loot","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_trick_nothing","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_trick_size","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_trick_m9","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_trick_chatty","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_trick_drain","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_treat_9mm","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_treat_12g","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_treat_556","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_treat_762","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_treat_super","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_bugle_inspiration_red","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_bugle_final_red","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_bugle_inspiration_blue","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_bugle_final_blue","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_thumbsup","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_sadface","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_happyface","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_boffy","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_sadboffy","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_surviv","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_gg","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_question","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_tombstone","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_joyface","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_sobface","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_thinkingface","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagus","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagthailand","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flaggermany","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagfrance","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagsouthkorea","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagbrazil","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagcanada","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagspain","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagrussia","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagmexico","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagpoland","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flaguk","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagcolombia","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagukraine","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagturkey","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagphilippines","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagczechia","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagperu","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagaustria","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagargentina","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagjapan","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagvenezuela","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagvietnam","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagswitzerland","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagnetherlands","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagchina","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagtaiwan","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagchile","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagaustralia","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagdenmark","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagitaly","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagsweden","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagecuador","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagslovakia","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flaghungary","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagromania","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flaghongkong","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagindonesia","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagfinland","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagnorway","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagbosnia","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flaglibya","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_heart","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_sleepy","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flex","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_angryface","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_upsidedownface","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_teabag","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_alienface","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagbelarus","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagbelgium","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagkazakhstan","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_egg","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_police","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_dabface","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagmalaysia","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagnewzealand","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_logosurviv","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_logoegg","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_logoswine","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_logohydra","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_logostorm","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flaghonduras","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_logocaduceus","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_impface","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_monocleface","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_sunglassface","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_headshotface","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_potato","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_tomato","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_cake","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_leek","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_eggplant","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_baguette","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_chick","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagbolivia","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagcroatia","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagindia","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flaggeorgia","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flaggreece","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagguatemala","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagportugal","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagserbia","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagsingapore","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagtrinidad","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flaguruguay","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_logoconch","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_pineapple","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_coconut","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_crab","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_whale","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_logometeor","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_salt","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_disappointface","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_logocrossing","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_fish","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_campfire","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_chickendinner","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_cattle","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_icecream","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_cupcake","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_donut","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_logohatchet","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_acorn","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_leaf","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_trunk","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_forest","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_pumpkin","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_candycorn","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_pilgrimhat","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_turkeyanimal","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_heartface","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_logochrysanthemum","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_santahat","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_snowman","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_snowflake","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagmorocco","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagestonia","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagalgeria","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagegypt","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagazerbaijan","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagalbania","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flaglithuania","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flaglatvia","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flaguae","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagdominicanrepublic","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagpalestine","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagiran","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flaglebanon","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagyemen","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagtransgender","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagpride","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flaglesbian","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flaggay","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagasexual","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagnonbinary","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flagbisexual","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_logocloud","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_ghost_base","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_bandagedface","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_picassoface","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_pooface","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_ok","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_rainbow","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_logotwins","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_antisocial","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_timeout","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_traumatizedface","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_bruh","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_flatteredface","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_salutingface","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"emote_screamingface","emote",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"explosion_frag","explosion",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"explosion_smoke","explosion",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"explosion_strobe","explosion",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"explosion_barrel","explosion",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"explosion_stove","explosion",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"explosion_usas","explosion",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"explosion_rounds","explosion",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"explosion_rounds_sg","explosion",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"explosion_mirv","explosion",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"explosion_mirv_mini","explosion",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"explosion_martyr_nade","explosion",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"explosion_snowball","explosion",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"explosion_snowball_heavy","explosion",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"explosion_potato","explosion",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"explosion_potato_heavy","explosion",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"explosion_potato_cannonball","explosion",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"explosion_potato_smgshot","explosion",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"explosion_potato_lmgshot","explosion",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"explosion_bomb_iron","explosion",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"explosion_coconut","explosion",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"explosion_tomato","explosion",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"explosion_cobalt","explosion",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"9mm","ammo",{"loot-ammo-box.img",0.2,1,16756224,0,0,0,0},1,"","",16711935,0},
    {"762mm","ammo",{"loot-ammo-box.img",0.2,1,26367,0,0,0,0},1,"","",16711935,0},
    {"556mm","ammo",{"loot-ammo-box.img",0.2,1,237056,0,0,0,0},1,"","",16711935,0},
    {"12gauge","ammo",{"loot-ammo-box.img",0.2,1,16711680,0,0,0,0},1,"","",16711935,0},
    {"50AE","ammo",{"loot-ammo-box.img",0.2,1,2697513,0,0,0,0},1,"","",16711935,0},
    {"308sub","ammo",{"loot-ammo-box.img",0.2,1,3225600,0,0,0,0},1,"","",16711935,0},
    {"flare","ammo",{"loot-ammo-box.img",0.2,1,13911552,0,0,0,0},1,"","",16711935,0},
    {"45acp","ammo",{"loot-ammo-box.img",0.2,1,7930111,0,0,0,0},1,"","",16711935,0},
    {"potato_ammo","ammo",{"loot-ammo-box.img",0.2,1,7618334,0,0,0,0},1,"","",16711935,0},
    {"bandage","heal",{"loot-medical-bandage.img",0.2,1,16777215,0,0,0,0},1,"heal","part-aura-circle-01.img",16711680,1},
    {"healthkit","heal",{"loot-medical-healthkit.img",0.2,1,16777215,0,0,0,0},1,"heal","part-aura-circle-01.img",16711680,1},
    {"soda","boost",{"loot-medical-soda.img",0.2,1,16777215,0,0,0,0},1,"boost","part-aura-circle-01.img",1676544,1},
    {"painkiller","boost",{"loot-medical-pill.img",0.2,1,16777215,0,0,0,0},1,"boost","part-aura-circle-01.img",1676544,1},
    {"backpack00","backpack",{"loot-pack-00.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"backpack01","backpack",{"loot-pack-01.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"backpack02","backpack",{"loot-pack-02.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"backpack03","backpack",{"loot-pack-03.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"backpack04","backpack",{"loot-pack-04.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"helmet01","helmet",{"loot-helmet-01.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"helmet02","helmet",{"loot-helmet-02.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"helmet03","helmet",{"loot-helmet-03.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"helmet04","helmet",{"loot-helmet-03.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"chest01","chest",{"loot-chest-01.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"chest02","chest",{"loot-chest-02.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"chest03","chest",{"loot-chest-03.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"chest04","chest",{"loot-chest-04.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"1xscope","scope",{"loot-scope-00.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"2xscope","scope",{"loot-scope-01.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"4xscope","scope",{"loot-scope-02.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"8xscope","scope",{"loot-scope-03.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"15xscope","scope",{"loot-scope-04.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"helmet03_leader","helmet",{"loot-helmet-03.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"helmet03_forest","helmet",{"player-helmet-forest.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"helmet03_moon","helmet",{"loot-helmet-03.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"helmet03_lt","helmet",{"loot-helmet-03.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"helmet03_lt_aged","helmet",{"player-helmet-lieutenant.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"helmet03_potato","helmet",{"player-helmet-potato.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"helmet03_marksman","helmet",{"player-helmet-marksman.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"helmet03_recon","helmet",{"player-helmet-recon.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"helmet03_grenadier","helmet",{"player-helmet-grenadier.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"helmet03_bugler","helmet",{"player-helmet-bugler.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"helmet04_medic","helmet",{"player-helmet-medic.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"helmet04_last_man_red","helmet",{"player-helmet-last-man-01.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"helmet04_last_man_blue","helmet",{"player-helmet-last-man-02.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"helmet04_leader","helmet",{"player-helmet-leader.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"helmet04_captain","helmet",{"player-helmet-captain.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"helmet04_classless","helmet",{"player-helmet-classless.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"backpack04_cloud","backpack",{"loot-pack-04-cloud.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"mp5","gun",{"loot-weapon-mp5.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"mac10","gun",{"loot-weapon-mac10.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"ump9","gun",{"loot-weapon-ump9.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"vector","gun",{"loot-weapon-vector.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"vector45","gun",{"loot-weapon-vector45.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"scorpion","gun",{"loot-weapon-scorpion.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"vss","gun",{"loot-weapon-vss.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"famas","gun",{"loot-weapon-famas.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"hk416","gun",{"loot-weapon-hk416.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"m4a1","gun",{"loot-weapon-m4a1.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"mk12","gun",{"loot-weapon-mk12.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"l86","gun",{"loot-weapon-l86.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"m249","gun",{"loot-weapon-m249.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"qbb97","gun",{"loot-weapon-qbb97.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"scout_elite","gun",{"loot-weapon-scout.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"ak47","gun",{"loot-weapon-ak.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"scar","gun",{"loot-weapon-scar.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"scarssr","gun",{"loot-weapon-scarssr.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"an94","gun",{"loot-weapon-an94.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"groza","gun",{"loot-weapon-groza.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"grozas","gun",{"loot-weapon-grozas.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"dp28","gun",{"loot-weapon-dp28.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"bar","gun",{"loot-weapon-bar.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"imbel","gun",{"loot-weapon-imbel.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"pkp","gun",{"loot-weapon-pkp.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"model94","gun",{"loot-weapon-model94.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"mkg45","gun",{"loot-weapon-mkg45.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"blr","gun",{"loot-weapon-blr.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"mosin","gun",{"loot-weapon-mosin.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"sv98","gun",{"loot-weapon-sv98.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"awc","gun",{"loot-weapon-awc.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"m39","gun",{"loot-weapon-m39.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"svd","gun",{"loot-weapon-svd.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"garand","gun",{"loot-weapon-garand.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"m870","gun",{"loot-weapon-m870.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"m1100","gun",{"loot-weapon-m1100.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"mp220","gun",{"loot-weapon-mp220.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"saiga","gun",{"loot-weapon-saiga.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"spas12","gun",{"loot-weapon-spas12.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"spas16","gun",{"loot-weapon-spas16.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"m1014","gun",{"loot-weapon-m1014.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"usas","gun",{"loot-weapon-usas.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"m9","gun",{"loot-weapon-m9.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"m9_dual","gun",{"loot-weapon-m9-dual.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"m9_cursed","gun",{"loot-weapon-m9-cursed.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"m93r","gun",{"loot-weapon-m93r.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"m93r_dual","gun",{"loot-weapon-m93r-dual.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"glock","gun",{"loot-weapon-glock.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"glock_dual","gun",{"loot-weapon-glock-dual.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"p30l","gun",{"loot-weapon-p30l.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"p30l_dual","gun",{"loot-weapon-p30l-dual.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"ot38","gun",{"loot-weapon-ot38.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"ot38_dual","gun",{"loot-weapon-ot38-dual.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"ots38","gun",{"loot-weapon-ots38.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"ots38_dual","gun",{"loot-weapon-ots38-dual.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"colt45","gun",{"loot-weapon-colt45.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"colt45_dual","gun",{"loot-weapon-colt45-dual.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"m1911","gun",{"loot-weapon-m1911.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"m1911_dual","gun",{"loot-weapon-m1911-dual.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"m1a1","gun",{"loot-weapon-m1a1.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"deagle","gun",{"loot-weapon-deagle.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"deagle_dual","gun",{"loot-weapon-deagle-dual.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"barrett","gun",{"loot-weapon-barrett.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"sw500","gun",{"loot-weapon-sw500.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"ash12","gun",{"loot-weapon-ash12.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"flare_gun","gun",{"loot-weapon-flare-gun.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"flare_gun_dual","gun",{"loot-weapon-flare-gun-dual.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"potato_cannon","gun",{"loot-weapon-potato-cannon.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"potato_smg","gun",{"loot-weapon-potato-smg.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"potato_lmg","gun",{"loot-weapon-potato-lmg.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"bugle","gun",{"loot-weapon-bugle.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"svd_winter","gun",{"loot-weapon-svd.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"sv98_winter","gun",{"loot-weapon-sv98.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"awc_winter","gun",{"loot-weapon-awc.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"fists","melee",{"loot-weapon-fists.img",0.3,1,65280,0,0,0,0},1,"","",16711935,0},
    {"knuckles","melee",{"loot-melee-knuckles-rusted.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"karambit","melee",{"loot-melee-karambit-rugged.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"bayonet","melee",{"loot-melee-bayonet-rugged.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"huntsman","melee",{"loot-melee-huntsman-rugged.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"bowie","melee",{"loot-melee-bowie-vintage.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"machete","melee",{"loot-melee-machete-taiga.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"saw","melee",{"loot-melee-bonesaw-rusted.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"woodaxe","melee",{"loot-melee-woodaxe.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"fireaxe","melee",{"loot-melee-fireaxe.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"katana","melee",{"loot-melee-katana.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"naginata","melee",{"loot-melee-naginata.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"stonehammer","melee",{"loot-melee-stonehammer.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"iceaxe","melee",{"loot-melee-ice_pick.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"hook","melee",{"loot-melee-hook-silver.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"pan","melee",{"loot-melee-pan-black.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"spade","melee",{"loot-melee-spade-assault.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"crowbar","melee",{"loot-melee-crowbar.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"cutlass","melee",{"loot-melee-cutlass.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"knuckles_rusted","melee",{"loot-melee-knuckles-rusted.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"knuckles_heroic","melee",{"loot-melee-knuckles-heroic.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"karambit_borealis","melee",{"loot-melee-karambit-borealis.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"karambit_rugged","melee",{"loot-melee-karambit-rugged.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"karambit_prismatic","melee",{"loot-melee-karambit-prismatic.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"karambit_drowned","melee",{"loot-melee-karambit-drowned.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"bayonet_rugged","melee",{"loot-melee-bayonet-rugged.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"bayonet_woodland","melee",{"loot-melee-bayonet-woodland.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"huntsman_rugged","melee",{"loot-melee-huntsman-rugged.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"huntsman_burnished","melee",{"loot-melee-huntsman-burnished.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"bowie_vintage","melee",{"loot-melee-bowie-vintage.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"bowie_frontier","melee",{"loot-melee-bowie-frontier.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"machete_taiga","melee",{"loot-melee-machete-taiga.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"kukri_trad","melee",{"loot-melee-kukri-trad.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"bonesaw_rusted","melee",{"loot-melee-bonesaw-rusted.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"woodaxe_bloody","melee",{"loot-melee-woodaxe-bloody.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"katana_rusted","melee",{"loot-melee-katana-rusted.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"katana_orchid","melee",{"loot-melee-katana-orchid.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"sledgehammer","melee",{"loot-melee-sledgehammer.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"crowbar_scout","melee",{"loot-melee-crowbar-scout.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"crowbar_recon","melee",{"loot-melee-crowbar-recon.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"kukri_sniper","melee",{"loot-melee-kukri-sniper.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"bonesaw_healer","melee",{"loot-melee-bonesaw-healer.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"katana_demo","melee",{"loot-melee-katana-demo.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"spade_assault","melee",{"loot-melee-spade-assault.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"warhammer_tank","melee",{"loot-melee-warhammer-tank.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"naginata_daemon","melee",{"loot-melee-naginata-daemon.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"cutlass_gold","melee",{"loot-melee-cutlass-gold.img",0.3,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"outfitBase","outfit",{"loot-shirt-01.img",0.2,1,16303476,0,0,0,0},1,"","",16711935,0},
    {"outfitDemo","outfit",{"loot-shirt-02.img",0.2,1,13068903,0,0,0,0},1,"","",16711935,0},
    {"outfitTank","outfit",{"loot-shirt-02.img",0.2,1,15382883,0,0,0,0},1,"","",16711935,0},
    {"outfitMedic","outfit",{"loot-shirt-02.img",0.2,1,14449116,0,0,0,0},1,"","",16711935,0},
    {"outfitScout","outfit",{"loot-shirt-02.img",0.2,1,11326819,0,0,0,0},1,"","",16711935,0},
    {"outfitSniper","outfit",{"loot-shirt-02.img",0.2,1,9293531,0,0,0,0},1,"","",16711935,0},
    {"outfitAssault","outfit",{"loot-shirt-02.img",0.2,1,14339929,0,0,0,0},1,"","",16711935,0},
    {"outfitClassless","outfit",{"loot-shirt-02.img",0.2,1,6579300,0,0,0,0},1,"","",16711935,0},
    {"outfitTurkey","outfit",{"loot-shirt-outfitTurkey.img",0.2,1,15781563,0,0,0,0},1,"","",16711935,0},
    {"outfitDev","outfit",{"loot-shirt-outfitDC.img",0.2,1,5295421,0,0,0,0},1,"","",16711935,0},
    {"outfitMaintainer","outfit",{"loot-shirt-outfitDC.img",0.2,1,9596644,0,0,0,0},1,"","",16711935,0},
    {"outfitGD","outfit",{"loot-shirt-outfitDC.img",0.2,1,13383993,0,0,0,0},1,"","",16711935,0},
    {"outfitMod","outfit",{"loot-shirt-outfitDC.img",0.2,1,3910655,0,0,0,0},1,"","",16711935,0},
    {"outfitWheat","outfit",{"loot-shirt-outfitWheat.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"outfitNoir","outfit",{"loot-shirt-02.img",0.2,1,1776411,0,0,0,0},1,"","",16711935,0},
    {"outfitRedLeaderAged","outfit",{"loot-shirt-02.img",0.2,1,10098712,0,0,0,0},1,"","",16711935,0},
    {"outfitBlueLeaderAged","outfit",{"loot-shirt-02.img",0.2,1,1523353,0,0,0,0},1,"","",16711935,0},
    {"outfitRedLeader","outfit",{"loot-shirt-02.img",0.2,1,10158080,0,0,0,0},1,"","",16711935,0},
    {"outfitBlueLeader","outfit",{"loot-shirt-02.img",0.2,1,12187,0,0,0,0},1,"","",16711935,0},
    {"outfitSpetsnaz","outfit",{"loot-shirt-outfitSpetsnaz.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"outfitWoodsCloak","outfit",{"loot-shirt-02.img",0.2,1,2817792,0,0,0,0},1,"","",16711935,0},
    {"outfitElf","outfit",{"loot-shirt-01.img",0.2,1,1489152,0,0,0,0},1,"","",16711935,0},
    {"outfitImperial","outfit",{"loot-shirt-01.img",0.2,1,12320813,0,0,0,0},1,"","",16711935,0},
    {"outfitLumber","outfit",{"loot-shirt-outfitLumber.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"outfitVerde","outfit",{"loot-shirt-02.img",0.2,1,1785868,0,0,0,0},1,"","",16711935,0},
    {"outfitPineapple","outfit",{"loot-shirt-02.img",0.2,1,10027008,0,0,0,0},1,"","",16711935,0},
    {"outfitTarkhany","outfit",{"loot-shirt-02.img",0.2,1,4927107,0,0,0,0},1,"","",16711935,0},
    {"outfitWaterElem","outfit",{"loot-shirt-02.img",0.2,1,7143401,0,0,0,0},1,"","",16711935,0},
    {"outfitHeaven","outfit",{"loot-shirt-outfitHeaven.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"outfitMeteor","outfit",{"loot-shirt-02.img",0.2,1,9764864,0,0,0,0},1,"","",16711935,0},
    {"outfitIslander","outfit",{"loot-shirt-01.img",0.2,1,16762368,0,0,0,0},1,"","",16711935,0},
    {"outfitAqua","outfit",{"loot-shirt-01.img",0.2,1,47778,0,0,0,0},1,"","",16711935,0},
    {"outfitCoral","outfit",{"loot-shirt-01.img",0.2,1,16736103,0,0,0,0},1,"","",16711935,0},
    {"outfitKhaki","outfit",{"loot-shirt-02.img",0.2,1,12824197,0,0,0,0},1,"","",16711935,0},
    {"outfitParma","outfit",{"loot-shirt-01.img",0.2,1,8746585,0,0,0,0},1,"","",16711935,0},
    {"outfitParmaPrestige","outfit",{"loot-shirt-outfitParmaPrestige.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"outfitCasanova","outfit",{"loot-shirt-01.img",0.2,1,4327436,0,0,0,0},1,"","",16711935,0},
    {"outfitPrisoner","outfit",{"loot-shirt-01.img",0.2,1,16735266,0,0,0,0},1,"","",16711935,0},
    {"outfitJester","outfit",{"loot-shirt-01.img",0.2,1,7798904,0,0,0,0},1,"","",16711935,0},
    {"outfitWoodland","outfit",{"loot-shirt-01.img",0.2,1,2831146,0,0,0,0},1,"","",16711935,0},
    {"outfitRoyalFortune","outfit",{"loot-shirt-01.img",0.2,1,8333091,0,0,0,0},1,"","",16711935,0},
    {"outfitKeyLime","outfit",{"loot-shirt-01.img",0.2,1,13107007,0,0,0,0},1,"","",16711935,0},
    {"outfitCobaltShell","outfit",{"loot-shirt-01.img",0.2,1,11095,0,0,0,0},1,"","",16711935,0},
    {"outfitFragtastic","outfit",{"loot-shirt-01.img",0.2,1,9668146,0,0,0,0},1,"","",16711935,0},
    {"outfitCarbonFiber","outfit",{"loot-shirt-01.img",0.2,1,2171169,0,0,0,0},1,"","",16711935,0},
    {"outfitDarkGloves","outfit",{"loot-shirt-01.img",0.2,1,12482560,0,0,0,0},1,"","",16711935,0},
    {"outfitDarkShirt","outfit",{"loot-shirt-01.img",0.2,1,9460480,0,0,0,0},1,"","",16711935,0},
    {"outfitGhillie","outfit",{"loot-shirt-01.img",0.2,1,8630096,0,0,0,0},1,"","",16711935,0},
    {"outfitDesertCamo","outfit",{"loot-shirt-01.img",0.2,1,13736782,0,0,0,0},1,"","",16711935,0},
    {"outfitCamo","outfit",{"loot-shirt-01.img",0.2,1,10066278,0,0,0,0},1,"","",16711935,0},
    {"outfitRed","outfit",{"loot-shirt-01.img",0.2,1,16711680,0,0,0,0},1,"","",16711935,0},
    {"outfitWhite","outfit",{"loot-shirt-01.img",0.2,1,14935011,0,0,0,0},1,"","",16711935,0},
    {"outfitSnow","outfit",{"loot-shirt-outfitSnow.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"outfitBlackIce","outfit",{"loot-shirt-02.img",0.2,1,6186099,0,0,0,0},1,"","",16711935,0},
    {"outfitBeachCamo","outfit",{"loot-shirt-01.img",0.2,1,15583870,0,0,0,0},1,"","",16711935,0},
    {"outfitCoconut","outfit",{"loot-shirt-01.img",0.2,1,7755830,0,0,0,0},1,"","",16711935,0},
    {"outfitWave","outfit",{"loot-shirt-02.img",0.2,1,1153260,0,0,0,0},1,"","",16711935,0},
    {"outfitParrotfish","outfit",{"loot-shirt-outfitParrotfish.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"outfitEvent","outfit",{"loot-shirt-outfitEvent.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"outfitGold","outfit",{"loot-shirt-outfitGold.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"outfitRain","outfit",{"loot-shirt-01.img",0.2,1,4485828,0,0,0,0},1,"","",16711935,0},
    {"outfitCowz","outfit",{"loot-shirt-01.img",0.2,1,11250603,0,0,0,0},1,"","",16711935,0},
    {"outfitChameleon","outfit",{"loot-shirt-outfitChameleon.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"outfitPastel","outfit",{"loot-shirt-outfitPastel.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"outfitChrys","outfit",{"loot-shirt-outfitChrys.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"outfitFahrenheit","outfit",{"loot-shirt-outfitFahrenheit.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"outfitPotatoskin","outfit",{"loot-shirt-outfitPotatoskin.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"outfitAurora","outfit",{"loot-shirt-outfitAurora.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"outfitBarrel","outfit",{"loot-shirt-01.img",0.2,1,3750201,0,0,0,0},1,"","",16711935,0},
    {"outfitWoodBarrel","outfit",{"loot-shirt-01.img",0.2,1,11235106,0,0,0,0},1,"","",16711935,0},
    {"outfitStone","outfit",{"loot-shirt-01.img",0.2,1,7434609,0,0,0,0},1,"","",16711935,0},
    {"outfitSpringTree","outfit",{"loot-shirt-01.img",0.2,1,4599058,0,0,0,0},1,"","",16711935,0},
    {"outfitHalloweenTree","outfit",{"loot-shirt-01.img",0.2,1,4599058,0,0,0,0},1,"","",16711935,0},
    {"outfitTreeSpooky","outfit",{"loot-shirt-01.img",0.2,1,1775895,0,0,0,0},1,"","",16711935,0},
    {"outfitStump","outfit",{"loot-shirt-01.img",0.2,1,8602624,0,0,0,0},1,"","",16711935,0},
    {"outfitBush","outfit",{"loot-shirt-01.img",0.2,1,3889951,0,0,0,0},1,"","",16711935,0},
    {"outfitLeafPile","outfit",{"loot-shirt-01.img",0.2,1,16731392,0,0,0,0},1,"","",16711935,0},
    {"outfitCrate","outfit",{"loot-shirt-01.img",0.2,1,6697728,0,0,0,0},1,"","",16711935,0},
    {"outfitTable","outfit",{"loot-shirt-01.img",0.2,1,6697728,0,0,0,0},1,"","",16711935,0},
    {"outfitSoviet","outfit",{"loot-shirt-01.img",0.2,1,6697728,0,0,0,0},1,"","",16711935,0},
    {"outfitAirdrop","outfit",{"loot-shirt-01.img",0.2,1,6579300,0,0,0,0},1,"","",16711935,0},
    {"outfitOven","outfit",{"loot-shirt-01.img",0.2,1,14935011,0,0,0,0},1,"","",16711935,0},
    {"outfitRefrigerator","outfit",{"loot-shirt-01.img",0.2,1,7733259,0,0,0,0},1,"","",16711935,0},
    {"outfitVending","outfit",{"loot-shirt-01.img",0.2,1,10925,0,0,0,0},1,"","",16711935,0},
    {"outfitPumpkin","outfit",{"loot-shirt-01.img",0.2,1,15889667,0,0,0,0},1,"","",16711935,0},
    {"outfitWoodpile","outfit",{"loot-shirt-01.img",0.2,1,9455616,0,0,0,0},1,"","",16711935,0},
    {"outfitToilet","outfit",{"loot-shirt-01.img",0.2,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"outfitBushRiver","outfit",{"loot-shirt-01.img",0.2,1,5339946,0,0,0,0},1,"","",16711935,0},
    {"outfitCrab","outfit",{"loot-shirt-01.img",0.2,1,16592920,0,0,0,0},1,"","",16711935,0},
    {"outfitStumpAxe","outfit",{"loot-shirt-01.img",0.2,1,11100701,0,0,0,0},1,"","",16711935,0},
    {"quest_top_solo","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_top_duo","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_top_squad","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_win_any","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_kills","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_kills_hard","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_kills_harder","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_damage","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_damage_hard","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_damage_harder","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_survived","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_damage_9mm","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_damage_9mm_ltm","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_damage_762mm","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_damage_762mm_ltm","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_damage_556mm","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_damage_556mm_ltm","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_damage_12gauge","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_damage_12gauge_ltm","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_damage_45acp","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_damage_potato_ammo","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_damage_rare_ammo","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_damage_rare_ammo_ltm","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_damage_woods_king","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_damage_grenade","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_damage_grenade_ltm","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_damage_melee","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_damage_melee_ltm","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_heal","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_boost","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_airdrop","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_airdrop_ltm","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_airdrop_ltm_hard","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_airdrop_rare","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_crates","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_toilets","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_furniture","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_barrels","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_lockers","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_pots","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_vending","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_hardstone","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_soviet_crate","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_initiative_crate","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_pvt_swappers","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_potatoes","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_club_kills","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_docks_kills","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_river_town_kills","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_desert_town_kills","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_reserve_kills","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_logging_complex_kills","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_be_mvp","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_promote_hunted","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_factions_damage","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_last_man_damage_hard","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_factions_kills","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_healer_kills","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_tank_kills","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_sniper_kills","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_scout_kills","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_demo_kills","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_assault_kills","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_healer_damage","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_tank_damage","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_sniper_damage","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_scout_damage","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_demo_damage","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_assault_damage","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"quest_classless_damage","quest",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"leadership","perk",{"loot-perk-leadership.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"assume_leadership","perk",{"loot-perk-assume-leadership.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"firepower","perk",{"loot-perk-firepower.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"gotw","perk",{"loot-perk-gotw.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"windwalk","perk",{"loot-perk-windwalk.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"rare_potato","perk",{"loot-perk-rare-potato.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"aoe_heal","perk",{"loot-perk-aoe-heal.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"endless_ammo","perk",{"loot-perk-endless-ammo.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"steelskin","perk",{"loot-perk-steelskin.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"ap_rounds","perk",{"loot-perk-ap-rounds.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"splinter","perk",{"loot-perk-splinter.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"small_arms","perk",{"loot-perk-small-arms.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"takedown","perk",{"loot-perk-takedown.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"lifeline","perk",{"loot-perk-lifeline.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"field_medic","perk",{"loot-perk-field-medic.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"combat_stims","perk",{"loot-perk-combat-stims.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"tree_climbing","perk",{"loot-perk-tree-climbing.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"scavenger","perk",{"loot-perk-scavenger.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"scavenger_adv","perk",{"loot-perk-scavenger_adv.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"pirate","perk",{"loot-perk-pirate.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"hunted","perk",{"loot-perk-hunted.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"chambered","perk",{"loot-perk-chambered.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"martyrdom","perk",{"loot-perk-martyrdom.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"targeting","perk",{"loot-perk-targeting.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"bonus_45","perk",{"loot-perk-bonus-45.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"broken_arrow","perk",{"loot-perk-broken-arrow.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"fabricate","perk",{"loot-perk-fabricate.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"self_revive","perk",{"loot-perk-self-revive.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"bonus_9mm","perk",{"loot-perk-bonus-9mm.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"flak_jacket","perk",{"loot-perk-flak-jacket.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"amped_explosives","perk",{"loot-perk-amped-explosives.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"explosive","perk",{"loot-perk-explosive.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"bonus_assault","perk",{"loot-perk-bonus-assault.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"inspiration","perk",{"loot-perk-inspiration.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"final_bugle","perk",{"loot-perk-final-bugle.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"high_velocity","perk",{"loot-perk-high-velocity.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"halloween_mystery","perk",{"loot-perk-halloween-mystery.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"trick_nothing","perk",{"loot-perk-trick-nothing.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"trick_size","perk",{"loot-perk-trick-size.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"trick_m9","perk",{"loot-perk-trick-m9.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"trick_chatty","perk",{"loot-perk-trick-chatty.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"trick_drain","perk",{"loot-perk-trick-drain.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"treat_9mm","perk",{"loot-perk-treat-9mm.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"treat_12g","perk",{"loot-perk-treat-12g.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"treat_556","perk",{"loot-perk-treat-556.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"treat_762","perk",{"loot-perk-treat-762.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"treat_super","perk",{"loot-perk-treat-super.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"turkey_shoot","perk",{"loot-perk-turkey_shoot.img",0.275,1,16777215,0,0,0,0},1,"","",16711935,0},
    {"pass_survivr1","pass",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"pass_survivr2","pass",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"ping_danger","ping",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"ping_coming","ping",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"ping_help","ping",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"ping_airdrop","ping",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"ping_airstrike","ping",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"ping_woodsking","ping",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"ping_unlock","ping",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"leader","role",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"captain","role",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"lieutenant","role",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"medic","role",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"marksman","role",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"recon","role",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"grenadier","role",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"bugler","role",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"last_man","role",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"woods_king","role",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"kill_leader","role",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"the_hunted","role",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"healer","role",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"tank","role",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"sniper","role",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"scout","role",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"demo","role",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"assault","role",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"classless","role",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"frag","throwable",{"loot-throwable-frag.img",0.2,1,65280,0,0,0,0},1,"","",16711935,0},
    {"mirv","throwable",{"loot-throwable-mirv.img",0.2,1,65280,0,0,0,0},1,"","",16711935,0},
    {"mirv_mini","throwable",{"loot-throwable-frag.img",0.2,1,65280,0,0,0,0},1,"","",16711935,0},
    {"martyr_nade","throwable",{"loot-throwable-frag.img",0.2,1,65280,0,0,0,0},1,"","",16711935,0},
    {"smoke","throwable",{"loot-throwable-smoke.img",0.2,1,65280,0,0,0,0},1,"","",16711935,0},
    {"strobe","throwable",{"loot-throwable-strobe.img",0.2,1,65280,0,0,0,0},1,"","",16711935,0},
    {"snowball","throwable",{"loot-throwable-snowball.img",0.2,1,65280,0,0,0,0},1,"","",16711935,0},
    {"snowball_heavy","throwable",{"loot-throwable-snowball.img",0.2,1,65280,0,0,0,0},1,"","",16711935,0},
    {"potato","throwable",{"loot-throwable-potato.img",0.2,1,65280,0,0,0,0},1,"","",16711935,0},
    {"potato_heavy","throwable",{"loot-throwable-potato.img",0.2,1,65280,0,0,0,0},1,"","",16711935,0},
    {"potato_cannonball","throwable",{"loot-throwable-potato.img",0.2,1,65280,0,0,0,0},1,"","",16711935,0},
    {"potato_smgshot","throwable",{"loot-throwable-potato.img",0.2,1,65280,0,0,0,0},1,"","",16711935,0},
    {"potato_lmgshot","throwable",{"loot-throwable-potato.img",0.2,1,65280,0,0,0,0},1,"","",16711935,0},
    {"bomb_iron","throwable",{"loot-throwable-frag.img",0.2,1,65280,0,0,0,0},1,"","",16711935,0},
    {"coconut","throwable",{"loot-throwable-coconut.img",0.2,1,65280,0,0,0,0},1,"","",16711935,0},
    {"tomato","throwable",{"loot-throwable-tomato.img",0.2,1,65280,0,0,0,0},1,"","",16711935,0},
    {"unlock_default","unlock",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"unlock_new_account","unlock",{"",1,1,16777215,0,0,0,0},0,"","",16711935,0},
    {"xp_10","xp",{"loot-xp-book-01.img",0.2,1,16777215,0,0,0,0},1,"xp_common","",16711935,0},
    {"xp_25","xp",{"loot-xp-book-01.img",0.2,1,16777215,0,0,0,0},1,"xp_rare","",16711935,0},
    {"xp_100","xp",{"loot-xp-book-01.img",0.2,1,16777215,0,0,0,0},1,"xp_mythic","",16711935,0},
    {"xp_book_tallow","xp",{"loot-xp-book-01.img",0.2,1,16777215,0,0,0,0},1,"xp_common","",16711935,0},
    {"xp_book_greene","xp",{"loot-xp-book-02.img",0.2,1,16777215,0,0,0,0},1,"xp_common","",16711935,0},
    {"xp_book_parma","xp",{"loot-xp-book-03.img",0.2,1,16777215,0,0,0,0},1,"xp_common","",16711935,0},
    {"xp_book_nevelskoy","xp",{"loot-xp-book-04.img",0.2,1,16777215,0,0,0,0},1,"xp_common","",16711935,0},
    {"xp_book_rinzo","xp",{"loot-xp-book-05.img",0.2,1,16777215,0,0,0,0},1,"xp_common","",16711935,0},
    {"xp_book_kuga","xp",{"loot-xp-book-06.img",0.2,1,16777215,0,0,0,0},1,"xp_common","",16711935,0},
    {"xp_glasses","xp",{"loot-xp-glasses-01.img",0.2,1,16777215,0,0,0,0},1,"xp_rare","",16711935,0},
    {"xp_compass","xp",{"loot-xp-compass-01.img",0.2,1,16777215,0,0,0,0},1,"xp_rare","",16711935,0},
    {"xp_stump","xp",{"loot-xp-stump-01.img",0.2,1,16777215,0,0,0,0},1,"xp_rare","",16711935,0},
    {"xp_bone","xp",{"loot-xp-bone-01.img",0.2,1,16777215,0,0,0,0},1,"xp_rare","",16711935,0},
    {"xp_donut","xp",{"loot-xp-donut-01.img",0.2,1,16777215,0,0,0,0},1,"xp_mythic","",16711935,0},
};
static const RawParticle kParticles[] = {
    {"archwayBreak",kParticleImgs_0,1,20,{0.5,1.5,0},{1,5,0},{0,9.42477796076938,0},{0.2,0.35,0},{0.08,0.12,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,7878419,0},
    {"bloodSplat",kParticleImgs_169,3,20,{0.5,0.5,1},{1,1,1},{0,0,1},{0.04,0.04,1},{0.15,0.2,0},{0,1,0},0,0,1,0,{0.75,1,0},0,0,0,0,0,{0,1,1},1,10485760,0},
    {"barrelPlank",kParticleImgs_325,1,20,{1,1.5,0},{3,5,0},{9.42477796076938,9.42477796076938,1},{0.08,0.18,0},{0.07,0.17,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,11234338,0},
    {"barrelChip",kParticleImgs_510,1,20,{0.5,0.5,1},{1,10,0},{0,0,1},{0.04,0.08,0},{0.01,0.02,0},{0,1,0},0,0,1,0,{0.95,1,0},0,0,0,0,0,{0,1,1},1,6644579,0},
    {"barrelBreak",kParticleImgs_667,1,20,{0.8,1,0},{1,5,0},{0,0,1},{0.07,0.12,0},{0.05,0.1,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,6644579,0},
    {"blackChip",kParticleImgs_820,1,20,{0.5,1,0},{1,5,0},{9.42477796076938,9.42477796076938,1},{0.04,0.08,0},{0.01,0.02,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,2828328,0},
    {"blueChip",kParticleImgs_1002,1,20,{0.5,0.5,1},{1,10,0},{0,0,1},{0.04,0.08,0},{0.01,0.02,0},{0,1,0},0,0,1,0,{0.95,1,0},0,0,0,0,0,{0,1,1},1,8918,0},
    {"book",kParticleImgs_1155,1,20,{1,1.5,0},{3,5,0},{9.42477796076938,9.42477796076938,1},{0.09,0.19,0},{0.07,0.17,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,12227180,0},
    {"bottleBrownChip",kParticleImgs_1334,1,20,{0.5,0.5,1},{1,5,0},{3.141592653589793,18.84955592153876,0},{0.02,0.04,0},{0.01,0.02,0},{0,1,0},0,0,1,0,{0.95,1,0},0,0,0,0,0,{0,1,1},1,7878664,0},
    {"bottleBrownBreak",kParticleImgs_1528,1,20,{0.4,0.8,0},{1,4,0},{3.141592653589793,18.84955592153876,0},{0.03,0.06,0},{0.05,0.1,0},{0,1,0},0,0,0.8,0,{0.75,1,0},0,0,0,0,0,{0,1,1},1,7878664,0},
    {"bottleBlueChip",kParticleImgs_1724,1,20,{0.5,0.5,1},{1,5,0},{3.141592653589793,18.84955592153876,0},{0.02,0.04,0},{0.01,0.02,0},{0,1,0},0,0,1,0,{0.95,1,0},0,0,0,0,0,{0,1,1},1,19544,0},
    {"bottleWhiteBreak",kParticleImgs_1915,1,20,{0.4,0.8,0},{1,4,0},{3.141592653589793,18.84955592153876,0},{0.03,0.06,0},{0.05,0.1,0},{0,1,0},0,0,0.75,0,{0.75,1,0},0,0,0,0,0,{0,1,1},1,16777215,0},
    {"bottleWhiteChip",kParticleImgs_2113,1,20,{0.5,0.5,1},{1,5,0},{3.141592653589793,18.84955592153876,0},{0.02,0.04,0},{0.01,0.02,0},{0,1,0},0,0,0.75,0,{0.95,1,0},0,0,0,0,0,{0,1,1},1,16777215,0},
    {"bottleBlueBreak",kParticleImgs_2311,1,20,{0.4,0.8,0},{1,4,0},{3.141592653589793,18.84955592153876,0},{0.03,0.06,0},{0.05,0.1,0},{0,1,0},0,0,0.8,0,{0.75,1,0},0,0,0,0,0,{0,1,1},1,19544,0},
    {"brickChip",kParticleImgs_2504,1,20,{0.5,0.5,1},{1,10,0},{0,0,1},{0.04,0.08,0},{0.01,0.02,0},{0,1,0},0,0,1,0,{0.95,1,0},0,0,0,0,0,{0,1,1},1,5511192,0},
    {"clothBreak",kParticleImgs_2661,1,20,{0.8,1,0},{1,5,0},{0,0,1},{0.07,0.12,0},{0.05,0.1,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,16382457,0},
    {"clothHit",kParticleImgs_2815,1,20,{0.5,0.5,1},{1,10,0},{0,0,1},{0.04,0.08,0},{0.01,0.02,0},{0,1,0},0,0,1,0,{0.95,1,0},0,0,0,0,0,{0,1,1},1,16382457,0},
    {"depositBoxGreyBreak",kParticleImgs_2972,1,20,{0.5,1,0},{7,8,0},{0,9.42477796076938,0},{0.15,0.25,0},{0.12,0.2,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,6184542,0},
    {"depositBoxGoldBreak",kParticleImgs_3149,1,20,{0.5,1,0},{6,8,0},{0,9.42477796076938,0},{0.2,0.35,0},{0.18,0.25,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,10909210,0},
    {"depositBoxSilverBreak",kParticleImgs_3327,1,20,{0.5,1,0},{6,8,0},{0,9.42477796076938,0},{0.2,0.35,0},{0.18,0.25,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,11776947,0},
    {"glassChip",kParticleImgs_3507,1,20,{0.5,0.5,1},{1,5,0},{3.141592653589793,18.84955592153876,0},{0.04,0.08,0},{0.01,0.02,0},{0,1,0},0,0,1,0,{0.95,1,0},0,0,0,0,0,{0,1,1},1,8444415,0},
    {"glassPlank",kParticleImgs_3695,1,20,{1,1.5,0},{1,5,0},{9.42477796076938,9.42477796076938,1},{0.1,0.2,0},{0.08,0.18,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,8444415,0},
    {"goldChip",kParticleImgs_3877,1,20,{0.5,0.5,1},{1,10,0},{0,0,1},{0.04,0.08,0},{0.01,0.02,0},{0,1,0},0,0,1,0,{0.95,1,0},0,0,0,0,0,{0,1,1},1,14918180,0},
    {"pinkChip",kParticleImgs_4034,1,20,{0.5,1,0},{1,5,0},{9.42477796076938,9.42477796076938,1},{0.04,0.08,0},{0.01,0.02,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,16611705,0},
    {"ltblueChip",kParticleImgs_4217,1,20,{0.5,1,0},{1,5,0},{9.42477796076938,9.42477796076938,1},{0.04,0.08,0},{0.01,0.02,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,5832189,0},
    {"yellowChip",kParticleImgs_4401,1,20,{0.5,1,0},{1,5,0},{9.42477796076938,9.42477796076938,1},{0.04,0.08,0},{0.01,0.02,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,16643396,0},
    {"greenChip",kParticleImgs_4586,1,20,{0.5,0.5,1},{1,10,0},{0,0,1},{0.04,0.08,0},{0.01,0.02,0},{0,1,0},0,0,1,0,{0.95,1,0},0,0,0,0,0,{0,1,1},1,7704448,0},
    {"greenPlank",kParticleImgs_4743,1,20,{1,1.5,0},{1,5,0},{9.42477796076938,9.42477796076938,1},{0.08,0.16,0},{0.05,0.1,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,3884335,0},
    {"greenhouseBreak",kParticleImgs_4926,3,20,{0.5,1.5,0},{1,5,0},{3.141592653589793,18.84955592153876,0},{0.25,0.55,0},{0.08,0.18,0},{0,1,0},0,0,0.8,0,{0.75,1,0},0,0,0,0,0,{0,1,1},1,8444415,0},
    {"hutBreak",kParticleImgs_5122,1,20,{0.5,1.5,0},{1,5,0},{0,9.42477796076938,0},{0.25,0.55,0},{0.08,0.18,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,13404711,0},
    {"leaf",kParticleImgs_5292,1,20,{0.5,1,0},{1,5,0},{9.42477796076938,9.42477796076938,1},{0.04,0.08,0},{0.01,0.02,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,2793472,0},
    {"leafSynthetic",kParticleImgs_5470,1,20,{0.5,1,0},{1,5,0},{9.42477796076938,9.42477796076938,1},{0.04,0.08,0},{0.01,0.02,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,868397,0},
    {"leafPrickly",kParticleImgs_5656,1,20,{0.5,1,0},{1,5,0},{9.42477796076938,9.42477796076938,1},{0.04,0.08,0},{0.01,0.02,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,13882323,0},
    {"leafRiver",kParticleImgs_5842,1,20,{0.5,1,0},{1,5,0},{9.42477796076938,9.42477796076938,1},{0.04,0.08,0},{0.01,0.02,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,10526880,0},
    {"lockerBreak",kParticleImgs_6026,1,20,{0.5,1,0},{7,8,0},{0,9.42477796076938,0},{0.15,0.2,0},{0.12,0.15,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,8747110,0},
    {"ltgreenChip",kParticleImgs_6195,1,20,{0.5,1,0},{1,5,0},{9.42477796076938,9.42477796076938,1},{0.04,0.08,0},{0.01,0.02,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,6121019,0},
    {"outhouseChip",kParticleImgs_6380,1,20,{0.5,1,0},{1,5,0},{9.42477796076938,9.42477796076938,1},{0.04,0.08,0},{0.01,0.02,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,7228719,0},
    {"outhouseBreak",kParticleImgs_6566,1,20,{0.5,1.5,0},{1,5,0},{0,9.42477796076938,0},{0.25,0.55,0},{0.08,0.18,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,8867868,0},
    {"outhousePlank",kParticleImgs_6740,1,20,{1,1.5,0},{1,5,0},{9.42477796076938,9.42477796076938,1},{0.1,0.2,0},{0.08,0.18,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,7228719,0},
    {"potChip",kParticleImgs_6925,1,20,{0.5,0.5,1},{1,10,0},{0,0,1},{0.04,0.08,0},{0.01,0.02,0},{0,1,0},0,0,1,0,{0.95,1,0},0,0,0,0,0,{0,1,1},1,12539935,0},
    {"potBreak",kParticleImgs_7081,1,20,{0.8,1,0},{1,5,0},{0,0,1},{0.07,0.12,0},{0.05,0.1,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,12539935,0},
    {"potatoChip",kParticleImgs_7233,1,20,{0.5,0.5,1},{1,10,0},{0,0,1},{0.04,0.08,0},{0.01,0.02,0},{0,1,0},0,0,1,0,{0.95,1,0},0,0,0,0,0,{0,1,1},1,8216391,0},
    {"potatoBreak",kParticleImgs_7391,1,20,{0.8,1,0},{1,5,0},{0,0,1},{0.07,0.12,0},{0.05,0.1,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,8216391,0},
    {"tomatoChip_01",kParticleImgs_7545,1,20,{0.5,0.5,1},{1,10,0},{0,0,1},{0.04,0.08,0},{0.01,0.02,0},{0,1,0},0,0,1,0,{0.95,1,0},0,0,0,0,0,{0,1,1},1,11752275,0},
    {"tomatoChip_02",kParticleImgs_7707,1,20,{0.5,0.5,1},{1,10,0},{0,0,1},{0.04,0.08,0},{0.01,0.02,0},{0,1,0},0,0,1,0,{0.95,1,0},0,0,0,0,0,{0,1,1},1,6261819,0},
    {"tomatoBreak_01",kParticleImgs_7868,1,20,{0.8,1,0},{1,5,0},{0,0,1},{0.07,0.12,0},{0.05,0.1,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,11752275,0},
    {"tomatoBreak_02",kParticleImgs_8026,1,20,{0.8,1,0},{1,5,0},{0,0,1},{0.07,0.12,0},{0.05,0.1,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,6261819,0},
    {"pumpkinChip",kParticleImgs_8183,1,20,{0.5,0.5,1},{1,10,0},{0,0,1},{0.04,0.08,0},{0.01,0.02,0},{0,1,0},0,0,1,0,{0.95,1,0},0,0,0,0,0,{0,1,1},1,16607744,0},
    {"pumpkinBreak",kParticleImgs_8343,1,20,{0.8,1,0},{1,5,0},{0,0,1},{0.07,0.12,0},{0.05,0.1,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,16086272,0},
    {"squashChip",kParticleImgs_8499,1,20,{0.5,0.5,1},{1,10,0},{0,0,1},{0.04,0.08,0},{0.01,0.02,0},{0,1,0},0,0,1,0,{0.95,1,0},0,0,0,0,0,{0,1,1},1,1595917,0},
    {"squashBreak",kParticleImgs_8657,1,20,{0.8,1,0},{1,5,0},{0,0,1},{0.07,0.12,0},{0.05,0.1,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,1595917,0},
    {"redChip",kParticleImgs_8811,1,20,{0.5,0.5,1},{1,10,0},{0,0,1},{0.04,0.08,0},{0.01,0.02,0},{0,1,0},0,0,1,0,{0.95,1,0},0,0,0,0,0,{0,1,1},1,8847376,0},
    {"redBreak",kParticleImgs_8966,1,20,{0.8,1,0},{1,5,0},{0,0,1},{0.07,0.12,0},{0.05,0.1,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,8847376,0},
    {"redPlank",kParticleImgs_9117,1,20,{1,1.5,0},{1,5,0},{9.42477796076938,9.42477796076938,1},{0.1,0.2,0},{0.08,0.18,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,4524032,0},
    {"rockChip",kParticleImgs_9297,1,20,{0.5,0.5,1},{1,10,0},{0,0,1},{0.04,0.08,0},{0.01,0.02,0},{0,1,0},0,0,1,0,{0.95,1,0},0,0,0,0,0,{0,1,1},1,10526880,0},
    {"rockBreak",kParticleImgs_9454,1,20,{0.8,1,0},{1,5,0},{0,0,1},{0.07,0.12,0},{0.05,0.1,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,10526880,0},
    {"rockEyeChip",kParticleImgs_9607,1,20,{0.5,0.5,1},{1,10,0},{0,0,1},{0.03,0.06,0},{0.01,0.02,0},{0,1,0},0,0,1,0,{0.95,1,0},0,0,0,0,0,{0,1,1},1,2696225,0},
    {"rockEyeBreak",kParticleImgs_9766,1,20,{0.8,1,0},{4,12,0},{0,0,1},{0.05,0.1,0},{0.03,0.06,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,2696225,0},
    {"shackBreak",kParticleImgs_9922,1,20,{0.5,1.5,0},{1,5,0},{0,9.42477796076938,0},{0.25,0.55,0},{0.08,0.18,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,6642509,0},
    {"shackGreenBreak",kParticleImgs_10093,1,20,{0.5,1.5,0},{1,5,0},{0,9.42477796076938,0},{0.25,0.55,0},{0.08,0.18,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,5730406,0},
    {"tanChip",kParticleImgs_10270,1,20,{0.5,1,0},{1,5,0},{9.42477796076938,9.42477796076938,1},{0.04,0.08,0},{0.01,0.02,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,8416851,0},
    {"teahouseBreak",kParticleImgs_10452,1,20,{0.5,1.5,0},{1,5,0},{0,9.42477796076938,0},{0.25,0.55,0},{0.08,0.18,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,5069423,0},
    {"teapavilionBreak",kParticleImgs_10627,1,20,{0.5,1.5,0},{1,5,0},{0,9.42477796076938,0},{0.25,0.55,0},{0.08,0.18,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,10231583,0},
    {"toiletBreak",kParticleImgs_10806,1,20,{0.8,1,0},{1,5,0},{0,0,1},{0.07,0.12,0},{0.05,0.1,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,16119285,0},
    {"toiletGoldChip",kParticleImgs_10962,1,20,{0.5,0.5,1},{1,10,0},{0,0,1},{0.04,0.08,0},{0.01,0.02,0},{0,1,0},0,0,1,0,{0.95,1,0},0,0,0,0,0,{0,1,1},1,13020457,0},
    {"toiletGoldBreak",kParticleImgs_11126,1,20,{0.8,1,0},{4,5,0},{0,0,1},{0.07,0.12,0},{0.05,0.1,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,13020457,0},
    {"toiletMetalBreak",kParticleImgs_11286,1,20,{0.8,1,0},{4,5,0},{0,0,1},{0.07,0.12,0},{0.05,0.1,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,6644579,0},
    {"turkeyFeathersHit",kParticleImgs_11446,2,20,{1,1.5,0},{1,10,0},{0,9.42477796076938,0},{0.1,0.2,0},{0.08,0.12,0},{0,1,0},0,0,1,0,{0.95,1,0},0,0,0,0,0,{0,1,1},1,16777215,0},
    {"turkeyFeathersDeath",kParticleImgs_11624,2,20,{1,1.5,0},{1,10,0},{0,9.42477796076938,0},{0.15,0.25,0},{0.12,0.2,0},{0,1,0},0,0,1,0,{0.95,1,0},0,0,0,0,0,{0,1,1},1,16777215,0},
    {"whiteChip",kParticleImgs_11805,1,20,{0.5,0.5,1},{1,10,0},{0,0,1},{0.04,0.08,0},{0.01,0.02,0},{0,1,0},0,0,1,0,{0.95,1,0},0,0,0,0,0,{0,1,1},1,16119285,0},
    {"whitePlank",kParticleImgs_11964,1,20,{1,1.5,0},{1,5,0},{9.42477796076938,9.42477796076938,1},{0.1,0.2,0},{0.08,0.18,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,16119285,0},
    {"windowBreak",kParticleImgs_12148,1,20,{0.4,0.8,0},{1,4,0},{3.141592653589793,18.84955592153876,0},{0.07,0.12,0},{0.05,0.1,0},{0,1,0},0,0,0.8,0,{0.75,1,0},0,0,0,0,0,{0,1,1},1,8444415,0},
    {"woodChip",kParticleImgs_12340,1,20,{0.5,1,0},{1,5,0},{9.42477796076938,9.42477796076938,1},{0.04,0.08,0},{0.01,0.02,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,6692608,0},
    {"woodLog",kParticleImgs_12523,1,20,{1,1.5,0},{1,5,0},{9.42477796076938,9.42477796076938,1},{0.1,0.2,0},{0.08,0.18,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,6692608,0},
    {"woodPlank",kParticleImgs_12703,1,20,{1,1.5,0},{1,5,0},{9.42477796076938,9.42477796076938,1},{0.1,0.2,0},{0.08,0.18,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,5052160,0},
    {"woodShard",kParticleImgs_12885,1,20,{1,1.5,0},{3,5,0},{9.42477796076938,9.42477796076938,1},{0.06,0.15,0},{0.02,0.1,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,5052160,0},
    {"9mm",kParticleImgs_13068,1,20,{0.5,0.75,0},{3,4,0},{9.42477796076938,9.42477796076938,1},{0.0625,0.0625,1},{0.0325,0.0325,1},{0,1,0},0,0,1,0,{0.95,1,0},0,0,0,0,0,{0,1,1},1,15527148,0},
    {"9mm_cursed",kParticleImgs_13259,1,20,{0.5,0.75,0},{3,4,0},{9.42477796076938,9.42477796076938,1},{0.0625,0.0625,1},{0.0325,0.0325,1},{0,1,0},0,0,1,0,{0.95,1,0},0,0,0,0,0,{0,1,1},1,15527148,0},
    {"762mm",kParticleImgs_13457,1,20,{0.75,1,0},{1.5,2.5,0},{7.853981633974483,7.853981633974483,1},{0.075,0.075,1},{0.045,0.045,1},{0,1,0},0,0,1,0,{0.925,1,0},0,0,0,0,0,{0,1,1},1,15527148,0},
    {"556mm",kParticleImgs_13651,1,20,{0.75,1,0},{1.5,2.5,0},{7.853981633974483,7.853981633974483,1},{0.075,0.075,1},{0.045,0.045,1},{0,1,0},0,0,1,0,{0.925,1,0},0,0,0,0,0,{0,1,1},1,15527148,0},
    {"12gauge",kParticleImgs_13845,1,20,{0.5,0.75,0},{1,2,0},{9.42477796076938,9.42477796076938,1},{0.1,0.1,1},{0.05,0.05,1},{0,1,0},0,0,1,0,{0.95,1,0},0,0,0,0,0,{0,1,1},1,15527148,0},
    {"50AE",kParticleImgs_14030,1,20,{0.5,0.75,0},{3,4,0},{9.42477796076938,9.42477796076938,1},{0.0625,0.0625,1},{0.0325,0.0325,1},{0,1,0},0,0,1,0,{0.95,1,0},0,0,0,0,0,{0,1,1},1,15527148,0},
    {"50cal",kParticleImgs_14222,1,20,{0.5,0.75,0},{3,4,0},{9.42477796076938,9.42477796076938,1},{0.0625,0.0625,1},{0.0325,0.0325,1},{0,1,0},0,0,1,0,{0.95,1,0},0,0,0,0,0,{0,1,1},1,15527148,0},
    {"308sub",kParticleImgs_14415,1,20,{0.5,0.75,0},{3,4,0},{9.42477796076938,9.42477796076938,1},{0.0625,0.0625,1},{0.0325,0.0325,1},{0,1,0},0,0,1,0,{0.95,1,0},0,0,0,0,0,{0,1,1},1,15527148,0},
    {"flare",kParticleImgs_14609,1,20,{0.5,0.75,0},{1,2,0},{9.42477796076938,9.42477796076938,1},{0.1,0.1,1},{0.05,0.05,1},{0,1,0},0,0,1,0,{0.95,1,0},0,0,0,0,0,{0,1,1},1,15527148,0},
    {"45acp",kParticleImgs_14792,1,20,{0.5,0.75,0},{3,4,0},{9.42477796076938,9.42477796076938,1},{0.07,0.07,1},{0.04,0.04,1},{0,1,0},0,0,1,0,{0.95,1,0},0,0,0,0,0,{0,1,1},1,15527148,0},
    {"potato_ammo",kParticleImgs_14977,1,20,{0.5,0.75,0},{3,4,0},{9.42477796076938,9.42477796076938,1},{0.07,0.07,1},{0.04,0.04,1},{0,1,0},0,0,1,0,{0.95,1,0},0,0,0,0,0,{0,1,1},1,16777215,0},
    {"bugle_ammo",kParticleImgs_15168,1,20,{1.25,1.3,0},{3,4,0},{3.141592653589793,3.141592653589793,1},{0.1,0.1,1},{0.14,0.14,1},{0,1,0},0,0,1,0,{0.5,1,0},0,0,0,0,0,{0,1,1},1,16767488,0},
    {"fragPin",kParticleImgs_15357,1,20,{0.5,0.5,1},{0.9,1,0},{0,0,1},{0.18,0.18,1},{0.14,0.14,1},{0,1,0},0,0,1,0,{0.5,1,0},0,0,0,0,0,{0,1,1},1,16777215,0},
    {"fragLever",kParticleImgs_15514,1,20,{0.5,0.5,1},{0.9,1,0},{28.274333882308138,28.274333882308138,1},{0.18,0.18,1},{0.14,0.14,1},{0,1,0},0,0,1,0,{0.5,1,0},0,0,0,0,0,{0,1,1},1,16777215,0},
    {"explosionBurst",kParticleImgs_15707,1,20,{0.5,0.5,1},{0,0,1},{0,0,1},{1,1,1},{4,4,1},{0,1,0},0,0,1,0,{0.75,1,0},0,0,0,0,0,{0,1,1},1,16474624,0},
    {"explosionMIRV",kParticleImgs_15858,1,20,{0.5,0.5,1},{0,0,1},{0,0,1},{1,1,1},{4,4,1},{0,1,0},0,0,1,0,{0.75,1,0},0,0,0,0,0,{0,1,1},1,13893632,0},
    {"explosionSmoke",kParticleImgs_16008,1,20,{2,3,0},{0,0,1},{0.7853981633974483,1.5707963267948966,0},{0.07,0.12,0},{0.05,0.1,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,15527148,0},
    {"explosionUSAS",kParticleImgs_16199,1,20,{0.5,0.5,1},{0,0,1},{0,0,1},{1,1,1},{4,4,1},{0,1,0},0,0,1,0,{0.75,1,0},0,0,0,0,0,{0,1,1},1,16480256,0},
    {"explosionRounds",kParticleImgs_16349,1,20,{0.5,0.5,1},{0,0,1},{0,0,1},{1,1,1},{4,4,1},{0,1,0},0,0,1,0,{0.75,1,0},0,0,0,0,0,{0,1,1},1,13008443,0},
    {"explosionBomb",kParticleImgs_16501,1,20,{0.5,0.5,1},{0,0,1},{0,0,1},{1,1,1},{4,4,1},{0,1,0},0,0,1,0,{0.75,1,0},0,0,0,0,0,{0,1,1},1,16777215,0},
    {"explosionPotato",kParticleImgs_16651,1,20,{0.5,0.5,1},{0,0,1},{0,0,1},{1,1,1},{4,4,1},{0,1,0},0,0,1,0,{0.75,1,0},0,0,0,0,0,{0,1,1},1,11363866,0},
    {"explosionPotatoSMG",kParticleImgs_16803,1,20,{0.5,0.5,1},{0,0,1},{0,0,1},{1,1,1},{4,4,1},{0,1,0},0,0,1,0,{0.75,1,0},0,0,0,0,0,{0,1,1},1,12888074,0},
    {"airdropSmoke",kParticleImgs_16958,2,499,{1,1.5,0},{0,0,1},{0.7853981633974483,1.5707963267948966,0},{0.67,0.72,0},{0.55,0.61,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,15527148,0},
    {"airdropCrate01",kParticleImgs_17151,1,20,{0.85,1.15,0},{2,2.25,0},{3.141592653589793,6.283185307179586,0},{0.5,0.5,1},{0.4,0.4,1},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,16777215,0},
    {"airdropCrate01h",kParticleImgs_17346,1,20,{0.85,1.15,0},{2,2.25,0},{3.141592653589793,6.283185307179586,0},{0.5,0.5,1},{0.4,0.4,1},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,16777215,0},
    {"airdropCrate01x",kParticleImgs_17542,1,20,{0.85,1.15,0},{2,2.25,0},{3.141592653589793,6.283185307179586,0},{0.5,0.5,1},{0.4,0.4,1},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,16777215,0},
    {"airdropCrate02",kParticleImgs_17738,1,20,{0.85,1.15,0},{1.85,2.15,0},{0,6.283185307179586,0},{0.5,0.5,1},{0.4,0.4,1},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,16777215,0},
    {"airdropCrate02h",kParticleImgs_17920,1,20,{0.85,1.15,0},{1.85,2.15,0},{0,6.283185307179586,0},{0.5,0.5,1},{0.4,0.4,1},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,16777215,0},
    {"airdropCrate02x",kParticleImgs_18103,1,20,{0.85,1.15,0},{1.85,2.15,0},{0,6.283185307179586,0},{0.5,0.5,1},{0.4,0.4,1},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,16777215,0},
    {"airdropCrate03",kParticleImgs_18286,1,20,{0.85,1.15,0},{2,2.25,0},{3.141592653589793,6.283185307179586,0},{0.5,0.5,1},{0.4,0.4,1},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,16777215,0},
    {"airdropCrate04",kParticleImgs_18481,1,20,{0.85,1.15,0},{1.85,2.15,0},{0,6.283185307179586,0},{0.5,0.5,1},{0.4,0.4,1},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,16777215,0},
    {"classShell01a",kParticleImgs_18663,1,20,{0.85,1.15,0},{2,2.25,0},{3.141592653589793,6.283185307179586,0},{0.5,0.5,1},{0.4,0.4,1},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,16777215,0},
    {"classShell01b",kParticleImgs_18857,1,20,{0.85,1.15,0},{1.85,2.15,0},{0,6.283185307179586,0},{0.5,0.5,1},{0.4,0.4,1},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,16777215,0},
    {"classShell02a",kParticleImgs_19038,1,20,{0.85,1.15,0},{2,2.25,0},{3.141592653589793,6.283185307179586,0},{0.5,0.5,1},{0.4,0.4,1},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,16777215,0},
    {"classShell02b",kParticleImgs_19232,1,20,{0.85,1.15,0},{1.85,2.15,0},{0,6.283185307179586,0},{0.5,0.5,1},{0.4,0.4,1},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,16777215,0},
    {"classShell03a",kParticleImgs_19413,1,20,{0.85,1.15,0},{2,2.25,0},{3.141592653589793,6.283185307179586,0},{0.5,0.5,1},{0.4,0.4,1},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,16777215,0},
    {"classShell03b",kParticleImgs_19607,1,20,{0.85,1.15,0},{1.85,2.15,0},{0,6.283185307179586,0},{0.5,0.5,1},{0.4,0.4,1},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,16777215,0},
    {"cabinSmoke",kParticleImgs_19788,2,20,{3,3.25,0},{0.2,0.22,0},{0.7853981633974483,1.5707963267948966,0},{0.2,0.25,0},{0.6,0.65,0},{0,1,0},0,0,0.7,0,{0.9,1,0},0,0,1,0,0.7,{0,0.1,0},1,11645361,0},
    {"bathhouseSteam",kParticleImgs_19988,2,20,{10,12,0},{0.04,0.06,0},{0.7853981633974483,1.5707963267948966,0},{0.2,0.25,0},{0.9,0.95,0},{0,1,0},0,0,0.5,0,{0.9,1,0},0,0,1,0,0.5,{0,0.1,0},1,16645629,0},
    {"bunkerBubbles",kParticleImgs_20192,1,10,{2.25,2.5,0},{1.85,2.15,0},{0.7853981633974483,1.5707963267948966,0},{0.2,0.25,0},{0.65,0.7,0},{0,1,0},0,0,0.25,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,16382457,0},
    {"waterRipple",kParticleImgs_20395,1,10,{1.75,1.75,1},{0,0,1},{0,0,1},{0.15,0.15,1},{0,0,1},{0,1,1},1,0.5,1,0,{0,1,1},1,-1,0,0,0,{0,1,1},1,11792639,0},
    {"leafAutumn",kParticleImgs_20551,4,20,{10,15,0},{0,0,1},{0.7853981633974483,1.5707963267948966,0},{0.12,0.15,0},{0.08,0.11,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,1,0,1,{0,0.05,0},1,15527148,0},
    {"leafHalloween",kParticleImgs_20744,4,20,{10,15,0},{0,0,1},{0.7853981633974483,1.5707963267948966,0},{0.12,0.15,0},{0.08,0.11,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,1,0,1,{0,0.05,0},1,8816262,1},
    {"leafSpring",kParticleImgs_20939,4,20,{10,15,0},{0,0,1},{0.7853981633974483,1.5707963267948966,0},{0.13,0.15,0},{0.08,0.11,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,1,0,1,{0,0.05,0},1,15527148,0},
    {"leafSummer",kParticleImgs_21132,1,20,{10,15,0},{0,0,1},{0.7853981633974483,1.5707963267948966,0},{0.12,0.15,0},{0.08,0.11,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,1,0,1,{0,0.05,0},1,13882323,1},
    {"leafPotato",kParticleImgs_21325,5,20,{10,15,0},{0,0,1},{0.7853981633974483,1.5707963267948966,0},{0.13,0.15,0},{0.08,0.11,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,1,0,1,{0,0.05,0},1,15527148,0},
    {"potato",kParticleImgs_21518,1,20,{10,15,0},{0,0,1},{0.7853981633974483,1.5707963267948966,0},{0.13,0.15,0},{0.08,0.11,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,1,0,1,{0,0.05,0},1,15527148,0},
    {"potato_factions",kParticleImgs_21707,2,20,{10,15,0},{0,0,1},{0.7853981633974483,1.5707963267948966,0},{0.13,0.15,0},{0.08,0.11,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,1,0,1,{0,0.05,0},1,15527148,0},
    {"snow",kParticleImgs_21905,1,20,{10,15,0},{0,0,1},{0.7853981633974483,1.5707963267948966,0},{0.07,0.12,0},{0.05,0.1,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,1,0,1,{0,0.05,0},1,15527148,0},
    {"snowball_impact",kParticleImgs_22091,1,20,{0.5,1,0},{0,0,1},{0.7853981633974483,1.5707963267948966,0},{0.13,0.23,0},{0.07,0.14,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,15527148,0},
    {"potato_impact",kParticleImgs_22286,1,20,{0.5,1,0},{0,0,1},{0.7853981633974483,1.5707963267948966,0},{0.13,0.23,0},{0.07,0.14,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,15527148,0},
    {"potato_smg_impact",kParticleImgs_22479,1,20,{0.5,1,0},{0,0,1},{0.7853981633974483,1.5707963267948966,0},{0.13,0.23,0},{0.07,0.14,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,16770437,0},
    {"coconut_impact",kParticleImgs_22676,3,20,{0.5,1,0},{0,0,1},{0.7853981633974483,1.5707963267948966,0},{0.13,0.23,0},{0.07,0.14,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,15527148,0},
    {"tomato_impact",kParticleImgs_22870,1,20,{0.5,1,0},{0,0,1},{0.7853981633974483,1.5707963267948966,0},{0.13,0.23,0},{0.07,0.14,0},{0,1,0},0,0,1,0,{0.9,1,0},0,0,0,0,0,{0,1,1},1,15527148,0},
    {"heal_basic",kParticleImgs_23063,1,20,{0.75,1,0},{0.25,0.25,1},{0,0,1},{0.1,0.12,0},{0.05,0.07,0},{0,1,0},0,0,1,0,{0.7,1,0},0,0,1,0,1,{0,0.05,0},1,14221312,1},
    {"heal_heart",kParticleImgs_23228,1,20,{0.75,1,0},{0.25,0.25,1},{0,0,1},{0.1,0.12,0},{0.05,0.07,0},{0,1,0},0,0,1,0,{0.7,1,0},0,0,1,0,1,{0,0.05,0},1,14221312,1},
    {"heal_moon",kParticleImgs_23393,1,20,{0.75,1,0},{0.25,0.25,1},{0.7853981633974483,1.5707963267948966,0},{0.1,0.12,0},{0.05,0.07,0},{0,1,0},0,0,1,0,{0.7,1,0},0,0,1,0,1,{0,0.05,0},1,14221312,1},
    {"heal_tomoe",kParticleImgs_23591,1,20,{0.75,1,0},{0.25,0.25,1},{1.5707963267948966,3.141592653589793,0},{0.1,0.12,0},{0.05,0.07,0},{0,1,0},0,0,1,0,{0.7,1,0},0,0,1,0,1,{0,0.05,0},1,14221312,1},
    {"heal_diamond",kParticleImgs_23789,1,20,{0.75,1,0},{0.25,0.25,1},{0,0,1},{0.1,0.12,0},{0.05,0.07,0},{0,1,0},0,0,1,0,{0.7,1,0},0,0,1,0,1,{0,0.05,0},1,14221312,1},
    {"heal_ankh",kParticleImgs_23956,1,20,{0.75,1,0},{0.25,0.25,1},{1.5707963267948966,3.141592653589793,0},{0.1,0.12,0},{0.05,0.07,0},{0,1,0},0,0,1,0,{0.7,1,0},0,0,1,0,1,{0,0.05,0},1,14221312,1},
    {"heal_menacing",kParticleImgs_24153,1,20,{0.75,1,0},{0.25,0.25,1},{0,0,1},{0.1,0.12,0},{0.05,0.07,0},{0,1,0},0,0,1,0,{0.7,1,0},0,0,1,0,1,{0,0.05,0},1,14221312,1},
    {"boost_basic",kParticleImgs_24321,1,20,{0.75,1,0},{0,0,1},{0.7853981633974483,1.5707963267948966,0},{0.12,0.14,0},{0.06,0.08,0},{0,1,0},0,0,1,0,{0.7,1,0},0,0,1,0,1,{0,0.05,0},1,2939136,1},
    {"boost_star",kParticleImgs_24515,1,20,{0.75,1,0},{0,0,1},{0.7853981633974483,1.5707963267948966,0},{0.12,0.14,0},{0.06,0.08,0},{0,1,0},0,0,1,0,{0.7,1,0},0,0,1,0,1,{0,0.05,0},1,2939136,1},
    {"boost_naturalize",kParticleImgs_24708,1,20,{0.75,1,0},{0,0,1},{1.0995574287564276,2.199114857512855,0},{0.12,0.14,0},{0.06,0.08,0},{0,1,0},0,0,1,0,{0.7,1,0},0,0,1,0,1,{0,0.05,0},1,2939136,1},
    {"boost_shuriken",kParticleImgs_24906,1,20,{0.75,1,0},{0,0,1},{3.141592653589793,6.283185307179586,0},{0.12,0.14,0},{0.06,0.08,0},{0,1,0},0,0,1,0,{0.7,1,0},0,0,1,0,1,{0,0.05,0},1,2939136,1},
    {"boost_club",kParticleImgs_25101,1,20,{0.75,1,0},{0,0,1},{0.7853981633974483,1.5707963267948966,0},{0.12,0.14,0},{0.06,0.08,0},{0,1,0},0,0,1,0,{0.7,1,0},0,0,1,0,1,{0,0.05,0},1,2939136,1},
    {"boost_lightning",kParticleImgs_25294,1,20,{0.75,1,0},{0,0,1},{0.7853981633974483,1.5707963267948966,0},{0.12,0.14,0},{0.06,0.08,0},{0,1,0},0,0,1,0,{0.7,1,0},0,0,1,0,1,{0,0.05,0},1,2939136,1},
    {"boost_hermes",kParticleImgs_25492,1,20,{0.75,1,0},{0,0,1},{0.7853981633974483,1.5707963267948966,0},{0.12,0.14,0},{0.06,0.08,0},{0,1,0},0,0,1,0,{0.7,1,0},0,0,1,0,1,{0,0.05,0},1,2939136,1},
    {"boost_gearshift_01",kParticleImgs_25687,1,20,{0.75,1,0},{0,0,1},{0,0,1},{0.12,0.14,0},{0.06,0.08,0},{0,1,0},0,0,1,0,{0.7,1,0},0,0,1,0,1,{0,0.05,0},1,2939136,1},
    {"boost_gearshift_02",kParticleImgs_25854,1,20,{0.75,1,0},{0,0,1},{3.141592653589793,6.283185307179586,0},{0.12,0.14,0},{0.06,0.08,0},{0,1,0},0,0,1,0,{0.7,1,0},0,0,1,0,1,{0,0.05,0},1,2939136,1},
    {"revive_basic",kParticleImgs_26053,1,20,{0.75,1,0},{0.25,0.25,1},{0,0,1},{0.1,0.12,0},{0.05,0.07,0},{0,1,0},0,0,1,0,{0.7,1,0},0,0,1,0,1,{0,0.05,0},1,13959385,1},
    {"leafStim",kParticleImgs_26220,4,20,{4,5,0},{0,0,1},{0.7853981633974483,1.5707963267948966,0},{0.12,0.14,0},{0.06,0.08,0},{0,1,0},0,0,1,0,{0.7,1,0},0,0,1,0,1,{0,0.05,0},1,63799,0},
    {"takedownStim",kParticleImgs_26406,1,20,{4,5,0},{0,0,1},{0.7853981633974483,1.5707963267948966,0},{0.12,0.14,0},{0.06,0.08,0},{0,1,0},0,0,1,0,{0.7,1,0},0,0,1,0,1,{0,0.05,0},1,13107200,0},
    {"inspireStim",kParticleImgs_26599,1,20,{4,5,0},{0,0,1},{0.7853981633974483,1.5707963267948966,0},{0.12,0.14,0},{0.06,0.08,0},{0,1,0},0,0,1,0,{0.7,1,0},0,0,1,0,1,{0,0.05,0},1,16631040,0},
    {"xp_common",kParticleImgs_26791,1,20,{0.75,1,0},{0,0,1},{0.7853981633974483,1.5707963267948966,0},{0.12,0.14,0},{0.06,0.08,0},{0,1,0},0,0,1,0,{0.7,1,0},0,0,1,0,1,{0,0.05,0},1,16305668,1},
    {"xp_rare",kParticleImgs_26984,1,20,{0.75,1,0},{0,0,1},{0.7853981633974483,1.5707963267948966,0},{0.12,0.14,0},{0.06,0.08,0},{0,1,0},0,0,1,0,{0.7,1,0},0,0,1,0,1,{0,0.05,0},1,15291661,1},
    {"xp_mythic",kParticleImgs_27175,1,20,{0.75,1,0},{0,0,1},{0.7853981633974483,1.5707963267948966,0},{0.12,0.14,0},{0.06,0.08,0},{0,1,0},0,0,1,0,{0.7,1,0},0,0,1,0,1,{0,0.05,0},1,15539210,1},
};
static const RawEmitter kEmitters[] = {
    {"smoke_barrel","explosionSmoke",{0.2,0.3,0},0,{2,3,0},0.3141592653589793,1,{0,6.283185307179586,0},3.402823466e+38,0,{0,0,1},0,0,0},
    {"cabin_smoke_parent","cabinSmoke",{0.72,0.83,0},0,{64,96,0},0.3141592653589793,1,{0,6.283185307179586,0},3.402823466e+38,0,{0,0,1},0,0,0},
    {"campfire_smoke","cabinSmoke",{2,4,0},0,{1,1.5,0},0.3141592653589793,1,{0,6.283185307179586,0},3.402823466e+38,0,{0,0,1},0,0,0},
    {"bathhouse_steam","bathhouseSteam",{2,3,0},1,{1.5,2,0},0.3141592653589793,0,{0,0,1},3.402823466e+38,0,{0,0,1},0,0,0},
    {"bunker_bubbles_01","bunkerBubbles",{0.3,0.325,0},0,{1.6,1.8,0},-6.911503837897546,1,{0,6.283185307179586,0},3.402823466e+38,0,{0,0,1},0,0,0},
    {"bunker_bubbles_02","bunkerBubbles",{0.4,0.425,0},0,{1.6,1.8,0},-6.911503837897546,1,{0,6.283185307179586,0},3.402823466e+38,0,{0,0,1},0,0,0},
    {"falling_leaf","leafAutumn",{0.08,0.12,0},120,{2,3,0},0.6283185307179586,1,{0,6.283185307179586,0},3.402823466e+38,0,{0,0,1},0,1,999},
    {"falling_leaf_halloween","leafHalloween",{0.08,0.12,0},120,{2,3,0},0.6283185307179586,1,{0,6.283185307179586,0},3.402823466e+38,0,{0,0,1},0,1,999},
    {"falling_leaf_spring","leafSpring",{0.1,0.14,0},120,{2,3,0},0.6283185307179586,1,{0,6.283185307179586,0},3.402823466e+38,0,{0,0,1},0,1,999},
    {"falling_leaf_summer","leafSummer",{0.18,0.24,0},120,{1.4,2.4,0},0.6283185307179586,0,{0,0,1},3.402823466e+38,0,{0,0,1},0,1,999},
    {"falling_leaf_potato","leafPotato",{0.1,0.14,0},120,{2,3,0},0.6283185307179586,1,{0,6.283185307179586,0},3.402823466e+38,0,{0,0,1},0,1,999},
    {"falling_potato","potato",{0.2,0.24,0},120,{2,3,0},0.6283185307179586,1,{0,6.283185307179586,0},3.402823466e+38,0,{0,0,1},0,1,999},
    {"falling_pvt","potato_factions",{0.2,0.24,0},120,{2,3,0},0.6283185307179586,1,{0,6.283185307179586,0},3.402823466e+38,0,{0,0,1},0,1,999},
    {"falling_snow_fast","snow",{0.12,0.17,0},70,{1,1.5,0},0.6283185307179586,1,{0,6.283185307179586,0},3.402823466e+38,1,{0.05,0.07,0},240,1,999},
    {"falling_snow_slow","snow",{0.08,0.12,0},70,{1,1.5,0},0.6283185307179586,1,{0,6.283185307179586,0},3.402823466e+38,0,{0,0,1},0,1,999},
    {"heal_basic","heal_basic",{0.3,0.35,0},1.5,{1,1.5,0},0,1,{0,0,1},3.402823466e+38,0,{0,0,1},0,0,0},
    {"heal_heart","heal_heart",{0.3,0.35,0},1.5,{1,1.5,0},0,1,{0,0,1},3.402823466e+38,0,{0,0,1},0,0,0},
    {"heal_moon","heal_moon",{0.3,0.35,0},1.5,{1,1.5,0},0,1,{0,0,1},3.402823466e+38,0,{0,0,1},0,0,0},
    {"heal_tomoe","heal_tomoe",{0.3,0.35,0},1.5,{1,1.5,0},0,1,{0,0,1},3.402823466e+38,0,{0,0,1},0,0,0},
    {"heal_diamond","heal_diamond",{0.3,0.35,0},1.5,{1,1.5,0},0,1,{0,0,1},3.402823466e+38,0,{0,0,1},0,0,0},
    {"heal_ankh","heal_ankh",{0.3,0.35,0},1.5,{1,1.5,0},0,1,{0,0,1},3.402823466e+38,0,{0,0,1},0,0,0},
    {"heal_menacing","heal_menacing",{0.3,0.35,0},1.5,{1,1.5,0},0,1,{0,0,1},3.402823466e+38,0,{0,0,1},0,0,0},
    {"boost_basic","boost_basic",{0.3,0.35,0},1.5,{1,1.5,0},0,1,{0,6.283185307179586,0},3.402823466e+38,0,{0,0,1},0,0,0},
    {"boost_star","boost_star",{0.3,0.35,0},1.5,{1,1.5,0},0,1,{0,6.283185307179586,0},3.402823466e+38,0,{0,0,1},0,0,0},
    {"boost_naturalize","boost_naturalize",{0.3,0.35,0},1.5,{1,1.5,0},0,1,{0,6.283185307179586,0},3.402823466e+38,0,{0,0,1},0,0,0},
    {"boost_shuriken","boost_shuriken",{0.3,0.35,0},1.5,{1,1.5,0},0,1,{0,6.283185307179586,0},3.402823466e+38,0,{0,0,1},0,0,0},
    {"boost_club","boost_club",{0.3,0.35,0},1.5,{1,1.5,0},0,1,{0,6.283185307179586,0},3.402823466e+38,0,{0,0,1},0,0,0},
    {"boost_lightning","boost_lightning",{0.3,0.35,0},1.5,{1,1.5,0},0,1,{0,6.283185307179586,0},3.402823466e+38,0,{0,0,1},0,0,0},
    {"boost_hermes","boost_hermes",{0.3,0.35,0},1.5,{1,1.5,0},0,1,{0,6.283185307179586,0},3.402823466e+38,0,{0,0,1},0,0,0},
    {"boost_gearshift_01","boost_gearshift_01",{0.99,1,0},1.5,{1,1.5,0},0,1,{-0.7853981633974483,-0.7853981633974483,1},3.402823466e+38,0,{0,0,1},0,0,0},
    {"boost_gearshift_02","boost_gearshift_02",{0.3,0.35,0},1.5,{1,1.5,0},0,1,{0,6.283185307179586,0},3.402823466e+38,0,{0,0,1},0,0,0},
    {"revive_basic","revive_basic",{0.5,0.55,0},1.5,{1,1.5,0},0,1,{0,0,1},3.402823466e+38,0,{0,0,1},0,0,0},
    {"windwalk","leafStim",{0.1,0.12,0},1.5,{1,1.5,0},0,1,{0,0,1},3.402823466e+38,0,{0,0,1},0,0,0},
    {"takedown","takedownStim",{0.1,0.12,0},1.5,{1,1.5,0},0,1,{0,0,1},3.402823466e+38,0,{0,0,1},0,0,0},
    {"inspire","inspireStim",{0.3,0.35,0},1.5,{1,1.5,0},0,1,{0,0,1},3.402823466e+38,0,{0,0,1},0,0,0},
    {"xp_common","xp_common",{0.3,0.35,0},1.5,{1,1.5,0},0,1,{0,0,1},3.402823466e+38,0,{0,0,1},0,0,0},
    {"xp_rare","xp_rare",{0.3,0.35,0},1.5,{1,1.5,0},0,1,{0,0,1},3.402823466e+38,0,{0,0,1},0,0,0},
    {"xp_mythic","xp_mythic",{0.3,0.35,0},1.5,{1,1.5,0},0,1,{0,0,1},3.402823466e+38,0,{0,0,1},0,0,0},
};

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
            d.randomRotation = r.randomRotation != 0;
            d.hasExplosion = r.hasExplosion != 0;
            d.explosionParticle = r.explosionParticle ? r.explosionParticle : "";
            d.doorSlideOffset = r.doorSlideOffset;
            d.doorCasingSprite = r.doorCasingSprite ? r.doorCasingSprite : "";
            d.doorCasingPos = Vec2(r.doorCasingPx, r.doorCasingPy);
            d.doorCasingScale = r.doorCasingScale;
            d.doorCasingTint = r.doorCasingTint;
            d.doorCasingAlpha = r.doorCasingAlpha;
            d.doorSpriteAnchor = Vec2(r.doorSpriteAnchorX, r.doorSpriteAnchorY);
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
            d.auraSprite = r.auraSprite ? r.auraSprite : "";
            d.auraTint = r.auraTint;
            d.hasAura = r.hasAura != 0;
            _gameObjs[d.type] = d;
        }
        { auto& d = _gameObjs.at("explosion_snowball");


        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={"player-snow-01.img","player-snow-02.img","player-snow-03.img"};






        }
        { auto& d = _gameObjs.at("explosion_snowball_heavy");


        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={"player-snow-01.img","player-snow-02.img","player-snow-03.img"};






        }
        { auto& d = _gameObjs.at("explosion_potato");


        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={"player-mash-01.img","player-mash-02.img","player-mash-03.img"};






        }
        { auto& d = _gameObjs.at("explosion_potato_heavy");


        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={"player-mash-01.img","player-mash-02.img","player-mash-03.img"};






        }
        { auto& d = _gameObjs.at("explosion_potato_smgshot");


        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={"player-mash-01.img","player-mash-02.img","player-mash-03.img"};






        }
        { auto& d = _gameObjs.at("explosion_potato_lmgshot");


        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={"player-mash-01.img","player-mash-02.img","player-mash-03.img"};






        }
        { auto& d = _gameObjs.at("explosion_coconut");


        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={"player-mash-01.img","player-mash-02.img","player-mash-03.img"};






        }
        { auto& d = _gameObjs.at("explosion_tomato");


        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={"player-mash-04.img","player-mash-05.img"};






        }
        { auto& d = _gameObjs.at("backpack00");


        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("backpack01");


        d.ghillie=false; d.isDual=false; d.level=1;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("backpack02");


        d.ghillie=false; d.isDual=false; d.level=2;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("backpack03");


        d.ghillie=false; d.isDual=false; d.level=3;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("backpack04");


        d.ghillie=false; d.isDual=false; d.level=4;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("helmet01");
        d.skin.baseSprite="player-circle-base-01.img";
        d.skin.handL="";
        d.skin.handR="";
        d.skin.footSprite=""; d.skin.backpackSprite="";
        d.skin.baseTint=3244031; d.skin.handTint=16777215;
        d.skin.footTint=16777215; d.skin.backpackTint=16777215;
        d.skin.baseTintRed=10972011;
        d.skin.baseTintBlue=6459582;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=1;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("helmet02");
        d.skin.baseSprite="player-circle-base-01.img";
        d.skin.handL="";
        d.skin.handR="";
        d.skin.footSprite=""; d.skin.backpackSprite="";
        d.skin.baseTint=13027014; d.skin.handTint=16777215;
        d.skin.footTint=16777215; d.skin.backpackTint=16777215;
        d.skin.baseTintRed=10027008;
        d.skin.baseTintBlue=20642;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=2;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("helmet03");
        d.skin.baseSprite="player-circle-base-01.img";
        d.skin.handL="";
        d.skin.handR="";
        d.skin.footSprite=""; d.skin.backpackSprite="";
        d.skin.baseTint=2434341; d.skin.handTint=16777215;
        d.skin.footTint=16777215; d.skin.backpackTint=16777215;
        d.skin.baseTintRed=2491396;
        d.skin.baseTintBlue=334125;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=3;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("helmet04");
        d.skin.baseSprite="player-circle-base-01.img";
        d.skin.handL="";
        d.skin.handR="";
        d.skin.footSprite=""; d.skin.backpackSprite="";
        d.skin.baseTint=2434341; d.skin.handTint=16777215;
        d.skin.footTint=16777215; d.skin.backpackTint=16777215;
        d.skin.baseTintRed=2491396;
        d.skin.baseTintBlue=334125;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=4;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("chest01");
        d.skin.baseSprite="player-armor-base-01.img";
        d.skin.handL="";
        d.skin.handR="";
        d.skin.footSprite=""; d.skin.backpackSprite="";
        d.skin.baseTint=11842740; d.skin.handTint=16777215;
        d.skin.footTint=16777215; d.skin.backpackTint=16777215;
        d.skin.baseTintRed=11842740;
        d.skin.baseTintBlue=11842740;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=1;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("chest02");
        d.skin.baseSprite="player-armor-base-01.img";
        d.skin.handL="";
        d.skin.handR="";
        d.skin.footSprite=""; d.skin.backpackSprite="";
        d.skin.baseTint=4934475; d.skin.handTint=16777215;
        d.skin.footTint=16777215; d.skin.backpackTint=16777215;
        d.skin.baseTintRed=4934475;
        d.skin.baseTintBlue=4934475;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=2;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("chest03");
        d.skin.baseSprite="player-armor-base-01.img";
        d.skin.handL="";
        d.skin.handR="";
        d.skin.footSprite=""; d.skin.backpackSprite="";
        d.skin.baseTint=0; d.skin.handTint=16777215;
        d.skin.footTint=16777215; d.skin.backpackTint=16777215;
        d.skin.baseTintRed=0;
        d.skin.baseTintBlue=0;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=3;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("chest04");
        d.skin.baseSprite="player-armor-base-01.img";
        d.skin.handL="";
        d.skin.handR="";
        d.skin.footSprite=""; d.skin.backpackSprite="";
        d.skin.baseTint=1846790; d.skin.handTint=16777215;
        d.skin.footTint=16777215; d.skin.backpackTint=16777215;
        d.skin.baseTintRed=1846790;
        d.skin.baseTintBlue=1846790;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=4;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("helmet03_leader");
        d.skin.baseSprite="player-helmet-leader.img";
        d.skin.handL="";
        d.skin.handR="";
        d.skin.footSprite=""; d.skin.backpackSprite="";
        d.skin.baseTint=16777215; d.skin.handTint=16777215;
        d.skin.footTint=16777215; d.skin.backpackTint=16777215;
        d.skin.baseTintRed=16777215;
        d.skin.baseTintBlue=16777215;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=3;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("helmet03_forest");
        d.skin.baseSprite="player-helmet-forest.img";
        d.skin.handL="";
        d.skin.handR="";
        d.skin.footSprite=""; d.skin.backpackSprite="";
        d.skin.baseTint=16777215; d.skin.handTint=16777215;
        d.skin.footTint=16777215; d.skin.backpackTint=16777215;
        d.skin.baseTintRed=16777215;
        d.skin.baseTintBlue=16777215;
        d.skin.spriteScale=0.3f;

        d.ghillie=false; d.isDual=false; d.level=3;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("helmet03_moon");
        d.skin.baseSprite="player-helmet-moon.img";
        d.skin.handL="";
        d.skin.handR="";
        d.skin.footSprite=""; d.skin.backpackSprite="";
        d.skin.baseTint=16777215; d.skin.handTint=16777215;
        d.skin.footTint=16777215; d.skin.backpackTint=16777215;
        d.skin.baseTintRed=16777215;
        d.skin.baseTintBlue=16777215;
        d.skin.spriteScale=0.3f;

        d.ghillie=false; d.isDual=false; d.level=3;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("helmet03_lt");
        d.skin.baseSprite="player-helmet-lieutenant.img";
        d.skin.handL="";
        d.skin.handR="";
        d.skin.footSprite=""; d.skin.backpackSprite="";
        d.skin.baseTint=16777215; d.skin.handTint=16777215;
        d.skin.footTint=16777215; d.skin.backpackTint=16777215;
        d.skin.baseTintRed=16777215;
        d.skin.baseTintBlue=16777215;
        d.skin.spriteScale=0.3f;

        d.ghillie=false; d.isDual=false; d.level=3;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("helmet03_lt_aged");
        d.skin.baseSprite="player-helmet-lieutenant.img";
        d.skin.handL="";
        d.skin.handR="";
        d.skin.footSprite=""; d.skin.backpackSprite="";
        d.skin.baseTint=16777215; d.skin.handTint=16777215;
        d.skin.footTint=16777215; d.skin.backpackTint=16777215;
        d.skin.baseTintRed=16777215;
        d.skin.baseTintBlue=16777215;
        d.skin.spriteScale=0.3f;

        d.ghillie=false; d.isDual=false; d.level=3;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("helmet03_potato");
        d.skin.baseSprite="player-helmet-potato.img";
        d.skin.handL="";
        d.skin.handR="";
        d.skin.footSprite=""; d.skin.backpackSprite="";
        d.skin.baseTint=16777215; d.skin.handTint=16777215;
        d.skin.footTint=16777215; d.skin.backpackTint=16777215;
        d.skin.baseTintRed=16777215;
        d.skin.baseTintBlue=16777215;
        d.skin.spriteScale=0.3f;

        d.ghillie=false; d.isDual=false; d.level=3;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("helmet03_marksman");
        d.skin.baseSprite="player-helmet-marksman.img";
        d.skin.handL="";
        d.skin.handR="";
        d.skin.footSprite=""; d.skin.backpackSprite="";
        d.skin.baseTint=16777215; d.skin.handTint=16777215;
        d.skin.footTint=16777215; d.skin.backpackTint=16777215;
        d.skin.baseTintRed=16777215;
        d.skin.baseTintBlue=16777215;
        d.skin.spriteScale=0.3f;

        d.ghillie=false; d.isDual=false; d.level=3;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("helmet03_recon");
        d.skin.baseSprite="player-helmet-recon.img";
        d.skin.handL="";
        d.skin.handR="";
        d.skin.footSprite=""; d.skin.backpackSprite="";
        d.skin.baseTint=16777215; d.skin.handTint=16777215;
        d.skin.footTint=16777215; d.skin.backpackTint=16777215;
        d.skin.baseTintRed=16777215;
        d.skin.baseTintBlue=16777215;
        d.skin.spriteScale=0.3f;

        d.ghillie=false; d.isDual=false; d.level=3;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("helmet03_grenadier");
        d.skin.baseSprite="player-helmet-grenadier.img";
        d.skin.handL="";
        d.skin.handR="";
        d.skin.footSprite=""; d.skin.backpackSprite="";
        d.skin.baseTint=16777215; d.skin.handTint=16777215;
        d.skin.footTint=16777215; d.skin.backpackTint=16777215;
        d.skin.baseTintRed=16777215;
        d.skin.baseTintBlue=16777215;
        d.skin.spriteScale=0.3f;

        d.ghillie=false; d.isDual=false; d.level=3;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("helmet03_bugler");
        d.skin.baseSprite="player-helmet-bugler.img";
        d.skin.handL="";
        d.skin.handR="";
        d.skin.footSprite=""; d.skin.backpackSprite="";
        d.skin.baseTint=16777215; d.skin.handTint=16777215;
        d.skin.footTint=16777215; d.skin.backpackTint=16777215;
        d.skin.baseTintRed=16777215;
        d.skin.baseTintBlue=16777215;
        d.skin.spriteScale=0.3f;

        d.ghillie=false; d.isDual=false; d.level=3;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("helmet04_medic");
        d.skin.baseSprite="player-helmet-medic.img";
        d.skin.handL="";
        d.skin.handR="";
        d.skin.footSprite=""; d.skin.backpackSprite="";
        d.skin.baseTint=16777215; d.skin.handTint=16777215;
        d.skin.footTint=16777215; d.skin.backpackTint=16777215;
        d.skin.baseTintRed=16777215;
        d.skin.baseTintBlue=16777215;
        d.skin.spriteScale=0.3f;

        d.ghillie=false; d.isDual=false; d.level=4;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("helmet04_last_man_red");
        d.skin.baseSprite="player-helmet-last-man-01.img";
        d.skin.handL="";
        d.skin.handR="";
        d.skin.footSprite=""; d.skin.backpackSprite="";
        d.skin.baseTint=16777215; d.skin.handTint=16777215;
        d.skin.footTint=16777215; d.skin.backpackTint=16777215;
        d.skin.baseTintRed=16777215;
        d.skin.baseTintBlue=16777215;
        d.skin.spriteScale=0.3f;

        d.ghillie=false; d.isDual=false; d.level=4;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("helmet04_last_man_blue");
        d.skin.baseSprite="player-helmet-last-man-02.img";
        d.skin.handL="";
        d.skin.handR="";
        d.skin.footSprite=""; d.skin.backpackSprite="";
        d.skin.baseTint=16777215; d.skin.handTint=16777215;
        d.skin.footTint=16777215; d.skin.backpackTint=16777215;
        d.skin.baseTintRed=16777215;
        d.skin.baseTintBlue=16777215;
        d.skin.spriteScale=0.3f;

        d.ghillie=false; d.isDual=false; d.level=4;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("helmet04_leader");
        d.skin.baseSprite="player-helmet-leader.img";
        d.skin.handL="";
        d.skin.handR="";
        d.skin.footSprite=""; d.skin.backpackSprite="";
        d.skin.baseTint=16777215; d.skin.handTint=16777215;
        d.skin.footTint=16777215; d.skin.backpackTint=16777215;
        d.skin.baseTintRed=16777215;
        d.skin.baseTintBlue=16777215;
        d.skin.spriteScale=0.3f;

        d.ghillie=false; d.isDual=false; d.level=4;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("helmet04_captain");
        d.skin.baseSprite="player-helmet-captain.img";
        d.skin.handL="";
        d.skin.handR="";
        d.skin.footSprite=""; d.skin.backpackSprite="";
        d.skin.baseTint=16777215; d.skin.handTint=16777215;
        d.skin.footTint=16777215; d.skin.backpackTint=16777215;
        d.skin.baseTintRed=16777215;
        d.skin.baseTintBlue=16777215;
        d.skin.spriteScale=0.3f;

        d.ghillie=false; d.isDual=false; d.level=4;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("helmet04_classless");
        d.skin.baseSprite="player-helmet-classless.img";
        d.skin.handL="";
        d.skin.handR="";
        d.skin.footSprite=""; d.skin.backpackSprite="";
        d.skin.baseTint=16777215; d.skin.handTint=16777215;
        d.skin.footTint=16777215; d.skin.backpackTint=16777215;
        d.skin.baseTintRed=16777215;
        d.skin.baseTintBlue=16777215;
        d.skin.spriteScale=0.275f;

        d.ghillie=false; d.isDual=false; d.level=4;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("backpack04_cloud");


        d.ghillie=false; d.isDual=false; d.level=4;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("mp5");

        d.worldImg.sprite="gun-med-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.49f); d.worldImg.rot=0.0f; d.worldImg.tint=1184274;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=1.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("mac10");

        d.worldImg.sprite="gun-med-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.44f); d.worldImg.rot=0.0f; d.worldImg.tint=3684408;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(1.4f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=1.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("ump9");

        d.worldImg.sprite="gun-med-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.515f); d.worldImg.rot=0.0f; d.worldImg.tint=1184274;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(5.6f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=1.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("vector");

        d.worldImg.sprite="gun-vector-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(7.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=0.89f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("vector45");

        d.worldImg.sprite="gun-vector45-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(7.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=0.89f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("scorpion");

        d.worldImg.sprite="gun-scorpion-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(8.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=1.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("vss");

        d.worldImg.sprite="gun-vss-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(9.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=1.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("famas");

        d.worldImg.sprite="gun-famas-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(-8.0f,0.0f); d.leftHandOffset=Vec2(12.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="bullpup";
        d.recoil=1.33f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("hk416");

        d.worldImg.sprite="gun-med-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.52f); d.worldImg.rot=0.0f; d.worldImg.tint=14402714;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(4.2f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=1.33f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("m4a1");

        d.worldImg.sprite="gun-m4a1-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(7.9f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=1.3f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("mk12");

        d.worldImg.sprite="gun-long-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.485f); d.worldImg.rot=0.0f; d.worldImg.tint=10984586;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(4.2f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=1.66f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("l86");

        d.worldImg.sprite="gun-l86-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(2.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=1.66f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("m249");

        d.worldImg.sprite="gun-m249-top-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(13.2f,0.0f);
        d.magSprite="gun-m249-bot-01.img"; d.magPos=Vec2(0.0f,-20.5f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=1.33f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("qbb97");

        d.worldImg.sprite="gun-qbb97-top-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(-8.0f,0.0f); d.leftHandOffset=Vec2(12.0f,0.0f);
        d.magSprite="gun-qbb97-bot-01.img"; d.magPos=Vec2(-1.5f,-7.5f);
        d.magTop=false; d.idlePose="bullpup";
        d.recoil=1.33f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("scout_elite");

        d.worldImg.sprite="gun-scout_elite-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(6.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=2.33f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("ak47");

        d.worldImg.sprite="gun-long-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.435f); d.worldImg.rot=0.0f; d.worldImg.tint=6433298;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(2.8f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=1.33f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("scar");

        d.worldImg.sprite="gun-scar-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(2.8f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=1.33f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("scarssr");

        d.worldImg.sprite="gun-scarssr-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(6.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=1.33f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("an94");

        d.worldImg.sprite="gun-an94-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(9.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=1.33f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("groza");

        d.worldImg.sprite="gun-groza-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(-8.0f,0.0f); d.leftHandOffset=Vec2(12.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="bullpup";
        d.recoil=1.4f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("grozas");

        d.worldImg.sprite="gun-grozas-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(-8.0f,0.0f); d.leftHandOffset=Vec2(12.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="bullpup";
        d.recoil=1.4f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("dp28");

        d.worldImg.sprite="gun-dp28-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(7.0f,0.0f);
        d.magSprite="gun-dp28-top-01.img"; d.magPos=Vec2(0.0f,-20.5f);
        d.magTop=true; d.idlePose="rifle";
        d.recoil=1.33f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("bar");

        d.worldImg.sprite="gun-bar-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(6.8f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=1.4f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("imbel");

        d.worldImg.sprite="gun-imbel-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(10.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=1.4f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("pkp");

        d.worldImg.sprite="gun-pkp-top-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(12.5f,0.0f);
        d.magSprite="gun-pkp-bot-01.img"; d.magPos=Vec2(0.0f,-17.5f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=1.33f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("model94");

        d.worldImg.sprite="gun-model94-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(3.2f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=2.33f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("mkg45");

        d.worldImg.sprite="gun-mkg45-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(4.2f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=1.66f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("blr");

        d.worldImg.sprite="gun-blr-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(6.4f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=2.75f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("mosin");

        d.worldImg.sprite="gun-mosin-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(7.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=2.33f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("sv98");

        d.worldImg.sprite="gun-sv98-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(6.5f,0.25f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=2.33f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("awc");

        d.worldImg.sprite="gun-awc-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(11.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=2.66f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("m39");

        d.worldImg.sprite="gun-long-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.4925f); d.worldImg.rot=0.0f; d.worldImg.tint=3355443;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(2.8f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=1.66f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("svd");

        d.worldImg.sprite="gun-svd-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(8.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=2.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("garand");

        d.worldImg.sprite="gun-garand-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(8.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=1.66f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("m870");

        d.worldImg.sprite="gun-long-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.435f); d.worldImg.rot=0.0f; d.worldImg.tint=3348992;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(7.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=1.33f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("m1100");

        d.worldImg.sprite="gun-m1100-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(7.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=1.33f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("mp220");

        d.worldImg.sprite="gun-mp220-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=1.33f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("saiga");

        d.worldImg.sprite="gun-saiga-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(8.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=1.33f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("spas12");

        d.worldImg.sprite="gun-spas12-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(4.9f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=1.33f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("spas16");

        d.worldImg.sprite="gun-spas16-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(6.5f,0.5f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=1.33f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("m1014");

        d.worldImg.sprite="gun-m1014-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(7.8f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=1.33f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("usas");

        d.worldImg.sprite="gun-usas-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(16.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=1.5f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("m9");

        d.worldImg.sprite="gun-short-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.505f); d.worldImg.rot=0.0f; d.worldImg.tint=1973790;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="pistol";
        d.recoil=1.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("m9_dual");

        d.worldImg.sprite="gun-short-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.505f); d.worldImg.rot=0.0f; d.worldImg.tint=1973790;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=true; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="dualPistol";
        d.recoil=1.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("m9_cursed");

        d.worldImg.sprite="gun-short-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.505f); d.worldImg.rot=0.0f; d.worldImg.tint=1973790;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="pistol";
        d.recoil=1.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("m93r");

        d.worldImg.sprite="gun-short-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.51f); d.worldImg.rot=0.0f; d.worldImg.tint=2766875;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.8f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=0.5f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("m93r_dual");

        d.worldImg.sprite="gun-short-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.51f); d.worldImg.rot=0.0f; d.worldImg.tint=2766875;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=true; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="dualPistol";
        d.recoil=1.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("glock");

        d.worldImg.sprite="gun-short-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.49f); d.worldImg.rot=0.0f; d.worldImg.tint=1973790;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="pistol";
        d.recoil=1.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("glock_dual");

        d.worldImg.sprite="gun-short-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.49f); d.worldImg.rot=0.0f; d.worldImg.tint=1973790;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=true; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="dualPistol";
        d.recoil=1.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("p30l");

        d.worldImg.sprite="gun-p30l-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="pistol";
        d.recoil=1.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("p30l_dual");

        d.worldImg.sprite="gun-p30l-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=true; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="dualPistol";
        d.recoil=1.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("ot38");

        d.worldImg.sprite="gun-short-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.4625f); d.worldImg.rot=0.0f; d.worldImg.tint=7368816;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="pistol";
        d.recoil=1.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("ot38_dual");

        d.worldImg.sprite="gun-short-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.4625f); d.worldImg.rot=0.0f; d.worldImg.tint=7368816;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=true; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="dualPistol";
        d.recoil=1.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("ots38");

        d.worldImg.sprite="gun-ots38-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="pistol";
        d.recoil=1.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("ots38_dual");

        d.worldImg.sprite="gun-ots38-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=true; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="dualPistol";
        d.recoil=1.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("colt45");

        d.worldImg.sprite="gun-colt45-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="pistol";
        d.recoil=1.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("colt45_dual");

        d.worldImg.sprite="gun-colt45-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=true; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="dualPistol";
        d.recoil=1.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("m1911");

        d.worldImg.sprite="gun-short-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=9605778;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="pistol";
        d.recoil=1.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("m1911_dual");

        d.worldImg.sprite="gun-short-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=9605778;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=true; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="dualPistol";
        d.recoil=1.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("m1a1");

        d.worldImg.sprite="gun-med-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.53f); d.worldImg.rot=0.0f; d.worldImg.tint=3674112;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(5.8f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=1.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("deagle");

        d.worldImg.sprite="gun-deagle-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="pistol";
        d.recoil=1.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("deagle_dual");

        d.worldImg.sprite="gun-deagle-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=true; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="dualPistol";
        d.recoil=1.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("barrett");

        d.worldImg.sprite="gun-barrett-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(10.0f,0.75f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=5.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("sw500");

        d.worldImg.sprite="gun-sw500-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="pistol";
        d.recoil=1.5f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("ash12");

        d.worldImg.sprite="gun-ash12-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(7.0f,0.5f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=2.5f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("flare_gun");

        d.worldImg.sprite="gun-flare-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="pistol";
        d.recoil=1.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("flare_gun_dual");

        d.worldImg.sprite="gun-flare-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=true; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="dualPistol";
        d.recoil=1.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("potato_cannon");

        d.worldImg.sprite="gun-potato-cannon-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=true;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(-10.0f,-4.0f); d.leftHandOffset=Vec2(7.0f,2.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="launcher";
        d.recoil=8.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("potato_smg");

        d.worldImg.sprite="gun-potato-smg-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite="gun-potato-smg-top-01.img"; d.magPos=Vec2(0.0f,-15.0f);
        d.magTop=true; d.idlePose="rifle";
        d.recoil=2.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("potato_lmg");

        d.worldImg.sprite="gun-potato-lmg-top-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(-40.0f,1.75f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="minigun";
        d.recoil=1.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("bugle");

        d.worldImg.sprite="gun-bugle-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(12.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="pistol";
        d.recoil=4.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("svd_winter");

        d.worldImg.sprite="gun-svd-02.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(8.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=2.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("sv98_winter");

        d.worldImg.sprite="gun-sv98-02.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(6.5f,0.25f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=2.33f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("awc_winter");

        d.worldImg.sprite="gun-awc-02.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.5f,0.5f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(11.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="rifle";
        d.recoil=2.66f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("fists");


        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={"fists"};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("knuckles");

        d.worldImg.sprite="loot-melee-knuckles-rusted.img"; d.worldImg.pos=Vec2(0.0f,-27.0f);
        d.worldImg.scale=Vec2(0.2f,0.2f); d.worldImg.rot=1.5707963267948966f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={"fists","fists"};
        d.deployAnims={"knuckles_spin","knuckles_slam"};
        d.idleAnims={"knuckles_bash"};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("karambit");

        d.worldImg.sprite="loot-melee-karambit-rugged.img"; d.worldImg.pos=Vec2(15.5f,-5.0f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=1.5707963267948966f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="slash";
        d.recoil=0.0f;
        d.attackAnims={"slash","stab"};
        d.deployAnims={"karambit_spin","karambit_rapidSpin"};
        d.idleAnims={"karambit_frontSpin","karambit_backSpin"};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("bayonet");

        d.worldImg.sprite="loot-melee-bayonet-rugged.img"; d.worldImg.pos=Vec2(-0.5f,-32.5f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=0.785f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={"cut","thrust"};
        d.deployAnims={"bayonet_unsheathe"};
        d.idleAnims={"knife_inspect"};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("huntsman");

        d.worldImg.sprite="loot-melee-huntsman-rugged.img"; d.worldImg.pos=Vec2(2.5f,-35.5f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=0.82f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={"cut","thrust"};
        d.deployAnims={"huntsman_catch"};
        d.idleAnims={"knife_inspect"};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("bowie");

        d.worldImg.sprite="loot-melee-bowie-vintage.img"; d.worldImg.pos=Vec2(-0.5f,-32.5f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=0.785f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={"cut","thrust"};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("machete");

        d.worldImg.sprite="loot-melee-machete-taiga.img"; d.worldImg.pos=Vec2(-2.5f,-48.5f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=1.885f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="machete";
        d.recoil=0.0f;
        d.attackAnims={"cutReverse"};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("saw");

        d.worldImg.sprite="loot-melee-bonesaw-rusted.img"; d.worldImg.pos=Vec2(-2.5f,-48.5f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=1.885f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="machete";
        d.recoil=0.0f;
        d.attackAnims={"sawSwing"};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("woodaxe");

        d.worldImg.sprite="loot-melee-woodaxe.img"; d.worldImg.pos=Vec2(-12.5f,-16.0f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=1.2f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=true;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="meleeTwoHanded";
        d.recoil=0.0f;
        d.attackAnims={"axeSwing"};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("fireaxe");

        d.worldImg.sprite="loot-melee-fireaxe.img"; d.worldImg.pos=Vec2(-12.5f,-4.0f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=1.2f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=true;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="meleeTwoHanded";
        d.recoil=0.0f;
        d.attackAnims={"axeSwing"};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("katana");

        d.worldImg.sprite="loot-melee-katana.img"; d.worldImg.pos=Vec2(52.5f,-2.0f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=3.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=true;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="meleeKatana";
        d.recoil=0.0f;
        d.attackAnims={"katanaSwing"};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("naginata");

        d.worldImg.sprite="loot-melee-naginata.img"; d.worldImg.pos=Vec2(42.5f,-3.0f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=1.9f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=true;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="meleeNaginata";
        d.recoil=0.0f;
        d.attackAnims={"naginataSwing"};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("stonehammer");

        d.worldImg.sprite="loot-melee-stonehammer.img"; d.worldImg.pos=Vec2(-12.5f,-4.0f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=1.2f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=true;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="meleeTwoHanded";
        d.recoil=0.0f;
        d.attackAnims={"hammerSwing"};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("iceaxe");

        d.worldImg.sprite="loot-melee-ice_pick.img"; d.worldImg.pos=Vec2(-12.5f,-10.0f);
        d.worldImg.scale=Vec2(0.4f,0.4f); d.worldImg.rot=1.2f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=true;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="meleeTwoHanded";
        d.recoil=0.0f;
        d.attackAnims={"axeSwing"};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("hook");

        d.worldImg.sprite="loot-melee-hook-silver.img"; d.worldImg.pos=Vec2(0.0f,-27.0f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=1.5707963267948966f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=true; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={"hook"};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("pan");

        d.worldImg.sprite="loot-melee-pan-black-side.img"; d.worldImg.pos=Vec2(0.0f,-40.0f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=1.125f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false; d.hipImg.sprite="loot-melee-pan-black-side.img"; d.hipImg.pos=Vec2(-17.25f,7.5f);
        d.hipImg.scale=Vec2(0.3f,0.3f); d.hipImg.rot=2.4504422698000385f; d.hipImg.tint=16777215;
        d.hipImg.renderOnHand=false; d.hipImg.leftHandOnTop=false;
        d.hipImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={"pan"};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("spade");

        d.worldImg.sprite="loot-melee-spade-assault.img"; d.worldImg.pos=Vec2(-0.5f,-41.5f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=1.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={"cut","thrust"};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("crowbar");

        d.worldImg.sprite="loot-melee-crowbar.img"; d.worldImg.pos=Vec2(-1.0f,-10.0f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=1.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={"cut","cutReverseShort"};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("cutlass");

        d.worldImg.sprite="loot-melee-cutlass.img"; d.worldImg.pos=Vec2(2.5f,-75.0f);
        d.worldImg.scale=Vec2(0.325f,0.325f); d.worldImg.rot=1.885f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="cutlass";
        d.recoil=0.0f;
        d.attackAnims={"cut","cutReverse"};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("knuckles_rusted");

        d.worldImg.sprite="loot-melee-knuckles-rusted.img"; d.worldImg.pos=Vec2(0.0f,-27.0f);
        d.worldImg.scale=Vec2(0.2f,0.2f); d.worldImg.rot=1.5707963267948966f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={"fists","fists"};
        d.deployAnims={"knuckles_spin","knuckles_slam"};
        d.idleAnims={"knuckles_bash"};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("knuckles_heroic");

        d.worldImg.sprite="loot-melee-knuckles-heroic.img"; d.worldImg.pos=Vec2(0.0f,-27.0f);
        d.worldImg.scale=Vec2(0.2f,0.2f); d.worldImg.rot=1.5707963267948966f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={"fists","fists"};
        d.deployAnims={"knuckles_spin","knuckles_slam"};
        d.idleAnims={"knuckles_bash"};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("karambit_borealis");

        d.worldImg.sprite="loot-melee-karambit-borealis.img"; d.worldImg.pos=Vec2(15.5f,-5.0f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=1.5707963267948966f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="slash";
        d.recoil=0.0f;
        d.attackAnims={"slash","stab"};
        d.deployAnims={"karambit_spin","karambit_rapidSpin"};
        d.idleAnims={"karambit_frontSpin","karambit_backSpin"};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("karambit_rugged");

        d.worldImg.sprite="loot-melee-karambit-rugged.img"; d.worldImg.pos=Vec2(15.5f,-5.0f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=1.5707963267948966f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="slash";
        d.recoil=0.0f;
        d.attackAnims={"slash","stab"};
        d.deployAnims={"karambit_spin","karambit_rapidSpin"};
        d.idleAnims={"karambit_frontSpin","karambit_backSpin"};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("karambit_prismatic");

        d.worldImg.sprite="loot-melee-karambit-prismatic.img"; d.worldImg.pos=Vec2(15.5f,-5.0f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=1.5707963267948966f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="slash";
        d.recoil=0.0f;
        d.attackAnims={"slash","stab"};
        d.deployAnims={"karambit_spin","karambit_rapidSpin"};
        d.idleAnims={"karambit_frontSpin","karambit_backSpin"};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("karambit_drowned");

        d.worldImg.sprite="loot-melee-karambit-drowned.img"; d.worldImg.pos=Vec2(15.5f,-5.0f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=1.5707963267948966f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="slash";
        d.recoil=0.0f;
        d.attackAnims={"slash","stab"};
        d.deployAnims={"karambit_spin","karambit_rapidSpin"};
        d.idleAnims={"karambit_frontSpin","karambit_backSpin"};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("bayonet_rugged");

        d.worldImg.sprite="loot-melee-bayonet-rugged.img"; d.worldImg.pos=Vec2(-0.5f,-32.5f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=0.785f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={"cut","thrust"};
        d.deployAnims={"bayonet_unsheathe"};
        d.idleAnims={"knife_inspect"};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("bayonet_woodland");

        d.worldImg.sprite="loot-melee-bayonet-woodland.img"; d.worldImg.pos=Vec2(-0.5f,-32.5f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=0.785f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={"cut","thrust"};
        d.deployAnims={"bayonet_unsheathe"};
        d.idleAnims={"knife_inspect"};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("huntsman_rugged");

        d.worldImg.sprite="loot-melee-huntsman-rugged.img"; d.worldImg.pos=Vec2(2.5f,-35.5f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=0.82f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={"cut","thrust"};
        d.deployAnims={"huntsman_catch"};
        d.idleAnims={"knife_inspect"};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("huntsman_burnished");

        d.worldImg.sprite="loot-melee-huntsman-burnished.img"; d.worldImg.pos=Vec2(2.5f,-35.5f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=0.82f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={"cut","thrust"};
        d.deployAnims={"huntsman_catch"};
        d.idleAnims={"knife_inspect"};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("bowie_vintage");

        d.worldImg.sprite="loot-melee-bowie-vintage.img"; d.worldImg.pos=Vec2(-0.5f,-32.5f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=0.785f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={"cut","thrust"};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("bowie_frontier");

        d.worldImg.sprite="loot-melee-bowie-frontier.img"; d.worldImg.pos=Vec2(-0.5f,-32.5f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=0.785f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={"cut","thrust"};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("machete_taiga");

        d.worldImg.sprite="loot-melee-machete-taiga.img"; d.worldImg.pos=Vec2(-2.5f,-48.5f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=1.885f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="machete";
        d.recoil=0.0f;
        d.attackAnims={"cutReverse"};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("kukri_trad");

        d.worldImg.sprite="loot-melee-kukri-trad.img"; d.worldImg.pos=Vec2(-0.5f,-46.5f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=1.885f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="machete";
        d.recoil=0.0f;
        d.attackAnims={"cutReverse"};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("bonesaw_rusted");

        d.worldImg.sprite="loot-melee-bonesaw-rusted.img"; d.worldImg.pos=Vec2(-2.5f,-48.5f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=1.885f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="machete";
        d.recoil=0.0f;
        d.attackAnims={"sawSwing"};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("woodaxe_bloody");

        d.worldImg.sprite="loot-melee-woodaxe-bloody.img"; d.worldImg.pos=Vec2(-12.5f,-16.0f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=1.2f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=true;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="meleeTwoHanded";
        d.recoil=0.0f;
        d.attackAnims={"axeSwing"};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("katana_rusted");

        d.worldImg.sprite="loot-melee-katana-rusted.img"; d.worldImg.pos=Vec2(52.5f,-2.0f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=3.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=true;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="meleeKatana";
        d.recoil=0.0f;
        d.attackAnims={"katanaSwing"};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("katana_orchid");

        d.worldImg.sprite="loot-melee-katana-orchid.img"; d.worldImg.pos=Vec2(52.5f,-2.0f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=3.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=true;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="meleeKatana";
        d.recoil=0.0f;
        d.attackAnims={"katanaSwing"};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("sledgehammer");

        d.worldImg.sprite="loot-melee-sledgehammer.img"; d.worldImg.pos=Vec2(-12.5f,-3.5f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=1.2f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=true;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="meleeTwoHanded";
        d.recoil=0.0f;
        d.attackAnims={"hammerSwing"};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("crowbar_scout");

        d.worldImg.sprite="loot-melee-crowbar-scout.img"; d.worldImg.pos=Vec2(-1.0f,-10.0f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=1.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={"cut","cutReverseShort"};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("crowbar_recon");

        d.worldImg.sprite="loot-melee-crowbar-recon.img"; d.worldImg.pos=Vec2(-1.0f,-10.0f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=1.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={"cut","cutReverseShort"};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("kukri_sniper");

        d.worldImg.sprite="loot-melee-kukri-sniper.img"; d.worldImg.pos=Vec2(-0.5f,-46.5f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=1.885f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="machete";
        d.recoil=0.0f;
        d.attackAnims={"cutReverse"};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("bonesaw_healer");

        d.worldImg.sprite="loot-melee-bonesaw-healer.img"; d.worldImg.pos=Vec2(-2.5f,-48.5f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=1.885f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="machete";
        d.recoil=0.0f;
        d.attackAnims={"sawSwing"};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("katana_demo");

        d.worldImg.sprite="loot-melee-katana-demo.img"; d.worldImg.pos=Vec2(52.5f,-2.0f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=3.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=true;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="meleeKatana";
        d.recoil=0.0f;
        d.attackAnims={"katanaSwing"};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("spade_assault");

        d.worldImg.sprite="loot-melee-spade-assault.img"; d.worldImg.pos=Vec2(-0.5f,-41.5f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=1.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={"cut","thrust"};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("warhammer_tank");

        d.worldImg.sprite="loot-melee-warhammer-tank.img"; d.worldImg.pos=Vec2(-10.5f,-3.0f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=1.2f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=true;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="meleeTwoHanded";
        d.recoil=0.0f;
        d.attackAnims={"hammerSwing"};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("naginata_daemon");

        d.worldImg.sprite="loot-melee-naginata-daemon.img"; d.worldImg.pos=Vec2(42.5f,-3.0f);
        d.worldImg.scale=Vec2(0.35f,0.35f); d.worldImg.rot=1.9f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=true;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="meleeNaginata";
        d.recoil=0.0f;
        d.attackAnims={"naginataSwing"};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("cutlass_gold");

        d.worldImg.sprite="loot-melee-cutlass-gold.img"; d.worldImg.pos=Vec2(2.5f,-75.0f);
        d.worldImg.scale=Vec2(0.325f,0.325f); d.worldImg.rot=1.885f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="cutlass";
        d.recoil=0.0f;
        d.attackAnims={"cut","cutReverse"};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitBase");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=16303476; d.skin.handTint=16303476;
        d.skin.footTint=16303476; d.skin.backpackTint=8480055;
        d.skin.baseTintRed=16303476;
        d.skin.baseTintBlue=16303476;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitDemo");
        d.skin.baseSprite="player-base-02.img";
        d.skin.handL="player-hands-02.img";
        d.skin.handR="player-hands-02.img";
        d.skin.footSprite="player-feet-02.img"; d.skin.backpackSprite="player-circle-base-02.img";
        d.skin.baseTint=13068903; d.skin.handTint=11882573;
        d.skin.footTint=11882573; d.skin.backpackTint=10368820;
        d.skin.baseTintRed=13068903;
        d.skin.baseTintBlue=13068903;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitTank");
        d.skin.baseSprite="player-base-02.img";
        d.skin.handL="player-hands-02.img";
        d.skin.handR="player-hands-02.img";
        d.skin.footSprite="player-feet-02.img"; d.skin.backpackSprite="player-circle-base-02.img";
        d.skin.baseTint=15382883; d.skin.handTint=14197835;
        d.skin.footTint=14197835; d.skin.backpackTint=12553007;
        d.skin.baseTintRed=15382883;
        d.skin.baseTintBlue=15382883;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitMedic");
        d.skin.baseSprite="player-base-02.img";
        d.skin.handL="player-hands-02.img";
        d.skin.handR="player-hands-02.img";
        d.skin.footSprite="player-feet-02.img"; d.skin.backpackSprite="player-circle-base-02.img";
        d.skin.baseTint=14449116; d.skin.handTint=12866756;
        d.skin.footTint=12866756; d.skin.backpackTint=11089833;
        d.skin.baseTintRed=14449116;
        d.skin.baseTintBlue=14449116;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitScout");
        d.skin.baseSprite="player-base-02.img";
        d.skin.handL="player-hands-02.img";
        d.skin.handR="player-hands-02.img";
        d.skin.footSprite="player-feet-02.img"; d.skin.backpackSprite="player-circle-base-02.img";
        d.skin.baseTint=11326819; d.skin.handTint=9880138;
        d.skin.footTint=9880138; d.skin.backpackTint=8630324;
        d.skin.baseTintRed=11326819;
        d.skin.baseTintBlue=11326819;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitSniper");
        d.skin.baseSprite="player-base-02.img";
        d.skin.handL="player-hands-02.img";
        d.skin.handR="player-hands-02.img";
        d.skin.footSprite="player-feet-02.img"; d.skin.backpackSprite="player-circle-base-02.img";
        d.skin.baseTint=9293531; d.skin.handTint=7387849;
        d.skin.footTint=7387849; d.skin.backpackTint=5415860;
        d.skin.baseTintRed=9293531;
        d.skin.baseTintBlue=9293531;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitAssault");
        d.skin.baseSprite="player-base-02.img";
        d.skin.handL="player-hands-02.img";
        d.skin.handR="player-hands-02.img";
        d.skin.footSprite="player-feet-02.img"; d.skin.backpackSprite="player-circle-base-02.img";
        d.skin.baseTint=14339929; d.skin.handTint=13024064;
        d.skin.footTint=13024064; d.skin.backpackTint=10918952;
        d.skin.baseTintRed=14339929;
        d.skin.baseTintBlue=14339929;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitClassless");
        d.skin.baseSprite="player-base-02.img";
        d.skin.handL="player-hands-02.img";
        d.skin.handR="player-hands-02.img";
        d.skin.footSprite="player-feet-02.img"; d.skin.backpackSprite="player-circle-base-02.img";
        d.skin.baseTint=6579300; d.skin.handTint=5263440;
        d.skin.footTint=5263440; d.skin.backpackTint=3289650;
        d.skin.baseTintRed=6579300;
        d.skin.baseTintBlue=6579300;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitTurkey");
        d.skin.baseSprite="player-base-outfitTurkey.img";
        d.skin.handL="player-hands-02.img";
        d.skin.handR="player-hands-02.img";
        d.skin.footSprite="player-feet-02.img"; d.skin.backpackSprite="player-circle-base-02.img";
        d.skin.baseTint=15781563; d.skin.handTint=10818304;
        d.skin.footTint=10818304; d.skin.backpackTint=11031846;
        d.skin.baseTintRed=15781563;
        d.skin.baseTintBlue=15781563;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitDev");
        d.skin.baseSprite="player-base-outfitDC.img";
        d.skin.handL="player-hands-02.img";
        d.skin.handR="player-hands-02.img";
        d.skin.footSprite="player-feet-02.img"; d.skin.backpackSprite="player-circle-base-02.img";
        d.skin.baseTint=3442216; d.skin.handTint=6937122;
        d.skin.footTint=6937122; d.skin.backpackTint=2902793;
        d.skin.baseTintRed=3442216;
        d.skin.baseTintBlue=3442216;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitMaintainer");
        d.skin.baseSprite="player-base-outfitDC.img";
        d.skin.handL="player-hands-02.img";
        d.skin.handR="player-hands-02.img";
        d.skin.footSprite="player-feet-02.img"; d.skin.backpackSprite="player-circle-base-02.img";
        d.skin.baseTint=9596644; d.skin.handTint=9596644;
        d.skin.footTint=9596644; d.skin.backpackTint=6441625;
        d.skin.baseTintRed=9596644;
        d.skin.baseTintBlue=9596644;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitGD");
        d.skin.baseSprite="player-base-outfitDC.img";
        d.skin.handL="player-hands-02.img";
        d.skin.handR="player-hands-02.img";
        d.skin.footSprite="player-feet-02.img"; d.skin.backpackSprite="player-circle-base-02.img";
        d.skin.baseTint=11218992; d.skin.handTint=14901087;
        d.skin.footTint=14901087; d.skin.backpackTint=7213072;
        d.skin.baseTintRed=11218992;
        d.skin.baseTintBlue=11218992;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitMod");
        d.skin.baseSprite="player-base-outfitDC.img";
        d.skin.handL="player-hands-02.img";
        d.skin.handR="player-hands-02.img";
        d.skin.footSprite="player-feet-02.img"; d.skin.backpackSprite="player-circle-base-02.img";
        d.skin.baseTint=3380187; d.skin.handTint=9684974;
        d.skin.footTint=9684974; d.skin.backpackTint=1529478;
        d.skin.baseTintRed=3380187;
        d.skin.baseTintBlue=3380187;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitWheat");
        d.skin.baseSprite="player-base-outfitWheat.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=16777215; d.skin.handTint=15785362;
        d.skin.footTint=15785362; d.skin.backpackTint=13346845;
        d.skin.baseTintRed=16777215;
        d.skin.baseTintBlue=16777215;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitNoir");
        d.skin.baseSprite="player-base-02.img";
        d.skin.handL="player-hands-02.img";
        d.skin.handR="player-hands-02.img";
        d.skin.footSprite="player-feet-02.img"; d.skin.backpackSprite="player-circle-base-02.img";
        d.skin.baseTint=1776411; d.skin.handTint=16777215;
        d.skin.footTint=16777215; d.skin.backpackTint=7829367;
        d.skin.baseTintRed=1776411;
        d.skin.baseTintBlue=1776411;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitRedLeaderAged");
        d.skin.baseSprite="player-base-02.img";
        d.skin.handL="player-hands-02.img";
        d.skin.handR="player-hands-02.img";
        d.skin.footSprite="player-feet-02.img"; d.skin.backpackSprite="player-circle-base-02.img";
        d.skin.baseTint=10098712; d.skin.handTint=16711680;
        d.skin.footTint=16711680; d.skin.backpackTint=5442572;
        d.skin.baseTintRed=10098712;
        d.skin.baseTintBlue=10098712;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitBlueLeaderAged");
        d.skin.baseSprite="player-base-02.img";
        d.skin.handL="player-hands-02.img";
        d.skin.handR="player-hands-02.img";
        d.skin.footSprite="player-feet-02.img"; d.skin.backpackSprite="player-circle-base-02.img";
        d.skin.baseTint=1523353; d.skin.handTint=20223;
        d.skin.footTint=20223; d.skin.backpackTint=794700;
        d.skin.baseTintRed=1523353;
        d.skin.baseTintBlue=1523353;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitRedLeader");
        d.skin.baseSprite="player-base-02.img";
        d.skin.handL="player-hands-02.img";
        d.skin.handR="player-hands-02.img";
        d.skin.footSprite="player-feet-02.img"; d.skin.backpackSprite="player-circle-base-02.img";
        d.skin.baseTint=10158080; d.skin.handTint=16711680;
        d.skin.footTint=16711680; d.skin.backpackTint=5439488;
        d.skin.baseTintRed=10158080;
        d.skin.baseTintBlue=10158080;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitBlueLeader");
        d.skin.baseSprite="player-base-02.img";
        d.skin.handL="player-hands-02.img";
        d.skin.handR="player-hands-02.img";
        d.skin.footSprite="player-feet-02.img"; d.skin.backpackSprite="player-circle-base-02.img";
        d.skin.baseTint=12187; d.skin.handTint=20223;
        d.skin.footTint=20223; d.skin.backpackTint=5964;
        d.skin.baseTintRed=12187;
        d.skin.baseTintBlue=12187;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitSpetsnaz");
        d.skin.baseSprite="player-base-outfitSpetsnaz.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=16777215; d.skin.handTint=15000804;
        d.skin.footTint=15000804; d.skin.backpackTint=13816530;
        d.skin.baseTintRed=16777215;
        d.skin.baseTintBlue=16777215;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitWoodsCloak");
        d.skin.baseSprite="player-base-02.img";
        d.skin.handL="player-hands-02.img";
        d.skin.handR="player-hands-02.img";
        d.skin.footSprite="player-feet-02.img"; d.skin.backpackSprite="player-circle-base-02.img";
        d.skin.baseTint=2817792; d.skin.handTint=16711594;
        d.skin.footTint=16711594; d.skin.backpackTint=15635271;
        d.skin.baseTintRed=2817792;
        d.skin.baseTintBlue=2817792;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitElf");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=12845056; d.skin.handTint=1489152;
        d.skin.footTint=1489152; d.skin.backpackTint=365312;
        d.skin.baseTintRed=12845056;
        d.skin.baseTintBlue=12845056;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitImperial");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=12320813; d.skin.handTint=16777215;
        d.skin.footTint=16777215; d.skin.backpackTint=12625727;
        d.skin.baseTintRed=12320813;
        d.skin.baseTintBlue=12320813;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitLumber");
        d.skin.baseSprite="player-base-outfitLumber.img";
        d.skin.handL="player-hands-02.img";
        d.skin.handR="player-hands-02.img";
        d.skin.footSprite="player-feet-02.img"; d.skin.backpackSprite="player-circle-base-02.img";
        d.skin.baseTint=16777215; d.skin.handTint=8258312;
        d.skin.footTint=8258312; d.skin.backpackTint=4854547;
        d.skin.baseTintRed=16777215;
        d.skin.baseTintBlue=16777215;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitVerde");
        d.skin.baseSprite="player-base-02.img";
        d.skin.handL="player-hands-02.img";
        d.skin.handR="player-hands-02.img";
        d.skin.footSprite="player-feet-02.img"; d.skin.backpackSprite="player-circle-base-02.img";
        d.skin.baseTint=1785868; d.skin.handTint=11912587;
        d.skin.footTint=11912587; d.skin.backpackTint=11238441;
        d.skin.baseTintRed=1785868;
        d.skin.baseTintBlue=1785868;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitPineapple");
        d.skin.baseSprite="player-base-02.img";
        d.skin.handL="player-hands-02.img";
        d.skin.handR="player-hands-02.img";
        d.skin.footSprite="player-feet-02.img"; d.skin.backpackSprite="player-circle-base-02.img";
        d.skin.baseTint=10027008; d.skin.handTint=4985105;
        d.skin.footTint=4985105; d.skin.backpackTint=16763904;
        d.skin.baseTintRed=10027008;
        d.skin.baseTintBlue=10027008;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitTarkhany");
        d.skin.baseSprite="player-base-02.img";
        d.skin.handL="player-hands-02.img";
        d.skin.handR="player-hands-02.img";
        d.skin.footSprite="player-feet-02.img"; d.skin.backpackSprite="player-circle-base-02.img";
        d.skin.baseTint=4927107; d.skin.handTint=16757760;
        d.skin.footTint=16757760; d.skin.backpackTint=4661344;
        d.skin.baseTintRed=4927107;
        d.skin.baseTintBlue=4927107;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitWaterElem");
        d.skin.baseSprite="player-base-02.img";
        d.skin.handL="player-hands-02.img";
        d.skin.handR="player-hands-02.img";
        d.skin.footSprite="player-feet-02.img"; d.skin.backpackSprite="player-circle-base-02.img";
        d.skin.baseTint=7143401; d.skin.handTint=15990876;
        d.skin.footTint=15990876; d.skin.backpackTint=32644;
        d.skin.baseTintRed=7143401;
        d.skin.baseTintBlue=7143401;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitHeaven");
        d.skin.baseSprite="player-base-outfitHeaven.img";
        d.skin.handL="player-hands-02.img";
        d.skin.handR="player-hands-02.img";
        d.skin.footSprite="player-feet-02.img"; d.skin.backpackSprite="player-circle-base-02.img";
        d.skin.baseTint=16777215; d.skin.handTint=13762639;
        d.skin.footTint=13762639; d.skin.backpackTint=36503;
        d.skin.baseTintRed=16777215;
        d.skin.baseTintBlue=16777215;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitMeteor");
        d.skin.baseSprite="player-base-02.img";
        d.skin.handL="player-hands-02.img";
        d.skin.handR="player-hands-02.img";
        d.skin.footSprite="player-feet-02.img"; d.skin.backpackSprite="player-circle-base-02.img";
        d.skin.baseTint=9764864; d.skin.handTint=16742400;
        d.skin.footTint=16742400; d.skin.backpackTint=4727582;
        d.skin.baseTintRed=9764864;
        d.skin.baseTintBlue=9764864;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitIslander");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=16762368; d.skin.handTint=148992;
        d.skin.footTint=148992; d.skin.backpackTint=4495104;
        d.skin.baseTintRed=16762368;
        d.skin.baseTintBlue=16762368;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitAqua");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=47778; d.skin.handTint=65502;
        d.skin.footTint=65502; d.skin.backpackTint=536620;
        d.skin.baseTintRed=47778;
        d.skin.baseTintBlue=47778;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitCoral");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=16736103; d.skin.handTint=16746895;
        d.skin.footTint=16746895; d.skin.backpackTint=16772298;
        d.skin.baseTintRed=16736103;
        d.skin.baseTintBlue=16736103;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitKhaki");
        d.skin.baseSprite="player-base-02.img";
        d.skin.handL="player-hands-02.img";
        d.skin.handR="player-hands-02.img";
        d.skin.footSprite="player-feet-02.img"; d.skin.backpackSprite="player-circle-base-02.img";
        d.skin.baseTint=12824197; d.skin.handTint=9404516;
        d.skin.footTint=9404516; d.skin.backpackTint=4208940;
        d.skin.baseTintRed=12824197;
        d.skin.baseTintBlue=12824197;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitParma");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=8746585; d.skin.handTint=12824197;
        d.skin.footTint=12824197; d.skin.backpackTint=4208940;
        d.skin.baseTintRed=8746585;
        d.skin.baseTintBlue=8746585;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitParmaPrestige");
        d.skin.baseSprite="player-base-outfitParmaPrestige.img";
        d.skin.handL="player-hands-02.img";
        d.skin.handR="player-hands-02.img";
        d.skin.footSprite="player-feet-02.img"; d.skin.backpackSprite="player-circle-base-02.img";
        d.skin.baseTint=14925953; d.skin.handTint=11113323;
        d.skin.footTint=11113323; d.skin.backpackTint=6640177;
        d.skin.baseTintRed=14925953;
        d.skin.baseTintBlue=14925953;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitCasanova");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=4327436; d.skin.handTint=7602183;
        d.skin.footTint=7602183; d.skin.backpackTint=1052688;
        d.skin.baseTintRed=4327436;
        d.skin.baseTintBlue=4327436;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitPrisoner");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=16735266; d.skin.handTint=16545059;
        d.skin.footTint=16545059; d.skin.backpackTint=16756224;
        d.skin.baseTintRed=16735266;
        d.skin.baseTintBlue=16735266;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitJester");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=7798904; d.skin.handTint=4915276;
        d.skin.footTint=4915276; d.skin.backpackTint=936960;
        d.skin.baseTintRed=7798904;
        d.skin.baseTintBlue=7798904;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitWoodland");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=2831146; d.skin.handTint=5925970;
        d.skin.footTint=5925970; d.skin.backpackTint=5056000;
        d.skin.baseTintRed=2831146;
        d.skin.baseTintBlue=2831146;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitRoyalFortune");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=8333091; d.skin.handTint=15254058;
        d.skin.footTint=15254058; d.skin.backpackTint=9981696;
        d.skin.baseTintRed=8333091;
        d.skin.baseTintBlue=8333091;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitKeyLime");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=13107007; d.skin.handTint=15662941;
        d.skin.footTint=15662941; d.skin.backpackTint=12355383;
        d.skin.baseTintRed=13107007;
        d.skin.baseTintBlue=13107007;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitCobaltShell");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=11095; d.skin.handTint=2711164;
        d.skin.footTint=2711164; d.skin.backpackTint=19093;
        d.skin.baseTintRed=11095;
        d.skin.baseTintBlue=11095;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitFragtastic");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=6445343; d.skin.handTint=8352810;
        d.skin.footTint=8352810; d.skin.backpackTint=10066329;
        d.skin.baseTintRed=6445343;
        d.skin.baseTintBlue=6445343;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitCarbonFiber");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=2171169; d.skin.handTint=1842204;
        d.skin.footTint=1842204; d.skin.backpackTint=3552822;
        d.skin.baseTintRed=2171169;
        d.skin.baseTintBlue=2171169;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitDarkGloves");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=16303476; d.skin.handTint=12482560;
        d.skin.footTint=12482560; d.skin.backpackTint=10708736;
        d.skin.baseTintRed=16303476;
        d.skin.baseTintBlue=16303476;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitDarkShirt");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=12482560; d.skin.handTint=16303476;
        d.skin.footTint=16303476; d.skin.backpackTint=15183443;
        d.skin.baseTintRed=12482560;
        d.skin.baseTintBlue=12482560;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitGhillie");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=8630096; d.skin.handTint=8630096;
        d.skin.footTint=8630096; d.skin.backpackTint=6697728;
        d.skin.baseTintRed=8630096;
        d.skin.baseTintBlue=8630096;
        d.skin.spriteScale=0.15f;

        d.ghillie=true; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitDesertCamo");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=13736782; d.skin.handTint=11169046;
        d.skin.footTint=11169046; d.skin.backpackTint=16763778;
        d.skin.baseTintRed=13736782;
        d.skin.baseTintBlue=13736782;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitCamo");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=10066278; d.skin.handTint=8684631;
        d.skin.footTint=8684631; d.skin.backpackTint=6710835;
        d.skin.baseTintRed=10066278;
        d.skin.baseTintBlue=10066278;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitRed");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=16711680; d.skin.handTint=13893632;
        d.skin.footTint=13893632; d.skin.backpackTint=11993088;
        d.skin.baseTintRed=16711680;
        d.skin.baseTintBlue=16711680;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitWhite");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=14935011; d.skin.handTint=15658734;
        d.skin.footTint=15658734; d.skin.backpackTint=14474460;
        d.skin.baseTintRed=14935011;
        d.skin.baseTintBlue=14935011;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitSnow");
        d.skin.baseSprite="player-base-outfitSnow.img";
        d.skin.handL="player-hands-outfitSnow.img";
        d.skin.handR="player-hands-outfitSnow.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=16777215; d.skin.handTint=16777215;
        d.skin.footTint=11725567; d.skin.backpackTint=7849181;
        d.skin.baseTintRed=16777215;
        d.skin.baseTintBlue=16777215;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitBlackIce");
        d.skin.baseSprite="player-base-02.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=6843758; d.skin.handTint=4278099;
        d.skin.footTint=3355453; d.skin.backpackTint=6186099;
        d.skin.baseTintRed=6843758;
        d.skin.baseTintBlue=6843758;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitBeachCamo");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=15583870; d.skin.handTint=16442806;
        d.skin.footTint=16442806; d.skin.backpackTint=8165723;
        d.skin.baseTintRed=15583870;
        d.skin.baseTintBlue=15583870;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitCoconut");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=7755830; d.skin.handTint=3550498;
        d.skin.footTint=15330806; d.skin.backpackTint=15330806;
        d.skin.baseTintRed=7755830;
        d.skin.baseTintBlue=7755830;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitWave");
        d.skin.baseSprite="player-base-02.img";
        d.skin.handL="player-hands-02.img";
        d.skin.handR="player-hands-02.img";
        d.skin.footSprite="player-feet-02.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=1153260; d.skin.handTint=16643569;
        d.skin.footTint=16643569; d.skin.backpackTint=2193582;
        d.skin.baseTintRed=1153260;
        d.skin.baseTintBlue=1153260;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitParrotfish");
        d.skin.baseSprite="player-base-outfitParrotfish.img";
        d.skin.handL="player-hands-02.img";
        d.skin.handR="player-hands-02.img";
        d.skin.footSprite="player-feet-02.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=16777215; d.skin.handTint=3851974;
        d.skin.footTint=3172240; d.skin.backpackTint=3649195;
        d.skin.baseTintRed=16777215;
        d.skin.baseTintBlue=16777215;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitEvent");
        d.skin.baseSprite="player-base-outfitEvent.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-02.img"; d.skin.backpackSprite="player-circle-base-02.img";
        d.skin.baseTint=16777215; d.skin.handTint=11369195;
        d.skin.footTint=11369195; d.skin.backpackTint=11369195;
        d.skin.baseTintRed=16777215;
        d.skin.baseTintBlue=16777215;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitGold");
        d.skin.baseSprite="player-base-outfitGold.img";
        d.skin.handL="player-hands-outfitGold.img";
        d.skin.handR="player-hands-outfitGold.img";
        d.skin.footSprite="player-feet-outfitGold.img"; d.skin.backpackSprite="player-bag-outfitGold.img";
        d.skin.baseTint=16777215; d.skin.handTint=16777215;
        d.skin.footTint=16777215; d.skin.backpackTint=16777215;
        d.skin.baseTintRed=16777215;
        d.skin.baseTintBlue=16777215;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitRain");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=3365795; d.skin.handTint=7838164;
        d.skin.footTint=2837640; d.skin.backpackTint=9806255;
        d.skin.baseTintRed=3365795;
        d.skin.baseTintBlue=3365795;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitCowz");
        d.skin.baseSprite="player-base-outfitCowz.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=16777215; d.skin.handTint=11250603;
        d.skin.footTint=11250603; d.skin.backpackTint=8224125;
        d.skin.baseTintRed=16777215;
        d.skin.baseTintBlue=16777215;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitChameleon");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=4705222; d.skin.handTint=9100079;
        d.skin.footTint=9100079; d.skin.backpackTint=14721591;
        d.skin.baseTintRed=4705222;
        d.skin.baseTintBlue=4705222;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitPastel");
        d.skin.baseSprite="player-base-outfitPastel.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-02.img";
        d.skin.baseTint=16777215; d.skin.handTint=14138866;
        d.skin.footTint=8819711; d.skin.backpackTint=8819711;
        d.skin.baseTintRed=16777215;
        d.skin.baseTintBlue=16777215;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitChrys");
        d.skin.baseSprite="player-base-outfitChrys.img";
        d.skin.handL="player-hands-02.img";
        d.skin.handR="player-hands-02.img";
        d.skin.footSprite="player-feet-02.img"; d.skin.backpackSprite="player-circle-base-02.img";
        d.skin.baseTint=16777215; d.skin.handTint=11627340;
        d.skin.footTint=11627340; d.skin.backpackTint=11627340;
        d.skin.baseTintRed=16777215;
        d.skin.baseTintBlue=16777215;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitFahrenheit");
        d.skin.baseSprite="player-base-outfitFahrenheit.img";
        d.skin.handL="player-hands-outfitFahrenheit.img";
        d.skin.handR="player-hands-outfitFahrenheit.img";
        d.skin.footSprite="player-feet-02.img"; d.skin.backpackSprite="player-circle-base-02.img";
        d.skin.baseTint=16777215; d.skin.handTint=16777215;
        d.skin.footTint=12067335; d.skin.backpackTint=8724499;
        d.skin.baseTintRed=16777215;
        d.skin.baseTintBlue=16777215;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitPotatoskin");
        d.skin.baseSprite="player-base-outfitPotatoskin.img";
        d.skin.handL="player-hands-outfitPotatoskin.img";
        d.skin.handR="player-hands-outfitPotatoskin.img";
        d.skin.footSprite="player-feet-outfitPotatoskin.img"; d.skin.backpackSprite="player-bag-outfitPotatoskin.img";
        d.skin.baseTint=16777215; d.skin.handTint=16777215;
        d.skin.footTint=16777215; d.skin.backpackTint=16777215;
        d.skin.baseTintRed=16777215;
        d.skin.baseTintBlue=16777215;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitAurora");
        d.skin.baseSprite="player-base-outfitAurora.img";
        d.skin.handL="player-hand-left-outfitAurora.img";
        d.skin.handR="player-hand-right-outfitAurora.img";
        d.skin.footSprite="player-feet-02.img"; d.skin.backpackSprite="player-bag-outfitAurora.img";
        d.skin.baseTint=16777215; d.skin.handTint=16777215;
        d.skin.footTint=2305360; d.skin.backpackTint=16777215;
        d.skin.baseTintRed=16777215;
        d.skin.baseTintBlue=16777215;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitBarrel");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=16303476; d.skin.handTint=16303476;
        d.skin.footTint=16303476; d.skin.backpackTint=8480055;
        d.skin.baseTintRed=16303476;
        d.skin.baseTintBlue=16303476;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitWoodBarrel");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=16303476; d.skin.handTint=16303476;
        d.skin.footTint=16303476; d.skin.backpackTint=8480055;
        d.skin.baseTintRed=16303476;
        d.skin.baseTintBlue=16303476;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitStone");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=16303476; d.skin.handTint=16303476;
        d.skin.footTint=16303476; d.skin.backpackTint=8480055;
        d.skin.baseTintRed=16303476;
        d.skin.baseTintBlue=16303476;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitSpringTree");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=16303476; d.skin.handTint=16303476;
        d.skin.footTint=16303476; d.skin.backpackTint=8480055;
        d.skin.baseTintRed=16303476;
        d.skin.baseTintBlue=16303476;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitHalloweenTree");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=16303476; d.skin.handTint=16303476;
        d.skin.footTint=16303476; d.skin.backpackTint=8480055;
        d.skin.baseTintRed=16303476;
        d.skin.baseTintBlue=16303476;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitTreeSpooky");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=16303476; d.skin.handTint=16303476;
        d.skin.footTint=16303476; d.skin.backpackTint=8480055;
        d.skin.baseTintRed=16303476;
        d.skin.baseTintBlue=16303476;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitStump");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=16303476; d.skin.handTint=16303476;
        d.skin.footTint=16303476; d.skin.backpackTint=8480055;
        d.skin.baseTintRed=16303476;
        d.skin.baseTintBlue=16303476;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitBush");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=16303476; d.skin.handTint=16303476;
        d.skin.footTint=16303476; d.skin.backpackTint=8480055;
        d.skin.baseTintRed=16303476;
        d.skin.baseTintBlue=16303476;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitLeafPile");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=16303476; d.skin.handTint=16303476;
        d.skin.footTint=16303476; d.skin.backpackTint=8480055;
        d.skin.baseTintRed=16303476;
        d.skin.baseTintBlue=16303476;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitCrate");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=16303476; d.skin.handTint=16303476;
        d.skin.footTint=16303476; d.skin.backpackTint=8480055;
        d.skin.baseTintRed=16303476;
        d.skin.baseTintBlue=16303476;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitTable");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=16303476; d.skin.handTint=16303476;
        d.skin.footTint=16303476; d.skin.backpackTint=8480055;
        d.skin.baseTintRed=16303476;
        d.skin.baseTintBlue=16303476;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitSoviet");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=16303476; d.skin.handTint=16303476;
        d.skin.footTint=16303476; d.skin.backpackTint=8480055;
        d.skin.baseTintRed=16303476;
        d.skin.baseTintBlue=16303476;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitAirdrop");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=16303476; d.skin.handTint=16303476;
        d.skin.footTint=16303476; d.skin.backpackTint=8480055;
        d.skin.baseTintRed=16303476;
        d.skin.baseTintBlue=16303476;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitOven");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=16303476; d.skin.handTint=16303476;
        d.skin.footTint=16303476; d.skin.backpackTint=8480055;
        d.skin.baseTintRed=16303476;
        d.skin.baseTintBlue=16303476;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitRefrigerator");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=16303476; d.skin.handTint=16303476;
        d.skin.footTint=16303476; d.skin.backpackTint=8480055;
        d.skin.baseTintRed=16303476;
        d.skin.baseTintBlue=16303476;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitVending");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=16303476; d.skin.handTint=16303476;
        d.skin.footTint=16303476; d.skin.backpackTint=8480055;
        d.skin.baseTintRed=16303476;
        d.skin.baseTintBlue=16303476;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitPumpkin");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=16303476; d.skin.handTint=16303476;
        d.skin.footTint=16303476; d.skin.backpackTint=8480055;
        d.skin.baseTintRed=16303476;
        d.skin.baseTintBlue=16303476;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitWoodpile");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=16303476; d.skin.handTint=16303476;
        d.skin.footTint=16303476; d.skin.backpackTint=8480055;
        d.skin.baseTintRed=16303476;
        d.skin.baseTintBlue=16303476;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitToilet");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=16303476; d.skin.handTint=16303476;
        d.skin.footTint=16303476; d.skin.backpackTint=8480055;
        d.skin.baseTintRed=16303476;
        d.skin.baseTintBlue=16303476;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitBushRiver");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=16303476; d.skin.handTint=16303476;
        d.skin.footTint=16303476; d.skin.backpackTint=8480055;
        d.skin.baseTintRed=16303476;
        d.skin.baseTintBlue=16303476;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitCrab");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=16303476; d.skin.handTint=16303476;
        d.skin.footTint=16303476; d.skin.backpackTint=8480055;
        d.skin.baseTintRed=16303476;
        d.skin.baseTintBlue=16303476;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("outfitStumpAxe");
        d.skin.baseSprite="player-base-01.img";
        d.skin.handL="player-hands-01.img";
        d.skin.handR="player-hands-01.img";
        d.skin.footSprite="player-feet-01.img"; d.skin.backpackSprite="player-circle-base-01.img";
        d.skin.baseTint=16303476; d.skin.handTint=16303476;
        d.skin.footTint=16303476; d.skin.backpackTint=8480055;
        d.skin.baseTintRed=16303476;
        d.skin.baseTintBlue=16303476;
        d.skin.spriteScale=0.15f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("healer");
         d.visor.baseSprite="player-visor-healer.img";
        d.visor.handL="";
        d.visor.handR="";
        d.visor.footSprite=""; d.visor.backpackSprite="";
        d.visor.baseTint=16777215; d.visor.handTint=16777215;
        d.visor.footTint=16777215; d.visor.backpackTint=16777215;
        d.visor.baseTintRed=16777215;
        d.visor.baseTintBlue=16777215;
        d.visor.spriteScale=0.3f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("tank");
         d.visor.baseSprite="player-visor-tank.img";
        d.visor.handL="";
        d.visor.handR="";
        d.visor.footSprite=""; d.visor.backpackSprite="";
        d.visor.baseTint=16777215; d.visor.handTint=16777215;
        d.visor.footTint=16777215; d.visor.backpackTint=16777215;
        d.visor.baseTintRed=16777215;
        d.visor.baseTintBlue=16777215;
        d.visor.spriteScale=0.3f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("sniper");
         d.visor.baseSprite="player-visor-sniper.img";
        d.visor.handL="";
        d.visor.handR="";
        d.visor.footSprite=""; d.visor.backpackSprite="";
        d.visor.baseTint=16777215; d.visor.handTint=16777215;
        d.visor.footTint=16777215; d.visor.backpackTint=16777215;
        d.visor.baseTintRed=16777215;
        d.visor.baseTintBlue=16777215;
        d.visor.spriteScale=0.3f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("scout");
         d.visor.baseSprite="player-visor-scout.img";
        d.visor.handL="";
        d.visor.handR="";
        d.visor.footSprite=""; d.visor.backpackSprite="";
        d.visor.baseTint=16777215; d.visor.handTint=16777215;
        d.visor.footTint=16777215; d.visor.backpackTint=16777215;
        d.visor.baseTintRed=16777215;
        d.visor.baseTintBlue=16777215;
        d.visor.spriteScale=0.3f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("demo");
         d.visor.baseSprite="player-visor-demo.img";
        d.visor.handL="";
        d.visor.handR="";
        d.visor.footSprite=""; d.visor.backpackSprite="";
        d.visor.baseTint=16777215; d.visor.handTint=16777215;
        d.visor.footTint=16777215; d.visor.backpackTint=16777215;
        d.visor.baseTintRed=16777215;
        d.visor.baseTintBlue=16777215;
        d.visor.spriteScale=0.3f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("assault");
         d.visor.baseSprite="player-visor-assault.img";
        d.visor.handL="";
        d.visor.handR="";
        d.visor.footSprite=""; d.visor.backpackSprite="";
        d.visor.baseTint=16777215; d.visor.handTint=16777215;
        d.visor.footTint=16777215; d.visor.backpackTint=16777215;
        d.visor.baseTintRed=16777215;
        d.visor.baseTintBlue=16777215;
        d.visor.spriteScale=0.3f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("classless");
         d.visor.baseSprite="player-visor-classless.img";
        d.visor.handL="";
        d.visor.handR="";
        d.visor.footSprite=""; d.visor.backpackSprite="";
        d.visor.baseTint=16777215; d.visor.handTint=16777215;
        d.visor.footTint=16777215; d.visor.backpackTint=16777215;
        d.visor.baseTintRed=16777215;
        d.visor.baseTintBlue=16777215;
        d.visor.spriteScale=0.3f;

        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="fists";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("frag");

        d.worldImg.sprite="proj-frag-nopin-nolever-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.12f,0.12f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="throwable";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};
        d.handImgs[0][0].sprite="none"; d.handImgs[0][0].pos=Vec2(0.0f,0.0f);
        d.handImgs[0][0].scale=Vec2(1.0f,1.0f); d.handImgs[0][0].rot=0.0f; d.handImgs[0][0].tint=16777215;
        d.handImgs[0][0].renderOnHand=false; d.handImgs[0][0].leftHandOnTop=false;
        d.handImgs[0][0].handsBelow=false;
d.handImgs[0][1].sprite="proj-frag-pin-01.img"; d.handImgs[0][1].pos=Vec2(4.2f,4.2f);
        d.handImgs[0][1].scale=Vec2(0.14f,0.14f); d.handImgs[0][1].rot=0.0f; d.handImgs[0][1].tint=16777215;
        d.handImgs[0][1].renderOnHand=false; d.handImgs[0][1].leftHandOnTop=false;
        d.handImgs[0][1].handsBelow=false;
d.handImgs[1][0].sprite="proj-frag-pin-part.img"; d.handImgs[1][0].pos=Vec2(4.2f,4.2f);
        d.handImgs[1][0].scale=Vec2(0.14f,0.14f); d.handImgs[1][0].rot=0.0f; d.handImgs[1][0].tint=16777215;
        d.handImgs[1][0].renderOnHand=false; d.handImgs[1][0].leftHandOnTop=false;
        d.handImgs[1][0].handsBelow=false;
d.handImgs[1][1].sprite="proj-frag-nopin-01.img"; d.handImgs[1][1].pos=Vec2(4.2f,4.2f);
        d.handImgs[1][1].scale=Vec2(0.14f,0.14f); d.handImgs[1][1].rot=0.0f; d.handImgs[1][1].tint=16777215;
        d.handImgs[1][1].renderOnHand=false; d.handImgs[1][1].leftHandOnTop=false;
        d.handImgs[1][1].handsBelow=false;
d.handImgs[2][0].sprite="none"; d.handImgs[2][0].pos=Vec2(0.0f,0.0f);
        d.handImgs[2][0].scale=Vec2(1.0f,1.0f); d.handImgs[2][0].rot=0.0f; d.handImgs[2][0].tint=16777215;
        d.handImgs[2][0].renderOnHand=false; d.handImgs[2][0].leftHandOnTop=false;
        d.handImgs[2][0].handsBelow=false;
d.handImgs[2][1].sprite="none"; d.handImgs[2][1].pos=Vec2(0.0f,0.0f);
        d.handImgs[2][1].scale=Vec2(1.0f,1.0f); d.handImgs[2][1].rot=0.0f; d.handImgs[2][1].tint=16777215;
        d.handImgs[2][1].renderOnHand=false; d.handImgs[2][1].leftHandOnTop=false;
        d.handImgs[2][1].handsBelow=false;
        }
        { auto& d = _gameObjs.at("mirv");

        d.worldImg.sprite="proj-mirv-nopin-nolever.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.13f,0.13f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="throwable";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};
        d.handImgs[0][0].sprite="none"; d.handImgs[0][0].pos=Vec2(0.0f,0.0f);
        d.handImgs[0][0].scale=Vec2(1.0f,1.0f); d.handImgs[0][0].rot=0.0f; d.handImgs[0][0].tint=16777215;
        d.handImgs[0][0].renderOnHand=false; d.handImgs[0][0].leftHandOnTop=false;
        d.handImgs[0][0].handsBelow=false;
d.handImgs[0][1].sprite="proj-mirv-pin.img"; d.handImgs[0][1].pos=Vec2(4.2f,4.2f);
        d.handImgs[0][1].scale=Vec2(0.15f,0.15f); d.handImgs[0][1].rot=0.0f; d.handImgs[0][1].tint=16777215;
        d.handImgs[0][1].renderOnHand=false; d.handImgs[0][1].leftHandOnTop=false;
        d.handImgs[0][1].handsBelow=false;
d.handImgs[1][0].sprite="proj-frag-pin-part.img"; d.handImgs[1][0].pos=Vec2(4.2f,4.2f);
        d.handImgs[1][0].scale=Vec2(0.15f,0.15f); d.handImgs[1][0].rot=0.0f; d.handImgs[1][0].tint=16777215;
        d.handImgs[1][0].renderOnHand=false; d.handImgs[1][0].leftHandOnTop=false;
        d.handImgs[1][0].handsBelow=false;
d.handImgs[1][1].sprite="proj-mirv-nopin.img"; d.handImgs[1][1].pos=Vec2(4.2f,4.2f);
        d.handImgs[1][1].scale=Vec2(0.15f,0.15f); d.handImgs[1][1].rot=0.0f; d.handImgs[1][1].tint=16777215;
        d.handImgs[1][1].renderOnHand=false; d.handImgs[1][1].leftHandOnTop=false;
        d.handImgs[1][1].handsBelow=false;
d.handImgs[2][0].sprite="none"; d.handImgs[2][0].pos=Vec2(0.0f,0.0f);
        d.handImgs[2][0].scale=Vec2(1.0f,1.0f); d.handImgs[2][0].rot=0.0f; d.handImgs[2][0].tint=16777215;
        d.handImgs[2][0].renderOnHand=false; d.handImgs[2][0].leftHandOnTop=false;
        d.handImgs[2][0].handsBelow=false;
d.handImgs[2][1].sprite="none"; d.handImgs[2][1].pos=Vec2(0.0f,0.0f);
        d.handImgs[2][1].scale=Vec2(1.0f,1.0f); d.handImgs[2][1].rot=0.0f; d.handImgs[2][1].tint=16777215;
        d.handImgs[2][1].renderOnHand=false; d.handImgs[2][1].leftHandOnTop=false;
        d.handImgs[2][1].handsBelow=false;
        }
        { auto& d = _gameObjs.at("mirv_mini");

        d.worldImg.sprite="proj-mirv-mini-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.12f,0.12f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="throwable";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("martyr_nade");

        d.worldImg.sprite="proj-martyrdom-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.12f,0.12f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="throwable";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("smoke");

        d.worldImg.sprite="proj-smoke-nopin-nolever.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.12f,0.12f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="throwable";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};
        d.handImgs[0][0].sprite="none"; d.handImgs[0][0].pos=Vec2(0.0f,0.0f);
        d.handImgs[0][0].scale=Vec2(1.0f,1.0f); d.handImgs[0][0].rot=0.0f; d.handImgs[0][0].tint=16777215;
        d.handImgs[0][0].renderOnHand=false; d.handImgs[0][0].leftHandOnTop=false;
        d.handImgs[0][0].handsBelow=false;
d.handImgs[0][1].sprite="proj-smoke-pin.img"; d.handImgs[0][1].pos=Vec2(3.0f,4.2f);
        d.handImgs[0][1].scale=Vec2(0.14f,0.14f); d.handImgs[0][1].rot=0.0f; d.handImgs[0][1].tint=16777215;
        d.handImgs[0][1].renderOnHand=false; d.handImgs[0][1].leftHandOnTop=false;
        d.handImgs[0][1].handsBelow=false;
d.handImgs[1][0].sprite="proj-frag-pin-part.img"; d.handImgs[1][0].pos=Vec2(3.0f,4.2f);
        d.handImgs[1][0].scale=Vec2(0.14f,0.14f); d.handImgs[1][0].rot=0.0f; d.handImgs[1][0].tint=16777215;
        d.handImgs[1][0].renderOnHand=false; d.handImgs[1][0].leftHandOnTop=false;
        d.handImgs[1][0].handsBelow=false;
d.handImgs[1][1].sprite="proj-smoke-nopin.img"; d.handImgs[1][1].pos=Vec2(3.0f,4.2f);
        d.handImgs[1][1].scale=Vec2(0.14f,0.14f); d.handImgs[1][1].rot=0.0f; d.handImgs[1][1].tint=16777215;
        d.handImgs[1][1].renderOnHand=false; d.handImgs[1][1].leftHandOnTop=false;
        d.handImgs[1][1].handsBelow=false;
d.handImgs[2][0].sprite="none"; d.handImgs[2][0].pos=Vec2(0.0f,0.0f);
        d.handImgs[2][0].scale=Vec2(1.0f,1.0f); d.handImgs[2][0].rot=0.0f; d.handImgs[2][0].tint=16777215;
        d.handImgs[2][0].renderOnHand=false; d.handImgs[2][0].leftHandOnTop=false;
        d.handImgs[2][0].handsBelow=false;
d.handImgs[2][1].sprite="none"; d.handImgs[2][1].pos=Vec2(0.0f,0.0f);
        d.handImgs[2][1].scale=Vec2(1.0f,1.0f); d.handImgs[2][1].rot=0.0f; d.handImgs[2][1].tint=16777215;
        d.handImgs[2][1].renderOnHand=false; d.handImgs[2][1].leftHandOnTop=false;
        d.handImgs[2][1].handsBelow=false;
        }
        { auto& d = _gameObjs.at("strobe");

        d.worldImg.sprite="proj-strobe-armed.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.12f,0.12f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="throwable";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};
        d.handImgs[0][0].sprite="none"; d.handImgs[0][0].pos=Vec2(0.0f,0.0f);
        d.handImgs[0][0].scale=Vec2(1.0f,1.0f); d.handImgs[0][0].rot=0.0f; d.handImgs[0][0].tint=16777215;
        d.handImgs[0][0].renderOnHand=false; d.handImgs[0][0].leftHandOnTop=false;
        d.handImgs[0][0].handsBelow=false;
d.handImgs[0][1].sprite="proj-strobe-unarmed.img"; d.handImgs[0][1].pos=Vec2(3.0f,4.2f);
        d.handImgs[0][1].scale=Vec2(0.14f,0.14f); d.handImgs[0][1].rot=0.0f; d.handImgs[0][1].tint=16777215;
        d.handImgs[0][1].renderOnHand=false; d.handImgs[0][1].leftHandOnTop=false;
        d.handImgs[0][1].handsBelow=false;
d.handImgs[1][0].sprite=""; d.handImgs[1][0].pos=Vec2(3.0f,4.2f);
        d.handImgs[1][0].scale=Vec2(0.14f,0.14f); d.handImgs[1][0].rot=0.0f; d.handImgs[1][0].tint=16777215;
        d.handImgs[1][0].renderOnHand=false; d.handImgs[1][0].leftHandOnTop=false;
        d.handImgs[1][0].handsBelow=false;
d.handImgs[1][1].sprite="proj-strobe-arming.img"; d.handImgs[1][1].pos=Vec2(3.0f,4.2f);
        d.handImgs[1][1].scale=Vec2(0.14f,0.14f); d.handImgs[1][1].rot=0.0f; d.handImgs[1][1].tint=16777215;
        d.handImgs[1][1].renderOnHand=false; d.handImgs[1][1].leftHandOnTop=false;
        d.handImgs[1][1].handsBelow=false;
d.handImgs[2][0].sprite="none"; d.handImgs[2][0].pos=Vec2(0.0f,0.0f);
        d.handImgs[2][0].scale=Vec2(1.0f,1.0f); d.handImgs[2][0].rot=0.0f; d.handImgs[2][0].tint=16777215;
        d.handImgs[2][0].renderOnHand=false; d.handImgs[2][0].leftHandOnTop=false;
        d.handImgs[2][0].handsBelow=false;
d.handImgs[2][1].sprite="none"; d.handImgs[2][1].pos=Vec2(0.0f,0.0f);
        d.handImgs[2][1].scale=Vec2(1.0f,1.0f); d.handImgs[2][1].rot=0.0f; d.handImgs[2][1].tint=16777215;
        d.handImgs[2][1].renderOnHand=false; d.handImgs[2][1].leftHandOnTop=false;
        d.handImgs[2][1].handsBelow=false;
        }
        { auto& d = _gameObjs.at("snowball");

        d.worldImg.sprite="proj-snowball-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.12f,0.12f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="throwable";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};
        d.handImgs[0][0].sprite="none"; d.handImgs[0][0].pos=Vec2(0.0f,0.0f);
        d.handImgs[0][0].scale=Vec2(1.0f,1.0f); d.handImgs[0][0].rot=0.0f; d.handImgs[0][0].tint=16777215;
        d.handImgs[0][0].renderOnHand=false; d.handImgs[0][0].leftHandOnTop=false;
        d.handImgs[0][0].handsBelow=false;
d.handImgs[0][1].sprite="proj-snowball-01.img"; d.handImgs[0][1].pos=Vec2(3.0f,4.2f);
        d.handImgs[0][1].scale=Vec2(0.14f,0.14f); d.handImgs[0][1].rot=0.0f; d.handImgs[0][1].tint=16777215;
        d.handImgs[0][1].renderOnHand=false; d.handImgs[0][1].leftHandOnTop=false;
        d.handImgs[0][1].handsBelow=false;
d.handImgs[1][0].sprite="none"; d.handImgs[1][0].pos=Vec2(0.0f,0.0f);
        d.handImgs[1][0].scale=Vec2(1.0f,1.0f); d.handImgs[1][0].rot=0.0f; d.handImgs[1][0].tint=16777215;
        d.handImgs[1][0].renderOnHand=false; d.handImgs[1][0].leftHandOnTop=false;
        d.handImgs[1][0].handsBelow=false;
d.handImgs[1][1].sprite="proj-snowball-01.img"; d.handImgs[1][1].pos=Vec2(3.0f,4.2f);
        d.handImgs[1][1].scale=Vec2(0.14f,0.14f); d.handImgs[1][1].rot=0.0f; d.handImgs[1][1].tint=16777215;
        d.handImgs[1][1].renderOnHand=false; d.handImgs[1][1].leftHandOnTop=false;
        d.handImgs[1][1].handsBelow=false;
d.handImgs[2][0].sprite="none"; d.handImgs[2][0].pos=Vec2(0.0f,0.0f);
        d.handImgs[2][0].scale=Vec2(1.0f,1.0f); d.handImgs[2][0].rot=0.0f; d.handImgs[2][0].tint=16777215;
        d.handImgs[2][0].renderOnHand=false; d.handImgs[2][0].leftHandOnTop=false;
        d.handImgs[2][0].handsBelow=false;
d.handImgs[2][1].sprite="none"; d.handImgs[2][1].pos=Vec2(0.0f,0.0f);
        d.handImgs[2][1].scale=Vec2(1.0f,1.0f); d.handImgs[2][1].rot=0.0f; d.handImgs[2][1].tint=16777215;
        d.handImgs[2][1].renderOnHand=false; d.handImgs[2][1].leftHandOnTop=false;
        d.handImgs[2][1].handsBelow=false;
        }
        { auto& d = _gameObjs.at("snowball_heavy");

        d.worldImg.sprite="proj-snowball-02.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.2f,0.2f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="throwable";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("potato");

        d.worldImg.sprite="proj-potato-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.12f,0.12f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="throwable";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};
        d.handImgs[0][0].sprite="none"; d.handImgs[0][0].pos=Vec2(0.0f,0.0f);
        d.handImgs[0][0].scale=Vec2(1.0f,1.0f); d.handImgs[0][0].rot=0.0f; d.handImgs[0][0].tint=16777215;
        d.handImgs[0][0].renderOnHand=false; d.handImgs[0][0].leftHandOnTop=false;
        d.handImgs[0][0].handsBelow=false;
d.handImgs[0][1].sprite="proj-potato-01.img"; d.handImgs[0][1].pos=Vec2(3.0f,4.2f);
        d.handImgs[0][1].scale=Vec2(0.14f,0.14f); d.handImgs[0][1].rot=0.0f; d.handImgs[0][1].tint=16777215;
        d.handImgs[0][1].renderOnHand=false; d.handImgs[0][1].leftHandOnTop=false;
        d.handImgs[0][1].handsBelow=false;
d.handImgs[1][0].sprite="none"; d.handImgs[1][0].pos=Vec2(0.0f,0.0f);
        d.handImgs[1][0].scale=Vec2(1.0f,1.0f); d.handImgs[1][0].rot=0.0f; d.handImgs[1][0].tint=16777215;
        d.handImgs[1][0].renderOnHand=false; d.handImgs[1][0].leftHandOnTop=false;
        d.handImgs[1][0].handsBelow=false;
d.handImgs[1][1].sprite="proj-potato-01.img"; d.handImgs[1][1].pos=Vec2(3.0f,4.2f);
        d.handImgs[1][1].scale=Vec2(0.14f,0.14f); d.handImgs[1][1].rot=0.0f; d.handImgs[1][1].tint=16777215;
        d.handImgs[1][1].renderOnHand=false; d.handImgs[1][1].leftHandOnTop=false;
        d.handImgs[1][1].handsBelow=false;
d.handImgs[2][0].sprite="none"; d.handImgs[2][0].pos=Vec2(0.0f,0.0f);
        d.handImgs[2][0].scale=Vec2(1.0f,1.0f); d.handImgs[2][0].rot=0.0f; d.handImgs[2][0].tint=16777215;
        d.handImgs[2][0].renderOnHand=false; d.handImgs[2][0].leftHandOnTop=false;
        d.handImgs[2][0].handsBelow=false;
d.handImgs[2][1].sprite="none"; d.handImgs[2][1].pos=Vec2(0.0f,0.0f);
        d.handImgs[2][1].scale=Vec2(1.0f,1.0f); d.handImgs[2][1].rot=0.0f; d.handImgs[2][1].tint=16777215;
        d.handImgs[2][1].renderOnHand=false; d.handImgs[2][1].leftHandOnTop=false;
        d.handImgs[2][1].handsBelow=false;
        }
        { auto& d = _gameObjs.at("potato_heavy");

        d.worldImg.sprite="proj-potato-02.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.2f,0.2f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="throwable";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("potato_cannonball");

        d.worldImg.sprite="proj-potato-02.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.2f,0.2f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="throwable";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("potato_smgshot");

        d.worldImg.sprite="proj-wedge-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.075f,0.075f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="throwable";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("potato_lmgshot");

        d.worldImg.sprite="proj-potato-03.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.06f,0.06f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="throwable";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("bomb_iron");

        d.worldImg.sprite="proj-bomb-iron-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.12f,0.12f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="throwable";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};






        }
        { auto& d = _gameObjs.at("coconut");

        d.worldImg.sprite="proj-coconut-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.15f,0.15f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="throwable";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};
        d.handImgs[0][0].sprite="none"; d.handImgs[0][0].pos=Vec2(0.0f,0.0f);
        d.handImgs[0][0].scale=Vec2(1.0f,1.0f); d.handImgs[0][0].rot=0.0f; d.handImgs[0][0].tint=16777215;
        d.handImgs[0][0].renderOnHand=false; d.handImgs[0][0].leftHandOnTop=false;
        d.handImgs[0][0].handsBelow=false;
d.handImgs[0][1].sprite="proj-coconut-01.img"; d.handImgs[0][1].pos=Vec2(3.0f,4.2f);
        d.handImgs[0][1].scale=Vec2(0.15f,0.15f); d.handImgs[0][1].rot=0.0f; d.handImgs[0][1].tint=16777215;
        d.handImgs[0][1].renderOnHand=false; d.handImgs[0][1].leftHandOnTop=false;
        d.handImgs[0][1].handsBelow=false;
d.handImgs[1][0].sprite="none"; d.handImgs[1][0].pos=Vec2(0.0f,0.0f);
        d.handImgs[1][0].scale=Vec2(1.0f,1.0f); d.handImgs[1][0].rot=0.0f; d.handImgs[1][0].tint=16777215;
        d.handImgs[1][0].renderOnHand=false; d.handImgs[1][0].leftHandOnTop=false;
        d.handImgs[1][0].handsBelow=false;
d.handImgs[1][1].sprite="proj-coconut-01.img"; d.handImgs[1][1].pos=Vec2(3.0f,4.2f);
        d.handImgs[1][1].scale=Vec2(0.15f,0.15f); d.handImgs[1][1].rot=0.0f; d.handImgs[1][1].tint=16777215;
        d.handImgs[1][1].renderOnHand=false; d.handImgs[1][1].leftHandOnTop=false;
        d.handImgs[1][1].handsBelow=false;
d.handImgs[2][0].sprite="none"; d.handImgs[2][0].pos=Vec2(0.0f,0.0f);
        d.handImgs[2][0].scale=Vec2(1.0f,1.0f); d.handImgs[2][0].rot=0.0f; d.handImgs[2][0].tint=16777215;
        d.handImgs[2][0].renderOnHand=false; d.handImgs[2][0].leftHandOnTop=false;
        d.handImgs[2][0].handsBelow=false;
d.handImgs[2][1].sprite="none"; d.handImgs[2][1].pos=Vec2(0.0f,0.0f);
        d.handImgs[2][1].scale=Vec2(1.0f,1.0f); d.handImgs[2][1].rot=0.0f; d.handImgs[2][1].tint=16777215;
        d.handImgs[2][1].renderOnHand=false; d.handImgs[2][1].leftHandOnTop=false;
        d.handImgs[2][1].handsBelow=false;
        }
        { auto& d = _gameObjs.at("tomato");

        d.worldImg.sprite="proj-tomato-01.img"; d.worldImg.pos=Vec2(0.0f,0.0f);
        d.worldImg.scale=Vec2(0.12f,0.12f); d.worldImg.rot=0.0f; d.worldImg.tint=16777215;
        d.worldImg.renderOnHand=false; d.worldImg.leftHandOnTop=false;
        d.worldImg.handsBelow=false;
        d.ghillie=false; d.isDual=false; d.level=0;
        d.gunOffset=Vec2(0.0f,0.0f); d.leftHandOffset=Vec2(0.0f,0.0f);
        d.magSprite=""; d.magPos=Vec2(0.0f,0.0f);
        d.magTop=false; d.idlePose="throwable";
        d.recoil=0.0f;
        d.attackAnims={};
        d.deployAnims={};
        d.idleAnims={};
        d.frozenSprites={};
        d.handImgs[0][0].sprite="none"; d.handImgs[0][0].pos=Vec2(0.0f,0.0f);
        d.handImgs[0][0].scale=Vec2(1.0f,1.0f); d.handImgs[0][0].rot=0.0f; d.handImgs[0][0].tint=16777215;
        d.handImgs[0][0].renderOnHand=false; d.handImgs[0][0].leftHandOnTop=false;
        d.handImgs[0][0].handsBelow=false;
d.handImgs[0][1].sprite="proj-tomato-01.img"; d.handImgs[0][1].pos=Vec2(3.0f,4.2f);
        d.handImgs[0][1].scale=Vec2(0.14f,0.14f); d.handImgs[0][1].rot=0.0f; d.handImgs[0][1].tint=16777215;
        d.handImgs[0][1].renderOnHand=false; d.handImgs[0][1].leftHandOnTop=false;
        d.handImgs[0][1].handsBelow=false;
d.handImgs[1][0].sprite="none"; d.handImgs[1][0].pos=Vec2(0.0f,0.0f);
        d.handImgs[1][0].scale=Vec2(1.0f,1.0f); d.handImgs[1][0].rot=0.0f; d.handImgs[1][0].tint=16777215;
        d.handImgs[1][0].renderOnHand=false; d.handImgs[1][0].leftHandOnTop=false;
        d.handImgs[1][0].handsBelow=false;
d.handImgs[1][1].sprite="proj-tomato-01.img"; d.handImgs[1][1].pos=Vec2(3.0f,4.2f);
        d.handImgs[1][1].scale=Vec2(0.14f,0.14f); d.handImgs[1][1].rot=0.0f; d.handImgs[1][1].tint=16777215;
        d.handImgs[1][1].renderOnHand=false; d.handImgs[1][1].leftHandOnTop=false;
        d.handImgs[1][1].handsBelow=false;
d.handImgs[2][0].sprite="none"; d.handImgs[2][0].pos=Vec2(0.0f,0.0f);
        d.handImgs[2][0].scale=Vec2(1.0f,1.0f); d.handImgs[2][0].rot=0.0f; d.handImgs[2][0].tint=16777215;
        d.handImgs[2][0].renderOnHand=false; d.handImgs[2][0].leftHandOnTop=false;
        d.handImgs[2][0].handsBelow=false;
d.handImgs[2][1].sprite="none"; d.handImgs[2][1].pos=Vec2(0.0f,0.0f);
        d.handImgs[2][1].scale=Vec2(1.0f,1.0f); d.handImgs[2][1].rot=0.0f; d.handImgs[2][1].tint=16777215;
        d.handImgs[2][1].renderOnHand=false; d.handImgs[2][1].leftHandOnTop=false;
        d.handImgs[2][1].handsBelow=false;
        }
        _poses["fists"]={{{Vec2(14.0f,-12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(14.0f,12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}};
        _poses["slash"]={{{Vec2(18.0f,-8.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(6.0f,20.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}};
        _poses["meleeTwoHanded"]={{{Vec2(10.5f,-14.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(18.0f,6.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}};
        _poses["meleeKatana"]={{{Vec2(8.5f,13.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(-3.0f,17.75f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}};
        _poses["meleeNaginata"]={{{Vec2(19.0f,-7.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(8.5f,24.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}};
        _poses["machete"]={{{Vec2(14.0f,-12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(1.0f,17.75f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}};
        _poses["cutlass"]={{{Vec2(14.0f,-12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(6.0f,16.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}};
        _poses["rifle"]={{{Vec2(28.0f,5.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(14.0f,1.75f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}};
        _poses["dualRifle"]={{{Vec2(5.75f,-16.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(5.75f,16.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}};
        _poses["bullpup"]={{{Vec2(28.0f,5.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(24.0f,1.75f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}};
        _poses["minigun"]={{{Vec2(18.0f,7.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(54.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}};
        _poses["launcher"]={{{Vec2(20.0f,10.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(2.0f,22.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}};
        _poses["pistol"]={{{Vec2(14.0f,1.75f),0.0f,Vec2(0.0f,0.0f)},{Vec2(14.0f,1.75f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}};
        _poses["dualPistol"]={{{Vec2(15.75f,-8.75f),0.0f,Vec2(0.0f,0.0f)},{Vec2(15.75f,8.75f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}};
        _poses["throwable"]={{{Vec2(15.75f,-9.625f),0.0f,Vec2(0.0f,0.0f)},{Vec2(15.75f,9.625f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}};
        _poses["downed"]={{{Vec2(14.0f,-12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(14.0f,12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(-15.75f,-9.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(-15.75f,9.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}};
        _animations["none"].keyframes={};
        _animations["fists"].keyframes={{0.0f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(14.0f,12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.1f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(29.75f,1.75f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.25f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(14.0f,12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0}};
        _animations["stab"].keyframes={{0.0f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(6.0f,20.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.1f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(29.75f,1.75f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.25f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(6.0f,20.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0}};
        _animations["cut"].keyframes={{0.0f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(14.0f,12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.025f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(14.0f,12.25f),-1.0995574287564276f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.125f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(14.0f,12.25f),1.0995574287564276f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.25f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(14.0f,12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0}};
        _animations["cutReverse"].keyframes={{0.0f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(1.0f,17.75f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.04000000000000001f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(25.0f,6.25f),0.9424777960769379f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.13999999999999999f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(25.0f,6.25f),-1.5707963267948966f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.25f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(1.0f,17.75f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0}};
        _animations["thrust"].keyframes={{0.0f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(14.0f,12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.04000000000000001f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(5.0f,12.25f),0.3141592653589793f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.13999999999999999f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(25.0f,6.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.25f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(14.0f,12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0}};
        _animations["slash"].keyframes={{0.0f,3,{{{Vec2(18.0f,-8.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(6.0f,20.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.1f,3,{{{Vec2(6.0f,-22.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(6.0f,20.25f),-1.8849555921538759f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.25f,3,{{{Vec2(18.0f,-8.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(6.0f,20.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0}};
        _animations["hook"].keyframes={{0.0f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(14.0f,12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.01875f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(14.0f,12.25f),0.3141592653589793f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.075f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(24.0f,1.75f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.125f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(14.0f,12.25f),-0.9424777960769379f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.175f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(14.0f,12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0}};
        _animations["pan"].keyframes={{0.0f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(14.0f,12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.15f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(22.0f,-8.25f),-0.6283185307179586f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.25f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(28.0f,-8.25f),1.5707963267948966f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.55f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(14.0f,12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0}};
        _animations["axeSwing"].keyframes={{0.0f,3,{{{Vec2(10.5f,-14.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(18.0f,6.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.072f,3,{{{Vec2(9.0f,-14.25f),1.2566370614359172f,Vec2(0.0f,0.0f)},{Vec2(18.0f,6.25f),1.2566370614359172f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.18f,3,{{{Vec2(9.0f,-14.25f),-1.2566370614359172f,Vec2(0.0f,0.0f)},{Vec2(18.0f,6.25f),-1.2566370614359172f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.36f,3,{{{Vec2(10.5f,-14.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(18.0f,6.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0}};
        _animations["hammerSwing"].keyframes={{0.0f,3,{{{Vec2(10.5f,-14.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(18.0f,6.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.1f,3,{{{Vec2(9.0f,-14.25f),1.2566370614359172f,Vec2(0.0f,0.0f)},{Vec2(18.0f,6.25f),1.2566370614359172f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.25f,3,{{{Vec2(9.0f,-14.25f),-1.2566370614359172f,Vec2(0.0f,0.0f)},{Vec2(18.0f,6.25f),-1.2566370614359172f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.5f,3,{{{Vec2(10.5f,-14.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(18.0f,6.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0}};
        _animations["katanaSwing"].keyframes={{0.0f,3,{{{Vec2(8.5f,13.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(-3.0f,17.75f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.06f,3,{{{Vec2(8.5f,13.25f),0.6283185307179586f,Vec2(0.0f,0.0f)},{Vec2(-3.0f,17.75f),0.6283185307179586f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.18000000000000002f,3,{{{Vec2(8.5f,13.25f),-3.7699111843077517f,Vec2(0.0f,0.0f)},{Vec2(-3.0f,17.75f),-3.7699111843077517f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.4f,3,{{{Vec2(8.5f,13.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(-3.0f,17.75f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0}};
        _animations["naginataSwing"].keyframes={{0.0f,3,{{{Vec2(19.0f,-7.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(8.5f,24.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.081f,3,{{{Vec2(19.0f,-7.25f),0.9424777960769379f,Vec2(0.0f,0.0f)},{Vec2(8.5f,24.25f),0.9424777960769379f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.24300000000000002f,3,{{{Vec2(19.0f,-7.25f),-2.670353755551324f,Vec2(0.0f,0.0f)},{Vec2(8.5f,24.25f),-2.670353755551324f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.54f,3,{{{Vec2(19.0f,-7.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(8.5f,24.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0}};
        _animations["sawSwing"].keyframes={{0.0f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(1.0f,17.75f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.04000000000000001f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(25.0f,6.25f),0.9424777960769379f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.1f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(25.0f,6.25f),-0.9424777960769379f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.4f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(25.0f,17.75f),-0.7853981633974483f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.3f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(-36.0f,7.75f),-0.7853981633974483f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.7f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(1.0f,17.75f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0}};
        _animations["cutReverseShort"].keyframes={{0.0f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(1.0f,17.75f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.04000000000000001f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(25.0f,6.25f),0.9424777960769379f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.1f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(25.0f,6.25f),-0.9424777960769379f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.25f,2,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(14.0f,17.75f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0}};
        _animations["cook"].keyframes={{0.0f,3,{{{Vec2(15.75f,-9.625f),0.0f,Vec2(0.0f,0.0f)},{Vec2(15.75f,9.625f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.1f,3,{{{Vec2(14.0f,-1.75f),0.0f,Vec2(0.0f,0.0f)},{Vec2(14.0f,1.75f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.3f,3,{{{Vec2(14.0f,-1.75f),0.0f,Vec2(0.0f,0.0f)},{Vec2(14.0f,1.75f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.4f,3,{{{Vec2(22.75f,-1.75f),0.0f,Vec2(0.0f,0.0f)},{Vec2(1.75f,14.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{99999.0f,3,{{{Vec2(22.75f,-1.75f),0.0f,Vec2(0.0f,0.0f)},{Vec2(1.75f,14.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0}};
        _animations["throw"].keyframes={{0.0f,3,{{{Vec2(22.75f,-1.75f),0.0f,Vec2(0.0f,0.0f)},{Vec2(1.75f,14.175f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.15f,3,{{{Vec2(5.25f,-15.75f),0.0f,Vec2(0.0f,0.0f)},{Vec2(29.75f,1.75f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.44999999999999996f,3,{{{Vec2(15.75f,-9.625f),0.0f,Vec2(0.0f,0.0f)},{Vec2(15.75f,9.625f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0}};
        _animations["crawl_forward"].keyframes={{0.0f,5,{{{Vec2(14.0f,-12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(-15.75f,-9.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.2475f,5,{{{Vec2(19.25f,-10.5f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(-20.25f,-9.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.495f,5,{{{Vec2(5.25f,-15.75f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(-11.25f,-9.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.75f,5,{{{Vec2(14.0f,-12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(-15.75f,-9.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0}};
        _animations["crawl_backward"].keyframes={{0.0f,5,{{{Vec2(14.0f,-12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(-15.75f,-9.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.2475f,5,{{{Vec2(5.25f,-15.75f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(-11.25f,-9.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.495f,5,{{{Vec2(19.25f,-10.5f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(-20.25f,-9.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.75f,5,{{{Vec2(14.0f,-12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(-15.75f,-9.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0}};
        _animations["revive"].keyframes={{0.0f,3,{{{Vec2(14.0f,-12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(14.0f,12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.2f,3,{{{Vec2(24.5f,-8.75f),0.0f,Vec2(0.0f,0.0f)},{Vec2(5.25f,21.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{8.2f,3,{{{Vec2(24.5f,-8.75f),0.0f,Vec2(0.0f,0.0f)},{Vec2(5.25f,21.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0}};
        _animations["karambit_spin"].keyframes={{0.0f,34,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(6.0f,20.25f),1.0995574287564276f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),21.991148575128552f,Vec2(17.5f,5.0f)}}},PoseEasing::Linear,0},{0.575f,34,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(6.0f,20.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(17.5f,5.0f)}}},PoseEasing::OutSine,0},{0.65f,34,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(6.0f,20.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0}};
        _animations["karambit_rapidSpin"].keyframes={{0.0f,34,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(6.0f,20.25f),1.0995574287564276f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),9.42477796076938f,Vec2(17.5f,5.0f)}}},PoseEasing::Linear,0},{0.45f,34,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(20.0f,10.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),-4.084070449666731f,Vec2(17.5f,5.0f)}}},PoseEasing::OutSine,0},{0.65f,34,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(6.0f,20.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::InSine,0}};
        _animations["bayonet_unsheathe"].keyframes={{0.0f,35,{{{Vec2(14.0f,-12.25f),-0.7853981633974483f,Vec2(0.0f,0.0f)},{Vec2(1.0f,17.75f),-3.141592653589793f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.3f,35,{{{Vec2(14.0f,-12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(1.0f,17.75f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.15707963267948966f,Vec2(0.0f,0.0f)}}},PoseEasing::InSine,0},{0.4f,35,{{{Vec2(14.0f,-12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(1.0f,17.75f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),1.0995574287564276f,Vec2(0.0f,0.0f)}}},PoseEasing::OutSine,0},{0.65f,35,{{{Vec2(14.0f,-12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(14.0f,12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::InSine,0}};
        _animations["knuckles_slam"].keyframes={{0.0f,33,{{{Vec2(14.0f,-12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(-5.0f,24.0f),-9.42477796076938f,Vec2(-20.0f,0.0f)}}},PoseEasing::Linear,2},{0.3f,35,{{{Vec2(18.0f,-6.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(14.0f,12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(-10.0f,18.0f),-1.2566370614359172f,Vec2(-20.0f,0.0f)}}},PoseEasing::Linear,0},{0.35f,35,{{{Vec2(18.0f,-6.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(23.0f,-6.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),-1.2566370614359172f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.45f,35,{{{Vec2(18.0f,-6.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(27.0f,-13.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),-1.2566370614359172f,Vec2(0.0f,0.0f)}}},PoseEasing::OutQuart,0},{0.65f,35,{{{Vec2(14.0f,-12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(14.0f,12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::InSine,0}};
        _animations["knuckles_spin"].keyframes={{0.0f,33,{{{Vec2(20.0f,6.25f),-1.2566370614359172f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,32.0f),-10.995574287564276f,Vec2(20.0f,10.0f)}}},PoseEasing::Linear,2},{0.2f,35,{{{Vec2(20.0f,6.25f),-0.6283185307179586f,Vec2(0.0f,0.0f)},{Vec2(14.0f,12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(-14.0f,16.0f),-5.497787143782138f,Vec2(20.0f,10.0f)}}},PoseEasing::Linear,0},{0.4f,35,{{{Vec2(20.0f,6.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(19.0f,12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),-0.7853981633974483f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.525f,35,{{{Vec2(17.0f,-3.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(23.0f,12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),-0.9424777960769379f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.65f,35,{{{Vec2(14.0f,-12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(14.0f,12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0}};
        _animations["huntsman_catch"].keyframes={{0.0f,34,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(14.0f,12.25f),1.5707963267948966f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),9.42477796076938f,Vec2(0.0f,0.0f)}}},PoseEasing::InSine,0},{0.25f,34,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(14.0f,12.25f),0.7853981633974483f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(-20.0f,0.0f),4.71238898038469f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.45f,34,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(28.0f,12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.47123889803846897f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.475f,34,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(31.0f,12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.47123889803846897f,Vec2(0.0f,0.0f)}}},PoseEasing::OutSine,0},{0.675f,34,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(14.0f,12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::InOutSine,0}};
        _animations["karambit_frontSpin"].keyframes={{0.0f,34,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(6.0f,20.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.2f,34,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(6.0f,20.25f),0.3141592653589793f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.9424777960769379f,Vec2(17.5f,5.0f)}}},PoseEasing::OutSine,0},{0.6f,34,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(6.0f,20.25f),-0.3141592653589793f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),-12.566370614359172f,Vec2(17.5f,5.0f)}}},PoseEasing::InSine,0},{0.7f,34,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(6.0f,20.25f),-0.3141592653589793f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),-14.137166941154069f,Vec2(17.5f,5.0f)}}},PoseEasing::OutSine,0},{0.85f,34,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(6.0f,20.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),-12.566370614359172f,Vec2(0.0f,0.0f)}}},PoseEasing::InSine,0}};
        _animations["karambit_backSpin"].keyframes={{0.0f,34,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(6.0f,20.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.2f,34,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(6.0f,20.25f),-0.3141592653589793f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),-1.5707963267948966f,Vec2(17.5f,5.0f)}}},PoseEasing::OutSine,0},{0.6f,34,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(6.0f,20.25f),0.3141592653589793f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),12.566370614359172f,Vec2(17.5f,5.0f)}}},PoseEasing::InSine,0},{0.7f,34,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(6.0f,20.25f),0.3141592653589793f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),13.50884841043611f,Vec2(17.5f,5.0f)}}},PoseEasing::OutSine,0},{0.85f,34,{{{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(6.0f,20.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),12.566370614359172f,Vec2(0.0f,0.0f)}}},PoseEasing::InSine,0}};
        _animations["knife_inspect"].keyframes={{0.0f,35,{{{Vec2(14.0f,-12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(14.0f,12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::InOutSine,0},{0.35f,35,{{{Vec2(21.0f,8.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(21.0f,17.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),-0.6283185307179586f,Vec2(0.0f,0.0f)}}},PoseEasing::InOutSine,0},{0.5f,35,{{{Vec2(21.0f,-10.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(21.0f,17.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),-0.7853981633974483f,Vec2(0.0f,0.0f)}}},PoseEasing::InOutSine,0},{0.8f,35,{{{Vec2(14.0f,-12.25f),-0.15707963267948966f,Vec2(0.0f,0.0f)},{Vec2(21.0f,17.25f),-1.0995574287564276f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),-2.9059732045705586f,Vec2(0.0f,0.0f)}}},PoseEasing::InOutSine,0},{0.85f,35,{{{Vec2(14.0f,-12.25f),-0.15707963267948966f,Vec2(0.0f,0.0f)},{Vec2(21.0f,17.25f),-1.0995574287564276f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),-2.9059732045705586f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{1.15f,35,{{{Vec2(14.0f,-12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(14.0f,12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::InOutSine,0}};
        _animations["knuckles_bash"].keyframes={{0.0f,35,{{{Vec2(14.0f,-12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(14.0f,12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.15f,35,{{{Vec2(16.0f,-18.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(16.0f,18.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),-0.7853981633974483f,Vec2(0.0f,0.0f)}}},PoseEasing::OutSine,0},{0.2f,35,{{{Vec2(16.0f,-18.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(16.0f,18.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),-0.7853981633974483f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.35f,35,{{{Vec2(16.0f,-18.0f),0.6283185307179586f,Vec2(0.0f,0.0f)},{Vec2(16.0f,18.0f),-0.6283185307179586f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),-0.7853981633974483f,Vec2(0.0f,0.0f)}}},PoseEasing::OutBounce,0},{0.45f,35,{{{Vec2(16.0f,-18.0f),0.6283185307179586f,Vec2(0.0f,0.0f)},{Vec2(16.0f,18.0f),-0.6283185307179586f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),-0.6283185307179586f,Vec2(0.0f,0.0f)}}},PoseEasing::Linear,0},{0.7f,35,{{{Vec2(14.0f,-12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(14.0f,12.25f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)},{Vec2(0.0f,0.0f),0.0f,Vec2(0.0f,0.0f)}}},PoseEasing::OutQuad,0}};

        _mapRenders.at("main").colors.playerGhillie=8630096;
        _mapRenders.at("main_spring").colors.playerGhillie=6000138;
        _mapRenders.at("main_summer").colors.playerGhillie=6658085;
        _mapRenders.at("desert").colors.playerGhillie=14657377;
        _mapRenders.at("faction").colors.playerGhillie=5005348;
        _mapRenders.at("faction_potato").colors.playerGhillie=5005348;
        _mapRenders.at("halloween").colors.playerGhillie=8630096;
        _mapRenders.at("potato").colors.playerGhillie=8630096;
        _mapRenders.at("potato_spring").colors.playerGhillie=6000138;
        _mapRenders.at("snow").colors.playerGhillie=12303291;
        _mapRenders.at("woods").colors.playerGhillie=9536812;
        _mapRenders.at("woods_snow").colors.playerGhillie=12303291;
        _mapRenders.at("woods_spring").colors.playerGhillie=4285194;
        _mapRenders.at("woods_summer").colors.playerGhillie=6658085;
        _mapRenders.at("savannah").colors.playerGhillie=11578411;
        _mapRenders.at("cobalt").colors.playerGhillie=4937830;
        _mapRenders.at("turkey").colors.playerGhillie=10784302;
        _mapRenders.at("birthday").colors.playerGhillie=8630096;
        _mapRenders.at("beach").colors.playerGhillie=8236134;
        _mapRenders.at("test_normal").colors.playerGhillie=8630096;
        _mapRenders.at("test_faction").colors.playerGhillie=8630096;
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
    const PlayerPose* playerPose(const std::string& type) const override {
        auto it = _poses.find(type);
        return it == _poses.end() ? nullptr : &it->second;
    }
    const PlayerAnimationDef* playerAnimation(const std::string& type) const override {
        auto it = _animations.find(type);
        return it == _animations.end() ? nullptr : &it->second;
    }

private:
    std::unordered_map<std::string, MapObjectDef> _mapObjs;
    std::unordered_map<std::string, MapRenderDef> _mapRenders;
    std::unordered_map<std::string, GameObjRenderDef> _gameObjs;
    std::unordered_map<std::string, ParticleDef> _particles;
    std::unordered_map<std::string, EmitterDef> _emitters;
    std::unordered_map<std::string, PlayerPose> _poses;
    std::unordered_map<std::string, PlayerAnimationDef> _animations;
};

} // namespace

void installGeneratedDefs() {
    static GeneratedProvider provider;
    setDefProvider(&provider);
}

} // namespace surv
