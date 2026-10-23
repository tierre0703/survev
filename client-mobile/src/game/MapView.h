#pragma once
// One minimap-style view of the whole map (client/src/ui/ui.ts + mapSprite.ts):
// the baked map texture, gas overlay/safe zone, player/team dots and map ping
// sprites, all projected from world units into a square design-space box and
// clipped to it.
//
// Used twice by game/Hud: the 140px in-game minimap and the full-screen big map.
#include "../core/Vec2.h"
#include "../render/PixiLike.h"
#include "EmoteManager.h"

#include <array>
#include <string>
#include <vector>

namespace surv {

// One player/team marker (ui.ts updatePlayerMapSprites addSprite input).
struct MapDot {
    Vec2 pos;
    float scale = 0.2f; // in map.ts screenScale units (256px minimap space)
    uint32_t tint = 0xffffff;
    std::string texture;
    float alpha = 1.0f;
    int zOrder = 0;
};

// Gas/safe-zone circles in world units (ui.ts m_render).
struct MapGas {
    bool active = false;
    Vec2 pos;
    float rad = 0.0f;
    bool hasSafe = false;
    Vec2 safePos;
    float safeRad = 0.0f;
    Vec2 playerPos;
};

// One airstrike warning circle (plane.ts AirstrikeZone).
struct MapAirstrike {
    Vec2 pos;
    float rad = 0.0f;
    float alpha = 1.0f;
};

class MapView {
public:
    static constexpr float kTextureSize = 256.0f;
    static constexpr int kDotPool = 40;
    static constexpr int kPingPool = 32;
    static constexpr int kLabelPool = 16;

    // Builds the view under `parent`, anchored at `pos` (parent/design space,
    // y up) with a `size` x `size` square. The whole view is clipped to the box.
    void build(pix::Factory* factory, pix::Container* parent, const Vec2& pos, float size);

    void ensurePingLayer();

    void setTexture(pix::RenderTexture* texture);
    void setMapSize(float width, float height) {
        _mapW = width;
        _mapH = height;
    }

    void setVisible(bool visible);
    bool visible() const { return _visible; }

    // World -> local (0..size) minimap coordinates.
    Vec2 project(const Vec2& worldPos) const;
    // Inverse, used by the big map for a tapped position.
    Vec2 unproject(const Vec2& localPos) const;

    bool contains(const Vec2& parentPos) const;
    const Vec2& pos() const { return _pos; }
    float size() const { return _size; }
    pix::Container* root() const { return _root; }

    void setDots(const std::vector<MapDot>& dots);
    void setPings(const std::vector<MapPing>& pings);
    void setGas(const MapGas& gas);
    void setAirstrikes(const std::vector<MapAirstrike>& strikes);
    // map.ts renderMap nameContainer: place names over the map.
    void setLabels(const std::vector<MapPlace>& places);

    // Test/diagnostic accessors.
    pix::Sprite* mapSprite() const { return _map; }
    pix::Sprite* dotSprite(int idx) const {
        return (idx >= 0 && idx < kDotPool) ? _dotSprites[static_cast<size_t>(idx)] : nullptr;
    }
    pix::Sprite* pingSprite(int idx) const {
        return (idx >= 0 && idx < kPingPool) ? _pingSprites[static_cast<size_t>(idx)] : nullptr;
    }
    pix::Graphics* gasGraphics() const { return _gasGfx; }
    pix::Graphics* airstrikeGraphics() const { return _strikeGfx; }
    pix::Text* labelText(int idx) const {
        return (idx >= 0 && idx < kLabelPool) ? _labels[static_cast<size_t>(idx)] : nullptr;
    }

private:
    pix::Factory* _factory = nullptr;
    pix::Container* _parent = nullptr;
    pix::Container* _root = nullptr;
    pix::Container* _dotLayer = nullptr;
    pix::Container* _labelLayer = nullptr;
    pix::Container* _pingLayer = nullptr;
    pix::Sprite* _map = nullptr;
    pix::Graphics* _gasGfx = nullptr;
    pix::Graphics* _strikeGfx = nullptr;
    std::array<pix::Sprite*, kDotPool> _dotSprites{};
    std::array<pix::Text*, kLabelPool> _labels{};
    std::array<pix::Sprite*, kPingPool> _pingSprites{};
    Vec2 _pos;
    float _size = 140.0f;
    float _mapW = 0.0f;
    float _mapH = 0.0f;
    bool _visible = true;
};

} // namespace surv
