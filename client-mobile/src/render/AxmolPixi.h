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

    ax::Node* axNode() const { return _node; }

    void setPosition(float x, float y) override { _node->setPosition(x, y); }
    void setScale(float x, float y) override { _node->setScale(x, y); }
    void setRotation(float rad) override {
        _node->setRotation(rad * 180.0f / 3.14159265358979f);
    }
    void setAnchor(float x, float y) override { _node->setAnchorPoint(ax::Vec2(x, y)); }
    void setAlpha(float alpha) override {
        const float a = alpha < 0.0f ? 0.0f : (alpha > 1.0f ? 1.0f : alpha);
        _node->setOpacity(static_cast<uint8_t>(a * 255.0f + 0.5f));
    }
    void setVisible(bool visible) override { _node->setVisible(visible); }
    void setTint(uint color) override { _node->setColor(detail::toColor3B(color)); }
    void setBlendMode(BlendMode) override {}
    void setLocalZOrder(int z) override { _node->setLocalZOrder(z); }
    void setSortKey(int ord, int idx) override {
        _sortOrd = ord;
        _sortIdx = idx;
    }
    int getSortOrd() const override { return _sortOrd; }
    int getSortIdx() const override { return _sortIdx; }

    void addChild(Node* child) override {
        _node->addChild(static_cast<ax::Node*>(child->native()));
        child->setParent(this);
        _children.push_back(child);
    }
    void addChildAt(Node* child, int index) override {
        _node->addChild(static_cast<ax::Node*>(child->native()));
        child->setParent(this);
        if (index < 0 || index > static_cast<int>(_children.size())) {
            index = static_cast<int>(_children.size());
        }
        _children.insert(_children.begin() + index, child);
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
            _clipper->setInverted(_maskInverted);
            ax::Node* parent = _node->getParent();
            _node->retain();
            _node->removeFromParent();
            _clipper->addChild(_node);
            _node->release();
            if (parent) {
                parent->addChild(_clipper);
            }
        }
        _clipper->setStencil(stencil);
    }
    void clearMask() override {
        if (!_clipper) {
            return;
        }
        _node->retain();
        _node->removeFromParent();
        ax::Node* parent = _clipper->getParent();
        if (parent) {
            parent->addChild(_node);
        }
        _node->release();
        if (_clipper->getParent()) {
            _clipper->removeFromParent();
        }
        _clipper = nullptr;
        _maskNode = nullptr;
    }

    void* native() override { return _node; }

protected:
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
        _draw = ax::DrawNode::create();
        _node = _draw;
    }

    void clear() override {
        _draw->clear();
        _path.clear();
        _fillActive = false;
        _hasLine = false;
    }
    void beginFill(uint color, float alpha) override {
        _fillColor = detail::toColor4F(color, alpha);
        _fillActive = true;
        _path.clear();
    }
    void endFill() override {
        if (_path.size() >= 3) {
            _draw->drawSolidPoly(_path.data(), static_cast<unsigned int>(_path.size()), _fillColor);
        }
        _path.clear();
        _fillActive = false;
    }
    // DrawNode cannot express polygon holes; the ported map falls back to the
    // canvas behaviour (draw the grass polygon on top) so these are no-ops.
    void beginHole() override {}
    void endHole() override {}

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
            _path.clear();
            _path.push_back(ax::Vec2(x, y));
        }
    }
    void lineTo(float x, float y) override {
        if (_hasLine) {
            if (_haveLineStart) {
                _draw->drawLine(_lineStart, ax::Vec2(x, y), _lineColor);
            }
        } else {
            _path.push_back(ax::Vec2(x, y));
        }
    }
    void closePath() override {}
    void drawRect(float x, float y, float w, float h) override {
        if (_hasLine) {
            _draw->drawRect(ax::Vec2(x, y), ax::Vec2(x + w, y + h), _lineColor);
        } else if (_fillActive) {
            _draw->drawSolidRect(ax::Vec2(x, y), ax::Vec2(x + w, y + h), _fillColor);
        }
    }
    void drawCircle(float x, float y, float radius) override {
        if (_hasLine) {
            _draw->drawCircle(ax::Vec2(x, y), radius, 0.0f, 64, false, _lineColor);
        } else if (_fillActive) {
            _draw->drawSolidCircle(ax::Vec2(x, y), radius, 0.0f, 64, _fillColor);
        }
    }
    void drawDot(float x, float y, float radius) override {
        _draw->drawDot(ax::Vec2(x, y), radius,
                       _fillActive ? _fillColor : ax::Color4F(1, 1, 1, 1));
    }
    void drawPolygon(const surv::Vec2* points, int count) override {
        std::vector<ax::Vec2> pts;
        pts.reserve(static_cast<size_t>(count));
        for (int i = 0; i < count; i++) {
            pts.push_back(ax::Vec2(points[i].x, points[i].y));
        }
        if (_fillActive && count >= 3) {
            _draw->drawSolidPoly(pts.data(), static_cast<unsigned int>(count), _fillColor);
        } else if (_hasLine) {
            _draw->drawPoly(pts.data(), static_cast<unsigned int>(count), true, _lineColor);
        }
    }

private:
    ax::DrawNode* _draw = nullptr;
    std::vector<ax::Vec2> _path;
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
        _sprite = ax::Sprite::create();
        _node = _sprite;
    }
    explicit AxSprite(const std::string& frame) {
        _sprite = ax::Sprite::create();
        _node = _sprite;
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
        _label = ax::Label::createWithSystemFont("", "Arial", 24);
        _node = _label;
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
    AxContainer() { _node = ax::Node::create(); }

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
    explicit AxRenderTexture(int w, int h) { _rt = ax::RenderTexture::create(w, h); }
    ~AxRenderTexture() override {
        // ax::RenderTexture is ref-counted; nothing to do.
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
