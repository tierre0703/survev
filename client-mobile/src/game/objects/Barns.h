#pragma once
// Ports of the client object barns (client/src/objects/*.ts): obstacle,
// building, loot, deadBody, projectile, smoke, bullet, explosion, player and
// the particle emitter. Draw calls go through the PixiLike adapter so the
// geometry/order mirrors the web client.
#include "../../core/Collider.h"
#include "../../core/Vec2.h"
#include "../../render/Camera.h"
#include "../../render/Defs.h"
#include "GameObject.h"
#include <cstdint>
#include <string>
#include <vector>

namespace pix {
class Container;
class Sprite;
class Graphics;
class Text;
class Factory;
} // namespace pix

namespace surv {

class GameWorld;

struct Bullet; // shared/net Messages.h

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

    pix::Container* container = nullptr;
    pix::Sprite* sprite = nullptr;
    std::string frame;

    void m_init() override;
    void m_free() override;
    void m_updateData(const ObjectData& data, bool fullUpdate, bool isNew, Ctx& ctx) override;
    void update(float dt, Ctx& ctx);
};

// --- building.ts (collision/ceiling subset) --------------------------------
class Building : public AbstractObject {
public:
    std::string type;
    Vec2 pos;
    int ori = 0;
    float scale = 1.0f;
    int layer = 0;
    int zIdx = 0;
    bool ceilingDead = false;
    bool occupied = false;
    bool ceilingDamaged = false;
    bool hasPuzzle = false;
    bool puzzleSolved = false;
    int puzzleErrSeq = 0;

    struct Surface {
        std::string type;
        std::vector<Collider> colliders;
    };
    std::vector<Surface> surfaces;
    Collider aabb;
    bool hasAabb = false;
    std::vector<Collider> ceilingRegions;
    float ceilingVisionTicker = 0.0f;
    float ceilingFadeAlpha = 0.0f;

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

    pix::Container* container = nullptr;
    pix::Sprite* sprite = nullptr;

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

// --- particles.ts (emitter subset) ----------------------------------------
class Particle {
public:
    bool active = false;
    Vec2 pos, vel;
    float life = 0.0f, lifeMax = 1.0f;
    float rad = 0.0f;
    uint32_t tint = 0xffffff;
    pix::Sprite* sprite = nullptr;
};

class Emitter {
public:
    bool enabled = false;
    Vec2 pos, dir;
    int layer = 0;
    float radius = 0.0f;
    float rateMult = 1.0f;
    float alpha = 1.0f;
    std::string type;
};

class ParticleBarn {
public:
    std::vector<Particle*> particles;
    std::vector<Emitter*> emitters;

    Emitter* addEmitter(pix::Factory* factory, const std::string& type, const Vec2& pos,
                        const Vec2& dir, int layer);
    void update(float dt, GameWorld& ctx);
};

// --- player.ts (render subset) --------------------------------------------
class Player : public AbstractObject {
public:
    Vec2 pos;
    Vec2 dir;
    int layer = 0;
    std::string name;
    std::string outfit;
    bool dead = false;
    bool downed = false;
    uint8_t teamId = 0;
    int animType = 0;

    pix::Container* container = nullptr;
    pix::Sprite* bodySprite = nullptr;
    pix::Text* nameText = nullptr;

    void m_init() override;
    void m_free() override;
    void m_updateData(const ObjectData& data, bool fullUpdate, bool isNew, Ctx& ctx) override;
    void update(float dt, Ctx& ctx);
};

class PlayerBarn {
public:
    Pool<Player> playerPool;
    std::unordered_map<uint16_t, std::string> names;
    void update(float dt, GameWorld& ctx);
    std::string getPlayerName(uint16_t playerId, uint16_t activePlayerId, bool anon);
    Player* getPlayerById(uint16_t playerId);
};

} // namespace surv
