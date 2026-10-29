// Host tests for the M4 rendering port: camera transform, terrain generation
// (bit-compatible with shared/utils/terrainGen.ts), renderer z-sorting, the
// object pool, and structure mask transforms.
#include "ReferenceData.h"
#include "TestFramework.h"
#include "FakeConnection.h"

#include "audio/Ambiance.h"
#include "audio/AudioManager.h"
#include "audio/GeneratedSoundDefs.h"
#include "core/GameConfig.h"
#include "game/GameWorld.h"
#include "game/Map.h"
#include "game/Gas.h"
#include "game/Game.h"
#include "game/Hud.h"
#include "game/RoleFeed.h"
#include "game/objects/Barns.h"
#include "game/objects/GameObject.h"
#include "game/objects/ObstacleScan.h"
#include "game/objects/Structure.h"
#include "render/Camera.h"
#include "render/Defs.h"
#include "render/GeneratedDefs.h"
#include "render/NullPixi.h"
#include "render/Renderer.h"
#include "render/Terrain.h"

#include <cmath>
#include <memory>
#include <string>
#include <unordered_map>
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

TEST(renderer_world_root_transform) {
    pix::NullPixiFactory factory;
    Renderer renderer(&factory, false);
    auto* root = static_cast<pix::NullContainer*>(factory.createContainer());
    Camera camera;
    camera.m_pos = Vec2(100.0f, 100.0f);
    camera.m_screenWidth = 800.0f;
    camera.m_screenHeight = 600.0f;
    camera.m_ppu = 16.0f;
    camera.m_zoom = 1.5f;

    // renderer.ts: the container's origin is world (0,0), so it goes to
    // pointToScreen(0,0) and carries a POSITIVE y scale in axmol's y-up scene
    // graph (m_pointToScreen is PIXI y-down, so the origin is mirrored).
    // m_pointToScreen(m_pos) would pin the world origin to the centre and the
    // camera would never scroll.
    renderer.applyCamera(root, camera);
    CHECK_NEAR(root->sx, 24.0f, 1e-4f);
    CHECK_NEAR(root->sy, 24.0f, 1e-4f);
    CHECK_NEAR(root->x, 400.0f - 100.0f * 24.0f, 1e-4f);
    CHECK_NEAR(root->y, 600.0f - (300.0f + 100.0f * 24.0f), 1e-4f);

    // World +y must be UP on screen (the old negative scale mirrored the map
    // and made the joystick's up/down feel inverted). Convert the root's
    // y-up scene position to a y-down visual coordinate.
    const auto visualY = [&](float worldY) {
        return camera.m_screenHeight - (root->y + root->sy * worldY);
    };
    CHECK_NEAR(visualY(camera.m_pos.y), 300.0f, 1e-4f); // camera y -> centre
    CHECK(visualY(camera.m_pos.y + 10.0f) < visualY(camera.m_pos.y));

    camera.m_zoom = 0.75f;
    renderer.applyCamera(root, camera);
    CHECK_NEAR(root->sx, 12.0f, 1e-4f);
    CHECK_NEAR(root->sy, 12.0f, 1e-4f);
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

// building.ts floor surfaces feed map.ts getGroundSurface: stepping into a
// building must report its floor material (not the terrain's grass), which is
// what selects the `footstep_<type>` sound group.
TEST(building_floor_surfaces) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    world.camera().m_screenWidth = 800.0f;
    world.camera().m_screenHeight = 600.0f;

    MapMsg msg;
    msg.mapName = "main";
    msg.seed = 123456;
    msg.width = 512.0f;
    msg.height = 512.0f;
    msg.shoreInset = 4.0f;
    msg.grassInset = 2.0f;
    world.loadMap(msg);

    const DefProvider* defs = getDefProvider();
    CHECK(defs != nullptr);
    const MapObjectDef* bank = defs ? defs->mapObject("bank_01") : nullptr;
    CHECK(bank != nullptr);
    if (!bank) {
        return;
    }
    // bank_01: one "tile" floor surface made of two collision AABBs.
    CHECK_EQ(bank->surfaces.size(), 1u);
    CHECK_EQ(bank->surfaces[0].type, std::string("tile"));
    CHECK_EQ(bank->surfaces[0].colliders.size(), 2u);
    CHECK(!bank->surfaces[0].hasWaterColor);
    // mapHelpers.getBoundingCollider() replaces the old (zero) collision-only
    // bounds; buildings need this for the minimap fallback and ceiling vision.
    CHECK(bank->hasBounding);
    CHECK(bank->boundingCollider.max.x > bank->boundingCollider.min.x);
    CHECK(bank->boundingCollider.max.y > bank->boundingCollider.min.y);

    const Vec2 origin(100.0f, 100.0f);
    Building* b = world.map().buildingPool.m_alloc();
    ObjectData data;
    data.type = "bank_01";
    data.pos = origin;
    data.ori = 0;
    data.layer = 0;
    data.__id = 7;
    b->m_updateData(data, true, true, world);

    CHECK_EQ(b->surfaces.size(), 1u);
    CHECK_EQ(b->surfaces[0].type, std::string("tile"));
    CHECK_EQ(b->surfaces[0].colliders.size(), 2u);

    // The authored colliders are building-local AABBs; they must be translated
    // by the building position (and mapped to the correct surface type).
    const Collider& local = bank->surfaces[0].colliders[0];
    CHECK_NEAR(b->surfaces[0].colliders[0].min.x, local.min.x + origin.x, 1e-4f);
    CHECK_NEAR(b->surfaces[0].colliders[0].max.y, local.max.y + origin.y, 1e-4f);

    const Vec2 surfaceCenter = v2Add(
        origin, v2Mul(v2Add(local.min, local.max), 0.5f));
    const Map::GroundSurface tile = world.map().getGroundSurface(surfaceCenter, 0);
    CHECK(tile.type == Map::SurfaceType::Tile);
    CHECK_EQ(std::string(surfaceTypeName(tile.type)), std::string("tile"));
    CHECK_EQ(std::string("footstep_") + surfaceTypeName(tile.type),
             std::string("footstep_tile"));

    // A bunker water surface carries its own water colours.
    const MapObjectDef* sub = defs->mapObject("bunker_cloud_sublevel_01");
    CHECK(sub != nullptr);
    if (sub) {
        const BuildingSurfaceDef* water = nullptr;
        for (const auto& s : sub->surfaces) {
            if (s.type == "water") {
                water = &s;
            }
        }
        CHECK(water != nullptr);
        if (water) {
            CHECK(water->hasWaterColor);
            CHECK_EQ(water->waterColor, 0x18425Bu);
            CHECK_EQ(water->rippleColor, 0xb3f0ffu);
            Building* wb = world.map().buildingPool.m_alloc();
            ObjectData wd;
            wd.type = "bunker_cloud_sublevel_01";
            wd.pos = Vec2(300.0f, 300.0f);
            wd.layer = 1;
            wd.__id = 8;
            wb->m_updateData(wd, true, true, world);
            CHECK_EQ(wb->surfaces.size(), sub->surfaces.size());
            // The last surface is the authored water pool; its data and
            // colliders survive the transform (getGroundSurface reports the
            // first overlapping surface, exactly like map.ts).
            const Building::Surface& wsurface = wb->surfaces.back();
            CHECK_EQ(wsurface.type, std::string("water"));
            CHECK(wsurface.hasWaterColor);
            CHECK_EQ(wsurface.waterColor, 0x18425Bu);
            CHECK(!wsurface.colliders.empty());
        }
    }
}

// structure.ts / player.ts isUnderground + the structure mask/stairs queries
// that renderer/audio use for the underground fade and reverb.
TEST(structure_mask_and_underground) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);

    const DefProvider* defs = getDefProvider();
    CHECK(defs != nullptr);
    const MapObjectDef* def = defs ? defs->mapObject("bunker_structure_02") : nullptr;
    CHECK(def != nullptr);
    if (!def) {
        return;
    }
    CHECK(def->layers.size() >= 2u);
    CHECK(!def->mask.empty());
    CHECK(!def->stairs.empty());
    CHECK(def->hasBounding);
    CHECK(def->boundingCollider.max.x > def->boundingCollider.min.x);

    Structure* s = world.map().structurePool.m_alloc();
    ObjectData data;
    data.type = "bunker_structure_02";
    data.pos = Vec2(200.0f, 200.0f);
    data.ori = 0;
    data.layer = 0;
    data.__id = 3;
    data.layerObjIds[0] = 10;
    data.layerObjIds[1] = 11;
    s->m_updateData(data, true, true, world);

    CHECK_EQ(s->layers.size(), def->layers.size());
    CHECK_EQ(s->mask.size(), def->mask.size());
    CHECK_EQ(s->stairs.size(), def->stairs.size());
    CHECK_EQ(s->layers[0].objId, 10);
    CHECK_EQ(s->layers[1].objId, 11);

    // The structure queries hit on the authored geometry.
    CHECK(world.map().insideStructureMask(s->mask[0]));
    CHECK(world.map().insideStructureStairs(s->stairs[0].collision));
    CHECK(!world.map().insideStructureMask(Collider::createCircle(Vec2(-10000.0f, -10000.0f), 0.5f)));

    // player.ts isUnderground():
    // - only layer 1 can be underground;
    // - only the second structure layer is tested, using the player radius;
    // - a layer-1 point that matches no structure defaults to underground.
    const Structure::Layer& l0 = s->layers[0];
    const Structure::Layer& l1 = s->layers[1];
    const Vec2 center = v2Mul(v2Add(l1.collision.min, l1.collision.max), 0.5f);
    // structure.ts defaults `underground` to `i == 1` when the def omits it.
    CHECK(!def->layers[0].hasUnderground);
    CHECK(!def->layers[1].hasUnderground);
    CHECK(!l0.underground);
    CHECK(l1.underground);
    // The layer collision is the layer type's real bounding collider (not a
    // zero-size AABB), so the player inside it is resolved.
    CHECK(l1.collision.max.x > l1.collision.min.x);
    CHECK(l1.collision.max.y > l1.collision.min.y);
    CHECK(world.map().isUnderground(center, 1, 0.5f));
    CHECK(!world.map().isUnderground(center, 0, 0.5f));
    // Outside every second layer the web default is underground = true.
    CHECK(world.map().isUnderground(Vec2(-10000.0f, -10000.0f), 1, 0.5f));
    // Radius: a point 0.75 outside the layer AABB still misses with rad 0.5.
    const Vec2 near(l1.collision.max.x + 0.75f, center.y);
    CHECK(world.map().isUnderground(near, 1, 0.5f));
    CHECK(world.map().isUnderground(near, 1, 1.0f));
}

// building.ts ceiling reveal uses collisionHelpers.scanCollider (obstacle
// filtered rays) and player.ts noCeilingRevealTicker.
TEST(building_ceiling_scan_and_no_reveal) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);

    MapMsg msg;
    msg.mapName = "main";
    msg.seed = 123456;
    msg.width = 512.0f;
    msg.height = 512.0f;
    msg.shoreInset = 4.0f;
    msg.grassInset = 2.0f;
    world.loadMap(msg);

    const MapObjectDef* room =
        getDefProvider() ? getDefProvider()->mapObject("bathhouse_sideroom_02") : nullptr;
    CHECK(room != nullptr);
    if (!room) {
        return;
    }
    CHECK_EQ(room->ceilingZoomIn.size(), 1u);

    Building* b = world.map().buildingPool.m_alloc();
    ObjectData bd;
    bd.type = "bathhouse_sideroom_02";
    bd.pos = Vec2(100.0f, 100.0f);
    bd.ori = 0;
    bd.layer = 0;
    bd.__id = 9;
    b->m_updateData(bd, true, true, world);
    CHECK(!b->ceilingRegions.empty());

    Player ap;
    ap.m_init();
    ObjectData pd;
    pd.pos = Vec2(83.0f, 100.0f); // 3 units left of the room's only zoomIn AABB
    pd.dir = Vec2(1.0f, 0.0f);
    pd.outfit = "outfitBase";
    pd.activeWeapon = "fists";
    pd.layer = 0;
    pd.__id = 5;
    ap.m_updateData(pd, true, true, world);
    world.setActivePlayer(&ap);

    // Clear line of sight: the ceiling reveal starts and fades in.
    b->ceilingVisionTicker = 0.0f;
    b->ceilingFadeAlpha = 1.0f;
    b->update(0.016f, world);
    CHECK(b->ceilingVisionTicker > 0.0f);
    CHECK(b->ceilingFadeAlpha < 1.0f);

    // A wall between the player and the zoomIn region blocks the reveal.
    Obstacle* wall = world.map().obstaclePool.m_alloc();
    wall->__id = 77;
    wall->pos = Vec2(84.5f, 100.0f);
    wall->collider = Collider::createAabb(Vec2(84.0f, 85.0f), Vec2(85.0f, 115.0f));
    wall->height = 1.0f;
    wall->layer = 0;
    wall->collidable = true;
    wall->isWindow = false;
    wall->dead = false;

    b->ceilingVisionTicker = 0.0f;
    b->update(0.016f, world);
    CHECK(b->ceilingVisionTicker <= 0.0f);

    // Direct check: the wall blocks the scanCollider rays, but the reveal
    // spots are still reachable when the wall is transparent.
    const auto& obstacles = world.map().obstaclePool.m_getPool();
    CHECK(!scanCollider(b->ceilingRegions[0], obstacles, ap.pos, 0, 0.5f,
                        b->vision.width * 2.0f, b->vision.dist, 5));

    // Windows and low obstacles do not block the scan (collisionHelpers).
    wall->isWindow = true;
    b->ceilingVisionTicker = 0.0f;
    b->update(0.016f, world);
    CHECK(b->ceilingVisionTicker > 0.0f);
    wall->isWindow = false;
    wall->collidable = false;
    b->ceilingVisionTicker = 0.0f;
    b->update(0.016f, world);
    CHECK(b->ceilingVisionTicker > 0.0f);
    wall->collidable = true;
    wall->height = 0.25f; // below the scan height (0.5)
    b->ceilingVisionTicker = 0.0f;
    b->update(0.016f, world);
    CHECK(b->ceilingVisionTicker > 0.0f);
    wall->height = 1.0f;
    world.map().obstaclePool.m_free(wall);

    // noCeilingRevealTicker suppresses the reveal (player.ts stairs hack).
    ap.noCeilingRevealTicker = 0.25f;
    b->ceilingVisionTicker = 0.0f;
    b->ceilingFadeAlpha = 1.0f;
    b->update(0.016f, world);
    CHECK(b->ceilingVisionTicker <= 0.0f);

    // ...and suppresses the immediate stairs reveal (ap.layer & 2).
    ap.layer = 3;
    ap.noCeilingRevealTicker = 0.25f;
    b->ceilingFadeAlpha = 1.0f;
    b->update(0.016f, world);
    CHECK(b->ceilingFadeAlpha > 0.5f);
    ap.layer = 0;
    ap.noCeilingRevealTicker = 0.0f;
    world.setActivePlayer(nullptr);
}

