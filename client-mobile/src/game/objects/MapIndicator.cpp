#include "MapIndicator.h"
#include "../../render/Defs.h"

#include <cmath>
#include <algorithm>

namespace surv {

void Indicator::free() {
    active = false;
    mapSprite = nullptr;
    pulseSprite = nullptr;
}

void MapIndicatorBarn::init(MapSpriteBarn* barn) {
    spriteBarn = barn;
}

void MapIndicatorBarn::addOrUpdate(int id, bool dead, bool equipped, const std::string& type,
                                    const Vec2& pos, pix::Factory* factory) {
    if (dead) {
        remove(id);
        return;
    }

    auto it = idToIndicator.find(id);
    Indicator* ind = nullptr;
    if (it == idToIndicator.end()) {
        // Find free slot.
        for (auto& i : indicators) {
            if (!i.active) {
                i.active = true;
                i.id = id;
                i.type = type;
                ind = &i;
                break;
            }
        }
        if (!ind) {
            indicators.push_back({});
            ind = &indicators.back();
            ind->active = true;
            ind->id = id;
        }
        ind->pulseScale = 0.5f;
        ind->pulseTicker = 0.0f;
        ind->pulseDir = 1.0f;
        ind->pulseSpeed = 0.3f;
        ind->mapSprite = spriteBarn ? spriteBarn->addSprite(factory) : nullptr;
        ind->pulseSprite = spriteBarn ? spriteBarn->addSprite(factory) : nullptr;
        idToIndicator[id] = ind;
    } else {
        ind = it->second;
        if (!ind) {
            remove(id);
            return;
        }
    }

    ind->pos = pos;
    ind->equipped = equipped;
    ind->type = type;

    const DefProvider* provider = getDefProvider();
    const GameObjRenderDef* objDef = provider ? provider->gameObject(ind->type) : nullptr;
    if (!objDef) return;

    const std::string& sprite = objDef->mapIndicatorSprite;
    if (sprite.empty()) return;

    const float indicatorScale = 0.15f * 1.25f;
    const int zOrder = ind->equipped ? 655350 : 1;

    if (ind->mapSprite) {
        ind->mapSprite->pos = ind->pos;
        ind->mapSprite->scale = indicatorScale;
        ind->mapSprite->alpha = 1.0f;
        ind->mapSprite->zOrder = zOrder;
        ind->mapSprite->visible = true;
        ind->mapSprite->frame = sprite;
        ind->mapSprite->tint = objDef->mapIndicatorTint;
        if (ind->mapSprite->sprite)
            ind->mapSprite->sprite->setTint(ind->mapSprite->tint);
    }

    if (objDef->mapIndicatorPulse && ind->pulseSprite) {
        ind->pulseSprite->pos = ind->pos;
        ind->pulseSprite->scale = 0.5f;
        ind->pulseSprite->alpha = 1.0f;
        ind->pulseSprite->zOrder = zOrder - 1;
        ind->pulseSprite->visible = ind->equipped;
        ind->pulseSprite->frame = "part-pulse-01.img";
        ind->pulseSprite->tint = objDef->mapIndicatorPulseTint;
        if (ind->pulseSprite->sprite)
            ind->pulseSprite->sprite->setTint(ind->pulseSprite->tint);
    }
}

void MapIndicatorBarn::remove(int id) {
    auto it = idToIndicator.find(id);
    if (it == idToIndicator.end()) return;
    Indicator* ind = it->second;
    if (ind) {
        if (ind->mapSprite) ind->mapSprite->free();
        if (ind->pulseSprite) ind->pulseSprite->free();
        ind->free();
    }
    idToIndicator.erase(it);
}

void MapIndicatorBarn::updatePulses(float dt) {
    for (auto& ind : indicators) {
        if (!ind.active) continue;
        if (ind.pulseSprite && ind.equipped) {
            ind.pulseTicker += dt * ind.pulseDir * ind.pulseSpeed;
            if (ind.pulseTicker < ind.pulseScaleMin) {
                ind.pulseTicker = ind.pulseScaleMin;
                ind.pulseDir = 1.0f;
            } else if (ind.pulseTicker > 1.0f) {
                ind.pulseTicker = 1.0f;
                ind.pulseDir = -1.0f;
            }
            ind.pulseScale = ind.pulseTicker * ind.pulseScaleMax;
            ind.pulseSprite->scale = ind.pulseScale;
        }
    }
}

void MapIndicatorBarn::free() {
    for (auto& ind : indicators) {
        if (ind.mapSprite) ind.mapSprite->free();
        if (ind.pulseSprite) ind.pulseSprite->free();
    }
    indicators.clear();
    idToIndicator.clear();
}

} // namespace surv