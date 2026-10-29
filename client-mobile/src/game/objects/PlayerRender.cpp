#include "Barns.h"
#include "ObstacleScan.h"
#include "../GameWorld.h"
#include "../Map.h"
#include "../../render/Renderer.h"
#include "../../audio/AudioManager.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace surv {
namespace {
constexpr float pi = 3.14159265358979f;
const GameObjRenderDef* item(const std::string& type) {
    auto* defs = getDefProvider();
    return defs ? defs->gameObject(type) : nullptr;
}
const MapObjectDef* providerDef(const std::string& type) {
    auto* defs = getDefProvider();
    return defs ? defs->mapObject(type) : nullptr;
}

// shared/utils/util.sameLayer.
bool sameLayer(int a, int b) {
    return ((a & 0x1) == (b & 0x1)) || (((a & 0x2) != 0) && ((b & 0x2) != 0));
}
float randf() { return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX); }
// util.random(min, max).
float randRange(float a, float b) { return math::lerp(randf(), a, b); }

BonePose blend(float t, const BonePose& a, const BonePose& b) {
    return {v2Lerp(t, a.pivot, b.pivot), math::lerp(t, a.rot, b.rot), v2Lerp(t, a.pos, b.pos)};
}
float ease(float t, PoseEasing easing) {
    switch (easing) {
        case PoseEasing::InSine: return 1.0f - std::cos(t * pi / 2.0f);
        case PoseEasing::OutSine: return std::sin(t * pi / 2.0f);
        case PoseEasing::InOutSine: return (1.0f - std::cos(pi * t)) / 2.0f;
        case PoseEasing::OutQuart: return 1.0f - std::pow(1.0f - t, 4.0f);
        case PoseEasing::OutQuad: return 1.0f - (1.0f - t) * (1.0f - t);
        case PoseEasing::OutBounce:
            if (t < 1.0f / 2.75f) return 7.5625f * t * t;
            if (t < 2.0f / 2.75f) { t -= 1.5f / 2.75f; return 7.5625f * t * t + 0.75f; }
            if (t < 2.5f / 2.75f) { t -= 2.25f / 2.75f; return 7.5625f * t * t + 0.9375f; }
            t -= 2.625f / 2.75f; return 7.5625f * t * t + 0.984375f;
        default: return t;
    }
}
void skinSprite(pix::Sprite* sprite, const std::string& frame, uint32_t tint, float scale,
                bool visible = true) {
    const bool valid = !frame.empty() && frame != "none";
    if (valid) sprite->setFrame(frame);
    sprite->setTint(tint);
    sprite->setScale(scale, scale);
    sprite->setVisible(valid && visible);
}
// player.ts stores every player-local offset (bone poses, gun/mag/melee
// positions, held-item positions) in PIXI's y-down local frame. The native
// player container lives in the y-up world frame, so local y must be negated
// before use. Rotations stay numeric: PIXI and axmol both rotate clockwise
// for positive values.
Vec2 localFromWeb(const Vec2& v) {
    return Vec2(v.x, -v.y);
}

void heldSprite(pix::Sprite* sprite, const HeldImageDef& def) {
    skinSprite(sprite, def.sprite, def.tint, 1.0f);
    const Vec2 pos = localFromWeb(def.pos);
    sprite->setPosition(pos.x, pos.y);
    sprite->setScale(def.scale.x, def.scale.y);
    sprite->setRotation(def.rot);
}
} // namespace

void Player::m_init() {
    dead = downed = false;
    // player.ts m_init(): interpolation state starts at the origin and is
    // seeded from the first net update (isNew) so a respawn never slides in
    // from the previous life's position.
    visualPos = Vec2();
    visualPosOld = Vec2();
    visualDir = Vec2();
    visualDirOld = Vec2();
    posInterpTicker = 0.0f;
    dirInterpolationTicker = 0.0f;
    animType = currentAnim = Anim_None;
    animSeq = -1;
    animTicker = 0.0f;
    animMask = 0;
    bones = {};
    animBones = {};
    animation.clear();
    throwableState = 0;
    visualsDirty = true;
    scale = 1.0f;
    // player.ts constructor seeds m_bodyRad from m_rad; net updates ease it.
    rad = PlayerConfig::radius;
    bodyRad = PlayerConfig::radius;
    teamId = 0;
    gunRecoil = {};
    healEffect = false;
    lastStandEffect = false;
    hasteType = HasteType_None;
    hasteSeq = -1;
    healEmitter = hasteEmitter = adrenalineEmitter = nullptr;
    auraVisible = false;
    paramsCached = false;
    cachedActionType = -1;
    cachedActionItem.clear();
    auraSprite.clear();
    auraTint = 0xff00ff;
    auraRadius = 0.0f;
    auraViewFade = 0.0f;
    auraPulseTicker = 0.0f;
    auraPulseDir = 1.0f;
    updateFrozenImage = true;
    frozenTicker = 0.0f;
    frozenActive = false;
    submersion = 0.0f;
    zoom = 0.0f;
    zoomFast = false;
    surfaceType = SurfaceType::Grass;
    stepDistance = 0.0f;
    wasInWater = false;
    cycleSoundInstance = audio::kInvalidSound;
    actionSoundInstance = audio::kInvalidSound;
    lastInsideObstacleTime = 0.0f;
    wasInsideObstacle = false;
    insideObstacleType.clear();
    bleedTicker = 0.0f;
    isNearDoorError = false;
    doorErrorTicker = 0.0f;
    noCeilingRevealTicker = 0.0f;
    playedDryFire = false;
    _prevActiveWeapon.clear();
}

void Player::m_free() {
    if (container) container->setVisible(false);
    if (healEmitter) healEmitter->stop();
    if (hasteEmitter) hasteEmitter->stop();
    if (adrenalineEmitter) adrenalineEmitter->stop();
    healEmitter = hasteEmitter = adrenalineEmitter = nullptr;
    auraVisible = false;
}

