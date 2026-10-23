#include "EmoteManager.h"
#include "GameWorld.h"
#include "../audio/AudioManager.h"
#include "../net/Messages.h"
#include "../render/Defs.h"
#include "../render/PixiLike.h"
#include "objects/Barns.h"

namespace surv {

// Pull in the DefProvider for game object lookups at runtime.
#ifndef SURV_DEFS_CHECK
// Pull in the DefProvider for game object lookups at runtime.
#endif

EmoteManager::EmoteManager() = default;
EmoteManager::~EmoteManager() = default;

void EmoteManager::reset() {
    _outgoing.clear();
    _emotePool.clear();
    _zIdxNext = 0;
    for (auto& ind : _indicators) {
        ind.displayed = false;
        ind.fadeIn = 0.0f;
        ind.life = 0.0f;
        ind.fadeOut = 0.0f;
    }
}

// ---- Emote pool (simple growable vector) ------------------------------------
EmoteSprite* EmoteManager::allocEmote() {
    for (auto& e : _emotePool) {
        if (!e.alive) {
            e.alive = true;
            e.isNew = true;
            return &e;
        }
    }
    _emotePool.push_back({});
    auto* e = &_emotePool.back();
    e->alive = true;
    e->isNew = true;
    return e;
}

void EmoteManager::freeEmote(EmoteSprite* e) {
    if (e) e->alive = false;
}

// ---- Add emote from server (UpdateMsg.emotes) --------------------------------
void EmoteManager::addEmote(const Emote& msg, GameWorld& ctx) {
    (void)ctx;
    // Find or create an emote slot
    EmoteSprite* existing = nullptr;
    for (auto& e : _emotePool) {
        if (e.alive && e.playerId == msg.playerId) {
            existing = &e;
            break;
        }
    }
    EmoteSprite* e = existing ? existing : allocEmote();
    if (!e) return;

    // Reset timers
    e->isNew = true;
    e->type = msg.type;
    e->playerId = msg.playerId;
    e->pos = Vec2(0, 0);
    e->lifeIn = kEmoteLifeIn;
    e->life = kEmoteLife;
    e->lifeOut = kEmoteLifeOut;
    e->zIdx = _zIdxNext++;

    // Create PIXI sprites lazily
    if (!e->container) {
        e->container = ctx.factory()->createContainer();
        e->circleOuter = ctx.factory()->createSprite("emote-circle-outer.img");
        e->circleOuter->setAnchor(0.5f, 0.5f);
        e->circleOuter->setScale(0.55f * 0.8f, 0.55f * 0.8f);
        e->circleOuter->setTint(0); // black tint makes white circle
        e->container->addChild(e->circleOuter);

        e->sprite = ctx.factory()->createSprite();
        e->sprite->setAnchor(0.5f, 0.5f);
        e->sprite->setScale(0.55f, 0.55f);
        e->container->addChild(e->sprite);
        e->container->setVisible(false);
    }

    // Set texture from the emote def (shared/defs/gameObjects/emoteDefs.ts).
    // Previously this fell back to emote-circle-outer.img, so every emote drew
    // the same circle instead of its face/food/logo art.
    const DefProvider* provider = getDefProvider();
    const EmoteRenderDef* emoteDef = provider ? provider->emote(msg.type) : nullptr;
    if (emoteDef && !emoteDef->texture.empty()) {
        e->sprite->setFrame(emoteDef->texture);
    } else {
        e->sprite->setFrame("emote-circle-outer.img");
    }

    // Handle emote_loot type (special weapon image display)
    if (msg.type == "emote_loot" && !msg.itemType.empty()) {
        const auto* goDef = provider ? provider->gameObject(msg.itemType) : nullptr;
        if (goDef) {
            e->sprite->setFrame(goDef->img.sprite);
            e->sprite->setTint(goDef->img.tint);
        }
    }

    // Play the def's sound on its channel (web: audioManager.playSound(emote.sound,
    // { channel: emote.channel, soundPos, layer })).
    if (ctx.audio()) {
        audio::PlaySoundOptions opts;
        opts.channel = emoteDef && !emoteDef->channel.empty() ? emoteDef->channel : "ui";
        opts.rangeMult = 48.0f;
        const std::string sound = emoteDef && !emoteDef->sound.empty() ? emoteDef->sound : "emote_01";
        ctx.audio()->playSound(sound, opts);
    }
}

// ---- Add ping from server (UpdateMsg.emotes with isPing=true) ---------------
void EmoteManager::addPing(const Emote& msg, bool factionMode, GameWorld& ctx) {
    // shared/defs/gameObjects/pingDefs.ts drives the sprite, sound, lifetime and
    // map-event flag. Before the codegen existed every ping drew the danger
    // icon and used ping_danger_01.
    const DefProvider* provider = getDefProvider();
    const PingRenderDef* pingDef = provider ? provider->ping(msg.type) : nullptr;
    if (!pingDef) {
        return;
    }

    // Pick the indicator slot exactly like emote.ts addPing():
    //   * airdrop/airstrike have dedicated slots 4/5
    //   * team pings use the sender's index in our sorted group member list
    //     (playerBarn.getGroupInfo(groupId).playerIds.indexOf(playerId))
    //   * pings from other groups play their sound but get no indicator
    int indicatorIdx = -1;
    if (msg.type == "ping_airdrop") {
        indicatorIdx = 4;
    } else if (msg.type == "ping_airstrike") {
        indicatorIdx = 5;
    } else {
        const uint8_t activeGroup = ctx.activeGroupId();
        if (ctx.hasPlayerGroup(msg.playerId) && ctx.hasPlayerGroup(ctx.activePlayerId())
            && ctx.playerGroupId(msg.playerId) == activeGroup) {
            const std::vector<uint16_t> members = ctx.groupMembers(activeGroup);
            for (size_t i = 0; i < members.size() && i < 4; ++i) {
                if (members[i] == msg.playerId) {
                    indicatorIdx = static_cast<int>(i);
                    break;
                }
            }
        }
    }

    // GameConfig.groupColors: the web tints team indicators by group slot and
    // map events by their pingDef.tint.
    static const uint32_t kGroupColors[4] = { 0xffff00, 0xff00ff, 0x0000ff, 0xff5400 };
    const uint32_t tint = pingDef->mapEvent
        ? pingDef->tint
        : (indicatorIdx >= 0 && indicatorIdx < 4 ? kGroupColors[indicatorIdx] : 0xffffff);

    if (indicatorIdx >= 0 && indicatorIdx < kPingIndicatorCount) {
        PingIndicator* ind = &_indicators[indicatorIdx];

        // Create indicator sprites lazily.
        if (!ind->pingContainer) {
            ind->pingContainer = ctx.factory()->createContainer();
            ind->indContainer = ctx.factory()->createContainer();

            ind->borderSprite = ctx.factory()->createSprite("ping-border.img");
            ind->borderSprite->setAnchor(0.5f, 0.5f);
            ind->borderSprite->setScale(0.4f, 0.4f);
            ind->pingContainer->addChild(ind->borderSprite);

            ind->pingSprite = ctx.factory()->createSprite("ping-team-danger.img");
            ind->pingSprite->setAnchor(0.5f, 0.5f);
            ind->pingSprite->setScale(0.4f, 0.4f);
            ind->pingContainer->addChild(ind->pingSprite);

            ind->indSpriteInner = ctx.factory()->createSprite("ping-team-danger.img");
            ind->indSpriteInner->setAnchor(0.5f, 0.5f);
            ind->indSpriteInner->setScale(0.5f, 0.5f);
            ind->indContainer->addChild(ind->indSpriteInner);

            ind->indSpriteOuter = ctx.factory()->createSprite("ping-indicator.img");
            ind->indSpriteOuter->setAnchor(0.5f, 0.0f);
            ind->indSpriteOuter->setScale(0.5f, 0.5f);
            ind->indContainer->addChild(ind->indSpriteOuter);
        }

        if (!pingDef->texture.empty()) {
            ind->pingSprite->setFrame(pingDef->texture);
            ind->indSpriteInner->setFrame(pingDef->texture);
        }
        ind->borderSprite->setTint(tint);
        ind->pingSprite->setTint(tint);
        // The edge indicator keeps the group colour unless this is a map event
        // (indSpriteInner.baseTint / indSpriteOuter.baseTint in emote.ts).
        const uint32_t edgeTint = pingDef->mapEvent ? pingDef->tint : 0xffffff;
        ind->indSpriteInner->setTint(edgeTint);
        ind->indSpriteOuter->setTint(edgeTint);

        ind->pos = msg.hasPos ? msg.pos : Vec2(0, 0);
        ind->playerId = msg.playerId;
        ind->fadeIn = kPingFadeIn;
        ind->life = kPingLife;
        ind->fadeOut = kPingFadeOut;
        ind->mapEvent = pingDef->mapEvent;
        ind->worldDisplay = pingDef->worldDisplay;
        ind->displayed = false;
        // A reused slot may still carry the previous ping's fade-out alpha;
        // without this every ping after a faded-out one was nearly invisible.
        ind->pingContainer->setAlpha(1.0f);
        ind->indContainer->setAlpha(1.0f);
        ind->borderSprite->setAlpha(1.0f);
        ind->pingSprite->setAlpha(1.0f);
        ind->indSpriteInner->setAlpha(0.0f);
        ind->indSpriteOuter->setAlpha(0.0f);
    }

    // Play the ping sound on the UI channel. Pings always play at full volume
    // (emote.ts addPing); airstrike falloff in non-faction mode is a follow-up.
    (void)factionMode;
    if (ctx.audio() && !pingDef->sound.empty()) {
        audio::PlaySoundOptions opts;
        opts.channel = "ui";
        opts.rangeMult = 48.0f;
        ctx.audio()->playSound(pingDef->sound, opts);
    }
}

// ---- Outgoing queue ---------------------------------------------------------
void EmoteManager::queueEmote(const std::string& type, const Vec2& pos) {
    _outgoing.push_back({type, pos, false});
}

void EmoteManager::queuePing(const std::string& type, const Vec2& pos) {
    _outgoing.push_back({type, pos, true});
}

std::vector<EmoteManager::OutgoingEmote> EmoteManager::consumeOutgoing() {
    std::vector<OutgoingEmote> result = std::move(_outgoing);
    _outgoing.clear();
    return result;
}

// ---- Per-frame update -------------------------------------------------------
void EmoteManager::update(float dt, GameWorld& ctx) {
    const Camera& camera = ctx.camera();

    // Update emotes
    for (auto& e : _emotePool) {
        if (!e.alive) continue;

        // Track target position
        bool hasTarget = false;
        Vec2 targetPos(0, 0);
        int targetLayer = 0;

        // Look up the player who sent this emote
        Player* targetPlayer = ctx.playerBarn().getPlayerById(e.playerId);
        if (targetPlayer && !targetPlayer->dead) {
            targetPos = targetPlayer->pos;
            targetLayer = targetPlayer->layer;
            hasTarget = true;
        }

        if (!hasTarget) {
            DeadBody* body = ctx.deadBodyBarn().getDeadBodyById(e.playerId);
            if (body) {
                targetPos = body->pos;
                targetLayer = body->layer;
                hasTarget = true;
            }
        }

        if (!hasTarget) {
            e.alive = false;
            if (e.container) e.container->setVisible(false);
            continue;
        }

        // Play sound on first frame
        if (e.isNew) {
            e.isNew = false;
        }

        e.pos = targetPos;

        if (e.lifeIn > 0) e.lifeIn -= dt;
        else if (e.life > 0) e.life -= dt;
        else if (e.lifeOut > 0) e.lifeOut -= dt;
        else { e.alive = false; if (e.container) e.container->setVisible(false); continue; }

        // Add to renderer on layer 3 (top overlay) with high z-ord
        int zOrd = 50000 + e.zIdx;
        const bool sameLayer = targetPlayer ? (targetLayer == ctx.activePlayer()->layer) : true;
        int layer = sameLayer ? 3 : targetLayer;
        ctx.renderer().addPIXIObj(e.container, layer, zOrd, e.zIdx);

        // Scale animation: ease-out-elastic on lifeIn, hold at 1, fade out on lifeOut
        float scale = 0.0f;
        if (e.lifeIn > 0) {
            float normLifeIn = 1.0f - e.lifeIn / kEmoteLifeIn;
            // Simple ease-out
            scale = normLifeIn < 0.5f ? 2.0f * normLifeIn * normLifeIn : -1.0f + (4.0f - 2.0f * normLifeIn) * normLifeIn;
            scale *= scale; // more aggressive elastic feel
        } else if (e.life > 0) {
            scale = 1.0f;
        } else if (e.lifeOut > 0) {
            scale = e.lifeOut / kEmoteLifeOut;
        }

        // Position with offset above player
        Vec2 offsetPos = targetPos;
        // Offset rises with camera zoom (similar to web's posOffset)
        float zoomClamp = camera.m_zoom < 0.75f ? 0.75f : (camera.m_zoom > 1.0f ? 1.0f : camera.m_zoom);
        offsetPos.y += 4.0f / zoomClamp;

        // World space (emoteManager.ts): the camera lives on the world root, so
        // the web's screen-space m_pixels() scale needs the /m_ppu conversion.
        const float emoteScale =
            scale * 0.55f * (camera.m_zoom < 0.9f ? 0.9f : (camera.m_zoom > 1.75f ? 1.75f : camera.m_zoom)) /
            camera.m_ppu;
        e.container->setPosition(offsetPos.x, offsetPos.y);
        e.container->setScale(emoteScale, emoteScale);
        e.container->setVisible(true);
    }

    // Update indicators
    for (int ti = 0; ti < kPingIndicatorCount; ++ti) {
        auto& ind = _indicators[ti];
        if (ind.fadeOut <= 0) {
            if (ind.displayed) {
                if (ind.pingContainer) ind.pingContainer->setVisible(false);
                if (ind.indContainer) ind.indContainer->setVisible(false);
                ind.displayed = false;
            }
            continue;
        }

        ind.fadeIn -= dt;
        ind.life -= dt;
        ind.fadeOut -= ind.life > 0 ? 0 : dt;

        if (ind.fadeOut > 0 && ind.pingContainer && ind.indContainer) {
            Vec2 dir = v2NormalizeSafe(v2Sub(ind.pos, camera.m_pos), Vec2(1.0f, 0.0f));

            Vec2 screenPos = camera.m_pointToScreen(ind.pos);
            // The indicator nodes live under the camera-transformed world root,
            // so the web's screen-space positions/scales are converted here.
            const Vec2 worldPos = camera.m_screenToPoint(screenPos);
            float borderScale = camera.m_pixels(0.4f) / camera.m_ppu;
            float pingScale = camera.m_pixels(0.4f) / camera.m_ppu;

            ind.borderSprite->setScale(borderScale, borderScale);
            ind.pingSprite->setScale(pingScale, pingScale);
            ind.borderSprite->setAlpha(1.0f);
            ind.pingSprite->setAlpha(1.0f);
            ind.indSpriteInner->setAlpha(ind.fadeIn > 0 ? 0.0f : 1.0f);
            ind.indSpriteOuter->setAlpha(ind.fadeIn > 0 ? 0.0f : 1.0f);

            ind.pingSprite->setPosition(worldPos.x, worldPos.y);
            ind.borderSprite->setPosition(worldPos.x, worldPos.y);

            // On-screen vs edge-of-screen positioning
            const float off = 64.0f;
            float leftConstrain = screenPos.x;
            float topConstrain = screenPos.y;
            if (screenPos.x < off) leftConstrain = off;
            else if (screenPos.x > camera.m_screenWidth - off) leftConstrain = camera.m_screenWidth - off;
            if (screenPos.y < off) topConstrain = off;
            else if (screenPos.y > camera.m_screenHeight - off) topConstrain = camera.m_screenHeight - off;

            float rot = std::atan2(dir.y, -dir.x) + 3.14159f * 0.5f;
            const Vec2 edgeWorld = camera.m_screenToPoint(Vec2(leftConstrain, topConstrain));
            ind.indSpriteOuter->setPosition(edgeWorld.x, edgeWorld.y);
            ind.indSpriteOuter->setRotation(rot);
            ind.indSpriteInner->setPosition(edgeWorld.x, edgeWorld.y);

            // Pulse effect on border
            float pulseAlpha = ind.borderSprite->getAlpha() <= 0 ? 1 : ind.borderSprite->getAlpha() - dt;
            ind.borderSprite->setAlpha(pulseAlpha);
            float pulseScale = camera.m_pixels(0.4f * (2 - pulseAlpha)) / camera.m_ppu;
            ind.borderSprite->setScale(pulseScale, pulseScale);

            // Fade in/out
            if (ind.fadeIn > 0) {
                float elemOpacity = 1 - ind.fadeIn / kPingFadeIn;
                ind.pingContainer->setAlpha(1.0f);
                ind.indContainer->setAlpha(1.0f);
                ind.indSpriteOuter->setAlpha(elemOpacity);
            }
            if (ind.life < 0) {
                float elemOpacity = ind.fadeOut / kPingFadeOut;
                ind.pingContainer->setAlpha(elemOpacity);
                ind.indContainer->setAlpha(elemOpacity);
            }

            if (!ind.displayed) {
                ind.pingContainer->setVisible(ind.worldDisplay);
                // emote.ts hides our own edge-of-screen indicator unless the
                // ping is a map event (airdrop/airstrike).
                ind.indContainer->setVisible(!(ind.playerId == ctx.activePlayerId()) || ind.mapEvent);
                ind.displayed = true;
            }

            // Add to world overlay
            ctx.renderer().addPIXIObj(ind.pingContainer, 3, 40000 + ti, ti);
            ctx.renderer().addPIXIObj(ind.indContainer, 3, 40000 + ti, ti + 100);
        }
    }

    // Clear indicators that timeout
    for (auto& ind : _indicators) {
        if (ind.fadeOut <= 0 && ind.displayed) {
            if (ind.pingContainer) ind.pingContainer->setVisible(false);
            if (ind.indContainer) ind.indContainer->setVisible(false);
            ind.displayed = false;
        }
    }
}

} // namespace surv