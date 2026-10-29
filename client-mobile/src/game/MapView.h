#pragma once
// One map view (client/src/ui/ui.ts + mapSprite.ts): the baked map texture, gas
// overlay/safe zone, player/team dots and map ping sprites, all projected from
// world units into a square design-space box.
//
// Used twice by game/Hud:
//  - the 140px in-game minimap: zoomed in (web mapSprite is ~3.9x the window)
//    and centred on the player, clipped to the window;
//  - the full-screen big map: the whole map fitted to the box.
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

// One server map indicator (mapIndicator.ts Indicator): a role/item marker
// drawn at its world position. `equipped` markers pulse and sort above the
// player dots (zOrder 655350 vs 1 in the web's sorted mapSprites container).
struct MapIndicatorMarker {
    Vec2 pos;
    std::string texture; // def mapIndicator.sprite
    uint32_t tint = 0xffffff;
    bool equipped = false;
    bool pulse = false; // def mapIndicator.pulse
    uint32_t pulseTint = 0xffffff;
    float pulseScale = 0.5f; // MapIndicatorBarn pulse (0.5..1)
    float scale = 0.1875f;   // ui.ts: (uiLayout Sm ? 0.15 : 0.2) * 1.25
};

// ui.ts updatePlayerMapSprites addSprite style for one player.
struct MapDotStyle {
    std::string texture = "player-map-inner.img";
    float scale = 0.2f;
    bool outer = false; // same-group white ring (hidden for custom role icons)
    int zBump = 0;      // role icons draw above normal dots
    bool custom = false;
};

// Pure style selection (dead/downed/role priority + scale, dotScale=0.2).
MapDotStyle mapDotStyle(bool sameGroup, bool dead, bool downed, const std::string& aliveIcon,
                        const std::string& deadIcon, float dotScale = 0.2f);

class MapView {
public:
    static constexpr float kTextureSize = 256.0f;
    // ui.ts: minimap window / map sprite size (256 / (0.75 * 1600 / 1.2)).
    // The minimap shows ~25.6% of the map width, centred on the player.
    static constexpr float kMinimapViewFraction = 0.256f;
    // ui.ts createPing: map ping textures drawn at ping.scale on a ~750px map
    // sprite land at `ping.scale * 64 * 0.5626` CSS px; reuse that constant
    // here so map pings keep the web's size on the minimap and big map.
    static constexpr float kPingDisplayPx = 36.0f;
    static constexpr int kDotPool = 40;
    static constexpr int kPingPool = 32;
    static constexpr int kLabelPool = 16;
    static constexpr int kIndicatorPool = 24;

    // Builds the view under `parent`, anchored at `pos` (parent/design space,
    // y up) with a `size` x `size` square. The whole view is clipped to the box.
    void build(pix::Factory* factory, pix::Container* parent, const Vec2& pos, float size);

    void ensurePingLayer();

    void setTexture(pix::RenderTexture* texture);
    void setMapSize(float width, float height) {
        _mapW = width;
        _mapH = height;
        updateViewTransform();
    }

    // Minimap mode (zoomed + player-centred + clipped) vs big-map mode (whole
    // map fitted). The big map never needs the clip because the texture fits.
    void setZoomed(bool zoomed);
    bool zoomed() const { return _zoomed; }
    // World position that should appear at the minimap centre (small map).
    void setCenter(const Vec2& worldPos);
    const Vec2& center() const { return _center; }

    void setVisible(bool visible);
    bool visible() const { return _visible; }

    // World -> local view coordinates. The minimap is zoomed, so these can be
    // outside [0,size] (the view is clipped); the big map stays inside.
    Vec2 project(const Vec2& worldPos) const;
    // Inverse, used by the big map for a tapped position.
    Vec2 unproject(const Vec2& localPos) const;

    bool contains(const Vec2& parentPos) const;
    const Vec2& pos() const { return _pos; }
    float size() const { return _size; }
    pix::Container* root() const { return _root; }

    void setDots(const std::vector<MapDot>& dots);
    void setPings(const std::vector<MapPing>& pings);
    // mapIndicator.ts: role/item markers (def sprite + optional pulse ring).
    void setIndicators(const std::vector<MapIndicatorMarker>& indicators);
    // mapIndicator.ts zOrder: an equipped indicator sorts above every player dot
    // (500+), a plain one below them; the pulse ring sits one lower. Pure so the
    // host tests can assert the rule (the Null adapter does not model z-sort).
    static int indicatorZOrder(bool equipped) { return equipped ? 655350 : 1; }
    static int indicatorPulseZOrder(bool equipped) { return indicatorZOrder(equipped) - 1; }
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
    pix::Sprite* indicatorSprite(int idx) const {
        return (idx >= 0 && idx < kIndicatorPool) ? _indicatorSprites[static_cast<size_t>(idx)] : nullptr;
    }
    pix::Sprite* indicatorPulseSprite(int idx) const {
        return (idx >= 0 && idx < kIndicatorPool) ? _indicatorPulseSprites[static_cast<size_t>(idx)] : nullptr;
    }
    pix::Text* labelText(int idx) const {
        return (idx >= 0 && idx < kLabelPool) ? _labels[static_cast<size_t>(idx)] : nullptr;
    }

private:
    pix::Factory* _factory = nullptr;
    pix::Container* _parent = nullptr;
    pix::Container* _root = nullptr;
    pix::Graphics* _clipMask = nullptr;
    pix::Container* _dotLayer = nullptr;
    pix::Container* _labelLayer = nullptr;
    pix::Container* _pingLayer = nullptr;
    pix::Sprite* _map = nullptr;
    pix::RenderTexture* _texture = nullptr;
    pix::Graphics* _gasGfx = nullptr;
    pix::Graphics* _strikeGfx = nullptr;
    std::array<pix::Sprite*, kDotPool> _dotSprites{};
    std::array<pix::Text*, kLabelPool> _labels{};
    std::array<pix::Sprite*, kPingPool> _pingSprites{};
    std::array<pix::Sprite*, kIndicatorPool> _indicatorSprites{};
    std::array<pix::Sprite*, kIndicatorPool> _indicatorPulseSprites{};
    Vec2 _pos;
    float _size = 140.0f;
    float _mapW = 0.0f;
    float _mapH = 0.0f;
    // World -> view: local = _mapOffset + world/map * _mapScale.
    float _mapScale = 140.0f;
    Vec2 _mapOffset;
    Vec2 _center;
    bool _zoomed = false;
    bool _visible = true;

    void updateViewTransform();
    void ensureClip();
};

} // namespace surv