// player.ts updateRenderLayer: crossing a noCeilingReveal stair sets the
// 0.25s suppression ticker (only for the active player, only off layer 0).
TEST(player_stairs_no_ceiling_reveal) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);

    const MapObjectDef* def =
        getDefProvider() ? getDefProvider()->mapObject("mansion_structure_01") : nullptr;
    CHECK(def != nullptr);
    if (!def) {
        return;
    }

    Structure* s = world.map().structurePool.m_alloc();
    ObjectData sd;
    sd.type = "mansion_structure_01";
    sd.pos = Vec2(200.0f, 200.0f);
    sd.ori = 0;
    sd.layer = 0;
    sd.__id = 4;
    sd.layerObjIds[0] = 1;
    sd.layerObjIds[1] = 2;
    s->m_updateData(sd, true, true, world);

    const Stair* stair = nullptr;
    for (const auto& st : s->stairs) {
        if (st.noCeilingReveal) {
            stair = &st;
            break;
        }
    }
    CHECK(stair != nullptr);
    if (!stair) {
        return;
    }
    const Vec2 center = v2Mul(v2Add(stair->collision.min, stair->collision.max), 0.5f);

    Player p;
    p.m_init();
    ObjectData d;
    d.pos = center;
    d.dir = Vec2(1.0f, 0.0f);
    d.outfit = "outfitBase";
    d.activeWeapon = "fists";
    d.layer = 1;
    p.m_updateData(d, true, true, world);
    world.setActivePlayer(&p);

    p.noCeilingRevealTicker = 0.0f;
    p.update(0.016f, world);
    CHECK(p.noCeilingRevealTicker > 0.0f);

    // Layer 0 never suppresses.
    p.layer = 0;
    p.noCeilingRevealTicker = 0.0f;
    p.update(0.016f, world);
    CHECK(p.noCeilingRevealTicker <= 0.0f);

    // Non-active players do not update the ticker.
    p.layer = 1;
    p.noCeilingRevealTicker = 0.0f;
    world.setActivePlayer(nullptr);
    p.update(0.016f, world);
    CHECK(p.noCeilingRevealTicker <= 0.0f);
}

// structure.ts updateInteriorSounds(): the interior_0/interior_1 ambience
// tracks follow the building distance, ceiling fade, layer and transition.
TEST(structure_interior_ambience) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);

    const MapObjectDef* def =
        getDefProvider() ? getDefProvider()->mapObject("reserve_structure_01") : nullptr;
    CHECK(def != nullptr);
    if (!def) {
        return;
    }
    CHECK(def->hasInteriorSound);
    CHECK_EQ(def->interiorSound.sound, std::string("reserve_music_01"));
    CHECK_EQ(def->interiorSound.soundAlt, std::string("reserve_music_02"));
    CHECK_EQ(def->interiorSound.filter, std::string("club"));
    CHECK_NEAR(def->interiorSound.transitionTime, 1.0f, 1e-4f);
    CHECK_NEAR(def->interiorSound.outsideMaxDist, 20.0f, 1e-4f);
    CHECK_NEAR(def->interiorSound.outsideVolume, 0.25f, 1e-4f);
    CHECK(def->interiorSound.hasUndergroundVolume);
    CHECK_NEAR(def->interiorSound.undergroundVolume, 0.707f, 1e-4f);

    surv::audio::Ambiance ambience;
    world.setAudio(nullptr, &ambience); // the track setters do not need a backend

    Structure* s = world.map().structurePool.m_alloc();
    ObjectData sd;
    sd.type = "reserve_structure_01";
    sd.pos = Vec2(200.0f, 200.0f);
    sd.ori = 0;
    sd.layer = 0;
    sd.__id = 3;
    sd.layerObjIds[0] = 11;
    sd.layerObjIds[1] = 12;
    sd.interiorSoundAlt = false;
    sd.interiorSoundEnabled = true;
    s->m_updateData(sd, true, true, world);
    CHECK_EQ(s->layers.size(), 2u);

    const auto makeBuilding = [&](uint16_t id, const char* type) {
        Building* b = world.map().buildingPool.m_alloc();
        ObjectData bd;
        bd.type = type;
        bd.pos = Vec2(100.0f, 100.0f);
        bd.ori = 0;
        bd.layer = 0;
        bd.__id = id;
        b->m_updateData(bd, true, true, world);
        b->__id = id;
        b->ceilingFadeAlpha = 1.0f;
        return b;
    };
    Building* b0 = makeBuilding(11, "barn_01");
    Building* b1 = makeBuilding(12, "barn_basement_floor_01");
    CHECK(!b0->ceilingRegions.empty());
    CHECK(!b1->ceilingRegions.empty());

    Player ap;
    ap.m_init();
    ObjectData pd;
    pd.pos = Vec2(100.0f, 100.0f);
    pd.dir = Vec2(1.0f, 0.0f);
    pd.outfit = "outfitBase";
    pd.activeWeapon = "fists";
    pd.layer = 0;
    pd.__id = 5;
    ap.m_updateData(pd, true, true, world);
    world.setActivePlayer(&ap);

    // Outside with the ceiling opaque: the filtered track plays (outsideVolume).
    b0->ceilingFadeAlpha = 1.0f;
    s->soundTransitionT = 0.0f;
    s->soundEnabledT = 1.0f;
    s->interiorSoundAlt = false;
    s->interiorSoundEnabled = true;
    s->update(0.016f, world);
    CHECK_EQ(ambience.trackSound("interior_0"), std::string("reserve_music_01"));
    CHECK_EQ(ambience.trackSound("interior_1"), std::string("reserve_music_01"));
    CHECK_EQ(ambience.trackFilter("interior_1"), std::string("club"));
    CHECK(ambience.trackWeight("interior_0") <= 0.0001f);
    CHECK_NEAR(ambience.trackWeight("interior_1"), 0.25f, 0.01f);

    // Seeing inside (ceiling faded): the un-filtered track takes over.
    b0->ceilingFadeAlpha = 0.0f;
    s->update(0.016f, world);
    CHECK_NEAR(ambience.trackWeight("interior_0"), 1.0f, 0.01f);
    CHECK(ambience.trackWeight("interior_1") <= 0.0001f);

    // Underground (layer 1, second building): undergroundVolume, no track 0.
    ap.layer = 1;
    b0->ceilingFadeAlpha = 1.0f;
    b1->ceilingFadeAlpha = 1.0f;
    s->update(0.016f, world);
    CHECK(ambience.trackWeight("interior_0") <= 0.0001f);
    CHECK_NEAR(ambience.trackWeight("interior_1"), 0.707f, 0.01f);

    // soundAlt transition: 0 -> 1 over transitionTime, then the alt track.
    s->interiorSoundAlt = true;
    s->soundTransitionT = 0.0f;
    s->update(0.5f, world);
    CHECK_NEAR(s->soundTransitionT, 0.5f, 0.01f);
    CHECK_EQ(ambience.trackSound("interior_1"), std::string("reserve_music_01"));
    s->update(0.6f, world);
    CHECK_NEAR(s->soundTransitionT, 1.0f, 0.01f);
    CHECK_EQ(ambience.trackSound("interior_0"), std::string("reserve_music_02"));
    CHECK_EQ(ambience.trackSound("interior_1"), std::string("reserve_music_02"));

    // interiorSoundEnabled=false fades soundEnabledT (and the weights) out.
    s->interiorSoundEnabled = false;
    s->update(1.0f, world);
    CHECK(s->soundEnabledT < 1.0f);
    CHECK(ambience.trackWeight("interior_1") < 0.707f);

    world.setActivePlayer(nullptr);
}

