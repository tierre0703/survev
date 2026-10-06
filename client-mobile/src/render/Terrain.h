#pragma once
// Port of shared/utils/spline.ts, river.ts and terrainGen.ts. These are pure
// geometry algorithms (no engine dependency) and are exercised by the host
// tests against the TypeScript implementation.
#include "../core/Collider.h"
#include "../core/Rand.h"
#include "../core/Vec2.h"
#include <vector>

namespace surv {

// --- spline.ts -------------------------------------------------------------

struct SplineControlPoints {
    float pt = 0.0f;
    Vec2 p0, p1, p2, p3;
};

SplineControlPoints getControlPoints(float t, const std::vector<Vec2>& points, bool looped);
float catmullRom(float t, float p0, float p1, float p2, float p3);

class Spline {
public:
    Spline() = default;
    Spline(const std::vector<Vec2>& points, bool looped);

    std::vector<Vec2> points;
    std::vector<float> arcLens;
    float totalArcLen = 0.0f;
    bool looped = false;

    Vec2 getPos(float t) const;
    Vec2 getTangent(float t) const;
    Vec2 getNormal(float t) const;
    float getClosestTtoPoint(const Vec2& pos) const;
    float getTfromArcLen(float arcLen) const;
    float getArcLen(float t) const;
};

// --- river.ts --------------------------------------------------------------

class River {
public:
    River() = default;
    River(const std::vector<Vec2>& splinePts, float riverWidth, bool looped,
          const std::vector<River>& otherRivers, const Collider& mapBounds);

    Spline spline;
    float waterWidth = 0.0f;
    float shoreWidth = 0.0f;
    bool looped = false;
    Vec2 center;
    std::vector<Vec2> waterPoly;
    std::vector<Vec2> shorePoly;
    std::vector<float> waterWidths;
    std::vector<float> shoreWidths;
    Collider aabb; // always Aabb

    float distanceToShore(const Vec2& pos) const;
    float getWaterWidth(float t) const;
};

// --- terrainGen.ts ---------------------------------------------------------

std::vector<Vec2> generateJaggedAabbPoints(const Collider& aabb, int divisionsX,
                                           int divisionsY, float variation, SeededRand& rand);
std::vector<Vec2> generateJaggedCirclePoints(const Vec2& center, float radius,
                                             int divisions, float variation, SeededRand& rand);

struct RiverDesc {
    float width = 0.0f;
    bool looped = false;
    std::vector<Vec2> points;
};

struct TerrainData {
    std::vector<Vec2> shore;
    std::vector<Vec2> grass;
    std::vector<River> rivers;
};

TerrainData generateTerrain(float width, float height, float shoreInset, float grassInset,
                            const std::vector<RiverDesc>& riverDescs, uint32_t seed);

} // namespace surv
