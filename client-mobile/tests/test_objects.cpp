// Tests for the object barns ported on the airdrop/plane/flare/shot pass
// (client/src/objects/{airdrop,plane,flare,shot}.ts). Host-only: the render
// adapter is the headless NullPixi recording implementation.
#include "TestFramework.h"
#include "render/NullPixi.h"

#include "game/GameWorld.h"
#include "game/AimLine.h"
#include "game/objects/Barns.h"
#include "game/TouchInput.h"
#include "net/Messages.h"
#include "render/Defs.h"
#include "render/GeneratedDefs.h"

#include <string>
#include <vector>

using namespace surv;
using namespace surv_test;

namespace {
// The recording adapter exposes a few extra accessors the tests assert against.
pix::NullSprite* asSprite(pix::Sprite* s) { return static_cast<pix::NullSprite*>(s); }

// Minimal playable map for the GameWorld (mirrors test_render's fixture).
MapMsg basicMap() {
    MapMsg msg;
    msg.mapName = "main";
    msg.seed = 123456;
    msg.width = 512.0f;
    msg.height = 512.0f;
    msg.shoreInset = 4.0f;
    msg.grassInset = 2.0f;
    return msg;
}

} // namespace

// ---------------------------------------------------------------------------
// Generated defs: the new GameConfig tables (planes, tracers, bullets).
// ---------------------------------------------------------------------------
TEST(plane_and_bullet_defs) {
    installGeneratedDefs();
    const DefProvider* provider = getDefProvider();
    CHECK(provider != nullptr);
    if (!provider) {
        return;
    }

    // GameConfig.Plane.Airdrop = 0 / Airstrike = 1 (gameConfig.ts).
    const PlaneDef* airdrop = provider->plane(0);
    const PlaneDef* airstrike = provider->plane(1);
    CHECK(airdrop != nullptr);
    CHECK(airstrike != nullptr);
    if (airdrop && airstrike) {
        CHECK_NEAR(airdrop->planeVel, 48.0f, 1e-4f);
        CHECK_NEAR(airdrop->planeRad, 150.0f, 1e-4f);
        CHECK_NEAR(airdrop->soundRangeMax, 92.0f, 1e-4f);
        CHECK_NEAR(airstrike->planeVel, 350.0f, 1e-4f);
        CHECK_NEAR(airstrike->planeRad, 120.0f, 1e-4f);
        CHECK_NEAR(airstrike->fallOff, 1.25f, 1e-4f);
    }

    // Tracer palette used by flare.ts / shot.ts.
    const TracerColorDef* nine = provider->tracerColor("9mm");
    CHECK(nine != nullptr);
    if (nine) {
        CHECK_EQ(nine->regular, 0xfee2c6u);
        CHECK_EQ(nine->saturated, 0xffd9b3u);
        CHECK_NEAR(nine->alphaRate, 0.92f, 1e-4f);
        CHECK_NEAR(nine->alphaMin, 0.14f, 1e-4f);
    }

    // Flare bullet def carries the flare colour + max flare scale.
    const BulletRenderDef* flare = provider->bullet("bullet_flare");
    CHECK(flare != nullptr);
    if (flare) {
        CHECK(flare->distance > 0.0f);
        CHECK(flare->tracerLength > 0.0f);
        CHECK_EQ(flare->tracerColor, std::string("flare"));
    }
}

// ---------------------------------------------------------------------------
// Airdrop: parachute fall + landed flag; the crate is a real object type.
// ---------------------------------------------------------------------------
TEST(airdrop_falls_and_lands) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    world.loadMap(basicMap());

    // The object type is registered (a silent drop before this pass).
    Airdrop* a = world.airdropBarn().airdropPool.m_alloc();
    ObjectData d;
    d.__id = 7;
    d.__type = ObjectType_Airdrop;
    d.pos = Vec2(100.0f, 100.0f);
    d.fallT = 0.0f;
    d.landed = false;

    // AirdropBarn::update only walks active pool entries and needs an active
    // player for the render-layer decision.
    Player* active = world.playerBarn().playerPool.m_alloc();
    active->__id = 1;
    active->active = true;
    active->layer = 0;
    active->pos = Vec2(100.0f, 100.0f);
    world.setActivePlayer(active);

    a->m_updateData(d, true, true, world);
    CHECK(a->sprite != nullptr);
    if (a->sprite) {
        CHECK_EQ(asSprite(a->sprite)->getFrame(), std::string("map-chute-01.img"));
    }
    CHECK(!a->landed);

    // A few frames of fall keep the chute visible.
    for (int i = 0; i < 10; i++) {
        world.airdropBarn().update(1.0f / 60.0f, world);
    }
    CHECK(a->sprite->isVisible());
    CHECK(a->fallTicker > 0.0f);
    CHECK(a->rad > 5.0f);

    // Landed: the sprite hides and the landing FX latches exactly once.
    ObjectData landed = d;
    landed.landed = true;
    a->m_updateData(landed, false, false, world);
    world.airdropBarn().update(1.0f / 60.0f, world);
    CHECK(a->playedLandFx);
    CHECK(!a->sprite->isVisible());

    // Snow biome overrides the chute sprite.
    const MapRenderDef* snow = getDefProvider()->mapRender("snow");
    CHECK(snow != nullptr);
    if (snow) {
        CHECK_EQ(snow->airdropChuteImg, std::string("map-chute-01x.img"));
        CHECK_EQ(snow->airdropPlaneSound, std::string("plane_02"));
    }
}