void Player::m_updateData(const ObjectData& data, bool fullUpdate, bool isNew, Ctx& ctx) {
    // player.ts m_updateData: remember where the visual position started before
    // the net position moves. A duplicated update keeps the current ticker.
    if (!v2Eq(data.pos, visualPosOld)) {
        visualPosOld = isNew ? data.pos : pos;
        posInterpTicker = 0.0f;
    }
    if (!v2Eq(data.dir, visualDirOld)) {
        visualDirOld = isNew ? data.dir : dir;
        dirInterpolationTicker = 0.0f;
    }
    pos = data.pos;
    dir = data.dir;
    if (!fullUpdate) return; // partial updates must not erase gear/animation state
    layer = data.layer;
    outfit = data.outfit;
    backpack = data.backpack;
    helmet = data.helmet;
    chest = data.chest;
    role = data.role;
    activeWeapon = data.activeWeapon;
    dead = data.dead;
    downed = data.downed;
    scale = data.scale;
    wearingPan = data.wearingPan;
    healEffect = data.healEffect;
    lastStandEffect = data.lastStandEffect;
    // player.ts: zoomDirty assigns m_localData.m_zoom and clears zoomFast so the
    // next scope change lerps in quickly.
    if (data.zoomDirty) {
        zoom = static_cast<float>(data.zoom);
        zoomFast = false;
    }
    // player.ts: m_rad = netData.m_scale * GameConfig.player.radius.
    rad = data.scale * PlayerConfig::radius;
    if (isNew) {
        // player.ts snaps m_bodyRad on the first update (isNew forces the full
        // step) so a respawn never visibly grows/shrinks into the new size.
        bodyRad = rad;
    }
    if ((hasteType != data.hasteType || hasteSeq != data.hasteSeq) && hasteEmitter) {
        hasteEmitter->stop();
        hasteEmitter = nullptr;
    }
    hasteType = data.hasteType;
    hasteSeq = data.hasteSeq;

    // Priority 1: play haste ability sound when haste seq changes (player.ts ~line 1122)
    if (hasteType != HasteType_None && hasteSeq >= 0 && !isNew && ctx.audio()) {
        audio::PlaySoundOptions opts;
        opts.channel = "sfx";
        opts.hasSoundPos = true;
        opts.soundPos = pos;
        opts.fallOff = 1.0f;
        opts.hasLayer = true;
        opts.layer = layer;
        opts.filter = "muffled";
        ctx.audio()->playSound("ability_stim_01", opts);
    }

    perks = data.perks;
    animType = data.animType;
    actionType = data.actionType;
    actionItem = data.actionItem;
    frozen = data.frozen;
    frozenActive = data.frozen;
    frozenOri = data.frozenOri;
    frozenType = data.frozenType;
    // player.ts updateFrozenState: refresh the patch image when it (re)froze.
    if (!frozen) {
        updateFrozenImage = true;
    } else if (frozen && updateFrozenImage && !frozenType.empty()) {
        const GameObjRenderDef* frozenDef = item(frozenType);
        if (frozenDef && !frozenDef->frozenSprites.empty()) {
            const std::string& spriteName =
                frozenDef->frozenSprites[std::rand() % frozenDef->frozenSprites.size()];
            if (bodyEffectSprite) {
                bodyEffectSprite->setFrame(spriteName);
                bodyEffectSprite->setRotation(
                    math::oriToRad(frozenOri) + pi * 0.5f +
                    ((static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) - 0.5f) *
                        pi * 0.25f);
                bodyEffectSprite->setTint(0xffffff);
                bodyEffectSprite->setScale(0.25f, 0.25f);
            }
        }
        updateFrozenImage = false;
    }
    visualsDirty = true;

    if (!container) {
        auto* factory = ctx.factory();
        container = factory->createContainer();
        bodyContainer = factory->createContainer();
        container->addChild(bodyContainer);
        auto sprite = [&](pix::Node* parent) {
            auto* s = factory->createSprite();
            s->setAnchor(0.5f, 0.5f);
            s->setVisible(false);
            parent->addChild(s);
            return s;
        };
        for (int i = 0; i < 4; ++i) {
            boneContainers[i] = factory->createContainer();
            limbSprites[i] = sprite(boneContainers[i]);
        }
        bodyContainer->addChild(boneContainers[2]);
        bodyContainer->addChild(boneContainers[3]);
        backpackSprite = sprite(bodyContainer);
        bodySprite = sprite(bodyContainer);
        chestSprite = sprite(bodyContainer);
        flakSprite = sprite(bodyContainer);
        steelskinSprite = sprite(bodyContainer);
        hipSprite = sprite(bodyContainer);
        bodyContainer->addChild(boneContainers[0]);
        bodyContainer->addChild(boneContainers[1]);
        visorSprite = sprite(bodyContainer);
        helmetSprite = sprite(bodyContainer);
        for (int i = 0; i < 2; ++i) {
            gunContainers[i] = factory->createContainer();
            gunSprites[i] = sprite(gunContainers[i]);
            magSprites[i] = sprite(gunContainers[i]);
            boneContainers[i]->addChildAt(gunContainers[i], 0);
            objectSprites[i] = sprite(boneContainers[i]);
        }
        meleeSprite = sprite(boneContainers[1]);
        // player.ts faction patch (rendered on the body in faction modes).
        patchSprite = sprite(bodyContainer);
        patchSprite->setAnchor(0.5f, 0.5f);
        patchSprite->setVisible(false);
        // player.ts: a frozen/body-effect sprite, a submerge body sprite (with
        // its own limb sprites) and the aura container + circle.
        bodyEffectSprite = sprite(bodyContainer);
        bodySubmergeSprite = sprite(bodyContainer);
        for (int i = 0; i < 4; ++i) {
            submergeLimbs[i] = sprite(boneContainers[i]);
            submergeLimbs[i]->setVisible(false);
        }
        bodySubmergeSprite->setVisible(false);
        bodyEffectSprite->setVisible(false);
        auraContainer = factory->createContainer();
        auraCircle = sprite(auraContainer);
        auraCircle->setScale(0.125f, 0.125f);
        auraContainer->setVisible(false);
        nameText = factory->createText();
        nameText->setAnchor(0.5f, 0.5f);
        nameText->setScale(0.5f, 0.5f);
        nameText->setColor(0xffffff, 0x000000, 1.0f, true);
        // World text is y-up like the camera root now (no counter-flip).
        nameText->setFlipY(false);
        nameText->setVisible(false);
        container->addChild(nameText);
    }
    if (isNew || data.animSeq != animSeq) playAnim(data.animType, data.animSeq);
    updateActions(data.actionType, data.actionItem, ctx);
}

void Player::playAnim(int type, int seq) {
    currentAnim = type;
    animSeq = seq;
    animTicker = 0.0f;
    animMask = 0;
    animBones = bones;
    animMirror = false;
    const auto* def = item(activeWeapon);
    const std::vector<std::string>* choices = nullptr;
    switch (type) {
        case Anim_Cook: animation = "cook"; break;
        case Anim_Throw: animation = "throw"; break;
        case Anim_Revive: animation = "revive"; break;
        case Anim_CrawlForward: animation = "crawl_forward"; animMirror = true; break;
        case Anim_CrawlBackward: animation = "crawl_backward"; animMirror = true; break;
        case Anim_Melee: if (def) choices = &def->attackAnims; break;
        case Anim_DeployMelee: if (def) choices = &def->deployAnims; break;
        case Anim_IdleMelee: if (def) choices = &def->idleAnims; break;
        default: animation = "none"; currentAnim = Anim_None; break;
    }
    if (type == Anim_Melee || type == Anim_DeployMelee || type == Anim_IdleMelee) {
        animation = choices && !choices->empty() ? (*choices)[std::rand() % choices->size()] : "fists";
        animMirror = animation == "fists" && (!choices || choices->size() <= 1);
    }
    animMirror = animMirror && std::rand() % 2 == 0;
    visualsDirty = true;
}

