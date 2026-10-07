#pragma once
// axmol implementation of the PixiLike adapter (render/PixiLike.h).
//
//   PIXI.Graphics -> ax::DrawNode (immediate-mode vector fills/lines)
//   PIXI.Sprite   -> ax::Sprite (+ SpriteFrameCache)
//   PIXI.Text     -> ax::Label
//   PIXI.Container-> ax::Node (RenderGroup z-sort applied via localZOrder)
//   mask          -> ax::ClippingNode (stencil)
//   RenderTexture -> ax::RenderTexture (minimap)
//
// NOTE: this file is only compiled in the axmol app build (CMakeLists globs
// src/*.cpp), never by the host test build.
#include "PixiLike.h"
#include "FillGeometry.h"
#include "axmol.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace pix {

namespace detail {
inline ax::Color4F toColor4F(uint color, float alpha) {
    return ax::Color4F(static_cast<float>((color >> 16) & 0xff) / 255.0f,
                       static_cast<float>((color >> 8) & 0xff) / 255.0f,
                       static_cast<float>(color & 0xff) / 255.0f, alpha);
}
inline ax::Color3B toColor3B(uint color) {
    return ax::Color3B(static_cast<uint8_t>((color >> 16) & 0xff),
                       static_cast<uint8_t>((color >> 8) & 0xff),
                       static_cast<uint8_t>(color & 0xff));
}
} // namespace detail

// Common axmol Node implementation shared by all adapter node types.
template <class Iface>
class AxNodeImpl : public Iface {
public:
    ax::Node* _node = nullptr;
    Node* _parentNode = nullptr;
    std::vector<Node*> _children;
    int _sortOrd = 0;
    int _sortIdx = 0;
    Node* _maskNode = nullptr;
    ax::ClippingNode* _clipper = nullptr;
    bool _maskInverted = false;

    ~AxNodeImpl() override {
        // Release the reference taken in ownNode(). axmol nodes are
        // autoreleased by create(); wrappers that are never added to a parent
        // (e.g. the layer mask, which is only used as a ClippingNode stencil)
        // would otherwise be freed when the frame's pool drains.
        if (_clipper) {
            _clipper->release();
        }
        if (_node) {
            _node->release();
        }
    }

    ax::Node* axNode() const { return _node; }

    // Take ownership of a create()'d axmol node (retain it for the wrapper's
    // lifetime; released in the destructor).
    void ownNode(ax::Node* node) {
        _node = node;
        if (_node) {
            _node->retain();
            _node->setCascadeOpacityEnabled(true);
        }
    }

    void setPosition(float x, float y) override { _node->setPosition(x, y); }
    void setScale(float x, float y) override { _node->setScale(x, y); }
    void setRotation(float rad) override {
        _node->setRotation(rad * 180.0f / 3.14159265358979f);
    }
    void setAnchor(float x, float y) override { _node->setAnchorPoint(ax::Vec2(x, y)); }
    float getAnchorX() const override { return _node->getAnchorPoint().x; }
    float getAnchorY() const override { return _node->getAnchorPoint().y; }
    float getAlpha() const override { return _node->getOpacity() / 255.0f; }
    bool isVisible() const override { return _node->isVisible(); }
    void setAlpha(float alpha) override {
        const float a = alpha < 0.0f ? 0.0f : (alpha > 1.0f ? 1.0f : alpha);
        _node->setOpacity(static_cast<uint8_t>(a * 255.0f + 0.5f));
    }
    void setVisible(bool visible) override { _node->setVisible(visible); }
    void setTint(uint color) override { _node->setColor(detail::toColor3B(color)); }
    void setBlendMode(BlendMode) override {}
    void setLocalZOrder(int z) override { (_clipper ? static_cast<ax::Node*>(_clipper) : _node)->setLocalZOrder(z); }
    void setSortKey(int ord, int idx) override {
        _sortOrd = ord;
        _sortIdx = idx;
    }
    int getSortOrd() const override { return _sortOrd; }
    int getSortIdx() const override { return _sortIdx; }