// building.ts soundEmitters: looping positional ambience (bathhouse steam).
TEST(building_sound_emitters) {
    installGeneratedDefs();
    audio::installGeneratedSoundDefs();

    class ProbeBackend : public audio::AudioBackend {
    public:
        struct Played {
            std::string id;
            bool loop = false;
            float volume = 0.0f;
        };
        std::vector<Played> played;
        std::unordered_map<audio::SoundHandle, float> volumes;
        std::vector<audio::SoundHandle> stopped;
        audio::SoundHandle next = 1;

        bool load(const std::string&, const std::string&) override { return true; }
        bool isLoaded(const std::string&) const override { return true; }
        audio::SoundHandle play(const std::string& id, const audio::PlayOptions& opts) override {
            played.push_back({id, opts.loop, opts.volume});
            volumes[next] = opts.volume;
            return next++;
        }
        void stop(audio::SoundHandle handle) override { stopped.push_back(handle); }
        void stopAll() override {}
        void setVolume(audio::SoundHandle handle, float volume) override { volumes[handle] = volume; }
        float getVolume(audio::SoundHandle handle) const override {
            auto it = volumes.find(handle);
            return it == volumes.end() ? 0.0f : it->second;
        }
        void setPan(audio::SoundHandle, float) override {}
        void setMute(bool) override {}
        bool isPlaying(audio::SoundHandle) const override { return true; }
    };

    ProbeBackend backend;
    audio::AudioManager audioManager(&backend);
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    world.setAudio(&audioManager, nullptr);

    const MapObjectDef* def =
        getDefProvider() ? getDefProvider()->mapObject("bathhouse_sideroom_01") : nullptr;
    CHECK(def != nullptr);
    if (!def) {
        return;
    }
    CHECK_EQ(def->soundEmitters.size(), 1u);
    if (def->soundEmitters.size() != 1u) {
        return;
    }
    const BuildingSoundEmitterDef& se = def->soundEmitters[0];
    CHECK_EQ(se.sound, std::string("ambient_steam_01"));
    CHECK_EQ(se.channel, std::string("ambient"));
    CHECK_NEAR(se.rangeMin, 9.0f, 1e-4f);
    CHECK_NEAR(se.rangeMax, 16.0f, 1e-4f);
    CHECK_NEAR(se.falloff, 1.0f, 1e-4f);
    CHECK_NEAR(se.volume, 0.2f, 1e-4f);

    audioManager.loadSound("ambient_steam_01", "ambient", "audio/ambient/ambient_steam_01.mp3");

    Building* b = world.map().buildingPool.m_alloc();
    ObjectData bd;
    bd.type = "bathhouse_sideroom_01";
    bd.pos = Vec2(150.0f, 160.0f);
    bd.ori = 0;
    bd.layer = 0;
    bd.__id = 21;
    b->m_updateData(bd, true, true, world);
    CHECK_EQ(b->soundEmitters.size(), 1u);
    if (b->soundEmitters.empty()) {
        return;
    }
    CHECK_NEAR(b->soundEmitters[0].pos.x, 150.0f, 1e-4f);
    CHECK_NEAR(b->soundEmitters[0].pos.y, 160.0f, 1e-4f);
    CHECK(b->soundEmitters[0].instance == audio::kInvalidSound);

    Player ap;
    ap.m_init();
    ObjectData pd;
    pd.pos = Vec2(0.0f, 0.0f); // away from the building: no ceiling reveal
    pd.dir = Vec2(1.0f, 0.0f);
    pd.outfit = "outfitBase";
    pd.activeWeapon = "fists";
    pd.layer = 0;
    pd.__id = 5;
    ap.m_updateData(pd, true, true, world);
    world.setActivePlayer(&ap);
    world.camera().m_pos = Vec2(150.0f, 160.0f);

    // First tick loads/plays the looping track silently, then sets its volume.
    const auto tick = [&]() {
        // Tick the sound emitter without meaningfully advancing the fade.
        b->soundEmitterTicker = 0.101f;
        b->ceilingVisionTicker = -10.0f;
        b->update(0.0002f, world);
    };
    b->ceilingFadeAlpha = 1.0f;
    tick();
    CHECK_EQ(backend.played.size(), 1u);
    if (!backend.played.empty()) {
        CHECK_EQ(backend.played[0].id, std::string("ambient_steam_01ambient"));
        CHECK(backend.played[0].loop);
        CHECK_NEAR(backend.played[0].volume, 0.0f, 1e-6f); // startSilent
    }
    const audio::SoundHandle handle = b->soundEmitters[0].instance;
    CHECK(handle != audio::kInvalidSound);
    // baseVolume 0.5 * masterVolume 0.5 * emitter volume 0.2 * distT 1 *
    // visibility 0.25.
    CHECK_NEAR(backend.getVolume(handle), 0.0125f, 1e-4f);

    // Halfway through the range: distT = 0.5.
    world.camera().m_pos = Vec2(162.5f, 160.0f);
    b->ceilingFadeAlpha = 1.0f;
    tick();
    CHECK_NEAR(backend.getVolume(handle), 0.00625f, 1e-4f);

    // Seeing inside (ceiling faded) raises the visibility multiplier to 1.
    b->ceilingFadeAlpha = 0.0f;
    tick();
    CHECK_NEAR(backend.getVolume(handle), 0.025f, 1e-3f);

    // Wrong audio layer mutes the emitter.
    ap.layer = 1;
    b->ceilingFadeAlpha = 0.0f;
    tick();
    CHECK_NEAR(backend.getVolume(handle), 0.0f, 1e-6f);
    ap.layer = 0;

    // Beyond the max range: no audible volume.
    world.camera().m_pos = Vec2(200.0f, 160.0f);
    b->ceilingFadeAlpha = 0.0f;
    tick();
    CHECK_NEAR(backend.getVolume(handle), 0.0f, 1e-6f);

    // The 10 Hz gate: a short frame does not re-run the volume update.
    world.camera().m_pos = Vec2(150.0f, 160.0f);
    b->ceilingFadeAlpha = 0.0f;
    b->update(0.01f, world);
    CHECK_NEAR(backend.getVolume(handle), 0.0f, 1e-6f); // still gated
    b->soundEmitterTicker = 0.1f;
    b->ceilingFadeAlpha = 0.0f;
    b->ceilingVisionTicker = -10.0f;
    b->update(0.001f, world); // ticker crossed 0.1 -> ~0.1 at dist 0
    CHECK(backend.getVolume(handle) > 0.045f);

    // Freeing stops the loop and drops the emitters.
    b->m_free();
    CHECK(b->soundEmitters.empty());
    CHECK_EQ(backend.stopped.size(), 1u);
    if (!backend.stopped.empty()) {
        CHECK_EQ(backend.stopped[0], handle);
    }

    world.setActivePlayer(nullptr);
}

// Automated footstep verification: the player standing on a building's tile
// floor must request the footstep_tile sound group (the on-device equivalent is
// the sample call; this asserts the selection without listening).
TEST(player_footstep_uses_floor_surface) {
    installGeneratedDefs();
    audio::installGeneratedSoundDefs();

    class FootstepBackend : public audio::AudioBackend {
    public:
        struct Played {
            std::string id;
            std::string filter;
            float volume = 0.0f;
        };
        std::vector<Played> played;
        audio::SoundHandle next = 1;

        bool load(const std::string&, const std::string&) override { return true; }
        bool isLoaded(const std::string&) const override { return true; }
        audio::SoundHandle play(const std::string& id, const audio::PlayOptions& opts) override {
            played.push_back({id, opts.filter, opts.volume});
            return next++;
        }
        void stop(audio::SoundHandle) override {}
        void stopAll() override {}
        void setVolume(audio::SoundHandle, float) override {}
        float getVolume(audio::SoundHandle) const override { return 0.0f; }
        void setPan(audio::SoundHandle, float) override {}
        void setMute(bool) override {}
        bool isPlaying(audio::SoundHandle) const override { return true; }
    };

    FootstepBackend backend;
    audio::AudioManager audioManager(&backend);
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    world.setAudio(&audioManager, nullptr);
    audioManager.activeLayer = 0;

    const MapObjectDef* bank = getDefProvider()->mapObject("bank_01");
    CHECK(bank != nullptr);
    if (!bank || bank->surfaces.empty() || bank->surfaces[0].colliders.empty()) {
        return;
    }
    const Vec2 origin(100.0f, 100.0f);
    Building* b = world.map().buildingPool.m_alloc();
    ObjectData bd;
    bd.type = "bank_01";
    bd.pos = origin;
    bd.layer = 0;
    bd.__id = 31;
    b->m_updateData(bd, true, true, world);

    const Collider& tile = bank->surfaces[0].colliders[0];
    const Vec2 center = v2Add(origin, v2Mul(v2Add(tile.min, tile.max), 0.5f));

    Player p;
    p.m_init();
    ObjectData pd;
    pd.pos = center;
    pd.dir = Vec2(1.0f, 0.0f);
    pd.outfit = "outfitBase";
    pd.activeWeapon = "fists";
    pd.layer = 0;
    pd.__id = 7;
    p.m_updateData(pd, true, true, world);
    world.setActivePlayer(&p);
    world.camera().m_pos = pd.pos;
    audioManager.cameraPos = pd.pos; // GameWorld normally copies camera -> audio each frame

    // Prime posOld, then move far enough for a step (>4 units).
    p.update(0.0f, world);
    p.pos = v2Add(center, Vec2(6.0f, 0.0f));
    world.camera().m_pos = p.pos;
    audioManager.cameraPos = p.pos;
    p.update(0.016f, world);

    bool footstepTile = false;
    bool reverbFilter = false;
    for (const auto& play : backend.played) {
        if (play.id.rfind("footstep_tile_", 0) == 0) {
            footstepTile = true;
            reverbFilter = play.filter == "reverb"; // same layer -> reverb send
        }
    }
    CHECK(footstepTile);
    CHECK(reverbFilter);
    // No grass footsteps were requested on the tile floor.
    for (const auto& play : backend.played) {
        CHECK(play.id.rfind("footstep_grass_", 0) != 0);
    }

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
pix::NullText* text(pix::Text* t) { return static_cast<pix::NullText*>(t); }
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
    // Gun offsets are PIXI y-down in player.ts (-1.75 is up); the native hand
    // container is y-up, so the converted position is +1.75.
    CHECK_NEAR(container(p.gunContainers[1])->y, 1.75f, 1e-4f);
    d.dir = Vec2(0, 1);
    p.m_updateData(d, false, false, world);
    p.update(0, world);
    CHECK_NEAR(container(p.bodyContainer)->rotation, -3.14159265358979f / 2, 1e-4f);
    CHECK_NEAR(container(p.container)->rotation, 0, 1e-4f); // name stays upright
    CHECK_EQ(p.activeWeapon, std::string("ak47"));
    CHECK_EQ(p.container->getSortOrd(), 18);
    // World-space labels are y-up like the corrected camera root (no
    // counter-flip; the old negative-Y camera needed one).
    CHECK(p.nameText != nullptr);
    CHECK(!text(p.nameText)->flipY);
}

// player.ts stores bone/art offsets in PIXI's y-down local frame; the native
// player container is y-up. Without the y conversion the left/right hands are
// swapped across the facing axis (visible with distinct hand art or a gun).
TEST(player_hand_side_matches_web) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    Player p;
    p.m_init();
    auto d = playerData();
    d.activeWeapon = "fists"; // fists: HandL at y -12.25, HandR at y +12.25

    // Facing right (dir 1,0): HandL is north/up in the y-up player frame,
    // HandR south/down (PIXI's HandL y=-12.25 is up on screen). This is the
    // regression check: the old raw PIXI y put HandL below.
    d.dir = Vec2(1.0f, 0.0f);
    p.m_updateData(d, true, true, world);
    p.update(0, world);
    const auto* handL = container(p.boneContainers[0]);
    const auto* handR = container(p.boneContainers[1]);
    CHECK(handL->y > 0.0f);
    CHECK(handR->y < 0.0f);

    // Facing up (dir 0,1): the body rotation (-90deg clockwise in axmol)
    // puts HandL on screen-left and HandR on screen-right.
    d.dir = Vec2(0.0f, 1.0f);
    p.m_updateData(d, true, false, world);
    p.update(0, world);
    const float bodyRot = -std::atan2(1.0f, 0.0f);
    const float c = std::cos(bodyRot);
    const float s = std::sin(bodyRot);
    const float leftX = handL->x * c + handL->y * s; // axmol clockwise rotation
    const float rightX = handR->x * c + handR->y * s;
    CHECK(leftX < 0.0f);
    CHECK(rightX > 0.0f);
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

// P0.3: player.ts interpolates m_visualPos/m_visualDir between net updates;
// the container (and the camera in GameWorld::update) follow the visual copy.
TEST(player_visual_pos_interpolates) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    world.camera().m_interpInterval = 0.1f;

    Player p;
    p.m_init();
    auto d = playerData();
    p.m_updateData(d, true, true, world);
    p.update(0.0f, world);
    CHECK_NEAR(p.visualPos.x, 50.0f, 1e-5f);
    CHECK_NEAR(container(p.container)->x, 50.0f, 1e-5f);

    // A net move seeds the old visual position, then the frame lerps halfway.
    d.pos = Vec2(60.0f, 50.0f);
    d.dir = Vec2(0.0f, 1.0f);
    p.m_updateData(d, true, false, world);
    CHECK_NEAR(p.pos.x, 60.0f, 1e-5f);
    CHECK_NEAR(p.visualPosOld.x, 50.0f, 1e-5f);
    CHECK_NEAR(p.posInterpTicker, 0.0f, 1e-5f);
    CHECK_NEAR(p.visualDirOld.x, 1.0f, 1e-5f);
    p.update(0.05f, world);
    CHECK_NEAR(p.visualPos.x, 55.0f, 1e-4f);
    CHECK_NEAR(container(p.container)->x, 55.0f, 1e-4f);
    // visualDir (0.5, 0.5) => body rotation -pi/4.
    CHECK_NEAR(container(p.bodyContainer)->rotation, -3.14159265358979f / 4.0f, 1e-4f);
    p.update(0.06f, world);
    CHECK_NEAR(p.visualPos.x, 60.0f, 1e-4f);

    // Disabling interpolation (the config toggle) snaps to the net position.
    world.setInterpolationEnabled(false);
    d.pos = Vec2(70.0f, 50.0f);
    p.m_updateData(d, true, false, world);
    p.update(0.0f, world);
    CHECK_NEAR(p.visualPos.x, 70.0f, 1e-5f);
    CHECK(world.camera().m_interpEnabled == false);

    // A duplicated net update re-seeds the start position from the current net
    // position (player.ts: `m_visualPosOld = isNew ? data.pos : this.m_pos`), so
    // it cannot slide backwards and the ticker restarts at the settled target.
    world.setInterpolationEnabled(true);
    d.pos = Vec2(80.0f, 50.0f);
    p.m_updateData(d, true, false, world);
    p.update(0.04f, world);
    CHECK(p.posInterpTicker > 0.0f);
    p.m_updateData(d, true, false, world);
    CHECK_NEAR(p.visualPosOld.x, 80.0f, 1e-5f);
    CHECK_NEAR(p.posInterpTicker, 0.0f, 1e-5f);
    p.update(0.04f, world);
    CHECK_NEAR(p.visualPos.x, 80.0f, 1e-5f);
}

