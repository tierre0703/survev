#pragma once
// Port of client/src/objects/mapIndicator.ts: role/item markers on the map.
// Driven by UpdateMsg.mapIndicators data from the server.
//
// The web keeps a MapSprite (mapSprite.ts) per indicator for its sorted minimap
// container; the native port keeps the data here and `Hud`/`MapView` draw the
// markers (def `mapIndicator.sprite` + the optional pulse ring), so there is no
// second sprite pool. Layout of the two call sites:
//   - UpdateMsg  -> GameWorld::applyUpdate -> MapIndicatorBarn::addOrUpdate
//   - GameWorld  -> update(dt)             -> MapIndicatorBarn::updatePulses
//   - GameScene  -> Hud::updateMapIndicators (builds MapIndicatorMarker list)
#include "../../core/Vec2.h"

#include <string>
#include <vector>

namespace surv {

// One map indicator (port of the mapIndicator.ts `Indicator` fields the view
// needs). `type` is the item/role def type (its def carries the sprite/tints).
class Indicator {
public:
    int id = 0;
    std::string type;
    Vec2 pos;
    bool equipped = false;
    bool active = false;

    // mapIndicator.ts updateIndicatorPulses: ticker eases 0.5..1, scale =
    // ticker * pulseScaleMax, direction flips at the ends.
    float pulseTicker = 0.5f;
    float pulseScale = 0.5f;
    float pulseDir = 1.0f;
    float pulseSpeed = 0.3f;

    void free() {
        active = false;
        type.clear();
        pos = Vec2(0.0f, 0.0f);
        equipped = false;
    }
};

class MapIndicatorBarn {
public:
    static constexpr float kPulseScaleMin = 0.5f;
    static constexpr float kPulseScaleMax = 1.0f;

    std::vector<Indicator> indicators;

    // `dead` frees the indicator (server removed it), otherwise add/update.
    void addOrUpdate(int id, bool dead, bool equipped, const std::string& type, const Vec2& pos);
    void updatePulses(float dt);
    void remove(int id);
    // Drops every indicator (world rebuild / match end).
    void clear();

    const Indicator* find(int id) const;
};

} // namespace surv