    void addChild(Node* child) override {
        if (!child) {
            return;
        }
        detachForReparent(child);
        _node->addChild(static_cast<ax::Node*>(child->native()));
        child->setParent(this);
        _children.push_back(child);
        reindex();
    }
    void addChildAt(Node* child, int index) override {
        if (!child) {
            return;
        }
        detachForReparent(child);
        _node->addChild(static_cast<ax::Node*>(child->native()));
        child->setParent(this);
        if (index < 0 || index > static_cast<int>(_children.size())) {
            index = static_cast<int>(_children.size());
        }
        _children.insert(_children.begin() + index, child);
        reindex();
    }
    void removeChild(Node* child) override {
        _node->removeChild(static_cast<ax::Node*>(child->native()));
        for (auto it = _children.begin(); it != _children.end(); ++it) {
            if (*it == child) {
                _children.erase(it);
                break;
            }
        }
        if (child->getParent() == this) {
            child->setParent(nullptr);
        }
    }
    void removeFromParent() override {
        if (_parentNode) {
            _parentNode->removeChild(this);
        }
    }
    Node* getParent() const override { return _parentNode; }
    void setParent(Node* parent) override { _parentNode = parent; }
    int getChildIndex(Node* child) const override {
        for (size_t i = 0; i < _children.size(); i++) {
            if (_children[i] == child) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }
    void setChildIndex(Node* child, int index) override {
        const int cur = getChildIndex(child);
        if (cur < 0) {
            return;
        }
        _children.erase(_children.begin() + cur);
        if (index < 0 || index > static_cast<int>(_children.size())) {
            index = static_cast<int>(_children.size());
        }
        _children.insert(_children.begin() + index, child);
        reindex();
    }

    void setMask(Node* mask, bool inverted) override {
        if (!mask) {
            clearMask();
            return;
        }
        _maskNode = mask;
        _maskInverted = inverted;
        ax::Node* stencil = static_cast<ax::Node*>(mask->native());
        if (!_clipper) {
            _clipper = ax::ClippingNode::create();
            _clipper->retain();
            const int z = _node->getLocalZOrder();
            _clipper->setLocalZOrder(z);
            ax::Node* parent = _node->getParent();
            _node->retain();
            _node->removeFromParent();
            _clipper->addChild(_node);
            _node->setLocalZOrder(0);
            _node->release();
            if (parent) {
                parent->addChild(_clipper);
            }
        }
        _clipper->setInverted(_maskInverted);
        _clipper->setStencil(stencil);
    }
    void clearMask() override {
        if (!_clipper) {
            return;
        }
        _node->retain();
        const int z = _clipper->getLocalZOrder();
        _node->removeFromParent();
        _node->setLocalZOrder(z);
        ax::Node* parent = _clipper->getParent();
        if (parent) {
            parent->addChild(_node);
        }
        _node->release();
        if (_clipper->getParent()) {
            _clipper->removeFromParent();
        }
        _clipper->release();
        _clipper = nullptr;
        _maskNode = nullptr;
    }

    // A masked node's scene-graph handle is the wrapper, not its inner node;
    // otherwise reparenting or z-index changes silently bypass the stencil.
    void* native() override { return _clipper ? static_cast<ax::Node*>(_clipper) : _node; }

protected:
    // PIXI's Container.addChild removes the child from its current parent first
    // (and re-appends when it is already a child). axmol's Node::addChild
    // asserts instead, so without this an object moved between render layers
    // (or re-added with a new zIdx) ends up with two parents / duplicate
    // entries, corrupting the scene graph. Mirror the PIXI semantics here.
    void detachForReparent(Node* child) {
        Node* parent = child->getParent();
        if (parent == this) {
            _node->removeChild(static_cast<ax::Node*>(child->native()));
            for (auto it = _children.begin(); it != _children.end(); ++it) {
                if (*it == child) {
                    _children.erase(it);
                    break;
                }
            }
        } else if (parent) {
            parent->removeChild(child);
        }
    }

    // Assign a distinct localZOrder per child matching the pix child order, so
    // axmol's own z-sort reproduces the RenderGroup ordering.
    void reindex() {
        for (size_t i = 0; i < _children.size(); i++) {
            _children[i]->setLocalZOrder(static_cast<int>(i));
        }
    }
};

class AxGraphics : public AxNodeImpl<Graphics> {
public:
    AxGraphics() {
        ownNode(ax::DrawNode::create());
        _draw = static_cast<ax::DrawNode*>(_node);
    }