// P0.3: projectile.ts lerps the visual position in the barn render pass.
TEST(projectile_visual_pos_interpolates) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    world.camera().m_interpInterval = 0.1f;

    Projectile* p = world.projectileBarn().projectilePool.m_alloc();
    p->__id = 7;
    p->__type = ObjectType_Projectile;
    ObjectData d;
    d.type = "proj_grenade";
    d.pos = Vec2(100.0f, 100.0f);
    d.layer = 0;
    d.scale = 1.0f;
    d.posZ = 2.0f;
    p->m_updateData(d, true, true, world);
    world.projectileBarn().update(0.0f, world);
    CHECK_NEAR(container(p->container)->x, 100.0f, 1e-5f);
    CHECK_NEAR(container(p->container)->y, 102.0f, 1e-5f);

    d.pos = Vec2(110.0f, 100.0f);
    p->m_updateData(d, true, false, world);
    CHECK_NEAR(p->visualPosOld.x, 100.0f, 1e-5f);
    world.projectileBarn().update(0.05f, world);
    CHECK_NEAR(container(p->container)->x, 105.0f, 1e-4f);
    CHECK_NEAR(container(p->container)->y, 102.0f, 1e-4f);
    world.projectileBarn().update(0.06f, world);
    CHECK_NEAR(container(p->container)->x, 110.0f, 1e-4f);
}

// P0.2: GameWorld::update feeds the active player's layer (and the underground
// flag) to the renderer before renderer.m_update(), so the bunker fade/mask and
// the ground fade track the same frame.
TEST(world_applies_active_layer_and_underground) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    world.camera().m_screenWidth = 800.0f;
    world.camera().m_screenHeight = 600.0f;
    MapMsg msg;
    msg.mapName = "main";
    msg.seed = 123456;
    msg.width = 512.0f;
    msg.height = 512.0f;
    msg.shoreInset = 4.0f;
    msg.grassInset = 2.0f;
    world.loadMap(msg);

    // A structure whose second layer is underground, covering the player.
    Structure* s = world.map().structurePool.m_alloc();
    s->layers.resize(2);
    s->layers[1].underground = true;
    s->layers[1].collision = Collider::createAabb(Vec2(40.0f, 40.0f), Vec2(60.0f, 60.0f));

    Player p;
    p.m_init();
    auto d = playerData();
    d.layer = 1;
    p.m_updateData(d, true, true, world);
    world.setActivePlayer(&p);
    world.setActivePlayerId(1);

    world.update(0.016f);
    CHECK_EQ(world.renderer().layer, 1);
    CHECK(world.renderer().layerAlpha > 0.0f);
    CHECK(world.renderer().underground);
    CHECK(world.renderer().groundAlpha > 0.0f);

    // The camera follows the interpolated visual position, not the raw one.
    p.visualPos = Vec2(51.0f, 52.0f);
    world.update(0.016f);
    CHECK_NEAR(world.camera().m_pos.x, p.visualPos.x, 1e-5f);
    CHECK_NEAR(world.camera().m_pos.y, p.visualPos.y, 1e-5f);

    // Leaving the structure drops the layer and lets both fades run back out.
    p.layer = 0;
    for (int i = 0; i < 200; i++) {
        world.update(0.016f);
    }
    CHECK_EQ(world.renderer().layer, 0);
    CHECK_NEAR(world.renderer().layerAlpha, 0.0f, 1e-3f);
    CHECK_NEAR(world.renderer().groundAlpha, 0.0f, 1e-3f);
    CHECK(!world.renderer().underground);
}

// P0.7: the game.ts config listeners (interpolation/shake/rotation/anon names)
// live on the world camera + player barn so every per-match world gets them.
TEST(gameworld_settings_flags) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);

    world.setInterpolationEnabled(false);
    world.setScreenShakeEnabled(false);
    world.setLocalRotationEnabled(true);
    world.setAnonPlayerNames(true);
    CHECK(!world.camera().m_interpEnabled);
    CHECK(!world.camera().m_shakeEnabled);
    CHECK(world.camera().m_localRotationEnabled);
    CHECK(world.anonPlayerNames());

    world.playerBarn().names[7] = "VisibleName";
    CHECK_EQ(world.playerBarn().getPlayerName(7, 1, true), std::string("Player"));
    CHECK_EQ(world.playerBarn().getPlayerName(7, 7, true), std::string("VisibleName"));

    world.setAnonPlayerNames(false);
    CHECK_EQ(world.playerBarn().getPlayerName(7, 1, world.anonPlayerNames()),
             std::string("VisibleName"));
}

TEST(player_downed_gear_and_pool_reuse) {    installGeneratedDefs();
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
    // shot.ts sets the recoil on the first ShotBarn tick (not in the update).
    world.shotBarn().update(0.0f, world);
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
    // obstacle.ts scales the sprite by camera.m_pixels(scale * imgScale) =
    // scale * imgScale * m_zoom. Sprites live in world units under the
    // camera-transformed root (m_ppu * m_zoom), so the native sprite scale is
    // that screen scale divided by m_ppu.
    CHECK_NEAR(sprite(o.sprite)->sx, 1.0f * barrelDef->img.scale / world.camera().m_ppu, 1e-4f);
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
    // obstacle.ts: closedPos/interpPos start at data.pos (the hinge).
    CHECK_NEAR(o.doorClosedPos.x, data.pos.x, 1e-4f);
    CHECK_NEAR(o.doorClosedPos.y, data.pos.y, 1e-4f);
    CHECK_NEAR(o.doorInterpPos.x, data.pos.x, 1e-4f);
    CHECK_NEAR(o.doorInterpPos.y, data.pos.y, 1e-4f);

    // A network move (door slides open) interpolates the visual position. The
    // casing stays at the closed slot (data.pos + slideOffset), while the door
    // itself remains at data.pos (the server's open position).
    data.doorOpen = true;
    data.pos = Vec2(20.0f, 30.0f);
    o.m_updateData(data, true, false, world);
    CHECK_NEAR(o.doorClosedPos.x, data.pos.x, 1e-4f);
    CHECK_NEAR(o.doorClosedPos.y, data.pos.y + 3.75f, 1e-4f);
    CHECK_NEAR(o.doorInterpPos.x, data.pos.x, 1e-4f);
    CHECK_NEAR(o.doorInterpPos.y, data.pos.y, 1e-4f);
    o.update(0.016f, world);
    o.render(world, 0);
    CHECK(o.casingSprite != nullptr);
    bool casingDrawn = false;
    for (pix::Node* child : static_cast<pix::NullContainer*>(world.renderer().layers[0])->children) {
        if (child == o.casingSprite) casingDrawn = true;
    }
    CHECK(casingDrawn);
    CHECK(sprite(o.casingSprite)->visible);

    // A door that is already open when first streamed in must render at its
    // data.pos, not one slideOffset (door width) next to it.
    Obstacle fresh;
    fresh.active = true;
    fresh.__id = 6;
    ObjectData openData = data;
    openData.pos = Vec2(50.0f, 60.0f);
    fresh.m_updateData(openData, true, true, world);
    CHECK_NEAR(fresh.doorInterpPos.x, openData.pos.x, 1e-4f);
    CHECK_NEAR(fresh.doorInterpPos.y, openData.pos.y, 1e-4f);
    CHECK_NEAR(fresh.doorClosedPos.x, openData.pos.x, 1e-4f);
    CHECK_NEAR(fresh.doorClosedPos.y, openData.pos.y + 3.75f, 1e-4f);

    // obstacle.ts interpolates at def.door.openSpeed (lab_door_01 = 7), not a
    // hardcoded constant. Position and rotation both use it.
    Obstacle sliding;
    sliding.active = true;
    sliding.__id = 7;
    ObjectData slideData = data;
    slideData.pos = Vec2(20.0f, 30.0f);
    slideData.ori = 0;
    sliding.m_updateData(slideData, true, true, world);
    slideData.pos = Vec2(20.0f, 33.75f);
    sliding.m_updateData(slideData, true, false, world);
    sliding.update(0.016f, world);
    CHECK_NEAR(sliding.doorInterpPos.y, 30.0f + 7.0f * 0.016f, 1e-4f);

    Obstacle swinging;
    swinging.active = true;
    swinging.__id = 8;
    ObjectData swingData = data;
    swingData.pos = Vec2(20.0f, 30.0f);
    swingData.ori = 0;
    swinging.m_updateData(swingData, true, true, world);
    swingData.ori = 1;
    swinging.m_updateData(swingData, true, false, world);
    swinging.update(0.016f, world);
    CHECK_NEAR(swinging.doorInterpRot, 3.14159265358979f * 7.0f * 0.016f, 1e-4f);
}

// ---------------------------------------------------------------------------
// In-game HUD (game/Hud.h): the plain-pix HUD built on the game root.
TEST(hud_health_color_steps) {
    // ui2.ts healthColourSteps (strict `>` walk, delerp interpolation):
    // 100 grey, 75..99 white, 25..74 the pink ramp, <=24 pure red.
    CHECK_EQ(Hud::healthColor(100.0f, false), 0xb3b3b3u);
    // The web's double lerp floors 254.999.. to 254 at these exact points.
    CHECK_EQ(Hud::healthColor(99.0f, false), 0xfefefeu);
    CHECK_EQ(Hud::healthColor(75.0f, false), 0xffffffu);
    CHECK_EQ(Hud::healthColor(74.0f, false), 0xff9c9cu);
    CHECK_EQ(Hud::healthColor(50.0f, false), 0xff7878u);
    CHECK_EQ(Hud::healthColor(25.0f, false), 0xff5252u);
    CHECK_EQ(Hud::healthColor(24.0f, false), 0xfe0000u);
    CHECK_EQ(Hud::healthColor(0.0f, false), 0xff0000u);
    // Downed players use the flat red step.
    CHECK_EQ(Hud::healthColor(50.0f, true), 0xff0000u);

    // CSS `ui-bar-danger` pulse: black at 0, red at 1, grey-ish halfway.
    CHECK_EQ(Hud::healthDangerColor(0.0f), 0x000000u);
    CHECK_EQ(Hud::healthDangerColor(1.0f), 0xff0000u);
    CHECK_EQ(Hud::healthDangerColor(0.5f), 0x800000u); // rgba(128,0,0)
    CHECK_EQ(Hud::healthDangerColor(-1.0f), 0x000000u);
    CHECK_EQ(Hud::healthDangerColor(2.0f), 0xff0000u);

    // ui.ts updateHealthBar squad palette: 100 grey, 75..99 white, below 75 the
    // darkpink -> lightpink interpolation, downed red, dead grey.
    CHECK_EQ(Hud::squadHealthColor(100.0f, false, false), 0xb3b3b3u);
    CHECK_EQ(Hud::squadHealthColor(90.0f, false, false), 0xffffffu);
    CHECK_EQ(Hud::squadHealthColor(75.0f, false, false), 0xffffffu);
    // Interpolate(45, 112, 45, 50) = floor(45 + 67/45*50) = 119 = 0x77.
    CHECK_EQ(Hud::squadHealthColor(50.0f, false, false), 0xff7777u);
    CHECK_EQ(Hud::squadHealthColor(50.0f, false, true), 0xff0000u);
    CHECK_EQ(Hud::squadHealthColor(0.0f, true, false), 0x444444u);
    // <= 25 is the pulsing danger colour (the caller supplies the phase).
    CHECK_EQ(Hud::squadHealthColor(25.0f, false, false), 0x000000u);
}

TEST(hud_builds_health_bar_at_bottom_centre) {
    pix::NullPixiFactory factory;
    auto* root = static_cast<pix::NullContainer*>(factory.createContainer());
    Hud hud;
    hud.build(&factory, root, Vec2(1280.0f, 720.0f));

    // #ui-health-counter: 400x32 bar in a 412x44 box (6px padding), 12px above
    // the design rect's bottom edge (game.css #ui-bottom-center-0). The HUD
    // lays out in design-space bottom-left coordinates but the container is
    // centred on the design rect (GameScene centres the HUD root), so the box
    // spans x [-206, 206] and y [-348, -304] (axmol is y-up).
    bool foundHealthBg = false;
    for (pix::Node* child : root->children) {
        auto* g = dynamic_cast<pix::NullGraphics*>(child);
        if (!g) continue;
        for (const auto& cmd : g->commands) {
            if (cmd.kind == pix::DrawCommand::Rect && std::fabs(cmd.c - 412.0f) < 0.01f &&
                std::fabs(cmd.d - 44.0f) < 0.01f) {
                CHECK_NEAR(cmd.b, 12.0f - 360.0f, 0.01f);
                CHECK_NEAR(cmd.a, -412.0f * 0.5f, 0.01f);
                foundHealthBg = true;
            }
        }
    }
    CHECK(foundHealthBg);

    // Hidden until the match is playable, like the web client's `stale` flag.
    CHECK(!hud.isVisible());
    Game game;
    hud.update(0.016f, game, false);
    CHECK(!hud.isVisible());
    hud.update(0.016f, game, true);
    CHECK(hud.isVisible());
}
TEST(gas_overlay_world_scale) {
    pix::NullPixiFactory factory;
    Camera camera;
    camera.m_zoom = 2.0f;
    Gas gas;
    gas.m_init(&factory);
    gas.setFullState(0.0f, GasData{});
    gas.setProgress(1.0f);
    gas.mode = 1; // GasMode_Active: the overlay is only visible in an active gas
    gas.m_render(&factory, 0.016f, camera);
    auto* gfx = static_cast<pix::NullGraphics*>(gas.gasRenderer.display);
    CHECK(gfx->visible);
    // gas.ts draws the quad in world units and the camera transform (on the
    // world root) scales it, so the node keeps the world radius 1 and the
    // camera zoom. The hole must not be shrunk by a second zoom factor.
    CHECK_NEAR(gfx->sx, 1.0f, 1e-4f);
    bool hasHole = false;
    for (const auto& cmd : gfx->commands) {
        if (cmd.kind == pix::DrawCommand::BeginHole) hasHole = true;
    }
    CHECK(hasHole);
}

