// Tests for the object barns ported on the airdrop/plane/flare/shot pass
// (client/src/objects/{airdrop,plane,flare,shot}.ts). Host-only: the render
// adapter is the headless NullPixi recording implementation.
#include "TestFramework.h"
#include "render/NullPixi.h"

#include "game/GameWorld.h"
#include "game/AimLine.h"
#include "game/objects/Barns.h"
#include "game/TouchInput.h"
#include "audio/AudioManager.h"
#include "audio/GeneratedSoundDefs.h"
#include "net/Messages.h"
#include "render/Defs.h"
#include "render/GeneratedDefs.h"

#include <cmath>
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

// Records the sound ids the ShotBarn asks for (id = sound + channel, like
// test_audio's backend) so the fire-cue variants can be asserted.
class ShotAudioBackend : public audio::AudioBackend {
public:
    std::vector<std::string> played;
    std::vector<audio::SoundHandle> stopped;
    bool muted = false;
    audio::SoundHandle nextHandle = 1;

    bool load(const std::string&, const std::string&) override { return true; }
    bool isLoaded(const std::string&) const override { return true; }
    audio::SoundHandle play(const std::string& id, const audio::PlayOptions&) override {
        played.push_back(id);
        return nextHandle++;
    }
    void stop(audio::SoundHandle handle) override { stopped.push_back(handle); }
    void stopAll() override { stopped.clear(); }
    void setVolume(audio::SoundHandle, float) override {}
    float getVolume(audio::SoundHandle) const override { return 1.0f; }
    void setPan(audio::SoundHandle, float) override {}
    void setMute(bool mute) override { muted = mute; }
    bool isPlaying(audio::SoundHandle) const override { return true; }
};

int countPlayed(const ShotAudioBackend& backend, const std::string& id) {
    int n = 0;
    for (const auto& played : backend.played) {
        if (played == id) {
            ++n;
        }
    }
    return n;
}

// A GameWorld with a local player (id 7) and a recording audio manager.
struct ShotFixture {
    pix::NullPixiFactory factory;
    ShotAudioBackend backend;
    audio::AudioManager manager{&backend};
    GameWorld world{&factory, false};
    Player* player = nullptr;

    explicit ShotFixture(uint8_t teamId) {
        installGeneratedDefs();
        audio::installGeneratedSoundDefs();
        manager.preloadSounds();
        world.setAudio(&manager, nullptr);
        world.loadMap(basicMap());
        player = world.playerBarn().playerPool.m_alloc();
        player->__id = 7;
        player->active = true;
        player->teamId = teamId;
        player->container = factory.createContainer();
        world.setActivePlayerId(7);
        world.setActivePlayer(player);
    }

    ShotObj* fire(const std::string& weapon, bool lastShot = false, bool alt = false) {
        player->activeWeapon = weapon;
        Bullet bullet;
        bullet.playerId = 7;
        bullet.shotFx = true;
        bullet.shotSourceType = weapon;
        bullet.lastShot = lastShot;
        bullet.shotAlt = alt;
        bullet.pos = Vec2(10.0f, 10.0f);
        world.shotBarn().addShot(bullet);
        world.shotBarn().update(0.01f, world);
        return world.shotBarn().shots.back();
    }
};

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

// Codegen carries the item action/pickup sounds added for P1.5 (they used to
// be guessed as "<type>_01" / "<item>_pickup_01" in the port).
TEST(game_object_sound_fields) {
    installGeneratedDefs();
    const DefProvider* provider = getDefProvider();
    CHECK(provider != nullptr);
    if (!provider) {
        return;
    }
    const GameObjRenderDef* mp5 = provider->gameObject("mp5");
    CHECK(mp5 != nullptr);
    if (mp5) {
        CHECK_EQ(mp5->soundReload, std::string("mp5_reload_01"));
        CHECK_EQ(mp5->soundDeploy, std::string("mp5_switch_01"));
        CHECK_EQ(mp5->soundPickup, std::string("gun_pickup_01"));
    }
    const GameObjRenderDef* bandage = provider->gameObject("bandage");
    CHECK(bandage != nullptr);
    if (bandage) {
        CHECK_EQ(bandage->soundUse, std::string("bandage_use_01"));
        CHECK_EQ(bandage->soundPickup, std::string("bandage_pickup_01"));
    }
    const GameObjRenderDef* mp220 = provider->gameObject("mp220");
    CHECK(mp220 != nullptr);
    if (mp220) {
        // Some guns carry a separate alt-reload sound.
        CHECK(mp220->soundReloadAlt.empty() || mp220->soundReloadAlt != mp220->soundReload);
    }
}

