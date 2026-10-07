// Ports of the more involved client object barns (client/src/objects/*.ts):
// obstacle, building, loot, deadBody, projectile, smoke, bullet and explosion,
// plus the particles.ts runtime. Player skeletal rendering lives in
// PlayerRender.cpp. Draw calls go through the PixiLike adapter so the
// geometry/order mirrors the web client.
#pragma once
#include "GameObject.h"
#include "../../core/Collider.h"
#include "../../core/Vec2.h"
#include "../../render/Camera.h"
#include "../../render/Defs.h"
#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace pix {
class Node;
class Container;
class Sprite;
class Graphics;
class Text;
class Factory;
} // namespace pix

namespace surv {

class GameWorld;
class Gas;

struct Bullet; // shared/net Messages.h
class Emitter; // particles.ts

// --- obstacle.ts -----------------------------------------------------------
class Obstacle : public AbstractObject {
public:
    std::string type;
    Vec2 pos;
    int ori = 0;
    float scale = 1.0f;
    int layer = 0;
    float healthT = 1.0f;
    bool isDoor = false;
    bool doorOpen = false;
    bool doorCanUse = false;
    bool doorLocked = false;
    int doorSeq = 0;
    bool isButton = false;
    bool buttonOnOff = false;
    bool buttonCanUse = false;
    int buttonSeq = 0;
    bool isPuzzlePiece = false;
    uint16_t parentBuildingId = 0;
    bool isSkin = false;
    uint16_t skinPlayerId = 0;

    // obstacle.ts runtime visual state.
    bool isNew = false;
    bool dead = false;
    bool exploded = false;
    float rot = 0.0f;
    float imgRot = 0.0f;
    float imgScale = 1.0f;
    bool imgMirrorX = false;
    bool imgMirrorY = false;
    int zOrd = 0;
    bool doorHasInterp = false;
    Vec2 doorInterpPos;
    float doorInterpRot = 0.0f;

    pix::Sprite* sprite = nullptr;
    pix::Sprite* casingSprite = nullptr;
    Emitter* smokeEmitter = nullptr;
    std::string frame;

    void m_init() override;
    void m_free() override;
    void m_updateData(const ObjectData& data, bool fullUpdate, bool isNew, Ctx& ctx) override;
    void update(float dt, Ctx& ctx);
    void render(Ctx& ctx, int activeLayer);
    void applyFrame(bool hasImage);

private:
    bool _firstUpdate = true;
    bool _anchorInit = false;
};

// --- building.ts -----------------------------------------------------------
// Full floor/ceiling rendering port: per-image sprites created on isNew,
// ceiling vision fade, removeOnDamaged, and the ceiling.destroy residue.
class Building : public AbstractObject {
public:
    // One building.ts `imgs[]` entry (floor or ceiling image).
    struct Img {
        pix::Sprite* sprite = nullptr;
        bool isCeiling = false;
        bool removeOnDamaged = false;
        int zOrd = 0;
        int zIdx = 0;
        Vec2 posOffset;
        float rotOffset = 0.0f;
        float imgAlpha = 1.0f;
        float defScale = 1.0f;
        bool mirrorX = false;
        bool mirrorY = false;
    };

    std::string type;
    Vec2 pos;
    int ori = 0;
    float rot = 0.0f;
    float scale = 1.0f;
    int layer = 0;
    int zIdx = 0;
    bool isNew = false;
    bool ceilingDead = false;
    bool occupied = false;
    bool ceilingDamaged = false;
    bool hasPuzzle = false;
    bool puzzleSolved = false;
    int puzzleErrSeq = 0;
    bool playedCeilingDeadFx = false;
    bool playedSolvedPuzzleFx = false;
    bool puzzleErrSeqModified = false;

    std::vector<Img> imgs;
    pix::Sprite* residue = nullptr;
    bool residueCreated = false;

    struct Surface {
        std::string type;
        std::vector<Collider> colliders;
    };
    std::vector<Surface> surfaces;
    Collider aabb;
    bool hasAabb = false;
    std::vector<Collider> ceilingRegions; // transformed ceiling zoomIn regions
    CeilingVisionDef vision;
    float ceilingVisionTicker = 0.0f;
    float ceilingFadeAlpha = 1.0f;
    std::vector<Emitter*> particleEmitters;

    void m_init() override;
    void m_free() override;
    void m_updateData(const ObjectData& data, bool fullUpdate, bool isNew, Ctx& ctx) override;
    void update(float dt, Ctx& ctx);
    bool isInsideCeiling(const Collider& c) const;
    float getDistanceToBuilding(const Vec2& p, float maxDist) const;
};

// --- loot.ts ---------------------------------------------------------------
class Loot : public AbstractObject {
public:
    std::string type;
    Vec2 pos;
    int layer = 0;
    bool isOld = false;
    bool isPreloadedGun = false;
    uint8_t count = 0;
    bool hasOwner = false;
    uint16_t ownerId = 0;
    float rad = 1.0f;
    float imgScale = 1.0f;
    float ticker = 0.0f;
    Vec2 visualPosOld;
    float posInterpTicker = 0.0f;
    bool updatedData = false;