void Player::updateVisuals(Ctx& ctx) {
    const auto* outfitDef = item(outfit);
    if (!outfitDef || outfitDef->skin.baseSprite.empty()) outfitDef = item("outfitBase");
    if (!outfitDef) return;
    const auto& skin = outfitDef->skin;
    const bool ghillie = outfitDef->ghillie;
    // player.ts updateVisuals: bodyScale = m_bodyRad / GameConfig.player.radius.
    // Eased so scale pickups grow the body (and shrink guns/melee to fit).
    const float bodyScale = bodyRad / PlayerConfig::radius;
    const uint32_t ghillieTint = ctx.map().mapDef.colors.playerGhillie;
    skinSprite(bodySprite, skin.baseSprite, ghillie ? ghillieTint : skin.baseTint, 0.25f);
    skinSprite(limbSprites[0], skin.handL, ghillie ? ghillieTint : skin.handTint, 0.175f);
    skinSprite(limbSprites[1], skin.handR, ghillie ? ghillieTint : skin.handTint, 0.175f);
    for (int i = 2; i < 4; ++i) {
        skinSprite(limbSprites[i], skin.footSprite, ghillie ? ghillieTint : skin.footTint, 0.45f, downed);
        limbSprites[i]->setRotation(pi / 2.0f);
    }
    const auto* bag = item(backpack);
    const int bagLevel = bag ? bag->level : 0;
    skinSprite(backpackSprite, skin.backpackSprite, skin.backpackTint,
               (0.4f + bagLevel * 0.03f) * 0.5f, bagLevel > 0 && !ghillie && !downed);
    const float bagOffsets[] = {10.25f, 11.5f, 12.75f};
    backpackSprite->setPosition(-bagOffsets[std::clamp(bagLevel - 1, 0, 2)], 0.0f);
    auto armor = [&](pix::Sprite* s, const std::string& type, bool isHelmet) {
        const auto* d = item(type);
        if (d && !ghillie) {
            uint32_t tint = d->skin.baseTint;
            if (isHelmet && ctx.map().mapDef.factionMode) tint = teamId == 1 ? d->skin.baseTintRed : d->skin.baseTintBlue;
            skinSprite(s, d->skin.baseSprite, tint, isHelmet ? d->skin.spriteScale : 0.25f);
        } else s->setVisible(false);
    };
    armor(chestSprite, chest, false);
    armor(helmetSprite, helmet, true);
    helmetSprite->setPosition(downed ? 3.33f : -3.33f, 0.0f);
    const auto* roleDef = item(role);
    if (roleDef && !helmet.empty() && !ghillie) {
        skinSprite(visorSprite, roleDef->visor.baseSprite, roleDef->visor.baseTint, roleDef->visor.spriteScale);
    } else visorSprite->setVisible(false);
    visorSprite->setPosition(downed ? 3.33f : -3.33f, 0.0f);
    auto hasPerk = [&](const char* type) {
        return std::any_of(perks.begin(), perks.end(), [&](const auto& p) { return p.type == type; });
    };
    skinSprite(flakSprite, "player-armor-base-01.img", 3671558, 0.215f, hasPerk("flak_jacket") && !ghillie);
    flakSprite->setAlpha(0.7f);
    skinSprite(steelskinSprite, "loot-melee-pan-black.img", 0xffffff, 0.4f, hasPerk("steelskin") && !ghillie);
    steelskinSprite->setAnchor(0.575f, 0.5f);
    if (patchSprite) {
        if (ctx.map().mapDef.factionMode && !ghillie) {
            const bool potato = ctx.map().mapDef.potatoMode;
            const std::string frames[2] = {
                potato ? "player-patch-01po.img" : "player-patch-01.img",
                potato ? "player-patch-02po.img" : "player-patch-02.img",
            };
            const int teamIdx = std::abs(static_cast<int>(teamId) - 1) % 2;
            patchSprite->setFrame(frames[teamIdx]);
            patchSprite->setRotation(-(math::deg2rad(3.0f) + pi * 0.5f));
            patchSprite->setScale(0.25f, 0.25f);
            patchSprite->setTint(potato ? 0xffffff : (teamIdx == 0 ? 0xcc0000u : 0x7effu));
            patchSprite->setVisible(true);
        } else {
            patchSprite->setVisible(false);
        }
    }
    const auto* pan = item("pan");
    if (wearingPan && pan) heldSprite(hipSprite, pan->hipImg);
    else hipSprite->setVisible(false);

    const auto* weapon = item(activeWeapon);
    const bool hideWeapon = downed || currentAnim == Anim_Revive;
    const bool gun = weapon && weapon->category == "gun" && !hideWeapon;
    const bool melee = weapon && weapon->category == "melee" && activeWeapon != "fists" && !hideWeapon;
    for (int i = 0; i < 2; ++i) {
        gunContainers[i]->setVisible(gun && (i == 1 || weapon->isDual));
        objectSprites[i]->setVisible(false);
        if (gun) {
            skinSprite(gunSprites[i], weapon->worldImg.sprite, weapon->worldImg.tint, 1.0f);
            // player.ts uses the PIXI anchor (0.5, 1): bottom-centre / grip
            // end. Axmol's anchor origin is bottom-left, so the equivalent is
            // (0.5, 0); using 1 here put the grip at the muzzle and flipped
            // the gun 180 degrees around the hand.
            gunSprites[i]->setAnchor(0.5f, 0.0f);
            gunSprites[i]->setScale(weapon->worldImg.scale.x * 0.5f / bodyScale, weapon->worldImg.scale.y * 0.5f / bodyScale);
            gunContainers[i]->setRotation(pi / 2.0f);
            const Vec2 gunPos = localFromWeb(Vec2((weapon->isDual ? -5.95f : -4.25f) + weapon->gunOffset.x,
                                                  (weapon->isDual ? 0.0f : -1.75f) + weapon->gunOffset.y));
            gunContainers[i]->setPosition(gunPos.x, gunPos.y);
            skinSprite(magSprites[i], weapon->magSprite, 0xffffff, 0.25f / bodyScale);
            const Vec2 magPos = localFromWeb(Vec2(
                weapon->magPos.x / bodyScale, weapon->magPos.y / bodyScale));
            magSprites[i]->setPosition(magPos.x, magPos.y);
            gunContainers[i]->setChildIndex(magSprites[i], weapon->magTop ? 1 : 0);
            boneContainers[i]->setChildIndex(gunContainers[i], weapon->worldImg.handsBelow ? 2 : 0);
        }
        if (weapon && weapon->category == "throwable" && !hideWeapon) {
            heldSprite(objectSprites[i], weapon->handImgs[throwableState][i]);
            objectSprites[i]->setRotation(pi / 2.0f);
        }
    }
    if (melee) {
        heldSprite(meleeSprite, weapon->worldImg);
        boneContainers[1]->setChildIndex(meleeSprite, weapon->worldImg.renderOnHand ? 2 : 0);
    } else meleeSprite->setVisible(false);
    // Restore a deterministic child order on every gear/downed transition,
    // including pool reuse. addChild reparents rather than duplicating nodes.
    const bool leftOnTop = gun ? !(weapon->magTop || weapon->worldImg.handsBelow)
                              : melee && weapon->worldImg.leftHandOnTop;
    auto hands = [&]() {
        bodyContainer->addChild(boneContainers[leftOnTop ? 1 : 0]);
        bodyContainer->addChild(boneContainers[leftOnTop ? 0 : 1]);
    };
    if (downed) hands();
    bodyContainer->addChild(boneContainers[2]);
    bodyContainer->addChild(boneContainers[3]);
    for (auto* s : {backpackSprite, bodySprite, chestSprite, flakSprite, steelskinSprite, hipSprite}) bodyContainer->addChild(s);
    if (!downed) hands();
    bodyContainer->addChild(visorSprite);
    bodyContainer->addChild(helmetSprite);
}