// P1.3: generated explosion effect table (burst/scatter/ripple/shake/lifetime).
TEST(explosion_effect_defs_provider) {
    installGeneratedDefs();
    const DefProvider* provider = getDefProvider();
    CHECK(provider != nullptr);
    if (!provider) {
        return;
    }
    const ExplosionEffectRenderDef* frag = provider->explosionEffect("frag");
    CHECK(frag != nullptr);
    if (frag) {
        CHECK_EQ(frag->burstParticle, std::string("explosionBurst"));
        CHECK_NEAR(frag->burstScale, 1.0f, 1e-4f);
        CHECK_NEAR(frag->lifetime, 2.0f, 1e-4f);
        CHECK_NEAR(frag->shakeStr, 0.2f, 1e-4f);
        CHECK_EQ(frag->rippleCount, 10);
    }
    const ExplosionEffectRenderDef* snowball = provider->explosionEffect("snowball");
    CHECK(snowball != nullptr);
    if (snowball) {
        CHECK_EQ(snowball->scatterParticle, std::string("snowball_impact"));
        CHECK_EQ(snowball->scatterCount, 5);
    }
    const GameObjRenderDef* explosion = provider->gameObject("explosion_frag");
    CHECK(explosion != nullptr);
    if (explosion) {
        CHECK_EQ(explosion->explosionEffectType, std::string("frag"));
    }
}

// P1.3: bullet.ts moves the tracer to the server distance and stops on impact.
TEST(bullet_trail_moves_and_collides) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    world.loadMap(basicMap());
    world.camera().m_screenWidth = 800.0f;
    world.camera().m_screenHeight = 600.0f;

    Bullet data;
    data.playerId = 2;
    data.bulletType = "bullet_mosin";
    data.pos = Vec2(100.0f, 100.0f);
    data.dir = Vec2(1.0f, 0.0f);
    data.layer = 0;
    data.clipDistance = true;
    data.distance = 10.0f;
    world.bulletBarn().spawn(&factory, data);
    CHECK_EQ(world.bulletBarn().bullets.size(), size_t(1));
    BulletVisual* b = world.bulletBarn().bullets[0];
    CHECK(b->trail != nullptr);
    CHECK(b->alive);
    CHECK_NEAR(b->speed, getDefProvider()->bullet("bullet_mosin")->speed, 1e-3f);

    world.bulletBarn().update(0.1f, world);
    CHECK_NEAR(b->pos.x, 110.0f, 1e-4f);
    CHECK(!b->alive);
    CHECK(b->collided);

    // bullet.ts rotation: atan2(dir.x, dir.y) - PI/2. A diagonal tracer must
    // not be mirrored across the x axis (the "slash" regression).
    Bullet diagonal = data;
    diagonal.pos = Vec2(200.0f, 200.0f);
    diagonal.dir = Vec2(0.70710678f, 0.70710678f);
    world.bulletBarn().spawn(&factory, diagonal);
    world.bulletBarn().update(0.0f, world);
    BulletVisual* d = world.bulletBarn().bullets.back();
    const auto* dTrail = static_cast<pix::NullSprite*>(d->trail);
    CHECK_NEAR(dTrail->rotation, -3.14159265358979f / 4.0f, 1e-4f);

    // The collided tracer fades out and is deactivated.
    for (int i = 0; i < 30; ++i) {
        world.bulletBarn().update(0.02f, world);
    }
    CHECK(!b->active);
    CHECK(!b->trail->isVisible());
}

