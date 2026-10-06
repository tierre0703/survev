#pragma once
// Port of client/src/renderer.ts: the z-sorted layer stack, stairs layer mask,
// and ground fade used by the web client.
#include "Camera.h"
#include "PixiLike.h"
#include <cstdint>
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

    void addPIXIObj(pix::Node* obj, int layer_, int zOrd, int zIdx = kNoZIdx);
    void setActiveLayer(int layer_) { this->layer = layer_; }
    void setUnderground(bool u) { underground = u; }

    void resize(Map& map, Camera& camera);
    void redrawLayerMask(Camera& camera, Map& map);
    void redrawDebugLayerMask(Camera& camera, Map& map);
    void m_update(float dt, Camera& camera, Map& map, bool debugLayerMaskEnabled);

private:
    bool _canvasMode = false;
    pix::Factory* _factory = nullptr;
    bool _layerDirty[4] = {true, true, true, true};
    std::unordered_map<pix::Node*, int> _objLayer;
};

} // namespace surv