    void clear() override {
        _draw->clear();
        _geometry.clear();
        _fillActive = false;
        _hasLine = false;
        _haveLineStart = false;
    }
    void beginFill(uint color, float alpha) override {
        if (_fillActive) endFill();
        _fillColor = detail::toColor4F(color, alpha);
        _fillActive = true;
        _geometry.clear();
    }
    void endFill() override {
        if (_fillActive) {
            for (const auto& triangle : _geometry.triangles()) {
                // Convex triangles only: never enter axmol's poly2tri CDT.
                _draw->drawTriangle(ax::Vec2(triangle[0].x, triangle[0].y),
                                    ax::Vec2(triangle[1].x, triangle[1].y),
                                    ax::Vec2(triangle[2].x, triangle[2].y), _fillColor);
            }
        }
        _geometry.clear();
        _fillActive = false;
    }
    void beginHole() override { _geometry.beginHole(); }
    void endHole() override { _geometry.endHole(); }

    void lineStyle(float width, uint color, float alpha) override {
        _lineWidth = width;
        _lineColor = detail::toColor4F(color, alpha);
        _hasLine = alpha > 0.0f;
    }
    void moveTo(float x, float y) override {
        if (_hasLine) {
            _lineStart = ax::Vec2(x, y);
            _haveLineStart = true;
        } else {
            _geometry.moveTo(x, y);
        }
    }
    void lineTo(float x, float y) override {
        if (_hasLine) {
            if (_haveLineStart) {
                _draw->drawLine(_lineStart, ax::Vec2(x, y), _lineColor);
            }
            _lineStart = ax::Vec2(x, y);
        } else {
            _geometry.lineTo(x, y);
        }
    }
    void closePath() override { _geometry.closePath(); }
    void drawRect(float x, float y, float w, float h) override {
        if (_hasLine) {
            _draw->drawRect(ax::Vec2(x, y), ax::Vec2(x + w, y + h), _lineColor);
        } else if (_fillActive) {
            _geometry.rect(x, y, w, h);
        }
    }
    void drawCircle(float x, float y, float radius) override {
        if (_hasLine) {
            _draw->drawCircle(ax::Vec2(x, y), radius, 0.0f, 64, false, _lineColor);
        } else if (_fillActive) {
            _geometry.circle(x, y, radius);
        }
    }
    void drawDot(float x, float y, float radius) override {
        if (_fillActive) _geometry.circle(x, y, radius);
        else _draw->drawDot(ax::Vec2(x, y), radius, ax::Color4F(1, 1, 1, 1));
    }
    void drawPolygon(const surv::Vec2* points, int count) override {
        std::vector<ax::Vec2> pts;
        pts.reserve(static_cast<size_t>(count));
        for (int i = 0; i < count; i++) {
            pts.push_back(ax::Vec2(points[i].x, points[i].y));
        }
        if (_fillActive && count >= 3 && !_hasLine) {
            _geometry.polygon(points, count);
        } else if (_hasLine) {
            _draw->drawPoly(pts.data(), static_cast<unsigned int>(count), true, _lineColor);
        }
    }

private:
    ax::DrawNode* _draw = nullptr;
    FillGeometry _geometry;
    ax::Color4F _fillColor = ax::Color4F(1, 1, 1, 1);
    ax::Color4F _lineColor = ax::Color4F(0, 0, 0, 1);
    float _lineWidth = 1.0f;
    bool _fillActive = false;
    bool _hasLine = false;
    bool _haveLineStart = false;
    ax::Vec2 _lineStart;
};

class AxSprite : public AxNodeImpl<Sprite> {
public:
    AxSprite() {
        ownNode(ax::Sprite::create());
        _sprite = static_cast<ax::Sprite*>(_node);
    }
    explicit AxSprite(const std::string& frame) {
        ownNode(ax::Sprite::create());
        _sprite = static_cast<ax::Sprite*>(_node);
        setFrame(frame);
    }
    void setFrame(const std::string& frameName) override {
        if (frameName.empty()) {
            return;
        }
        auto* cache = ax::SpriteFrameCache::getInstance();
        if (auto* frame = cache->getSpriteFrameByName(frameName)) {
            _sprite->setSpriteFrame(frame);
        } else {
            _sprite->setTexture(frameName);
        }
    }
    float sourceWidth() const override { return _sprite->getContentSize().width; }
    float sourceHeight() const override { return _sprite->getContentSize().height; }

private:
    ax::Sprite* _sprite = nullptr;
};

class AxText : public AxNodeImpl<Text> {
public:
    AxText() {
        ownNode(ax::Label::createWithSystemFont("", "Arial", 24));
        _label = static_cast<ax::Label*>(_node);
    }
    void setText(const std::string& text) override { _label->setString(text); }
    void setFontSize(float size) override { _label->setSystemFontSize(size); }
    void setFontFamily(const std::string& family) override { _label->setSystemFontName(family); }
    void setColor(uint fill, uint stroke, float strokeThickness, bool bold) override {
        (void)bold;
        _label->setTextColor(ax::Color4B(detail::toColor3B(fill)));
        if (strokeThickness > 0.0f) {
            _label->enableOutline(ax::Color4B(detail::toColor3B(stroke)),
                                  static_cast<int>(strokeThickness));
        }
    }

private:
    ax::Label* _label = nullptr;
};

class AxContainer : public AxNodeImpl<Container> {
public:
    AxContainer() { ownNode(ax::Node::create()); }