// P1.3: loot.ts outer ring + inner art rotation/scale.
TEST(loot_border_and_art) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    world.loadMap(basicMap());
    Loot* loot = world.lootBarn().lootPool.m_alloc();
    CHECK(loot != nullptr);
    if (!loot) {
        return;
    }
    ObjectData data;
    data.type = "mp5";
    data.pos = Vec2(120.0f, 120.0f);
    data.layer = 0;
    loot->m_updateData(data, true, true, world);
    CHECK(loot->borderSprite != nullptr);
    CHECK(loot->borderSprite->isVisible());
    CHECK_EQ(static_cast<pix::NullSprite*>(loot->borderSprite)->frame,
             std::string("loot-circle-outer-01.img"));
    CHECK_NEAR(static_cast<pix::NullSprite*>(loot->sprite)->sx, 0.8f, 1e-4f);
}

// P1.3: projectile.ts spins the sprite and creates a trail for throwables.
TEST(projectile_spin_and_trail) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    world.loadMap(basicMap());
    world.camera().m_screenWidth = 800.0f;
    world.camera().m_screenHeight = 600.0f;

    Projectile* p = world.projectileBarn().projectilePool.m_alloc();
    ObjectData data;
    data.type = "potato_cannonball";
    data.pos = Vec2(100.0f, 100.0f);
    data.dir = Vec2(1.0f, 0.0f);
    data.layer = 0;
    data.posZ = 2.0f;
    p->m_updateData(data, true, true, world);
    CHECK(p->trail != nullptr);
    CHECK(p->rotVel != 0.0f);
    const float rotVel = p->rotVel;
    p->lastPos = Vec2(98.0f, 100.0f);
    world.projectileBarn().update(0.1f, world);
    CHECK(p->rot != 0.0f);
    CHECK(std::fabs(p->rotVel) < std::fabs(rotVel));
    CHECK(p->trail->isVisible());
    // projectile.ts: sprite.rotation = rot and trail.rotation =
    // -atan2(dir.y, dir.x), used verbatim by the pix adapter.
    const auto* pSprite = static_cast<pix::NullSprite*>(p->sprite);
    const auto* pTrail = static_cast<pix::NullSprite*>(p->trail);
    CHECK_NEAR(pSprite->rotation, p->rot, 1e-6f);
    // P2: the additive blend now reaches the adapter (flares/projectile trails).
    CHECK(pTrail->blend == pix::BlendMode::Add);
    p->dir = Vec2(0.70710678f, 0.70710678f);
    p->lastPos = p->pos;
    world.projectileBarn().update(0.0f, world);
    CHECK_NEAR(pTrail->rotation, -3.14159265358979f / 4.0f, 1e-4f);
}

