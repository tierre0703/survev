#include "Renderer.h"
#include "../game/Map.h"
#include "../game/objects/Structure.h"
#include "../net/Net.h"

#include <cmath>

namespace surv {

static float stepToward(float cur, float target, float rate) {
    const float delta = target - cur;
    const float s = delta * rate;
    return std::fabs(s) < 0.01f ? delta : s;
}

static void traceRect(pix::Graphics* gfx, float x, float y, float w, float h) {
    gfx->moveTo(x, y);
    gfx->lineTo(x, y + h);
    gfx->lineTo(x + w, y + h);
    gfx->lineTo(x + w, y);
    gfx->lineTo(x, y);
    gfx->closePath();
}

Renderer::Renderer(pix::Factory* factory, bool canvasMode)
    : _canvasMode(canvasMode), _factory(factory) {
    for (int i = 0; i < 4; i++) {
        layers[i] = factory->createContainer();
    }
    ground = factory->createGraphics();
    layerMask = factory->createGraphics();
    ground->setAlpha(0.0f);
}

void Renderer::m_free() {
    if (layerMask) {
        layerMask->removeFromParent();
    }
    debugLayerMask = nullptr;
}

void Renderer::addPIXIObj(pix::Node* obj, int layer_, int zOrd, int zIdx_) {
    if (!obj) {
        return;
    }
    int layerIdx = layer_;
    const bool onStairs = (layer_ & 0x2) != 0;
    // Hack to render large/high objects (trees, smokes) on a separate layer
    // that isn't masked off by the bunkers.
    if (onStairs) {
        layerIdx = zOrd >= 100 ? 3 : 2;
    }
    // layers[] has 4 entries; never index out of range on a bad server value.
    if (layerIdx < 0) {
        layerIdx = 0;
    } else if (layerIdx > 3) {
        layerIdx = 3;
    }

    auto it = _objLayer.find(obj);
    const int prevLayer = it == _objLayer.end() ? -1 : it->second;
    if (prevLayer == layerIdx && obj->getSortOrd() == zOrd &&
        (zIdx_ == kNoZIdx || obj->getSortIdx() == zIdx_)) {
        return;
    }

    _objLayer[obj] = layerIdx;
    obj->setSortKey(zOrd, zIdx_ != kNoZIdx ? zIdx_ : zIdx++);
    layers[layerIdx]->addChild(obj);
    _layerDirty[layerIdx] = true;
}

void Renderer::resize(Map& map, Camera& camera) {
    const uint32_t undergroundColor =
        map.mapLoaded ? map.mapDef.colors.underground : 1772803u;

    ground->clear();
    ground->beginFill(undergroundColor);
    ground->drawRect(0.0f, 0.0f, camera.m_screenWidth, camera.m_screenHeight);
    ground->endFill();

    layerMaskDirty = true;
}

void Renderer::redrawLayerMask(Camera& camera, Map& map) {
    auto& structures = map.structurePool.m_getPool();
    if (_canvasMode) {
        layerMask->clear();
        if (layerMaskActive) {
            layerMask->beginFill(0xffffff, 1.0f);
            layerMask->drawRect(0.0f, 0.0f, camera.m_screenWidth, camera.m_screenHeight);
            for (auto* structure : structures) {
                if (!structure->active) {
                    continue;
                }
                for (const auto& m : structure->mask) {
                    const Vec2 halfExtents = v2Mul(v2Sub(m.max, m.min), 0.5f);
                    const Vec2 center = v2Add(m.min, halfExtents);
                    const Vec2 bottomLeft = camera.m_pointToScreen(v2Sub(center, halfExtents));
                    const Vec2 topRight = camera.m_pointToScreen(v2Add(center, halfExtents));
                    layerMask->drawRect(bottomLeft.x, bottomLeft.y, topRight.x - bottomLeft.x,
                                        topRight.y - bottomLeft.y);
                }
            }
            layerMask->endFill();
        }
        return;
    }

    if (layerMaskDirty) {
        layerMaskDirty = false;
        layerMask->clear();
        // axmol DrawNode has no polygon holes, so use the canvas fallback
        // (same as the _canvasMode branch below): the full-map rect plus each
        // structure mask as separate solid rects. Emitting this as one giant
        // polygon (with no-op beginHole/endHole) both loses the holes and made
        // axmol's poly2tri triangulation crash.
        layerMask->beginFill(0xffffff, 1.0f);
        layerMask->drawRect(0.0f, 0.0f, Constants::MaxPosition, Constants::MaxPosition);
        for (auto* structure : structures) {
            if (!structure->active) {
                continue;
            }
            for (const auto& m : structure->mask) {
                const Vec2 halfExtents = v2Mul(v2Sub(m.max, m.min), 0.5f);
                const Vec2 center = v2Add(m.min, halfExtents);
                layerMask->drawRect(center.x - halfExtents.x, center.y - halfExtents.y,
                                    halfExtents.x * 2.0f, halfExtents.y * 2.0f);
            }
        }
        layerMask->endFill();
    }
    const Vec2 p0 = camera.m_pointToScreen(Vec2(0.0f, 0.0f));
    const float s = camera.m_scaleToScreen(1.0f);
    layerMask->setPosition(p0.x, p0.y);
    layerMask->setScale(s, -s);
}

void Renderer::redrawDebugLayerMask(Camera& camera, Map& map) {
    if (!debugLayerMask) {
        // create the debug mask and add it on top of the main layer containers
        if (!_factory) {
            return;
        }
        debugLayerMask = _factory->createGraphics();
        if (pix::Node* parent = layers[3]->getParent()) {
            parent->addChild(debugLayerMask);
        }
    }
    debugLayerMask->clear();
    debugLayerMask->beginFill(0xff00ff, 0.5f);
    for (auto* structure : map.structurePool.m_getPool()) {
        if (!structure->active) {
            continue;
        }
        for (const auto& m : structure->mask) {
            const Vec2 halfWidths = v2Mul(v2Sub(m.max, m.min), 0.5f);
            const Vec2 center = v2Add(m.min, halfWidths);
            traceRect(debugLayerMask, center.x - halfWidths.x, center.y - halfWidths.y,
                      halfWidths.x * 2.0f, halfWidths.y * 2.0f);
        }
    }
    debugLayerMask->endFill();
    const Vec2 p0 = camera.m_pointToScreen(Vec2(0.0f, 0.0f));
    const float s = camera.m_scaleToScreen(1.0f);
    debugLayerMask->setPosition(p0.x, p0.y);
    debugLayerMask->setScale(s, -s);
}

void Renderer::m_update(float dt, Camera& camera, Map& map, bool debugLayerMaskEnabled) {
    const float alphaTarget = layer > 0 ? 1.0f : 0.0f;
    layerAlpha += stepToward(layerAlpha, alphaTarget, dt * 12.0f);
    const float groundTarget = (layer == 1 && underground) ? 1.0f : 0.0f;
    groundAlpha += stepToward(groundAlpha, groundTarget, dt * 12.0f);

    layers[0]->setAlpha(1.0f);
    layers[1]->setAlpha(layerAlpha);
    layers[2]->setAlpha(1.0f);
    layers[3]->setAlpha(1.0f);
    ground->setAlpha(groundAlpha);

    layers[0]->setVisible(groundAlpha < 1.0f);
    layers[1]->setVisible(layerAlpha > 0.0f);
    ground->setVisible(groundAlpha > 0.0f);

    redrawLayerMask(camera, map);

    if (debugLayerMaskEnabled) {
        redrawDebugLayerMask(camera, map);
    }
    if (debugLayerMask) {
        debugLayerMask->setVisible(debugLayerMaskEnabled);
    }

    const bool maskActive = layer == 0;
    if (maskActive && !layerMaskActive) {
        layers[2]->setMask(layerMask);
        layerMaskActive = true;
    } else if (!maskActive && layerMaskActive) {
        layers[2]->clearMask();
        layerMaskActive = false;
    }

    for (int i = 0; i < 4; i++) {
        if (_layerDirty[i]) {
            layers[i]->sortChildren();
            _layerDirty[i] = false;
        }
    }
}

} // namespace surv