// ---------------------------------------------------------------------------
// Plane: flyover reconciliation + sprite/sound selection per biome.
// ---------------------------------------------------------------------------
TEST(plane_reconcile_and_elevate) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    world.loadMap(basicMap());

    Player* active = world.playerBarn().playerPool.m_alloc();
    active->__id = 1;
    active->active = true;
    active->layer = 0;
    active->pos = Vec2(256.0f, 256.0f);
    world.setActivePlayer(active);

    PlaneData pd;
    pd.id = 3;
    pd.pos = Vec2(10.0f, 10.0f);
    pd.planeDir = Vec2(1.0f, 0.0f);
    pd.actionComplete = false;
    pd.action = 0; // airdrop plane

    world.updatePlanes({pd});
    CHECK_EQ(world.planeBarn().planes.size(), 1u);
    PlaneObj* plane = world.planeBarn().planes[0];
    CHECK(plane->active);
    CHECK(plane->sprite != nullptr);
    CHECK_EQ(plane->planeSound, std::string("plane_01"));
    CHECK_EQ(plane->type, 0);
    CHECK_NEAR(plane->rad, 150.0f, 1e-4f);
    CHECK_NEAR(plane->pos.x, 10.0f, 1e-4f);
    if (plane->sprite) {
        CHECK_EQ(asSprite(plane->sprite)->getFrame(), std::string("map-plane-01.img"));
    }

    // Each frame advances along planeDir at planeVel.
    world.planeBarn().update(1.0f, world);
    CHECK_NEAR(plane->pos.x, 58.0f, 1e-3f);

    // A 5-unit server correction is ignored; a 20-unit one is applied.
    PlaneData near = pd;
    near.pos = Vec2(plane->pos.x + 5.0f, plane->pos.y);
    world.updatePlanes({near});
    CHECK_NEAR(plane->pos.x, 58.0f, 1e-3f);
    PlaneData far = pd;
    far.pos = Vec2(plane->pos.x + 20.0f, plane->pos.y);
    world.updatePlanes({far});
    CHECK_NEAR(plane->pos.x, 78.0f, 1e-3f);

    // An empty plane list retires the plane.
    world.updatePlanes({});
    CHECK(!plane->active);
    if (plane->sprite) {
        CHECK(!plane->sprite->isVisible());
    }

    // Airstrike planes use the fixed fighter sprite/sound and GameConfig.
    PlaneData strikeData = pd;
    strikeData.action = 1;
    PlaneObj strike;
    strike.init(world, strikeData, world.map());
    CHECK_EQ(strike.planeSound, std::string("fighter_01"));
    if (strike.sprite) {
        CHECK_EQ(asSprite(strike.sprite)->getFrame(), std::string("map-plane-02.img"));
    }
}