// P2: projectile.ts wall/ground impact cues (thrown grenades hitting a crate or
// landing on the map surface) come from the obstacle def's bullet group and the
// ground-surface group, once per obstacle/landing.
TEST(projectile_impact_cues) {
    installGeneratedDefs();
    audio::installGeneratedSoundDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    ShotAudioBackend backend;
    audio::AudioManager manager(&backend);
    manager.preloadSounds();
    world.setAudio(&manager, nullptr);
    world.loadMap(basicMap());
    world.camera().m_screenWidth = 800.0f;
    world.camera().m_screenHeight = 600.0f;

    const DefProvider* provider = getDefProvider();
    const MapObjectDef* crateDef = provider ? provider->mapObject("crate_01") : nullptr;
    CHECK(crateDef != nullptr);
    Obstacle* crate = world.map().obstaclePool.m_alloc();
    crate->__id = 11;
    crate->type = "crate_01";
    crate->active = true;
    crate->dead = false;
    crate->layer = 0;
    crate->height = 1.0f;
    crate->collider = createAabbExtents(Vec2(256.0f, 256.0f), Vec2(2.25f, 2.25f));
    crate->hitParticle = crateDef ? crateDef->hitParticle : "";

    // Thrown into the crate: 60 units/s (posOld 6 units behind over 0.1 s).
    Projectile* p = world.projectileBarn().projectilePool.m_alloc();
    ObjectData data;
    data.type = "frag";
    data.pos = Vec2(254.0f, 256.0f);
    data.dir = Vec2(1.0f, 0.0f);
    data.layer = 0;
    data.posZ = 0.5f;
    p->m_updateData(data, true, true, world);
    CHECK_NEAR(p->rad, 0.5f, 1e-5f); // itemDef.rad (1) * 0.5
    CHECK(p->playHitSfx);            // frag.explodeOnImpact is false
    p->posOld = Vec2(248.0f, 256.0f);
    manager.cameraPos = Vec2(254.0f, 256.0f);
    const size_t wallPlaysBefore = backend.played.size();
    world.projectileBarn().update(0.1f, world);
    CHECK_EQ(p->lastSoundObjId, 11);
    CHECK(backend.played.size() > wallPlaysBefore);
    CHECK(!world.particleBarn().particles.empty()); // bullet.ts playHitFx particles

    // The same crate does not replay the cue on every frame.
    const size_t afterWall = backend.played.size();
    world.projectileBarn().update(0.1f, world);
    CHECK_EQ(backend.played.size(), afterWall);

    // projectiles that explode on impact (snowball) do not play the wall cue,
    // but the obstacle id is still latched like the web.
    Projectile* snowball = world.projectileBarn().projectilePool.m_alloc();
    data.type = "snowball";
    data.pos = Vec2(254.0f, 256.0f);
    snowball->m_updateData(data, true, true, world);
    CHECK(!snowball->playHitSfx);
    snowball->posOld = Vec2(248.0f, 256.0f);
    const size_t beforeSnowball = backend.played.size();
    world.projectileBarn().update(0.1f, world);
    CHECK_EQ(snowball->lastSoundObjId, 11);
    CHECK_EQ(backend.played.size(), beforeSnowball);

    // Landing: posZ stays 0 after a fall, so velZ flips to >= 0 and the
    // surface group plays once (grass/sand/water -> *frag_** on `hits`,
    // everything else -> footstep_* on `sfx`).
    const Vec2 landPos(200.0f, 200.0f);
    const Map::GroundSurface landSurface = world.map().getGroundSurface(landPos, 0);
    Projectile* g = world.projectileBarn().projectilePool.m_alloc();
    data.type = "frag";
    data.pos = landPos;
    data.posZ = 0.0f;
    g->m_updateData(data, true, true, world);
    g->posZ = 0.0f;
    g->posZOld = 0.0f;
    g->velZ = -2.0f; // falling on the previous tick
    manager.cameraPos = landPos;
    const size_t beforeLanding = backend.played.size();
    world.projectileBarn().update(0.1f, world);
    CHECK(g->grounded);
    CHECK(backend.played.size() > beforeLanding);
    if (backend.played.size() > beforeLanding) {
        const std::string& id = backend.played.back();
        const bool mapped = landSurface.type == Map::SurfaceType::Grass
            || landSurface.type == Map::SurfaceType::Sand
            || landSurface.type == Map::SurfaceType::Water;
        const std::string suffix = mapped ? "hits" : "sfx";
        CHECK(id.size() >= suffix.size()
              && id.compare(id.size() - suffix.size(), suffix.size(), suffix) == 0);
    }
    // The landing cue is once per throw (grounded latches).
    const size_t afterLanding = backend.played.size();
    world.projectileBarn().update(0.1f, world);
    CHECK_EQ(backend.played.size(), afterLanding);
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
        // plane.ts rotation: atan2(planeDir.x, planeDir.y).
        CHECK_NEAR(asSprite(plane->sprite)->rotation, std::atan2(1.0f, 0.0f), 1e-4f);
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
    b.dir = Vec2(0.0f, 1.0f);
    b.layer = 0;
    b.distAdjIdx = 16;
    b.varianceT = 0.0f;
    b.reflectCount = 0;

    world.flareBarn().addFlare(world, b);
    CHECK_EQ(world.flareBarn().bullets.size(), 1u);
    FlareVisual* f = world.flareBarn().bullets[0];
    CHECK(f->alive);
    CHECK(f->container != nullptr);
    // flare.ts rotation: atan2(dir.x, dir.y) - PI/2.
    CHECK_NEAR(static_cast<pix::NullContainer*>(f->container)->rotation,
               std::atan2(0.0f, 1.0f) - 3.14159265358979f / 2.0f, 1e-4f);
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
// Shot: gun fire cues (shot.ts sound.shoot/shootLast/shootAlt/shootTeam,
// ignoreDetune, cycle/pull) come from the generated gun defs.
// ---------------------------------------------------------------------------
TEST(gun_fire_cue_defs) {
    installGeneratedDefs();
    const DefProvider* provider = getDefProvider();
    CHECK(provider != nullptr);
    if (!provider) {
        return;
    }
    const GameObjRenderDef* garand = provider->gameObject("garand");
    CHECK(garand != nullptr);
    if (garand) {
        CHECK_EQ(garand->soundShoot, std::string("garand_01"));
        CHECK_EQ(garand->soundShootLast, std::string("garand_02"));
        CHECK(garand->soundShootAlt.empty());
        CHECK(garand->soundShootTeamRed.empty());
        CHECK(garand->fireModeSingle);
    }
    const GameObjRenderDef* bugle = provider->gameObject("bugle");
    CHECK(bugle != nullptr);
    if (bugle) {
        CHECK_EQ(bugle->soundShootAlt, std::string("bugle_03"));
        CHECK_EQ(bugle->soundShootTeamRed, std::string("bugle_01"));
        CHECK_EQ(bugle->soundShootTeamBlue, std::string("bugle_02"));
        // gameConfig.ts FactionTeam.Red = 1 / Blue = 2.
        CHECK(bugle->ignoreDetune);
    }
    const GameObjRenderDef* scout = provider->gameObject("scout_elite");
    CHECK(scout != nullptr);
    if (scout) {
        CHECK_EQ(scout->cycleSound, std::string("scout_cycle_01"));
        CHECK_EQ(scout->pullSound, std::string("scout_pull_01"));
        CHECK(scout->hasPullDelay);
        CHECK_NEAR(scout->pullDelay, 1.0f, 1e-5f);
        CHECK(scout->fireModeSingle);
    }
    const GameObjRenderDef* ak = provider->gameObject("ak47");
    CHECK(ak != nullptr);
    if (ak) {
        CHECK(!ak->fireModeSingle);
        CHECK(!ak->hasPullDelay);
        // player.ts dry fire needs the empty sound, ammoInfinite and the
        // reserve-ammo inventory key (GunDef.ammo).
        CHECK_EQ(ak->soundEmpty, std::string("empty_fire_02"));
        CHECK(!ak->ammoInfinite);
        CHECK_EQ(ak->ammoType, std::string("762mm"));
    }
    const GameObjRenderDef* cursed = provider->gameObject("m9_cursed");
    CHECK(cursed != nullptr);
    if (cursed) {
        CHECK(cursed->ammoInfinite);
        CHECK_EQ(cursed->ammoType, std::string("9mm_cursed"));
    }
}

TEST(shot_fire_sound_variants) {
    // Faction gun: the team sound replaces sound.shoot.
    {
        ShotFixture fx(1);
        fx.fire("bugle");
        CHECK_EQ(countPlayed(fx.backend, "bugle_01activePlayer"), 1);
        CHECK_EQ(countPlayed(fx.backend, "bugle_01otherPlayers"), 0);
    }
    // ... and shootAlt wins over the faction sound.
    {
        ShotFixture fx(2);
        fx.fire("bugle", false, true);
        CHECK_EQ(countPlayed(fx.backend, "bugle_01activePlayer"), 0);
        CHECK_EQ(countPlayed(fx.backend, "bugle_03activePlayer"), 1);
    }
    // Last shot replaces sound.shoot (garand_02).
    {
        ShotFixture fx(0);
        fx.fire("garand", true);
        CHECK_EQ(countPlayed(fx.backend, "garand_02activePlayer"), 1);
        CHECK_EQ(countPlayed(fx.backend, "garand_01activePlayer"), 0);
    }
    // Other players use the otherPlayers channel; the shot position drives the
    // distance attenuation (the sound still plays at this range).
    {
        pix::NullPixiFactory factory;
        GameWorld world(&factory, false);
        ShotAudioBackend backend;
        audio::AudioManager manager(&backend);
        audio::installGeneratedSoundDefs();
        manager.preloadSounds();
        world.setAudio(&manager, nullptr);
        world.loadMap(basicMap());
        Bullet bullet;
        bullet.playerId = 99;
        bullet.shotFx = true;
        bullet.shotSourceType = "ak47";
        bullet.pos = Vec2(10.0f, 10.0f);
        world.shotBarn().addShot(bullet);
        world.shotBarn().update(0.01f, world);
        CHECK_EQ(countPlayed(backend, "ak47_01otherPlayers"), 1);
    }
}

// shot.ts pullDelay: the shell is ejected after pullDelay * 0.45 (scout_elite
// pullDelay 1 -> 0.45 s) and the local single-fire gun plays cycle when a round
// is left, pull when the clip is empty.
TEST(shot_pull_delay_cycle_and_pull) {
    {
        ShotFixture fx(0);
        fx.world.setLocalAmmo(3);
        ShotObj* shot = fx.fire("scout_elite");
        CHECK_NEAR(shot->pullDelay, 0.45f, 1e-5f);
        CHECK_EQ(countPlayed(fx.backend, "scout_cycle_01activePlayer"), 1);
        CHECK_EQ(countPlayed(fx.backend, "scout_pull_01activePlayer"), 0);
        // No casing on the firing frame: it waits for the rechamber.
        CHECK(fx.world.particleBarn().particles.empty());

        // After the pull delay the casing is ejected with the player's state.
        fx.world.shotBarn().update(0.45f, fx.world);
        CHECK(!shot->active);
        CHECK(!fx.world.particleBarn().particles.empty());
    }
    {
        ShotFixture fx(0);
        fx.world.setLocalAmmo(0);
        fx.fire("scout_elite");
        CHECK_EQ(countPlayed(fx.backend, "scout_pull_01activePlayer"), 1);
        CHECK_EQ(countPlayed(fx.backend, "scout_cycle_01activePlayer"), 0);
    }
    // Automatic guns have no rechamber cue and eject on the firing frame.
    {
        ShotFixture fx(0);
        fx.world.setLocalAmmo(30);
        ShotObj* shot = fx.fire("ak47");
        CHECK_NEAR(shot->pullDelay, 0.0f, 1e-6f);
        CHECK(!fx.world.particleBarn().particles.empty());
        CHECK_EQ(fx.backend.played.size(), 1u);
    }
}

// ---------------------------------------------------------------------------
// Player: dry fire (player.ts m_update ~1218) is a local-only one-shot cue
// that fires when the clip and the reserve ammo are both empty.
// ---------------------------------------------------------------------------
namespace {
// A fully built local player (m_updateData creates its sprite tree) for the
// player.ts m_update cues that need the input/ammo snapshot.
Player* makeLocalPlayer(GameWorld& world, uint16_t id, const std::string& weapon) {
    Player* p = world.playerBarn().playerPool.m_alloc();
    p->__id = id;
    p->__type = ObjectType_Player;
    p->active = true;
    p->layer = 0;
    p->pos = Vec2(100.0f, 100.0f);
    p->dir = Vec2(1.0f, 0.0f);
    p->activeWeapon = weapon;
    ObjectData data;
    data.__id = id;
    data.__type = ObjectType_Player;
    data.pos = p->pos;
    data.dir = p->dir;
    data.layer = 0;
    data.scale = 1.0f;
    data.outfit = "outfitBase";
    data.activeWeapon = weapon;
    data.actionType = Action_None;
    p->m_updateData(data, true, true, world);
    world.setActivePlayer(p);
    world.setActivePlayerId(id);
    world.setLocalPlayerId(id);
    return p;
}
} // namespace

TEST(player_dry_fire_cue) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    ShotAudioBackend backend;
    audio::AudioManager manager{&backend};
    audio::installGeneratedSoundDefs();
    manager.preloadSounds();
    GameWorld world{&factory, false};
    world.setAudio(&manager, nullptr);
    world.loadMap(basicMap());
    Player* p = makeLocalPlayer(world, 7, "m9");

    auto frame = [&](bool held, bool pressed, bool prevent = false) {
        world.setLocalInput(held, pressed, prevent);
        p->update(0.016f, world);
    };
    const std::string cue = "empty_fire_01activePlayer";

    // Empty clip + empty reserve: one cue per press, even while held.
    world.setLocalAmmo(0, 0);
    frame(true, true);
    CHECK_EQ(countPlayed(backend, cue), 1);
    frame(true, false);
    CHECK_EQ(countPlayed(backend, cue), 1);
    // Releasing the trigger re-arms the cue.
    frame(false, false);
    frame(true, true);
    CHECK_EQ(countPlayed(backend, cue), 2);
    frame(false, false);

    // A round in the clip or in the reserve suppresses the cue.
    world.setLocalAmmo(1, 0);
    frame(true, true);
    world.setLocalAmmo(0, 30);
    frame(true, false);
    CHECK_EQ(countPlayed(backend, cue), 2);
    frame(false, false);

    // Auto guns also play on a held trigger; single-fire guns do not.
    p->activeWeapon = "mp5";
    world.setLocalAmmo(0, 0);
    frame(true, false);
    CHECK_EQ(countPlayed(backend, cue), 3);
    frame(true, false);
    CHECK_EQ(countPlayed(backend, cue), 3);
    p->activeWeapon = "m9";
    frame(false, false);
    frame(true, false);
    CHECK_EQ(countPlayed(backend, cue), 3);
    frame(false, false);

    // The emote/ping wheel blocker suppresses the cue.
    frame(true, true, true);
    CHECK_EQ(countPlayed(backend, cue), 3);
    frame(false, false);

    // ammoInfinite guns never dry-fire.
    p->activeWeapon = "m9_cursed";
    frame(true, true);
    CHECK_EQ(countPlayed(backend, cue), 3);
    frame(false, false);

    // Not a gun.
    p->activeWeapon = "fists";
    frame(true, true);
    CHECK_EQ(countPlayed(backend, cue), 3);
    frame(false, false);

    // Spectating another player never dry-fires (isSpectating guard); the
    // player still has to be the camera target for the branch to be reached.
    p->activeWeapon = "m9";
    world.setLocalPlayerId(9);
    world.setActivePlayerId(7);
    world.setLocalInput(true, true, false);
    p->update(0.016f, world);
    CHECK_EQ(countPlayed(backend, cue), 3);
    frame(false, false);
}

