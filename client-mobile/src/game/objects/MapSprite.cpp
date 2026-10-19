#include "MapSprite.h"

#include <cmath>
#include <algorithm>

namespace surv {

void MapSprite::init(pix::Factory* factory) {
    if (!sprite && factory) {
        sprite = factory->createSprite();
        sprite->setAnchor(0.5f, 0.5f);
        sprite->setScale(1.0f, 1.0f);
        sprite->setVisible(false);
    }
    active = true;
    retained = true;
    pos = Vec2(0.0f, 0.0f);
    scale = 1.0f;
    alpha = 1.0f;
    visible = true;
    pulse = false;
    lifetime = 3.402823466e+38f; // FLT_MAX
    ticker = 0.0f;
    zOrder = 0;
    frame.clear();
    tint = 0xffffff;
    if (sprite) {
        sprite->setVisible(true);
    }
}

void MapSprite::free() {
    active = false;
    if (sprite) {
        sprite->setVisible(false);
    }
}

void MapSprite::release() {
    retained = false;
}

static float smoothstep(float t) {
    return t * t * (3.0f - 2.0f * t);
}



void MapSpriteBarn::free() {
    for (auto& ms : sprites) {
        if (ms.sprite) {
            ms.sprite->removeFromParent();
        }
    }
    sprites.clear();
    if (container) {
        container->removeFromParent();
        container = nullptr;
    }
}

MapSprite* MapSpriteBarn::addSprite(pix::Factory* factory) {
    for (auto& ms : sprites) {
        if (!ms.active) {
            ms.init(factory);
            return &ms;
        }
    }
    sprites.push_back({});
    auto* ms = &sprites.back();
    ms->init(factory);
    if (ms->sprite && container) {
        container->addChild(ms->sprite);
    }
    return ms;
}

void MapSpriteBarn::update(float dt, float mapWidth, float mapHeight,
                            float minimapX, float minimapY,
                            float minimapW, float minimapH) {
    bool sortNeeded = false;
    for (auto& ms : sprites) {
        if (!ms.active) continue;

        ms.ticker += dt;
        if (ms.pulse) {
            ms.scale += dt / 2.5f;
        }

        // Convert world pos to minimap screen coords.
        const float sx = minimapX + (ms.pos.x / mapWidth) * minimapW;
        const float sy = minimapY + minimapH - (ms.pos.y / mapHeight) * minimapH;

        // Fade: smoothstep fade-in over 0.1s, smoothstep fade-out over last 0.5s.
        const float fadeInTime = 0.1f;
        const float fadeOutTime = 0.5f;
        float fade = 1.0f;
        if (ms.ticker < fadeInTime) {
            fade = smoothstep(ms.ticker / fadeInTime);
        }
        if (ms.lifetime - ms.ticker < fadeOutTime && ms.lifetime > 0) {
            const float t = (ms.lifetime - ms.ticker) / fadeOutTime;
            fade *= smoothstep(t > 0 ? t : 0);
        }

        if (ms.sprite) {
            ms.sprite->setPosition(sx, sy);
            ms.sprite->setScale(ms.scale, ms.scale);
            ms.sprite->setAlpha(ms.alpha * fade);
            ms.sprite->setVisible(ms.visible && (ms.alpha * fade) > 0.0001f);
            if (!ms.frame.empty()) {
                ms.sprite->setFrame(ms.frame);
            }
            if (ms.tint != 0xffffff) {
                ms.sprite->setTint(ms.tint);
            }
        }

        if (!ms.retained && ms.ticker >= ms.lifetime) {
            ms.free();
        }
    }
}

} // namespace surv