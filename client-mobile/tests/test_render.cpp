// Host tests for the M4 rendering port: camera transform, terrain generation
// (bit-compatible with shared/utils/terrainGen.ts), renderer z-sorting, the
// object pool, and structure mask transforms.
#include "ReferenceData.h"
#include "TestFramework.h"

#include "game/GameWorld.h"
#include "game/Map.h"
#include "game/Gas.h"
#include "game/objects/Barns.h"
#include "game/objects/GameObject.h"
#include "game/objects/Structure.h"
#include "render/Camera.h"
#include "render/Defs.h"
#include "render/GeneratedDefs.h"
#include "render/NullPixi.h"
#include "render/Renderer.h"
#include "render/Terrain.h"

#include <string>
#include <vector>

using namespace surv;
using namespace surv_test;

namespace {

std::string fixture(const std::string& name) {
    for (const auto& kv : surv_reference::fixtures()) {
        if (kv.first == name) {
            return kv.second;
        }
    }
    return "";
}

std::vector<std::string> splitSemi(const std::string& s) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == ';') {
            out.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    if (!cur.empty()) {
        out.push_back(cur);
    }
    return out;
}

Vec2 parseVec(const std::string& tok) {
    const size_t comma = tok.find(',');
    return Vec2(std::stof(tok.substr(0, comma)), std::stof(tok.substr(comma + 1)));
}

struct TerrainFixture {
    std::vector<Vec2> shore;
    std::vector<Vec2> grass;
    struct R {
        Collider aabb;
        std::vector<Vec2> waterPoly;
        std::vector<Vec2> shorePoly;
    };
    std::vector<R> rivers;
};

TerrainFixture parseTerrain(const std::vector<std::string>& t) {
    TerrainFixture out;
    size_t i = 0;
    auto expect = [&](const char* prefix) {
        const std::string p(prefix);
        const std::string& tok = t[i];
        if (tok.rfind(p, 0) != 0) {
            return 0;
        }
        return std::stoi(tok.substr(p.size()));
    };
    int n = expect("shore:");
    i++;
    for (int k = 0; k < n; k++) {
        out.shore.push_back(parseVec(t[i++]));
    }
    n = expect("grass:");
    i++;
    for (int k = 0; k < n; k++) {
        out.grass.push_back(parseVec(t[i++]));
    }
    n = expect("rivers:");
    i++;
    for (int k = 0; k < n; k++) {
        TerrainFixture::R r;
        const std::string& aabbTok = t[i++];
        // "aabb:x,y,x,y"
        const std::string body = aabbTok.substr(std::string("aabb:").size());
        std::vector<float> nums;
        std::string cur;
        for (char c : body) {
            if (c == ',') {
                nums.push_back(std::stof(cur));
                cur.clear();
            } else {
                cur.push_back(c);
            }
        }
        if (!cur.empty()) {
            nums.push_back(std::stof(cur));
        }
        r.aabb = Collider::createAabb(Vec2(nums[0], nums[1]), Vec2(nums[2], nums[3]));
        int w = expect("wp:");
        i++;
        for (int j = 0; j < w; j++) {
            r.waterPoly.push_back(parseVec(t[i++]));
        }
        int sp = expect("sp:");
        i++;
        for (int j = 0; j < sp; j++) {
            r.shorePoly.push_back(parseVec(t[i++]));
        }
        out.rivers.push_back(std::move(r));
    }
    return out;
}

void checkVec(const Vec2& a, const Vec2& b, float eps = 2e-3f) {
    CHECK_NEAR(a.x, b.x, eps);
    CHECK_NEAR(a.y, b.y, eps);
}

} // namespace

// ---------------------------------------------------------------------------
TEST(camera_transform) {
    Camera cam;
    cam.m_pos = Vec2(100.0f, 100.0f);
    cam.m_screenWidth = 800.0f;
    cam.m_screenHeight = 600.0f;
    cam.m_ppu = 16.0f;
    cam.m_zoom = 1.5f;

    CHECK_NEAR(cam.m_z(), 24.0f, 1e-4f);
    checkVec(cam.m_pointToScreen(Vec2(100.0f, 100.0f)), Vec2(400.0f, 300.0f));
    checkVec(cam.m_pointToScreen(Vec2(0.0f, 0.0f)), Vec2(-2000.0f, 2700.0f));
    checkVec(cam.m_screenToPoint(Vec2(400.0f, 300.0f)), Vec2(100.0f, 100.0f));
    checkVec(cam.m_screenToPoint(Vec2(0.0f, 0.0f)), Vec2(100.0f - 400.0f / 24.0f,
                                                          100.0f + 300.0f / 24.0f));
    CHECK_NEAR(cam.m_pixels(2.0f), 3.0f, 1e-4f);
    CHECK_NEAR(cam.m_scaleToScreen(1.0f), 24.0f, 1e-4f);

    cam.m_shakeInt = 0.0f;
    cam.m_addShake(Vec2(100.0f - 40.0f, 100.0f), 1.0f); // dist 40 -> 0
    CHECK_NEAR(cam.m_shakeInt, 0.0f, 1e-4f);
    cam.m_addShake(Vec2(100.0f - 10.0f, 100.0f), 2.0f); // dist 10 -> 1 * 2
    CHECK_NEAR(cam.m_shakeInt, 2.0f, 1e-4f);
}