// ---------------------------------------------------------------------------
// Shot: a shotFx bullet drives the weapon recoil on the owner's hands.
// ---------------------------------------------------------------------------
TEST(shot_handles_recoil_and_dual) {
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    world.loadMap(basicMap());

    Player* p = world.playerBarn().playerPool.m_alloc();
    p->__id = 42;
    p->active = true;
    // shot.ts only ejects the shell while this weapon is still equipped.
    p->activeWeapon = "m9_dual";
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

    // P2: shot.ts createCasingParticle ejected a 9mm shell for this shot.
    bool casing = false;
    for (const Particle* particle : world.particleBarn().particles) {
        if (!particle || !particle->def) continue;
        for (const std::string& image : particle->def->images) {
            if (image.find("part-shell-01") != std::string::npos) {
                casing = true;
                break;
            }
        }
        if (casing) break;
    }
    CHECK(casing);

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

// A bullet for the JustSpawned paths (the barn copies the server's fields).
Bullet makeBullet(uint16_t playerId, float x, float y) {
    Bullet bullet;
    bullet.playerId = playerId;
    bullet.startPos = Vec2(x, y);
    bullet.pos = Vec2(x, y);
    bullet.dir = Vec2(1.0f, 0.0f);
    bullet.bulletType = "bullet_mosin";
    bullet.layer = 0;
    bullet.distance = 100.0f;
    return bullet;
}
} // namespace

