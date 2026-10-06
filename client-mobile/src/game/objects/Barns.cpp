#include "Barns.h"

#include "../GameWorld.h"
#include "../Map.h"
#include "../../render/Renderer.h"

#include <cmath>
#include <cstdlib>

namespace surv {

static float rnd(float a, float b) {
    return a + (b - a) * (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX));
}

static const MapObjectDef* mapDefFor(const std::string& type) {
    const DefProvider* p = getDefProvider();
    return p ? p->mapObject(type) : nullptr;
}

static const GameObjRenderDef* gameDefFor(const std::string& type) {
    const DefProvider* p = getDefProvider();
    return p ? p->gameObject(type) : nullptr;
}

static pix::Sprite* ensureSprite(Ctx& ctx, pix::Container*& container, pix::Sprite*& sprite,
                                 float anchor = 0.5f) {
    if (!container) {
        container = ctx.factory()->createContainer();
    }
    if (!sprite) {
        sprite = ctx.factory()->createSprite();
        sprite->setAnchor(anchor, anchor);
        container->addChild(sprite);
    }
    return sprite;
}

// ---------------------------------------------------------------------------
// Obstacle
// ---------------------------------------------------------------------------
void Obstacle::m_init() {
    healthT = 1.0f;
    doorOpen = false;
    doorSeq = 0;
    buttonOnOff = false;
    buttonSeq = 0;
}

void Obstacle::m_free() {
    if (container) {
        container->setVisible(false);
    }
}

void Obstacle::m_updateData(const ObjectData& data, bool fullUpdate, bool isNew, Ctx& ctx) {
    (void)isNew;
    pos = data.pos;
    ori = data.ori;
    scale = data.scale;
    if (!fullUpdate) {
        return;
    }
    type = data.type;
    layer = data.layer;
    healthT = data.healthT;
    isDoor = data.isDoor;
    doorOpen = data.doorOpen;
    doorCanUse = data.doorCanUse;
    doorLocked = data.doorLocked;
    doorSeq = data.doorSeq;
    isButton = data.isButton;
    buttonOnOff = data.buttonOnOff;
    buttonCanUse = data.buttonCanUse;
    buttonSeq = data.buttonSeq;
    isPuzzlePiece = data.isPuzzlePiece;
    parentBuildingId = data.parentBuildingId;
    isSkin = data.isSkin;
    skinPlayerId = data.skinPlayerId;

    ensureSprite(ctx, container, sprite);
    const MapObjectDef* def = mapDefFor(type);
    if (def && !def->img.sprite.empty() && frame != def->img.sprite) {
        frame = def->img.sprite;
        sprite->setFrame(frame);
        sprite->setTint(def->img.tint);
        sprite->setAlpha(def->img.alpha);
    }
}

void Obstacle::update(float dt, Ctx& ctx) {
    (void)dt;
    if (!container || !sprite) {
        return;
    }
    const MapObjectDef* def = mapDefFor(type);
    const int zOrd = def ? def->img.zIdx : 0;
    ctx.renderer().addPIXIObj(container, layer, zOrd, __id);

    const Vec2 screenPos = ctx.camera().m_pointToScreen(pos);
    const float imgScale = def ? def->img.scale : 1.0f;
    const float s = ctx.camera().m_pixels(scale * imgScale);
    container->setPosition(screenPos.x, screenPos.y);
    container->setScale(s, s);
    container->setVisible(true);
}

// ---------------------------------------------------------------------------
// Building
// ---------------------------------------------------------------------------
void Building::m_init() {
    ceilingVisionTicker = 0.0f;
    ceilingFadeAlpha = 0.0f;
}

void Building::m_free() {}

void Building::m_updateData(const ObjectData& data, bool fullUpdate, bool isNew, Ctx& ctx) {
    (void)isNew;
    if (!fullUpdate) {
        pos = data.pos;
        return;
    }
    type = data.type;
    pos = data.pos;
    ori = data.ori;
    scale = data.scale;
    layer = data.layer;
    ceilingDead = data.ceilingDead;
    occupied = data.occupied;
    ceilingDamaged = data.ceilingDamaged;
    hasPuzzle = data.hasPuzzle;
    puzzleSolved = data.puzzleSolved;
    puzzleErrSeq = data.puzzleErrSeq;

    const MapObjectDef* def = mapDefFor(type);
    if (def) {
        zIdx = def->img.zIdx;
        aabb = colliderTransform(def->hasBounding ? def->boundingCollider
                                                   : Collider::createAabb(Vec2(), Vec2()),
                                 pos, math::oriToRad(ori), scale);
        hasAabb = def->hasBounding;
        ceilingRegions.clear();
        for (const auto& shape : def->mapShapes) {
            ceilingRegions.push_back(shape.collider);
        }
    }
    ctx.renderer().layerMaskDirty = true;
}

