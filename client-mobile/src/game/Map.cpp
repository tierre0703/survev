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

#include <algorithm>
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

Map::Map(pix::Factory* factory, bool canvasMode)
    : groundGfx(factory->createGraphics()), _canvasMode(canvasMode) {}

void Map::loadMap(const MapMsg& msg, Camera& camera, float gridThickness) {
    (void)camera;
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
    // map.ts drops the previous match's objects on a (re)load. GameWorld reuses
    // a single Map across matches, so the obstacle/building pools must be reset
    // here or the new match would render/query the old map's survivors. The
    // ObjectCreator's id->object map is reset by GameWorld::loadMap (map.ts
    // `clear()` also resets the creator, but the creator is owned there).
    resetObjectPools();
    mapLoaded = true;

    // map.ts draws the whole terrain into the one ground Graphics; the camera
    // transform lives on the world container it is parented under (GameWorld).
    groundGfx->clear();
    renderTerrain(groundGfx, gridThickness, _canvasMode, false);
}

void Map::resetObjectPools() {
    // map.ts `clear()` walks the pools with m_free and then empties the
    // per-match suppression lists. The pools keep their allocations (Pool<T>
    // has no shrink path) but every survivor is deactivated so a pooled reload
    // cannot hand out a stale object.
    // Pool::m_free decrements the active count, so only live objects may be
    // freed; the pools keep their (now inactive) allocations for reuse.
    if (obstaclePool.activeCount() > 0) {
        for (auto* obstacle : obstaclePool.m_getPool()) {
            if (obstacle->active) {
                obstaclePool.m_free(obstacle);
            }
        }
    }
    if (buildingPool.activeCount() > 0) {
        for (auto* building : buildingPool.m_getPool()) {
            if (building->active) {
                buildingPool.m_free(building);
            }
        }
    }
    if (structurePool.activeCount() > 0) {
        for (auto* structure : structurePool.m_getPool()) {
            if (structure->active) {
                structurePool.m_free(structure);
            }
        }
    }
    deadObstacleIds.clear();
    deadCeilingIds.clear();
    solvedPuzzleIds.clear();
    lootDropSfxIds.clear();
    // The biome camera emitter outlives the map def; GameWorld::loadMap stops
    // it after this returns, so drop the pointer here to avoid a dangling read.
    cameraEmitter = nullptr;
}

void Map::renderTerrain(pix::Graphics* g, float gridThickness, bool canvasMode, bool mapRender) {
    const float w = width;
    const float h = height;
    const Vec2 ll(0.0f, 0.0f);
    const Vec2 lr(w, 0.0f);
    const Vec2 ul(0.0f, h);
    const Vec2 ur(w, h);
    const BiomeColors& colors = mapDef.colors;

    // Background surround. map.ts draws four separate strips; a single fill
    // with four rects makes FillGeometry's slab scanline split the full
    // 240px-thick ring, so it emits ~2 slivers per pixel row and the GPU
    // quad-budget drops/overdraws them (slab-coloured screen).
    const float surround[][4] = {
        {-120.0f, -120.0f, w + 240.0f, 120.0f}, // top
        {-120.0f, h, w + 240.0f, 120.0f},       // bottom
        {-120.0f, -120.0f, 120.0f, h + 240.0f}, // left
        {w, -120.0f, 120.0f, h + 240.0f},       // right
    };
    for (const auto& r : surround) {
        g->beginFill(colors.background);
        g->drawRect(r[0], r[1], r[2], r[3]);
        g->endFill();
    }

    // Water -> shore ring -> grass, each layer overpainting the previous one,
    // like map.ts' canvas branch. The other branch punched the shore/grass
    // contours out as holes, but FillGeometry tessellates every contour with a
    // horizontal-slab scanline, and a 260-point island contour produced ~18k
    // overlapping slabs (~35k triangles) whose overdraw read as a flat slab of
    // colour on the GPU instead of the island. Overpaint needs 4 fills.
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
        g->endFill();
    }

    // River water.
    for (const auto& river : terrain.rivers) {
        g->beginFill(river.looped ? colors.lakeWater : colors.water);
        tracePath(g, river.waterPoly);
        g->endFill();
    }

    // Grid (renderer.ts drawGrid). map.ts skips it at zero thickness. Every
    // line is its own DrawNode quad, i.e. the largest single batch of the
    // ground node; `gridThickness <= 0` also lets the app drop the grid.
    if (gridThickness > 0.0f) {
        g->lineStyle(gridThickness, 0, 0.15f);
        for (float x = 0.0f; x <= w; x += MapConfig::gridSize) {
            drawLine(g, Vec2(x, 0.0f), Vec2(x, h));
        }
        for (float y = 0.0f; y <= h; y += MapConfig::gridSize) {
            drawLine(g, Vec2(0.0f, y), Vec2(w, y));
        }
        g->lineStyle(gridThickness, 0, 0.0f);
    }

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
    (void)camera;
    // map.ts draws the terrain at world scale (the camera transform lives on
    // the world container), so the ground node keeps the identity transform.
    groundGfx->setPosition(0.0f, 0.0f);
    groundGfx->setScale(1.0f, 1.0f);
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

void Map::onFactoryReset() {
    if (_mapTexture) {
        _mapTexture = nullptr;  // adapter-owned, released by factory
    }
}

