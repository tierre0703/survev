#pragma once
#include "Vec2.h"

namespace surv {

// Mirrors shared/utils/coldet.ts Collider union + collider.ts Type enum.
struct Collider {
    enum Type : uint8_t {
        Circle = 0,
        Aabb = 1,
    };

    Type type = Aabb;
    Vec2 pos; // circle center
    float rad = 0.0f;
    Vec2 min; // aabb min
    Vec2 max; // aabb max

    static Collider createCircle(const Vec2& pos, float rad) {
        Collider c;
        c.type = Type::Circle;
        c.pos = pos;
        c.rad = rad;
        return c;
    }

    static Collider createAabb(const Vec2& min, const Vec2& max) {
        Collider c;
        c.type = Type::Aabb;
        c.min = min;
        c.max = max;
        return c;
    }
};

} // namespace surv