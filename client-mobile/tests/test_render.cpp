// Host tests for the M4 rendering port: camera transform, terrain generation
// (bit-compatible with shared/utils/terrainGen.ts), renderer z-sorting, the
// object pool, and structure mask transforms.
#include "ReferenceData.h"
#include "TestFramework.h"

#include "game/GameWorld.h"
#include "game/Map.h"
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
}
