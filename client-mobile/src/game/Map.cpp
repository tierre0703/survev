#include "Map.h"
#include "GameWorld.h"
#include "Gas.h"
#include "objects/Barns.h"
#include "objects/Structure.h"

#include "../core/GameConfig.h"
#include "../core/MathUtil.h"
#include "../core/Rand.h"
#include "../core/Collider.h"
#include "../render/Camera.h"

#include <cmath>

namespace surv {

// util.sameLayer (shared/utils/util.ts): objects on the same layer interact.
static bool sameLayerMask(int a, int b) {
    return ((a & 0x1) == (b & 0x1)) || (((a & 0x2) != 0) && ((b & 0x2) != 0));
}

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

    // The island hole is real on native: the water is a full-map rect with the
    // shore contour punched out (FillGeometry tessellates in both adapters), so
    // beach/grass never need an overpaint fallback. The canvas keeps its
    // overpaint ordering because canvas Graphics has no hole support.
    if (canvasMode) {
        g->beginFill(colors.water);
        g->drawRect(0.0f, 0.0f, w, h);
        g->endFill();
        g->beginFill(colors.beach);
        tracePath(g, terrain.shore);
        g->endFill();
    } else {
        g->beginFill(colors.water);
        g->drawRect(0.0f, 0.0f, w, h);
        g->beginHole();
        tracePath(g, terrain.shore);
        g->endHole();
        g->endFill();
        // Beach ring: the beach contour minus the (smaller) grass contour, so
        // the grass drawn next is not covered by beach fill.
        g->beginFill(colors.beach);
        tracePath(g, terrain.shore);
        g->beginHole();
        tracePath(g, terrain.grass);
        g->endHole();
        g->endFill();
    }

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
        g->endFill();
    }

    // River water.
    for (const auto& river : terrain.rivers) {
        g->beginFill(river.looped ? colors.lakeWater : colors.water);
        tracePath(g, river.waterPoly);
        g->endFill();
    }

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
    const int activeLayer = ctx.activePlayer() ? ctx.activePlayer()->layer : 0;
    for (auto* obstacle : obstaclePool.m_getPool()) {
        if (obstacle->active) {
            obstacle->update(dt, ctx);
            obstacle->render(ctx, activeLayer);
        }
    }
    for (auto* structure : structurePool.m_getPool()) {
        if (structure->active) {
            // Structures only do debug rendering in the web client.
        }
    }

    // map.ts computes this once per frame; obstacle.ts caches it.
    const bool valueAdjustChanged = mapDef.valueAdjust != _lastValueAdjust;
    if (valueAdjustChanged) {
        _lastValueAdjust = mapDef.valueAdjust;
    }
    for (auto* building : buildingPool.m_getPool()) {
        if (building->active) {
            building->update(dt, ctx);
        }
    }
    if (valueAdjustChanged) {
        for (auto* obstacle : obstaclePool.m_getPool()) {
            if (obstacle->active) {
                obstacle->applyColor();
            }
        }
    }

    // map.ts cameraEmitter: follow the camera, scale radius/rate by zoom.
    if (cameraEmitter && ctx.activePlayer()) {
        cameraEmitter->pos = ctx.camera().m_pos;
        cameraEmitter->enabled = true;
        const float maxRadius = 120.0f;
        const float camRadius = ctx.camera().m_zoom * 2.5f;
        cameraEmitter->radius = math::min(camRadius, maxRadius);
        const float ratio = (cameraEmitter->radius * cameraEmitter->radius) / (maxRadius * maxRadius);
        cameraEmitter->rateMult = ratio > 0.0f ? 1.0f / ratio : 1.0f;
        const float alphaTarget = ctx.activePlayer()->layer == 0 ? 1.0f : 0.0f;
        cameraEmitter->alpha =
            math::lerp(dt * 6.0f, cameraEmitter->alpha, alphaTarget);
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

Map::GroundSurface Map::getGroundSurface(const Vec2& pos, int layer) const {
    // Decals are checked first (map.ts): a decal painted over water (e.g. the
    // bathhouse pool) overrides the ground surface, including its gore fade.
    if (decalBarn) {
        for (auto* decal : decalBarn->decalPool.m_getPool()) {
            if (decal->active && decal->hasSurface && sameLayerMask(decal->layer, layer) &&
                colliderIntersectCircle(decal->collider, pos, 0.0001f)) {
                GroundSurface out;
                out.type = decal->surfaceType == SurfaceTypeDef::Grass
                               ? SurfaceType::Grass
                               : (decal->surfaceType == SurfaceTypeDef::Sand
                                      ? SurfaceType::Sand
                                      : SurfaceType::Water);
                out.waterColor = decal->surfaceWaterColor;
                out.rippleColor = decal->surfaceRippleColor;
                if (out.type == SurfaceType::Water) {
                    // River/decalless defaults for a decal surface are authored
                    // in the def; fall back to the map colors when unset.
                    if (out.waterColor == 0) out.waterColor = mapDef.colors.water;
                    if (out.rippleColor == 0) out.rippleColor = mapDef.colors.waterRipple;
                }
                return out;
            }
        }
    }

    // Buildings can override the ground (layer 2 surfaces), matching map.ts.
    int zIdx = 0;
    const Building::Surface* surface = nullptr;
    const bool onStairs = (layer & 2) != 0;
    for (auto* building : buildingPool.m_getPool()) {
        if (!building->active || building->zIdx < zIdx) {
            continue;
        }
        if (!(building->layer == layer || onStairs)) {
            continue;
        }
        if (building->layer == 1 && onStairs) {
            continue;
        }
        for (const auto& s : building->surfaces) {
            bool hit = false;
            for (const auto& c : s.colliders) {
                if (colliderIntersectCircle(c, pos, 0.0001f)) {
                    hit = true;
                    break;
                }
            }
            if (hit) {
                zIdx = building->zIdx;
                surface = &s;
                break;
            }
        }
    }
    if (surface) {
        GroundSurface out;
        if (surface->type == "water") out.type = SurfaceType::Water;
        else if (surface->type == "sand") out.type = SurfaceType::Sand;
        else if (surface->type == "grass") out.type = SurfaceType::Grass;
        else out.type = SurfaceType::Water;
        out.waterColor = mapDef.colors.water;
        out.rippleColor = mapDef.colors.waterRipple;
        return out;
    }

    // Rivers.
    bool onRiverShore = false;
    if (layer != 1) {
        for (const auto& river : terrain.rivers) {
            if (testPointAabb(pos, river.aabb.min, river.aabb.max) &&
                math::pointInsidePolygon(pos, river.shorePoly.data(),
                                         static_cast<int>(river.shorePoly.size()))) {
                onRiverShore = true;
                if (math::pointInsidePolygon(pos, river.waterPoly.data(),
                                             static_cast<int>(river.waterPoly.size()))) {
                    GroundSurface out;
                    out.type = SurfaceType::Water;
                    out.waterColor = river.looped ? mapDef.colors.lakeWater : mapDef.colors.water;
                    out.rippleColor =
                        river.looped ? mapDef.colors.lakeWaterRipple : mapDef.colors.waterRipple;
                    return out;
                }
            }
        }
    }

    // Terrain.
    GroundSurface out;
    if (math::pointInsidePolygon(pos, terrain.grass.data(),
                                 static_cast<int>(terrain.grass.size()))) {
        out.type = onRiverShore ? SurfaceType::Sand : SurfaceType::Grass;
    } else if (math::pointInsidePolygon(pos, terrain.shore.data(),
                                        static_cast<int>(terrain.shore.size()))) {
        out.type = SurfaceType::Sand;
    } else {
        out.type = SurfaceType::Water;
        out.waterColor = mapDef.colors.water;
        out.rippleColor = mapDef.colors.waterRipple;
    }
    return out;
}

bool Map::isInOcean(const Vec2& pos) const {
    return !math::pointInsidePolygon(pos, terrain.shore.data(),
                                     static_cast<int>(terrain.shore.size()));
}

float Map::distanceToShore(const Vec2& pos) const {
    return math::distToPolygon(pos, terrain.shore.data(),
                               static_cast<int>(terrain.shore.size()));
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

bool Map::isUnderground(const Vec2& pos, int layer) const {
    if (layer != 1) {
        return false;
    }
    const Collider c = Collider::createCircle(pos, 1.0f);
    for (auto* structure : structurePool.m_getPool()) {
        if (!structure->active) {
            continue;
        }
        for (const auto& l : structure->layers) {
            if (l.underground && colliderIntersect(l.collision, c)) {
                return true;
            }
        }
    }
    return false;
}

} // namespace surv
