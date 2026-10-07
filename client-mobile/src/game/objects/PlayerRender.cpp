#include "Barns.h"
#include "../GameWorld.h"
#include "../Map.h"
#include "../../render/Renderer.h"
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
void heldSprite(pix::Sprite* sprite, const HeldImageDef& def) {
    skinSprite(sprite, def.sprite, def.tint, 1.0f);
    sprite->setPosition(def.pos.x, def.pos.y);
    sprite->setScale(def.scale.x, def.scale.y);
    sprite->setRotation(def.rot);
}
} // namespace

void Player::m_init() {
    dead = downed = false;
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
    teamId = 0;
    gunRecoil = {};
    healEffect = false;
    hasteType = HasteType_None;
    hasteSeq = -1;
    healEmitter = hasteEmitter = nullptr;
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
}

void Player::m_free() {
    if (container) container->setVisible(false);
    if (healEmitter) healEmitter->stop();
    if (hasteEmitter) hasteEmitter->stop();
    healEmitter = hasteEmitter = nullptr;
    auraVisible = false;
}

void Player::m_updateData(const ObjectData& data, bool fullUpdate, bool isNew, Ctx& ctx) {
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
    if ((hasteType != data.hasteType || hasteSeq != data.hasteSeq) && hasteEmitter) {
        hasteEmitter->stop();
        hasteEmitter = nullptr;
    }
    hasteType = data.hasteType;
    hasteSeq = data.hasteSeq;
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
        nameText->setAnchor(0.5f, -1.0f);
        nameText->setScale(0.5f, 0.5f);
        nameText->setColor(0xffffff, 0x000000, 1.0f, true);
        container->addChild(nameText);
    }
    if (isNew || data.animSeq != animSeq) playAnim(data.animType, data.animSeq);
    updateActions(data.actionType, data.actionItem);
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
            gunSprites[i]->setAnchor(0.5f, 1.0f);
            gunSprites[i]->setScale(weapon->worldImg.scale.x * 0.5f / scale, weapon->worldImg.scale.y * 0.5f / scale);
            gunContainers[i]->setRotation(pi / 2.0f);
            gunContainers[i]->setPosition((weapon->isDual ? -5.95f : -4.25f) + weapon->gunOffset.x,
                                          (weapon->isDual ? 0.0f : -1.75f) + weapon->gunOffset.y);
            skinSprite(magSprites[i], weapon->magSprite, 0xffffff, 0.25f / scale);
            magSprites[i]->setPosition(weapon->magPos.x / scale, weapon->magPos.y / scale);
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
    const float maxVisualRadius = 1.0f;
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
    } else if (currentAnim != Anim_None) playAnim(Anim_None, animSeq);
    const auto* idle = provider ? provider->playerPose(downed ? "downed" : weapon ? weapon->idlePose : "fists") : nullptr;
    if (!idle && provider) idle = provider->playerPose("fists");
    for (int i = 0; i < PlayerBoneCount; ++i) bones[i] = (animMask & (1u << i)) ? animBones[i] : idle ? (*idle)[i] : BonePose{};
    const int state = currentAnim == Anim_Throw ? 2 : currentAnim == Anim_Cook && animTicker >= 0.1f ? 1 : 0;
    if (state != throwableState) { throwableState = state; visualsDirty = true; }
    if (visualsDirty) { updateVisuals(ctx); visualsDirty = false; }
    for (int i = 0; i < 4; ++i) {
        // PIXI pivot=-pose.pivot: equivalent translation avoids adding an
        // engine-specific pivot API to the adapter.
        Vec2 offset = v2Add(bones[i].pos, v2Rotate(bones[i].pivot, bones[i].rot));
        if (i == 0 && weapon && weapon->category == "gun" && !downed && currentAnim != Anim_Revive) offset = v2Add(offset, weapon->leftHandOffset);
        if (i < 2) offset.x -= gunRecoil[i] * 1.125f;
        boneContainers[i]->setPosition(offset.x, offset.y);
        boneContainers[i]->setRotation(bones[i].rot);
    }
    if (weapon && weapon->category == "melee") {
        const auto& bone = bones[5];
        const auto& img = weapon->worldImg;
        const float r = img.rot + bone.rot;
        Vec2 offset = v2Add(bone.pos, img.pos);
        offset = Vec2(offset.x * img.scale.x / scale, offset.y * img.scale.y / scale);
        offset = v2Sub(v2Rotate(offset, r), bone.pivot);
        meleeSprite->setPosition(offset.x, offset.y);
        meleeSprite->setRotation(r);
        meleeSprite->setScale(img.scale.x / scale, img.scale.y / scale);
    }
    bodyContainer->setScale(scale, scale);
    bodyContainer->setRotation(-std::atan2(dir.y, dir.x));
    const Vec2 screenPos = ctx.camera().m_pointToScreen(pos);
    const float screenScale = ctx.camera().m_pixels(1.0f);
    container->setPosition(screenPos.x, screenPos.y);
    container->setScale(screenScale, screenScale);
    container->setVisible(!dead);
    int renderLayer = layer;
    int zOrd = 18;
    const auto* ap = ctx.activePlayer();
    const Collider col = Collider::createCircle(pos, maxVisualRadius);
    if (ap && ctx.map().insideStructureStairs(col)) {
        const bool mask = ctx.map().insideStructureMask(col);
        if (((layer & 1) && ((ap->layer & 1) || !ctx.map().insideBuildingCeiling(col, true))) || ((ap->layer & 2) && !mask)) renderLayer |= 2;
        if ((layer & 1) == (ap->layer & 1) && (!mask || ap->layer == 0)) { renderLayer |= 2; zOrd += 100; }
    }
    const int zIdx = __id + (downed ? 0 : 262144) + (ap == this ? 65536 : 0) + (scale > 1.0f ? 131072 : 0);
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
        auraContainer->setPosition(screenPos.x, screenPos.y);
        auraContainer->setScale(screenScale, screenScale);
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
void Player::updateActions(int actionType, const std::string& actionItem) {
    if (paramsCached && cachedActionType == actionType && cachedActionItem == actionItem) {
        return;
    }
    paramsCached = true;
    cachedActionType = actionType;
    cachedActionItem = actionItem;
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
    const Map::GroundSurface surface = ctx.map().getGroundSurface(pos, layer);
    const bool inWater = surface.type == Map::SurfaceType::Water;
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
        bodySubmergeSprite->setTint(surface.waterColor);
    }
    for (auto* limb : submergeLimbs) {
        if (!limb) {
            continue;
        }
        const float alpha = downed ? submersionAlpha : 0.0f;
        limb->setAlpha(alpha);
        limb->setVisible(alpha > 0.001f);
        if (inWater) {
            limb->setTint(surface.waterColor);
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
} // namespace surv