void Player::update(float dt, Ctx& ctx) {
    if (!container) return;
    dt = std::max(dt, 0.0f);
    // player.ts gives the stairs test a larger radius (GameConfig.player.maxVisualRadius).
    const float maxVisualRadius = PlayerConfig::maxVisualRadius;
    // player.ts: ease radius transitions (scale pickups / adrenaline).
    if (!math::eqAbs(rad, bodyRad)) {
        const float bodyRadDist = rad - bodyRad;
        float bodyRadStep = std::fabs(bodyRadDist) > 0.0001f ? bodyRadDist * dt * 6.0f : bodyRadDist;
        bodyRad += bodyRadStep;
        visualsDirty = true;
    }
    for (auto& recoil : gunRecoil) recoil = std::max(0.0f, recoil - recoil * dt * 5.0f - dt);
    scale = std::max(scale, 0.001f);
    const auto* provider = getDefProvider();
    const auto* weapon = item(activeWeapon);
    if ((currentAnim == Anim_Cook || currentAnim == Anim_Throw) && (!weapon || weapon->category != "throwable")) {
        playAnim(Anim_None, animSeq);
    }
    const auto* anim = provider ? provider->playerAnimation(animation) : nullptr;
    bool finished = false;
    if (currentAnim != Anim_None && anim && !anim->keyframes.empty()) {
        animTicker += dt;
        const auto& frames = anim->keyframes;
        size_t b = 0;
        while (b + 1 < frames.size() && animTicker >= frames[b].time) ++b;
        const size_t a = b > 0 ? b - 1 : 0;
        const float duration = frames[b].time - frames[a].time;
        const float t = duration > 0.0f ? std::clamp((animTicker - frames[a].time) / duration, 0.0f, 1.0f) : 1.0f;
        for (int i = 0; i < PlayerBoneCount; ++i) {
            const int source = animMirror ? (i ^ 1) : i;
            const unsigned bit = 1u << i;
            // player.ts builds a per-frame bone set: a bone that a keyframe omits
            // is drawn with the idle pose, not interpolated across the gap.
            if (frames[b].noMask & bit) {
                animMask &= ~bit;
                continue;
            }
            if ((frames[a].mask & frames[b].mask & (1u << source)) == 0) continue;
            animBones[i] = blend(ease(t, frames[b].easing), frames[a].bones[source], frames[b].bones[source]);
            if (animMirror) {
                animBones[i].pivot.y *= -1.0f;
                animBones[i].pos.y *= -1.0f;
                animBones[i].rot *= -1.0f;
            }
            animMask |= bit;
        }
        finished = animTicker >= frames.back().time;
        // player.ts: run animation effects whose time falls in (prevTicker,
        // ticker]; the final frame also processes the animation end.
        const float effectTicker = animTicker + (finished ? 1.0f : 0.0f);
        runAnimEffects(ctx, animTicker - dt, effectTicker, finished);
    } else if (currentAnim != Anim_None) playAnim(Anim_None, animSeq);
    const auto* idle = provider ? provider->playerPose(downed ? "downed" : weapon ? weapon->idlePose : "fists") : nullptr;
    if (!idle && provider) idle = provider->playerPose("fists");
    for (int i = 0; i < PlayerBoneCount; ++i) bones[i] = (animMask & (1u << i)) ? animBones[i] : idle ? (*idle)[i] : BonePose{};
    const int state = currentAnim == Anim_Throw ? 2 : currentAnim == Anim_Cook && animTicker >= 0.1f ? 1 : 0;
    if (state != throwableState) { throwableState = state; visualsDirty = true; }
    if (visualsDirty) { updateVisuals(ctx); visualsDirty = false; }
    for (int i = 0; i < 4; ++i) {
        // player.ts `e.position = pose.pos` with `e.pivot = -pose.pivot`
        // (PIXI y-down). Convert the local positions to the y-up world frame;
        // the pivot rotation is the same clockwise rotation PIXI uses, so the
        // offset is pos + R(clockwise, rot) * pivot.
        const Vec2 posePos = localFromWeb(bones[i].pos);
        const Vec2 posePivot = localFromWeb(bones[i].pivot);
        Vec2 offset = v2Add(posePos, v2Rotate(posePivot, -bones[i].rot));
        if (i == 0 && weapon && weapon->category == "gun" && !downed && currentAnim != Anim_Revive) {
            offset = v2Add(offset, localFromWeb(weapon->leftHandOffset));
        }
        if (i < 2) offset.x -= gunRecoil[i] * 1.125f;
        boneContainers[i]->setPosition(offset.x, offset.y);
        boneContainers[i]->setRotation(bones[i].rot);
    }
    if (weapon && weapon->category == "melee") {
        const auto& bone = bones[5];
        const auto& img = weapon->worldImg;
        const float bodyScale = bodyRad / PlayerConfig::radius;
        const float r = img.rot + bone.rot;
        // player.ts: meleeSprite.position = -MeleeR.pivot and
        // meleeSprite.pivot = -(MeleeR.pos + worldImg.pos), both in the
        // hand's PIXI-y-down local frame.
        const Vec2 webPos = v2Add(bone.pos, img.pos);
        const Vec2 spritePos = localFromWeb(Vec2(-bone.pivot.x, -bone.pivot.y));
        const Vec2 spritePivot = localFromWeb(Vec2(-webPos.x, -webPos.y));
        const Vec2 offset = v2Add(spritePos, v2Rotate(spritePivot, -r));
        meleeSprite->setPosition(offset.x, offset.y);
        meleeSprite->setRotation(r);
        meleeSprite->setScale(img.scale.x / bodyScale, img.scale.y / bodyScale);
    }
    bodyContainer->setScale(bodyRad / PlayerConfig::radius, bodyRad / PlayerConfig::radius);
    // player.ts update(): interpolate the render position/direction between the
    // previous and the current net position over camera.m_interpInterval. The
    // camera follows m_visualPos, so this is what makes 33 Hz motion smooth.
    if (ctx.camera().m_interpEnabled) {
        posInterpTicker += dt;
        const float interval = ctx.camera().m_interpInterval;
        const float posT = interval > 0.0f ? std::clamp(posInterpTicker / interval, 0.0f, 1.0f) : 1.0f;
        visualPos = v2Lerp(posT, visualPosOld, pos);
        dirInterpolationTicker += dt;
        const float dirT = interval > 0.0f ? std::clamp(dirInterpolationTicker / interval, 0.0f, 1.0f) : 1.0f;
        visualDir = v2Lerp(dirT, visualDirOld, dir);
    } else {
        visualPos = pos;
        visualDir = dir;
    }
    bodyContainer->setRotation(-std::atan2(visualDir.y, visualDir.x));
    // player.ts renders the container at the player's visual world position;
    // the camera transform lives on the world root.
    container->setPosition(visualPos.x, visualPos.y);
    // player.ts: container.scale = camera.m_pixels(1) = m_zoom. The port keeps
    // the container in world units under the camera-transformed root, so the
    // equivalent world scale is m_zoom / (m_ppu * m_zoom) = 1 / m_ppu.
    container->setScale(1.0f / ctx.camera().m_ppu, 1.0f / ctx.camera().m_ppu);
    container->setVisible(!dead);
    int renderLayer = layer;
    int zOrd = 18;
    const auto* ap = ctx.activePlayer();
    const Collider col = Collider::createCircle(pos, maxVisualRadius);
    // player.ts updateRenderLayer: stairs flagged noCeilingReveal suppress
    // ceiling reveals while the active player is on them.
    if (ap) {
        noCeilingRevealTicker -= dt;
        if (ap == this && layer != 0) {
            for (auto* structure : ctx.map().structurePool.m_getPool()) {
                if (!structure->active) {
                    continue;
                }
                for (const auto& stair : structure->stairs) {
                    if (stair.noCeilingReveal && colliderIntersect(stair.collision, col)) {
                        noCeilingRevealTicker = 0.25f;
                    }
                }
            }
        }
    }
    if (ap && ctx.map().insideStructureStairs(col)) {
        const bool mask = ctx.map().insideStructureMask(col);
        if (((layer & 1) && ((ap->layer & 1) || !ctx.map().insideBuildingCeiling(col, true))) || ((ap->layer & 2) && !mask)) renderLayer |= 2;
        if ((layer & 1) == (ap->layer & 1) && (!mask || ap->layer == 0)) { renderLayer |= 2; zOrd += 100; }
    }
    const int zIdx = __id + (downed ? 0 : 262144) + (ap == this ? 65536 : 0) + (scale > 1.0f ? 131072 : 0);

    // --- Priority 1: Player action sounds (footsteps, actions, deploy, bleed, etc.) ---
    if (!dead && ctx.audio()) {
        audio::AudioManager* audio = ctx.audio();
        const bool isActivePlayer = (ap == this);

        // player.ts: weapon deploy/switch sound (~line 1042).
        // Detect activeWeapon change from previous frame via a cached string.
        // Played for the active player only (mirrors web client's isActivePlayer guard).
        if (isActivePlayer && !_prevActiveWeapon.empty() && _prevActiveWeapon != activeWeapon) {
            const GameObjRenderDef* prevWeapon = item(_prevActiveWeapon);
            const GameObjRenderDef* newWeapon = item(activeWeapon);
            if (newWeapon && (newWeapon->category == "melee" || newWeapon->category == "throwable")) {
                // player.ts: itemDef.sound.deploy (melee/throwable equip).
                const std::string& deploySound = newWeapon->soundDeploy;
                if (!deploySound.empty()) {
                    audio::PlaySoundOptions opts;
                    opts.channel = "sfx";
                    opts.hasSoundPos = true;
                    opts.soundPos = pos;
                    opts.fallOff = 3.0f;
                    audio->playSound(deploySound, opts);
                }
            } else if (newWeapon && newWeapon->category == "gun") {
                // player.ts: gun_switch_01 for a quick swap, the gun's
                // def.sound.deploy when the full deployment animation plays.
                std::string switchSound = "gun_switch_01";
                bool deployFull = false;
                if (prevWeapon && prevWeapon->category == "gun" &&
                    !newWeapon->soundDeploy.empty()) {
                    deployFull = true;
                }
                if (deployFull) {
                    switchSound = newWeapon->soundDeploy;
                }
                if (cycleSoundInstance != audio::kInvalidSound) {
                    audio->stopSound(cycleSoundInstance);
                    cycleSoundInstance = audio::kInvalidSound;
                }
                audio::PlaySoundOptions opts;
                opts.channel = "activePlayer";
                cycleSoundInstance = audio->playSound(switchSound, opts);
            }
        }
        _prevActiveWeapon = activeWeapon;

        // player.ts: update surface and footsteps
        auto gs = ctx.map().getGroundSurface(pos, layer);
        surfaceType = gs.type;
        surfaceWaterColor = gs.waterColor;
        surfaceRippleColor = gs.rippleColor;
        const bool inWater = surfaceType == SurfaceType::Water;
        // Footsteps (player.ts ~line 964)
        stepDistance += v2Length(v2Sub(posOld, pos));
        if ((stepDistance > 5.0f && inWater) || (inWater && !wasInWater)) {
            stepDistance = 0.0f;
            ctx.particleBarn().addRippleParticle(ctx.factory(), pos, layer, surfaceRippleColor);
            audio::PlaySoundOptions opts;
            opts.channel = "sfx";
            opts.hasSoundPos = true;
            opts.soundPos = pos;
            opts.fallOff = 3.0f;
            opts.hasLayer = true;
            opts.layer = layer;
            opts.filter = "muffled";
            audio->playGroup("footstep_water", opts);
        } else if (stepDistance > 4.0f && !inWater) {
            stepDistance = 0.0f;
            audio::PlaySoundOptions opts;
            opts.channel = "sfx";
            opts.hasSoundPos = true;
            opts.soundPos = pos;
            opts.fallOff = 3.0f;
            opts.hasLayer = true;
            opts.layer = layer;
            opts.filter = "muffled";
            // player.ts: `footstep_${this.surface.type}` (a shipped sound group
            // for every map material: grass/sand/water/tile/asphalt/brick/
            // stone/carpet/bunker/container/warehouse/house/shack/snow).
            const std::string footstepGroup = std::string("footstep_") + surfaceTypeName(surfaceType);
            audio->playGroup(footstepGroup, opts);
        }
        wasInWater = inWater;

        // player.ts ~line 1018: Bleeding sound
        if (downed && isActivePlayer) {
            bleedTicker -= dt;
            if (bleedTicker < 0.0f) {
                bleedTicker = 2.5f;
                audio::PlaySoundOptions opts;
                opts.channel = "hits";
                opts.hasSoundPos = true;
                opts.soundPos = pos;
                opts.fallOff = 3.0f;
                opts.hasLayer = true;
                opts.layer = layer;
                opts.filter = "muffled";
                audio->playSound("player_bullet_hit_02", opts);
            }
        }

        // player.ts ~line 1122: Haste ability sound
        // Played once in m_updateData when hasteSeq changes.

        // --- Dry fire sound (player.ts m_update ~1218) ---
        // The cue is local-only: never while spectating a teammate and never
        // while the emote/ping wheel holds the input (`preventInput`). It needs
        // both the equipped gun's clip and its reserve ammo to be empty.
        if (isActivePlayer && !ctx.spectating()) {
            const GameObjRenderDef* gunDef = item(activeWeapon);
            const bool isGun = gunDef && gunDef->category == "gun";
            const bool held = ctx.localFireHeld();
            if (!playedDryFire && isGun
                && (ctx.localFirePressed() || (held && !gunDef->fireModeSingle))
                && actionType == Action_None && !ctx.localPreventInput()
                && !gunDef->ammoInfinite) {
                const int ammoLeft = ctx.localReserveAmmo();
                const int currentClip = ctx.localAmmo();
                if (ammoLeft == 0 && currentClip == 0) {
                    if (!gunDef->soundEmpty.empty()) {
                        audio->playSound(gunDef->soundEmpty);
                    }
                    playedDryFire = true;
                }
            }
            if (!held) {
                playedDryFire = false;
            }
        } else if (!isActivePlayer) {
            playedDryFire = false;
        }

        // --- Priority 2: Bush enter/exit sound and particles (player.ts ~line 895) ---
        if (isActivePlayer) {
            const auto& obstacles = ctx.map().obstaclePool.m_getPool();
            bool isInBush = false;
            std::string bushType;
            const MapObjectDef* bushDef = nullptr;
            for (auto* obstacle : obstacles) {
                if (!obstacle || !obstacle->active || obstacle->dead ||
                    obstacle->layer != layer) continue;
                const MapObjectDef* od = getDefProvider() ? getDefProvider()->mapObject(obstacle->type) : nullptr;
                if (od && od->isBush) {
                    const float bushRad = rad * 0.25f;
                    if (colliderIntersectCircle(obstacle->collider, pos, bushRad)) {
                        isInBush = true;
                        bushType = obstacle->type;
                        bushDef = od;
                        break;
                    }
                }
            }
            lastInsideObstacleTime -= dt;
            const bool wasInside = wasInsideObstacle;
            wasInsideObstacle = isInBush;
            if (isInBush) insideObstacleType = bushType;
            if (wasInside != isInBush && lastInsideObstacleTime < 0 && bushDef) {
                lastInsideObstacleTime = 0.2f;
                // player.ts: the bush cue/particle come from the obstacle def
                // (sound.enter / hitParticle), not a hardcoded leaf.
                if (!bushDef->soundEnter.empty() && bushDef->soundEnter != "none") {
                    audio::PlaySoundOptions bushOpts;
                    bushOpts.channel = "sfx";
                    bushOpts.hasSoundPos = true;
                    bushOpts.soundPos = pos;
                    bushOpts.fallOff = 1.0f;
                    bushOpts.hasLayer = true;
                    bushOpts.layer = layer;
                    bushOpts.filter = "muffled";
                    audio->playSound(bushDef->soundEnter, bushOpts);
                }
                const Vec2 moveDir = v2NormalizeSafe(v2Sub(posOld, pos), Vec2(1.0f, 0.0f));
                const float partDir = isInBush ? 1.0f : -1.0f;
                const int numParts = static_cast<int>(randRange(3.0f, 5.0f));
                for (int i = 0; i < numParts; i++) {
                    const Vec2 vel = v2Mul(
                        v2Rotate(v2Mul(moveDir, partDir),
                                 ((randf() - 0.5f) * pi) / 1.5f),
                        6.0f + randf() * 2.0f);
                    if (!bushDef->hitParticle.empty()) {
                        ctx.particleBarn().addParticle(ctx.factory(), bushDef->hitParticle, layer, pos, vel);
                    }
                }
            }
        }

        // --- Priority 3: Door error sound (player.ts ~line 940) ---
        if (isActivePlayer) {
            const auto& obstacles = ctx.map().obstaclePool.m_getPool();
            Obstacle* doorErrorObstacle = nullptr;
            for (auto* obstacle : obstacles) {
                if (!obstacle || !obstacle->active || obstacle->dead ||
                    obstacle->layer != layer || !obstacle->isDoor) continue;
                const float doorRad = rad + 0.25f;
                const Vec2 toDoor = v2Sub(obstacle->pos, pos);
                const Vec2 doorDir = v2Rotate(Vec2(1.0f, 0.0f), obstacle->rot);
                if (colliderIntersectCircle(obstacle->collider, pos, doorRad) &&
                    !obstacle->doorOpen &&
                    (obstacle->doorLocked || obstacle->doorCanUse && v2Dot(toDoor, doorDir) < 0)) {
                    doorErrorObstacle = obstacle;
                    break;
                }
            }
            const bool wasNearDoorError = isNearDoorError;
            isNearDoorError = doorErrorObstacle != nullptr;
            doorErrorTicker -= dt;
            if (isNearDoorError && !wasNearDoorError && doorErrorTicker <= 0) {
                doorErrorTicker = 0.5f;
                audio::PlaySoundOptions doorErrOpts;
                doorErrOpts.channel = "sfx";
                doorErrOpts.hasSoundPos = true;
                doorErrOpts.soundPos = pos;
                doorErrOpts.fallOff = 1.0f;
                doorErrOpts.hasLayer = true;
                doorErrOpts.layer = layer;
                doorErrOpts.filter = "muffled";
                audio->playSound("door_error_01", doorErrOpts);
            }
        }
    }

    // player.ts: downed players (and trick_drain) drip blood every 1 s (3 s for
    // trick_drain); the particles are parented to the player container.
    {
        bool trickDrain = false;
        for (const auto& perk : perks) {
            if (perk.type == "trick_drain") {
                trickDrain = true;
                break;
            }
        }
        if (!dead && ((downed && actionType == Action_None) || trickDrain)) {
            bleedTicker -= dt;
            if (bleedTicker < 0.0f) {
                bleedTicker = trickDrain ? 3.0f : 1.0f;
                Vec2 vel = v2Rotate(v2Neg(dir), (randf() - 0.5f) * pi / 3.0f);
                vel.y *= -1.0f;
                ctx.particleBarn().addParticle(
                    ctx.factory(), "bloodSplat", renderLayer, Vec2(), v2Mul(vel, ctx.camera().m_ppu),
                    1.0f, randf() * pi * 2.0f, container, renderZOrd + 1);
                if (ap == this && ctx.audio()) {
                    audio::PlaySoundOptions opts;
                    opts.channel = "hits";
                    opts.hasSoundPos = true;
                    opts.soundPos = pos;
                    opts.fallOff = 3.0f;
                    opts.hasLayer = true;
                    opts.layer = layer;
                    opts.filter = "muffled";
                    ctx.audio()->playSound("player_bullet_hit_02", opts);
                }
            }
        }
    }

    // Cache position for next frame's movement delta.
    posOld = pos;
    // Published for player skins (obstacle.ts reads renderLayer/renderZOrd/renderZIdx).
    renderZLayer = renderLayer;
    renderZOrd = zOrd;
    renderZIdx = zIdx;

    // player.ts: the aura is added under the player container and has special
    // layer visibility rules (it does not clip well with the bunker mask).
    if (ap) {
        const bool auraLayerMatch = ((ap->layer & 2) != 0) || ((ap->layer & 1) == 1) ||
                                    ((layer & 1) == 0);
        auraVisible = !dead && auraLayerMatch;
    } else {
        auraVisible = false;
    }
    if (auraContainer) {
        ctx.renderer().addPIXIObj(auraContainer, renderLayer, zOrd - 1, zIdx);
        auraContainer->setPosition(visualPos.x, visualPos.y);
        auraContainer->setScale(1.0f / ctx.camera().m_ppu, 1.0f / ctx.camera().m_ppu);
        auraContainer->setVisible(auraVisible && auraCircle && auraCircle->getSortOrd() >= 0);
    }

    ctx.renderer().addPIXIObj(container, renderLayer, zOrd, zIdx);
    auto emitter = [&](Emitter*& e, bool enabled, const char* type) {
        if (enabled && !dead && !e) {
            EmitterOptions opts;
            opts.pos = pos;
            opts.layer = renderLayer;
            e = ctx.particleBarn().addEmitter(ctx.factory(), type, opts);
        } else if ((!enabled || dead) && e) { e->stop(); e = nullptr; }
        if (e) { e->pos = v2Add(pos, Vec2(0.0f, 0.1f)); e->layer = renderLayer; e->zOrd = zOrd + 1; }
    };
    emitter(healEmitter, healEffect, "heal_basic");
    if (lastStandEffect) {
        if (!adrenalineEmitter) {
            EmitterOptions opts;
            opts.pos = pos;
            opts.layer = renderLayer;
            opts.rateMult = 0.33f;
            opts.hasColor = true;
            opts.color = 0x4da6ff;
            adrenalineEmitter = ctx.particleBarn().addEmitter(ctx.factory(), "boost_basic", opts);
        }
    } else if (adrenalineEmitter) {
        adrenalineEmitter->stop();
        adrenalineEmitter = nullptr;
    }
    if (adrenalineEmitter) {
        adrenalineEmitter->pos = v2Add(pos, Vec2(0.0f, 0.1f));
        adrenalineEmitter->layer = renderLayer;
        adrenalineEmitter->zOrd = zOrd + 1;
    }
    const char* hasteTypes[] = {"", "windwalk", "takedown", "inspire"};
    const int haste = std::clamp(hasteType, 0, 3);
    emitter(hasteEmitter, haste != 0, hasteTypes[haste]);
    updateSubmersion(dt, ctx);
    updateFrozenState(dt, ctx);
    updateAura(dt, ctx, ap == this);
    if (finished) playAnim(Anim_None, animSeq);
}