// ---------------------------------------------------------------------------
TEST(terrain_matches_ts) {
    const std::string raw = fixture("terrain");
    CHECK(!raw.empty());
    const TerrainFixture expected = parseTerrain(splitSemi(raw));

    TerrainData actual = generateTerrain(
        512.0f, 512.0f, 4.0f, 2.0f,
        {{3.0f,
          false,
          {Vec2(10.0f, 20.0f), Vec2(30.0f, 40.0f)}},
         {5.0f,
          true,
          {Vec2(100.0f, 100.0f), Vec2(150.0f, 120.0f), Vec2(200.0f, 100.0f),
           Vec2(150.0f, 80.0f)}}},
        123456);

    CHECK_EQ(actual.shore.size(), expected.shore.size());
    CHECK_EQ(actual.grass.size(), expected.grass.size());
    CHECK_EQ(actual.rivers.size(), expected.rivers.size());

    for (size_t i = 0; i < actual.shore.size(); i++) {
        checkVec(actual.shore[i], expected.shore[i]);
    }
    for (size_t i = 0; i < actual.grass.size(); i++) {
        checkVec(actual.grass[i], expected.grass[i]);
    }
    for (size_t i = 0; i < actual.rivers.size(); i++) {
        CHECK_EQ(actual.rivers[i].waterPoly.size(), expected.rivers[i].waterPoly.size());
        CHECK_EQ(actual.rivers[i].shorePoly.size(), expected.rivers[i].shorePoly.size());
        CHECK_NEAR(actual.rivers[i].aabb.min.x, expected.rivers[i].aabb.min.x, 2e-3f);
        CHECK_NEAR(actual.rivers[i].aabb.min.y, expected.rivers[i].aabb.min.y, 2e-3f);
        CHECK_NEAR(actual.rivers[i].aabb.max.x, expected.rivers[i].aabb.max.x, 2e-3f);
        CHECK_NEAR(actual.rivers[i].aabb.max.y, expected.rivers[i].aabb.max.y, 2e-3f);
        for (size_t j = 0; j < actual.rivers[i].waterPoly.size(); j++) {
            checkVec(actual.rivers[i].waterPoly[j], expected.rivers[i].waterPoly[j]);
        }
        for (size_t j = 0; j < actual.rivers[i].shorePoly.size(); j++) {
            checkVec(actual.rivers[i].shorePoly[j], expected.rivers[i].shorePoly[j]);
        }
    }
}

// ---------------------------------------------------------------------------
TEST(spline_matches_ts) {
    const std::string raw = fixture("spline");
    CHECK(!raw.empty());
    const std::vector<std::string> toks = splitSemi(raw);
    CHECK(toks.size() >= 11);
    CHECK(toks[0].rfind("total:", 0) == 0);
    const float total = std::stof(toks[0].substr(6));

    Spline spline({Vec2(0.0f, 0.0f), Vec2(10.0f, 5.0f), Vec2(20.0f, 0.0f), Vec2(30.0f, 5.0f)},
                  false);
    CHECK_NEAR(spline.totalArcLen, total, 2e-3f);
    for (int i = 0; i <= 8; i++) {
        checkVec(spline.getPos(static_cast<float>(i) / 8.0f), parseVec(toks[1 + i]));
    }
    CHECK(toks[10].rfind("closeT:", 0) == 0);
    CHECK_NEAR(spline.getClosestTtoPoint(Vec2(12.0f, 3.0f)),
               std::stof(toks[10].substr(7)), 2e-3f);
}

// ---------------------------------------------------------------------------
TEST(pool_reuse) {
    Pool<Structure> pool;
    auto* a = pool.m_alloc();
    auto* b = pool.m_alloc();
    CHECK_EQ(pool.activeCount(), 2);
    CHECK(a != b);
    pool.m_free(a);
    auto* c = pool.m_alloc();
    CHECK(c == a);
    CHECK_EQ(pool.activeCount(), 2);
}

// ---------------------------------------------------------------------------
namespace {
class FakeDefProvider : public DefProvider {
public:
    MapObjectDef def;
    const MapObjectDef* mapObject(const std::string& type) const override {
        return type == def.type ? &def : nullptr;
    }
    const MapRenderDef* mapRender(const std::string&) const override { return nullptr; }
    const GameObjRenderDef* gameObject(const std::string&) const override { return nullptr; }
};
} // namespace

