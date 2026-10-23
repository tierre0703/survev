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
    // Alpha is in the rebuilt vertices. Keep node opacity at one so axmol
    // versions with a DrawNode u_alpha uniform do not multiply it by zero.
    ground->setAlpha(1.0f);
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

// New-match reset (GameWorld::loadMap). The assembly lists only ever hold live
// objects, so clearing them here is what stops the next frame from masking or
// stepping a pooled object that belongs to the previous match.
void Renderer::reset() {
    zIdx = 0;
    layer = 0;
    layerAlpha = 0.0f;
    groundAlpha = 0.0f;
    underground = false;
    _objLayer.clear();
    for (int i = 0; i < 4; i++) {
        _layerDirty[i] = true;
        if (layers[i]) {
            layers[i]->removeAllChildren();
        }
    }
    ground->clear();
    ground->setAlpha(1.0f);
    _groundAlphaDrawn = -1.0f;
    layerMaskDirty = true;
    layerMaskActive = false;
    if (layerMask) {
        layerMask->clear();
    }
}

void Renderer::resize(Map& map, Camera& camera) {
    _groundColor = map.mapLoaded ? map.mapDef.colors.underground : 1772803u;
    _groundAlphaDrawn = -1.0f;
    redrawGround(camera);
    layerMaskDirty = true;
}

// renderer.ts container transform. The old port projected every object by hand
// into screen space (`camera.m_pointToScreen`) while the containers stayed
// unscaled; combined with the sprites' point-space sizes the framing was off
// whenever m_z() != 1 (zoom in/out, world aspect). Objects now stay in world
// units (the web client's convention) and the root carries the camera.
void Renderer::applyCamera(pix::Node* root, const Camera& camera) {
    // Never accept a null target and keep the transform unconditional: the gas
    // overlay is a sibling of the world content container and only inherits a
    // transform from this call.
    if (!root) {
        return;
    }
    // A masked layer's handle is the wrapper node (see AxNodeImpl::setMask), so
    // the transform lands on the real container even while the stencil is live.
    // (Camera::m_pointToScreen already handles the pre-setSize() case, where
    // m_pos is the screen point under the viewport centre, so no special case
    // is needed here.)
    // The container's local origin is world (0, 0), so it goes to the screen
    // position of the world origin -- NOT of the camera itself (which is always
    // the viewport centre). Using m_pointToScreen(m_pos) pinned world (0,0) to
    // the centre and the camera never scrolled. map.ts does the same:
    // `p0 = camera.m_pointToScreen(v2.create(0, 0))`.
    //
    // m_pointToScreen returns web/PIXI screen coordinates (y down); axmol's
    // scene graph is y up, so the origin is mirrored vertically and the world
    // keeps a POSITIVE y scale. (The old `scale(s, -s)` made north point down,
    // which was visible once the minimap showed the real map orientation and
    // inverted the joystick's up/down.)
    const float s = camera.m_z();
    const Vec2 p = camera.m_pointToScreen(Vec2(0.0f, 0.0f));
    root->setScale(s, s);
    root->setPosition(p.x, camera.m_screenHeight - p.y);
}

// drawGround draws the snap-to-underground-colour rect that fills the floor of a
// building (renderer.ts `drawGround`). The geometry is a screen-space rectangle
// (0,0)-(w,h); the parent is the camera-transformed root, so the rect is
// converted back into world units here to stay put on screen.
// T6: DrawNode bakes vertex alpha into the geometry, so setAlpha() on the node
// does not fade the drawn rect. Rebuild the ground rect with the current alpha.
void Renderer::redrawGround(Camera& camera) {
    ground->clear();
    if (camera.m_z() <= 0.0f) {
        return;
    }
    const Vec2 min = camera.m_screenToPoint(Vec2(0.0f, camera.m_screenHeight));
    const Vec2 max = camera.m_screenToPoint(Vec2(camera.m_screenWidth, 0.0f));
    ground->beginFill(_groundColor, groundAlpha);
    ground->drawRect(min.x, min.y, max.x - min.x, max.y - min.y);
    ground->endFill();
}

