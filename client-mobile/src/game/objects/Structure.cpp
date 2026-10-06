#include "Structure.h"

#include "../GameWorld.h"
#include "../../render/Defs.h"
#include "../../render/Renderer.h"

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
            const bool underground = layerDef.underground;
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