TEST(structure_mask_transform) {
    FakeDefProvider provider;
    provider.def.type = "struct_test";
    provider.def.boundingCollider = Collider::createAabb(Vec2(-2.0f, -2.0f), Vec2(2.0f, 2.0f));
    provider.def.hasBounding = true;
    provider.def.mask.push_back(Collider::createAabb(Vec2(1.0f, 1.0f), Vec2(3.0f, 3.0f)));
    StructureLayerDef layer;
    layer.type = "struct_test";
    layer.pos = Vec2(0.0f, 0.0f);
    layer.ori = 0;
    layer.inheritOri = true;
    provider.def.layers.push_back(layer);
    StairDef stair;
    stair.collision = Collider::createAabb(Vec2(-1.0f, -1.0f), Vec2(1.0f, 1.0f));
    stair.downDir = Vec2(0.0f, -1.0f);
    provider.def.stairs.push_back(stair);

    setDefProvider(&provider);

    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    Structure s;
    s.active = true;
    ObjectData data;
    data.type = "struct_test";
    data.pos = Vec2(10.0f, 20.0f);
    data.ori = 0;
    data.layerObjIds[0] = 7;
    data.layerObjIds[1] = 8;
    s.m_updateData(data, true, true, world);

    CHECK_EQ(s.layers.size(), 1u);
    CHECK_EQ(s.layers[0].objId, 7);
    CHECK_EQ(s.stairs.size(), 1u);
    CHECK_EQ(s.mask.size(), 1u);
    CHECK_NEAR(s.mask[0].min.x, 11.0f, 1e-4f);
    CHECK_NEAR(s.mask[0].min.y, 21.0f, 1e-4f);
    CHECK_NEAR(s.mask[0].max.x, 13.0f, 1e-4f);
    CHECK_NEAR(s.mask[0].max.y, 23.0f, 1e-4f);
    CHECK_NEAR(s.aabb.min.x, 8.0f, 1e-4f);
    CHECK_NEAR(s.aabb.max.y, 22.0f, 1e-4f);

    setDefProvider(nullptr);
}

// ---------------------------------------------------------------------------
TEST(renderer_z_sort) {
    pix::NullPixiFactory factory;
    Renderer renderer(&factory, false);
    Camera camera;
    Map map(&factory, false);

    pix::Sprite* a = factory.createSprite();
    pix::Sprite* b = factory.createSprite();
    pix::Sprite* c = factory.createSprite();

    renderer.addPIXIObj(a, 0, 5, 100);
    renderer.addPIXIObj(b, 0, 1, 50);
    renderer.addPIXIObj(c, 0, 5, 20);

    // Same zOrd/zIdx early-return: no duplicate child.
    renderer.addPIXIObj(a, 0, 5, 100);

    renderer.m_update(0.016f, camera, map, false);

    auto* layer0 = static_cast<pix::NullContainer*>(renderer.layers[0]);
    CHECK_EQ(layer0->children.size(), 3u);
    CHECK_EQ(layer0->children[0], b); // zOrd 1
    CHECK_EQ(layer0->children[1], c); // zOrd 5, zIdx 20
    CHECK_EQ(layer0->children[2], a); // zOrd 5, zIdx 100

    // Stairs layer remap: layer & 2 -> layer 2/3 by zOrd >= 100.
    pix::Sprite* d = factory.createSprite();
    renderer.addPIXIObj(d, 2, 50, 1);
    pix::Sprite* e = factory.createSprite();
    renderer.addPIXIObj(e, 2, 150, 2);
    auto* layer2 = static_cast<pix::NullContainer*>(renderer.layers[2]);
    auto* layer3 = static_cast<pix::NullContainer*>(renderer.layers[3]);
    CHECK_EQ(layer2->children.size(), 1u);
    CHECK_EQ(layer2->children[0], d);
    CHECK_EQ(layer3->children.size(), 1u);
    CHECK_EQ(layer3->children[0], e);

    // Layer alpha: layer 0 => layerAlpha -> 0, layer 1 alpha tracks it.
    renderer.setActiveLayer(0);
    renderer.m_update(0.016f, camera, map, false);
    CHECK(renderer.layerAlpha < 0.5f);
    renderer.setActiveLayer(1);
    for (int i = 0; i < 200; i++) {
        renderer.m_update(0.016f, camera, map, false);
    }
    CHECK_NEAR(renderer.layerAlpha, 1.0f, 1e-3f);
}