// playAnim for a cached action item (player.ts selectAnim). The C++ port does
// not receive Action/item for every player, so the aura caller passes them in.
void Player::updateActions(int actionType, const std::string& actionItem, Ctx& ctx) {
    if (paramsCached && cachedActionType == actionType && cachedActionItem == actionItem) {
        return;
    }
    paramsCached = true;
    cachedActionType = actionType;
    cachedActionItem = actionItem;

    // Priority 1: Play action start sound (player.ts playActionStartEffect).
    // Stop any previous action sound first.
    if (ctx.audio() && actionSoundInstance != audio::kInvalidSound) {
        ctx.audio()->stopSound(actionSoundInstance);
        actionSoundInstance = audio::kInvalidSound;
    }
    if (ctx.audio() && (actionType == Action_UseItem || actionType == Action_Reload || actionType == Action_ReloadAlt)) {
        const GameObjRenderDef* actionItemDef =
            actionItem.empty() ? nullptr : item(actionItem);
        std::string soundName;
        std::string channel;
        // Determine isActivePlayer by checking ctx's activePlayerId.
        // Since updateActions is called from m_updateData which has a Ctx&,
        // we store it in a context-aware way. Use the stored reference.
        if (actionType == Action_UseItem && !actionItem.empty()) {
            // player.ts: HealDef/BoostDef.sound.use (no string guessing).
            soundName = actionItemDef ? actionItemDef->soundUse : std::string();
            channel = ctx.activePlayer() == this ? "activePlayer" : "otherPlayers";
        } else if (actionType == Action_Reload || actionType == Action_ReloadAlt) {
            // player.ts: GunDef.sound.reload / reloadAlt.
            if (actionItemDef) {
                soundName = actionType == Action_ReloadAlt ? actionItemDef->soundReloadAlt
                                                           : actionItemDef->soundReload;
            }
            channel = ctx.activePlayer() == this ? "activePlayer" : "otherPlayers";
        }
        // The web client looks up HealDef/BoostDef.sound.use, GunDef.sound.reload
        // etc. directly from the item def.
        if (!soundName.empty()) {
            audio::PlaySoundOptions opts;
            opts.channel = channel;
            opts.hasSoundPos = true;
            opts.soundPos = pos;
            opts.fallOff = 2.0f;
            opts.hasLayer = true;
            opts.layer = layer;
            opts.filter = "muffled";
            actionSoundInstance = ctx.audio()->playSound(soundName, opts);
        }
    }

    // player.ts playActionStartEffect: reload caseTiming == "reload" guns eject
    // one casing per chambered round (revolvers/pistols), alternating sides.
    if ((actionType == Action_Reload || actionType == Action_ReloadAlt) && !actionItem.empty()) {
        const GameObjRenderDef* actionItemDef = item(actionItem);
        if (actionItemDef && actionItemDef->casingOnReload) {
            const int maxReload = std::max(1, actionItemDef->maxReload);
            for (int n = 0; n < maxReload; n++) {
                const float shellDir = (n % 2 == 0) ? -1.0f : 1.0f;
                const float shellAngle = pi + (pi / 4.0f) * shellDir;
                const float shellSpeedMult = maxReload <= 2 ? 1.0f : math::lerp(randf(), 0.8f, 1.2f);
                createCasingParticle(ctx.factory(), *actionItemDef, shellAngle, shellSpeedMult, pos, dir,
                                     renderZLayer, renderZOrd + 1, ctx.particleBarn());
            }
        }
    }
}

