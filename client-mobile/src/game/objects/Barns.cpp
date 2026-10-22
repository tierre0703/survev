#include "Barns.h"

#include "../GameWorld.h"
#include "../Map.h"
#include "../../render/Renderer.h"
#include "../../audio/AudioManager.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace surv {

static constexpr float pi = 3.14159265358979f;

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

// World space -> the parent node of a render layer. The render layer containers
// live under the camera-transformed world root (renderer.ts `pixiContainers`),
// so every object node/geometry is authored in world (point) units, exactly like
// the web client. Projecting into screen space here would apply the camera
// twice and offset the world whenever m_z() != 1 or the aspect is not 16:9.
static void placeInWorldLayer(pix::Node* node, const Vec2& worldPos) {
    node->setPosition(worldPos.x, worldPos.y);
}

// AABB of a map-object def collider (obstacle.ts uses collider.toAabb).
static Collider colliderToAabbDef(const Collider& c) {
    if (c.type == Collider::Circle) {
        return Collider::createAabb(Vec2(c.pos.x - c.rad, c.pos.y - c.rad),
                                    Vec2(c.pos.x + c.rad, c.pos.y + c.rad));
    }
    return Collider::createAabb(c.min, c.max);
}

// util.sameLayer (shared/utils/util.ts): objects on the same render layer.
static bool sameGameLayer(int a, int b) {
    return ((a & 0x1) == (b & 0x1)) || (((a & 0x2) != 0) && ((b & 0x2) != 0));
}

static Vec2 randomUnitDir() {
    const float angle = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) *
                        2.0f * 3.14159265358979f;
    return Vec2(std::cos(angle), std::sin(angle));
}

// Darkens a tint by a biome valueAdjust factor (util.adjustValue).
static uint32_t adjustValue(uint32_t tint, float value) {
    if (value >= 1.0f) {
        return tint;
    }
    const int r = static_cast<int>(std::lround(((tint >> 16) & 0xff) * value));
    const int g = static_cast<int>(std::lround(((tint >> 8) & 0xff) * value));
    const int b = static_cast<int>(std::lround((tint & 0xff) * value));
    return (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) |
           static_cast<uint32_t>(b);
}

// util.lerpColor (decal.ts): an sRGB<->linear lerp used by the decal gore fade.
static uint32_t lerpColor(float t, uint32_t start, uint32_t end) {
    const auto toLinear = [](float c) { return std::pow(c / 255.0f, 2.2f); };
    const auto toSRGB = [](float c) { return std::pow(c, 1.0f / 2.2f) * 255.0f; };
    const auto channel = [&](uint32_t a, uint32_t b, int shift) {
        const float s = toLinear(static_cast<float>((a >> shift) & 0xff));
        const float e = toLinear(static_cast<float>((b >> shift) & 0xff));
        const float v = math::lerp(t, s, e);
        return static_cast<uint32_t>(std::lround(toSRGB(v))) & 0xff;
    };
    return (channel(start, end, 16) << 16) | (channel(start, end, 8) << 8) |
           channel(start, end, 0);
}

// GameConfig.lootRadius keyed by item type (shared/gameConfig.ts).
static float lootRadius(const std::string& type) {
    if (type == "melee" || type == "gun" || type == "perk") return 1.25f;
    if (type == "ammo") return 1.2f;
    return 1.0f;
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
    buttonCanUse = false;
    isButton = false;
    isDoor = false;
    layer = 0;
    isSkin = false;
    exploded = false;
    isNew = false;
    dead = false;
    rot = 0.0f;
    imgRot = Vec2(0.0f, 0.0f);
    imgDirty = true;
    imgAlpha = 1.0f;
    imgTint = 0xffffff;
    zIdx = 0;
    _visible = false;
    casingEnabled = false;
    posInterpTicker = 0.0f;
    posInterpOld = Vec2(0.0f, 0.0f);
    _anchor = Vec2(0.5f, 0.5f);
    _anchorApplied = false;
    _firstUpdate = true;
    if (smokeEmitter) {
        smokeEmitter->stop();
        smokeEmitter = nullptr;
    }
}

void Obstacle::m_free() {
    if (sprite) {
        sprite->setVisible(false);
    }
    if (casingSprite) {
        casingSprite->setVisible(false);
    }
    if (smokeEmitter) {
        smokeEmitter->stop();
        smokeEmitter = nullptr;
    }
}

void Obstacle::m_updateData(const ObjectData& data, bool fullUpdate, bool isNew_, Ctx& ctx) {
    const bool first = _firstUpdate;
    _firstUpdate = false;
    const MapObjectDef* def = mapDefFor(data.type);

    if (fullUpdate) {
        type = data.type;
        layer = data.layer;
        healthT = data.healthT;
        // obstacle.ts: the collider is transformed once from the def and the
        // def's collision flags/height gate melee/bullet hits.
        const MapObjectDef* d = mapDefFor(data.type);
        collider = colliderTransform(d ? d->collision : Collider{}, data.pos, rot, data.scale);
        height = d ? d->height : 0.0f;
        collidable = d ? d->collidable : false;
        isWindow = d ? d->isWindow : false;
        if (!data.dead) {
            auto& ids = ctx.map().deadObstacleIds;
            ids.erase(std::remove(ids.begin(), ids.end(), __id), ids.end());
            exploded = false;
        }
        dead = data.dead;
        isSkin = data.isSkin;
        isDoor = data.isDoor;
        if (isSkin) {
            skinPlayerId = data.skinPlayerId;
        }
    }

    // Skins interpolate between their previous and current position.
    if (!v2Eq(data.pos, posInterpOld)) {
        posInterpOld = isNew_ || first ? data.pos : pos;
        posInterpTicker = 0.0f;
    }

    pos = data.pos;
    rot = math::oriToRad(data.ori);
    scale = data.scale;
    if (def) {
        imgScale = def->img.scale;
        imgMirrorX = def->img.mirrorX;
        imgMirrorY = def->img.mirrorY;
    }

    const bool newObj = isNew_ || first;
    if (newObj) {
        isNew = true;
        exploded = std::find(ctx.map().deadObstacleIds.begin(), ctx.map().deadObstacleIds.end(),
                              __id) != ctx.map().deadObstacleIds.end();
        // obstacle.ts: imgRot holds the def's per-image rot, or a random
        // rotation derived from the stable id.
        imgRot = Vec2(0.0f, 0.0f);
        if (def && !def->isDoor) {
            imgRot.x = math::oriToRad(def->img.ori);
        }
        if (def && def->randomRotation) {
            // Use the id (stable per obstacle) so the rotation doesn't change
            // as it is re-added to the screen.
            imgRot.x += math::deg2rad(static_cast<float>(__id % 360));
        }
        if (data.isDoor) {
            const float casingRot = rot + pi * 0.5f;
            doorHasInterp = true;
            doorInterpPos = data.pos;
            doorInterpRot = math::oriToRad(data.ori);
            doorClosedPos = data.pos;
            casingEnabled = def && !def->doorCasingSprite.empty();
            if (casingEnabled) {
                casingPosOffset = v2Rotate(def->doorCasingPos, casingRot);
                casingImgScale = def->doorCasingScale;
                casingTint = def->doorCasingTint;
                casingAlpha = def->doorCasingAlpha;
            }
        } else {
            doorHasInterp = false;
        }
    }

    if (data.isDoor && doorHasInterp) {
        const float slideOffset = def ? def->doorSlideOffset : 0.0f;
        const Vec2 offset = v2Rotate(Vec2(slideOffset, 0.0f), rot + pi * 0.5f);
        const Vec2 closedPos = data.doorOpen ? v2Add(data.pos, offset) : data.pos;
        // Re-seed the interpolation target after a respawn/pool reuse.
        if (newObj) {
            doorInterpPos = closedPos;
            doorInterpRot = rot;
        }
    }
    doorOpen = data.doorOpen;
    doorCanUse = data.doorCanUse;
    doorLocked = data.doorLocked;
    doorSeq = data.doorSeq;
    isButton = data.isButton;
    buttonOnOff = data.buttonOnOff;
    buttonCanUse = data.buttonCanUse;
    buttonSeq = data.buttonSeq;
    isPuzzlePiece = data.isPuzzlePiece;
    parentBuildingId = data.isPuzzlePiece ? data.parentBuildingId : 0;

    // Health smoke emitter for explodable obstacles (obstacle.ts uses a higher
    // health threshold for player skins).
    if (def && def->hasExplosion && !smokeEmitter && data.healthT < 0.5f && !data.dead) {
        const Vec2 dir = v2Normalize(Vec2(1.0f, 1.0f));
        EmitterOptions opts;
        opts.pos = pos;
        opts.dir = dir;
        opts.layer = layer;
        smokeEmitter = ctx.particleBarn().addEmitter(ctx.factory(), "smoke_barrel", opts);
    }

    // obstacle.ts `this.img`: only rebake when the sprite name changes.
    std::string currentImg = data.dead ? (def ? def->img.residue : "") : (def ? def->img.sprite : "");
    if (currentImg != frame) {
        frame = currentImg;
        imgDirty = true;
    }
}