// ---------------------------------------------------------------------------
TEST(renderer_reparents_between_layers) {
    pix::NullPixiFactory factory;
    Renderer renderer(&factory, false);
    Camera camera;
    Map map(&factory, false);

    pix::Sprite* a = factory.createSprite();
    renderer.addPIXIObj(a, 0, 5, 1);
    auto* layer0 = static_cast<pix::NullContainer*>(renderer.layers[0]);
    auto* layer1 = static_cast<pix::NullContainer*>(renderer.layers[1]);
    CHECK_EQ(layer0->children.size(), 1u);

    // Moving to another layer must reparent (remove from the old layer), like
    // PIXI's Container.addChild; otherwise axmol ends up with two parents.
    renderer.addPIXIObj(a, 1, 5, 1);
    CHECK_EQ(layer0->children.size(), 0u);
    CHECK_EQ(layer1->children.size(), 1u);
    CHECK_EQ(layer1->children[0], a);

    // Re-adding with a changed zIdx must not duplicate the child.
    renderer.addPIXIObj(a, 1, 5, 2);
    CHECK_EQ(layer1->children.size(), 1u);
    renderer.addPIXIObj(a, 1, 5, 2);
    CHECK_EQ(layer1->children.size(), 1u);
}

// ---------------------------------------------------------------------------
TEST(generated_defs_provider) {
    installGeneratedDefs();
    const DefProvider* provider = getDefProvider();
    CHECK(provider != nullptr);
    if (!provider) {
        return;
    }

    // Biome colors for a known map.
    const MapRenderDef* mainMap = provider->mapRender("main");
    CHECK(mainMap != nullptr);
    if (mainMap) {
        CHECK(mainMap->colors.grass != 0);
        CHECK(mainMap->colors.water != 0);
    }

    // An obstacle image + collision.
    const MapObjectDef* barrel = provider->mapObject("barrel_01");
    CHECK(barrel != nullptr);
    if (barrel) {
        CHECK(!barrel->img.sprite.empty());
        CHECK(barrel->hasCollision);
    }

    // A structure exposes layers/stairs/mask.
    const MapObjectDef* structure = provider->mapObject("bridge_lg_structure_01");
    CHECK(structure != nullptr);
    if (structure) {
        CHECK(!structure->layers.empty());
        CHECK(!structure->mask.empty());
    }

    // Loot image for a known gun.
    const GameObjRenderDef* ak = provider->gameObject("ak47");
    CHECK(ak != nullptr);
    if (ak) {
        CHECK(ak->hasImg);
        CHECK(!ak->img.sprite.empty());
    }

    // T2: a building exposes floor/ceiling images + vision.
    const MapObjectDef* bank = provider->mapObject("bank_01");
    CHECK(bank != nullptr);
    if (bank) {
        CHECK(!bank->floorImgs.empty());
        CHECK(!bank->ceilingImgs.empty());
        CHECK(bank->ceilingVision.dist > 0.0f);
    }

    // image definitions carry their per-image rotation (obstacle.ts img.ori).
    if (barrel) {
        CHECK(std::isfinite(barrel->img.ori));
    }

    // obstacle.ts door casing image.
    const MapObjectDef* door = provider->mapObject("lab_door_01");
    CHECK(door != nullptr);
    if (door) {
        CHECK(!door->doorCasingSprite.empty());
        CHECK(door->doorCasingScale > 0.0f);
    }

    // T3: particle/emitter tables are emitted.
    const ParticleDef* splat = provider->particle("bloodSplat");
    CHECK(splat != nullptr);
    if (splat) {
        CHECK(!splat->images.empty());
    }
    const EmitterDef* campfire = provider->emitter("campfire_smoke");
    CHECK(campfire != nullptr);
    if (campfire) {
        CHECK_EQ(campfire->particle, std::string("cabinSmoke"));
    }
}

// ---------------------------------------------------------------------------
TEST(building_floor_ceiling_render) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    world.camera().m_screenWidth = 800.0f;
    world.camera().m_screenHeight = 600.0f;

    Building b;
    b.active = true;
    b.__id = 7;
    ObjectData data;
    data.type = "bank_01";
    data.pos = Vec2(100.0f, 100.0f);
    data.ori = 0;
    data.layer = 0;
    data.__id = 7;
    b.m_updateData(data, true, true, world);

    CHECK(b.imgs.size() >= 2u);
    CHECK(b.zIdx != 0 || !b.imgs.empty());
    // Floor zOrd == def zIdx; ceiling zOrd == 750 - zIdx.
    for (const auto& img : b.imgs) {
        CHECK_EQ(img.zOrd, img.isCeiling ? (750 - b.zIdx) : b.zIdx);
    }
    CHECK(b.ceilingFadeAlpha == 1.0f);

    b.update(0.016f, world);
    auto* layer0 = static_cast<pix::NullContainer*>(world.renderer().layers[0]);
    CHECK(layer0->children.size() >= b.imgs.size());

    // Reveal: with the active player at the building, the ceiling fades out.
    Player ap;
    ap.pos = Vec2(100.0f, 100.0f);
    ap.layer = 0;
    world.setActivePlayer(&ap);
    for (int i = 0; i < 120; i++) {
        b.update(0.016f, world);
    }
    CHECK(b.ceilingFadeAlpha < 0.5f);
    world.setActivePlayer(nullptr);
}