// ---------------------------------------------------------------------------
// Flare: the flare gun bullet spawns a swelling flare + fading trail.
// ---------------------------------------------------------------------------
TEST(flare_bullet_visual) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    world.loadMap(basicMap());

    Player* active = world.playerBarn().playerPool.m_alloc();
    active->__id = 1;
    active->active = true;
    active->layer = 0;
    world.setActivePlayer(active);

    Bullet b;
    b.playerId = 1;
    b.bulletType = "bullet_flare";
    b.pos = Vec2(100.0f, 100.0f);
    b.dir = Vec2(1.0f, 0.0f);
    b.layer = 0;
    b.distAdjIdx = 16;
    b.varianceT = 0.0f;
    b.reflectCount = 0;

    world.flareBarn().addFlare(world, b);
    CHECK_EQ(world.flareBarn().bullets.size(), 1u);
    FlareVisual* f = world.flareBarn().bullets[0];
    CHECK(f->alive);
    CHECK(f->container != nullptr);
    CHECK(f->flare != nullptr);
    CHECK(f->trail != nullptr);
    CHECK(f->speed > 0.0f);
    CHECK(f->distance > 0.0f);
    CHECK_NEAR(f->flareScale, 0.01f, 1e-5f);
    // The flare is tinted with the bullet def's flare colour.
    const BulletRenderDef* def = getDefProvider()->bullet("bullet_flare");
    CHECK(def != nullptr);
    if (def && f->flare) {
        CHECK_EQ(asSprite(f->flare)->getTint(), def->flareColor);
        CHECK_NEAR(f->maxFlareScale, def->maxFlareScale, 1e-5f);
    }

    // It grows over time and eventually collides (burning out, then fading).
    world.flareBarn().update(0.2f, world);
    CHECK(f->flareScale > 0.01f);
    for (int i = 0; i < 40 && (f->alive || f->collided); i++) {
        world.flareBarn().update(0.2f, world);
    }
    CHECK(!f->alive);
}

// ---------------------------------------------------------------------------
// Shot: a shotFx bullet drives the weapon recoil on the owner's hands.
// ---------------------------------------------------------------------------
TEST(shot_handles_recoil_and_dual) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    world.loadMap(basicMap());

    Player* p = world.playerBarn().playerPool.m_alloc();
    p->__id = 42;
    p->active = true;
    p->container = factory.createContainer();
    world.playerBarn().playerPool.m_getPool();

    // Dual-wield offhand shot: only the left hand kicks.
    Bullet b;
    b.playerId = 42;
    b.shotFx = true;
    b.shotSourceType = "m9_dual";
    b.shotOffhand = true;
    b.pos = Vec2(10.0f, 10.0f);
    world.shotBarn().addShot(b);
    CHECK_EQ(world.shotBarn().shots.size(), 1u);
    ShotObj* shot = world.shotBarn().shots[0];
    CHECK(shot->active);
    CHECK_EQ(shot->weaponType, std::string("m9_dual"));
    CHECK(shot->offhand);

    world.shotBarn().update(0.0f, world);
    CHECK(p->gunRecoil[0] > 0.0f);
    CHECK_NEAR(p->gunRecoil[1], 0.0f, 1e-5f);

    // The shot is released once the pull delay elapses (0 for a gun without one).
    world.shotBarn().update(1.0f, world);
    CHECK(!shot->active);

    // Non-dual weapons kick both hands.
    p->gunRecoil = {0.0f, 0.0f};
    Bullet b2;
    b2.playerId = 42;
    b2.shotFx = true;
    b2.shotSourceType = "ak47";
    b2.shotOffhand = false;
    world.shotBarn().addShot(b2);
    world.shotBarn().update(0.0f, world);
    CHECK(p->gunRecoil[0] > 0.0f);
    CHECK(p->gunRecoil[1] > 0.0f);
}

// ---------------------------------------------------------------------------
// Airdrop objects flow through UpdateMsg into the barn (end to end).
// ---------------------------------------------------------------------------
TEST(airdrop_via_update_message) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    world.loadMap(basicMap());

    UpdateMsg msg;
    FullObjectData full;
    full.data.__id = 9;
    full.data.__type = ObjectType_Airdrop;
    full.data.pos = Vec2(200.0f, 200.0f);
    full.data.fallT = 0.25f;
    full.data.landed = false;
    msg.fullObjects.push_back(full);
    world.applyUpdate(msg);

    Airdrop* airdrop = nullptr;
    for (auto* a : world.airdropBarn().airdropPool.m_getPool()) {
        if (a->active && a->__id == 9) {
            airdrop = a;
            break;
        }
    }
    CHECK(airdrop != nullptr);
    if (airdrop) {
        // fallTicker starts at fallT * fallTime (airdrop.ts).
        CHECK_NEAR(airdrop->fallTicker, 0.25f * defs::kAirdropFallTime, 1e-4f);
        CHECK_EQ(airdrop->pos.x, 200.0f);
    }
}

// ---------------------------------------------------------------------------
// SmokeParticle: the sprite survives a world reset (the adapter nodes are
// factory-owned and are released together with the factory).
// ---------------------------------------------------------------------------
namespace {
// Counts sprite allocations so the test can tell a re-create from a reuse.
struct CountingFactory : public pix::NullPixiFactory {
    int sprites = 0;
    pix::Sprite* createSprite() override { sprites++; return pix::NullPixiFactory::createSprite(); }
};
} // namespace