// P1.3 follow-up: player.ts gunBarrel.anchor=(0.5,1) is the PIXI grip end.
// The Axmol adapter uses bottom-left anchors, so the equivalent native anchor
// is (0.5,0); a direct 1.0 put the grip at the muzzle (180-degree flip).
TEST(player_gun_anchor_matches_grip) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    world.loadMap(basicMap());

    Player* p = addGunPlayer(world, "mosin", 100.0f, 100.0f);
    ObjectData data;
    data.__id = 1;
    data.__type = ObjectType_Player;
    data.pos = Vec2(100.0f, 100.0f);
    data.dir = Vec2(1.0f, 0.0f);
    data.layer = 0;
    data.activeWeapon = "mosin";
    p->m_updateData(data, true, true, world);
    p->update(0.016f, world);

    CHECK(p->gunSprites[1] != nullptr);
    CHECK(p->gunContainers[1] != nullptr);
    if (p->gunSprites[1] && p->gunContainers[1]) {
        const auto* gun = static_cast<pix::NullSprite*>(p->gunSprites[1]);
        CHECK_NEAR(gun->ay, 0.0f, 1e-6f);
        CHECK_NEAR(static_cast<pix::NullContainer*>(p->gunContainers[1])->rotation,
                   3.14159265358979f / 2.0f, 1e-6f);
    }
}