TEST(map_ground_surface_and_island_hole) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    Camera camera;
    camera.m_screenWidth = 800.0f;
    camera.m_screenHeight = 600.0f;
    Map map(&factory, false);

    MapMsg msg;
    msg.mapName = "main";
    msg.seed = 123456;
    msg.width = 512.0f;
    msg.height = 512.0f;
    msg.shoreInset = 4.0f;
    msg.grassInset = 2.0f;
    map.loadMap(msg, camera);

    // Ground surface classes match map.ts (water outside the shore, sand in the
    // beach ring, grass inside the grass contour).
    CHECK(map.getGroundSurface(Vec2(-50.0f, -50.0f), 0).type == Map::SurfaceType::Water);
    // A point inside the grass contour: the centroid-ish average of its verts
    // (the first vertex itself may lie exactly on the contour edge).
    Vec2 inside = Vec2(0.0f, 0.0f);
    for (const auto& v : map.terrain.grass) {
        inside = v2Add(inside, v);
    }
    inside = v2Div(inside, static_cast<float>(map.terrain.grass.size()));
    CHECK(map.getGroundSurface(inside, 0).type == Map::SurfaceType::Grass);
    CHECK(!map.isInOcean(inside));
    CHECK(map.isInOcean(Vec2(-50.0f, -50.0f)));
    CHECK(map.distanceToShore(Vec2(-50.0f, -50.0f)) > 0.0f);

    // Terrain is drawn as water rect -> shore ring -> grass overpaint (map.ts'
    // canvas ordering) for every adapter: a punched hole would need the
    // slab-scanline tessellator to split the full island contour, which
    // produced tens of thousands of overlapping triangles and broke the
    // terrain on the GPU.
    map.renderTerrain(map.groundGfx, 0.5f, false, false);
    auto* g = static_cast<pix::NullGraphics*>(map.groundGfx);
    bool hasHole = false;
    int fills = 0;
    for (const auto& cmd : g->commands) {
        if (cmd.kind == pix::DrawCommand::BeginHole) hasHole = true;
        if (cmd.kind == pix::DrawCommand::BeginFill) fills++;
    }
    CHECK(!hasHole);
    CHECK(fills >= 3); // surround + water + beach + grass
    CHECK(!g->fillTriangles.empty());
    // The grass contour (last fill) is tessellated, not dropped: its centroid
    // must still be covered after the tessellation regression above.
    CHECK(!map.terrain.grass.empty());

    // Canvas keeps the overpaint ordering.
    Map canvasMap(&factory, true);
    canvasMap.loadMap(msg, camera);
    canvasMap.renderTerrain(canvasMap.groundGfx, 0.5f, true, false);
    auto* canvasG = static_cast<pix::NullGraphics*>(canvasMap.groundGfx);
    bool canvasHole = false;
    for (const auto& cmd : canvasG->commands) {
        if (cmd.kind == pix::DrawCommand::BeginHole) canvasHole = true;
    }
    CHECK(!canvasHole);
}

// The whole terrain is drawn into the single ground node (map.ts
// display.ground) and the world container carries the camera. The island
// (grass) must actually be tessellated: the terrain centroid has to be covered
// by at least one emitted triangle, otherwise the map renders as flat water.
TEST(map_ground_covers_island) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    Camera camera;
    camera.m_screenWidth = 800.0f;
    camera.m_screenHeight = 600.0f;
    Map map(&factory, false);
    MapMsg msg;
    msg.mapName = "main";
    msg.seed = 123456;
    msg.width = 512.0f;
    msg.height = 512.0f;
    msg.shoreInset = 4.0f;
    msg.grassInset = 2.0f;
    map.loadMap(msg, camera);

    auto* g = static_cast<pix::NullGraphics*>(map.groundGfx);
    CHECK(g != nullptr);
    CHECK(!g->fillTriangles.empty());

    Vec2 inside = Vec2(0.0f, 0.0f);
    for (const auto& v : map.terrain.grass) {
        inside = v2Add(inside, v);
    }
    inside = v2Div(inside, static_cast<float>(map.terrain.grass.size()));

    const auto bary = [](const pix::FillTriangle& t, const Vec2& p) {
        const float d = (t[1].y - t[2].y) * (t[0].x - t[2].x) + (t[2].x - t[1].x) * (t[0].y - t[2].y);
        if (std::fabs(d) < 1e-6f) return false;
        const float a = ((t[1].y - t[2].y) * (p.x - t[2].x) + (t[2].x - t[1].x) * (p.y - t[2].y)) / d;
        const float b = ((t[2].y - t[0].y) * (p.x - t[2].x) + (t[0].x - t[2].x) * (p.y - t[2].y)) / d;
        const float c = 1.0f - a - b;
        return a >= -1e-4f && b >= -1e-4f && c >= -1e-4f;
    };
    bool covered = false;
    for (const auto& t : g->fillTriangles) {
        if (bary(t, inside)) {
            covered = true;
            break;
        }
    }
    CHECK(covered);
}

TEST(player_aura_submerge_frozen) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    world.camera().m_screenWidth = 800.0f;
    world.camera().m_screenHeight = 600.0f;

    Player ap;
    ap.m_init();
    auto apd = playerData();
    world.setActivePlayer(&ap);
    ap.m_updateData(apd, true, true, world);
    ap.update(0, world);

    Player p;
    p.m_init();
    auto d = playerData();
    d.pos = Vec2(200.0f, 200.0f);
    d.actionType = Action_UseItem;
    d.actionItem = "bandage";
    d.perks.push_back({"aoe_heal", false});
    p.m_updateData(d, true, true, world);
    p.update(0.1f, world);
    CHECK(p.auraVisible);
    CHECK(p.auraContainer != nullptr);
    CHECK(sprite(p.auraCircle)->visible);

    // Losing aoe_heal stops the aura (player.ts role visuals).
    d.perks.clear();
    p.m_updateData(d, true, false, world);
    p.update(0.1f, world);
    CHECK(!sprite(p.auraCircle)->visible);

    // Submersion: in the ocean the submerge sprite fades in.
    world.setActivePlayer(&ap);
    Player water;
    water.m_init();
    auto wd = playerData();
    wd.pos = Vec2(-100.0f, -100.0f);
    water.m_updateData(wd, true, true, world);
    for (int i = 0; i < 200; i++) {
        water.update(0.05f, world);
    }
    CHECK(water.submersion > 0.5f);
    CHECK(sprite(water.bodySubmergeSprite)->visible);
    CHECK_NEAR(sprite(water.bodySubmergeSprite)->tint, world.map().mapDef.colors.water, 1.0f);

    // Frozen: the body-effect sprite fades in, then out (player.ts keeps the
    // patch visible for fadeDuration=0.25s after the freeze ends).
    const GameObjRenderDef* snowball = getDefProvider()->gameObject("explosion_snowball");
    CHECK(snowball != nullptr);
    CHECK(snowball != nullptr && !snowball->frozenSprites.empty());
    Player frozen;
    frozen.m_init();
    auto fd = playerData();
    fd.frozen = true;
    fd.frozenType = "explosion_snowball";
    frozen.m_updateData(fd, true, true, world);
    frozen.update(0.001f, world);
    CHECK(frozen.frozenActive);
    CHECK(sprite(frozen.bodyEffectSprite)->visible);
    CHECK_EQ(sprite(frozen.bodyEffectSprite)->frame, std::string());
    fd.frozen = false;    frozen.m_updateData(fd, true, false, world);
    frozen.update(0.1f, world);
    CHECK(sprite(frozen.bodyEffectSprite)->visible);
    for (int i = 0; i < 20; i++) {
        frozen.update(0.05f, world);
    }
    CHECK(!sprite(frozen.bodyEffectSprite)->visible);

    // Order matters: clear the active player before the stack objects die so
    // the world never points at a destroyed player.
    world.setActivePlayer(nullptr);
}

TEST(obstacle_door_sprite_anchor) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    Obstacle o;
    o.active = true;
    o.__id = 9;
    ObjectData data;
    data.type = "lab_door_01";
    data.pos = Vec2(10.0f, 10.0f);
    data.isDoor = true;
    o.m_updateData(data, true, true, world);
    o.update(0.016f, world);
    o.render(world, 0);
    const MapObjectDef* def = getDefProvider()->mapObject("lab_door_01");
    CHECK(def != nullptr);
    if (!def) {
        return;
    }
    CHECK(sprite(o.sprite) != nullptr);
    if (!sprite(o.sprite)) {
        return;
    }
    // obstacle.ts: the sprite anchor is 0.5, or def.door.spriteAnchor for doors.
    // PIXI uses a top-left anchor; the adapter uses axmol's bottom-left anchor,
    // so Y is converted (`y' = 1 - y`).
    CHECK_NEAR(o.debugAnchor().x, def->doorSpriteAnchor.x, 1e-4f);
    CHECK_NEAR(o.debugAnchor().y, 1.0f - def->doorSpriteAnchor.y, 1e-4f);
    CHECK_NEAR(sprite(o.sprite)->ax, def->doorSpriteAnchor.x, 1e-4f);
    CHECK_NEAR(sprite(o.sprite)->ay, 1.0f - def->doorSpriteAnchor.y, 1e-4f);
    CHECK_NEAR(sprite(o.sprite)->ax, 0.5f, 1e-4f);
    (void)def;

    // A custom anchor is honoured (def.door.spriteAnchor is 0.5 for shipped
    // doors, so use a fake def to exercise the non-default path).
    FakeDefProvider provider;
    provider.def.type = "custom_door";
    provider.def.isDoor = true;
    provider.def.img.sprite = "lab_door_01.img";
    provider.def.doorSpriteAnchor = Vec2(0.25f, 0.75f);
    provider.def.img.zIdx = 15;
    provider.def.map.display = true;
    provider.def.hasCollision = true;
    provider.def.collision = Collider::createAabb(Vec2(-1.0f, -1.0f), Vec2(1.0f, 1.0f));
    provider.def.hasBounding = true;
    provider.def.boundingCollider = Collider::createAabb(Vec2(-1.0f, -1.0f), Vec2(1.0f, 1.0f));
    setDefProvider(&provider);
    Obstacle custom;
    custom.active = true;
    custom.__id = 10;
    ObjectData cdata;
    cdata.type = "custom_door";
    cdata.pos = Vec2(10.0f, 10.0f);
    cdata.isDoor = true;
    custom.m_init();
    custom.m_updateData(cdata, true, true, world);
    custom.update(0.016f, world);
    custom.render(world, 0);
    // The custom PIXI anchor (0.25, 0.75) becomes (0.25, 0.25) for axmol.
    CHECK_EQ(custom.isDoor, true);
    CHECK_EQ(custom.type, std::string("custom_door"));
    CHECK_NEAR(custom.debugAnchor().x, 0.25f, 1e-4f);
    CHECK_NEAR(custom.debugAnchor().y, 0.25f, 1e-4f);
    CHECK_NEAR(sprite(custom.sprite) != nullptr ? sprite(custom.sprite)->ax : -1.0f, 0.25f,
               1e-4f);
    CHECK_NEAR(sprite(custom.sprite) != nullptr ? sprite(custom.sprite)->ay : -1.0f, 0.25f,
               1e-4f);
    (void)def;
    installGeneratedDefs();
}

