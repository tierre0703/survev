#include "Map.h"
#include "GameWorld.h"

#include "../core/GameConfig.h"
#include "../core/MathUtil.h"
#include "../core/Rand.h"
#include "../render/Camera.h"

#include <cmath>

namespace surv {

static void tracePath(pix::Graphics* g, const std::vector<Vec2>& path) {
    if (path.empty()) {
        return;
    }
    g->moveTo(path[0].x, path[0].y);
    for (size_t i = 1; i < path.size(); ++i) {
        g->lineTo(path[i].x, path[i].y);
    }
    g->closePath();
}

static void drawLine(pix::Graphics* g, const Vec2& a, const Vec2& b) {
    g->moveTo(a.x, a.y);
    g->lineTo(b.x, b.y);
}

static void traceGroundPatch(pix::Graphics* g, const GroundPatch& patch, uint32_t seed) {
    const float offset = math::max(patch.offsetDist, 0.001f);
    const float roughness = patch.roughness;
    SeededRand seededRand(seed);

    if (patch.bound.type == Collider::Circle) {
        const int divisions = static_cast<int>(
            std::round((2.0f * 3.14159265358979f * patch.bound.rad * roughness) / offset));
        tracePath(g, generateJaggedCirclePoints(patch.bound.pos, patch.bound.rad, divisions, offset,
                                                seededRand));
    } else {
        const float w = patch.bound.max.x - patch.bound.min.x;
        const float h = patch.bound.max.y - patch.bound.min.y;
        const int divisionsX = static_cast<int>(std::round((w * roughness) / offset));
        const int divisionsY = static_cast<int>(std::round((h * roughness) / offset));
        tracePath(g, generateJaggedAabbPoints(patch.bound, divisionsX, divisionsY, offset,
                                              seededRand));
    }
}

Map::Map(pix::Factory* factory, bool canvasMode) : groundGfx(factory->createGraphics()), _canvasMode(canvasMode) {}

void Map::loadMap(const MapMsg& msg, Camera& camera) {
    mapName = msg.mapName;
    const DefProvider* provider = getDefProvider();
    const MapRenderDef* renderDef = provider ? provider->mapRender(mapName) : nullptr;
    mapDef = renderDef ? *renderDef : MapRenderDef{};
    factionMode = mapDef.factionMode;
    potatoMode = mapDef.potatoMode;
    perkMode = mapDef.perkMode;
    turkeyMode = mapDef.turkeyMode;

    seed = msg.seed;
    width = msg.width;
    height = msg.height;

    std::vector<RiverDesc> riverDescs;
    riverDescs.reserve(msg.rivers.size());
    for (const auto& r : msg.rivers) {
        RiverDesc desc;
        desc.width = static_cast<float>(r.width);
        desc.looped = r.looped;
        desc.points = r.points;
        riverDescs.push_back(std::move(desc));
    }
    terrain = generateTerrain(width, height, msg.shoreInset, msg.grassInset, riverDescs, seed);

    places = msg.places;
    objects = msg.objects;
    groundPatches = msg.groundPatches;
    mapLoaded = true;

    groundGfx->clear();
    renderTerrain(groundGfx, 2.0f / camera.m_ppu, _canvasMode, false);
}

void Map::renderTerrain(pix::Graphics* g, float gridThickness, bool canvasMode, bool mapRender) {
    const float w = width;
    const float h = height;
    const Vec2 ll(0.0f, 0.0f);
    const Vec2 lr(w, 0.0f);
    const Vec2 ul(0.0f, h);
    const Vec2 ur(w, h);
    const BiomeColors& colors = mapDef.colors;

    // Background surround.
    g->beginFill(colors.background);
    g->drawRect(-120.0f, -120.0f, w + 240.0f, 120.0f);
    g->drawRect(-120.0f, h, w + 240.0f, 120.0f);
    g->drawRect(-120.0f, -120.0f, 120.0f, h + 240.0f);
    g->drawRect(w, -120.0f, 120.0f, h + 240.0f);
    g->endFill();

    // The web client draws the beach, then the full-map water with a hole for
    // the island (PIXI holes). axmol DrawNode has no polygon holes, so we use
    // the canvas fallback: water first, then beach/grass on top.
    g->beginFill(colors.water);
    g->drawRect(0.0f, 0.0f, w, h);
    g->endFill();

    g->beginFill(colors.beach);
    tracePath(g, terrain.shore);
    g->endFill();

    g->beginFill(colors.grass);
    tracePath(g, terrain.grass);
    g->endFill();

    // Order 0 ground patches.
    for (const auto& patch : groundPatches) {
        if (patch.order == 0 && (!mapRender || patch.useAsMapShape)) {
            g->beginFill(patch.color);
            traceGroundPatch(g, patch, seed);
            g->endFill();
        }
    }

    // River shore.
    for (const auto& river : terrain.rivers) {
        g->beginFill(river.looped ? colors.lakeRiverbank : colors.riverbank);
        tracePath(g, river.shorePoly);
    }
    g->endFill();

    // River water.
    for (const auto& river : terrain.rivers) {
        g->beginFill(river.looped ? colors.lakeWater : colors.water);
        tracePath(g, river.waterPoly);
    }
    g->endFill();

    // Grid.
    g->lineStyle(gridThickness, 0, 0.15f);
    for (float x = 0.0f; x <= w; x += MapConfig::gridSize) {
        drawLine(g, Vec2(x, 0.0f), Vec2(x, h));
    }
    for (float y = 0.0f; y <= h; y += MapConfig::gridSize) {
        drawLine(g, Vec2(0.0f, y), Vec2(w, y));
    }
    g->lineStyle(gridThickness, 0, 0.0f);

    // Order 1 ground patches.
    for (const auto& patch : groundPatches) {
        if (patch.order == 1 && (!mapRender || patch.useAsMapShape)) {
            g->beginFill(patch.color);
            traceGroundPatch(g, patch, seed);
            g->endFill();
        }
    }
}

void Map::m_render(const Camera& camera) {
    const Vec2 p0 = camera.m_pointToScreen(Vec2(0.0f, 0.0f));
    const Vec2 p1 = camera.m_pointToScreen(Vec2(1.0f, 1.0f));
    const Vec2 s = v2Sub(p1, p0);
    groundGfx->setPosition(p0.x, p0.y);
    groundGfx->setScale(s.x, s.y);
}

void Map::update(float dt, GameWorld& ctx) {
    for (auto* obstacle : obstaclePool.m_getPool()) {
        if (obstacle->active) {
            obstacle->update(dt, ctx);
        }
    }
    for (auto* building : buildingPool.m_getPool()) {
        if (building->active) {
            building->update(dt, ctx);
        }
    }
    for (auto* structure : structurePool.m_getPool()) {
        if (structure->active) {
            // Structures only do debug rendering in the web client.
        }
    }
}

Building* Map::getBuildingById(uint16_t id) {
    for (auto* building : buildingPool.m_getPool()) {
        if (building->active && building->__id == id) {
            return building;
        }
    }
    return nullptr;
}

bool Map::insideStructureStairs(const Collider& c) const {
    for (auto* structure : structurePool.m_getPool()) {
        if (structure->active && structure->insideStairs(c)) {
            return true;
        }
    }
    return false;
}

bool Map::insideStructureMask(const Collider& c) const {
    for (auto* structure : structurePool.m_getPool()) {
        if (structure->active && structure->insideMask(c)) {
            return true;
        }
    }
    return false;
}

bool Map::insideBuildingCeiling(const Collider& c, bool checkVisible) const {
    for (auto* building : buildingPool.m_getPool()) {
        if (building->active && (!checkVisible || (building->ceilingVisionTicker > 0.0f &&
                                                   !building->ceilingDead)) &&
            building->isInsideCeiling(c)) {
            return true;
        }
    }
    return false;
}

} // namespace surv
