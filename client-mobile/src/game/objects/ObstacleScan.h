#pragma once
// Port of shared/utils/collisionHelpers.ts: obstacle-filtered segment casts
// and the scan used by building.ts ceiling reveals.
//
// The helpers live in a header because both player.ts (melee cleave, line of
// sight) and building.ts (ceiling vision) use them; `Obstacle` is the pooled
// obstacle object from Barns.h.
#include "Barns.h"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace surv {

// shared/utils/util.sameLayer.
inline bool scanSameLayer(int a, int b) {
    return ((a & 0x1) == (b & 0x1)) || (((a & 0x2) != 0) && ((b & 0x2) != 0));
}

// collisionHelpers.intersectSegmentObstacle: filters + nearest hit point.
inline bool intersectSegmentObstacle(const Obstacle& o, const Vec2& s0, const Vec2& s1, float height,
                                     int layer, bool hackStairs, Vec2* pointOut) {
    if (!o.active || o.dead || !o.collidable || o.isWindow || o.height < height ||
        !scanSameLayer(o.layer, layer)) {
        return false;
    }
    if (hackStairs && (layer & 0x2) != 0 && o.layer == 0) {
        return false;
    }
    return colliderIntersectSegmentRes(o.collider, s0, s1, pointOut);
}

// collisionHelpers.intersectSegment: nearest obstacle along pos + dir*len.
// Returns true and writes the hit id + distance.
inline bool intersectSegmentNearest(const std::vector<Obstacle*>& obstacles, const Vec2& pos,
                                    const Vec2& dir, float len, float height, int layer,
                                    bool hackStairs, uint16_t* idOut, float* distOut) {
    const Vec2 end = v2Add(pos, v2Mul(dir, len));
    bool found = false;
    float bestDist = 0.0f;
    uint16_t bestId = 0;
    for (auto* o : obstacles) {
        if (!o) {
            continue;
        }
        Vec2 point;
        if (intersectSegmentObstacle(*o, pos, end, height, layer, hackStairs, &point)) {
            const float dist = v2Length(v2Sub(point, pos));
            if (!found || dist < bestDist) {
                found = true;
                bestDist = dist;
                bestId = o->__id;
            }
        }
    }
    *idOut = bestId;
    *distOut = bestDist;
    return found;
}

// collisionHelpers.intersectSegmentDist: nearest obstacle distance (defaults len).
inline float intersectSegmentDist(const std::vector<Obstacle*>& obstacles, const Vec2& pos,
                                  const Vec2& dir, float len, float height, int layer,
                                  bool hackStairs) {
    float dist = len;
    const Vec2 end = v2Add(pos, v2Mul(dir, len));
    for (auto* o : obstacles) {
        if (!o) {
            continue;
        }
        Vec2 point;
        if (intersectSegmentObstacle(*o, pos, end, height, layer, hackStairs, &point)) {
            dist = math::min(dist, v2Length(v2Sub(point, pos)));
        }
    }
    return dist;
}

inline Vec2 getIntersectSegmentEnd(const std::vector<Obstacle*>& obstacles, const Vec2& pos,
                                   const Vec2& dir, float len, int layer) {
    const float dist = intersectSegmentDist(obstacles, pos, dir, len, 0.0f, layer, false);
    return v2Add(pos, v2Mul(dir, dist));
}

// collisionHelpers.scanCollider(): cast `rayCount` parallel rays from `pos`
// toward `col` and report whether any of them reaches it unobstructed.
inline bool scanCollider(const Collider& col, const std::vector<Obstacle*>& obstacles,
                         const Vec2& pos, int layer, float height, float scanWidth,
                         float scanDist, int rayCount) {
    Vec2 toDir;
    float toPen = 0.0f;
    if (!colliderIntersectCircleRes(col, pos, scanDist, &toDir, &toPen)) {
        return false;
    }
    if (toPen >= scanDist) {
        // Inside the collider.
        return true;
    }

    const Vec2 perp = v2Perp(toDir);
    const Vec2 scanStart = getIntersectSegmentEnd(obstacles, pos, v2Neg(perp), 0.5f * scanWidth, layer);
    const Vec2 scanEnd = getIntersectSegmentEnd(obstacles, pos, perp, 0.5f * scanWidth, layer);
    Vec2 scanDir = v2Sub(scanEnd, scanStart);
    const float scanLen = v2Length(scanDir);
    scanDir = scanLen > 0.0001f ? v2Div(scanDir, scanLen) : Vec2(1.0f, 0.0f);

    const int rays = std::max(rayCount, 1);
    for (int i = 0; i < rays; i++) {
        const float t = rays > 1 ? static_cast<float>(i) / static_cast<float>(rays - 1) : 0.0f;
        const Vec2 rayPos = v2Add(scanStart, v2Mul(scanDir, scanLen * t));

        Vec2 circleDir;
        float circlePen = 0.0f;
        if (!colliderIntersectCircleRes(col, rayPos, scanDist, &circleDir, &circlePen)) {
            continue;
        }

        const Vec2 rayDir = v2Neg(circleDir);
        const float maxDist =
            intersectSegmentDist(obstacles, rayPos, rayDir, scanDist, height, layer, true);
        Vec2 hitPoint;
        const bool hit = colliderIntersectSegmentRes(
            col, rayPos, v2Add(rayPos, v2Mul(rayDir, scanDist)), &hitPoint);
        const float dist = hit ? v2Length(v2Sub(hitPoint, rayPos)) : 0.0f;
        if (hit && dist <= maxDist) {
            return true;
        }
    }
    return false;
}

} // namespace surv