    pix::Container* container = nullptr;
    pix::Sprite* sprite = nullptr;
    Emitter* emitter = nullptr;

    void m_init() override;
    void m_free() override;
    void m_updateData(const ObjectData& data, bool fullUpdate, bool isNew, Ctx& ctx) override;
    void update(float dt, Ctx& ctx);
};

class LootBarn {
public:
    Pool<Loot> lootPool;
    void update(float dt, GameWorld& ctx);
};

// --- deadBody.ts -----------------------------------------------------------
class DeadBody : public AbstractObject {
public:
    Vec2 pos;
    int layer = 0;
    uint16_t playerId = 0;
    bool nameTextSet = false;
    pix::Container* container = nullptr;
    pix::Sprite* sprite = nullptr;
    pix::Text* nameText = nullptr;

    void m_init() override;
    void m_free() override;
    void m_updateData(const ObjectData& data, bool fullUpdate, bool isNew, Ctx& ctx) override;
};

class DeadBodyBarn {
public:
    Pool<DeadBody> deadBodyPool;
    void update(float dt, GameWorld& ctx);
    DeadBody* getDeadBodyById(uint16_t playerId);
};

// --- projectile.ts ---------------------------------------------------------
class Projectile : public AbstractObject {
public:
    std::string type;
    Vec2 pos;
    float posZ = 0.0f;
    int layer = 0;
    int ori = 0;
    pix::Container* container = nullptr;
    pix::Sprite* sprite = nullptr;

    void m_init() override;
    void m_free() override;
    void m_updateData(const ObjectData& data, bool fullUpdate, bool isNew, Ctx& ctx) override;
    void update(float dt, Ctx& ctx);
};

class ProjectileBarn {
public:
    Pool<Projectile> projectilePool;
    void update(float dt, GameWorld& ctx);
};

// --- smoke.ts --------------------------------------------------------------
class SmokeParticle {
public:
    bool active = false;
    int zIdx = 0;
    pix::Sprite* sprite = nullptr;
    Vec2 pos, posTarget;
    float rad = 0.0f, radTarget = 0.0f;
    float rot = 0.0f, rotVel = 0.0f;
    bool fade = false;
    float fadeTicker = 0.0f, fadeDuration = 0.0f;
    uint32_t tint = 0xffffff;
    int layer = 0, interior = 0;

    void m_init(pix::Factory* factory, const Vec2& pos, float rad, int layer, int interior);
    void fadeOut() { fade = true; }
};

class Smoke : public AbstractObject {
public:
    Vec2 pos;
    float rad = 0.0f;
    int layer = 0, interior = 0;
    SmokeParticle* particle = nullptr;

    void m_init() override;
    void m_free() override;
    void m_updateData(const ObjectData& data, bool fullUpdate, bool isNew, Ctx& ctx) override;
};

class SmokeBarn {
public:
    Pool<Smoke> m_smokePool;
    std::vector<SmokeParticle*> m_particles;
    int zIdx = INT32_MAX;

    SmokeParticle* m_allocParticle(pix::Factory* factory);
    void update(float dt, GameWorld& ctx);
};

// --- bullet.ts (visual only) ----------------------------------------------
struct BulletVisual {
    bool active = false;
    Vec2 startPos;
    Vec2 pos;
    Vec2 dir;
    std::string bulletType;
    int layer = 0;
    float distance = 0.0f;
    bool clipDistance = false;
    float speedMult = 1.0f;
    float distanceMult = 1.0f;
    float t = 0.0f;
    pix::Graphics* gfx = nullptr;
};

class BulletBarn {
public:
    std::vector<BulletVisual*> bullets;
    void spawn(pix::Factory* factory, const Bullet& data);
    void update(float dt, GameWorld& ctx);
};

// --- explosion.ts (visual only) -------------------------------------------
class ExplosionObj : public AbstractObject {
public:
    Vec2 pos;
    std::string type;
    int layer = 0;
    float t = 0.0f;
    pix::Container* container = nullptr;
    pix::Graphics* gfx = nullptr;

    void m_init() override;
    void m_free() override;
    void m_updateData(const ObjectData& data, bool fullUpdate, bool isNew, Ctx& ctx) override;
    void update(float dt, Ctx& ctx);
};

class ExplosionBarn {
public:
    Pool<ExplosionObj> explosionPool;
    void update(float dt, GameWorld& ctx);
};

// --- particles.ts ----------------------------------------------------------
// Port of the Particle/Emitter runtime. Defs come from the DefProvider
// (generated from client/src/objects/particles.ts by codegen_render_defs.mjs).
class Particle {
public:
    bool active = false;
    float ticker = 0.0f;
    const ParticleDef* def = nullptr;
    pix::Sprite* sprite = nullptr;
    bool hasParent = false;

    Vec2 pos, vel;
    float rot = 0.0f;
    float delay = 0.0f;
    float life = 1.0f;
    float drag = 0.0f;
    float rotVel = 0.0f;
    float rotDrag = 0.0f;

    bool scaleUseExp = false;
    float scale = 1.0f;
    float scaleEnd = 0.0f;
    float scaleExp = 0.0f;

