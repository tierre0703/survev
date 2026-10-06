#include "Terrain.h"

#include "../core/GameConfig.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace surv {

// ---------------------------------------------------------------------------
// spline.ts
// ---------------------------------------------------------------------------

SplineControlPoints getControlPoints(float t, const std::vector<Vec2>& points, bool looped) {
    const int count = static_cast<int>(points.size());
    int i = 0, i0 = 0, i1 = 0, i2 = 0, i3 = 0;
    if (looped) {
        // Assume that with looped rails, points 0 and count are the same.
        t = math::fmod(t, 1.0f);
        i = static_cast<int>(t * (count - 1));
        i1 = i;
        i2 = (i1 + 1) % (count - 1);
        i0 = i1 > 0 ? i1 - 1 : count - 2;
        i3 = (i2 + 1) % (count - 1);
    } else {
        t = math::clamp(t, 0.0f, 1.0f);
        i = static_cast<int>(t * (count - 1));
        i1 = i == count - 1 ? i - 1 : i;
        i2 = i1 + 1;
        i0 = i1 > 0 ? i1 - 1 : i1;
        i3 = i2 < count - 1 ? i2 + 1 : i2;
    }
    SplineControlPoints out;
    out.pt = t * (count - 1) - static_cast<float>(i1);
    out.p0 = points[i0];
    out.p1 = points[i1];
    out.p2 = points[i2];
    out.p3 = points[i3];
    return out;
}

float catmullRom(float t, float p0, float p1, float p2, float p3) {
    return 0.5f *
           (2.0f * p1 + t * (-p0 + p2) + t * t * (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) +
            t * t * t * (-p0 + 3.0f * p1 - 3.0f * p2 + p3));
}

static float catmullRomDerivative(float t, float p0, float p1, float p2, float p3) {
    return 0.5f *
           (-p0 + p2 + 2.0f * t * (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) +
            3.0f * t * t * (-p0 + 3.0f * p1 - 3.0f * p2 + p3));
}

Spline::Spline(const std::vector<Vec2>& src, bool looped_) : looped(looped_) {
    totalArcLen = 0.0f;
    points = src;

    const int arcLenSamples = static_cast<int>(points.size()) * 4;
    arcLens.assign(static_cast<size_t>(arcLenSamples) + 1, 0.0f);
    Vec2 cur = points[0];
    for (int i = 0; i <= arcLenSamples; i++) {
        const float t = static_cast<float>(i) / static_cast<float>(arcLenSamples);
        const Vec2 next = getPos(t);
        const float arcLenPrev = i == 0 ? 0.0f : arcLens[static_cast<size_t>(i - 1)];
        arcLens[static_cast<size_t>(i)] = arcLenPrev + v2Length(v2Sub(next, cur));
        cur = next;
    }
    totalArcLen = arcLens.back();
}

Vec2 Spline::getPos(float t) const {
    const SplineControlPoints c = getControlPoints(t, points, looped);
    return Vec2(catmullRom(c.pt, c.p0.x, c.p1.x, c.p2.x, c.p3.x),
                catmullRom(c.pt, c.p0.y, c.p1.y, c.p2.y, c.p3.y));
}

Vec2 Spline::getTangent(float t) const {
    const SplineControlPoints c = getControlPoints(t, points, looped);
    return Vec2(catmullRomDerivative(c.pt, c.p0.x, c.p1.x, c.p2.x, c.p3.x),
                catmullRomDerivative(c.pt, c.p0.y, c.p1.y, c.p2.y, c.p3.y));
}

Vec2 Spline::getNormal(float t) const {
    const Vec2 tangent = getTangent(t);
    return v2Perp(v2NormalizeSafe(tangent, Vec2(1.0f, 0.0f)));
}