void Obstacle::applyFrame(bool hasImage) {
    imgDirty = false;
    const MapObjectDef* def = mapDefFor(type);

    float zOrd_ = def ? def->img.zIdx : 0;
    int zIdx_ = static_cast<int>(std::floor(scale * 1000.0f)) * 65535 + __id;
    float imgAlpha_ = dead ? 0.75f : (def ? def->img.alpha : 1.0f);
    // Player skins tint by health (updateVisibility); other obstacles by the
    // value-adjusted def tint (updateVisibility + map's biome valueAdjust).
    uint32_t tint = def ? def->img.tint : 0xffffff;
    const float valueAdjust = _mapValueAdjust;
    if (isSkin) {
        tint = adjustValue(0xff0000, math::clamp(healthT, 0.0f, 1.0f));
    } else if (valueAdjust < 1.0f) {
        tint = adjustValue(tint, valueAdjust);
    }
    imgAlpha = imgAlpha_;
    zOrd = zOrd_;
    zIdx = zIdx_;
    imgTint = tint;
    hitParticle = def ? def->hitParticle : "";
    punchSound = def ? def->punchSound : "";

    if (!sprite) {
        return;
    }
    // obstacle.ts: anchor 0.5 normally, or def.door.spriteAnchor for doors.
    // The anchor is applied unconditionally (the node is a menu-less sprite;
    // re-applying the same value is cheap and keeps pool reuse correct).
    const Vec2 anchor = (def && isDoor) ? def->doorSpriteAnchor : Vec2(0.5f, 0.5f);
    _anchor = anchor;
    sprite->setAnchor(anchor.x, anchor.y);
    if (hasImage) {
        sprite->setFrame(frame);
    }
    sprite->setVisible(hasImage);
    if (hasImage) {
        sprite->setTint(tint);
        sprite->setAlpha(imgAlpha);
    }
    _visible = hasImage;
}

void Obstacle::applyColor() {
    if (!sprite || isSkin || !_visible) {
        return;
    }
    const MapObjectDef* def = mapDefFor(type);
    uint32_t tint = def ? def->img.tint : imgTint;
    if (_mapValueAdjust < 1.0f) {
        tint = adjustValue(tint, _mapValueAdjust);
    }
    imgTint = tint;
    sprite->setTint(tint);
    sprite->setAlpha(imgAlpha);
}

void Obstacle::update(float dt, Ctx& ctx) {
    const MapObjectDef* def = mapDefFor(type);
    if (!def) {
        return;
    }
    _mapValueAdjust = ctx.map().mapDef.valueAdjust;

    const bool explodedNow = dead && !exploded;
    if (explodedNow) {
        ctx.map().deadObstacleIds.push_back(__id);
        exploded = true;
        if (smokeEmitter) {
            smokeEmitter->stop();
            smokeEmitter = nullptr;
        }
    }

    if (doorHasInterp) {
        const float moveSpd = 15.0f * scale;
        const Vec2 posDiff = v2Sub(pos, doorInterpPos);
        const float diffLen = v2Length(posDiff);
        float posMove = moveSpd * dt;
        if (diffLen < posMove) posMove = diffLen;
        const Vec2 moveDir = diffLen > 0.0001f ? v2Div(posDiff, diffLen) : Vec2(1.0f, 0.0f);
        doorInterpPos = v2Add(doorInterpPos, v2Mul(moveDir, posMove));
        const float rotSpd = pi * 15.0f * scale;
        const float angDiff = math::angleDiff(doorInterpRot, rot);
        float angMove = math::sign(angDiff) * rotSpd * dt;
        if (std::fabs(angDiff) < std::fabs(angMove)) angMove = angDiff;
        doorInterpRot += angMove;
    }

    if (smokeEmitter) {
        const float smokeHealthT = isSkin ? 0.3f : 0.5f;
        smokeEmitter->pos = pos;
        smokeEmitter->enabled = !dead && healthT < smokeHealthT;
    }

    isNew = false;
}

void Obstacle::render(Ctx& ctx, int activeLayer) {
    const MapObjectDef* def = mapDefFor(type);
    const bool explodedNow = dead && !exploded;

    if (dead && !explodedNow && def && def->hasExplosion && !isNew) {
        const Collider aabb = colliderToAabbDef(def->hasCollision ? def->collision : Collider{});
        const Vec2 extent = v2Mul(v2Sub(aabb.max, aabb.min), 0.5f);
        const Vec2 center = v2Add(aabb.min, extent);
        const int numParticles = static_cast<int>(rnd(5.0f, 11.0f));
        for (int i = 0; i < numParticles; i++) {
            const Vec2 vel = v2RandomUnit(rnd(5.0f, 15.0f));
            ctx.particleBarn().addParticle(ctx.factory(), def->explosionParticle, layer, center, vel);
        }
    }

    if (!sprite) {
        sprite = ctx.factory()->createSprite();
    }
    // obstacle.ts m_updateData: anchor 0.5, or def.door.spriteAnchor for doors.
    // Bake it into the sprite before rendering the current frame.
    const Vec2 obstacleAnchor = (def && isDoor) ? def->doorSpriteAnchor : Vec2(0.5f, 0.5f);
    if (!_anchorApplied || obstacleAnchor.x != _anchor.x || obstacleAnchor.y != _anchor.y) {
        _anchor = obstacleAnchor;
        _anchorApplied = true;
        if (sprite) {
            sprite->setAnchor(_anchor.x, _anchor.y);
        }
    }
    if (imgDirty) {
        applyFrame(!frame.empty() && frame != "none");
    }
    if (!_visible) {
        isNew = false;
        return;
    }

    Vec2 renderPos = doorHasInterp ? doorInterpPos : pos;
    const float renderRot = doorHasInterp ? doorInterpRot : rot;

    if (isSkin && ctx.camera().m_interpEnabled) {
        posInterpTicker += ctx.dt();
        const float posT = ctx.camera().m_interpInterval > 0.0f
                               ? math::clamp(posInterpTicker / ctx.camera().m_interpInterval, 0.0f, 1.0f)
                               : 1.0f;
        renderPos = v2Lerp(posT, posInterpOld, pos);
    }

    // obstacle.ts projects the sprite to screen space and scales it by
    // camera.m_pixels(scale * imgScale) = x * m_zoom. The port keeps sprites in
    // world units under the camera-transformed content root (m_ppu * m_zoom),
    // so the same screen size needs the /m_ppu conversion.
    const float spriteScale = scale * imgScale / ctx.camera().m_ppu;
    // obstacle.ts renders the sprite directly at the object's world position;
    placeInWorldLayer(sprite, renderPos);
    float sx = spriteScale;
    float sy = spriteScale;
    if (imgMirrorY) sy *= -1.0f;
    if (imgMirrorX) sx *= -1.0f;
    sprite->setScale(sx, sy);
    sprite->setRotation(-renderRot + imgRot.x);

    int curZOrd = dead ? 5 : zOrd;
    int curZIdx = zIdx;
    int renderLayer = layer;
    // Render trees/bushes above stair elements when viewing only the ground.
    if (!dead && curZOrd >= 50 && layer == 0 && activeLayer == 0) {
        curZOrd += 100;
        renderLayer |= 2;
    }

    // Player skins inherit the skinned player's sort key (obstacle.ts).
    if (!dead && isSkin) {
        Player* skinPlayer = ctx.playerBarn().getPlayerById(skinPlayerId);
        if (skinPlayer) {
            curZOrd = math::max(math::max(curZOrd, skinPlayer->renderZOrd), 21);
            if (skinPlayer->renderZLayer != 0) {
                renderLayer = skinPlayer->renderZLayer;
                curZOrd = skinPlayer->renderZOrd;
            }
            curZIdx = skinPlayer->renderZIdx + 262144;
        }
    }
    ctx.renderer().addPIXIObj(sprite, renderLayer, curZOrd, curZIdx);

    if (doorHasInterp && casingEnabled) {
        if (!casingSprite) {
            casingSprite = ctx.factory()->createSprite();
            casingSprite->setAnchor(0.5f, 0.5f);
        }
        const Vec2 casingWorld = v2Add(doorClosedPos, casingPosOffset);
        const float casingScale = scale * casingImgScale / ctx.camera().m_ppu;
        placeInWorldLayer(casingSprite, casingWorld);
        casingSprite->setScale(casingScale, casingScale);
        casingSprite->setRotation(-renderRot);
        casingSprite->setVisible(!dead);
        ctx.renderer().addPIXIObj(casingSprite, renderLayer, curZOrd + 1, curZIdx);
    } else if (casingSprite) {
        casingSprite->setVisible(false);
    }
    isNew = false;
}

int Obstacle::updateDistanceToStairs(Ctx& ctx) const {
    return 0;
}

// ---------------------------------------------------------------------------
// Building
// ---------------------------------------------------------------------------
static bool sameLayerMask(int a, int b) {
    return ((a & 0x1) == (b & 0x1)) || ((a & 0x2) != 0 && (b & 0x2) != 0);
}

static float stepToward(float cur, float target, float rate) {
    const float delta = target - cur;
    const float s = delta * rate;
    return std::fabs(s) < 0.001f ? delta : s;
}

void Building::m_init() {
    isNew = false;
    residue = nullptr;
    ceilingDead = false;
    ceilingDamaged = false;
    playedCeilingDeadFx = false;
    playedSolvedPuzzleFx = false;
    hasPuzzle = false;
    puzzleErrSeqModified = false;
    puzzleErrSeq = 0;
    puzzleSolved = false;
    ceilingVisionTicker = 0.0f;
    ceilingFadeAlpha = 1.0f;
    residueCreated = false;
    imgs.clear();
    surfaces.clear();
    ceilingRegions.clear();
    particleEmitters.clear();
    hasAabb = false;
}

void Building::m_free() {
    for (auto& img : imgs) {
        if (img.sprite) {
            img.sprite->setVisible(false);
        }
    }
    for (auto* e : particleEmitters) {
        if (e) {
            e->stop();
        }
    }
    particleEmitters.clear();
    imgs.clear();
    if (residue) {
        residue->setVisible(false);
        residue = nullptr;
    }
}

