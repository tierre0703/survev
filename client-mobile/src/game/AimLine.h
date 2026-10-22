#pragma once
// Port of the `LineSprites` helper in client/src/ui/touch.ts: the dotted aim
// line drawn from the active player toward the aim direction, clipped at the
// first obstacle it hits. Kept engine-independent (it only touches the adapter
// factory + Map/Camera/Renderer), so the host tests can drive it with
// NullPixi and assert the dot placement.
#include "../core/GameConfig.h"
#include "../core/MathUtil.h"
#include "../core/Vec2.h"
#include "../net/generatedDefs.inc"
#include "../render/Camera.h"
#include "../render/PixiLike.h"
#include "../render/Renderer.h"
#include "Map.h"
#include "objects/Barns.h"

#include <cmath>
#include <vector>

namespace surv {

// collisionHelpers.intersectSegmentObstacle (PlayerRender.cpp): the obstacle
// filters used by bullets/melee. The aim line adds its own height/collidable
// checks first (touch.ts inlines them).
inline bool aimLineSameLayer(int a, int b) {
    return ((a & 0x1) == (b & 0x1)) || (((a & 0x2) != 0) && ((b & 0x2) != 0));
}

class LineSprites {
public:
    // touch.ts LineSprites.update(touch, activePlayer, map, camera, renderer).
    // `visible` is device.touch && touch.touchingAim && touch.touchAimLine;
    // `mobile` mirrors device.mobile for the player zoom remap.
    void update(bool visible, Player& activePlayer, Map& map, Camera& camera, Renderer& renderer,
                pix::Factory* factory, bool mobile) {
        if (!factory) {
            return;
        }
        if (!container) {
            container = factory->createContainer();
            container->setVisible(false);
        }

        if (visible) {
            const DefProvider* provider = getDefProvider();
            const GameObjRenderDef* weapDef = provider ? provider->gameObject(activePlayer.activeWeapon)
                                                       : nullptr;

            // Determine the max range of the aim line: guns add their barrel
            // length to the bullet's travel distance, everything else uses the
            // 30-unit default (fists/melee/throwables).
            float maxRange = 30.0f;
            if (weapDef && weapDef->category == "gun") {
                const BulletRenderDef* bulletDef =
                    provider ? provider->bullet(weapDef->bulletType) : nullptr;
                const float bulletDist = bulletDef ? bulletDef->distance : 0.0f;
                maxRange = weapDef->barrelLength + bulletDist;
            }

            // Clamp the range to the camera radius (sqrt(zoom * 1.414 * zoom)).
            // When the player has no scope (zoom == 0), the line is not clamped
            // to a camera-visible radius.
            const float cameraZoom = activePlayer.getZoom(mobile);
            const float cameraRad = cameraZoom > 0.0f ? std::sqrt(cameraZoom * 1.414f * cameraZoom) : 1e6f;
            maxRange = math::min(maxRange, cameraRad);

            const Vec2 start = activePlayer.pos;
            Vec2 end = v2Add(start, v2Mul(activePlayer.dir, maxRange));

            // Shrink to the nearest intersecting obstacle.
            const bool isThrowable = weapDef && weapDef->category == "throwable";
            for (auto* obstacle : map.obstaclePool.m_getPool()) {
                if (!obstacle) {
                    continue;
                }
                if (!obstacle->active || obstacle->dead ||
                    obstacle->height < defs::kBulletHeight || !obstacle->collidable ||
                    obstacle->isWindow || !aimLineSameLayer(activePlayer.layer, obstacle->layer)) {
                    continue;
                }
                if (isThrowable && obstacle->height <= defs::kProjectileMaxHeightGame) {
                    continue;
                }
                Vec2 point;
                if (colliderIntersectSegmentRes(obstacle->collider, start, end, &point)) {
                    const float dist = v2Length(v2Sub(point, start));
                    if (dist < maxRange) {
                        maxRange = dist;
                        end = point;
                    }
                }
            }

            const float startOffset = 3.5f;
            const float increment = 1.5f;
            const float dist = v2Length(v2Sub(end, start));
            const int dotCount =
                math::max(static_cast<int>(std::ceil((dist - startOffset) / increment)), 0);

            while (static_cast<int>(dots.size()) < dotCount) {
                pix::Sprite* dot = factory->createSprite();
                dot->setAnchor(0.5f, 0.5f);
                dot->setPosition(0.0f, 0.0f);
                dot->setScale(1.0f, 1.0f);
                dot->setTint(0xffffff);
                dot->setAlpha(1.0f);
                dot->setVisible(false);
                container->addChild(dot);
                dots.push_back(dot);
            }
            line = dots.empty() ? nullptr : dots.front();

            for (size_t i = 0; i < dots.size(); i++) {
                pix::Sprite* dot = dots[i];
                const float offset = startOffset + static_cast<float>(i) * increment;
                const Vec2 pos = v2Add(activePlayer.pos, v2Mul(activePlayer.dir, offset));
                const float scale = (1.0f / 32.0f) * 0.375f;
                dot->setFrame("dot.img");
                dot->setPosition(pos.x, pos.y);
                dot->setScale(scale, scale);
                dot->setVisible(static_cast<int>(i) < dotCount);
            }

            // touch.ts draws the container in world space; the camera transform
            // lives on the world root.
            container->setPosition(0.0f, 0.0f);
            container->setScale(1.0f, 1.0f);
            container->setAlpha(0.3f);
            renderer.addPIXIObj(container, activePlayer.layer, 19, 0);
        }
        container->setVisible(visible);
    }

    // Release the adapter nodes (the world root is torn down separately).
    void m_free() {
        if (container) {
            container->removeFromParent();
            container->setVisible(false);
        }
        dots.clear();
        line = nullptr;
    }

    // Diagnostics/tests.
    pix::Container* container = nullptr;
    std::vector<pix::Sprite*> dots;
    pix::Sprite* line = nullptr;
};

} // namespace surv
