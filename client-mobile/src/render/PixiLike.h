#pragma once
// Minimal PIXI.js -> axmol adapter surface. The game code (map, objects,
// renderer) is ported against this interface so ported draw code stays close
// to the original TS. Implementations live in app/ (axmol) and can be stubbed
// in tests.
//
// Mapping summary (from plan.md section 5.3):
//   PIXI.Application        -> ax::Scene (created in GameScene)
//   PIXI.Container          -> ax::Node (setLocalZOrder for __zOrd/__zIdx)
//   PIXI.Graphics           -> ax::DrawNode (vector paths; no hole support)
//   PIXI.Sprite             -> ax::Sprite (+ SpriteFrameCache for atlases)
//   mask / beginHole        -> ax::ClippingNode (stencil)
//   tint / alpha / blend    -> Sprite::setColor / setOpacity / BlendFunc
//   PIXI.Text               -> ax::Label (BMFont/TTF)
#include "../core/Vec2.h"
#include <cstdint>

namespace pix {

using uint = uint32_t;
using float32 = float;

// Minimal vector drawing surface used by ported render code.
class Graphics {
public:
    virtual ~Graphics() = default;
    virtual void clear() = 0;
    virtual void beginFill(uint color, float alpha = 1.0f) = 0;
    virtual void endFill() = 0;
    virtual void beginHole() = 0;
    virtual void endHole() = 0;
    virtual void moveTo(float x, float y) = 0;
    virtual void lineTo(float x, float y) = 0;
    virtual void closePath() = 0;
    virtual void drawRect(float x, float y, float w, float h) = 0;
    virtual void drawCircle(float x, float y, float radius) = 0;
    virtual void setPosition(float x, float y) = 0;
    virtual void setScale(float x, float y) = 0;
    virtual void setAlpha(float alpha) = 0;
};

// Sprite-like node with z-ordering (PIXI.Container child with __zOrd/__zIdx).
class Sprite {
public:
    virtual ~Sprite() = default;
    virtual void setPosition(float x, float y) = 0;
    virtual void setScale(float x, float y) = 0;
    virtual void setRotation(float rad) = 0;
    virtual void setAnchor(float x, float y) = 0;
    virtual void setTint(uint color) = 0;
    virtual void setAlpha(float alpha) = 0;
    virtual void setVisible(bool visible) = 0;
    virtual void setLocalZOrder(int zOrd) = 0;
    // PIXI zIdx tie-breaker; axmol keeps insertion order so this is implicit.
    virtual void addChild(Sprite* child) = 0;
};

// Container with the RenderGroup sorted-children semantics from renderer.ts.
class Container {
public:
    virtual ~Container() = default;
    virtual void addChild(Sprite* child) = 0;
    virtual void removeChild(Sprite* child) = 0;
    virtual void setAlpha(float alpha) = 0;
    virtual void setVisible(bool visible) = 0;
    // like RenderGroup::checkSort()
    virtual void sortChildren() = 0;
};

} // namespace pix