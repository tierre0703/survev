#pragma once
// Minimal PIXI.js -> axmol adapter surface. The game code (renderer, map,
// objects) is ported against this interface so the ported draw code stays
// close to the original TS. Concrete implementations:
//   * render/AxmolPixi.{h,cpp}  - real ax::Node/Sprite/DrawNode/ClippingNode
//   * render/NullPixi.h         - headless recording impl used by host tests
//
// Mapping summary (plan.md section 5.3):
//   PIXI.Application        -> ax::Scene (created in GameScene)
//   PIXI.Container          -> ax::Node (setLocalZOrder for __zOrd/__zIdx)
//   PIXI.Graphics           -> ax::DrawNode (tessellated polygon fills/holes)
//   PIXI.Sprite             -> ax::Sprite (+ SpriteFrameCache for atlases)
//   mask                    -> ax::ClippingNode (stencil)
//   tint / alpha / blend    -> Sprite::setColor / setOpacity / BlendFunc
//   PIXI.Text               -> ax::Label (BMFont/TTF)
#include "../core/Vec2.h"
#include <cstdint>
#include <string>
#include <vector>

namespace pix {

using uint = uint32_t;

enum class BlendMode {
    Normal,
    Add,
    Multiply,
    Screen,
};

// PIXI tint helpers (0xRRGGBB).
inline uint rgbToInt(int r, int g, int b) {
    return (static_cast<uint>(r & 0xff) << 16) | (static_cast<uint>(g & 0xff) << 8) |
           static_cast<uint>(b & 0xff);
}

// Base scene-graph node. Mirrors the subset of PIXI.DisplayObject used by the
// ported code.
class Node {
public:
    virtual ~Node() = default;

    virtual void setPosition(float x, float y) = 0;
    virtual void setScale(float x, float y) = 0;
    virtual void setRotation(float rad) = 0; // radians, like PIXI
    virtual void setAnchor(float x, float y) = 0;
    virtual void setAlpha(float alpha) = 0;
    virtual void setVisible(bool visible) = 0;
    virtual void setTint(uint color) = 0;
    virtual void setBlendMode(BlendMode mode) = 0;
    virtual void setLocalZOrder(int zOrd) = 0;

    // RenderGroup sort key (renderer.ts __zOrd / __zIdx). The concrete
    // Container::sortChildren orders children by these.
    virtual void setSortKey(int zOrd, int zIdx) = 0;
    virtual int getSortOrd() const = 0;
    virtual int getSortIdx() const = 0;

    virtual void addChild(Node* child) = 0;
    virtual void addChildAt(Node* child, int index) = 0;
    virtual void removeChild(Node* child) = 0;
    virtual void removeFromParent() = 0;
    virtual Node* getParent() const = 0;
    virtual void setParent(Node* parent) = 0;
    virtual int getChildIndex(Node* child) const = 0;
    virtual void setChildIndex(Node* child, int index) = 0;

    // PIXI `node.mask = mask`. On axmol this is realised with a ClippingNode.
    // `inverted` mirrors ClippingNode::setInverted (used when the mask is a
    // "hole" set rather than a "reveal" set).
    virtual void setMask(Node* mask, bool inverted = false) = 0;
    virtual void clearMask() = 0;

    // Opaque native handle (ax::Node*) for attaching to a scene.
    virtual void* native() = 0;
};

// PIXI.Graphics equivalent (vector paths).
class Graphics : public virtual Node {
public:
    virtual void clear() = 0;
    virtual void beginFill(uint color, float alpha = 1.0f) = 0;
    virtual void endFill() = 0;
    virtual void beginHole() = 0;
    virtual void endHole() = 0;
    virtual void lineStyle(float width, uint color, float alpha = 1.0f) = 0;
    virtual void moveTo(float x, float y) = 0;
    virtual void lineTo(float x, float y) = 0;
    virtual void closePath() = 0;
    virtual void drawRect(float x, float y, float w, float h) = 0;
    virtual void drawCircle(float x, float y, float radius) = 0;
    virtual void drawDot(float x, float y, float radius) = 0;
    virtual void drawPolygon(const surv::Vec2* points, int count) = 0;
};

// PIXI.Sprite equivalent.
class Sprite : public virtual Node {
public:
    virtual void setFrame(const std::string& frameName) = 0;
    // Source size in pixels (PIXI sprite.width/height), used to convert a
    // world-space radius into a sprite scale.
    virtual float sourceWidth() const = 0;
    virtual float sourceHeight() const = 0;
};

// PIXI.Text equivalent.
class Text : public virtual Node {
public:
    virtual void setText(const std::string& text) = 0;
    virtual void setFontSize(float size) = 0;
    virtual void setFontFamily(const std::string& family) = 0;
    virtual void setColor(uint fill, uint stroke, float strokeThickness, bool bold) = 0;
};

// PIXI.Container equivalent with RenderGroup sorted-child semantics.
class Container : public virtual Node {
public:
    virtual void sortChildren() = 0;
    virtual std::size_t childCount() const = 0;
};

// PIXI.RenderTexture equivalent (used for the minimap).
class RenderTexture {
public:
    virtual ~RenderTexture() = default;
    virtual void resize(float width, float height) = 0;
};

// PIXI.IRenderer equivalent.
class Renderer {
public:
    virtual ~Renderer() = default;
    virtual void render(Node* node, RenderTexture* rt, bool clear) = 0;
};

// Factory for the concrete node implementations.
class Factory {
public:
    virtual ~Factory() = default;
    virtual Graphics* createGraphics() = 0;
    virtual Sprite* createSprite() = 0;
    virtual Sprite* createSprite(const std::string& frameName) = 0;
    virtual Text* createText() = 0;
    virtual Container* createContainer() = 0;
    virtual RenderTexture* createRenderTexture() = 0;
};

} // namespace pix