void Building::m_updateData(const ObjectData& data, bool fullUpdate, bool isNew_, Ctx& ctx) {
    if (fullUpdate) {
        type = data.type;
        pos = data.pos;
        ori = data.ori;
        rot = math::oriToRad(data.ori);
        scale = 1.0f;
        layer = data.layer;
    }
    ceilingDead = data.ceilingDead;
    ceilingDamaged = data.ceilingDamaged;
    occupied = data.occupied;
    hasPuzzle = data.hasPuzzle;
    if (hasPuzzle) {
        puzzleErrSeqModified = data.puzzleErrSeq != puzzleErrSeq;
        puzzleSolved = data.puzzleSolved;
        puzzleErrSeq = data.puzzleErrSeq;
    }

    const MapObjectDef* def = mapDefFor(type);
    if (isNew_ && def) {
        isNew = true;
        const auto& deadCeilings = ctx.map().deadCeilingIds;
        const auto& solvedPuzzles = ctx.map().solvedPuzzleIds;
        playedCeilingDeadFx =
            std::find(deadCeilings.begin(), deadCeilings.end(), __id) != deadCeilings.end();
        playedSolvedPuzzleFx = hasPuzzle &&
            std::find(solvedPuzzles.begin(), solvedPuzzles.end(), __id) != solvedPuzzles.end();

        aabb = colliderTransform(def->hasBounding ? def->boundingCollider
                                                   : Collider::createAabb(Vec2(), Vec2()),
                                 pos, rot, scale);
        hasAabb = def->hasBounding;
        zIdx = def->zIdx;

        vision = def->ceilingVision;

        ceilingRegions.clear();
        for (const auto& zoomIn : def->ceilingZoomIn) {
            ceilingRegions.push_back(colliderTransform(zoomIn, pos, rot, scale));
        }

        const float valueAdjust = ctx.map().mapDef.valueAdjust;

        auto createImg = [&](const BuildingImageDef& imgDef, bool isCeiling, int index) {
            Img out;
            out.sprite = ctx.factory()->createSprite();
            out.sprite->setAnchor(0.5f, 0.5f);
            if (!imgDef.sprite.empty() && imgDef.sprite != "none") {
                out.sprite->setFrame(imgDef.sprite);
            }
            uint32_t tint = imgDef.tint;
            if (valueAdjust < 1.0f) {
                tint = adjustValue(tint, valueAdjust);
            }
            out.sprite->setTint(tint);
            out.posOffset = v2Rotate(imgDef.pos, rot);
            out.rotOffset = math::oriToRad(static_cast<int>(imgDef.rot));
            out.imgAlpha = imgDef.alpha;
            out.defScale = imgDef.scale;
            out.mirrorX = imgDef.mirrorX;
            out.mirrorY = imgDef.mirrorY;
            out.isCeiling = isCeiling;
            out.removeOnDamaged = isCeiling && imgDef.removeOnDamaged;
            out.zOrd = isCeiling ? (750 - zIdx) : zIdx;
            out.zIdx = __id * 100 + index;
            out.sprite->setVisible(true);
            out.sprite->setAlpha(imgDef.alpha);
            return out;
        };

        imgs.clear();
        for (size_t i = 0; i < def->floorImgs.size(); i++) {
            imgs.push_back(createImg(def->floorImgs[i], false, static_cast<int>(i)));
        }
        for (size_t i = 0; i < def->ceilingImgs.size(); i++) {
            imgs.push_back(createImg(def->ceilingImgs[i], true, static_cast<int>(i)));
        }

        // Occupied particle emitters.
        particleEmitters.clear();
        for (const auto& e : def->occupiedEmitters) {
            const float r = rot + e.rot;
            Vec2 epos = v2Add(pos, v2Rotate(e.pos, r));
            Vec2 edir = v2Rotate(e.dir, r);
            float escale = e.scale;
            pix::Node* parent = nullptr;
            if (e.parentToCeiling) {
                int lastIdx = -1;
                for (size_t b = 0; b < imgs.size(); b++) {
                    if (imgs[b].isCeiling) {
                        lastIdx = static_cast<int>(b);
                    }
                }
                if (lastIdx >= 0) {
                    parent = imgs[lastIdx].sprite;
                    // Parented sprites use a different coordinate system.
                    epos = v2Mul(e.pos, 32.0f);
                    epos.y *= -1.0f;
                    edir = v2Rotate(Vec2(1.0f, 0.0f), e.rot);
                    escale = 1.0f / imgs[lastIdx].defScale;
                }
            }
            EmitterOptions opts;
            opts.pos = epos;
            opts.dir = edir;
            opts.scale = escale;
            opts.layer = e.layer;
            opts.parent = parent;
            particleEmitters.push_back(
                ctx.particleBarn().addEmitter(ctx.factory(), e.type, opts));
        }
    }
    ctx.renderer().layerMaskDirty = true;
}

