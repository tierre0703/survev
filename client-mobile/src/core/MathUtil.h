#pragma once
#include "Vec2.h"
#include <algorithm>
#include <cmath>

namespace surv {

constexpr float kEpsilon = 0.000001f;

namespace math {

inline float clamp(float a, float min, float max) {
    return a < max ? (a > min ? a : min) : max;
}

inline float min(float a, float b) {
    return a < b ? a : b;
}

inline float max(float a, float b) {
    return a > b ? a : b;
}

inline float lerp(float t, float a, float b) {
    return a * (1.0f - t) + b * t;
}

inline float delerp(float t, float a, float b) {
    return clamp((t - a) / (b - a), 0.0f, 1.0f);
}

inline float smoothstep(float v, float a, float b) {
    const float t = clamp((v - a) / (b - a), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

inline float easeOutElastic(float e, float t = 0.3f) {
    return std::pow(2.0f, e * -10.0f) * std::sin(((e - t / 4.0f) * (3.14159265358979f * 2.0f)) / t) + 1.0f;
}

inline float easeOutExpo(float e) {
    if (e == 1.0f) {
        return 1.0f;
    }
    return 1.0f - std::pow(2.0f, e * -10.0f);
}

inline float easeInExpo(float e) {
    if (e == 0.0f) {
        return 0.0f;
    }
    return std::pow(2.0f, (e - 1.0f) * 10.0f);
}

inline float easeInSine(float x) {
    return 1.0f - std::cos((x * 3.14159265358979f) / 2.0f);
}

inline float easeOutSine(float x) {
    return std::sin((x * 3.14159265358979f) / 2.0f);
}

inline float easeInOutSine(float x) {
    return -(std::cos(3.14159265358979f * x) - 1.0f) / 2.0f;
}

inline float easeOutQuad(float x) {
    return 1.0f - (1.0f - x) * (1.0f - x);
}

inline float easeOutQuart(float e) {
    return 1.0f - std::pow(1.0f - e, 4.0f);
}

inline float easeOutBounce(float x) {
    const float n1 = 7.5625f;
    const float d1 = 2.75f;
    if (x < 1.0f / d1) {
        return n1 * x * x;
    } else if (x < 2.0f / d1) {
        x -= 1.5f / d1;
        return n1 * x * x + 0.75f;
    } else if (x < 2.5f / d1) {
        x -= 2.25f / d1;
        return n1 * x * x + 0.9375f;
    } else {
        x -= 2.625f / d1;
        return n1 * x * x + 0.984375f;
    }
}

inline float remap(float v, float a, float b, float x, float y) {
    const float t = clamp((v - a) / (b - a), 0.0f, 1.0f);
    return lerp(t, x, y);
}

inline bool eqAbs(float a, float b, float eps = kEpsilon) {
    return std::fabs(a - b) < eps;
}

inline bool eqRel(float a, float b, float eps = kEpsilon) {
    return std::fabs(a - b) <= eps * std::max(1.0f, std::max(std::fabs(a), std::fabs(b)));
}

inline float deg2rad(float deg) {
    return (deg * 3.14159265358979f) / 180.0f;
}

inline Vec2 deg2vec2(float deg) {
    deg *= 3.14159265358979f / 180.0f;
    return Vec2(std::cos(deg), std::sin(deg));
}

inline float rad2deg(float rad) {
    return (rad * 180.0f) / 3.14159265358979f;
}

inline Vec2 rad2Direction(float rad) {
    return Vec2(std::cos(rad), std::sin(rad));
}

inline float rad2degFromDirection(float y, float x) {
    const float rad = std::atan2(y, x);
    float angle = (rad * 180.0f) / 3.14159265358979f;
    if (angle < 0.0f) {
        angle += 360.0f;
    }
    return angle;
}

inline float fract(float n) {
    return n - std::floor(n);
}

inline float sign(float n) {
    return n < 0.0f ? -1.0f : 1.0f;
}

inline float mod(float num, float n) {
    return std::fmod(std::fmod(num, n) + n, n);
}

inline float fmod(float num, float n) {
    return num - std::floor(num / n) * n;
}

inline float angleDiff(float a, float b) {
    float d = fmod(b - a + 3.14159265358979f, 3.14159265358979f * 2.0f) - 3.14159265358979f;
    return d < -3.14159265358979f ? d + 3.14159265358979f * 2.0f : d;
}

inline float oriToRad(int ori) {
    return (ori % 4) * 0.5f * 3.14159265358979f;
}

inline float oriToAngle(int ori) {
    return ori * (180.0f / 3.14159265358979f);
}

inline int radToOri(float rad) {
    return static_cast<int>(std::floor(fmod(rad + 3.14159265358979f * 0.25f, 3.14159265358979f * 2.0f) / (3.14159265358979f * 0.5f)));
}

inline float quantize(float f, float min, float max, int bits) {
    const int range = (1 << bits) - 1;
    const float x = clamp(f, min, max);
    const float t = (x - min) / (max - min);
    const float a = t * static_cast<float>(range) + 0.5f;
    const float b = a < 0.0f ? std::ceil(a) : std::floor(a);
    return min + (b / static_cast<float>(range)) * (max - min);
}

inline Vec2 v2Quantize(const Vec2& v, float minX, float minY, float maxX, float maxY, int bits) {
    return Vec2(quantize(v.x, minX, maxX, bits), quantize(v.y, minY, maxY, bits));
}

// Ray-line intersection. Returns true and sets tOut if intersected.
inline bool rayLineIntersect(const Vec2& origin, const Vec2& direction, const Vec2& lineA, const Vec2& lineB, float* tOut) {
    const Vec2 segment = v2Sub(lineB, lineA);
    const Vec2 segmentPerp(segment.y, -segment.x);
    const float perpDotDir = v2Dot(direction, segmentPerp);

    // Parallel lines, no intersection
    if (std::fabs(perpDotDir) <= kEpsilon) {
        return false;
    }

    const Vec2 d = v2Sub(lineA, origin);

    // Distance of intersection along ray
    const float t = v2Dot(segmentPerp, d) / perpDotDir;

    // Distance of intersection along line
    const float s = v2Dot(Vec2(direction.y, -direction.x), d) / perpDotDir;

    if (t >= 0.0f && s >= 0.0f && s <= 1.0f) {
        *tOut = t;
        return true;
    }
    return false;
}

// Returns true if intersected, sets tOut to closest intersection distance.
inline bool rayPolygonIntersect(const Vec2& origin, const Vec2& direction, const Vec2* vertices, int count, float* tOut) {
    float t = 3.402823466e+38f;
    bool intersected = false;
    for (int i = 0, j = count - 1; i < count; j = i++) {
        float distance = 0.0f;
        if (rayLineIntersect(origin, direction, vertices[j], vertices[i], &distance)) {
            if (distance < t) {
                intersected = true;
                t = distance;
            }
        }
    }
    if (intersected) {
        *tOut = t;
    }
    return intersected;
}

// Ray-casting algorithm point-in-polygon test.
inline bool pointInsidePolygon(const Vec2& point, const Vec2* poly, int count) {
    const float x = point.x;
    const float y = point.y;
    bool inside = false;
    for (int i = 0, j = count - 1; i < count; j = i++) {
        const float xi = poly[i].x;
        const float yi = poly[i].y;
        const float xj = poly[j].x;
        const float yj = poly[j].y;

        const bool intersect = (yi > y) != (yj > y) && x < ((xj - xi) * (y - yi)) / (yj - yi) + xi;
        if (intersect) {
            inside = !inside;
        }
    }
    return inside;
}

inline float distToSegmentSq(const Vec2& p, const Vec2& a, const Vec2& b) {
    const Vec2 ab = v2Sub(b, a);
    const float c = v2Dot(v2Sub(p, a), ab) / v2Dot(ab, ab);
    const Vec2 d = v2Add(a, v2Mul(ab, clamp(c, 0.0f, 1.0f)));
    const Vec2 e = v2Sub(d, p);
    return v2Dot(e, e);
}

inline float distToPolygon(const Vec2& p, const Vec2* poly, int count) {
    float closestDistSq = 3.402823466e+38f;
    for (int i = 0; i < count; i++) {
        const Vec2& a = poly[i];
        const Vec2& b = i == count - 1 ? poly[0] : poly[i + 1];
        const float distSq = distToSegmentSq(p, a, b);
        if (distSq < closestDistSq) {
            closestDistSq = distSq;
        }
    }
    return std::sqrt(closestDistSq);
}

inline Vec2 transformSegmentP0(const Vec2& p0, const Vec2& pos, const Vec2& dir) {
    const float ang = std::atan2(dir.y, dir.x);
    return v2Add(pos, v2Rotate(p0, ang));
}

inline Vec2 transformSegmentP1(const Vec2& p1, const Vec2& pos, const Vec2& dir) {
    const float ang = std::atan2(dir.y, dir.x);
    return v2Add(pos, v2Rotate(p1, ang));
}

inline Vec2 addAdjust(const Vec2& pos1, const Vec2& pos, int ori) {
    if (ori == 0) {
        return v2Add(pos1, pos);
    }
    float xOffset;
    float yOffset;
    switch (ori) {
        case 1:
            xOffset = -pos.y;
            yOffset = pos.x;
            break;
        case 2:
            xOffset = -pos.x;
            yOffset = -pos.y;
            break;
        case 3:
            xOffset = pos.y;
            yOffset = -pos.x;
            break;
        default:
            xOffset = 0.0f;
            yOffset = 0.0f;
            break;
    }
    return v2Add(pos1, Vec2(xOffset, yOffset));
}

inline Vec2 v2Clamp(const Vec2& vector, const Vec2& minV2, const Vec2& maxV2) {
    const float minX = minV2.x > maxV2.x ? maxV2.x : minV2.x;
    const float maxX = minV2.x > maxV2.x ? minV2.x : maxV2.x;
    const float minY = minV2.y > maxV2.y ? maxV2.y : minV2.y;
    const float maxY = minV2.y > maxV2.y ? minV2.y : maxV2.y;
    const float resX = vector.x < maxX ? (vector.x > minX ? vector.x : minX) : maxX;
    const float resY = vector.y < maxY ? (vector.y > minY ? vector.y : minY) : maxY;
    return Vec2(resX, resY);
}

} // namespace math

} // namespace surv