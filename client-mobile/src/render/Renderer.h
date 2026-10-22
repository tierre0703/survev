#pragma once
// Port of client/src/renderer.ts: the z-sorted layer stack, stairs layer mask,
// and ground fade used by the web client.
#include "Camera.h"
#include "PixiLike.h"
#include <unordered_map>
#include <vector>

namespace surv {

class Map;

class Renderer {
public:
    static constexpr int kNoZIdx = INT32_MIN;

    int zIdx = 0;
    int layer = 0;
    float layerAlpha = 0.0f;
    float groundAlpha = 0.0f;
    bool underground = false;

    pix::Container* layers[4] = {nullptr, nullptr, nullptr, nullptr};
    pix::Graphics* ground = nullptr;
    pix::Graphics* layerMask = nullptr;
    pix::Graphics* debugLayerMask = nullptr;
    bool layerMaskDirty = true;
    bool layerMaskActive = false;

    Renderer(pix::Factory* factory, bool canvasMode);

    void m_free();
    // Match-change reset (GameWorld::loadMap): detaches every assembled object
    // and clears the per-match z-index/layer state.
    void reset();

    void addPIXIObj(pix::Node* obj, int layer_, int zOrd, int zIdx = kNoZIdx);
    void setActiveLayer(int layer_) { this->layer = layer_; }
    void setUnderground(bool u) { underground = u; }

    void resize(Map& map, Camera& camera);
    // Port of renderer.ts m_update()'s view transform (position + scale on
    // `pixiContainers`, i.e. the node GameWorld::attachTo() receives). Every
    // per-object draw already works in world/point space, so the camera lives
    // here instead of inside each object's projection.
    //   root.scale = camera.m_z(), y flipped
    //   root.position = camera.m_pointToScreen(m_pos)
    void applyCamera(pix::Node* root, const Camera& camera);
    // Re-apply the view transform after the zoom changed (ground rect, layer
    // mask and the stale debug mask all depend on the camera).
    void markViewDirty() { layerMaskDirty = true; }
    void redrawLayerMask(Camera& camera, Map& map);
    void redrawDebugLayerMask(Camera& camera, Map& map);
    void m_update(float dt, Camera& camera, Map& map, bool debugLayerMaskEnabled);

private:
    void redrawGround(Camera& camera);

    bool _canvasMode = false;
    pix::Factory* _factory = nullptr;
    bool _layerDirty[4] = {true, true, true, true};
    std::unordered_map<pix::Node*, int> _objLayer;
    // T6: DrawNode bakes vertex alpha, so the ground fade is applied by
    // rebuilding the rect with the current alpha rather than setAlpha().
    uint32_t _groundColor = 0x1b0e0b;
    float _groundAlphaDrawn = -1.0f;
};

} // namespace surv
