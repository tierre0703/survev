#pragma once
// Port of client/src/objects/mapIndicator.ts: minimap indicators for items/roles.
// Driven by UpdateMsg.mapIndicators data from the server.
// Uses MapSpriteBarn (pooled) for the main + pulse sprites.
#include "MapSprite.h"
#include "../../render/Defs.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace surv {

// One minimap indicator (port of Indicator interface).
class Indicator {
public:
    int id = 0;
    std::string type;
    Vec2 pos;
    bool equipped = false;
    bool active = false;

    MapSprite* mapSprite = nullptr;
    MapSprite* pulseSprite = nullptr;

    float pulseScale = 0.5f;
    float pulseScaleMin = 0.5f;
    float pulseScaleMax = 1.0f;
    float pulseTicker = 0.0f;
    float pulseDir = 1.0f;
    float pulseSpeed = 0.3f;

    void free();
};

// Pooled barn for indicators.
class MapIndicatorBarn {
public:
    std::vector<Indicator> indicators;
    std::unordered_map<int, Indicator*> idToIndicator;
    MapSpriteBarn* spriteBarn = nullptr;

    void init(MapSpriteBarn* barn);
    // Process indicator data: accepts raw struct arrays (surv::MapIndicator from Messages.h).
    // Since Messages.h would create a circular dep, callers pass fields directly.
    void addOrUpdate(int id, bool dead, bool equipped, const std::string& type,
                     const Vec2& pos, pix::Factory* factory);
    void updatePulses(float dt);
    void remove(int id);
    void free();
};

} // namespace surv