// ---------------------------------------------------------------------------
TEST(particles_emitter_spawns) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);

    EmitterOptions opts;
    opts.pos = Vec2(50.0f, 50.0f);
    opts.dir = Vec2(0.0f, 1.0f);
    Emitter* e = world.particleBarn().addEmitter(&factory, "campfire_smoke", opts);
    CHECK(e != nullptr);
    CHECK(e->active);

    for (int i = 0; i < 10; i++) {
        world.particleBarn().update(0.05f, world);
    }
    int active = 0;
    for (auto* p : world.particleBarn().particles) {
        if (p->active) {
            active++;
        }
    }
    CHECK(active > 0);

    // Direct addParticle also resolves its def + frame.
    Particle* p = world.particleBarn().addParticle(&factory, "bloodSplat", 0, Vec2(0.0f, 0.0f),
                                                  Vec2(1.0f, 0.0f));
    CHECK(p != nullptr);
    CHECK(p->active);
    CHECK(p->def != nullptr);
}

// ---------------------------------------------------------------------------
TEST(renderer_ground_fade) {
    pix::NullPixiFactory factory;
    Renderer renderer(&factory, false);
    Camera camera;
    camera.m_screenWidth = 800.0f;
    camera.m_screenHeight = 600.0f;
    Map map(&factory, false);
    map.mapLoaded = true;
    map.mapDef.colors.underground = 0x1b0e0b;

    renderer.resize(map, camera);
    renderer.setUnderground(true);
    renderer.setActiveLayer(1);
    for (int i = 0; i < 400; i++) {
        renderer.m_update(0.016f, camera, map, false);
    }
    CHECK_NEAR(renderer.groundAlpha, 1.0f, 1e-2f);

    // T6: the ground geometry must be redrawn with the faded alpha (DrawNode
    // bakes vertex alpha), not just the node opacity.
    auto* g = static_cast<pix::NullGraphics*>(renderer.ground);
    CHECK_NEAR(g->alpha, 1.0f, 1e-5f);
    float maxFillAlpha = 0.0f;
    bool hasRect = false;
    for (const auto& cmd : g->commands) {
        if (cmd.kind == pix::DrawCommand::BeginFill && cmd.alpha > maxFillAlpha) {
            maxFillAlpha = cmd.alpha;
        }
        if (cmd.kind == pix::DrawCommand::Rect) {
            hasRect = true;
        }
    }
    CHECK(hasRect);
    CHECK(maxFillAlpha > 0.9f);
}

namespace {
double filledArea(const std::vector<pix::FillTriangle>& triangles) {
    double area = 0;
    for (const auto& t : triangles) {
        area += std::fabs((double(t[1].x) - t[0].x) * (double(t[2].y) - t[0].y) -
                          (double(t[1].y) - t[0].y) * (double(t[2].x) - t[0].x)) / 2;
    }
    return area;
}
ObjectData playerData() {
    ObjectData d;
    d.pos = Vec2(50, 50);
    d.dir = Vec2(1, 0);
    d.outfit = "outfitBase";
    d.activeWeapon = "fists";
    return d;
}
pix::NullSprite* sprite(pix::Sprite* s) { return static_cast<pix::NullSprite*>(s); }
pix::NullContainer* container(pix::Container* c) { return static_cast<pix::NullContainer*>(c); }
}

TEST(graphics_polygon_hole) {
    pix::NullGraphics g;
    g.beginFill(0xffffff, 1);
    g.drawRect(0, 0, 10, 10);
    g.beginHole();
    const Vec2 diamond[] = {{5, 1}, {9, 5}, {5, 9}, {1, 5}, {5, 1}};
    g.drawPolygon(diamond, 5);
    g.endHole();
    g.endFill();
    CHECK_NEAR(filledArea(g.fillTriangles), 68.0, 1e-5);
    // All triangle centroids must be outside the diamond hole.
    for (const auto& t : g.fillTriangles) {
        const Vec2 center = v2Mul(v2Add(v2Add(t[0], t[1]), t[2]), 1.0f / 3.0f);
        CHECK(std::fabs(center.x - 5) + std::fabs(center.y - 5) >= 4 - 1e-5f);
    }
    CHECK_EQ(g.commands[2].kind, pix::DrawCommand::BeginHole);
    g.clear();
    CHECK(g.fillTriangles.empty());
}

TEST(graphics_concave_path_and_holes) {
    pix::NullGraphics g;
    g.beginFill(0xffffff, 1);
    // L-shape, reversed winding, with duplicate/collinear vertices.
    const Vec2 points[] = {{0, 0}, {0, 6}, {2, 6}, {2, 2}, {4, 2}, {6, 2}, {6, 0}, {0, 0}};
    for (size_t i = 0; i < 8; ++i) {
        if (i == 0) g.moveTo(points[i].x, points[i].y);
        else g.lineTo(points[i].x, points[i].y);
    }
    g.closePath();
    g.beginHole();
    // Negative-size rect (camera's flipped Y), partially outside the fill.
    g.drawRect(1, 3, 2, -2);
    g.endHole();
    g.endFill();
    CHECK_NEAR(filledArea(g.fillTriangles), 17.0, 1e-5);
}