float Spline::getClosestTtoPoint(const Vec2& pos) const {
    float closestDistSq = std::numeric_limits<float>::max();
    int closestSegIdx = 0;
    for (int i = 0; i < static_cast<int>(points.size()) - 1; i++) {
        const float distSq = math::distToSegmentSq(pos, points[static_cast<size_t>(i)],
                                                   points[static_cast<size_t>(i + 1)]);
        if (distSq < closestDistSq) {
            closestDistSq = distSq;
            closestSegIdx = i;
        }
    }
    const int idx0 = closestSegIdx;
    const int idx1 = idx0 + 1;
    const Vec2 s0 = points[static_cast<size_t>(idx0)];
    const Vec2 s1 = points[static_cast<size_t>(idx1)];
    const Vec2 seg = v2Sub(s1, s0);
    const float t = math::clamp(v2Dot(v2Sub(pos, s0), seg) / v2Dot(seg, seg), 0.0f, 1.0f);
    const float len = static_cast<float>(points.size() - 1);
    const float tMin = math::clamp((static_cast<float>(idx0) + t - 0.1f) / len, 0.0f, 1.0f);
    const float tMax = math::clamp((static_cast<float>(idx0) + t + 0.1f) / len, 0.0f, 1.0f);

    float nearestT = (static_cast<float>(idx0) + t) / len;
    float nearestDistSq = std::numeric_limits<float>::max();
    const int kIter = 8;
    for (int i = 0; i <= kIter; i++) {
        const float testT = math::lerp(static_cast<float>(i) / kIter, tMin, tMax);
        const Vec2 testPos = getPos(testT);
        const float testDistSq = v2LengthSqr(v2Sub(testPos, pos));
        if (testDistSq < nearestDistSq) {
            nearestT = testT;
            nearestDistSq = testDistSq;
        }
    }

    const Vec2 tangent = getTangent(nearestT);
    const float tanLen = v2Length(tangent);
    if (tanLen > 0.0f) {
        const Vec2 nearest = getPos(nearestT);
        const float offset = v2Dot(tangent, v2Sub(pos, nearest)) / tanLen;
        const float offsetT = nearestT + offset / (tanLen * len);
        if (v2LengthSqr(v2Sub(pos, getPos(offsetT))) < v2LengthSqr(v2Sub(pos, nearest))) {
            nearestT = offsetT;
        }
    }
    return nearestT;
}

float Spline::getTfromArcLen(float arcLen) const {
    arcLen = math::clamp(arcLen, 0.0f, totalArcLen);
    size_t idx = 0;
    while (idx < arcLens.size() && arcLen > arcLens[idx]) {
        idx++;
    }
    if (idx == 0) {
        return 0.0f;
    }
    const float arcT = math::delerp(arcLen, arcLens[idx - 1], arcLens[idx]);
    const float arcCount = static_cast<float>(arcLens.size() - 1);
    const float t0 = static_cast<float>(idx - 1) / arcCount;
    const float t1 = static_cast<float>(idx) / arcCount;
    return math::lerp(arcT, t0, t1);
}

float Spline::getArcLen(float t) const {
    t = math::clamp(t, 0.0f, 1.0f);
    const int arcCount = static_cast<int>(arcLens.size()) - 1;
    const int idx0 = static_cast<int>(std::floor(t * arcCount));
    const int idx1 = idx0 < arcCount - 1 ? idx0 + 1 : idx0;
    const float arcT = math::fmod(t, 1.0f / arcCount) / (1.0f / arcCount);
    return math::lerp(arcT, arcLens[static_cast<size_t>(idx0)], arcLens[static_cast<size_t>(idx1)]);
}

// ---------------------------------------------------------------------------
// river.ts
// ---------------------------------------------------------------------------