void Building::update(float dt, Ctx& ctx) {
    (void)ctx;
    if (ceilingVisionTicker > 0.0f) {
        ceilingVisionTicker = math::max(0.0f, ceilingVisionTicker - dt);
    }
}

bool Building::isInsideCeiling(const Collider& c) const {
    for (const auto& region : ceilingRegions) {
        if (colliderIntersect(region, c)) {
            return true;
        }
    }
    return false;
}

float Building::getDistanceToBuilding(const Vec2& p, float maxDist) const {
    if (!hasAabb) {
        return maxDist;
    }
    const Vec2 closest = clampPosToAabb(p, aabb.min, aabb.max);
    return math::min(v2Length(v2Sub(p, closest)), maxDist);
}

// ---------------------------------------------------------------------------
// Loot
// ---------------------------------------------------------------------------
void Loot::m_init() {
    count = 0;
    hasOwner = false;
    ownerId = 0;
}

void Loot::m_free() {
    if (container) {
        container->setVisible(false);
    }
}

void Loot::m_updateData(const ObjectData& data, bool fullUpdate, bool isNew, Ctx& ctx) {
    (void)isNew;
    pos = data.pos;
    if (!fullUpdate) {
        return;
    }
    type = data.type;
    layer = data.layer;
    isOld = data.isOld;
    isPreloadedGun = data.isPreloadedGun;
    count = data.count;
    hasOwner = data.hasOwner;
    ownerId = data.ownerId;

    ensureSprite(ctx, container, sprite);
    const GameObjRenderDef* def = gameDefFor(type);
    if (def && def->hasImg && !def->img.sprite.empty()) {
        sprite->setFrame(def->img.sprite);
        sprite->setTint(def->img.tint);
    }
}

void Loot::update(float dt, Ctx& ctx) {
    (void)dt;
    if (!container) {
        return;
    }
    ctx.renderer().addPIXIObj(container, layer, 10, __id);
    const Vec2 screenPos = ctx.camera().m_pointToScreen(pos);
    const float s = ctx.camera().m_pixels(1.0f);
    container->setPosition(screenPos.x, screenPos.y);
    container->setScale(s, s);
    container->setVisible(true);
}

void LootBarn::update(float dt, GameWorld& ctx) {
    for (auto* loot : lootPool.m_getPool()) {
        if (loot->active) {
            loot->update(dt, ctx);
        }
    }
}

// ---------------------------------------------------------------------------
// DeadBody
// ---------------------------------------------------------------------------
void DeadBody::m_init() {
    nameTextSet = false;
}

void DeadBody::m_free() {
    if (container) {
        container->setVisible(false);
    }
}

void DeadBody::m_updateData(const ObjectData& data, bool fullUpdate, bool isNew, Ctx& ctx) {
    pos = data.pos;
    if (fullUpdate) {
        layer = data.layer;
        playerId = data.playerId;
    }
    if (!container) {
        container = ctx.factory()->createContainer();
        sprite = ctx.factory()->createSprite("skull.img");
        sprite->setAnchor(0.5f, 0.5f);
        sprite->setScale(0.4f, 0.4f);
        sprite->setTint(5921370u);
        container->addChild(sprite);
        nameText = ctx.factory()->createText();
        nameText->setAnchor(0.5f, -1.0f);
        nameText->setScale(0.5f, 0.5f);
        nameText->setColor(0x808080, 0x000000, 0.0f, true);
        container->addChild(nameText);
    }
    if (isNew) {
        nameTextSet = false;
        container->setVisible(true);
    }
}

void DeadBodyBarn::update(float dt, GameWorld& ctx) {
    (void)dt;
    for (auto* d : deadBodyPool.m_getPool()) {
        if (!d->active) {
            continue;
        }
        if (!d->nameTextSet && d->nameText) {
            d->nameText->setText(ctx.playerBarn().getPlayerName(d->playerId, ctx.activePlayerId(), false));
            d->nameTextSet = true;
        }
        const Collider col = Collider::createCircle(d->pos, 1.0f);
        const bool onStairs = ctx.map().insideStructureStairs(col);

        int layer = d->layer;
        int zOrd = 12;
        if (d->layer == 0 && ctx.activePlayer() && ctx.activePlayer()->layer == 0 && onStairs) {
            layer |= 2;
            zOrd += 100;
        }
        ctx.renderer().addPIXIObj(d->container, layer, zOrd, d->__id);

        const Vec2 screenPos = ctx.camera().m_pointToScreen(d->pos);
        const float screenScale = ctx.camera().m_pixels(1.0f);
        d->container->setPosition(screenPos.x, screenPos.y);
        d->container->setScale(screenScale, screenScale);
        d->container->setVisible(true);
    }
}