void Player::updateAura(float dt, Ctx& ctx, bool isActivePlayer) {
    if (!auraContainer) {
        return;
    }
    // player.ts role visuals: only the active player's use-item/revive action
    // (with aoe_heal / self-revive) shows the aura.
    bool hasPerkAoe = std::any_of(perks.begin(), perks.end(),
                                  [](const auto& p) { return p.type == "aoe_heal"; });
    const int action = cachedActionType;
    const bool auraAction = action == Action_UseItem || action == Action_Revive;
    if (!auraAction || dead || (!hasPerkAoe)) {
        auraPulseTicker = 0.0f;
        auraPulseDir = 1.0f;
        if (auraCircle) {
            auraCircle->setVisible(false);
            auraCircle->setSortKey(-1, -1);
        }
        return;
    }
    const GameObjRenderDef* actionItemDef =
        cachedActionItem.empty() ? nullptr : item(cachedActionItem);
    if (actionItemDef) {
        auraSprite = actionItemDef->auraSprite.empty() ? "part-aura-circle-01.img"
                                                       : actionItemDef->auraSprite;
        auraTint = actionItemDef->auraSprite.empty() ? 0xff00ff : actionItemDef->auraTint;
        auraRadius = PlayerConfig::medicHealRange;
    } else {
        auraSprite = "part-aura-circle-01.img";
        auraTint = 0xff00ff;
        auraRadius = PlayerConfig::medicReviveRange;
    }
    auraRadius *= 0.125f;
    auraCircle->setFrame(auraSprite);
    auraCircle->setScale(auraRadius, auraRadius);
    auraCircle->setTint(auraTint);
    auraCircle->setVisible(true);
    auraCircle->setSortKey(0, 0);

    // player.ts updateAura: fade at the active player's view edge, pulse scale.
    bool inView = true;
    if (!isActivePlayer) {
        const Player* ap = ctx.activePlayer();
        if (ap && ap->viewHalfWidth > 0.0f) {
            const Vec2 min = v2Sub(ap->pos, Vec2(ap->viewHalfWidth, ap->viewHalfHeight));
            const Vec2 max = v2Add(ap->pos, Vec2(ap->viewHalfWidth, ap->viewHalfHeight));
            inView = intersectAabbCircle(min, max, pos, 1.0f);
        }
    }
    auraViewFade = math::lerp(dt * 6.0f, auraViewFade, inView ? 1.0f : 0.0f);
    auraPulseTicker = math::clamp(auraPulseTicker + dt * auraPulseDir * 1.5f, 0.0f, 1.0f);
    const float pulseAlpha = math::easeOutExpo(auraPulseTicker) * 0.75f + 0.25f;
    if (auraPulseTicker >= 1.0f || auraPulseTicker <= 0.0f) {
        auraPulseDir *= -1.0f;
    }
    auraCircle->setAlpha(pulseAlpha * auraViewFade);
}