void Renderer::redrawLayerMask(Camera& camera, Map& map) {
    (void)camera;
    auto& structures = map.structurePool.m_getPool();
    if (_canvasMode) {
        // Canvas mode masks with a screen rect; same world-space conversion as
        // redrawGround() (the parent already carries the camera transform).
        layerMask->clear();
        if (layerMaskActive) {
            const Vec2 min = camera.m_screenToPoint(Vec2(0.0f, camera.m_screenHeight));
            const Vec2 max = camera.m_screenToPoint(Vec2(camera.m_screenWidth, 0.0f));
            layerMask->beginFill(0xffffff, 1.0f);
            layerMask->drawRect(min.x, min.y, max.x - min.x, max.y - min.y);
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
        return;
    }

    if (layerMaskDirty) {
        layerMaskDirty = false;
        layerMask->clear();
        // T5: the layer mask is the full map with the structure masks punched
        // out as holes. AxGraphics tessellates the fill directly (no
        // poly2tri), so this matches the web
        // client's non-canvas branch.
        layerMask->beginFill(0xffffff, 1.0f);
        layerMask->drawRect(0.0f, 0.0f, Constants::MaxPosition, Constants::MaxPosition);
        for (auto* structure : structures) {
            if (!structure->active) {
                continue;
            }
            for (const auto& m : structure->mask) {
                const Vec2 halfExtents = v2Mul(v2Sub(m.max, m.min), 0.5f);
                const Vec2 center = v2Add(m.min, halfExtents);
                layerMask->beginHole();
                layerMask->drawRect(center.x - halfExtents.x, center.y - halfExtents.y,
                                    halfExtents.x * 2.0f, halfExtents.y * 2.0f);
                layerMask->endHole();
            }
        }
        layerMask->endFill();
    }
    // World space, like the layers it masks: the parent (and the stencil branch's
    // clipper) already carries the camera transform, so the mask needs no
    // transform of its own.
    layerMask->setPosition(0.0f, 0.0f);
    layerMask->setScale(1.0f, 1.0f);
}

void Renderer::redrawDebugLayerMask(Camera& camera, Map& map) {
    (void)camera;
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
    // World space (see redrawLayerMask).
    debugLayerMask->setPosition(0.0f, 0.0f);
    debugLayerMask->setScale(1.0f, 1.0f);
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

    // T6: rebuild the ground geometry when the fade changes (DrawNode bakes
    // vertex alpha, so node opacity alone does not fade it). The rect is sized
    // to the viewport through the camera, so any zoom change rebuilds it too
    // (GameWorld re-runs resize() after applyCamera()).
    if (std::fabs(groundAlpha - _groundAlphaDrawn) > 0.001f) {
        _groundAlphaDrawn = groundAlpha;
        redrawGround(camera);
    }

    // Layer container visibility as in renderer.ts. layers[2] is also the
    // mask's scene parent, so it must stay visible: the ClippingNode would
    // otherwise hide the whole player layer, and the mask must never be drawn
    // outside it.
    layers[0]->setVisible(groundAlpha < 1.0f);
    layers[1]->setVisible(layerAlpha > 0.0f);
    layers[2]->setVisible(true);
    layers[3]->setVisible(true);
    ground->setVisible(groundAlpha > 0.0f);

    redrawLayerMask(camera, map);

    if (debugLayerMaskEnabled) {
        redrawDebugLayerMask(camera, map);
    }
    if (debugLayerMask) {
        debugLayerMask->setVisible(debugLayerMaskEnabled);
    }

#ifndef TEMP_NO_TREE_MASK
    const bool maskActive = layer == 0;
    if (maskActive && !layerMaskActive) {
        layers[2]->setMask(layerMask);
        layerMaskActive = true;
    } else if (!maskActive && layerMaskActive) {
        layers[2]->clearMask();
        layerMaskActive = false;
    }
#endif

    for (int i = 0; i < 4; i++) {
        if (_layerDirty[i]) {
            layers[i]->sortChildren();
            _layerDirty[i] = false;
        }
    }
}

} // namespace surv