    bool alphaUseExp = false;
    float alpha = 1.0f;
    float alphaEnd = 0.0f;
    float alphaExp = 0.0f;

    bool alphaIn = false;
    float alphaInStart = 0.0f;
    float alphaInEnd = 0.0f;

    int emitterIdx = -1;
    float valueAdjust = 1.0f;
    int layer = 0;
    int zOrd = 20;

    void m_init(pix::Factory* factory, const std::string& type, int layer_, const Vec2& pos_,
                const Vec2& vel_, float scaleParam, float rot_, pix::Node* parent, int zOrd_,
                float valueAdjust_);
    void m_free();
    void setColor(uint32_t color);
};

// Options mirroring particles.ts EmitterOptions.
struct EmitterOptions {
    Vec2 pos;
    Vec2 dir{0.0f, 1.0f};
    float scale = 1.0f;
    int layer = 0;
    float duration = 3.402823466e+38f;
    float radius = -1.0f; // <0 => EmitterDef.radius
    float rateMult = 1.0f;
    pix::Node* parent = nullptr;
    bool hasColor = false;
    uint32_t color = 0xffffff;
};

class Emitter {
public:
    bool active = false;
    bool enabled = true;
    std::string type;
    const EmitterDef* def = nullptr;
    Vec2 pos, dir;
    float scale = 1.0f;
    int layer = 0;
    float duration = 3.402823466e+38f;
    float radius = 0.0f;
    float ticker = 0.0f;
    float nextSpawn = 0.0f;
    float spawnCount = 0.0f;
    pix::Node* parent = nullptr;
    float alpha = 1.0f;
    float rateMult = 1.0f;
    int zOrd = 20;
    bool hasColor = false;
    uint32_t color = 0xffffff;

    void m_init(const std::string& type, const EmitterOptions& opts);
    void m_free();
    void stop() { duration = ticker; }
};

class ParticleBarn {
public:
    std::vector<Particle*> particles;
    std::vector<Emitter*> emitters;
    float valueAdjust = 1.0f;

    Particle* addParticle(pix::Factory* factory, const std::string& type, int layer, const Vec2& pos,
                          const Vec2& vel, float scale = 1.0f, float rot = -1.0f,
                          pix::Node* parent = nullptr, int zOrd = -1);
    Particle* addRippleParticle(pix::Factory* factory, const Vec2& pos, int layer, uint32_t color);
    Emitter* addEmitter(pix::Factory* factory, const std::string& type, const EmitterOptions& opts);
    void update(float dt, GameWorld& ctx);
};

// --- player.ts skeletal rendering -----------------------------------------
class Player : public AbstractObject {
public:
    Vec2 pos;
    Vec2 dir;
    int layer = 0;
    std::string name;
    std::string outfit;
    std::string backpack, helmet, chest, activeWeapon, role;
    float scale = 1.0f;
    bool wearingPan = false;
    bool healEffect = false;
    int hasteType = HasteType_None;
    int hasteSeq = -1;
    std::array<float, 2> gunRecoil{};
    Emitter* healEmitter = nullptr;
    Emitter* hasteEmitter = nullptr;
    std::vector<ObjectData::Perk> perks;
    bool dead = false;
    bool downed = false;
    uint8_t teamId = 0;
    int animType = 0;
    int animSeq = -1;
    int currentAnim = 0;
    float animTicker = 0.0f;
    bool animMirror = false;
    std::string animation;
    PlayerPose bones{}, animBones{};
    unsigned animMask = 0;
    bool visualsDirty = true;
    int throwableState = 0;

    pix::Container* container = nullptr;
    pix::Container* bodyContainer = nullptr;
    std::array<pix::Container*, 4> boneContainers{};
    pix::Sprite* bodySprite = nullptr;
    pix::Sprite* backpackSprite = nullptr;
    pix::Sprite* chestSprite = nullptr;
    pix::Sprite* helmetSprite = nullptr;
    pix::Sprite* visorSprite = nullptr;
    pix::Sprite* hipSprite = nullptr;
    pix::Sprite* meleeSprite = nullptr;
    pix::Sprite* flakSprite = nullptr;
    pix::Sprite* steelskinSprite = nullptr;
    std::array<pix::Sprite*, 4> limbSprites{};
    std::array<pix::Container*, 2> gunContainers{};
    std::array<pix::Sprite*, 2> gunSprites{}, magSprites{}, objectSprites{};
    pix::Text* nameText = nullptr;

    void m_init() override;
    void m_free() override;
    void m_updateData(const ObjectData& data, bool fullUpdate, bool isNew, Ctx& ctx) override;
    void update(float dt, Ctx& ctx);
    void playAnim(int type, int seq);
    void updateVisuals(Ctx& ctx);
};

class PlayerBarn {
public:
    Pool<Player> playerPool;
    std::unordered_map<uint16_t, std::string> names;
    std::unordered_map<uint16_t, uint8_t> teams;
    void update(float dt, GameWorld& ctx);
    std::string getPlayerName(uint16_t playerId, uint16_t activePlayerId, bool anon);
    Player* getPlayerById(uint16_t playerId);
};

} // namespace surv
