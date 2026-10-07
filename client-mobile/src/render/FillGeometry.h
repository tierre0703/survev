#pragma once
// Engine-independent polygon fill tessellation. Horizontal slabs bounded by
// vertices/edge intersections produce convex trapezoids, emitted as triangles.
// Outer contours are unioned; holes are subtracted (including overlapping or
// partially out-of-bounds holes). No poly2tri, winding dependency, or stencil
// depth limit. Shared by the GPU and recording adapters.
#include "../core/Vec2.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace pix {
using FillTriangle = std::array<surv::Vec2, 3>;
class FillGeometry {
public:
    void clear() { rings.clear(); path.clear(); inHole = false; }
    void beginHole() { flushPath(); inHole = true; }
    void endHole() { flushPath(); inHole = false; }
    void moveTo(float x, float y) { flushPath(); path.emplace_back(x, y); }
    void lineTo(float x, float y) { path.emplace_back(x, y); }
    void closePath() { flushPath(); }
    void polygon(const surv::Vec2* points, int count) {
        flushPath();
        if (count >= 3) rings.push_back({std::vector<surv::Vec2>(points, points + count), inHole});
    }
    void rect(float x, float y, float w, float h) {
        const surv::Vec2 points[] = {{x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}};
        polygon(points, 4);
    }
    void circle(float x, float y, float radius) {
        std::array<surv::Vec2, 64> points;
        for (int i = 0; i < 64; ++i) {
            const float angle = i * 6.28318530717959f / 64.0f;
            points[i] = surv::Vec2(x + radius * std::cos(angle), y + radius * std::sin(angle));
        }
        polygon(points.data(), 64);
    }
    std::vector<FillTriangle> triangles() {
        flushPath();
        struct Edge {
            surv::Vec2 a, b;
            size_t ring;
            double x(double y) const { return a.x + (y - a.y) * (b.x - a.x) / (b.y - a.y); }
        };
        std::vector<Edge> edges;
        std::vector<double> ys;
        for (size_t r = 0; r < rings.size(); ++r) {
            const auto& points = rings[r].points;
            for (size_t i = 0; i < points.size(); ++i) {
                auto a = points[i], b = points[(i + 1) % points.size()];
                if (!std::isfinite(a.x) || !std::isfinite(a.y) || !std::isfinite(b.x) || !std::isfinite(b.y)) continue;
                ys.push_back(a.y);
                if (a.y == b.y) continue; // horizontal/duplicate edges do not cross a slab
                if (a.y > b.y) std::swap(a, b);
                edges.push_back({a, b, r});
            }
        }
        // Split wherever edge ordering changes, so overlapping holes/contours
        // remain a true union/difference, not an XOR or doubled alpha fill.
        for (size_t i = 0; i < edges.size(); ++i) {
            for (size_t j = i + 1; j < edges.size(); ++j) {
                const auto& a = edges[i]; const auto& b = edges[j];
                const double lo = std::max(a.a.y, b.a.y), hi = std::min(a.b.y, b.b.y);
                if (hi <= lo) continue;
                const double d0 = a.x(lo) - b.x(lo), d1 = a.x(hi) - b.x(hi);
                if ((d0 < 0 && d1 > 0) || (d0 > 0 && d1 < 0)) ys.push_back(lo + (hi - lo) * d0 / (d0 - d1));
            }
        }
        std::sort(ys.begin(), ys.end());
        ys.erase(std::unique(ys.begin(), ys.end()), ys.end());
        std::vector<FillTriangle> out;
        auto triangle = [&](surv::Vec2 a, surv::Vec2 b, surv::Vec2 c) {
            const double area = (double(b.x) - a.x) * (double(c.y) - a.y) - (double(b.y) - a.y) * (double(c.x) - a.x);
            if (std::fabs(area) > 1e-10) out.push_back({a, b, c});
        };
        for (size_t y = 1; y < ys.size(); ++y) {
            const double lo = ys[y - 1], hi = ys[y], mid = (lo + hi) / 2;
            if (hi - lo <= 1e-10) continue;
            std::vector<const Edge*> crossings;
            for (const auto& edge : edges) if (edge.a.y < mid && edge.b.y > mid) crossings.push_back(&edge);
            std::sort(crossings.begin(), crossings.end(), [&](const Edge* a, const Edge* b) { return a->x(mid) < b->x(mid); });
            std::vector<bool> inside(rings.size(), false);
            int outerCount = 0, holeCount = 0;
            const Edge* left = nullptr;
            for (size_t i = 0; i < crossings.size();) {
                const bool before = outerCount > 0 && holeCount == 0;
                const Edge* edge = crossings[i];
                const double x = edge->x(mid);
                do {
                    const size_t r = crossings[i]->ring;
                    int& count = rings[r].hole ? holeCount : outerCount;
                    count += inside[r] ? -1 : 1;
                    inside[r] = !inside[r];
                    ++i;
                } while (i < crossings.size() && std::fabs(crossings[i]->x(mid) - x) < 1e-9);
                const bool after = outerCount > 0 && holeCount == 0;
                if (!before && after) left = edge;
                if (before && !after && left) {
                    const surv::Vec2 a(static_cast<float>(left->x(lo)), static_cast<float>(lo));
                    const surv::Vec2 b(static_cast<float>(edge->x(lo)), static_cast<float>(lo));
                    const surv::Vec2 c(static_cast<float>(edge->x(hi)), static_cast<float>(hi));
                    const surv::Vec2 d(static_cast<float>(left->x(hi)), static_cast<float>(hi));
                    triangle(a, b, c); triangle(a, c, d);
                    left = nullptr;
                }
            }
        }
        return out;
    }
private:
    struct Ring { std::vector<surv::Vec2> points; bool hole; };
    std::vector<Ring> rings;
    std::vector<surv::Vec2> path;
    bool inHole = false;
    void flushPath() {
        if (path.size() >= 3) rings.push_back({std::move(path), inHole});
        path.clear();
    }
};
} // namespace pix