// SmokeParticle::ensureSprite() creates exactly one sprite per particle and
// reuses it afterwards.
TEST(smoke_particle_ensure_sprite) {
    CountingFactory factory;
    SmokeParticle particle;
    particle.m_init(&factory, Vec2(10.0f, 20.0f), 4.0f, 0, 0);
    CHECK(particle.sprite != nullptr);
    CHECK_EQ(factory.sprites, 1);

    pix::Sprite* first = particle.sprite;
    particle.ensureSprite();
    CHECK(particle.sprite == first);
    CHECK_EQ(factory.sprites, 1);

    // m_init() keeps the existing sprite (no re-allocation).
    particle.m_init(&factory, Vec2(11.0f, 21.0f), 4.0f, 0, 0);
    CHECK(particle.sprite == first);
    CHECK_EQ(factory.sprites, 1);

    // With no factory cached, ensureSprite() cannot create one.
    SmokeParticle orphan;
    orphan.ensureSprite();
    CHECK(orphan.sprite == nullptr);
}

// SmokeBarn::onFactoryReset() clears every particle so the next world re-creates
// its sprite instead of dereferencing a node the (destroyed) factory owned.
TEST(smoke_barn_factory_reset) {
    CountingFactory factory;
    SmokeBarn barn;
    SmokeParticle* a = barn.m_allocParticle(&factory);
    SmokeParticle* b = barn.m_allocParticle(&factory);
    CHECK(a != nullptr);
    CHECK(b != nullptr);
    if (a) {
        a->m_init(&factory, Vec2(1.0f, 2.0f), 3.0f, 0, 0);
        CHECK(a->sprite != nullptr);
    }
    barn.onFactoryReset();
    CHECK(a->sprite == nullptr);
    CHECK(a->factory == nullptr);
    CHECK(b->sprite == nullptr);

    // The next world re-creates the sprite lazily (new factory, same pool).
    a->m_init(&factory, Vec2(3.0f, 4.0f), 3.0f, 0, 0);
    CHECK(a->sprite != nullptr);
}

// ---------------------------------------------------------------------------
// touch.ts LineSprites: the dotted aim line (M5 parity gap).
// ---------------------------------------------------------------------------
namespace {
// A live player with a loaded gun, standing on an empty map.
Player* addGunPlayer(GameWorld& world, const std::string& weapon, float x, float y) {
    Player* p = world.playerBarn().playerPool.m_alloc();
    p->__id = 1;
    p->__type = ObjectType_Player;
    p->active = true;
    p->layer = 0;
    p->pos = Vec2(x, y);
    p->dir = Vec2(1.0f, 0.0f);
    p->activeWeapon = weapon;
    p->zoom = 0.0f;
    world.setActivePlayer(p);
    world.setActivePlayerId(1);
    return p;
}

bool lineVisible(GameWorld& world) {
    return world.lineSprites().container && world.lineSprites().container->isVisible();
}
} // namespace

// The gun def carries the barrel length + bullet type the line needs.
TEST(aim_line_gun_def_fields) {
    installGeneratedDefs();
    const DefProvider* provider = getDefProvider();
    CHECK(provider != nullptr);
    if (!provider) {
        return;
    }
    const GameObjRenderDef* mosin = provider->gameObject("mosin");
    CHECK(mosin != nullptr);
    if (mosin) {
        CHECK_EQ(mosin->category, std::string("gun"));
        CHECK_EQ(mosin->bulletType, std::string("bullet_mosin"));
        // gunDefs.ts barrelLength (mosin = 3.95).
        CHECK_NEAR(mosin->barrelLength, 3.95f, 1e-4f);
    }
    // The mosin range is barrelLength + BulletDefs.bullet_mosin.distance.
    CHECK_NEAR(mosin->barrelLength + provider->bullet(mosin->bulletType)->distance, 503.95f, 1e-3f);
    const BulletRenderDef* bullet = provider->bullet("bullet_mosin");
    CHECK(bullet != nullptr);
    if (bullet) {
        // BulletDefs.bullet_mosin.distance = 500 (the aim-line range, not speed).
        CHECK_NEAR(bullet->distance, 500.0f, 1e-4f);
    }
    // Non-guns keep the 30-unit default: no bullet/barrel data.
    const GameObjRenderDef* fists = provider->gameObject("fists");
    CHECK(fists != nullptr);
    if (fists) {
        CHECK_EQ(fists->category, std::string("melee"));
        CHECK(fists->bulletType.empty());
    }
}