River::River(const std::vector<Vec2>& splinePts, float riverWidth, bool looped_,
             const std::vector<River>& otherRivers, const Collider& mapBounds)
    : spline(splinePts, looped_), waterWidth(riverWidth), looped(looped_) {
    shoreWidth = math::clamp(riverWidth * 0.75f, 4.0f, 8.0f);

    center = Vec2(0.0f, 0.0f);
    for (const auto& p : spline.points) {
        center = v2Add(center, p);
    }
    center = v2Div(center, static_cast<float>(spline.points.size()));

    float avgDistToCenter = 0.0f;
    for (const auto& p : spline.points) {
        avgDistToCenter += v2Length(v2Sub(p, center));
    }
    avgDistToCenter /= static_cast<float>(spline.points.size());

    const Vec2 mapExtent = v2Mul(v2Sub(mapBounds.max, mapBounds.min), 0.5f);
    const Vec2 mapCenter = v2Add(mapBounds.min, mapExtent);

    for (size_t i = 0; i < splinePts.size(); i++) {
        const Vec2 vert = splinePts[i];
        Vec2 norm = spline.getNormal(static_cast<float>(i) /
                                     static_cast<float>(splinePts.size() - 1));

        // If the endpoints are near the map boundary, adjust the normal to be
        // parallel to the map aabb at that point.
        bool nearMapEdge = false;
        if (!looped && (i == 0 || i == splinePts.size() - 1)) {
            const Vec2 e = v2Sub(vert, mapCenter);
            Vec2 edgePos(0.0f, 0.0f);
            Vec2 edgeNorm(1.0f, 0.0f);
            if (std::fabs(e.x) > std::fabs(e.y)) {
                edgePos = Vec2(e.x > 0.0f ? mapBounds.max.x : mapBounds.min.x, vert.y);
                edgeNorm = Vec2(e.x > 0.0f ? 1.0f : -1.0f, 0.0f);
            } else {
                edgePos = Vec2(vert.x, e.y > 0.0f ? mapBounds.max.y : mapBounds.min.y);
                edgeNorm = Vec2(0.0f, e.y > 0.0f ? 1.0f : -1.0f);
            }
            if (v2LengthSqr(v2Sub(edgePos, vert)) < 1.0f) {
                Vec2 perpNorm = v2Perp(edgeNorm);
                if (v2Dot(norm, perpNorm) < 0.0f) {
                    perpNorm = v2Neg(perpNorm);
                }
                norm = perpNorm;
                nearMapEdge = true;
            }
        }

        float waterWidth = this->waterWidth;
        if (!looped) {
            const float len = static_cast<float>(splinePts.size());
            const float end = 2.0f * (std::max(1.0f - static_cast<float>(i) / len,
                                               static_cast<float>(i) / len) - 0.5f);
            waterWidth = (1.0f + std::pow(end, 3.0f) * 1.5f) * this->waterWidth;
        }
        waterWidths.push_back(waterWidth);

        float shoreWidth = this->shoreWidth;
        const River* boundingRiver = nullptr;
        for (const auto& river : otherRivers) {
            const float t = river.spline.getClosestTtoPoint(vert);
            const Vec2 p = river.spline.getPos(t);
            const float d = v2Length(v2Sub(p, vert));
            if (d < river.waterWidth * 2.0f) {
                shoreWidth = math::max(shoreWidth, river.shoreWidth);
            }
            if ((i == 0 || i == splinePts.size() - 1) && d < 1.5f && !nearMapEdge) {
                boundingRiver = &river;
            }
        }
        if (i > 0) {
            shoreWidth = (shoreWidths[i - 1] + shoreWidth) / 2.0f;
        }
        shoreWidths.push_back(shoreWidth);
        shoreWidth += waterWidth;

        auto clipRayToPoly = [](const Vec2& pt, const Vec2& dir, const std::vector<Vec2>& poly) {
            const Vec2 end = v2Add(pt, dir);
            if (!math::pointInsidePolygon(end, poly.data(), static_cast<int>(poly.size()))) {
                float t = 0.0f;
                if (math::rayPolygonIntersect(pt, dir, poly.data(), static_cast<int>(poly.size()),
                                              &t)) {
                    return v2Mul(dir, t);
                }
            }
            return dir;
        };

        Vec2 waterPtA, waterPtB, shorePtA, shorePtB;
        if (looped) {
            Vec2 toVert = v2Sub(vert, center);
            const float d = v2Length(toVert);
            toVert = d > 0.0001f ? v2Div(toVert, d) : Vec2(1.0f, 0.0f);

            const float interiorWaterWidth = math::lerp(
                std::pow(math::min(waterWidth / avgDistToCenter, 1.0f), 0.5f), waterWidth,
                (1.0f - (avgDistToCenter - waterWidth) / d) * d);
            const float interiorShoreWidth = math::lerp(
                std::pow(math::min(shoreWidth / avgDistToCenter, 1.0f), 0.5f), shoreWidth,
                (1.0f - (avgDistToCenter - shoreWidth) / d) * d);

            waterPtA = v2Add(vert, v2Mul(toVert, waterWidth));
            waterPtB = v2Add(vert, v2Mul(toVert, -interiorWaterWidth));
            shorePtA = v2Add(vert, v2Mul(toVert, shoreWidth));
            shorePtB = v2Add(vert, v2Mul(toVert, -interiorShoreWidth));
        } else {
            Vec2 waterRayA = v2Mul(norm, waterWidth);
            Vec2 waterRayB = v2Mul(norm, -waterWidth);
            Vec2 shoreRayA = v2Mul(norm, shoreWidth);
            Vec2 shoreRayB = v2Mul(norm, -shoreWidth);

            if (boundingRiver) {
                waterRayA = clipRayToPoly(vert, waterRayA, boundingRiver->waterPoly);
                waterRayB = clipRayToPoly(vert, waterRayB, boundingRiver->waterPoly);
                shoreRayA = clipRayToPoly(vert, shoreRayA, boundingRiver->shorePoly);
                shoreRayB = clipRayToPoly(vert, shoreRayB, boundingRiver->shorePoly);
            }

            waterPtA = v2Add(vert, waterRayA);
            waterPtB = v2Add(vert, waterRayB);
            shorePtA = v2Add(vert, shoreRayA);
            shorePtB = v2Add(vert, shoreRayB);
        }

        waterPtA = clampPosToAabb(waterPtA, mapBounds.min, mapBounds.max);
        waterPtB = clampPosToAabb(waterPtB, mapBounds.min, mapBounds.max);
        shorePtA = clampPosToAabb(shorePtA, mapBounds.min, mapBounds.max);
        shorePtB = clampPosToAabb(shorePtB, mapBounds.min, mapBounds.max);

        // splice(i, 0, A) then splice(len - i, 0, B), mirroring TS. The second
        // index uses the length *after* A was inserted.
        waterPoly.insert(waterPoly.begin() + static_cast<long>(i), waterPtA);
        waterPoly.insert(waterPoly.begin() + static_cast<long>(waterPoly.size() - i), waterPtB);
        shorePoly.insert(shorePoly.begin() + static_cast<long>(i), shorePtA);
        shorePoly.insert(shorePoly.begin() + static_cast<long>(shorePoly.size() - i), shorePtB);
    }

    Vec2 aabbMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2 aabbMax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    for (const auto& p : shorePoly) {
        aabbMin = v2MinElems(aabbMin, p);
        aabbMax = v2MaxElems(aabbMax, p);
    }
    aabb = Collider::createAabb(aabbMin, aabbMax);
}