// animData.ts animation effects + decal.ts / map.ts getGroundSurface.
TEST(player_anim_effects_and_decals) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    world.camera().m_screenWidth = 800.0f;
    world.camera().m_screenHeight = 600.0f;

    const DefProvider* defs = getDefProvider();
    CHECK(defs != nullptr);
    const PlayerAnimationDef* fists = defs ? defs->playerAnimation("fists") : nullptr;
    CHECK(fists != nullptr);
    if (fists) {
        CHECK_EQ(fists->effects.size(), 2u);
        CHECK(fists->effects[0].fn == AnimEffectFn::PlaySound);
        CHECK_EQ(fists->effects[0].sound, std::string("swing"));
        CHECK(fists->effects[1].fn == AnimEffectFn::MeleeCollision);
    }
    const GameObjRenderDef* axe = defs ? defs->gameObject("woodaxe") : nullptr;
    CHECK(axe != nullptr);
    if (axe) {
        CHECK(axe->hasAttack);
        CHECK(!axe->cleave);
        CHECK_NEAR(axe->attackOffset.x, 1.35f, 1e-4f);
        CHECK_NEAR(axe->attackRad, 1.0f, 1e-4f);
    }
    const MapObjectDef* barrel = defs ? defs->mapObject("barrel_01") : nullptr;
    CHECK(barrel != nullptr);
    if (barrel) {
        // barrel_01: collidable circle r=1.75, height 0.5.
        CHECK_NEAR(barrel->height, 0.5f, 1e-4f);
        CHECK(barrel->collidable);
        CHECK_EQ(barrel->hitParticle, std::string("barrelChip"));
        CHECK_EQ(barrel->punchSound, std::string("barrel_bullet"));
    }

    // Melee collision: fists hit an obstacle in front of the player.
    Player p;
    p.m_init();
    auto d = playerData();
    d.pos = Vec2(50.0f, 50.0f);
    d.dir = Vec2(1.0f, 0.0f);
    d.activeWeapon = "fists";
    d.animType = Anim_Melee;
    d.animSeq = 7;
    p.m_updateData(d, true, true, world);

    // Decals: the bathhouse pool def carries a water surface.
    const MapObjectDef* poolDecal = defs ? defs->mapObject("decal_bathhouse_pool_01") : nullptr;
    CHECK(poolDecal != nullptr);
    if (poolDecal) {
        CHECK(poolDecal->hasSurface);
        CHECK(poolDecal->hasGore);
        CHECK_EQ(poolDecal->surfaceWaterColor, 0x4eb2c4u);
    }

    // The map consults the decal barn before buildings/rivers/terrain.
    MapMsg msg;
    msg.mapName = "main";
    msg.seed = 123456;
    msg.width = 512.0f;
    msg.height = 512.0f;
    msg.shoreInset = 4.0f;
    msg.grassInset = 2.0f;
    world.loadMap(msg);
    Decal dcl;
    dcl.m_init();
    dcl.active = true;
    dcl.__id = 3;
    ObjectData dd;
    dd.type = "decal_bathhouse_pool_01";
    dd.pos = Vec2(-50.0f, 0.0f);
    dd.scale = 1.0f;
    dd.layer = 0;
    dcl.m_updateData(dd, true, true, world);
    // Mirror DecalBarn::update for the gore colour path (goreKills=0 -> goreT=0).
    dcl.update(0.016f, world);
    world.decalBarn().update(0.0f, world);
    const Map::GroundSurface s0 = world.map().getGroundSurface(Vec2(-50.0f, 0.0f), 0);
    CHECK(s0.type == Map::SurfaceType::Water);
    // goreKills = 0 -> goreT = 0; the def's own surface water colour is used
    // (decal_bathhouse_pool_01 has no explicit gore waterColour override).
    CHECK_EQ(dcl.surfaceWaterColor, 0x4eb2c4u);
    // The decal is visible to getGroundSurface while active...
    CHECK_EQ(dcl.collider.type, Collider::Aabb);
    CHECK(dcl.active && dcl.hasSurface);
    // ...and its pooled render node exists for sprite output.
    CHECK(dcl.decalRender != nullptr);
    CHECK(dcl.decalRender == nullptr || dcl.decalRender->active);
    // Outside the pool the unmodified terrain surface is unchanged.
    const Map::GroundSurface s1 = world.map().getGroundSurface(Vec2(0.0f, -50.0f), 0);
    CHECK(s1.waterColor != 0x4eb2c4u);

    installGeneratedDefs();
}

// P2 rendering parity: player.ts eases m_bodyRad (scale pickups/adrenaline) and
// the body/gun/melee visual scale follows it instead of snapping to the net
// scale. gameConfig.player.maxVisualRadius is 3.75 (used by the stairs sort).
TEST(player_body_radius_eases) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);

    Player p;
    p.m_init();
    auto d = playerData();
    d.scale = 1.0f;
    p.m_updateData(d, true, true, world);
    p.update(0.0f, world);
    CHECK_NEAR(p.rad, 1.0f, 1e-5f);
    CHECK_NEAR(p.bodyRad, 1.0f, 1e-5f);
    CHECK_NEAR(container(p.bodyContainer)->sx, 1.0f, 1e-5f);
    CHECK_NEAR(PlayerConfig::maxVisualRadius, 3.75f, 1e-5f);

    // A scale change (e.g. adrenaline) snaps into m_rad but eases m_bodyRad.
    d.scale = 1.5f;
    p.m_updateData(d, true, false, world);
    CHECK_NEAR(p.rad, 1.5f, 1e-5f);
    CHECK_NEAR(p.bodyRad, 1.0f, 1e-5f);
    p.update(0.05f, world);
    CHECK_NEAR(p.bodyRad, 1.0f + 0.5f * 0.05f * 6.0f, 1e-5f);
    CHECK_NEAR(container(p.bodyContainer)->sx, p.bodyRad, 1e-5f);
    for (int i = 0; i < 100; i++) p.update(0.016f, world);
    CHECK_NEAR(p.bodyRad, 1.5f, 1e-4f);
    CHECK_NEAR(container(p.bodyContainer)->sx, 1.5f, 1e-4f);

    // A respawn/new object snaps rather than growing in from the old size.
    Player fresh;
    fresh.m_init();
    d.scale = 2.0f;
    fresh.m_updateData(d, true, true, world);
    CHECK_NEAR(fresh.bodyRad, 2.0f, 1e-5f);
    fresh.update(0.0f, world);
    CHECK_NEAR(container(fresh.bodyContainer)->sx, 2.0f, 1e-5f);
}

// P2 rendering parity: obstacle.ts swaps button useImg/offImg sprites with the
// button state, spawns the button useParticle, and emits the def's destroy
// particles exactly once (the old port spawned them every render frame).
TEST(obstacle_button_swap_and_destroy_fx) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    const auto* defs = getDefProvider();

    auto activeParticles = [&]() {
        int n = 0;
        for (auto* particle : world.particleBarn().particles) {
            if (particle->active) n++;
        }
        return n;
    };

    // switch_01 carries both useImg and offImg.
    const MapObjectDef* sw = defs->mapObject("switch_01");
    CHECK(sw != nullptr);
    CHECK(sw && !sw->buttonUseImg.empty());
    CHECK(sw && !sw->buttonOffImg.empty());

    Obstacle b;
    b.active = true;
    b.__id = 77;
    ObjectData bd;
    bd.type = "switch_01";
    bd.pos = Vec2(10.0f, 10.0f);
    bd.layer = 0;
    bd.scale = 1.0f;
    bd.isButton = true;
    bd.buttonCanUse = true;
    bd.buttonOnOff = false;
    bd.buttonSeq = 1;
    b.m_updateData(bd, true, true, world);
    CHECK_EQ(b.frame, sw->img.sprite);

    bd.buttonOnOff = true;
    bd.buttonSeq = 2;
    b.m_updateData(bd, true, false, world);
    CHECK_EQ(b.frame, sw->buttonUseImg);

    bd.buttonOnOff = false;
    bd.buttonCanUse = false;
    bd.buttonSeq = 3;
    b.m_updateData(bd, true, false, world);
    CHECK_EQ(b.frame, sw->buttonOffImg);

    // A partial update (pos/ori/scale only) must not clear the button state.
    ObjectData partial;
    partial.pos = bd.pos;
    partial.scale = bd.scale;
    partial.ori = bd.ori;
    b.m_updateData(partial, false, false, world);
    CHECK(b.isButton);
    CHECK(!b.buttonCanUse);
    CHECK_EQ(b.buttonSeq, 3);
    CHECK_EQ(b.frame, sw->buttonOffImg);

    // Airdrop crates have button.useParticle; a seq change spawns one.
    const MapObjectDef* crate = defs->mapObject("airdrop_crate_01");
    CHECK(crate != nullptr);
    CHECK(crate && !crate->buttonUseParticle.empty());
    Obstacle c;
    c.active = true;
    c.__id = 78;
    ObjectData cd;
    cd.type = "airdrop_crate_01";
    cd.pos = Vec2(20.0f, 20.0f);
    cd.layer = 0;
    cd.scale = 1.0f;
    cd.isButton = true;
    cd.buttonCanUse = true;
    cd.buttonSeq = 1;
    c.m_updateData(cd, true, true, world);
    c.update(0.016f, world); // first frame: no FX
    const int beforeButton = activeParticles();
    cd.buttonSeq = 2;
    c.m_updateData(cd, true, false, world);
    c.update(0.016f, world);
    CHECK_EQ(activeParticles(), beforeButton + 1);

    // Destroy FX: barrel_01 has explodeParticle "barrelBreak" -> 5..10 pieces
    // on the death update, and the same pieces must not spawn again.
    Obstacle o;
    o.active = true;
    o.__id = 91;
    ObjectData d;
    d.type = "barrel_01";
    d.pos = Vec2(30.0f, 30.0f);
    d.layer = 0;
    d.scale = 1.0f;
    o.m_updateData(d, true, true, world);
    o.update(0.016f, world); // clear isNew
    const int beforeDeath = activeParticles();
    d.dead = true;
    o.m_updateData(d, true, false, world);
    o.update(0.016f, world);
    const int afterDeath = activeParticles();
    CHECK(afterDeath - beforeDeath >= 5);
    CHECK(afterDeath - beforeDeath <= 10);
    o.update(0.016f, world);
    CHECK_EQ(activeParticles(), afterDeath);

    // A partial update keeps the destroyed state (dead is a full-update field).
    ObjectData deadPart;
    deadPart.pos = d.pos;
    deadPart.scale = d.scale;
    o.m_updateData(deadPart, false, false, world);
    CHECK(o.dead);
    CHECK_EQ(o.frame, getDefProvider()->mapObject("barrel_01")->img.residue);
}

// P2 rendering parity: building.ts destroyCeilingFx spawns the def's ceiling
// destroy particle at random points inside the first surface collider.
TEST(building_ceiling_destroy_particles) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    const auto* defs = getDefProvider();

    const MapObjectDef* shack = defs->mapObject("shack_01");
    CHECK(shack != nullptr);
    CHECK(shack && shack->ceilingDestroyParticle == "shackBreak");
    CHECK(shack && shack->ceilingDestroyParticleCount == 25);
    // The ceiling destroy sound is a distinct def field (not the whole
    // building `sound` object); shack_01 uses the native default.
    CHECK(shack && shack->ceilingDestroySound.empty());

    auto activeParticles = [&]() {
        int n = 0;
        for (auto* particle : world.particleBarn().particles) {
            if (particle->active) n++;
        }
        return n;
    };

    Building b;
    b.m_init();
    b.type = "shack_01";
    b.layer = 0;
    b.ceilingDead = true;
    b.isNew = false;
    Building::Surface surface;
    surface.type = "wood";
    surface.colliders.push_back(Collider::createAabb(Vec2(0.0f, 0.0f), Vec2(4.0f, 4.0f)));
    b.surfaces.push_back(surface);

    const int before = activeParticles();
    b.update(0.016f, world);
    CHECK_EQ(activeParticles() - before, 25);
    b.update(0.016f, world);
    CHECK_EQ(activeParticles() - before, 25);
}

// util.ts hsvToRgb/rgbToInt port used by particles.ts color closures.
TEST(hsv_to_rgb_matches_web) {
    CHECK_EQ(math::hsvToRgbInt(0.0f, 0.0f, 1.0f), 0xffffffu);
    CHECK_EQ(math::hsvToRgbInt(0.0f, 1.0f, 1.0f), 0xff0000u);
    CHECK_EQ(math::hsvToRgbInt(1.0f / 3.0f, 1.0f, 1.0f), 0x00ff00u);
    CHECK_EQ(math::hsvToRgbInt(2.0f / 3.0f, 1.0f, 1.0f), 0x0000ffu);
    CHECK_EQ(math::hsvToRgbInt(0.0f, 0.0f, 0.0f), 0x000000u);
    CHECK_EQ(math::hsvToRgbInt(0.0f, 0.0f, 0.5f), 0x808080u);
}

