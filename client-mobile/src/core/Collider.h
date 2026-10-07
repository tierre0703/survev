#pragma once
#include "MathUtil.h"
#include "Vec2.h"
#include <limits>
#include <vector>

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

// --- collider.ts helpers (ported 1:1) -------------------------------------

inline Collider colliderCopy(const Collider& c) {
    return c.type == Collider::Circle ? Collider::createCircle(c.pos, c.rad)
                                      : Collider::createAabb(c.min, c.max);
}

inline Collider circleToAabb(const Vec2& pos, float rad) {
    const Vec2 extent(rad, rad);
    return Collider::createAabb(v2Sub(pos, extent), v2Add(pos, extent));
}

inline Collider colliderToAabb(const Collider& c) {
    if (c.type == Collider::Aabb) {
        return Collider::createAabb(c.min, c.max);
    }
    return circleToAabb(c.pos, c.rad);
}

inline Collider createAabbExtents(const Vec2& pos, const Vec2& extent) {
    return Collider::createAabb(v2Sub(pos, extent), v2Add(pos, extent));
}

inline Collider colliderTransform(const Collider& col, const Vec2& pos, float rot, float scale) {
    if (col.type == Collider::Aabb) {
        const Vec2 e = v2Mul(v2Sub(col.max, col.min), 0.5f);
        const Vec2 c = v2Add(col.min, e);
        const Vec2 pts[4] = {
            Vec2(c.x - e.x, c.y - e.y),
            Vec2(c.x - e.x, c.y + e.y),
            Vec2(c.x + e.x, c.y - e.y),
            Vec2(c.x + e.x, c.y + e.y),
        };
        Vec2 mn(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
        Vec2 mx(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
        for (int i = 0; i < 4; i++) {
            const Vec2 p = v2Add(v2Rotate(v2Mul(pts[i], scale), rot), pos);
            mn.x = math::min(mn.x, p.x);
            mn.y = math::min(mn.y, p.y);
            mx.x = math::max(mx.x, p.x);
            mx.y = math::max(mx.y, p.y);
        }
        return Collider::createAabb(mn, mx);
    }
    return Collider::createCircle(
        v2Add(v2Rotate(v2Mul(col.pos, scale), rot), pos),
        col.rad * scale);
}

inline bool intersectCircleCircle(const Vec2& pos0, float rad0, const Vec2& pos1, float rad1) {
    const Vec2 d = v2Sub(pos0, pos1);
    const float distSq = v2LengthSqr(d);
    const float radSum = rad0 + rad1;
    return distSq <= radSum * radSum;
}

inline bool intersectAabbCircle(const Vec2& min, const Vec2& max, const Vec2& pos, float rad) {
    const float x = math::clamp(pos.x, min.x, max.x);
    const float y = math::clamp(pos.y, min.y, max.y);
    const float dx = pos.x - x;
    const float dy = pos.y - y;
    return dx * dx + dy * dy <= rad * rad;
}

inline bool intersectAabbAabb(const Vec2& min0, const Vec2& max0, const Vec2& min1, const Vec2& max1) {
    return min0.x <= max1.x && min0.y <= max1.y && min1.x <= max0.x && min1.y <= max0.y;
}

inline bool colliderIntersectCircle(const Collider& col, const Vec2& pos, float rad) {
    if (col.type == Collider::Aabb) {
        return intersectAabbCircle(col.min, col.max, pos, rad);
    }
    return intersectCircleCircle(col.pos, col.rad, pos, rad);
}

// Full collider-vs-collider intersection matching collider.intersect().
inline bool colliderIntersect(const Collider& a, const Collider& b) {
    if (b.type == Collider::Aabb) {
        if (a.type == Collider::Aabb) {
            return intersectAabbAabb(a.min, a.max, b.min, b.max);
        }
        return intersectAabbCircle(b.min, b.max, a.pos, a.rad);
    }
    if (a.type == Collider::Aabb) {
        return intersectAabbCircle(a.min, a.max, b.pos, b.rad);
    }
    return intersectCircleCircle(a.pos, a.rad, b.pos, b.rad);
}

inline Vec2 clampPosToAabb(const Vec2& pos, const Vec2& min, const Vec2& max) {
    return Vec2(math::clamp(pos.x, min.x, max.x), math::clamp(pos.y, min.y, max.y));
}

inline bool testPointAabb(const Vec2& pos, const Vec2& min, const Vec2& max) {
    return pos.x >= min.x && pos.x <= max.x && pos.y >= min.y && pos.y <= max.y;
}

// coldet.intersectCircleCircle: returns the push direction + penetration.
// Callers must check the boolean return before reading `out`.
inline bool intersectCircleCircleRes(const Vec2& pos0, float rad0, const Vec2& pos1, float rad1,
                                     Vec2* dirOut, float* penOut) {
    const float r = rad0 + rad1;
    const Vec2 toP1 = v2Sub(pos1, pos0);
    const float distSqr = v2LengthSqr(toP1);
    if (distSqr < r * r) {
        const float dist = std::sqrt(distSqr);
        *dirOut = dist > 0.00001f ? v2Div(toP1, dist) : Vec2(1.0f, 0.0f);
        *penOut = r - dist;
        return true;
    }
    return false;
}

// coldet.intersectAabbCircle: returns the push direction + penetration.
inline bool intersectAabbCircleRes(const Vec2& min, const Vec2& max, const Vec2& pos, float rad,
                                   Vec2* dirOut, float* penOut) {
    if (pos.x >= min.x && pos.x <= max.x && pos.y >= min.y && pos.y <= max.y) {
        const Vec2 e = v2Mul(v2Sub(max, min), 0.5f);
        const Vec2 c = v2Add(min, e);
        const Vec2 p = v2Sub(pos, c);
        const float xp = std::fabs(p.x) - e.x - rad;
        const float yp = std::fabs(p.y) - e.y - rad;
        if (xp > yp) {
            *dirOut = Vec2(p.x > 0.0f ? 1.0f : -1.0f, 0.0f);
            *penOut = -xp;
        } else {
            *dirOut = Vec2(0.0f, p.y > 0.0f ? 1.0f : -1.0f);
            *penOut = -yp;
        }
        return true;
    }
    const Vec2 cpt(math::clamp(pos.x, min.x, max.x), math::clamp(pos.y, min.y, max.y));
    const Vec2 dir = v2Sub(pos, cpt);
    const float dstSqr = v2LengthSqr(dir);
    if (dstSqr < rad * rad) {
        const float dst = std::sqrt(dstSqr);
        *dirOut = dst > 0.0001f ? v2Div(dir, dst) : Vec2(1.0f, 0.0f);
        *penOut = rad - dst;
        return true;
    }
    return false;
}

// collider.intersectCircle(col, pos, rad) with the push result (coldet forms).
inline bool colliderIntersectCircleRes(const Collider& col, const Vec2& pos, float rad,
                                       Vec2* dirOut, float* penOut) {
    if (col.type == Collider::Aabb) {
        return intersectAabbCircleRes(col.min, col.max, pos, rad, dirOut, penOut);
    }
    return intersectCircleCircleRes(col.pos, col.rad, pos, rad, dirOut, penOut);
}

// coldet.intersectSegmentCircle: nearest point on segment s0->s1 inside circle.
inline bool intersectSegmentCircleRes(const Vec2& s0, const Vec2& s1, const Vec2& pos, float rad,
                                      Vec2* pointOut) {
    Vec2 d = v2Sub(s1, s0);
    const float len = math::max(v2Length(d), 0.000001f);
    d = v2Div(d, len);
    const Vec2 m = v2Sub(s0, pos);
    const float b = v2Dot(m, d);
    const float c = v2Dot(m, m) - rad * rad;
    if (c > 0.0f && b > 0.0f) {
        return false;
    }
    const float discSq = b * b - c;
    if (discSq < 0.0f) {
        return false;
    }
    const float disc = std::sqrt(discSq);
    float t = -b - disc;
    if (t < 0.0f) {
        t = -b + disc;
    }
    if (t <= len) {
        *pointOut = v2Add(s0, v2Mul(d, t));
        return true;
    }
    return false;
}

// coldet.intersectSegmentAabb: nearest segment/AABB hit point.
inline bool intersectSegmentAabbRes(const Vec2& s0, const Vec2& s1, const Vec2& min, const Vec2& max,
                                    Vec2* pointOut) {
    float tmin = 0.0f;
    float tmax = 3.402823466e+38f;
    const float eps = 0.00001f;
    const Vec2 r = s0;
    Vec2 d = v2Sub(s1, s0);
    const float dist = v2Length(d);
    d = dist > eps ? v2Div(d, dist) : Vec2(1.0f, 0.0f);

    float absDx = std::fabs(d.x);
    float absDy = std::fabs(d.y);
    if (absDx < eps) {
        d.x = eps * 2.0f;
        absDx = d.x;
    }
    if (absDy < eps) {
        d.y = eps * 2.0f;
        absDy = d.y;
    }
    if (absDx > eps) {
        const float tx1 = (min.x - r.x) / d.x;
        const float tx2 = (max.x - r.x) / d.x;
        tmin = math::max(tmin, math::min(tx1, tx2));
        tmax = math::min(tmax, math::max(tx1, tx2));
        if (tmin > tmax) {
            return false;
        }
    }
    if (absDy > eps) {
        const float ty1 = (min.y - r.y) / d.y;
        const float ty2 = (max.y - r.y) / d.y;
        tmin = math::max(tmin, math::min(ty1, ty2));
        tmax = math::min(tmax, math::max(ty1, ty2));
        if (tmin > tmax) {
            return false;
        }
    }
    if (tmin > dist) {
        return false;
    }
    *pointOut = v2Add(s0, v2Mul(d, tmin));
    return true;
}

// collider.intersectSegment(col, a, b) -> nearest hit point.
inline bool colliderIntersectSegmentRes(const Collider& col, const Vec2& a, const Vec2& b,
                                        Vec2* pointOut) {
    if (col.type == Collider::Aabb) {
        return intersectSegmentAabbRes(a, b, col.min, col.max, pointOut);
    }
    return intersectSegmentCircleRes(a, b, col.pos, col.rad, pointOut);
}

inline Collider boundingAabb(const std::vector<Collider>& colliders) {
    Vec2 mn(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2 mx(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    for (const auto& c : colliders) {
        const Collider a = colliderToAabb(c);
        mn.x = math::min(mn.x, a.min.x);
        mn.y = math::min(mn.y, a.min.y);
        mx.x = math::max(mx.x, a.max.x);
        mx.y = math::max(mx.y, a.max.y);
    }
    return Collider::createAabb(mn, mx);
}

// coldet.splitAabb: splits along axis into [toward axis, away from axis].
inline void splitAabb(const Vec2& min, const Vec2& max, const Vec2& axis,
                      Collider& out0, Collider& out1) {
    const Vec2 e = v2Mul(v2Sub(max, min), 0.5f);
    const Vec2 c = v2Add(min, e);
    Vec2 leftMin = min, leftMax = max, rightMin = min, rightMax = max;
    if (std::fabs(axis.y) > std::fabs(axis.x)) {
        leftMax = Vec2(max.x, c.y);
        rightMin = Vec2(min.x, c.y);
    } else {
        leftMax = Vec2(c.x, max.y);
        rightMin = Vec2(c.x, min.y);
    }
    const Vec2 dir = v2Sub(max, min);
    if (v2Dot(dir, axis) > 0.0f) {
        out0 = Collider::createAabb(rightMin, rightMax);
        out1 = Collider::createAabb(leftMin, leftMax);
    } else {
        out0 = Collider::createAabb(leftMin, leftMax);
        out1 = Collider::createAabb(rightMin, rightMax);
    }
}

} // namespace surv
