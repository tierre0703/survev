#pragma once
// Port of client/src/objects/structure.ts (the render-relevant parts).
#include "../../core/Collider.h"
#include "../../core/Vec2.h"
#include "GameObject.h"
#include <string>
#include <vector>

namespace surv {

struct Stair {
    Collider collision;
    Vec2 center;
    Vec2 downDir;
    Collider downAabb;
    Collider upAabb;
    bool noCeilingReveal = false;
    bool lootOnly = false;
};

class Structure : public AbstractObject {
public:
    float soundTransitionT = 0.0f;
    float soundEnabledT = 0.0f;

    std::string type;
    int layer = 0;
    Vec2 pos;
    float rot = 0.0f;
    float scale = 1.0f;
    bool interiorSoundAlt = false;
    bool interiorSoundEnabled = false;

    Collider aabb;

    struct Layer {
        uint16_t objId = 0;
        Collider collision;
        bool underground = false;
    };
    std::vector<Layer> layers;
    std::vector<Stair> stairs;
    std::vector<Collider> mask;

    void m_init() override;
    void m_free() override;
    void m_updateData(const ObjectData& data, bool fullUpdate, bool isNew, Ctx& ctx) override;

    bool insideStairs(const Collider& c) const;
    bool insideMask(const Collider& c) const;
};

} // namespace surv