float River::distanceToShore(const Vec2& pos) const {
    const float t = spline.getClosestTtoPoint(pos);
    const float dist = v2Length(v2Sub(pos, spline.getPos(t)));
    return math::max(waterWidth - dist, 0.0f);
}

float River::getWaterWidth(float t) const {
    const int count = static_cast<int>(spline.points.size());
    int idx = static_cast<int>(std::floor(t * count));
    idx = static_cast<int>(math::clamp(static_cast<float>(idx), 0.0f, static_cast<float>(count - 1)));
    return waterWidths[static_cast<size_t>(idx)];
}

// ---------------------------------------------------------------------------
// terrainGen.ts
// ---------------------------------------------------------------------------

std::vector<Vec2> generateJaggedAabbPoints(const Collider& aabb, int divisionsX, int divisionsY,
                                           float variation, SeededRand& rand) {
    const Vec2 ll(aabb.min.x, aabb.min.y);
    const Vec2 lr(aabb.max.x, aabb.min.y);
    const Vec2 ul(aabb.min.x, aabb.max.y);
    const Vec2 ur(aabb.max.x, aabb.max.y);

    const float distanceX = lr.x - ll.x;
    const float distanceY = ul.y - ll.y;
    const float spanX = distanceX / static_cast<float>(divisionsX + 1);
    const float spanY = distanceY / static_cast<float>(divisionsY + 1);

    std::vector<Vec2> points;
    points.push_back(ll);
    for (int i = 1; i <= divisionsX; ++i) {
        points.push_back(Vec2(ll.x + spanX * i, ll.y + rand(-variation, variation)));
    }
    points.push_back(lr);
    for (int i = 1; i <= divisionsY; ++i) {
        points.push_back(Vec2(lr.x + rand(-variation, variation), lr.y + spanY * i));
    }
    points.push_back(ur);
    for (int i = 1; i <= divisionsX; ++i) {
        points.push_back(Vec2(ur.x - spanX * i, ur.y + rand(-variation, variation)));
    }
    points.push_back(ul);
    for (int i = 1; i <= divisionsY; ++i) {
        points.push_back(Vec2(ul.x + rand(-variation, variation), ul.y - spanY * i));
    }
    return points;
}

