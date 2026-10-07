#include "Ui.h"

#include <cctype>
#include <cmath>
#include <cstdint>
#include <vector>

USING_NS_AX;

namespace ui {

namespace {
inline float clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
} // namespace

// --- colours ---------------------------------------------------------------
namespace {

int hexVal(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool parseCss(const std::string& css, float& r, float& g, float& b, float& a) {
    std::string s = css;
    // Trim whitespace.
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
    if (s.rfind("rgb(", 0) == 0 || s.rfind("rgba(", 0) == 0) {
        const bool hasAlpha = s[3] == 'a';
        const std::size_t start = hasAlpha ? 5 : 4;
        const std::size_t end = s.find(')');
        const std::string inner = s.substr(start, end == std::string::npos ? std::string::npos : end - start);
        std::vector<float> parts;
        std::string cur;
        for (char c : inner + ",") {
            if (c == ',') {
                if (!cur.empty()) {
                    parts.push_back(std::stof(cur));
                    cur.clear();
                }
            } else {
                cur.push_back(c);
            }
        }
        if (parts.size() < 3) {
            return false;
        }
        r = parts[0] / 255.0f;
        g = parts[1] / 255.0f;
        b = parts[2] / 255.0f;
        a = parts.size() > 3 ? parts[3] : 1.0f;
        return true;
    }
    if (s.empty() || s[0] != '#') {
        return false;
    }
    s.erase(s.begin());
    if (s.size() == 3 || s.size() == 4) {
        std::string expanded;
        for (char c : s) {
            expanded.push_back(c);
            expanded.push_back(c);
        }
        s = expanded;
    }
    if (s.size() != 6 && s.size() != 8) {
        return false;
    }
    const int rr = hexVal(s[0]) * 16 + hexVal(s[1]);
    const int gg = hexVal(s[2]) * 16 + hexVal(s[3]);
    const int bb = hexVal(s[4]) * 16 + hexVal(s[5]);
    const int aa = s.size() == 8 ? hexVal(s[6]) * 16 + hexVal(s[7]) : 255;
    if (rr < 0 || gg < 0 || bb < 0 || aa < 0) {
        return false;
    }
    r = rr / 255.0f;
    g = gg / 255.0f;
    b = bb / 255.0f;
    a = aa / 255.0f;
    return true;
}

} // namespace

ax::Color3B parseHexColor(const std::string& css, const ax::Color3B& fallback) {
    float r = 0, g = 0, b = 0, a = 1;
    if (!parseCss(css, r, g, b, a)) {
        return fallback;
    }
    return ax::Color3B(static_cast<uint8_t>(r * 255.0f + 0.5f), static_cast<uint8_t>(g * 255.0f + 0.5f),
                       static_cast<uint8_t>(b * 255.0f + 0.5f));
}

ax::Color4B parseHexColorA(const std::string& css, const ax::Color4B& fallback) {
    float r = 0, g = 0, b = 0, a = 1;
    if (!parseCss(css, r, g, b, a)) {
        return fallback;
    }
    return ax::Color4B(static_cast<uint8_t>(r * 255.0f + 0.5f), static_cast<uint8_t>(g * 255.0f + 0.5f),
                       static_cast<uint8_t>(b * 255.0f + 0.5f), static_cast<uint8_t>(a * 255.0f + 0.5f));
}

// --- Panel -----------------------------------------------------------------
Panel* Panel::create(float width, float height) {
    auto* p = new Panel();
    if (p && p->init(width, height)) {
        p->autorelease();
        return p;
    }
    AX_SAFE_DELETE(p);
    return nullptr;
}

bool Panel::init(float width, float height) {
    if (!ax::Node::init()) {
        return false;
    }
    setContentSize(ax::Size(width, height));
    setAnchorPoint(ax::Vec2(0.0f, 1.0f));
    _background = ax::LayerColor::create(ax::Color4B(0, 0, 0, 0), width, height);
    _background->setAnchorPoint(ax::Vec2(0.0f, 1.0f));
    _background->setPosition(ax::Vec2(0.0f, height));
    _background->setIgnoreAnchorPointForPosition(false);
    addChild(_background, -1);
    return true;
}

void Panel::setBackgroundColor(const ax::Color3B& color, float alpha) {
    _background->setColor(color);
    _background->setOpacity(static_cast<uint8_t>(clamp01(alpha) * 255.0f + 0.5f));
}

void Panel::setCssPosition(float x, float y) {
    // Panel anchor is top-left; axmol wants the node's bottom-left.
    ax::Node::setPosition(ax::Vec2(x, kit::fromCssY(y + getContentSize().height)));
}

void Panel::setInteractive(bool interactive) {
    if (_interactive == interactive) {
        return;
    }
    _interactive = interactive;
    if (interactive) {
        swallowTouches(this);
    } else {
        getEventDispatcher()->removeEventListenersForTarget(this);
    }
}

// --- Button ----------------------------------------------------------------
Button* Button::create(const std::string& label, float width, float height, const std::string& bgFrame) {
    auto* b = new Button();
    if (b && b->init(label, width, height, bgFrame)) {
        b->autorelease();
        return b;
    }
    AX_SAFE_DELETE(b);
    return nullptr;
}

bool Button::init(const std::string& label, float width, float height, const std::string& bgFrame) {
    if (!ax::Node::init()) {
        return false;
    }
    setContentSize(ax::Size(width, height));
    setAnchorPoint(ax::Vec2(0.0f, 1.0f));

    _background = ax::ui::Scale9Sprite::create();
    _background->setContentSize(ax::Size(width, height));
    _background->setAnchorPoint(ax::Vec2(0.0f, 1.0f));
    _background->setPosition(ax::Vec2(0.0f, height));
    _background->setColor(ax::Color3B(40, 40, 40));
    _background->setOpacity(200);
    addChild(_background, 0);

    _label = label;
    _title = ax::Label::createWithSystemFont(label, "sans-serif", _fontSize);
    _title->setAnchorPoint(ax::Vec2(0.5f, 0.5f));
    _title->setPosition(ax::Vec2(width * 0.5f, height * 0.5f));
    _title->setTextColor(ax::Color4B::WHITE);
    addChild(_title, 1);

    if (!bgFrame.empty() && ax::SpriteFrameCache::getInstance()->getSpriteFrameByName(bgFrame)) {
        _background->setSpriteFrame(bgFrame);
        _background->setColor(ax::Color3B::WHITE);
        _background->setOpacity(255);
        _background->setContentSize(ax::Size(width, height));
    }

    // Tap handling: the whole button node swallows touches and fires onClick.
    _handler = [this](ax::Object*) {
        if (_enabled && onClick) {
            onClick();
        }
    };
    auto touch = ax::EventListenerTouchOneByOne::create();
    touch->setSwallowTouches(true);
    touch->onTouchBegan = [](ax::Touch*, ax::Event*) { return true; };
    touch->onTouchEnded = [this](ax::Touch* t, ax::Event*) {
        if (!_enabled || !onClick) {
            return;
        }
        const ax::Vec2 p = convertToNodeSpace(t->getLocation());
        const ax::Size size = getContentSize();
        if (p.x >= 0.0f && p.x <= size.width && p.y >= 0.0f && p.y <= size.height) {
            onClick();
        }
    };
    getEventDispatcher()->addEventListenerWithSceneGraphPriority(touch, this);
    return true;
}

void Button::setLabel(const std::string& text) {
    _label = text;
    if (_title) {
        _title->setString(text);
    }
}

void Button::setEnabled(bool enabled) {
    _enabled = enabled;
    if (_title) {
        _title->setColor(enabled ? ax::Color3B::WHITE : ax::Color3B(150, 150, 150));
    }
    if (_background) {
        _background->setColor(enabled ? ax::Color3B(40, 40, 40) : ax::Color3B(28, 28, 28));
    }
}

void Button::setFontSize(float size) {
    _fontSize = size;
    if (_title) {
        _title->setSystemFontSize(size);
    }
}

void Button::setTitleColor(const ax::Color3B& color) {
    if (_title) {
        _title->setColor(color);
    }
}

void Button::useTranslucentBackground(float alpha) {
    if (_background) {
        _background->setColor(ax::Color3B(20, 20, 20));
        _background->setOpacity(static_cast<uint8_t>(clamp01(alpha) * 255.0f));
    }
}

void Button::showBadge(bool show) {
    if (show && !_badge) {
        _badge = ax::DrawNode::create();
        _badge->drawSolidCircle(ax::Vec2(0, 0), 5.0f, 0.0f, 16, ax::Color4F(1.0f, 0.3f, 0.3f, 1.0f));
        _badge->setPosition(ax::Vec2(getContentSize().width - 6.0f, getContentSize().height - 6.0f));
        addChild(_badge, 3);
    } else if (!show && _badge) {
        _badge->setVisible(false);
    } else if (show && _badge) {
        _badge->setVisible(true);
    }
}

// --- TextField -------------------------------------------------------------
TextField* TextField::create(const std::string& placeholder, float width, float height) {
    auto* t = new TextField();
    if (t && t->init(placeholder, width, height)) {
        t->autorelease();
        return t;
    }
    AX_SAFE_DELETE(t);
    return nullptr;
}

bool TextField::init(const std::string& placeholder, float width, float height) {
    if (!ax::Node::init()) {
        return false;
    }
    setContentSize(ax::Size(width, height));
    setAnchorPoint(ax::Vec2(0.0f, 1.0f));

    _background = ax::LayerColor::create(ax::Color4B(20, 20, 20, 230), width, height);
    _background->setAnchorPoint(ax::Vec2(0.0f, 1.0f));
    _background->setPosition(ax::Vec2(0.0f, height));
    _background->setIgnoreAnchorPointForPosition(false);
    addChild(_background, 0);

    _input = ax::ui::TextField::create(placeholder, "sans-serif", 18);
    _input->setContentSize(ax::Size(width - 16.0f, height));
    _input->setPosition(ax::Vec2(width * 0.5f, height * 0.5f));
    _input->setMaxLengthEnabled(true);
    _input->setMaxLength(16);
    _input->addEventListener([this](ax::Object*, ax::ui::TextField::EventType) {
        if (_onChanged) {
            _onChanged(getText());
        }
    });
    addChild(_input, 1);
    return true;
}

std::string TextField::getText() const {
    return _input ? std::string(_input->getString()) : std::string();
}

void TextField::setText(const std::string& text) {
    if (_input) {
        _input->setString(text);
    }
}

// --- Slider ----------------------------------------------------------------
Slider* Slider::create(float width, float value) {
    auto* s = new Slider();
    if (s && s->init(width, value)) {
        s->autorelease();
        return s;
    }
    AX_SAFE_DELETE(s);
    return nullptr;
}

bool Slider::init(float width, float value) {
    if (!ax::Node::init()) {
        return false;
    }
    constexpr float kHeight = 24.0f;
    setContentSize(ax::Size(width, kHeight));
    setAnchorPoint(ax::Vec2(0.0f, 1.0f));
    _trackWidth = width - 16.0f;

    _track = ax::DrawNode::create();
    _track->drawSolidRect(ax::Vec2(8.0f, 10.0f), ax::Vec2(width - 8.0f, 14.0f), ax::Color4F(0, 0, 0, 0.55f));
    addChild(_track, 0);
    _fill = ax::DrawNode::create();
    addChild(_fill, 1);
    _knob = ax::DrawNode::create();
    _knob->drawSolidCircle(ax::Vec2(0, 0), 8.0f, 0.0f, 20, ax::Color4F(0.9f, 0.9f, 0.9f, 1.0f));
    addChild(_knob, 2);

    _value = value;
    layoutKnob();

    auto listener = ax::EventListenerTouchOneByOne::create();
    listener->setSwallowTouches(true);
    listener->onTouchBegan = AX_CALLBACK_2(Slider::onTouchBegan, this);
    listener->onTouchMoved = AX_CALLBACK_2(Slider::onTouchMoved, this);
    listener->onTouchEnded = AX_CALLBACK_2(Slider::onTouchEnded, this);
    getEventDispatcher()->addEventListenerWithSceneGraphPriority(listener, this);
    return true;
}

void Slider::layoutKnob() {
    const float x = 8.0f + _trackWidth * _value;
    if (_knob) {
        _knob->setPosition(ax::Vec2(x, 12.0f));
    }
    if (_fill) {
        _fill->clear();
        _fill->drawSolidRect(ax::Vec2(8.0f, 10.0f), ax::Vec2(x, 14.0f), ax::Color4F(0.35f, 0.8f, 0.35f, 1.0f));
    }
}

void Slider::setValue(float value, bool fire) {
    _value = clamp01(value);
    layoutKnob();
    if (fire && onChanged) {
        onChanged(_value);
    }
}

bool Slider::onTouchBegan(ax::Touch* touch, ax::Event*) {
    // Convert to node space using the world->local transform.
    const ax::Vec2 p = convertToNodeSpace(touch->getLocation());
    const float t = (p.x - 8.0f) / (_trackWidth > 0.0f ? _trackWidth : 1.0f);
    setValue(t, true);
    return true;
}

void Slider::onTouchMoved(ax::Touch* touch, ax::Event*) {
    const ax::Vec2 p = convertToNodeSpace(touch->getLocation());
    const float t = (p.x - 8.0f) / (_trackWidth > 0.0f ? _trackWidth : 1.0f);
    setValue(t, true);
}

void Slider::onTouchEnded(ax::Touch*, ax::Event*) {}

// --- Modal -----------------------------------------------------------------
Modal::~Modal() {
    if (_overlay) {
        _overlay->removeFromParent();
        _overlay = nullptr;
    }
}

Panel* Modal::build(float width, float height, const std::string& title) {
    _width = width;
    _height = height;
    _overlay = ax::LayerColor::create(ax::Color4B(0, 0, 0, 170), kit::designWidth(), kit::designHeight());
    _overlay->setAnchorPoint(ax::Vec2(0.0f, 0.0f));
    _overlay->setPosition(ax::Vec2::ZERO);
    _overlay->setIgnoreAnchorPointForPosition(false);
    _overlay->setVisible(false);
    swallowTouches(_overlay);

    _panel = Panel::create(width, height);
    _panel->setBackgroundColor(ax::Color3B(60, 60, 60), 0.98f);
    _overlay->addChild(_panel);
    _panel->placeCenter();

    if (!title.empty()) {
        auto* header = ax::Label::createWithSystemFont(title, "sans-serif", 24);
        header->setAnchorPoint(ax::Vec2(0.5f, 1.0f));
        header->setPosition(ax::Vec2(width * 0.5f, height - 10.0f));
        _panel->addChild(header, 5);
    }
    addCloseButton();

    _root = nullptr;
    return _panel;
}

void Modal::addCloseButton() {
    auto* close = Button::create("X", 34.0f, 30.0f);
    close->setCssPosition(_width - 40.0f, 8.0f);
    close->onClick = [this] { close(); };
    _panel->addChild(static_cast<ax::Node*>(close), 10);
}

void Modal::refreshSize() {
    if (_panel) {
        _panel->setContentSize(ax::Size(_width, _height));
        _panel->placeCenter();
    }
}

void Modal::show() {
    if (!_overlay) {
        return;
    }
    _overlay->setVisible(true);
    _visible = true;
    if (onShow) {
        onShow();
    }
}

void Modal::hide() {
    if (!_overlay) {
        return;
    }
    _overlay->setVisible(false);
    _visible = false;
    if (onHide) {
        onHide();
    }
}

// --- shared scene plumbing -------------------------------------------------
ax::Node* uiRoot(ax::Scene* scene, int zOrder) {
    auto* root = ax::Node::create();
    root->setAnchorPoint(ax::Vec2(0.0f, 0.0f));
    root->setPosition(ax::Vec2::ZERO);
    root->setContentSize(ax::Size(kit::designWidth(), kit::designHeight()));
    scene->addChild(root, zOrder);
    return root;
}

void layoutUiRoot(ax::Node* root, const ax::Vec2& visibleSize, const ax::Vec2& designSize) {
    if (!root) {
        return;
    }
    // Fit the design rect into the visible area (like the web client's
    // full-viewport CSS with fixed aspect content).
    const float scale = std::fmin(visibleSize.x / designSize.x, visibleSize.y / designSize.y);
    root->setScale(scale);
    root->setPosition(ax::Vec2((visibleSize.x - designSize.x * scale) * 0.5f,
                               (visibleSize.y - designSize.y * scale) * 0.5f));
}

void swallowTouches(ax::Node* node) {
    if (!node) {
        return;
    }
    auto listener = ax::EventListenerTouchOneByOne::create();
    listener->setSwallowTouches(true);
    listener->onTouchBegan = [](ax::Touch*, ax::Event*) { return true; };
    node->getEventDispatcher()->addEventListenerWithSceneGraphPriority(listener, node);
}

} // namespace ui