// P2 rendering parity: particles.ts color closures are generated as HSV ranges
// and each Particle picks a fresh value (blood/leaf/chip variance).
TEST(particle_color_variance) {
    installGeneratedDefs();
    const auto* defs = getDefProvider();
    const ParticleDef* leaf = defs->particle("leaf");
    CHECK(leaf != nullptr);
    CHECK(leaf && leaf->colorVariance);
    CHECK_NEAR(leaf->colorHueMin, 0.29f, 1e-5f);
    CHECK_NEAR(leaf->colorHueMax, 0.29f, 1e-5f);
    CHECK_NEAR(leaf->colorSatMin, 1.0f, 1e-5f);
    CHECK_NEAR(leaf->colorValMin, 0.5f, 1e-5f);
    CHECK_NEAR(leaf->colorValMax, 0.75f, 1e-5f);

    // Static colors stay exact (glassChip = 0x80d9ff) and do not vary.
    const ParticleDef* glass = defs->particle("glassChip");
    CHECK(glass != nullptr);
    CHECK(glass && glass->hasColor);
    CHECK(glass && !glass->colorVariance);
    CHECK(glass && glass->color == 0x80d9ffu);

    pix::NullPixiFactory factory;
    std::vector<uint32_t> tints;
    for (int i = 0; i < 96; i++) {
        Particle p;
        p.m_init(&factory, "leaf", 0, Vec2(0.0f, 0.0f), Vec2(), 1.0f, 0.0f, nullptr, -1, 1.0f);
        CHECK(p.sprite != nullptr);
        tints.push_back(sprite(p.sprite)->tint);
    }
    const uint32_t lo = math::hsvToRgbInt(0.29f, 1.0f, 0.5f);
    const uint32_t hi = math::hsvToRgbInt(0.29f, 1.0f, 0.75f);
    int distinct = 0;
    for (size_t i = 0; i < tints.size(); i++) {
        const uint32_t c = tints[i];
        CHECK(((c >> 16) & 0xff) >= ((lo >> 16) & 0xff) && ((c >> 16) & 0xff) <= ((hi >> 16) & 0xff));
        CHECK(((c >> 8) & 0xff) >= ((lo >> 8) & 0xff) && ((c >> 8) & 0xff) <= ((hi >> 8) & 0xff));
        CHECK((c & 0xff) <= (hi & 0xff));
        bool first = true;
        for (size_t j = 0; j < i; j++) {
            if (tints[j] == c) {
                first = false;
                break;
            }
        }
        if (first) distinct++;
    }
    CHECK(distinct > 8);
}

// P2 rendering parity: the river-shore footstep surface comes from
// mapDef.biome.sound.riverShore (stone on the spring maps, sand elsewhere).
TEST(river_shore_surface_from_map_def) {
    installGeneratedDefs();
    const auto* defs = getDefProvider();
    const auto* main = defs->mapRender("main");
    const auto* spring = defs->mapRender("main_spring");
    const auto* woodsSpring = defs->mapRender("woods_spring");
    CHECK(main != nullptr);
    CHECK(spring != nullptr);
    CHECK(woodsSpring != nullptr);
    CHECK(main && main->riverShoreSurface == SurfaceType::Sand);
    CHECK(spring && spring->riverShoreSurface == SurfaceType::Stone);
    CHECK(woodsSpring && woodsSpring->riverShoreSurface == SurfaceType::Stone);
}

// shot.ts createCasingParticle: shot casings use the gun's shell offset/scale,
// and caseTiming == "reload" guns (revolvers/pistols) eject maxReload shells on
// the reload action instead of on every shot.
TEST(gun_casing_fields_and_reload_shells) {
    installGeneratedDefs();
    const auto* defs = getDefProvider();
    const auto* m9 = defs->gameObject("m9");
    const auto* ot38 = defs->gameObject("ot38");
    const auto* bugle = defs->gameObject("bugle");
    CHECK(m9 != nullptr);
    CHECK(ot38 != nullptr);
    CHECK(bugle != nullptr);
    CHECK(m9 && !m9->casingOnReload);
    CHECK(m9 && m9->maxReload == 15);
    CHECK_NEAR(m9 ? m9->shellOffset : 0.0f, 0.25f, 1e-5f);
    CHECK_NEAR(m9 ? m9->shellScale : 0.0f, 1.0f, 1e-5f);
    CHECK(ot38 && ot38->casingOnReload);
    CHECK(ot38 && ot38->maxReload == 5);
    CHECK(bugle && bugle->shellForward == 1.0f);
    CHECK(bugle && bugle->shellScale == 4.0f);

    auto activeParticles = [&](GameWorld& world) {
        int n = 0;
        for (auto* particle : world.particleBarn().particles) {
            if (particle->active) n++;
        }
        return n;
    };

    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    // A single shot casing at the expected muzzle offset.
    createCasingParticle(&factory, *m9, -3.14159265358979f * 0.5f, 1.0f, Vec2(10.0f, 20.0f), Vec2(1.0f, 0.0f),
                         0, 19, world.particleBarn());
    CHECK_EQ(activeParticles(world), 1);
    const Particle* casing = nullptr;
    for (auto* particle : world.particleBarn().particles) {
        if (particle->active) casing = particle;
    }
    CHECK(casing != nullptr);
    if (casing) {
        CHECK_NEAR(casing->pos.x, 10.0f + PlayerConfig::radius + m9->shellOffset, 1e-5f);
        CHECK_NEAR(casing->pos.y, 20.0f, 1e-5f);
        CHECK_NEAR(v2Length(casing->vel), 9.5f, 1e-4f);
        CHECK_EQ(casing->layer, 0);
        CHECK_NEAR(casing->rot, 3.14159265358979f * 0.5f, 1e-5f);
        CHECK_EQ(casing->zOrd, 19);
    }

    // Revolver reload ejects maxReload shells (5 for ot38), and a
    // caseTiming == "shoot" gun spawns none on reload.
    Player p;
    p.m_init();
    auto d = playerData();
    d.activeWeapon = "ot38";
    p.m_updateData(d, true, true, world);
    const int beforeReload = activeParticles(world);
    p.updateActions(Action_Reload, "ot38", world);
    const int afterReload = activeParticles(world);
    CHECK_EQ(afterReload - beforeReload, 5);
    int reloadShells = 0;
    for (auto* particle : world.particleBarn().particles) {
        if (particle->active && particle->zOrd == p.renderZOrd + 1) reloadShells++;
    }
    CHECK(reloadShells >= 6); // includes the earlier shot casing

    p.updateActions(Action_None, "", world);
    p.updateActions(Action_Reload, "m9", world);
    CHECK_EQ(activeParticles(world), afterReload);
}

// HUD/menu polish: gear level badges + tap-to-drop, squad group-color dots and
// the downed pulse.
TEST(hud_gear_levels_and_squad_polish) {
    pix::NullPixiFactory factory;
    auto* root = static_cast<pix::NullContainer*>(factory.createContainer());
    Hud hud;
    hud.build(&factory, root, Vec2(1280.0f, 720.0f));
    hud.setVisible(true);

    std::vector<Hud::GearSlot> slots;
    slots.push_back(Hud::GearSlot{"loot-helmet-02.img", "helmet02", 3, true});
    slots.push_back(Hud::GearSlot{"loot-chest-02.img", "chest02", 4, true});
    slots.push_back(Hud::GearSlot{"loot-backpack-02.img", "backpack02", 2, false});
    slots.push_back(Hud::GearSlot{"loot-perk-takedown.img", "takedown", 0, true});
    hud.setGearSlots(slots);

    CHECK(static_cast<const pix::NullText*>(hud.gearLevelText(0)) != nullptr);
    CHECK_EQ(static_cast<const pix::NullText*>(hud.gearLevelText(0))->text, std::string("Lvl. 3"));
    CHECK(static_cast<const pix::NullText*>(hud.gearLevelText(0))->visible);
    CHECK_EQ(static_cast<const pix::NullText*>(hud.gearLevelText(1))->text, std::string("Lvl. 4"));
    CHECK_EQ(static_cast<const pix::NullText*>(hud.gearLevelText(2))->text, std::string("Lvl. 2"));
    // Perks have no level badge.
    CHECK(!static_cast<const pix::NullText*>(hud.gearLevelText(3))->visible);
    CHECK_EQ(hud.gearType(0), std::string("helmet02"));
    CHECK(hud.gearDroppable(0));
    CHECK(hud.gearDroppable(1));
    // Web excludes the backpack from drop actions.
    CHECK(!hud.gearDroppable(2));
    CHECK(hud.gearDroppable(3));

    // Tap the droppable slots: hitTest maps back through the centred root.
    // The gear row sits left of the health bar (hudX - 34 - i*32, bottom).
    const float gearSlotX = 400.0f; // hudX - 34 design px, i.e. slot 0 centre
    Hud::TapResult helmet = hud.hitTest(gearSlotX, 23.0f);
    CHECK(helmet.kind == Hud::TapKind::GearSlot);
    CHECK_EQ(helmet.slot, 0);
    Hud::TapResult backpack = hud.hitTest(gearSlotX - 2.0f * 32.0f, 23.0f);
    CHECK(backpack.kind == Hud::TapKind::None);
    Hud::TapResult perk = hud.hitTest(gearSlotX - 3.0f * 32.0f, 23.0f);
    CHECK(perk.kind == Hud::TapKind::GearSlot);
    CHECK_EQ(perk.slot, 3);

    std::vector<SquadMember> squad(2);
    squad[0].name = "Alice";
    squad[0].health = 60.0f;
    squad[0].downed = true;
    squad[1].name = "Bob";
    squad[1].health = 0.0f;
    squad[1].dead = true;
    hud.setSquadState(squad);
    CHECK(hud.squadColorDot(0) != nullptr);
    CHECK(static_cast<const pix::NullGraphics*>(hud.squadColorDot(0))->visible);
    // Rows sit right of the minimap and carry a translucent backing sized to
    // the bar. The group dot is above the bar's left end (not its right end).
    const auto* rowBg = static_cast<const pix::NullGraphics*>(hud.squadRowBg(0));
    CHECK(rowBg != nullptr);
    CHECK(rowBg->visible);
    CHECK_EQ(rowBg->commands.size(), static_cast<size_t>(3));
    CHECK_NEAR(rowBg->commands[1].c, Hud::kSquadRowWidth, 1e-4f);
    CHECK_NEAR(rowBg->commands[1].d, Hud::kSquadRowHeight, 1e-4f);
    CHECK(rowBg->commands[0].alpha > 0.0f);
    const float squadX = Hud::kMinimapLeft + Hud::kMinimapSize + 8.0f;
    CHECK_NEAR(rowBg->commands[1].a, squadX - 640.0f, 1e-4f);
    const auto* dot = static_cast<const pix::NullGraphics*>(hud.squadColorDot(0));
    const float dotRightEdge = dot->commands[1].a + dot->commands[1].c;
    CHECK(dotRightEdge < (squadX + Hud::kSquadRowPadding + Hud::kSquadBarWidth) - 640.0f);
    // The 16px name's bottom edge stays above the bar's top edge.
    const auto* name0 = static_cast<const pix::NullText*>(hud.squadNameText(0));
    const auto* barBg0 = static_cast<const pix::NullGraphics*>(hud.squadBarBg(0));
    CHECK(name0 != nullptr && barBg0 != nullptr);
    CHECK(name0->y - 20.0f > barBg0->commands[1].b + barBg0->commands[1].d);
    CHECK_EQ(static_cast<const pix::NullSprite*>(hud.squadStatusIcon(0))->frame,
             std::string("gui/down.png"));
    CHECK_EQ(static_cast<const pix::NullSprite*>(hud.squadStatusIcon(1))->frame,
             std::string("gui/skull-team.png"));
    // Dead/disconnected names fade to 30% (ui.ts updateTeam).
    CHECK_NEAR(static_cast<const pix::NullText*>(hud.squadNameText(1))->alpha, 0.3f, 1e-5f);

    // Downed status pulses (icon-pulse: 2 s black -> red); the dot alpha moves
    // off 1.0 as the pulse advances.
    Game game;
    hud.update(0.5f, game, true);
    CHECK(static_cast<const pix::NullGraphics*>(hud.squadColorDot(0))->alpha < 1.0f);
}

