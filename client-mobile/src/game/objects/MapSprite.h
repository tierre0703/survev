#pragma once
// Port of client/src/objects/mapSprite.ts: pooled sprite for the minimap overlay.
// MapSprites live on the minimap/bigmap, not in the main world renderer.
#include "../../render/PixiLike.h"
#include "../../core/Vec2.h"

#include <cstdint>
#include <vector>

namespace surv {

// A single sprite on the minimap/bigmap overlay (port of MapSprite).
class MapSprite {
public:
    bool active = false;
    bool retained = true;    // if true, lifetime does not auto-free

    pix::Sprite* sprite = nullptr; // the PIXI sprite (lazily created from factory)
    Vec2 pos;                // world position
    float scale = 1.0f;
    float alpha = 1.0f;
    bool visible = true;
    bool pulse = false;      // auto-expanding scale over time
    float lifetime = 0.0f;
    float ticker = 0.0f;
    int zOrder = 0;
    std::string frame;
    uint32_t tint = 0xffffff;

    void init(pix::Factory* factory);
    void free();
    void release();
};

// Pool of MapSprites (port of mapSprite.ts MapSpriteBarn).
class MapSpriteBarn {
public:
    MapSpriteBarn() = default;
    ~MapSpriteBarn() = default;

    pix::Container* container = nullptr; // parent container on minimap
    std::vector<MapSprite> sprites;      // all sprites (active + inactive)

    void free();
    MapSprite* addSprite(pix::Factory* factory);
    void update(float dt, float mapWidth, float mapHeight,
                float minimapX, float minimapY, float minimapW, float minimapH);
};

} // namespace surv