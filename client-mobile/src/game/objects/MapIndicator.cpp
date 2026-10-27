#include "MapIndicator.h"

#include "../../core/MathUtil.h"

namespace surv {

const Indicator* MapIndicatorBarn::find(int id) const {
    for (const auto& ind : indicators) {
        if (ind.active && ind.id == id) {
            return &ind;
        }
    }
    return nullptr;
}

void MapIndicatorBarn::addOrUpdate(int id, bool dead, bool equipped, const std::string& type,
                                   const Vec2& pos) {
    if (dead) {
        remove(id);
        return;
    }
    Indicator* ind = nullptr;
    for (auto& candidate : indicators) {
        if (candidate.active && candidate.id == id) {
            ind = &candidate;
            break;
        }
    }
    if (!ind) {
        // Reuse a freed slot first so a long match does not grow unbounded.
        for (auto& candidate : indicators) {
            if (!candidate.active) {
                candidate = Indicator{};
                candidate.active = true;
                candidate.id = id;
                ind = &candidate;
                break;
            }
        }
        if (!ind) {
            indicators.emplace_back();
            ind = &indicators.back();
            ind->active = true;
            ind->id = id;
        }
    }
    ind->pos = pos;
    ind->equipped = equipped;
    ind->type = type;
}

void MapIndicatorBarn::remove(int id) {
    for (auto& ind : indicators) {
        if (ind.active && ind.id == id) {
            ind.free();
            return;
        }
    }
}

void MapIndicatorBarn::updatePulses(float dt) {
    for (auto& ind : indicators) {
        if (!ind.active) {
            continue;
        }
        ind.pulseTicker = math::clamp(ind.pulseTicker + dt * ind.pulseDir * ind.pulseSpeed,
                                      kPulseScaleMin, 1.0f);
        ind.pulseScale = ind.pulseTicker * kPulseScaleMax;
        if (ind.pulseScale >= kPulseScaleMax || ind.pulseTicker <= kPulseScaleMin) {
            ind.pulseDir *= -1.0f;
        }
    }
}

void MapIndicatorBarn::clear() {
    for (auto& ind : indicators) {
        ind.free();
    }
    indicators.clear();
}

} // namespace surv