    void sortChildren() override {
        std::stable_sort(_children.begin(), _children.end(), [](Node* a, Node* b) {
            if (a->getSortOrd() != b->getSortOrd()) {
                return a->getSortOrd() < b->getSortOrd();
            }
            return a->getSortIdx() < b->getSortIdx();
        });
        reindex();
    }
    std::size_t childCount() const override { return _children.size(); }
};

class AxRenderTexture : public RenderTexture {
public:
    explicit AxRenderTexture(int w, int h) {
        _rt = ax::RenderTexture::create(w, h);
        if (_rt) {
            _rt->retain();
        }
    }
    ~AxRenderTexture() override {
        if (_rt) {
            _rt->release();
        }
    }
    void resize(float width, float height) override {
        _rt->setContentSize(ax::Size(width, height));
    }
    ax::RenderTexture* get() const { return _rt; }

private:
    ax::RenderTexture* _rt = nullptr;
};

class AxRenderer : public Renderer {
public:
    void render(Node* node, RenderTexture* rt, bool clear) override {
        auto* axRt = dynamic_cast<AxRenderTexture*>(rt);
        if (!axRt || !axRt->get()) {
            return;
        }
        auto* axNode = static_cast<ax::Node*>(node->native());
        auto* target = axRt->get();
        if (clear) {
            target->beginWithClear(0.0f, 0.0f, 0.0f, 0.0f);
        } else {
            target->begin();
        }
        axNode->visit();
        target->end();
    }
};

// Factory owning the wrapper nodes for the lifetime of the game scene.
class AxPixiFactory : public Factory {
public:
    Graphics* createGraphics() override { return keep(new AxGraphics()); }
    Sprite* createSprite() override { return keep(new AxSprite()); }
    Sprite* createSprite(const std::string& frameName) override {
        return keep(new AxSprite(frameName));
    }
    Text* createText() override { return keep(new AxText()); }
    Container* createContainer() override { return keep(new AxContainer()); }
    RenderTexture* createRenderTexture() override {
        auto* rt = new AxRenderTexture(1, 1);
        _textures.emplace_back(rt);
        return rt;
    }

private:
    template <class T>
    T* keep(T* node) {
        _nodes.emplace_back(static_cast<Node*>(node));
        return node;
    }
    std::vector<std::unique_ptr<Node>> _nodes;
    std::vector<std::unique_ptr<RenderTexture>> _textures;
};

} // namespace pix
