#pragma once
// Headless recording implementation of the PixiLike adapter, used by the host
// tests. It mirrors the transform/z-order bookkeeping of the axmol adapter and
// records draw commands so renderer/map output can be asserted without a GPU.
#include "PixiLike.h"
#include "FillGeometry.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace pix {

struct DrawCommand {
    enum Kind { BeginFill, EndFill, BeginHole, EndHole, MoveTo, LineTo, ClosePath, Rect, Circle, Dot, Polygon, LineStyle };
    Kind kind;
    float a = 0.0f, b = 0.0f, c = 0.0f, d = 0.0f;
    uint color = 0;
    float alpha = 1.0f;
};

template <class Iface>
class NullNodeImpl : public Iface {
public:
    float x = 0.0f, y = 0.0f;
    float sx = 1.0f, sy = 1.0f;
    float rotation = 0.0f;
    float ax = 0.0f, ay = 0.0f;
    float alpha = 1.0f;
    bool visible = true;
    uint tint = 0xffffff;
    BlendMode blend = BlendMode::Normal;
    int localZ = 0;
    int sortOrd = 0;
    int sortIdx = 0;
    Node* parentNode = nullptr;
    std::vector<Node*> children;
    std::vector<DrawCommand> commands;

    void setPosition(float px, float py) override { x = px; y = py; }
    void setScale(float nsx, float nsy) override { sx = nsx; sy = nsy; }
    void setRotation(float r) override { rotation = r; }
    void setAnchor(float nax, float nay) override { ax = nax; ay = nay; }
    void setAlpha(float a) override { alpha = a; }
    void setVisible(bool v) override { visible = v; }
    void setTint(uint t) override { tint = t; }
    void setBlendMode(BlendMode m) override { blend = m; }
    void setLocalZOrder(int z) override { localZ = z; }
    void setSortKey(int ord, int idx) override { sortOrd = ord; sortIdx = idx; }
    int getSortOrd() const override { return sortOrd; }
    int getSortIdx() const override { return sortIdx; }

