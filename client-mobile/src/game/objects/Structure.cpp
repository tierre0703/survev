#include "Structure.h"
#include "Barns.h"

#include "../GameWorld.h"
#include "../Map.h"
#include "../../audio/Ambiance.h"
#include "../../render/Defs.h"
#include "../../render/Renderer.h"

#include <cmath>

namespace surv {

static Collider boundingFor(const std::string& type) {
    const DefProvider* provider = getDefProvider();
    const MapObjectDef* def = provider ? provider->mapObject(type) : nullptr;
    if (def && def->hasBounding) {
        return def->boundingCollider;
    }
    return Collider::createAabb(Vec2(0.0f, 0.0f), Vec2(0.0f, 0.0f));
}

void Structure::m_init() {
    soundTransitionT = 0.0f;
    soundEnabledT = 0.0f;
    layers.clear();
    stairs.clear();
    mask.clear();
}

void Structure::m_free() {
    layers.clear();
    stairs.clear();
    mask.clear();
}

void Structure::m_updateData(const ObjectData& data, bool fullUpdate, bool isNew, Ctx& ctx) {
    if (!fullUpdate) {
        return;
    }
    type = data.type;
    layer = 0;
    pos = data.pos;
    rot = math::oriToRad(data.ori);
    scale = 1.0f;
    interiorSoundAlt = data.interiorSoundAlt;
    interiorSoundEnabled = data.interiorSoundEnabled;
    if (isNew) {
        soundTransitionT = interiorSoundAlt ? 1.0f : 0.0f;
        soundEnabledT = interiorSoundEnabled ? 1.0f : 0.0f;
    }
    aabb = colliderTransform(boundingFor(type), pos, rot, scale);

    const DefProvider* provider = getDefProvider();
    const MapObjectDef* def = provider ? provider->mapObject(type) : nullptr;

    layers.clear();
    if (def) {
        for (size_t i = 0; i < def->layers.size(); i++) {
            const StructureLayerDef& layerDef = def->layers[i];
            Layer out;
            out.objId = i < 2 ? data.layerObjIds[i] : 0;
            const bool underground = layerDef.hasUnderground ? layerDef.underground : (i == 1);
            const Vec2 lpos = v2Add(pos, layerDef.pos);
            const float lrot =
                math::oriToRad(layerDef.inheritOri ? (data.ori + layerDef.ori) : layerDef.ori);
            out.collision = colliderTransform(boundingFor(layerDef.type), lpos, lrot, 1.0f);
            out.underground = underground;
            layers.push_back(out);
        }
    }

    stairs.clear();
    if (def) {
        for (const auto& stairDef : def->stairs) {
            Stair s;
            s.collision = colliderTransform(stairDef.collision, pos, rot, scale);
            s.downDir = v2Rotate(stairDef.downDir, rot);
            const Collider aabbCol = colliderToAabb(s.collision);
            Collider child0, child1;
            splitAabb(aabbCol.min, aabbCol.max, s.downDir, child0, child1);
            s.center = v2Midpoint(aabbCol.min, aabbCol.max);
            s.downAabb = child0;
            s.upAabb = child1;
            s.noCeilingReveal = stairDef.noCeilingReveal;
            s.lootOnly = stairDef.lootOnly;
            stairs.push_back(s);
        }
    }

    mask.clear();
    if (def) {
        for (const auto& m : def->mask) {
            mask.push_back(colliderTransform(m, pos, rot, scale));
        }
    }
    ctx.renderer().layerMaskDirty = true;
}

void Structure::update(float dt, Ctx& ctx) {
    // structure.ts updateInteriorSounds(): drive the two immediate-mode
    // Ambiance tracks from the building distance + ceiling fade.
    const DefProvider* provider = getDefProvider();
    const MapObjectDef* def = provider ? provider->mapObject(type) : nullptr;
    if (!def || !def->hasInteriorSound) {
        return;
    }
    Player* ap = ctx.activePlayer();
    audio::Ambiance* ambience = ctx.ambiance();
    if (!ap || !ambience) {
        return;
    }
    const InteriorSoundDef& snd = def->interiorSound;

    Building* building0 = !layers.empty() ? ctx.map().getBuildingById(layers[0].objId) : nullptr;
    Building* building1 = layers.size() > 1 ? ctx.map().getBuildingById(layers[1].objId) : nullptr;

    float weight0 = 0.0f;
    float weight1 = 0.0f;
    if (ap->layer != 1) {
        if (building0) {
            const float dist = building0->getDistanceToBuilding(ap->pos, snd.outsideMaxDist);
            const float weight = math::remap(dist, snd.outsideMaxDist, 0.0f, 0.0f, 1.0f);
            const bool onStairs = (ap->layer & 2) != 0;
            const float visionT = building0->ceilingFadeAlpha;
            weight0 = weight * (1.0f - visionT);
            weight1 = weight * visionT *
                (onStairs ? snd.undergroundVolume : snd.outsideVolume);
        }
    } else if (building1) {
        // Immediately play the filtered track at full weight underground.
        const float dist = building1->getDistanceToBuilding(ap->pos, snd.outsideMaxDist);
        const float weight = math::remap(dist, snd.outsideMaxDist, 0.0f, 0.0f, 1.0f);
        weight0 = 0.0f;
        weight1 = weight * snd.undergroundVolume;
    }

    // Transition between the normal and alternative tracks.
    const float transitionTime = snd.transitionTime > 0.0f ? snd.transitionTime : 1.0f;
    if (interiorSoundAlt) {
        soundTransitionT = math::clamp(soundTransitionT + dt / transitionTime, 0.0f, 1.0f);
    }
    const float transitionWeight = std::fabs(soundTransitionT - 0.5f) * 2.0f;

    // Fade out the track after it is disabled.
    if (!interiorSoundEnabled) {
        soundEnabledT = math::clamp(soundEnabledT - dt * 0.5f, 0.0f, 1.0f);
    }

    const std::string& sound = soundTransitionT > 0.5f ? snd.soundAlt : snd.sound;

    ambience->setTrackSound("interior_0", sound);
    ambience->setTrackFilter("interior_0", "");
    ambience->setTrackWeight("interior_0",
                             sound.empty() ? 0.0f : weight0 * transitionWeight * soundEnabledT);

    ambience->setTrackSound("interior_1", sound);
    ambience->setTrackFilter("interior_1", snd.filter);
    ambience->setTrackWeight("interior_1",
                             sound.empty() ? 0.0f : weight1 * transitionWeight * soundEnabledT);
}

bool Structure::insideStairs(const Collider& c) const {
    for (const auto& s : stairs) {
        if (colliderIntersect(s.collision, c)) {
            return true;
        }
    }
    return false;
}

bool Structure::insideMask(const Collider& c) const {
    for (const auto& m : mask) {
        if (colliderIntersect(m, c)) {
            return true;
        }
    }
    return false;
}

} // namespace surv