void Building::update(float dt, Ctx& ctx) {
    // Destroy ceiling fx (audio/particles are stubbed; the residue + reveal
    // below carry the visual).
    if (ceilingDead && !playedCeilingDeadFx) {
        ctx.map().deadCeilingIds.push_back(__id);
        playedCeilingDeadFx = true;
        if (!isNew && ctx.audio()) {
            audio::PlaySoundOptions opts;
            opts.channel = "sfx";
            opts.hasSoundPos = true;
            opts.soundPos = pos;
            opts.hasLayer = true;
            opts.layer = layer;
            ctx.audio()->playSound("ceiling_break_01", opts);
        }
    }
    isNew = false;

    // Residue left behind by a destroyed ceiling.
    if (ceilingDead && !residueCreated) {
        const MapObjectDef* def = mapDefFor(type);
        if (def && !def->ceilingDestroyResidue.empty() && def->ceilingDestroyResidue != "none" &&
            !imgs.empty() && imgs[0].sprite) {
            residue = ctx.factory()->createSprite(def->ceilingDestroyResidue);
            residue->setAnchor(0.5f, 0.5f);
            residue->setPosition(0.0f, 0.0f);
            residue->setScale(1.0f, 1.0f);
            residue->setRotation(0.0f);
            residue->setTint(0xffffff);
            residue->setVisible(true);
            imgs[0].sprite->addChild(residue);
            residueCreated = true;
        }
    }

    // Determine ceiling visibility.
    ceilingVisionTicker -= dt;
    Player* ap = ctx.activePlayer();
    bool canSeeInside = false;
    if (ap) {
        const bool layerMatch = (layer == ap->layer) || ((ap->layer & 2) != 0);
        if (layerMatch) {
            const Collider scan = Collider::createCircle(ap->pos, vision.width);
            for (const auto& zoomIn : ceilingRegions) {
                if (colliderIntersect(zoomIn, scan)) {
                    canSeeInside = true;
                    break;
                }
            }
            if (!canSeeInside && hasAabb &&
                getDistanceToBuilding(ap->pos, vision.dist) < vision.dist) {
                canSeeInside = true;
            }
        }
    }
    if (ceilingDead) {
        canSeeInside = true;
    }
    if (canSeeInside) {
        ceilingVisionTicker = vision.linger + 0.0001f;
    }

    const bool visible = ceilingVisionTicker > 0.0f;
    ceilingFadeAlpha += stepToward(ceilingFadeAlpha, visible ? 0.0f : 1.0f,
                                   dt * (visible ? 12.0f : vision.fadeRate));

    // Immediately reveal a ceiling when on stairs and able to see the other layer.
    if (canSeeInside && ap && (ap->layer & 2) != 0 && !sameLayerMask(ap->layer, layer)) {
        ceilingFadeAlpha = 0.0f;
    }

    for (auto* e : particleEmitters) {
        if (e) {
            e->enabled = occupied;
        }
    }

    // Position sprites for rendering.
    for (auto& img : imgs) {
        if (!img.sprite) {
            continue;
        }
        const float alpha = img.isCeiling ? ceilingFadeAlpha : 1.0f;
        // building.ts: camera.m_pixels(scale * sprite.defScale); world units
        // need the /m_ppu conversion (see Obstacle::update).
        const float imgScale = scale * img.defScale / ctx.camera().m_ppu;
        placeInWorldLayer(img.sprite, v2Add(pos, img.posOffset));
        float sx = imgScale;
        float sy = imgScale;
        if (img.mirrorY) {
            sy *= -1.0f;
        }
        if (img.mirrorX) {
            sx *= -1.0f;
        }
        img.sprite->setScale(sx, sy);
        img.sprite->setRotation(-rot + img.rotOffset);
        img.sprite->setAlpha(img.imgAlpha * alpha);

        if (img.removeOnDamaged && ceilingDamaged) {
            img.sprite->setVisible(!ceilingDamaged);
        }

        int renderLayer = layer;
        if (img.isCeiling && ap &&
            (layer == ap->layer || (((ap->layer & 2) != 0) && layer == 1))) {
            renderLayer |= 2;
        }
        ctx.renderer().addPIXIObj(img.sprite, renderLayer, img.zOrd, img.zIdx);
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
    ticker = 0.0f;
    rad = 1.0f;
    imgScale = 1.0f;
    visualPosOld = Vec2();
    posInterpTicker = 0.0f;
    updatedData = false;
}

void Loot::m_free() {
    if (container) {
        container->setVisible(false);
    }
    if (emitter) {
        emitter->stop();
        emitter = nullptr;
    }
}

void Loot::m_updateData(const ObjectData& data, bool fullUpdate, bool isNew, Ctx& ctx) {
    updatedData = true;
    if (!v2Eq(data.pos, visualPosOld)) {
        visualPosOld = isNew ? data.pos : pos;
        posInterpTicker = 0.0f;
    }
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
    ownerId = data.hasOwner ? data.ownerId : 0;

    ensureSprite(ctx, container, sprite);
    const GameObjRenderDef* def = gameDefFor(type);
    if (isNew) {
        ticker = isOld ? 10.0f : 0.0f;
        rad = lootRadius(type);
        imgScale = def ? def->img.scale * 1.25f : 1.25f;
        const float innerScale = 0.8f;
        sprite->setScale(innerScale, innerScale);
        if (def && def->hasImg && !def->img.sprite.empty()) {
            sprite->setFrame(def->img.sprite);
            sprite->setTint(def->img.tint);
        }
        container->setVisible(true);
    }
    if (isNew && def && !def->emitter.empty()) {
        EmitterOptions opts;
        opts.pos = pos;
        opts.layer = layer;
        emitter = ctx.particleBarn().addEmitter(ctx.factory(), def->emitter, opts);
    }
}

void Loot::update(float dt, Ctx& ctx) {
    if (!container) {
        return;
    }
    ticker += dt;
    if (emitter) {
        emitter->pos = v2Add(pos, Vec2(0.0f, 0.1f));
        emitter->layer = layer;
    }
    const float scaleIn = math::delerp(ticker, 0.0f, 1.0f);
    const float scale = math::easeOutElastic(scaleIn, 0.75f);
    Vec2 renderPos = pos;
    Camera& camera = ctx.camera();
    if (camera.m_interpEnabled) {
        posInterpTicker += dt;
        const float posT = camera.m_interpInterval > 0.0f
                               ? math::clamp(posInterpTicker / camera.m_interpInterval, 0.0f, 1.0f)
                               : 1.0f;
        renderPos = v2Lerp(posT, visualPosOld, pos);
    }
    // loot.ts: camera.m_pixels(loot.imgScale * scale) -> /m_ppu in world units.
    const float lootScale = imgScale * scale / camera.m_ppu;
    placeInWorldLayer(container, renderPos);
    container->setScale(lootScale, lootScale);
    container->setVisible(true);
    ctx.renderer().addPIXIObj(container, layer, 13, __id);
}

void LootBarn::update(float dt, GameWorld& ctx) {
    for (auto* loot : lootPool.m_getPool()) {
        if (loot->active) {
            loot->update(dt, ctx);
        }
    }
}

// ---------------------------------------------------------------------------
// Decal (decal.ts)
// ---------------------------------------------------------------------------
void DecalRenderObj::init(pix::Factory* factory, const std::string& type, const Vec2& decalPos,
                          float decalRot, float decalScale, int decalLayer, int id,
                          const Map& map) {
    const DefProvider* provider = getDefProvider();
    const MapObjectDef* def = provider ? provider->mapObject(type) : nullptr;

    pos = decalPos;
    rot = decalRot;
    scale = decalScale;
    layer = decalLayer;
    zIdx = def ? def->img.zIdx : 0;
    zOrd = id;

    if (!sprite) {
        sprite = factory->createSprite();
        sprite->setAnchor(0.5f, 0.5f);
    }
    sprite->setFrame(def ? def->img.sprite : "");
    sprite->setAlpha(1.0f);
    sprite->setVisible(true);

    imgScale = def ? def->img.scale : 1.0f;
    spriteAlpha = def ? def->img.alpha : 1.0f;
    valueAdjust = (def && def->imgIgnoreAdjust) ? 1.0f : map.mapDef.valueAdjust;
    setTint(def ? def->img.tint : 0xffffff);

    inWater = false;
    if (def && def->height < 0.25f) {
        const Map::GroundSurface surface = map.getGroundSurface(decalPos, decalLayer);
        inWater = surface.type == Map::SurfaceType::Water;
    }

    flicker = def && def->imgFlicker;
    if (flicker) {
        flickerMin = def->imgFlickerMin;
        flickerMax = def->imgFlickerMax;
        flickerTarget = imgScale;
        flickerRate = def->imgFlickerRate;
        flickerCooldown = 0.0f;
    }

    active = true;
    deactivated = false;
    fadeout = def && def->hasLifetime;
    fadeAlpha = 1.0f;
}

void DecalRenderObj::free() {
    deactivated = true;
}

void DecalRenderObj::setTint(uint32_t color) {
    if (valueAdjust < 1.0f) {
        color = adjustValue(color, valueAdjust);
    }
    if (sprite) {
        sprite->setTint(color);
    }
}

void DecalRenderObj::update(float dt, float valueAdjust_, const Camera& camera, Renderer& renderer) {
    (void)valueAdjust_;
    if (deactivated && fadeout) {
        fadeAlpha = math::lerp(dt * 3.0f, fadeAlpha, 0.0f);
        if (fadeAlpha < 0.01f) {
            fadeAlpha = 0.0f;
        }
    }
    if (deactivated && (!fadeout || math::eqAbs(fadeAlpha, 0.0f))) {
        if (sprite) {
            sprite->setVisible(false);
        }
        active = false;
    }

    if (flicker) {
        if (flickerCooldown < 0.0f) {
            flickerTarget = rnd(flickerMin, flickerMax);
            flickerCooldown = rnd(0.05f, flickerRate);
        } else {
            imgScale = math::lerp(flickerRate - flickerCooldown, imgScale, flickerTarget);
            flickerCooldown -= dt;
        }
    }
    if (!sprite) {
        return;
    }
    // decal.ts: camera.m_pixels(scale * imgScale) -> /m_ppu in world units.
    const float spriteScale = scale * imgScale / camera.m_ppu;
    placeInWorldLayer(sprite, pos);
    sprite->setScale(spriteScale, spriteScale);
    sprite->setRotation(-rot);
    sprite->setAlpha(spriteAlpha * (inWater ? 0.3f : 1.0f) * fadeAlpha);
    renderer.addPIXIObj(sprite, layer, zIdx, zOrd);
}

void Decal::m_init() {
    isNew = false;
    goreT = 0.0f;
    hasGore = false;
    goreKills = 0;
    hasSurface = false;
    decalRender = nullptr;
}

void Decal::m_free() {
    if (decalRender) {
        decalRender->free();
        decalRender = nullptr;
    }
}

void Decal::m_updateData(const ObjectData& data, bool fullUpdate, bool isNew_, Ctx& ctx) {
    (void)isNew_;
    if (!fullUpdate) {
        return;
    }
    const DefProvider* provider = getDefProvider();
    const MapObjectDef* def = provider ? provider->mapObject(data.type) : nullptr;

    type = data.type;
    pos = data.pos;
    rot = math::oriToRad(data.ori);
    scale = data.scale;
    layer = data.layer;
    goreKills = data.count;
    collider = colliderTransform(def ? def->collision : Collider{}, pos, rot, scale);
    hasSurface = def && def->hasSurface;
    if (hasSurface) {
        surfaceType = def->surfaceType;
        surfaceWaterColor = def->surfaceWaterColor;
        surfaceRippleColor = def->surfaceRippleColor;
    }
    hasGore = def && def->hasGore;

    isNew = isNew_;
    if (isNew) {
        decalRender = ctx.decalBarn().allocDecalRender();
        decalRender->init(ctx.factory(), type, pos, rot, scale, layer, __id, ctx.map());
    }
}

void Decal::update(float dt, GameWorld& ctx) {
    const DefProvider* provider = getDefProvider();
    const MapObjectDef* def = provider ? provider->mapObject(type) : nullptr;
    if (hasGore && def && def->hasGore) {
        float goreTarget = math::delerp(goreKills, def->goreFadeStart, def->goreFadeEnd);
        goreTarget = std::pow(goreTarget, def->goreFadePow);
        goreT = isNew ? goreTarget
                      : math::lerp(dt * def->goreFadeSpeed, goreT, goreTarget);

        if (def->goreHasTint && decalRender) {
            decalRender->setTint(lerpColor(goreT, def->img.tint, def->goreTint));
        }
        if (def->goreHasAlpha && decalRender) {
            decalRender->spriteAlpha = math::lerp(goreT, def->img.alpha, def->goreAlpha);
        }
        if (def->goreHasWaterColor && hasSurface) {
            surfaceWaterColor = lerpColor(goreT, def->surfaceWaterColor, def->goreWaterColor);
        }
        if (def->goreHasRippleColor && hasSurface) {
            surfaceRippleColor = lerpColor(goreT, def->surfaceRippleColor, def->goreRippleColor);
        }
    }
    isNew = false;
}

DecalRenderObj* DecalBarn::allocDecalRender() {
    for (auto* d : decalRenders) {
        if (!d->active) {
            return d;
        }
    }
    auto* d = new DecalRenderObj();
    decalRenders.push_back(d);
    return d;
}

void DecalBarn::update(float dt, GameWorld& ctx) {
    for (auto* decal : decalPool.m_getPool()) {
        if (decal->active) {
            decal->update(dt, ctx);
        }
    }
    for (auto* decalRender : decalRenders) {
        if (decalRender->active) {
            decalRender->update(dt, ctx.map().mapDef.valueAdjust, ctx.camera(), ctx.renderer());
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
        // The sprite colour is baked into the skull frame; no extra tint.
        sprite->setTint(5921370u);
    }
    if (isNew) {
        nameTextSet = false;
        container->setVisible(true);
        sprite->setVisible(true);
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

        placeInWorldLayer(d->container, d->pos);
        // deadBody.ts: container.scale = camera.m_pixels(1) -> 1/m_ppu.
        d->container->setScale(1.0f / ctx.camera().m_ppu, 1.0f / ctx.camera().m_ppu);
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
        imgScale = def->img.scale;
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
        // projectile.ts: pos + (0, posZ) in world space.
        placeInWorldLayer(p->container, Vec2(p->pos.x, p->pos.y + p->posZ));
        // projectile.ts: camera.m_pixels(imgScale * remap(posZ, 0, maxHeight,
        // 1, 4.75)); world units need the extra /m_ppu.
        const float scale = p->imgScale * math::remap(p->posZ, 0.0f, defs::kProjectileMaxHeight, 1.0f, 4.75f);
        const float worldScale = scale / ctx.camera().m_ppu;
        p->container->setScale(worldScale, worldScale);
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
    // Cache the factory: the smoke pool outlives the adapter nodes, which are
    // released on a world reset, so the sprite is re-created in ensureSprite().
    this->factory = factory;
    if (factory && !sprite) {
        sprite = factory->createSprite();
        sprite->setAnchor(0.5f, 0.5f);
    }
    if (sprite) {
        const char* frames[2] = {"part-smoke-02.img", "part-smoke-03.img"};
        sprite->setFrame(frames[std::rand() % 2]);
        sprite->setVisible(true);
    }
}

void SmokeParticle::ensureSprite() {
    if (sprite || !factory) {
        return;
    }
    sprite = factory->createSprite();
    sprite->setAnchor(0.5f, 0.5f);
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
    particle->factory = factory;
    particle->active = true;
    particle->zIdx = zIdx--;
    return particle;
}

void SmokeBarn::onFactoryReset() {
    for (auto* p : m_particles) {
        p->sprite = nullptr;
        p->factory = nullptr;
    }
}

void SmokeBarn::update(float dt, GameWorld& ctx) {
    for (auto* p : m_particles) {
        if (!p->active) {
            continue;
        }
        p->ensureSprite();
        if (!p->sprite) {
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

        // smoke.ts: camera.m_pixels((rad * 2) / m_ppu); world units add /m_ppu.
        const float smokeScale = (p->rad * 2.0f) / (ctx.camera().m_ppu * ctx.camera().m_ppu);
        placeInWorldLayer(p->sprite, p->pos);
        p->sprite->setScale(smokeScale, smokeScale);
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
    b->playerId = data.playerId;
    b->whizHeard = false;
    b->t = 0.0f;
    if (!b->gfx) {
        b->gfx = factory->createGraphics();
    }
    b->gfx->clear();
    b->gfx->setVisible(true);
}

void BulletBarn::clear() {
    for (auto* b : bullets) {
        if (!b) {
            continue;
        }
        b->active = false;
        if (b->gfx) {
            b->gfx->setVisible(false);
            b->gfx->clear();
        }
    }
    // The visuals are heap-owned by the barn (not a Pool<T>), so drop them: the
    // next spawn reallocates lazily and nothing references the old frames.
    for (auto* b : bullets) {
        delete b;
    }
    bullets.clear();
}

void BulletBarn::update(float dt, GameWorld& ctx) {
    const Player* activePlayer = ctx.activePlayer();
    for (size_t i = 0; i < bullets.size(); i++) {
        BulletVisual* b = bullets[i];
        if (!b->active) {
            continue;
        }
        b->t += dt;

        // M6: bullet whiz sound (bullet.ts — close bullets near the camera that
        // are not from the active player produce a whiz group).
        if (!b->whizHeard && activePlayer && b->playerId != activePlayer->__id &&
            !activePlayer->dead && math::sameAudioLayer(activePlayer->layer, b->layer) &&
            v2Length(v2Sub(ctx.camera().m_pos, b->pos)) < 7.5f && ctx.audio()) {
            audio::PlaySoundOptions whizOpts;
            whizOpts.channel = "sfx";
            whizOpts.hasSoundPos = true;
            whizOpts.soundPos = b->pos;
            whizOpts.fallOff = 4.0f;
            ctx.audio()->playGroup("bullet_whiz", whizOpts);
            b->whizHeard = true;
        }

        const float length = b->clipDistance && b->distance > 0.0f ? b->distance : 30.0f;
        const Vec2 end = v2Add(b->startPos, v2Mul(b->dir, length));
        b->gfx->clear();
        b->gfx->lineStyle(2.0f, 0xffe08a, math::clamp(1.0f - b->t * 4.0f, 0.0f, 1.0f));
        b->gfx->moveTo(b->startPos.x, b->startPos.y);
        b->gfx->lineTo(end.x, end.y);
        ctx.renderer().addPIXIObj(b->gfx, b->layer, 900, b->layer * 100000 + static_cast<int>(i));
        // bullet.ts draws the tracer in world space; the world root scales it.
        b->gfx->setPosition(0.0f, 0.0f);
        b->gfx->setScale(1.0f, 1.0f);
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
        // The explosion graphics is authored in world units and the world root
        // carries the camera.
        placeInWorldLayer(e->container, e->pos);
        e->container->setScale(1.0f, 1.0f);
        e->container->setVisible(true);
        if (e->t >= dur) {
            e->active = false;
            e->container->setVisible(false);
        }
    }
}

// ---------------------------------------------------------------------------
// Player barn (skeletal runtime is in PlayerRender.cpp)
// ---------------------------------------------------------------------------
void PlayerBarn::update(float dt, GameWorld& ctx) {
    for (auto* p : playerPool.m_getPool()) {
        if (!p->active || !p->container) {
            continue;
        }
        auto team = teams.find(p->__id);
        if (team != teams.end() && p->teamId != team->second) {
            p->teamId = team->second;
            p->visualsDirty = true;
        }
        p->update(dt, ctx);
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
// Airdrop (airdrop.ts)
// ---------------------------------------------------------------------------
namespace GameConfigAirdrop {
constexpr float fallTime = defs::kAirdropFallTime;
}
namespace GameConfigBullet {
constexpr float reflectDistDecay = defs::kBulletReflectDistDecay;
}

void Airdrop::m_init() {
    playedLandFx = false;
    landed = false;
    isNew = false;
    fallTicker = 0.0f;
    rad = 1.0f;
    pos = Vec2(0.0f, 0.0f);
}

void Airdrop::m_free() {
    if (sprite) {
        sprite->setVisible(false);
    }
}

void Airdrop::m_updateData(const ObjectData& data, bool fullUpdate, bool isNew_, Ctx& ctx) {
    if (isNew_) {
        isNew = true;
        fallTicker = data.fallT * GameConfigAirdrop::fallTime;
        if (!sprite) {
            sprite = ctx.factory()->createSprite();
            sprite->setAnchor(0.5f, 0.5f);
        }
        const MapRenderDef& mapDef = ctx.map().mapDef;
        sprite->setFrame(mapDef.airdropChuteImg.empty() ? std::string("map-chute-01.img")
                                                        : mapDef.airdropChuteImg);
    }
    if (fullUpdate) {
        pos = data.pos;
    }
    landed = data.landed;
}

void AirdropBarn::update(float dt, GameWorld& ctx) {
    if (!ctx.activePlayer()) {
        return;
    }
    const int activeLayer = ctx.activePlayer()->layer;
    for (auto* airdrop : airdropPool.m_getPool()) {
        if (!airdrop->active || !airdrop->sprite) {
            continue;
        }

        airdrop->fallTicker += dt;
        const float fallT = math::clamp(airdrop->fallTicker / GameConfigAirdrop::fallTime, 0.0f, 1.0f);

        int layer = 0;
        if ((sameGameLayer(0, activeLayer) || (activeLayer & 0x2) != 0) &&
            ((activeLayer & 0x2) == 0 ||
             !ctx.map().insideStructureMask(Collider::createCircle(airdrop->pos, 1.0f)))) {
            layer |= 0x2;
        }

        if (airdrop->landed && !airdrop->playedLandFx) {
            airdrop->playedLandFx = true;

            if (!airdrop->isNew) {
                // Crate dust.
                for (int j = 0; j < 10; j++) {
                    const Vec2 vel = randomUnitDir();
                    ctx.particleBarn().addParticle(ctx.factory(), "airdropSmoke", layer, airdrop->pos, vel);
                }

                // Water landing ripples.
                const Map::GroundSurface surface = ctx.map().getGroundSurface(airdrop->pos, layer);
                if (surface.type == Map::SurfaceType::Water) {
                    for (int j = 0; j < 12; j++) {
                        const Vec2 ripplePos =
                            v2Add(airdrop->pos, v2RandomUnit(rnd(4.5f, 6.0f)));
                        ctx.particleBarn().addRippleParticle(ctx.factory(), ripplePos, layer,
                                                             surface.rippleColor);
                    }
                }

                // Crate hitting the ground.
                if (ctx.audio()) {
                    audio::PlaySoundOptions opts;
                    opts.channel = "sfx";
                    opts.hasSoundPos = true;
                    opts.soundPos = airdrop->pos;
                    opts.hasLayer = true;
                    opts.layer = layer;
                    opts.filter = "muffled";
                    ctx.audio()->playSound(surface.type == Map::SurfaceType::Water ? "airdrop_crash_02"
                                                                                  : "airdrop_crash_01",
                                           opts);
                }
            }
        }

        // Chute + falling loop sounds (only for a live fall, not a join snapshot).
        if (ctx.audio() && !airdrop->isNew) {
            if (fallT <= 0.1f) {
                audio::PlaySoundOptions opts;
                opts.channel = "sfx";
                opts.hasSoundPos = true;
                opts.soundPos = airdrop->pos;
                opts.hasLayer = true;
                opts.layer = layer;
                opts.rangeMult = 1.75f;
                ctx.audio()->playSound("airdrop_chute_01", opts);
            }
            if (!airdrop->landed) {
                audio::PlaySoundOptions opts;
                opts.channel = "sfx";
                opts.hasSoundPos = true;
                opts.soundPos = airdrop->pos;
                opts.hasLayer = true;
                opts.layer = layer;
                opts.rangeMult = 1.75f;
                opts.ignoreMinAllowable = true;
                opts.offset = airdrop->fallTicker;
                ctx.audio()->playSound("airdrop_fall_01", opts);
            }
        }

        airdrop->rad = math::lerp(std::pow(1.0f - fallT, 1.1f), 5.0f, 12.0f);
        ctx.renderer().addPIXIObj(airdrop->sprite, layer, 1500, airdrop->__id);

        // airdrop.ts: camera.m_pixels((2 * rad) / m_ppu); world units add /m_ppu.
        const float airdropScale = (2.0f * airdrop->rad) / (ctx.camera().m_ppu * ctx.camera().m_ppu);
        placeInWorldLayer(airdrop->sprite, airdrop->pos);
        airdrop->sprite->setScale(airdropScale, airdropScale);
        airdrop->sprite->setTint(0xffff00);
        airdrop->sprite->setAlpha(1.0f);
        airdrop->sprite->setVisible(!airdrop->landed);

        airdrop->isNew = false;
    }
}

// ---------------------------------------------------------------------------
// Flare (flare.ts)
// ---------------------------------------------------------------------------
void FlareBarn::addFlare(GameWorld& ctx, const Bullet& bullet) {
    FlareVisual* b = nullptr;
    for (auto* v : bullets) {
        if (!v->alive && !v->collided) {
            b = v;
            break;
        }
    }
    if (!b) {
        b = new FlareVisual();
        b->container = ctx.factory()->createContainer();
        b->container->setVisible(false);
        b->trail = ctx.factory()->createSprite("player-bullet-trail-02.img");
        b->trail->setAnchor(0.5f, 0.5f);
        b->trail->setBlendMode(pix::BlendMode::Add);
        b->container->addChild(b->trail);
        b->flare = ctx.factory()->createSprite("part-flare-01.img");
        b->flare->setAnchor(0.5f, 0.5f);
        b->flare->setBlendMode(pix::BlendMode::Add);
        b->container->addChild(b->flare);
        bullets.push_back(b);
    }

    const DefProvider* provider = getDefProvider();
    const BulletRenderDef* bulletDef = provider ? provider->bullet(bullet.bulletType) : nullptr;
    const float defDistance = bulletDef ? bulletDef->distance : 0.0f;
    const float defSpeed = bulletDef ? bulletDef->speed : 0.0f;
    const float variance = 1.0f + bullet.varianceT * (bulletDef ? bulletDef->variance : 0.0f);
    const float distAdj = math::remap(static_cast<float>(bullet.distAdjIdx), 0.0f, 32.0f, -1.0f, 1.0f);
    const float distance = defDistance / std::pow(GameConfigBullet::reflectDistDecay, bullet.reflectCount);

    b->alive = true;
    b->collided = false;
    b->flareScale = 0.01f;
    b->trailScale = 1.0f;
    b->timeAlive = 0.0f;
    b->maxTimeAlive = 2.5f;
    b->startPos = bullet.pos;
    b->pos = bullet.pos;
    b->dir = bullet.dir;
    b->layer = bullet.layer;
    b->playerId = bullet.playerId;
    b->speed = defSpeed * variance;
    b->distance = distance * variance + distAdj;

    b->container->setRotation(std::atan2(b->dir.x, b->dir.y) - pi / 2.0f);

    Player* player = ctx.playerBarn().getPlayerById(b->playerId);
    if (player && (player->layer & 2) != 0) {
        b->layer |= 2;
    }

    const std::string tracerName = bulletDef ? bulletDef->tracerColor : std::string();
    const TracerColorDef* tracer = provider ? provider->tracerColor(tracerName) : nullptr;
    const uint32_t tracerColor = tracer ? tracer->regular : 0xffffff;
    b->tracerLength = bulletDef ? bulletDef->tracerLength : 0.0f;
    b->maxFlareScale = bulletDef ? bulletDef->maxFlareScale : 1.0f;
    b->tracerAlphaRate = tracer ? tracer->alphaRate : 0.0f;
    b->tracerAlphaMin = tracer ? tracer->alphaMin : 0.0f;
    b->trail->setScale(0.8f, bulletDef ? bulletDef->tracerWidth : 1.0f);
    b->trail->setTint(tracerColor);
    b->trail->setAlpha(1.0f);
    b->flare->setScale(1.0f, 1.0f);
    b->flare->setTint(bulletDef ? bulletDef->flareColor : 0xffffff);
    b->flare->setAlpha(0.8f);
    b->container->setVisible(true);

    (void)defDistance;
}

void FlareBarn::update(float dt, GameWorld& ctx) {
    const Player* activePlayer = ctx.activePlayer();
    const int activeLayer = activePlayer ? activePlayer->layer : 0;

    for (auto* d : bullets) {
        if (!d->container) {
            continue;
        }
        // The flare keeps the bullet def it was spawned with via the tracer
        // length; the flare render only needs the max flare scale, stashed on
        // the object at spawn time.
        if (d->collided) {
            d->flareScale = math::max(d->flareScale - dt * 0.5f, 0.0f);
            d->flare->setAlpha(math::max(d->flare->getAlpha() - dt, 0.0f));
            d->trailScale = math::max(d->trailScale - dt * 6.0f, 0.0f);
            d->trail->setAlpha(math::max(d->trail->getAlpha() - dt, 0.0f));
            d->pos = v2Add(d->pos, v2Mul(d->dir, dt * d->speed));
            if (d->flare->getAlpha() <= 0.0f) {
                d->collided = false;
                d->container->setVisible(false);
            }
        }

        if (d->alive) {
            if (d->tracerAlphaRate > 0.0f) {
                const float rate = (activePlayer && activePlayer->__id == d->playerId)
                                       ? d->tracerAlphaRate
                                       : d->tracerAlphaRate * 0.9f;
                d->trail->setAlpha(math::max(d->tracerAlphaMin, d->trail->getAlpha() * rate));
            }

            d->timeAlive += dt;
            d->flareScale = math::easeOutExpo(d->timeAlive / d->maxTimeAlive) * d->maxFlareScale;

            const float distLeft = d->distance - v2Length(v2Sub(d->startPos, d->pos));
            const float distTravel = math::min(distLeft, dt * d->speed);
            d->pos = v2Add(d->pos, v2Mul(d->dir, distTravel));
            if (math::eqAbs(distLeft, distTravel)) {
                d->collided = true;
                d->alive = false;
            }

            int layer = 0;
            if ((sameGameLayer(0, activeLayer) || (activeLayer & 2) != 0) &&
                ((activeLayer & 2) == 0 ||
                 !ctx.map().insideStructureMask(Collider::createCircle(d->pos, 1.0f)))) {
                layer |= 2;
            }
            ctx.renderer().addPIXIObj(d->trail, layer, 1000, 0);
            ctx.renderer().addPIXIObj(d->flare, layer, 1000, 1);
        }

        // flare.ts m_render: world-space placement (camera lives on the root).
        if (d->alive || d->collided) {
            placeInWorldLayer(d->container, d->pos);
            // flare.ts: both containers scale by camera.m_pixels(1) = m_zoom;
            // in world units that is 1/m_ppu, with the flare's own factor on
            // the sprite (trail keeps its relative length).
            const float containerScale = 1.0f / ctx.camera().m_ppu;
            d->container->setScale(containerScale, containerScale);
            d->flare->setScale(d->flareScale, d->flareScale);
            const float dist = v2Length(v2Sub(d->pos, d->startPos));
            const float trailLength = math::min(d->tracerLength * 15.0f, dist / 2.0f);
            d->trail->setPosition(0.0f, 0.0f);
            d->trail->setScale(trailLength * d->trailScale, 1.0f);
        }
    }
}
// ---------------------------------------------------------------------------
// Plane + airstrike zones (plane.ts)
// ---------------------------------------------------------------------------
void PlaneObj::init(GameWorld& ctx, const PlaneData& data, const Map& map) {
    id = data.id;
    pos = data.pos;
    planeDir = data.planeDir;
    actionComplete = data.actionComplete;
    active = true;

    alpha = 0.75f;
    renderAlpha = 1.0f;
    spriteUpdateTime = 0.0f;
    soundUpdateThrottle = 0.0f;
    soundPlaying = false;
    type = data.action;

    const DefProvider* provider = getDefProvider();
    const PlaneDef* def = provider ? provider->plane(type) : nullptr;
    config = def ? *def : PlaneDef{};

    rad = config.planeRad;

    // plane.ts picks the sprite + sound from the biome's airdrop block for an
    // airdrop plane, and the fixed fighter sprite for an airstrike.
    const std::string spriteName = type == 0
        ? (map.mapDef.airdropPlaneImg.empty() ? std::string("map-plane-01.img")
                                              : map.mapDef.airdropPlaneImg)
        : std::string("map-plane-02.img");
    planeSound = type == 0
        ? (map.mapDef.airdropPlaneSound.empty() ? std::string("plane_01")
                                                : map.mapDef.airdropPlaneSound)
        : std::string("fighter_01");

    if (!sprite) {
        sprite = ctx.factory()->createSprite();
        sprite->setAnchor(0.5f, 0.5f);
        sprite->setBlendMode(pix::BlendMode::Add);
    }
    sprite->setFrame(spriteName);
    sprite->setVisible(true);
    sprite->setRotation(std::atan2(planeDir.x, planeDir.y));
}
void PlaneBarn::updatePlanes(GameWorld& ctx, const std::vector<PlaneData>& data) {
    for (auto* p : planes) {
        p->dirty = true;
    }

    for (const auto& d : data) {
        PlaneObj* plane = nullptr;
        for (auto* p : planes) {
            if (p->active && p->id == d.id) {
                plane = p;
                break;
            }
        }
        if (!plane) {
            for (auto* p : planes) {
                if (!p->active) {
                    plane = p;
                    break;
                }
            }
            if (!plane) {
                plane = new PlaneObj();
                planes.push_back(plane);
            }
            plane->init(ctx, d, ctx.map());
        }
        plane->dirty = false;
        plane->actionComplete = d.actionComplete;

        // Reconcile if the server position is too far from the local guess.
        if (std::fabs(plane->pos.x - d.pos.x) > 8.0f ||
            std::fabs(plane->pos.y - d.pos.y) > 8.0f) {
            plane->pos = d.pos;
        }
    }

    // Delete planes absent from the update.
    for (auto* p : planes) {
        if (p->active && p->dirty) {
            p->active = false;
            if (p->sprite) {
                p->sprite->setVisible(false);
            }
        }
    }
}

void PlaneBarn::update(float dt, GameWorld& ctx) {
    if (!ctx.activePlayer()) {
        return;
    }
    const Player* activePlayer = ctx.activePlayer();
    const int activeLayer = activePlayer->layer;

    for (auto* p : planes) {
        if (!p->active) {
            continue;
        }
        int layer = 0;
        if ((sameGameLayer(0, activeLayer) || (activeLayer & 2) != 0) &&
            ((activeLayer & 2) == 0 ||
             !ctx.map().insideStructureMask(Collider::createCircle(p->pos, 1.0f)))) {
            layer |= 2;
        }

        p->pos = v2Add(p->pos, v2Mul(p->planeDir, dt * p->config.planeVel));

        if (p->actionComplete) {
            p->spriteUpdateTime = math::min(p->spriteUpdateTime + dt, 2.0f);
            p->rad = math::lerp(p->spriteUpdateTime, p->config.planeRad, p->config.planeRad * 1.25f);
            p->alpha = math::lerp(p->spriteUpdateTime, 0.75f, 0.75f * 0.75f);
            p->soundRangeMult =
                math::max(0.0f, math::lerp(p->spriteUpdateTime, p->config.soundRangeMult,
                                            p->config.soundRangeMult - p->config.soundRangeDelta));
        }

        ctx.renderer().addPIXIObj(p->sprite, layer, 1501, p->id);

        // plane.ts: camera.m_pixels(rad / m_ppu); world units add /m_ppu.
        const float planeScale = p->rad / (ctx.camera().m_ppu * ctx.camera().m_ppu);
        const bool indoors = ctx.map().insideBuildingCeiling(
            Collider::createCircle(activePlayer->pos, 0.01f), true);
        float alphaTarget = p->alpha;
        if (activeLayer == 1) {
            alphaTarget = 0.0f;
        } else if (indoors || (activeLayer & 1) != 0) {
            alphaTarget = 0.15f;
        }
        p->renderAlpha = math::lerp(dt * 3.0f, p->renderAlpha, alphaTarget);
        placeInWorldLayer(p->sprite, p->pos);
        p->sprite->setScale(planeScale, planeScale);
        p->sprite->setTint(0xffff00);
        p->sprite->setAlpha(p->renderAlpha);
        p->sprite->setVisible(true);
    }

    for (auto* zone : airstrikeZones) {
        if (zone->active) {
            zone->ticker += dt;
            if (zone->ticker >= zone->duration) {
                zone->active = false;
                if (zone->gfx) {
                    zone->gfx->setVisible(false);
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Shot (shot.ts)
// ---------------------------------------------------------------------------
void ShotBarn::addShot(const Bullet& data) {
    ShotObj* shot = nullptr;
    for (auto* s : shots) {
        if (!s->active) {
            shot = s;
            break;
        }
    }
    if (!shot) {
        shot = new ShotObj();
        shots.push_back(shot);
    }
    shot->active = true;
    shot->pos = data.pos;
    shot->layer = data.layer;
    shot->playerId = data.playerId;
    shot->weaponType = data.shotSourceType;
    shot->offhand = data.shotOffhand;
    shot->lastShot = data.lastShot;
    shot->shotAlt = data.shotAlt;
    shot->splinter = data.splinter;
    shot->trailSaturated = data.trailSaturated;
    shot->apRounds = data.apRounds;
    shot->highVelocity = data.highVelocity;
    shot->ticker = 0.0f;
    shot->pullDelay = 0.0f;
}

void ShotBarn::update(float dt, GameWorld& ctx) {
    const uint16_t activePlayerId = ctx.activePlayerId();
    const Player* activePlayer = ctx.activePlayer();
    for (auto* shot : shots) {
        if (!shot->active) {
            continue;
        }
        const GameObjRenderDef* def = gameDefFor(shot->weaponType);

        if (shot->ticker == 0.0f) {
            Player* player = ctx.playerBarn().getPlayerById(shot->playerId);

            // M6: per-shot weapon fire sound (port of shot.ts).
            // Only play the shot sound if there's a weapon def to look up.
            if (def && ctx.audio()) {
                // shot.ts: start with weaponDef.sound.shoot, then apply
                // team/last/alt overrides.
                // Since the C++ port doesn't have direct access to the full
                // GameObjectDef sound tables, we derive the sound name from
                // the weapon type — the sound defs use the type as a prefix
                // (e.g. "ak47_01.mp3" is registered as sound "ak47_01").
                // For now, we play via the 'players' channel group; the
                // channel is resolved from the player's identity.
                const std::string& channel =
                    shot->playerId == activePlayerId ? "activePlayer" : "otherPlayers";

                // Compute detune (shot.ts splinter/trailSaturated/logic).
                float detune = 0.0f;
                if (shot->trailSaturated && def->category == "gun") {
                    detune = 300.0f;
                }
                if (shot->splinter) {
                    detune = -300.0f;
                }

                // Build the sound name from the weapon type + "_01" suffix
                // (the soundDefs convention: each weapon has a _01 shoot sound).
                const std::string shootSound = shot->weaponType + "_01";

                audio::PlaySoundOptions opts;
                opts.channel = channel;
                opts.hasSoundPos = true;
                opts.soundPos = shot->pos;
                opts.hasLayer = true;
                opts.layer = shot->layer;
                opts.filter = "muffled";
                opts.detune = detune;
                ctx.audio()->playSound(shootSound, opts);

                // Splinter second layer (shot.ts lines 152-167).
                if (shot->splinter) {
                    audio::PlaySoundOptions splinterOpts = opts;
                    splinterOpts.detune = 1200.0f;
                    splinterOpts.delay = 30.0f / 1000.0f;
                    splinterOpts.volumeScale = 0.75f;
                    ctx.audio()->playSound(shootSound, splinterOpts);
                }

                // AP rounds layer (shot.ts lines 169-184).
                if (shot->apRounds) {
                    audio::PlaySoundOptions apOpts = opts;
                    apOpts.detune = 1000.0f;
                    apOpts.delay = 45.0f / 1000.0f;
                    apOpts.volumeScale = 0.75f;
                    ctx.audio()->playSound(shootSound, apOpts);
                }

                // High-velocity layer (shot.ts lines 186-200).
                if (shot->highVelocity) {
                    audio::PlaySoundOptions hvOpts = opts;
                    hvOpts.detune = 800.0f;
                    hvOpts.delay = 10.0f / 1000.0f;
                    hvOpts.volumeScale = 2.0f;
                    hvOpts.filter = ""; // high-velocity has no muffled filter
                    ctx.audio()->playSound(shootSound, hvOpts);
                }
            }

            if (player) {
                // Hand + gun recoil.
                const bool leftHand = shot->offhand || !(def && def->isDual);
                const bool rightHand = !shot->offhand || !(def && def->isDual);
                if (def && def->category == "gun") {
                    if (leftHand) player->gunRecoil[0] += def->recoil;
                    if (rightHand) player->gunRecoil[1] += def->recoil;
                }
            }
        }

        shot->ticker += dt;
        if (shot->ticker >= shot->pullDelay) {
            shot->active = false;
        }
    }
}

// ---------------------------------------------------------------------------
// ParticleBarn (particles.ts)
// ---------------------------------------------------------------------------
static Vec2 randomPointInCircle(float rad) {
    float a = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    float b = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    if (b < a) {
        const float c = a;
        a = b;
        b = c;
    }
    const float angle = (2.0f * 3.14159265358979f * a) / b;
    return Vec2(b * rad * std::cos(angle), b * rad * std::sin(angle));
}

void Particle::m_init(pix::Factory* factory, const std::string& type, int layer_, const Vec2& pos_,
                      const Vec2& vel_, float scaleParam, float rot_, pix::Node* parent, int zOrd_,
                      float valueAdjust_) {
    const DefProvider* provider = getDefProvider();
    def = provider ? provider->particle(type) : nullptr;
    active = true;
    ticker = 0.0f;
    if (!sprite) {
        sprite = factory->createSprite();
        sprite->setAnchor(0.5f, 0.5f);
        sprite->setScale(1.0f, 1.0f);
    }
    if (parent) {
        hasParent = true;
        parent->addChild(sprite);
    } else {
        hasParent = false;
        sprite->removeFromParent();
    }
    pos = pos_;
    vel = vel_;
    rot = rot_;
    delay = 0.0f;
    emitterIdx = -1;
    layer = layer_;
    zOrd = zOrd_ != -1 ? zOrd_ : (def ? def->zOrd : 20);
    valueAdjust = valueAdjust_;

    if (!def) {
        // Unknown particle type: don't leave the slot active forever.
        active = false;
        sprite->setVisible(false);
        return;
    }
    life = def->life.random();
    drag = def->drag.random();
    rotVel = def->rotVel.random() * ((std::rand() % 2 == 0) ? -1.0f : 1.0f);
    rotDrag = def->drag.random() / 2.0f;
    scaleUseExp = def->scaleUseExp;
    scale = def->scaleStart.random() * scaleParam;
    scaleEnd = scaleUseExp ? 0.0f : def->scaleEnd.random() * scaleParam;
    scaleExp = scaleUseExp ? def->scaleExp : 0.0f;
    alphaUseExp = def->alphaUseExp;
    alpha = def->alphaStart;
    alphaEnd = alphaUseExp ? 0.0f : def->alphaEnd;
    alphaExp = alphaUseExp ? def->alphaExp : 0.0f;
    alphaIn = def->hasAlphaIn;
    alphaInStart = alphaIn ? def->alphaInStart : 0.0f;
    alphaInEnd = alphaIn ? def->alphaInEnd : 0.0f;

    if (!def->images.empty()) {
        const size_t idx = def->images.size() == 1 ? 0 : static_cast<size_t>(std::rand()) % def->images.size();
        sprite->setFrame(def->images[idx]);
    }
    sprite->setVisible(false);
    if (def->hasColor) {
        setColor(def->color);
    } else {
        sprite->setTint(0xffffff);
    }
}

void Particle::m_free() {
    active = false;
    if (sprite) {
        sprite->setVisible(false);
    }
}

void Particle::setColor(uint32_t color) {
    if (valueAdjust < 1.0f) {
        color = adjustValue(color, valueAdjust);
    }
    if (sprite) {
        sprite->setTint(color);
    }
}

Emitter* ParticleBarn::addEmitter(pix::Factory* factory, const std::string& type,
                                  const EmitterOptions& opts) {
    (void)factory;
    Emitter* e = nullptr;
    for (auto* existing : emitters) {
        if (!existing->active) {
            e = existing;
            break;
        }
    }
    if (!e) {
        e = new Emitter();
        emitters.push_back(e);
    }
    e->m_init(type, opts);
    return e;
}

void Emitter::m_init(const std::string& type_, const EmitterOptions& opts) {
    const DefProvider* provider = getDefProvider();
    def = provider ? provider->emitter(type_) : nullptr;
    active = true;
    enabled = true;
    type = type_;
    pos = opts.pos;
    dir = opts.dir;
    scale = opts.scale;
    layer = opts.layer;
    duration = opts.duration;
    radius = opts.radius >= 0.0f ? opts.radius : (def ? def->radius : 0.0f);
    ticker = 0.0f;
    nextSpawn = 0.0f;
    spawnCount = 0.0f;
    parent = opts.parent;
    alpha = 1.0f;
    rateMult = opts.rateMult;
    hasColor = opts.hasColor;
    color = opts.color;

    int zOrd_ = 20;
    if (def) {
        if (def->hasZOrd) {
            zOrd_ = def->zOrd;
        } else {
            const ParticleDef* pd = provider ? provider->particle(def->particle) : nullptr;
            if (pd) {
                zOrd_ = pd->zOrd;
            }
        }
    }
    zOrd = zOrd_;
}

void Emitter::m_free() {
    active = false;
}

Particle* ParticleBarn::addParticle(pix::Factory* factory, const std::string& type, int layer,
                                    const Vec2& pos, const Vec2& vel, float scale, float rot,
                                    pix::Node* parent, int zOrd) {
    Particle* p = nullptr;
    for (auto* existing : particles) {
        if (!existing->active) {
            p = existing;
            break;
        }
    }
    if (!p) {
        p = new Particle();
        particles.push_back(p);
    }
    const float r = rot >= 0.0f
                        ? rot
                        : (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) *
                              3.14159265358979f * 2.0f;
    int z = zOrd;
    if (z < 0) {
        const DefProvider* provider = getDefProvider();
        const ParticleDef* pd = provider ? provider->particle(type) : nullptr;
        z = (pd && pd->zOrd != 0) ? pd->zOrd : 20;
    }
    p->m_init(factory, type, layer, pos, vel, scale, r, parent, z, valueAdjust);
    return p;
}

Particle* ParticleBarn::addRippleParticle(pix::Factory* factory, const Vec2& pos, int layer,
                                          uint32_t color) {
    Particle* p = addParticle(factory, "waterRipple", layer, pos, Vec2(0.0f, 0.0f), 1.0f, 0.0f,
                              nullptr, -1);
    p->setColor(color);
    return p;
}

void ParticleBarn::update(float dt, GameWorld& ctx) {
    (void)ctx;

    // Update emitters.
    for (size_t i = 0; i < emitters.size(); i++) {
        Emitter* e = emitters[i];
        if (!e->active || !e->enabled || !e->def) {
            continue;
        }
        e->ticker += dt;
        e->nextSpawn -= dt;
        const EmitterDef* def = e->def;
        while (e->nextSpawn <= 0.0f && e->spawnCount < def->maxCount) {
            const float rad = e->scale * e->radius;
            const Vec2 pos = v2Add(e->pos, randomPointInCircle(rad));
            const float jitter = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) -
                                  0.5f) *
                                 def->angle;
            const Vec2 dir = v2Rotate(e->dir, jitter);
            const Vec2 vel = v2Mul(dir, def->speed.random());
            const float rot = def->hasRot ? def->rot.random() : -1.0f;
            Particle* particle = addParticle(ctx.factory(), def->particle, e->layer, pos, vel, e->scale,
                                             rot, e->parent, e->zOrd);
            if (e->hasColor) {
                particle->setColor(e->color);
            }
            particle->emitterIdx = static_cast<int>(i);
            float rate = def->rate.random();
            if (def->hasMaxRate) {
                const float w = math::easeInExpo(
                    math::min(1.0f, def->maxElapsed > 0.0f ? e->ticker / def->maxElapsed : 1.0f));
                const float maxRate = def->maxRate.random();
                rate = math::lerp(w, rate, maxRate);
            }
            e->nextSpawn += rate * e->rateMult;
            e->spawnCount += 1.0f;
            // Defensive: a non-positive step would spin forever.
            if (rate * e->rateMult <= 0.0f) {
                break;
            }
        }
        if (e->ticker >= e->duration) {
            e->m_free();
        }
    }

    // Update particles.
    for (size_t i = 0; i < particles.size(); i++) {
        Particle* p = particles[i];
        if (!p->active) {
            continue;
        }
        p->ticker += dt;
        if (p->ticker < p->delay || !p->def) {
            continue;
        }
        const ParticleDef* def = p->def;
        const float t = math::min((p->ticker - p->delay) / p->life, 1.0f);
        p->vel = v2Mul(p->vel, 1.0f / (1.0f + dt * p->drag));
        p->pos = v2Add(p->pos, v2Mul(p->vel, dt));
        p->rotVel *= 1.0f / (1.0f + dt * p->rotDrag);
        p->rot += p->rotVel * dt;
        if (p->scaleUseExp) {
            p->scale += dt * p->scaleExp;
        }
        if (p->alphaUseExp) {
            p->alpha = math::max(p->alpha + dt * p->alphaExp, 0.0f);
        }
        const Vec2 particlePos = p->pos;
        float scale = p->scaleUseExp
                          ? p->scale
                          : math::remap(t, def->scaleLerp.min, def->scaleLerp.max, p->scale,
                                        p->scaleEnd);
        float alpha = p->alphaUseExp
                          ? p->alpha
                          : math::remap(t, def->alphaLerp.min, def->alphaLerp.max, p->alpha,
                                        p->alphaEnd);
        if (p->alphaIn && t < def->alphaInLerp.max) {
            alpha = math::remap(t, def->alphaInLerp.min, def->alphaInLerp.max, p->alphaInStart,
                                p->alphaInEnd);
        }
        if (p->emitterIdx >= 0 && p->emitterIdx < static_cast<int>(emitters.size())) {
            alpha *= emitters[p->emitterIdx]->alpha;
        }
        if (!p->hasParent) {
            ctx.renderer().addPIXIObj(p->sprite, p->layer, p->zOrd);
        }
        // particles.ts renders in world space; the world root scales it.
        p->sprite->setPosition(particlePos.x, particlePos.y);
        // particles.ts: camera.m_pixels(scale) = scale * m_zoom; the world root
        // already applies m_ppu * m_zoom, so the sprite needs scale / m_ppu.
        const float worldScale = scale / ctx.camera().m_ppu;
        p->sprite->setScale(worldScale, worldScale);
        p->sprite->setRotation(p->rot);
        p->sprite->setAlpha(alpha);
        p->sprite->setVisible(true);

        if (t >= 1.0f) {
            p->m_free();
        }
    }
}

void ParticleBarn::clear() {
    for (auto* emitter : emitters) {
        emitter->m_free();
    }
    for (auto* particle : particles) {
        particle->m_free();
    }
    // particles.ts re-allocates these lazily; dropping the containers keeps the
    // next match from stepping a previous match's (now inactive) objects.
    emitters.clear();
    particles.clear();
}

} // namespace surv