    void addChild(Node* child) override {
        if (!child) {
            return;
        }
        detachForReparent(child);
        child->setParent(this);
        children.push_back(child);
        reindex();
    }
    void addChildAt(Node* child, int index) override {
        if (!child) {
            return;
        }
        detachForReparent(child);
        child->setParent(this);
        if (index < 0 || index > static_cast<int>(children.size())) {
            index = static_cast<int>(children.size());
        }
        children.insert(children.begin() + index, child);
        reindex();
    }
    void removeChild(Node* child) override {
        for (auto it = children.begin(); it != children.end(); ++it) {
            if (*it == child) {
                children.erase(it);
                break;
            }
        }
        if (child->getParent() == this) {
            child->setParent(nullptr);
        }
    }
    void removeFromParent() override {
        if (parentNode) {
            parentNode->removeChild(this);
        }
    }
    Node* getParent() const override { return parentNode; }
    void setParent(Node* parent) override { parentNode = parent; }
    int getChildIndex(Node* child) const override {
        for (size_t i = 0; i < children.size(); i++) {
            if (children[i] == child) {
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
        children.erase(children.begin() + cur);
        if (index < 0 || index > static_cast<int>(children.size())) {
            index = static_cast<int>(children.size());
        }
        children.insert(children.begin() + index, child);
        reindex();
    }
    void setMask(Node* mask, bool inverted) override {
        maskNode = mask;
        maskInverted = inverted;
    }
    void clearMask() override {
        maskNode = nullptr;
        maskInverted = false;
    }
    void* native() override { return this; }

    Node* maskNode = nullptr;
    bool maskInverted = false;

protected:
    // Mirror PIXI's Container.addChild reparenting (see AxmolPixi.h).
    void detachForReparent(Node* child) {
        Node* parent = child->getParent();
        if (parent == this) {
            for (auto it = children.begin(); it != children.end(); ++it) {
                if (*it == child) {
                    children.erase(it);
                    break;
                }
            }
        } else if (parent) {
            parent->removeChild(child);
        }
    }

    void reindex() {
        for (size_t i = 0; i < children.size(); i++) {
            children[i]->setLocalZOrder(static_cast<int>(i));
        }
    }
};

class NullGraphics : public NullNodeImpl<Graphics> {
public:
    std::vector<FillTriangle> fillTriangles;
    void clear() override { commands.clear(); fillTriangles.clear(); geometry.clear(); filling = false; line = false; }
    void beginFill(uint color, float a) override {
        if (filling) endFill();
        geometry.clear();
        filling = true;
        commands.push_back({DrawCommand::BeginFill, 0, 0, 0, 0, color, a});
    }
    void endFill() override {
        if (filling) {
            auto triangles = geometry.triangles();
            fillTriangles.insert(fillTriangles.end(), triangles.begin(), triangles.end());
        }
        filling = false;
        commands.push_back({DrawCommand::EndFill});
    }
    void beginHole() override { geometry.beginHole(); commands.push_back({DrawCommand::BeginHole}); }
    void endHole() override { geometry.endHole(); commands.push_back({DrawCommand::EndHole}); }
    void lineStyle(float w, uint color, float a) override {
        commands.push_back({DrawCommand::LineStyle, w, 0, 0, 0, color, a});
        line = a > 0.0f;
    }
    void moveTo(float mx, float my) override {
        commands.push_back({DrawCommand::MoveTo, mx, my});
        if (filling && !line) geometry.moveTo(mx, my);
    }
    void lineTo(float lx, float ly) override {
        commands.push_back({DrawCommand::LineTo, lx, ly});
        if (filling && !line) geometry.lineTo(lx, ly);
    }
    void closePath() override { if (filling && !line) geometry.closePath(); commands.push_back({DrawCommand::ClosePath}); }
    void drawRect(float rx, float ry, float w, float h) override {
        commands.push_back({DrawCommand::Rect, rx, ry, w, h});
        if (filling && !line) geometry.rect(rx, ry, w, h);
    }
    void drawCircle(float cx, float cy, float r) override {
        commands.push_back({DrawCommand::Circle, cx, cy, r});
        if (filling && !line) geometry.circle(cx, cy, r);
    }
    void drawDot(float dx, float dy, float r) override {
        commands.push_back({DrawCommand::Dot, dx, dy, r});
        if (filling) geometry.circle(dx, dy, r);
    }
    void drawPolygon(const surv::Vec2* pts, int count) override {
        if (filling && !line) geometry.polygon(pts, count);
        for (int i = 0; i < count; i++) {
            commands.push_back({DrawCommand::Polygon, pts[i].x, pts[i].y});
        }
    }
private:
    FillGeometry geometry;
    bool filling = false, line = false;
};

class NullSprite : public NullNodeImpl<Sprite> {
public:
    std::string frame;
    void setFrame(const std::string& f) override { frame = f; }
    float sourceWidth() const override { return 1.0f; }
    float sourceHeight() const override { return 1.0f; }
};

class NullText : public NullNodeImpl<Text> {
public:
    std::string text;
    float fontSize = 24.0f;
    std::string family;
    void setText(const std::string& t) override { text = t; }
    void setFontSize(float s) override { fontSize = s; }
    void setFontFamily(const std::string& f) override { family = f; }
    void setColor(uint, uint, float, bool) override {}
};

class NullContainer : public NullNodeImpl<Container> {
public:
    void sortChildren() override {
        std::stable_sort(children.begin(), children.end(), [](Node* a, Node* b) {
            if (a->getSortOrd() != b->getSortOrd()) {
                return a->getSortOrd() < b->getSortOrd();
            }
            return a->getSortIdx() < b->getSortIdx();
        });
        reindex();
    }
    std::size_t childCount() const override { return children.size(); }
};

class NullRenderTexture : public RenderTexture {
public:
    float width = 1.0f, height = 1.0f;
    void resize(float w, float h) override {
        width = w;
        height = h;
    }
};

class NullRenderer : public Renderer {
public:
    int renderCount = 0;
    void render(Node*, RenderTexture*, bool) override { renderCount++; }
};

class NullPixiFactory : public Factory {
public:
    Graphics* createGraphics() override { return keep(new NullGraphics()); }
    Sprite* createSprite() override { return keep(new NullSprite()); }
    Sprite* createSprite(const std::string& f) override {
        auto* s = new NullSprite();
        s->frame = f;
        return keep(s);
    }
    Text* createText() override { return keep(new NullText()); }
    Container* createContainer() override { return keep(new NullContainer()); }
    RenderTexture* createRenderTexture() override {
        auto* rt = new NullRenderTexture();
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
