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

// AABB of a map-object def collider (obstacle.ts uses collider.toAabb).
static Collider colliderToAabbDef(const Collider& c) {
    if (c.type == Collider::Circle) {
        return Collider::createAabb(Vec2(c.pos.x - c.rad, c.pos.y - c.rad),
                                    Vec2(c.pos.x + c.rad, c.pos.y + c.rad));
    }
    return Collider::createAabb(c.min, c.max);
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

    const Vec2 screenPos = ctx.camera().m_pointToScreen(renderPos);
    const float screenScale = ctx.camera().m_pixels(scale * imgScale);
    // obstacle.ts renders the sprite directly at the object screen position;
    // the sprite's own anchor (0.5 or the door's def.door.spriteAnchor) is the
    // pivot, applied by applyFrame.
    sprite->setPosition(screenPos.x, screenPos.y);
    float sx = screenScale;
    float sy = screenScale;
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
        const Vec2 casingScreen = ctx.camera().m_pointToScreen(casingWorld);
        const float casingScale = ctx.camera().m_pixels(scale * casingImgScale);
        casingSprite->setPosition(casingScreen.x, casingScreen.y);
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
        const Vec2 screenPos = ctx.camera().m_pointToScreen(v2Add(pos, img.posOffset));
        const float screenScale = ctx.camera().m_pixels(scale * img.defScale);
        img.sprite->setPosition(screenPos.x, screenPos.y);
        float sx = screenScale;
        float sy = screenScale;
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
    const Vec2 screenPos = camera.m_pointToScreen(renderPos);
    const float screenScale = camera.m_pixels(imgScale * scale);
    container->setPosition(screenPos.x, screenPos.y);
    container->setScale(screenScale, screenScale);
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
    const Vec2 screenPos = camera.m_pointToScreen(pos);
    const float screenScale = camera.m_pixels(scale * imgScale);
    sprite->setPosition(screenPos.x, screenPos.y);
    sprite->setScale(screenScale, screenScale);
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
    Camera& camera = ctx.camera();

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
        const Vec2 screenPos = p->hasParent ? p->pos : camera.m_pointToScreen(p->pos);
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
            scale = camera.m_pixels(scale);
            ctx.renderer().addPIXIObj(p->sprite, p->layer, p->zOrd);
        }
        p->sprite->setPosition(screenPos.x, screenPos.y);
        p->sprite->setScale(scale, scale);
        p->sprite->setRotation(p->rot);
        p->sprite->setAlpha(alpha);
        p->sprite->setVisible(true);

        if (t >= 1.0f) {
            p->m_free();
        }
    }
}

} // namespace surv