// Fists (no barrel) render dots out to the default 30-unit range, scaled 0.3.
TEST(aim_line_default_range) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    world.loadMap(basicMap());
    Player* p = addGunPlayer(world, "fists", 256.0f, 256.0f);

    LineSprites line;
    line.update(true, *p, world.map(), world.camera(), world.renderer(), &factory, true);

    CHECK(line.container != nullptr);
    CHECK(lineVisible(world) == false); // not attached to the renderer yet
    CHECK(line.container->isVisible());
    CHECK_NEAR(line.container->getAlpha(), 0.3f, 1e-4f);
    // dist = 30, startOffset 3.5, increment 1.5 -> ceil(26.5/1.5) = 18 dots.
    CHECK_EQ(line.dots.size(), 18u);
    CHECK_EQ(asSprite(line.dots[0])->getFrame(), std::string("dot.img"));
    CHECK_NEAR(asSprite(line.dots[0])->getScaleX(), (1.0f / 32.0f) * 0.375f, 1e-6f);
    CHECK_NEAR(asSprite(line.dots[0])->x, 256.0f + 3.5f, 1e-4f);
    CHECK_NEAR(asSprite(line.dots[1])->x, 256.0f + 5.0f, 1e-4f);
    CHECK(!line.dots[17]->isVisible());
    CHECK(line.dots[16]->isVisible());

    // Hidden when the aim pad is not touched / the option is off.
    line.update(false, *p, world.map(), world.camera(), world.renderer(), &factory, true);
    CHECK(!line.container->isVisible());
}

// A gun's range is barrelLength + bullet distance, clamped to the camera radius.
TEST(aim_line_gun_range_and_clamp) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    world.loadMap(basicMap());
    Player* p = addGunPlayer(world, "mosin", 256.0f, 256.0f);

    LineSprites line;
    // Unscoped (zoom 0): cameraRad = sqrt(0 * 1.414 * 0) = 0 -> clamped to 0.
    line.update(true, *p, world.map(), world.camera(), world.renderer(), &factory, true);
    CHECK_EQ(line.dots.size(), 0u);

    // 1x scope (desktop 28 -> mobile 32): cameraRad = sqrt(32 * 1.414 * 32) = 38.05,
    // still below the mosin's 3.95 + 178 = 181.95 range.
    p->zoom = 28.0f;
    CHECK_NEAR(p->getZoom(true), 32.0f, 1e-4f);
    CHECK_NEAR(p->getZoom(false), 28.0f, 1e-4f);
    line.update(true, *p, world.map(), world.camera(), world.renderer(), &factory, true);
    const float rad = std::sqrt(32.0f * 1.414f * 32.0f);
    const int expected = static_cast<int>(std::ceil((rad - 3.5f) / 1.5f));
    CHECK_EQ(static_cast<int>(line.dots.size()), expected);
    CHECK(line.dots.size() > 18u);
}

// The line is clipped at the first obstacle that blocks bullets.
TEST(aim_line_stops_at_obstacle) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    world.loadMap(basicMap());
    Player* p = addGunPlayer(world, "mosin", 100.0f, 100.0f);
    p->zoom = 28.0f;

    // A tall, collidable wall 10 units ahead (obstacle.ts sets these from the def).
    Obstacle* wall = world.map().obstaclePool.m_alloc();
    wall->__id = 5;
    wall->__type = ObjectType_Obstacle;
    wall->active = true;
    wall->layer = 0;
    wall->pos = Vec2(110.0f, 100.0f);
    wall->collider = Collider::createAabb(Vec2(109.0f, 95.0f), Vec2(111.0f, 105.0f));
    wall->height = 2.0f;
    wall->collidable = true;

    LineSprites line;
    line.update(true, *p, world.map(), world.camera(), world.renderer(), &factory, true);
    // Clipped at x = 109 -> dist 9 -> ceil((9 - 3.5) / 1.5) = 4 dots.
    CHECK_EQ(line.dots.size(), 4u);
    CHECK_NEAR(asSprite(line.dots[3])->x, 100.0f + 3.5f + 3.0f * 1.5f, 1e-3f);

    // A window (or a low obstacle) does not block the aim line.
    wall->isWindow = true;
    line.update(true, *p, world.map(), world.camera(), world.renderer(), &factory, true);
    CHECK(line.dots.size() > 4u);
    wall->isWindow = false;
    wall->height = 0.1f; // below GameConfig.bullet.height
    line.update(true, *p, world.map(), world.camera(), world.renderer(), &factory, true);
    CHECK(line.dots.size() > 4u);
}