std::vector<Vec2> generateJaggedCirclePoints(const Vec2& center, float radius, int divisions,
                                             float variation, SeededRand& rand) {
    std::vector<Vec2> points;
    for (int i = 0; i < divisions; i++) {
        const float angle = (static_cast<float>(i) / static_cast<float>(divisions)) *
                            3.14159265358979f * 2.0f;
        const float r = radius + rand(-variation, variation);
        points.push_back(Vec2(center.x + std::cos(angle) * r, center.y + std::sin(angle) * r));
    }
    return points;
}

TerrainData generateTerrain(float width, float height, float shoreInset, float grassInset,
                            const std::vector<RiverDesc>& riverDescs, uint32_t seed) {
    const float shoreDivisions = 64.0f;
    const float shoreVariation = MapConfig::shoreVariation;
    const float grassVariation = MapConfig::grassVariation;

    SeededRand seededRand(seed);

    const Vec2 ll(shoreInset, shoreInset);
    const Vec2 ur(width - shoreInset, height - shoreInset);
    const Collider aabb = Collider::createAabb(ll, ur);
    std::vector<Vec2> shore = generateJaggedAabbPoints(
        aabb, static_cast<int>(shoreDivisions), static_cast<int>(shoreDivisions), shoreVariation,
        seededRand);

    const Vec2 center(width * 0.5f, height * 0.5f);
    std::vector<Vec2> grass;
    grass.reserve(shore.size());
    for (const auto& pos : shore) {
        const Vec2 toCenter = v2Normalize(v2Sub(center, pos));
        const float variation = seededRand(-grassVariation, grassVariation);
        const float inset = grassInset + variation;
        grass.push_back(v2Add(pos, v2Mul(toCenter, inset)));
    }

    const Collider mapBounds = Collider::createAabb(Vec2(0.0f, 0.0f), Vec2(width, height));

    TerrainData data;
    data.shore = std::move(shore);
    data.grass = std::move(grass);
    for (const auto& desc : riverDescs) {
        River river(desc.points, desc.width, desc.looped, data.rivers, mapBounds);
        data.rivers.push_back(std::move(river));
    }
    return data;
}

} // namespace surv