DeadBody* DeadBodyBarn::getDeadBodyById(uint16_t playerId) {
    for (auto* d : deadBodyPool.m_getPool()) {
        if (d->active && d->playerId == playerId) {
            return d;
        }
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// Projectile
// ---------------------------------------------------------------------------
void Projectile::m_init() {
    posZ = 0.0f;
}

void Projectile::m_free() {
    if (container) {
        container->setVisible(false);
    }
}

void Projectile::m_updateData(const ObjectData& data, bool fullUpdate, bool isNew, Ctx& ctx) {
    (void)isNew;
    pos = data.pos;
    if (!fullUpdate) {
        return;
    }
    type = data.type;
    layer = data.layer;
    ori = data.ori;
    posZ = data.posZ;

    ensureSprite(ctx, container, sprite);
    const GameObjRenderDef* def = gameDefFor(type);
    if (def && def->hasImg && !def->img.sprite.empty()) {
        sprite->setFrame(def->img.sprite);
        sprite->setTint(def->img.tint);
    }
}

void ProjectileBarn::update(float dt, GameWorld& ctx) {
    (void)dt;
    for (auto* p : projectilePool.m_getPool()) {
        if (!p->active || !p->container) {
            continue;
        }
        ctx.renderer().addPIXIObj(p->container, p->layer, 200, p->__id);
        Vec2 screenPos = ctx.camera().m_pointToScreen(p->pos);
        screenPos.y -= ctx.camera().m_pixels(p->posZ);
        const float s = ctx.camera().m_pixels(1.0f);
        p->container->setPosition(screenPos.x, screenPos.y);
        p->container->setScale(s, s);
        p->container->setVisible(true);
    }
}

// ---------------------------------------------------------------------------
// Smoke
// ---------------------------------------------------------------------------
void SmokeParticle::m_init(pix::Factory* factory, const Vec2& p, float r, int l, int interior_) {
    pos = p;
    posTarget = p;
    rad = r;
    radTarget = r;
    rot = rnd(0.0f, 3.14159265358979f * 2.0f);
    rotVel = 3.14159265358979f * rnd(0.25f, 0.5f) * (std::rand() % 2 == 0 ? -1.0f : 1.0f);
    fade = false;
    fadeTicker = 0.0f;
    fadeDuration = rnd(0.5f, 0.75f);
    tint = 0xf2f2f2;
    layer = l;
    interior = interior_;
    if (!sprite) {
        sprite = factory->createSprite();
        sprite->setAnchor(0.5f, 0.5f);
    }
    const char* frames[2] = {"part-smoke-02.img", "part-smoke-03.img"};
    sprite->setFrame(frames[std::rand() % 2]);
    sprite->setVisible(true);
}

void Smoke::m_init() {
    particle = nullptr;
}

void Smoke::m_free() {
    if (particle) {
        particle->fadeOut();
        particle = nullptr;
    }
}

void Smoke::m_updateData(const ObjectData& data, bool fullUpdate, bool isNew, Ctx& ctx) {
    pos = data.pos;
    rad = data.rad;
    if (fullUpdate) {
        layer = data.layer;
        interior = data.interior;
    }
    if (isNew) {
        particle = ctx.smokeBarn().m_allocParticle(ctx.factory());
        if (particle) {
            particle->m_init(ctx.factory(), pos, rad, layer, interior);
        }
    }
    if (particle) {
        particle->posTarget = pos;
        particle->radTarget = rad;
    }
}

SmokeParticle* SmokeBarn::m_allocParticle(pix::Factory* factory) {
    SmokeParticle* particle = nullptr;
    for (auto* p : m_particles) {
        if (!p->active) {
            particle = p;
            break;
        }
    }
    if (!particle) {
        particle = new SmokeParticle();
        m_particles.push_back(particle);
    }
    (void)factory;
    particle->active = true;
    particle->zIdx = zIdx--;
    return particle;
}

void SmokeBarn::update(float dt, GameWorld& ctx) {
    for (auto* p : m_particles) {
        if (!p->active || !p->sprite) {
            continue;
        }
        p->rad = math::lerp(dt * 3.0f, p->rad, p->radTarget);
        p->pos = v2Lerp(dt * 3.0f, p->pos, p->posTarget);
        p->rotVel *= 1.0f / (1.0f + dt * 0.1f);
        p->rot += p->rotVel * dt;
        p->fadeTicker += p->fade ? dt : 0.0f;
        p->active = p->fadeTicker < p->fadeDuration;

        const float alpha = math::clamp(1.0f - p->fadeTicker / p->fadeDuration, 0.0f, 1.0f) * 0.9f;

        int layer = p->layer;
        const int activeLayer = ctx.activePlayer() ? ctx.activePlayer()->layer : 0;
        const bool sameLayer = (p->layer == activeLayer);
        const bool onStairs = (activeLayer & 2) != 0;
        if ((sameLayer || onStairs) &&
            (p->layer == 1 || !onStairs ||
             !ctx.map().insideStructureMask(Collider::createCircle(p->pos, 1.0f)))) {
            layer |= 2;
        }
        const int zOrd = p->interior ? 500 : 1000;
        ctx.renderer().addPIXIObj(p->sprite, layer, zOrd, p->zIdx);

        const Vec2 screenPos = ctx.camera().m_pointToScreen(p->pos);
        const float screenScale = ctx.camera().m_pixels((p->rad * 2.0f) / ctx.camera().m_ppu);
        p->sprite->setPosition(screenPos.x, screenPos.y);
        p->sprite->setScale(screenScale, screenScale);
        p->sprite->setRotation(p->rot);
        p->sprite->setTint(p->tint);
        p->sprite->setAlpha(alpha);
        p->sprite->setVisible(p->active);
    }
}

// ---------------------------------------------------------------------------
// Bullet (visual)
// ---------------------------------------------------------------------------
void BulletBarn::spawn(pix::Factory* factory, const Bullet& data) {
    BulletVisual* b = nullptr;
    for (auto* v : bullets) {
        if (!v->active) {
            b = v;
            break;
        }
    }
    if (!b) {
        b = new BulletVisual();
        bullets.push_back(b);
    }
    b->active = true;
    b->startPos = data.startPos;
    b->pos = data.pos;
    b->dir = data.dir;
    b->bulletType = data.bulletType;
    b->layer = data.layer;
    b->distance = data.distance;
    b->clipDistance = data.clipDistance;
    b->speedMult = data.speedMult;
    b->distanceMult = data.distanceMult;
    b->t = 0.0f;
    if (!b->gfx) {
        b->gfx = factory->createGraphics();
    }
    b->gfx->clear();
    b->gfx->setVisible(true);
}

void BulletBarn::update(float dt, GameWorld& ctx) {
    for (size_t i = 0; i < bullets.size(); i++) {
        BulletVisual* b = bullets[i];
        if (!b->active) {
            continue;
        }
        b->t += dt;
        const float length = b->clipDistance && b->distance > 0.0f ? b->distance : 30.0f;
        const Vec2 end = v2Add(b->startPos, v2Mul(b->dir, length));
        b->gfx->clear();
        b->gfx->lineStyle(2.0f, 0xffe08a, math::clamp(1.0f - b->t * 4.0f, 0.0f, 1.0f));
        b->gfx->moveTo(b->startPos.x, b->startPos.y);
        b->gfx->lineTo(end.x, end.y);
        ctx.renderer().addPIXIObj(b->gfx, b->layer, 900, b->layer * 100000 + static_cast<int>(i));
        const Vec2 p0 = ctx.camera().m_pointToScreen(Vec2(0.0f, 0.0f));
        const float s = ctx.camera().m_scaleToScreen(1.0f);
        b->gfx->setPosition(p0.x, p0.y);
        b->gfx->setScale(s, -s);
        if (b->t > 0.25f) {
            b->active = false;
            b->gfx->setVisible(false);
        }
    }
}

// ---------------------------------------------------------------------------
// Explosion (visual)
// ---------------------------------------------------------------------------
void ExplosionObj::m_init() {
    t = 0.0f;
}

void ExplosionObj::m_free() {
    if (gfx) {
        gfx->setVisible(false);
    }
}

void ExplosionObj::m_updateData(const ObjectData& data, bool fullUpdate, bool isNew, Ctx& ctx) {
    (void)fullUpdate;
    (void)isNew;
    (void)ctx;
    pos = data.pos;
    type = data.type;
    layer = data.layer;
}

void ExplosionBarn::update(float dt, GameWorld& ctx) {
    for (auto* e : explosionPool.m_getPool()) {
        if (!e->active) {
            continue;
        }
        if (!e->container) {
            e->container = ctx.factory()->createContainer();
            e->gfx = ctx.factory()->createGraphics();
            e->container->addChild(e->gfx);
        }
        e->t += dt;
        const float dur = 0.5f;
        const float k = math::clamp(e->t / dur, 0.0f, 1.0f);
        const float radius = 24.0f * k;
        e->gfx->clear();
        e->gfx->beginFill(0xffaa33, 1.0f - k);
        e->gfx->drawCircle(0.0f, 0.0f, radius);
        e->gfx->endFill();

        ctx.renderer().addPIXIObj(e->container, e->layer, 1000, e->__id);
        const Vec2 screenPos = ctx.camera().m_pointToScreen(e->pos);
        const float s = ctx.camera().m_pixels(1.0f);
        e->container->setPosition(screenPos.x, screenPos.y);
        e->container->setScale(s, s);
        e->container->setVisible(true);
        if (e->t >= dur) {
            e->active = false;
            e->container->setVisible(false);
        }
    }
}

// ---------------------------------------------------------------------------
// Player (render subset)
// ---------------------------------------------------------------------------
void Player::m_init() {
    dead = false;
    downed = false;
}

void Player::m_free() {
    if (container) {
        container->setVisible(false);
    }
}

void Player::m_updateData(const ObjectData& data, bool fullUpdate, bool isNew, Ctx& ctx) {
    (void)isNew;
    pos = data.pos;
    dir = data.dir;
    if (!fullUpdate) {
        return;
    }
    layer = data.layer;
    outfit = data.outfit;
    dead = data.dead;
    downed = data.downed;
    animType = data.animType;

    ensureSprite(ctx, container, bodySprite);
    // Outfit body sprite (player.ts bodySprite). The full skeletal pose/anim
    // port is still pending; this renders the outfit at the web client's 0.25
    // body scale so players are visible and rotate to face their aim.
    const GameObjRenderDef* outfitDef = gameDefFor(outfit);
    if (outfitDef && outfitDef->hasImg && !outfitDef->img.sprite.empty()) {
        bodySprite->setFrame(outfitDef->img.sprite);
        bodySprite->setTint(outfitDef->img.tint);
        bodySprite->setAlpha(outfitDef->img.alpha);
        bodySprite->setScale(0.25f, 0.25f);
    }
    if (!nameText) {
        nameText = ctx.factory()->createText();
        nameText->setAnchor(0.5f, -1.0f);
        nameText->setScale(0.5f, 0.5f);
        nameText->setColor(0xffffff, 0x000000, 1.0f, true);
        container->addChild(nameText);
    }
}

void PlayerBarn::update(float dt, GameWorld& ctx) {
    (void)dt;
    for (auto* p : playerPool.m_getPool()) {
        if (!p->active || !p->container) {
            continue;
        }
        ctx.renderer().addPIXIObj(p->container, p->layer, 10, p->__id);
        const Vec2 screenPos = ctx.camera().m_pointToScreen(p->pos);
        const float s = ctx.camera().m_pixels(1.0f);
        p->container->setPosition(screenPos.x, screenPos.y);
        p->container->setScale(s, s);
        // Rotate the body to face the aim, leaving the name label upright.
        if (p->bodySprite) {
            p->bodySprite->setRotation(std::atan2(p->dir.y, p->dir.x) - 3.14159265358979f * 0.5f);
        }
        p->container->setVisible(!p->dead);
        if (p->nameText) {
            auto it = names.find(p->__id);
            p->nameText->setText(it == names.end() ? std::string() : it->second);
        }
    }
}

std::string PlayerBarn::getPlayerName(uint16_t playerId, uint16_t activePlayerId, bool anon) {
    if (anon && playerId != activePlayerId) {
        return "Player";
    }
    auto it = names.find(playerId);
    return it == names.end() ? std::string() : it->second;
}

Player* PlayerBarn::getPlayerById(uint16_t playerId) {
    for (auto* p : playerPool.m_getPool()) {
        if (p->active && p->__id == playerId) {
            return p;
        }
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// ParticleBarn (emitter subset)
// ---------------------------------------------------------------------------
Emitter* ParticleBarn::addEmitter(pix::Factory* factory, const std::string& type, const Vec2& pos,
                                  const Vec2& dir, int layer) {
    (void)factory;
    auto* e = new Emitter();
    e->type = type;
    e->pos = pos;
    e->dir = dir;
    e->layer = layer;
    e->enabled = false;
    e->radius = 0.0f;
    e->rateMult = 1.0f;
    e->alpha = 1.0f;
    emitters.push_back(e);
    return e;
}

void ParticleBarn::update(float dt, GameWorld& ctx) {
    (void)dt;
    (void)ctx;
    // Minimal emitter handling: emitters exist and are driven by the map, but
    // particle spawning is not yet ported (see plan.md M4 status).
}

} // namespace surv