void Player::updateSubmersion(float dt, Ctx& ctx) {
    if (!bodySubmergeSprite) {
        return;
    }
    auto surface = ctx.map().getGroundSurface(pos, layer);
    surfaceType = surface.type;
    surfaceWaterColor = surface.waterColor;
    surfaceRippleColor = surface.rippleColor;
    const bool inWater = surfaceType == SurfaceType::Water;
    float submersionAmount = 0.0f;
    if (inWater) {
        const float dist = ctx.map().distanceToShore(pos);
        submersionAmount = math::remap(dist, 0.0f, 16.0f, 0.6f, 1.0f);
    }
    submersion = math::lerp(dt * 4.0f, submersion, submersionAmount);
    const float submersionAlpha = submersion * 0.8f;
    const float submersionScale = (0.9f - submersion * 0.4f) * 2.0f;
    bodySubmergeSprite->setFrame("player-wading-01.img");
    bodySubmergeSprite->setScale(submersionScale, submersionScale);
    bodySubmergeSprite->setAlpha(submersionAlpha);
    bodySubmergeSprite->setVisible(submersionAlpha > 0.001f);
    if (inWater) {
        bodySubmergeSprite->setTint(surfaceWaterColor);
    }
    for (auto* limb : submergeLimbs) {
        if (!limb) {
            continue;
        }
        const float alpha = downed ? submersionAlpha : 0.0f;
        limb->setAlpha(alpha);
        limb->setVisible(alpha > 0.001f);
        if (inWater) {
            limb->setTint(surfaceWaterColor);
        }
    }
}

void Player::updateFrozenState(float dt, Ctx& ctx) {
    (void)ctx;
    if (!bodyEffectSprite) {
        return;
    }
    const float fadeDuration = 0.25f;
    if (frozenActive) {
        frozenTicker = fadeDuration;
    } else {
        frozenTicker -= dt;
        updateFrozenImage = true;
    }
    bodyEffectSprite->setAlpha(frozenActive
                                   ? 1.0f
                                   : math::remap(frozenTicker, 0.0f, fadeDuration, 0.0f, 1.0f));
    bodyEffectSprite->setVisible(frozenTicker > 0.0f);
}

bool Player::hasActivePan() const {
    return wearingPan || (activeWeapon == "pan" && currentAnim != Anim_Melee);
}

math::Segment Player::getPanSegment() const {
    const GameObjRenderDef* pan = item("pan");
    if (!pan || !pan->hasPanSurface) {
        return { Vec2(), Vec2() };
    }
    const bool equipped = !wearingPan;
    Vec2 p0 = equipped ? pan->panEquippedP0 : pan->panUnequippedP0;
    Vec2 p1 = equipped ? pan->panEquippedP1 : pan->panUnequippedP1;
    if (!math::eqAbs(scale, 1.0f)) {
        if (equipped) {
            // player.ts: equipped pans offset by (scale - 1) * 0.75, down-left.
            const float s = (scale - 1.0f) * 0.75f;
            const Vec2 off(s, -s);
            p0 = v2Add(p0, off);
            p1 = v2Add(p1, off);
        } else {
            p0 = v2Mul(p0, scale);
            p1 = v2Mul(p1, scale);
        }
    }
    return { p0, p1 };
}

Collider Player::getMeleeCollider() const {
    const GameObjRenderDef* meleeDef = item(activeWeapon);
    const float ang = std::atan2(dir.y, dir.x);
    const Vec2 off = v2Add(meleeDef && meleeDef->hasAttack ? meleeDef->attackOffset : Vec2(),
                           v2Mul(Vec2(1.0f, 0.0f), scale - 1.0f));
    const Vec2 pos = v2Add(this->pos, v2Rotate(off, ang));
    const float rad = meleeDef && meleeDef->hasAttack ? meleeDef->attackRad : 0.0f;
    return Collider::createCircle(pos, rad);
}

// player.ts m_getZoom(): m_zoom holds the step radius (a zoomDirty net field);
// device.mobile remaps the desktop radii through the mobile table.
float Player::getZoom(bool mobile) const {
    const float zoom = this->zoom;
    if (!mobile) {
        return zoom;
    }
    for (int i = 0; i < defs::kScopeZoomRadiusCount; i++) {
        if (defs::kScopeZoomRadiusDesktop[static_cast<size_t>(i)] != zoom) {
            continue;
        }
        if (i < defs::kScopeZoomRadiusCount) {
            return defs::kScopeZoomRadiusMobile[static_cast<size_t>(i)];
        }
    }
    return zoom;
}

void Player::runAnimEffects(Ctx& ctx, float ticker, float effectTicker, bool lastFrame) {
    (void)lastFrame;
    const DefProvider* provider = getDefProvider();
    const PlayerAnimationDef* anim = provider ? provider->playerAnimation(animation) : nullptr;
    if (!anim) {
        return;
    }
    for (const auto& effect : anim->effects) {
        if (effect.time < ticker || effect.time >= effectTicker) {
            continue;
        }
        switch (effect.fn) {
            case AnimEffectFn::PlaySound: {
                if (effect.sound == "__noop__" || effect.sound.empty()) {
                    break;
                }
                const GameObjRenderDef* def = item(activeWeapon);
                if (!def) {
                    break;
                }
                // animPlaySound: melee/throwable sound keyed by the def's sound
                // table. Melee and throwable defs both carry their swing/throw
                // sounds; the shared table is exposed via the render def's
                // punch/playerHit fields where applicable.
                const std::string* sound = nullptr;
                if (effect.sound == "swing") sound = &def->punchSound;
                else if (effect.sound == "playerHit") sound = &def->playerHit;
                if (!sound || sound->empty()) {
                    break;
                }
                if (ctx.audio()) {
                    audio::PlaySoundOptions opts;
                    opts.channel = "sfx";
                    opts.hasSoundPos = true;
                    opts.soundPos = pos;
                    opts.fallOff = 3.0f;
                    opts.hasLayer = true;
                    opts.layer = layer;
                    opts.filter = "muffled";
                    ctx.audio()->playSound(*sound, opts);
                }
                break;
            }
            case AnimEffectFn::MeleeCollision:
                animMeleeCollision(ctx, effect.playerHit);
                break;
            case AnimEffectFn::ThrowableParticles:
                animThrowableParticles(ctx);
                break;
        }
    }
}