// ui.ts team arrows: Hud places each indicator at the caller-supplied
// screen-edge position, not at a fixed radius around the HUD centre (which
// put the arrow next to the local player's head). The indicator is the
// up-pointing `/img/gui/indicator.svg` art plus the group-colour dot.
TEST(hud_team_arrow_positions) {
    pix::NullPixiFactory factory;
    auto* root = static_cast<pix::NullContainer*>(factory.createContainer());
    Hud hud;
    hud.build(&factory, root, Vec2(1280.0f, 720.0f));
    hud.setVisible(true);

    std::vector<TeammateArrow> arrows(2);
    // Hud child coordinates are centred on the 1280x720 design rect.
    arrows[0].slot = 0;
    arrows[0].pos = Vec2(0.0f, 344.0f); // top edge, 16px inset
    arrows[0].rot = Hud::teamArrowRotation(Vec2(0.0f, 1.0f)); // up
    arrows[1].slot = 1;
    arrows[1].pos = Vec2(624.0f, 0.0f); // right edge
    arrows[1].rot = Hud::teamArrowRotation(Vec2(1.0f, 0.0f)); // right
    hud.setTeammateArrows(arrows);

    const auto* first = static_cast<const pix::NullContainer*>(hud.teamArrow(0));
    CHECK(first != nullptr);
    CHECK(first->visible);
    CHECK_NEAR(first->x, 0.0f, 1e-4f);
    CHECK_NEAR(first->y, 344.0f, 1e-4f);
    CHECK_NEAR(first->rotation, 0.0f, 1e-4f);

    // .ui-team-indicator-pos: 32x32 group-colour dot, 7px below the centre of
    // the 64px box and scaled by 0.5 with the rest of the indicator.
    const auto* dot0 = static_cast<const pix::NullGraphics*>(hud.teamArrowDot(0));
    CHECK(dot0 != nullptr);
    CHECK_EQ(dot0->commands.size(), static_cast<size_t>(3));
    CHECK_EQ(dot0->commands[0].color, kGroupColors[0]);
    CHECK_NEAR(dot0->commands[1].a, 0.0f, 1e-4f);
    CHECK_NEAR(dot0->commands[1].b, Hud::kTeamArrowDotOffsetY, 1e-4f);
    CHECK_NEAR(dot0->commands[1].c, Hud::kTeamArrowDotRadius, 1e-4f);
    CHECK_NEAR(dot0->alpha, Hud::kTeamArrowDotOpacity, 1e-5f);

    const auto* second = static_cast<const pix::NullContainer*>(hud.teamArrow(1));
    CHECK(second->visible);
    CHECK_NEAR(second->x, 624.0f, 1e-4f);
    CHECK_NEAR(second->y, 0.0f, 1e-4f);
    CHECK_NEAR(second->rotation, 1.5707964f, 1e-4f);
    const auto* dot1 = static_cast<const pix::NullGraphics*>(hud.teamArrowDot(1));
    CHECK_EQ(dot1->commands[0].color, kGroupColors[1]);

    // Unused group slots stay hidden (the web has five main indicators).
    CHECK(!static_cast<const pix::NullContainer*>(hud.teamArrow(2))->visible);
    // The big map hides every arrow.
    hud.setBigMapVisible(true);
    CHECK(!first->visible);
    CHECK(!second->visible);
    hud.setBigMapVisible(false);
    CHECK(first->visible);
    CHECK(second->visible);

    hud.setTeammateArrows({});
    CHECK(!first->visible);
}

// ui.ts rotation for the up-pointing indicator.svg art: cardinals must point
// at the teammate (right -> +90deg clockwise, up -> 0, ...).
TEST(hud_team_arrow_rotation) {
    constexpr float kPi = 3.14159265358979f;
    CHECK_NEAR(Hud::teamArrowRotation(Vec2(0.0f, 1.0f)), 0.0f, 1e-5f);
    CHECK_NEAR(Hud::teamArrowRotation(Vec2(1.0f, 0.0f)), kPi * 0.5f, 1e-5f);
    CHECK_NEAR(Hud::teamArrowRotation(Vec2(-1.0f, 0.0f)), -kPi * 0.5f, 1e-5f);
    CHECK_NEAR(std::fabs(Hud::teamArrowRotation(Vec2(0.0f, -1.0f))), kPi, 1e-5f);
    CHECK_NEAR(Hud::teamArrowRotation(Vec2(1.0f, 1.0f)), kPi * 0.25f, 1e-5f);
}

// game.ts RoleAnnouncement: promoted/dead lines go into the kill feed with the
// role's name and colour (or the player's team colour), and the custom feed
// lines are rendered by the HUD.
TEST(role_killfeed_lines) {
    installGeneratedDefs();
    const auto* defs = getDefProvider();

    const RoleRenderDef* leader = defs->role("leader");
    CHECK(leader != nullptr);
    CHECK(leader && leader->name == "Commander");
    CHECK(leader && leader->nameRed == "Red Commander");
    CHECK(leader && leader->nameBlue == "Blue Commander");
    CHECK(leader && leader->killFeedAssign);
    CHECK(leader && leader->killFeedDead);

    RoleFeedLine promoted = roleFeedLine(*leader, "leader", true, false, 1, "Alice", "");
    CHECK(promoted.show);
    CHECK_EQ(promoted.text, std::string("Alice promoted to Red Commander!"));
    CHECK_EQ(promoted.color, kTeamColors[0]);

    RoleFeedLine killed = roleFeedLine(*leader, "leader", false, true, 2, "", "Bob");
    CHECK(killed.show);
    CHECK_EQ(killed.text, std::string("Bob killed Blue Commander!"));
    CHECK_EQ(killed.color, kTeamColors[1]);

    RoleFeedLine died = roleFeedLine(*leader, "leader", false, true, 1, "", "");
    CHECK(died.show);
    CHECK_EQ(died.text, std::string("Red Commander is dead!"));

    // kill_leader carries its own orange colour.
    const RoleRenderDef* killLeader = defs->role("kill_leader");
    CHECK(killLeader != nullptr);
    CHECK(killLeader && killLeader->hasKillFeedColor);
    CHECK(killLeader && killLeader->killFeedColor == 0xff8400u);
    RoleFeedLine leaderLine = roleFeedLine(*killLeader, "kill_leader", true, false, 1, "Alice", "");
    CHECK_EQ(leaderLine.color, 0xff8400u);
    CHECK_EQ(leaderLine.text, std::string("Alice promoted to Kill Leader!"));

    // Captain only announces promotions (killFeed.assign, no dead line).
    const RoleRenderDef* captain = defs->role("captain");
    CHECK(captain != nullptr);
    CHECK(captain && captain->killFeedAssign);
    CHECK(captain && !captain->killFeedDead);
    CHECK(!roleFeedLine(*captain, "captain", false, true, 1, "", "Bob").show);

    // HUD renders custom role lines (Game::addKillFeedText).
    pix::NullPixiFactory factory;
    auto* root = static_cast<pix::NullContainer*>(factory.createContainer());
    Hud hud;
    hud.build(&factory, root, Vec2(1280.0f, 720.0f));
    Game game;
    game.addKillFeedText("Alice promoted to Commander!", 0xff8400);
    hud.update(0.0f, game, true);
    CHECK(static_cast<const pix::NullText*>(hud.killFeedRowText(0)) != nullptr);
    CHECK_EQ(static_cast<const pix::NullText*>(hud.killFeedRowText(0))->text,
             std::string("Alice promoted to Commander!"));
}

// ui.ts #ui-gas-icon: gas.svg while waiting, danger.svg while the gas moves,
// with the danger pulse animation.
TEST(hud_gas_icon_state) {
    pix::NullPixiFactory factory;
    auto* root = static_cast<pix::NullContainer*>(factory.createContainer());
    Hud hud;
    hud.build(&factory, root, Vec2(1280.0f, 720.0f));
    hud.setVisible(true);
    const auto* icon = static_cast<const pix::NullSprite*>(hud.gasIcon());
    CHECK(icon != nullptr);
    CHECK_EQ(icon->frame, std::string("gui/gas.png"));

    GasHudState gas;
    gas.active = true;
    gas.moving = false;
    gas.secondsLeft = 90.0f;
    hud.setGasState(gas);
    CHECK(icon->visible);
    CHECK_EQ(icon->frame, std::string("gui/gas.png"));

    gas.moving = true;
    hud.setGasState(gas);
    CHECK_EQ(icon->frame, std::string("gui/danger.png"));
    Game game;
    hud.update(0.5f, game, true);
    CHECK(icon->alpha < 1.0f);

    gas.active = false;
    hud.setGasState(gas);
    CHECK(!icon->visible);
}

namespace {
// Serializes an UpdateMsg for the fake connection (see tests/test_game.cpp).
std::vector<uint8_t> updateFrame(UpdateMsg& msg) {
    MsgStream stream(std::vector<uint8_t>(1024, 0));
    stream.serializeMsg(MsgType_Update, [&msg](NetBitStream& s) { msg.serialize(s); });
    return stream.getBuffer();
}
} // namespace

TEST(hud_health_depleted_trail) {
    // #ui-health-depleted: the grey trail lags the fill with the web's 1 s
    // ease-out width transition (game.css #ui-health-counter #ui-health-depleted).
    Game game;
    std::vector<FakeConnection*> created;
    game.init(nullptr, [&created](const std::string& url) -> std::unique_ptr<Connection> {
        auto* conn = new FakeConnection(url);
        created.push_back(conn);
        return std::unique_ptr<Connection>(conn);
    });
    game.tryJoinGame("ws://dev/play", "tok");
    CHECK_EQ(created.size(), size_t(1));
    if (created.empty()) return;
    created.back()->open();
    game.update(0.0f);

    UpdateMsg update;
    FullObjectData fo;
    fo.data.__id = 42;
    fo.data.__type = ObjectType_Player;
    fo.data.pos = Vec2(12.5f, 25.75f);
    fo.data.dir = Vec2(0.1f, -0.99f);
    fo.data.outfit = "outfitBase";
    fo.data.backpack = "backpack00";
    fo.data.helmet = "helmet02";
    fo.data.chest = "chest02";
    fo.data.activeWeapon = "ak47";
    fo.data.layer = 1;
    fo.data.scale = 1.1f;
    fo.data.role = "";
    update.fullObjects.push_back(fo);
    update.activePlayerId = 42;
    update.activePlayerIdDirty = true;
    update.activePlayerData.healthDirty = true;
    update.activePlayerData.health = 100.0f;
    PlayerInfo pi;
    pi.playerId = 42;
    pi.teamId = 1;
    pi.groupId = 0;
    pi.name = "Player";
    pi.heal = "bandage";
    pi.boost = "soda";
    update.playerInfos.push_back(pi);
    created.back()->pushFrame(updateFrame(update));
    game.update(0.0f);
    CHECK_NEAR(game.activePlayerData().health, 100.0f, 0.01f);

    pix::NullPixiFactory factory;
    pix::Container* root = factory.createContainer();
    Hud hud;
    hud.build(&factory, root, Vec2(1280.0f, 720.0f));
    hud.setVisible(true);
    hud.update(0.016f, game, true);
    // The first render starts the trail at 0 and settles at 100 after ~1 s.
    for (int i = 0; i < 70; ++i) hud.update(0.016f, game, true);
    CHECK_NEAR(hud.healthDepletedWidth(), 100.0f, 0.5f);

    // Damage to 50: the fill snaps to 50%, the grey trail is still wide, then
    // settles at 50 over the 1 s transition.
    update.activePlayerData.healthDirty = true;
    update.activePlayerData.health = 50.0f;
    created.back()->pushFrame(updateFrame(update));
    game.update(0.0f);
    // The wire quantises health (byte-scaled), so compare against the value the
    // client actually received.
    const float damagedHealth = game.activePlayerData().health;
    CHECK(damagedHealth > 49.0f && damagedHealth < 51.0f);
    hud.update(0.016f, game, true);
    CHECK(hud.healthDepletedWidth() > damagedHealth);
    CHECK(hud.healthDepletedWidth() < 100.0f);
    for (int i = 0; i < 70; ++i) {
        hud.update(0.016f, game, true);
    }
    CHECK_NEAR(hud.healthDepletedWidth(), damagedHealth, 0.5f);

    // The graphics is a #929292 rect at the new width (400px * 50%).
    const auto* depleted = static_cast<const pix::NullGraphics*>(hud.healthDepleted());
    CHECK(depleted != nullptr);
    bool sawFill = false;
    bool sawRect = false;
    if (depleted) {
        for (const auto& cmd : depleted->commands) {
            if (cmd.kind == pix::DrawCommand::BeginFill) {
                CHECK_EQ(cmd.color, 0x929292u);
                sawFill = true;
            } else if (cmd.kind == pix::DrawCommand::Rect) {
                CHECK_NEAR(cmd.c, 400.0f * damagedHealth / 100.0f, 0.5f);
                CHECK_NEAR(cmd.d, 32.0f, 0.5f);
                sawRect = true;
            }
        }
    }
    CHECK(sawFill);
    CHECK(sawRect);
}

TEST(hud_faction_flair) {
    // ui.ts faction flair: the red/blue player patch beside the health counter
    // (map.factionMode only; team 1 = red, team 2 = blue).
    pix::NullPixiFactory factory;
    pix::Container* root = factory.createContainer();
    Hud hud;
    hud.build(&factory, root, Vec2(1280.0f, 720.0f));
    hud.setVisible(true);
    Game game;

    const auto* left = static_cast<const pix::NullSprite*>(hud.flairLeft());
    const auto* right = static_cast<const pix::NullSprite*>(hud.flairRight());
    CHECK(left != nullptr && right != nullptr);
    if (!left || !right) return;
    CHECK(!left->visible && !right->visible);

    hud.setFactionMode(true);
    hud.setLocalTeamId(1);
    hud.update(0.016f, game, true);
    CHECK(left->visible && right->visible);
    CHECK_EQ(left->frame, std::string("gui/player-patch-red.png"));
    CHECK_EQ(right->frame, std::string("gui/player-patch-red.png"));

    hud.setLocalTeamId(2);
    hud.update(0.016f, game, true);
    CHECK_EQ(left->frame, std::string("gui/player-patch-blue.png"));

    hud.setFactionMode(false);
    hud.update(0.016f, game, true);
    CHECK(!left->visible && !right->visible);

    hud.setFactionMode(true);
    hud.setLocalTeamId(0); // solo/team id: no faction patch
    hud.update(0.016f, game, true);
    CHECK(!left->visible && !right->visible);
}