TEST(graphics_overlapping_holes_and_fills) {
    pix::NullGraphics g;
    g.beginFill(0xffffff, 1);
    g.drawRect(0, 0, 10, 10);
    g.drawRect(0, 0, 10, 10); // union, no double alpha
    g.beginHole(); g.drawRect(2, 2, 4, 4); g.endHole();
    g.beginHole(); g.drawRect(4, 4, 4, 4); g.endHole();
    g.endFill();
    CHECK_NEAR(filledArea(g.fillTriangles), 72.0, 1e-5);
    g.clear();
    g.beginFill(0xffffff, 1); g.drawRect(0, 0, 10, 10);
    g.beginHole(); g.drawRect(-1, -1, 12, 12); g.endHole(); g.endFill();
    CHECK(g.fillTriangles.empty());
}

TEST(graphics_intersecting_polygon_holes) {
    pix::NullGraphics g;
    g.beginFill(0xffffff, 1); g.drawRect(0, 0, 10, 10);
    const Vec2 a[] = {{2, 2}, {8, 2}, {5, 8}};
    const Vec2 b[] = {{2, 8}, {8, 8}, {5, 2}};
    g.beginHole(); g.drawPolygon(a, 3); g.endHole();
    g.beginHole(); g.drawPolygon(b, 3); g.endHole();
    g.endFill();
    CHECK_NEAR(filledArea(g.fillTriangles), 73.0, 1e-5);
}

TEST(player_skin_and_equipment) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    Player p;
    p.m_init();
    auto d = playerData();
    d.backpack = "backpack02"; d.helmet = "helmet01"; d.chest = "chest01";
    d.activeWeapon = "ak47";
    p.m_updateData(d, true, true, world);
    p.update(0, world);
    CHECK_EQ(sprite(p.bodySprite)->frame, std::string("player-base-01.img"));
    CHECK_EQ(sprite(p.bodySprite)->tint, 0xf8c574u);
    CHECK_EQ(sprite(p.limbSprites[0])->frame, std::string("player-hands-01.img"));
    CHECK(sprite(p.backpackSprite)->visible);
    CHECK(sprite(p.helmetSprite)->visible);
    CHECK(sprite(p.chestSprite)->visible);
    CHECK(!sprite(p.limbSprites[2])->visible);
    CHECK(container(p.gunContainers[1])->visible);
    CHECK(!container(p.gunContainers[0])->visible);
    CHECK_EQ(sprite(p.gunSprites[1])->frame, getDefProvider()->gameObject("ak47")->worldImg.sprite);
    CHECK(sprite(p.gunSprites[1])->frame != getDefProvider()->gameObject("ak47")->img.sprite);
    CHECK_NEAR(container(p.boneContainers[0])->x, 28.0f + getDefProvider()->gameObject("ak47")->leftHandOffset.x, 1e-4f);
    CHECK_NEAR(container(p.boneContainers[1])->x, 14.0f, 1e-4f);
    d.dir = Vec2(0, 1);
    p.m_updateData(d, false, false, world);
    p.update(0, world);
    CHECK_NEAR(container(p.bodyContainer)->rotation, -3.14159265358979f / 2, 1e-4f);
    CHECK_NEAR(container(p.container)->rotation, 0, 1e-4f); // name stays upright
    CHECK_EQ(p.activeWeapon, std::string("ak47"));
    CHECK_EQ(p.container->getSortOrd(), 18);
}

TEST(player_animation_sequence_and_interpolation) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    Player p;
    p.m_init();
    auto d = playerData();
    p.m_updateData(d, true, true, world);
    p.update(0, world);
    d.animType = Anim_Melee; d.animSeq = 1;
    p.m_updateData(d, true, false, world);
    p.animMirror = false;
    p.update(0.05f, world);
    CHECK_NEAR(p.bones[1].pivot.x, 21.875f, 1e-4f); // midway to the punch
    CHECK_NEAR(container(p.boneContainers[1])->x, 21.875f, 1e-4f);
    p.m_updateData(d, true, false, world);
    CHECK_NEAR(p.animTicker, 0.05f, 1e-5f); // same seq never restarts
    p.update(1.0f, world);
    CHECK_EQ(p.currentAnim, Anim_None);
    p.update(0, world);
    CHECK_NEAR(p.bones[1].pivot.x, 14.0f, 1e-4f);
    d.animSeq = 2;
    p.m_updateData(d, true, false, world);
    CHECK_EQ(p.currentAnim, Anim_Melee);
    CHECK_NEAR(p.animTicker, 0, 1e-5f);
}