void Map::generateMinimapTexture(pix::Factory* factory, const Camera& camera) {
    (void)camera;
    if (!factory || _mapTexture) {
        return;
    }

    // map.ts renderMap(): bake the terrain at world scale, flip it vertically
    // around map `height`, draw the map-object shapes at (x, height - y), then
    // scale the whole graph down to a square texture.
    const float texSize = kMinimapTextureSize;
    _mapTexture = factory->createRenderTexture();
    _mapTexture->resize(texSize, texSize);

    const float mapScale = texSize / std::max(width, height);
    if (mapScale <= 0.0f) {
        return;
    }

    pix::Container* scratch = factory->createContainer();
    scratch->setScale(mapScale, mapScale);

    // --- Terrain: world-space drawing flipped around y = height ---
    pix::Graphics* terrainGfx = factory->createGraphics();
    terrainGfx->setPosition(0.0f, height);
    terrainGfx->setScale(1.0f, -1.0f);
    // The web passes height / screenScale as the grid thickness (world units)
    // so the grid lands on ~1 texture pixel; mapRender keeps only map shapes.
    renderTerrain(terrainGfx, 1.0f / mapScale, false, true);

    // map.ts border for extra spiffiness (2px at texture scale).
    const float lineW = 2.0f / mapScale;
    terrainGfx->lineStyle(lineW, 0, 1.0f);
    drawLine(terrainGfx, Vec2(0.0f, 0.0f), Vec2(0.0f, height));
    drawLine(terrainGfx, Vec2(0.0f, height), Vec2(width, height));
    drawLine(terrainGfx, Vec2(width, height), Vec2(width, 0.0f));
    drawLine(terrainGfx, Vec2(width, 0.0f), Vec2(0.0f, 0.0f));
    terrainGfx->lineStyle(0.0f, 0, 0.0f);
    scratch->addChild(terrainGfx);

    // --- Map objects (map.ts getMinimapRender + renderMap) ---
    struct MinimapObj {
        float zIdx = 0.0f;
        Collider col;
        float shapeScale = 1.0f;
        uint32_t color = 0x808080;
    };
    std::vector<MinimapObj> renderList;

    const DefProvider* provider = getDefProvider();

    const auto addShapes = [&](const MapObjectDef* def, const Vec2& objPos, int ori, float objScale,
                               float zIdx, bool building) {
        if (!def) {
            return;
        }
        if (!def->mapShapes.empty()) {
            for (const auto& shape : def->mapShapes) {
                MinimapObj mo;
                mo.zIdx = zIdx;
                mo.col = colliderTransform(shape.collider, objPos, math::oriToRad(ori), objScale);
                mo.shapeScale = shape.scale > 0.0f ? shape.scale : 1.0f;
                mo.color = shape.color;
                renderList.push_back(mo);
            }
            return;
        }
        if (building) {
            if (!def->hasBounding) {
                return;
            }
            MinimapObj mo;
            mo.zIdx = zIdx;
            mo.col = colliderTransform(def->boundingCollider, objPos, math::oriToRad(ori), objScale);
            mo.shapeScale = def->map.scale > 0.0f ? def->map.scale : 1.0f;
            mo.color = def->map.hasColor ? def->map.color : 0x808080;
            renderList.push_back(mo);
        } else if (def->hasCollision) {
            MinimapObj mo;
            mo.zIdx = zIdx;
            mo.col = colliderTransform(def->collision, objPos, math::oriToRad(ori), objScale);
            mo.shapeScale = def->map.scale > 0.0f ? def->map.scale : 1.0f;
            mo.color = def->map.hasColor ? def->map.color : 0x808080;
            renderList.push_back(mo);
        }
    };

    for (auto* obstacle : obstaclePool.m_getPool()) {
        if (!obstacle->active || obstacle->dead || !obstacle->collidable) {
            continue;
        }
        const MapObjectDef* def = provider ? provider->mapObject(obstacle->type) : nullptr;
        if (!def) {
            continue;
        }
        addShapes(def, obstacle->pos, obstacle->ori, obstacle->scale,
                  static_cast<float>(def->img.zIdx), false);
    }
    for (auto* building : buildingPool.m_getPool()) {
        if (!building->active) {
            continue;
        }
        const MapObjectDef* def = provider ? provider->mapObject(building->type) : nullptr;
        if (!def) {
            continue;
        }
        addShapes(def, building->pos, building->ori, building->scale,
                  750.0f + static_cast<float>(def->zIdx), true);
    }

    std::sort(renderList.begin(), renderList.end(),
              [](const MinimapObj& a, const MinimapObj& b) { return a.zIdx < b.zIdx; });

    pix::Graphics* objGfx = factory->createGraphics();
    for (const auto& mo : renderList) {
        objGfx->beginFill(mo.color, 1.0f);
        if (mo.col.type == Collider::Circle) {
            // World -> texture space: x stays, y flips around map height.
            objGfx->drawCircle(mo.col.pos.x, height - mo.col.pos.y, mo.col.rad * mo.shapeScale);
        } else if (mo.col.type == Collider::Aabb && mo.col.max.x > mo.col.min.x) {
            const Vec2 center = v2Mul(v2Add(mo.col.min, mo.col.max), 0.5f);
            const Vec2 half = v2Mul(v2Sub(mo.col.max, mo.col.min), 0.5f * mo.shapeScale);
            objGfx->drawRect(center.x - half.x, (height - center.y) - half.y, half.x * 2.0f, half.y * 2.0f);
        }
        objGfx->endFill();
    }
    scratch->addChild(objGfx);

    if (pix::Renderer* renderer = factory->createRenderer()) {
        renderer->render(scratch, _mapTexture, true);
    }
    scratch->removeAllChildren();
}

float Map::getMinimapScale() const {
    return _mapTexture ? kMinimapTextureSize / std::max(width, height) : 0.0f;
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