void Player::animMeleeCollision(Ctx& ctx, const std::string& playerHit) {
    const GameObjRenderDef* meleeDef = item(activeWeapon);
    if (!meleeDef || meleeDef->category != "melee") {
        return;
    }
    const Collider meleeCol = getMeleeCollider();
    const float meleeDist = meleeCol.rad + v2Length(v2Sub(pos, meleeCol.pos));

    struct Hit {
        float pen = 0.0f;
        int prio = 0;
        Vec2 pos;
        Vec2 vel;
        int layer = 0;
        int zOrd = 0;
        std::string particle;
        std::string sound;
        bool playGroup = false;
    };
    std::vector<Hit> hits;

    const auto& obstacles = ctx.map().obstaclePool.m_getPool();
    for (auto* obstacle : obstacles) {
        if (!obstacle) {
            continue;
        }
        if (!colliderIntersect(obstacle->collider, meleeCol)) {
            continue;
        }
        if (!obstacle->active || obstacle->dead || obstacle->isSkin) {
            continue;
        }
        if (obstacle->height < PlayerConfig::meleeHeight) {
            continue;
        }
        if (!sameLayer(obstacle->layer, layer & 1)) {
            continue;
        }
        Vec2 dirPush;
        float pen = 0.0f;
        if (!colliderIntersectCircleRes(obstacle->collider, meleeCol.pos, meleeCol.rad, &dirPush,
                                        &pen)) {
            continue;
        }
        if (meleeDef->cleave) {
            const Vec2 meleeDir = v2NormalizeSafe(v2Sub(obstacle->pos, pos), Vec2(1.0f, 0.0f));
            uint16_t hitId = 0;
            float hitDist = 0.0f;
            if (intersectSegmentNearest(obstacles, pos, meleeDir, meleeDist,
                                        PlayerConfig::meleeHeight, layer, false, &hitId,
                                        &hitDist) &&
                hitId != obstacle->__id) {
                continue;
            }
        }
        const Vec2 closestPt = v2Add(meleeCol.pos, v2Mul(v2Neg(dirPush), meleeCol.rad - pen));
        const Vec2 vel = v2Rotate(v2Mul(dirPush, 7.5f), ((randf() - 0.5f) * pi) / 3.0f);
        Hit hit;
        hit.pen = pen;
        hit.prio = 1;
        hit.pos = closestPt;
        hit.vel = vel;
        hit.layer = renderZLayer;
        hit.zOrd = renderZOrd;
        hit.particle = obstacle ? obstacle->hitParticle : "";
        hit.sound = obstacle ? obstacle->punchSound : "";
        hit.playGroup = true;
        hits.push_back(std::move(hit));
    }

    // Players.
    const uint8_t ourTeamId = teamId;
    for (auto* playerCol : ctx.playerBarn().playerPool.m_getPool()) {
        if (!playerCol) {
            continue;
        }
        if (!playerCol->active) {
            continue;
        }
        if (playerCol->__id == __id || playerCol->dead) {
            continue;
        }
        if (!sameLayer(playerCol->layer, layer)) {
            continue;
        }
        Vec2 dirPush;
        float pen = 0.0f;
        if (!intersectCircleCircleRes(meleeCol.pos, meleeCol.rad, playerCol->pos, playerCol->rad,
                                      &dirPush, &pen)) {
            continue;
        }
        const Vec2 meleeDir = v2NormalizeSafe(v2Sub(playerCol->pos, pos), Vec2(1.0f, 0.0f));
        Vec2 linePoint;
        const bool lineHit = intersectSegmentCircleRes(
            pos, v2Add(pos, v2Mul(meleeDir, meleeDist)), playerCol->pos, playerCol->rad, &linePoint);
        const Vec2 pt = lineHit ? linePoint : playerCol->pos;
        const float distToPlayer = v2Length(v2Sub(pt, pos));
        const float distToObstacle = intersectSegmentDist(
            obstacles, pos, meleeDir, meleeDist, PlayerConfig::meleeHeight, layer, false);
        if (distToObstacle < distToPlayer) {
            continue;
        }
        const uint8_t teamId2 = ctx.playerBarn().teams.count(playerCol->__id)
                                    ? ctx.playerBarn().teams[playerCol->__id]
                                    : 0;
        const Vec2 vel = v2Rotate(meleeDir, ((randf() - 0.5f) * pi) / 3.0f);
        std::string hitSound = !playerHit.empty() ? playerHit : meleeDef->playerHit;
        if (hitSound.empty()) {
            hitSound = meleeDef->playerHit;
        }
        Hit hit;
        hit.pen = pen;
        hit.prio = teamId2 == ourTeamId ? 2 : 0;
        hit.pos = playerCol->pos;
        hit.vel = vel;
        hit.layer = playerCol->renderZLayer;
        hit.zOrd = playerCol->renderZOrd;
        hit.particle = "bloodSplat";
        hit.sound = hitSound;
        hit.playGroup = false;
        hits.push_back(std::move(hit));
    }

    std::sort(hits.begin(), hits.end(), [](const Hit& a, const Hit& b) {
        if (a.prio == b.prio) {
            return a.pen > b.pen;
        }
        return a.prio < b.prio;
    });

    size_t hitCount = hits.size();
    if (!meleeDef->cleave) {
        hitCount = math::min(hitCount, static_cast<size_t>(1));
    }
    for (size_t i = 0; i < hitCount; i++) {
        const Hit& hit = hits[i];
        if (!hit.particle.empty()) {
            ctx.particleBarn().addParticle(ctx.factory(), hit.particle, hit.layer, hit.pos, hit.vel,
                                           1.0f, randf() * pi * 2.0f, nullptr, hit.zOrd + 1);
        }
        if (!hit.sound.empty() && ctx.audio()) {
            audio::PlaySoundOptions opts;
            opts.channel = "hits";
            opts.hasSoundPos = true;
            opts.soundPos = hit.pos;
            opts.hasLayer = true;
            opts.layer = layer;
            opts.filter = "muffled";
            if (hit.playGroup) {
                ctx.audio()->playGroup(hit.sound, opts);
            } else {
                ctx.audio()->playSound(hit.sound, opts);
            }
        }
    }
}

void Player::animThrowableParticles(Ctx& ctx) {
    const GameObjRenderDef* weapon = item(activeWeapon);
    if (!weapon || !weapon->useThrowParticles) {
        return;
    }
    // Pin.
    const Vec2 pinOff = v2Rotate(Vec2(0.75f, 0.75f), std::atan2(dir.y, dir.x));
    ctx.particleBarn().addParticle(ctx.factory(), "fragPin", renderZLayer,
                                   v2Add(pos, pinOff),
                                   v2Mul(v2Rotate(dir, pi * 0.5f), 4.5f), 1.0f, randf() * pi * 2.0f,
                                   nullptr, renderZOrd + 1);
    // Lever.
    const Vec2 leverOff = v2Rotate(Vec2(0.75f, -0.75f), std::atan2(dir.y, dir.x));
    ctx.particleBarn().addParticle(ctx.factory(), "fragLever", renderZLayer,
                                   v2Add(pos, leverOff),
                                   v2Mul(v2Rotate(dir, -pi * 0.25f), 3.5f), 1.0f, randf() * pi * 2.0f,
                                   nullptr, renderZOrd + 1);
}
} // namespace surv