// --- Match change (game.ts: a new MapMsg starts a new match) ---------------
// Loading a second map into the same GameWorld must drop everything the first
// match left behind: map.ts clear() empties the barns/creator, so a reused
// world cannot render (or collide with) the previous match's objects.
TEST(world_load_map_resets_objects) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    world.loadMap(basicMap());

    // First match: an obstacle, a live player and a bullet in flight.
    Obstacle* obstacle = world.map().obstaclePool.m_alloc();
    obstacle->__id = 11;
    obstacle->__type = ObjectType_Obstacle;
    obstacle->pos = Vec2(40.0f, 40.0f);
    obstacle->active = true;
    Player* player = addGunPlayer(world, "mosin", 100.0f, 100.0f);
    CHECK_EQ(world.map().obstaclePool.activeCount(), 1);
    CHECK(player->active);

    world.bulletBarn().spawn(&factory, makeBullet(1, 100.0f, 100.0f));
    CHECK_EQ(world.bulletBarn().bullets.size(), size_t(1));


    // A second map arrives (a re-queued match on the same world).
    world.loadMap(basicMap());

    // map.ts drops the previous match's pooled objects and the creator table.
    // The pooled instances themselves are recycled (the same Player allocation
    // comes back for the new match), so the *id table* is what must be empty;
    // the pointers below are only used to reach it.
    (void)player;
    (void)obstacle;
    CHECK_EQ(world.map().obstaclePool.activeCount(), 0);
    CHECK_EQ(world.playerBarn().playerPool.activeCount(), 0);
    // The bullet was deleted (the barn owns its visuals); other pools reset too.
    CHECK_EQ(world.bulletBarn().bullets.size(), size_t(0));
    CHECK_EQ(world.lootBarn().lootPool.activeCount(), 0);
    CHECK_EQ(world.decalBarn().decalPool.activeCount(), 0);
    // The active player does not carry over into the new match.
    CHECK(world.activePlayer() == nullptr);
}

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
    CHECK(line.dots[17]->isVisible());
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
    // Unscoped (zoom 0): no camera clamp, the full range (mosin 3.95 + 178 = 182)
    // is visible -> ceil((182 - 3.5) / 1.5) = 119 dots.
    line.update(true, *p, world.map(), world.camera(), world.renderer(), &factory, true);
    CHECK(line.dots.size() > 100u);

    // 1x scope (desktop 28 -> mobile 32): cameraRad = sqrt(32 * 1.414 * 32) = 38.05,
    // still below the mosin's 3.95 + 178 = 181.95 range.
    p->zoom = 28.0f;
    CHECK_NEAR(p->getZoom(true), 32.0f, 1e-4f);
    CHECK_NEAR(p->getZoom(false), 28.0f, 1e-4f);
    line.update(true, *p, world.map(), world.camera(), world.renderer(), &factory, true);
    const float rad = std::sqrt(32.0f * 1.414f * 32.0f);
    // Count visible dots (dots.size() grows monotonically, so check visibility).
    int visible = 0;
    for (auto* dot : line.dots) {
        if (dot->isVisible()) visible++;
    }
    CHECK_EQ(visible, static_cast<int>(std::ceil((rad - 3.5f) / 1.5f)));
    CHECK(visible > 18u);
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