TEST(player_downed_gear_and_pool_reuse) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    Player p;
    p.m_init();
    auto d = playerData();
    d.activeWeapon = "ak47"; d.backpack = "backpack03"; d.downed = true;
    p.m_updateData(d, true, true, world); p.update(0, world);
    CHECK(sprite(p.limbSprites[2])->visible);
    CHECK(!sprite(p.backpackSprite)->visible);
    CHECK(!container(p.gunContainers[1])->visible);
    CHECK(p.bodyContainer->getChildIndex(p.boneContainers[0]) < p.bodyContainer->getChildIndex(p.bodySprite));
    p.m_free();
    CHECK(!container(p.container)->visible);
    const size_t childCount = p.bodyContainer->childCount();
    p.m_init();
    d = playerData();
    p.m_updateData(d, true, true, world); p.update(0, world);
    CHECK(container(p.container)->visible);
    CHECK(!sprite(p.limbSprites[2])->visible);
    CHECK(p.bodyContainer->getChildIndex(p.boneContainers[0]) > p.bodyContainer->getChildIndex(p.bodySprite));
    CHECK_EQ(p.bodyContainer->childCount(), childCount);
    CHECK_EQ(p.animSeq, 0);
}

TEST(player_throwable_and_melee_pose) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    Player p;
    p.m_init();
    auto d = playerData(); d.activeWeapon = "frag"; d.animType = Anim_Cook; d.animSeq = 1;
    p.m_updateData(d, true, true, world); p.update(0.2f, world);
    CHECK_EQ(p.throwableState, 1);
    CHECK_EQ(sprite(p.objectSprites[1])->frame, getDefProvider()->gameObject("frag")->handImgs[1][1].sprite);
    d.activeWeapon = "katana"; d.animType = Anim_None; d.animSeq = 2;
    p.m_updateData(d, true, false, world); p.update(0, world);
    CHECK(sprite(p.meleeSprite)->visible);
    CHECK(!sprite(p.objectSprites[1])->visible);
    CHECK_NEAR(p.bones[0].pivot.x, 8.5f, 1e-5f);
    CHECK_NEAR(p.bones[1].pivot.x, -3.0f, 1e-5f);
    d.dead = true;
    p.m_updateData(d, true, false, world); p.update(0, world);
    CHECK(!container(p.container)->visible);
}

TEST(player_generated_animation_tables) {
    installGeneratedDefs();
    const auto* defs = getDefProvider();
    CHECK(defs->playerPose("rifle") != nullptr);
    CHECK(defs->playerPose("missing") == nullptr);
    const auto* deploy = defs->playerAnimation("karambit_spin");
    CHECK(deploy != nullptr);
    CHECK_EQ(deploy->keyframes.size(), 3u);
    CHECK_EQ(deploy->keyframes[1].easing, PoseEasing::OutSine);
    CHECK((deploy->keyframes[1].mask & (1u << 5)) != 0);
    CHECK(defs->gameObject("fists") != nullptr); // fists has no lootImg
    CHECK_EQ(defs->gameObject("fists")->attackAnims[0], std::string("fists"));
}

TEST(player_dual_recoil_and_emitters) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    auto d = playerData();
    d.__type = ObjectType_Player; d.__id = 42; d.activeWeapon = "m9_dual";
    d.healEffect = true;
    UpdateMsg msg;
    FullObjectData full;
    full.data = d;
    msg.fullObjects.push_back(full);
    PlayerInfo info;
    info.playerId = 42; info.teamId = 2; info.name = "Test";
    msg.playerInfos.push_back(info);
    Bullet bullet;
    bullet.playerId = 42; bullet.shotFx = true; bullet.shotSourceType = "m9_dual";
    bullet.shotOffhand = true;
    msg.bullets.push_back(bullet);
    world.applyUpdate(msg);
    auto* p = world.playerBarn().getPlayerById(42);
    CHECK(p != nullptr);
    if (!p) return;
    CHECK(p->gunRecoil[0] > 0);
    CHECK_NEAR(p->gunRecoil[1], 0, 1e-5f);
    world.playerBarn().update(0, world);
    CHECK_EQ(p->teamId, 2);
    CHECK(container(p->gunContainers[0])->visible);
    CHECK(container(p->gunContainers[1])->visible);
    CHECK(p->healEmitter != nullptr);
    CHECK(p->healEmitter->active);
    const float recoil = p->gunRecoil[0];
    p->update(0.1f, world);
    CHECK(p->gunRecoil[0] < recoil);
    auto* e = p->healEmitter;
    p->m_free();
    CHECK_EQ(e->duration, e->ticker);
    CHECK(p->healEmitter == nullptr);
}

TEST(player_revive_and_missing_defs) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    Player p;
    p.m_init();
    auto d = playerData();
    d.outfit = "unknown_outfit"; d.activeWeapon = "ak47";
    d.animType = Anim_Revive; d.animSeq = 3;
    p.m_updateData(d, true, true, world); p.update(0.2f, world);
    CHECK_EQ(sprite(p.bodySprite)->frame, std::string("player-base-01.img"));
    CHECK(!container(p.gunContainers[1])->visible);
    CHECK_NEAR(p.bones[0].pivot.x, 24.5f, 1e-4f);
    setDefProvider(nullptr);
    p.playAnim(Anim_Melee, 4);
    p.update(0.1f, world); // an unavailable def must terminate, not crash
    CHECK_EQ(p.currentAnim, Anim_None);
    installGeneratedDefs();
}

TEST(obstacle_render_parity) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    world.camera().m_screenWidth = 800.0f;
    world.camera().m_screenHeight = 600.0f;

    const MapObjectDef* barrelDef = getDefProvider()->mapObject("barrel_01");
    CHECK(barrelDef != nullptr);

    Obstacle o;
    o.active = true;
    o.__id = 42;
    ObjectData data;
    data.type = "barrel_01";
    data.pos = Vec2(100.0f, 100.0f);
    data.ori = 0;
    data.layer = 0;
    data.scale = 1.0f;
    o.m_updateData(data, true, true, world);
    CHECK(o.imgDirty);
    CHECK(o.frame == barrelDef->img.sprite);
    o.update(0.016f, world);
    o.render(world, 0);
    CHECK(!o.imgDirty);
    CHECK(!sprite(o.sprite)->visible == false);
    // zIdx = floor(scale * 1000) * 65535 + __id (obstacle.ts sprite.zIdx).
    CHECK_EQ(o.zIdx, 1000 * 65535 + 42);
    CHECK_EQ(o.zOrd, barrelDef->img.zIdx);
    CHECK_NEAR(sprite(o.sprite)->sx, world.camera().m_pixels(1.0f * barrelDef->img.scale), 1e-4f);
    // The sprite is only rendered when it is visible.
    auto* layer0 = static_cast<pix::NullContainer*>(world.renderer().layers[0]);
    CHECK(layer0->children.size() >= 1u);

    // randomRotation folds the stable __id into the image rotation.
    const MapObjectDef* treeDef = nullptr;
    for (const char* cand : {"tree_01", "bush_01", "rock_01"}) {
        const MapObjectDef* d = getDefProvider()->mapObject(cand);
        if (d && d->randomRotation) { treeDef = d; data.type = cand; break; }
    }
    if (treeDef) {
        Obstacle t;
        t.active = true;
        t.__id = 90;
        ObjectData td = data;
        t.m_updateData(td, true, true, world);
        CHECK_NEAR(t.imgRot.x, math::deg2rad(90.0f), 1e-4f);
    }
}

TEST(obstacle_door_interp_and_casing) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    world.camera().m_screenWidth = 800.0f;
    world.camera().m_screenHeight = 600.0f;

    Obstacle o;
    o.active = true;
    o.__id = 5;
    ObjectData data;
    data.type = "lab_door_01";
    data.pos = Vec2(20.0f, 30.0f);
    data.ori = 0;
    data.layer = 0;
    data.scale = 1.0f;
    data.isDoor = true;
    data.doorOpen = false;
    data.doorSeq = 1;
    o.m_updateData(data, true, true, world);
    CHECK(o.casingEnabled);
    CHECK(o.doorHasInterp);

    // A network move (door slides open) interpolates the visual position.
    data.doorOpen = true;
    data.pos = Vec2(20.0f, 30.0f);
    o.m_updateData(data, true, false, world);
    o.update(0.016f, world);
    o.render(world, 0);
    CHECK(o.casingSprite != nullptr);
    bool casingDrawn = false;
    for (pix::Node* child : static_cast<pix::NullContainer*>(world.renderer().layers[0])->children) {
        if (child == o.casingSprite) casingDrawn = true;
    }
    CHECK(casingDrawn);
    CHECK(sprite(o.casingSprite)->visible);
}

TEST(gas_overlay_world_scale) {
    pix::NullPixiFactory factory;
    Camera camera;
    camera.m_zoom = 2.0f;
    Gas gas;
    gas.m_init(&factory);
    gas.setFullState(0.0f, GasData{});
    gas.gasRenderer.render(&factory, Vec2(100.0f, 100.0f), camera.m_scaleToScreen(50.0f), true,
                           camera.m_zoom);
    auto* gfx = static_cast<pix::NullGraphics*>(gas.gasRenderer.display);
    CHECK(gfx->visible);
    CHECK_NEAR(gfx->sx, camera.m_scaleToScreen(50.0f) * camera.m_zoom, 1e-4f);
    bool hasHole = false;
    for (const auto& cmd : gfx->commands) {
        if (cmd.kind == pix::DrawCommand::BeginHole) hasHole = true;
    }
    CHECK(hasHole);
}

