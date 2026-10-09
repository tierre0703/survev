#include "Ui.h"
#include "Files.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

USING_NS_AX;

namespace ui {

namespace {
// Web `.btn-*:active` uses `transition: all 0.25s ease` on the brightness
// filter; the native button tweens its brightness over the same duration.
constexpr float kBrightnessAnimSeconds = 0.25f;
inline float clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
float parentHeight(ax::Node* node) {
    return node->getParent() ? node->getParent()->getContentSize().height : kit::designHeight();
}

// `touch->getLocation()` is in the render view's design (point) space, i.e. the
// 1280x720 canvas, while node positions live under the scaled UI root. Map the
// point down the parent chain with each ancestor's local transform (position +
// scale); axmol's `convertToNodeSpace` assumes an identity-rotation world matrix
// and mishandles the UI root's scale.
ax::Vec2 localPointFromDesign(const ax::Vec2& designPoint, ax::Node* node) {
    ax::Vec2 p = designPoint;
    for (auto* n = node ? node->getParent() : nullptr; n; n = n->getParent()) {
        const float scaleX = n->getScaleX();
        const float scaleY = n->getScaleY();
        if (scaleX == 0.0f || scaleY == 0.0f) {
            return ax::Vec2(-1.0f, -1.0f);
        }
        p = (p - n->getPosition()) / ax::Vec2(scaleX, scaleY);
    }
    return p;
}

bool hit(ax::Node* node, ax::Touch* touch) {
    for (auto* n = node; n; n = n->getParent()) {
        if (!n->isVisible()) return false;
    }
    const auto p = localPointFromDesign(touch->getLocation(), node);
    const auto size = node->getContentSize();
    const auto anchor = node->getAnchorPoint();
    // Node space is anchored: with a top-left anchor (0,1) the box spans
    // x∈[0,w], y∈[-h,0]; handle both so callers can use either anchor.
    const float minX = -anchor.x * size.width;
    const float minY = -anchor.y * size.height;
    return p.x >= minX && p.x <= minX + size.width && p.y >= minY && p.y <= minY + size.height;
}
} // namespace

// --- colours ---------------------------------------------------------------
// The parser itself lives in Layout.cpp (engine-free) so the host tests can
// exercise it; these wrappers convert the parsed bytes to axmol colours.

ax::Color3B parseHexColor(const std::string& css, const ax::Color3B& fallback) {
    const Rgb rgb = parseHexColorRgb(css, Rgb{fallback.r, fallback.g, fallback.b});
    return ax::Color3B(rgb.r, rgb.g, rgb.b);
}

ax::Color4B parseHexColorA(const std::string& css, const ax::Color4B& fallback) {
    const Rgba rgba = parseHexColorRgba(css, Rgba{fallback.r, fallback.g, fallback.b, fallback.a});
    return ax::Color4B(rgba.r, rgba.g, rgba.b, rgba.a);
}

// --- Typography ------------------------------------------------------------
namespace {
// The UI font is the web family; the effective face is resolved per weight via
// `Files::fontFace` ("" -> system font).
constexpr const char* kGuiFamily = "RobotoCondensed";
} // namespace

std::string fontPath(const std::string& family, bool bold) {
    return Files::fontFace(family.empty() ? kGuiFamily : family, bold);
}

namespace {
// Font size, in points, that maps to a label's face size of `size` under the
// project's content scale (`Content/` is authored at half the 1280x720 design
// space, so a 16px design label is rendered from an 8pt face). axmol otherwise
// substitutes a fixed ~12pt TTF face and silently ignores the requested size.
float ttfFacePoints(float size) {
    return size * ax::Director::getInstance()->getContentScaleFactor();
}
} // namespace

ax::Label* makeLabel(const std::string& text, float size, bool bold, bool bulk) {
    // The web client renders UI text in Roboto Condensed. Prefer the bundled TTF
    // so the native UI matches the web glyphs; `bulk` (tight grids) keeps the
    // system font, whose per-glyph metrics are uniform enough to avoid overflow.
    if (!bulk) {
        const std::string face = fontPath(kGuiFamily, bold);
        if (!face.empty() && ax::FileUtils::getInstance()->isFileExist(face)) {
            // axmol substitutes a fixed ~12pt face whenever `createWithTTF` is
            // given the (float) point size, so route through `createWithTTF`'s
            // `FontDefinition` to get an integer point size, then scale the
            // label by the content factor so it rasterises at the right size.
            // (This is what `axmol::ui::RichText`/`Label::createWithTTF` do
            // internally for their own content-scale handling.)
            ax::FontDefinition def;
            def._fontName = face;
            def._fontSize = static_cast<int>(size + 0.5f);
            if (auto* label = ax::Label::createWithTTF(def, text)) {
                const float scale = ax::Director::getInstance()->getContentScaleFactor();
                if (scale > 0.0f) {
                    label->setScale(scale);
                }
                return label;
            }
        }
    }
    return ax::Label::createWithSystemFont(text, bold ? "sans-serif-bold" : "sans-serif", size);
}

void setLabelSize(ax::Label* label, float size) {
    if (!label) {
        return;
    }
    // TTF labels expose the face size (which may be SDF-scaled); system-font
    // labels keep the point size in the system-font field. Handle both so a
    // resize never corrupts a label created by `makeLabel`.
    if (label->getLabelType() == ax::Label::LabelType::TTF) {
        label->setTTFFaceSize(static_cast<int>(ttfFacePoints(size) + 0.5f));
        label->setScale(1.0f);
    } else {
        label->setSystemFontSize(size);
    }
}

void setLabelBold(ax::Label* label, bool bold, bool bulk) {
    if (!label) {
        return;
    }
    if (label->getLabelType() != ax::Label::LabelType::TTF) {
        label->setSystemFontName(bold ? "sans-serif-bold" : "sans-serif");
        return;
    }
    // Rebuild the TTF label with the other weight at the same position/style.
    const float facePoints = static_cast<float>(label->getTTFFaceSize());
    const float size = facePoints / ax::Director::getInstance()->getContentScaleFactor();
    auto* replacement = makeLabel(std::string(label->getString()), size, bold, bulk);
    if (!replacement || replacement == label) {
        return;
    }
    replacement->setAnchorPoint(label->getAnchorPoint());
    replacement->setPosition(label->getPosition());
    replacement->setTextColor(label->getTextColor());
    replacement->setLocalZOrder(label->getLocalZOrder());
    auto* parent = label->getParent();
    if (!parent) {
        replacement->release();
        return;
    }
    parent->addChild(replacement);
    label->removeFromParent();
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
    _cssPosition = ax::Vec2(x, y);
    _hasCssPosition = true;
    // Panel anchor is top-left; axmol wants the node's bottom-left.
    ax::Node::setPosition(ax::Vec2(x, kit::fromCssY(y, parentHeight(this))));
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

Button::~Button() {
    delete _brightness;
    _brightness = nullptr;
}

bool Button::init(const std::string& label, float width, float height, const std::string& bgFrame) {
    if (!ax::Node::init()) {
        return false;
    }
    _brightness = new float(1.0f);
    setContentSize(ax::Size(width, height));
    setAnchorPoint(ax::Vec2(0.0f, 1.0f));

    // CSS `.btn-*` body: filled background plus a 2px bottom border (the web
    // `border-bottom` + inset box-shadow). The border is drawn as two rects so
    // the text stays vertically centered like `line-height: 36px`.
    constexpr float kBorder = 2.0f;
    _fallbackBackground = ax::LayerColor::create(ax::Color4B(122, 122, 122, 255), width, height - kBorder);
    _fallbackBackground->setAnchorPoint(ax::Vec2(0.0f, 1.0f));
    _fallbackBackground->setPosition(ax::Vec2(0.0f, height - kBorder));
    _fallbackBackground->setIgnoreAnchorPointForPosition(false);
    addChild(_fallbackBackground, -2);
    auto* border = ax::LayerColor::create(ax::Color4B(62, 62, 62, 255), width, kBorder);
    border->setAnchorPoint(ax::Vec2(0.0f, 1.0f));
    border->setPosition(ax::Vec2(0.0f, kBorder));
    border->setIgnoreAnchorPointForPosition(false);
    border->setTag(0xB07D);
    addChild(border, -3);

    _background = ax::ui::Scale9Sprite::create();
    _background->setContentSize(ax::Size(width, height));
    _background->setAnchorPoint(ax::Vec2(0.0f, 1.0f));
    _background->setPosition(ax::Vec2(0.0f, height));
    _background->setVisible(false);
    addChild(_background, -1);

    _label = label;
    _title = makeLabel(label, _fontSize, false, _bulkFont);
    _title->setAnchorPoint(ax::Vec2(0.5f, 0.5f));
    _title->setPosition(ax::Vec2(width * 0.5f, height * 0.5f));
    _title->setTextColor(ax::Color4B::WHITE);
    addChild(_title, 1);

    applyBodyColor();

    if (!bgFrame.empty() && ax::SpriteFrameCache::getInstance()->getSpriteFrameByName(bgFrame)) {
        _background->setSpriteFrame(bgFrame);
        _background->setVisible(true);
        _background->setContentSize(ax::Size(width, height));
        _fallbackBackground->setVisible(false);
        if (auto* borderNode = getChildByTag(0xB07D)) borderNode->setVisible(false);
    }

    // Tap handling: the whole button node swallows touches and fires onClick.
    _handler = [this](ax::Object*) {
        if (_enabled && onClick) {
            onClick();
        }
    };
    auto touch = ax::EventListenerTouchOneByOne::create();
    touch->setSwallowTouches(true);
    touch->onTouchBegan = [this](ax::Touch* t, ax::Event*) {
        const bool began = _enabled && hit(this, t);
        if (began) {
            setPressed(true);
        }
        return began;
    };
    touch->onTouchMoved = [this](ax::Touch* t, ax::Event*) {
        // Web `.btn-darken:hover` / `:active`: dragging off cancels the press.
        if (_enabled && _pressed) {
            setPressed(hit(this, t));
        }
    };
    touch->onTouchEnded = [this](ax::Touch* t, ax::Event*) {
        const bool wasPressed = _pressed;
        setPressed(false);
        if (!_enabled || !wasPressed || !onClick) {
            return;
        }
        if (hit(this, t)) {
            onClick();
        }
    };
    touch->onTouchCancelled = [this](ax::Touch*, ax::Event*) { setPressed(false); };
    getEventDispatcher()->addEventListenerWithSceneGraphPriority(touch, this);
    scheduleUpdate();
    return true;
}

void Button::setPressed(bool pressed) {
    if (_pressed == pressed) {
        return;
    }
    _pressed = pressed;
    // Web `.btn-darken:hover, .btn-darken:active { filter: brightness(80%) }`
    // with a 0.25s ease. Fade the effective brightness so the native button
    // animates exactly like the CSS transition instead of popping.
    applyBodyColor();
    if (_brightness == nullptr) {
        _brightness = new float(1.0f);
    }
    const float target = pressed ? 0.8f : 1.0f;
    stopAllActions();
    auto* fade = ax::ActionTween::create(kBrightnessAnimSeconds, "btnBrightness", *_brightness, target);
    runAction(fade);
}

void Button::setToggle(bool on) {
    _toggleOn = on;
    // `.btn-hollow-selected`: 2px `#00ff00` border + `rgba(0,0,0,.35)` body.
    setHollowBorder(on);
}

void Button::update(float /*dt*/) {
    if (_brightness == nullptr || *_brightness == _lastBrightness) {
        return;
    }
    _lastBrightness = *_brightness;
    applyBodyColor();
}

void Button::setLabel(const std::string& text) {
    _label = text;
    if (_title) {
        _title->setString(text);
    }
    layoutLabel();
}
void Button::setIcon(const std::string& name) {
    if (_icon) {
        _icon->removeFromParent();
        _icon = nullptr;
    }
    if (name.empty()) {
        layoutLabel();
        return;
    }
    // `gui/<name>.png` is generated from public/img/gui/*.svg at 4x the web CSS
    // size; the sprite is scaled down to the design canvas.
    const std::string path = "gui/" + name + ".png";
    if (!ax::FileUtils::getInstance()->isFileExist(path)) {
        layoutLabel();
        return;
    }
    _icon = ax::Sprite::create(path);
    if (_icon) {
        addChild(_icon, 2);
        _icon->setColor(_enabled ? ax::Color3B::WHITE : ax::Color3B(150, 150, 150));
    }
    layoutLabel();
}

void Button::setIconScale(float scale) {
    if (_icon) {
        const auto size = _icon->getContentSize();
        const float base = std::fmax(size.width, size.height);
        _icon->setScale(base > 0.0f ? scale / base : 1.0f);
    }
}

void Button::setIconLayout(bool left) {
    _iconLeft = left;
    layoutLabel();
}

void Button::layoutLabel() {
    if (!_title) {
        return;
    }
    const ax::Size size = getContentSize();
    if (!_icon) {
        _title->setPosition(ax::Vec2(size.width * 0.5f, size.height * 0.5f));
        return;
    }
    const auto iconSize = _icon->getContentSize();
    const float iconW = iconSize.width * _icon->getScale();
    const float iconH = iconSize.height * _icon->getScale();
    if (_iconLeft) {
        // Icon on the left, label centered in the remaining space (web
        // `.btn-social` with a 44px icon and an 8px gap).
        constexpr float kGap = 8.0f;
        _icon->setPosition(ax::Vec2(6.0f + iconW * 0.5f, size.height * 0.5f));
        _title->setPosition(ax::Vec2(6.0f + iconW + kGap + (size.width - 6.0f - iconW - kGap) * 0.5f,
                                     size.height * 0.5f));
    } else {
        // Icon centered, label below it (icon-only buttons carry an empty
        // label, so this is mostly a fallback).
        _icon->setPosition(ax::Vec2(size.width * 0.5f, size.height * 0.65f));
        _title->setPosition(ax::Vec2(size.width * 0.5f, size.height * 0.25f - iconH * 0.5f));
    }
}

void Button::applyBodyColor() {
    const float brightness = _brightness ? *_brightness : (_pressed ? 0.8f : 1.0f);
    auto dim = [brightness](const ax::Color3B& c) {
        // CSS `filter: brightness(N)`.
        return ax::Color3B(static_cast<uint8_t>(c.r * brightness + 0.5f),
                           static_cast<uint8_t>(c.g * brightness + 0.5f),
                           static_cast<uint8_t>(c.b * brightness + 0.5f));
    };
    const ax::Color3B body = _enabled ? _bodyColor : ax::Color3B(56, 56, 56);
    if (_fallbackBackground) {
        _fallbackBackground->setColor(dim(body));
        // `.btn-hollow` body is transparent; `.btn-hollow-selected` is
        // `rgba(0,0,0,.35)`. The plain `.btn-darken`/`.btn-green` are opaque.
        if (_hollowBorder) {
            _fallbackBackground->setOpacity(_hollowSelected ? 90 : 0);
        } else {
            _fallbackBackground->setOpacity(_hollow ? 90 : 255);
        }
    }
    if (auto* borderNode = getChildByTag(0xB07D)) {
        const ax::Color3B shadow = _enabled ? _shadowColor : ax::Color3B(40, 40, 40);
        borderNode->setColor(dim(shadow));
        borderNode->setOpacity(_hollow ? 200 : 255);
    }
    for (auto* child : getChildren()) {
        if (child->getTag() == 0xB07E) {
            const ax::Color3B accent = _enabled ? _shadowColor : ax::Color3B(120, 120, 120);
            child->setColor(dim(accent));
        }
    }
    if (_background && _background->isVisible()) {
        _background->setColor(_enabled ? ax::Color3B::WHITE : ax::Color3B(150, 150, 150));
    }
    if (_title) {
        // Web `.btn-*:active` dims the whole element (text and all), so the
        // label follows the same brightness curve.
        const uint8_t v = static_cast<uint8_t>(255.0f * brightness + 0.5f);
        _title->setColor(_enabled ? ax::Color3B(v, v, v) : ax::Color3B(180, 180, 180));
    }
    if (_icon) {
        const uint8_t v = static_cast<uint8_t>(255.0f * brightness + 0.5f);
        _icon->setColor(_enabled ? ax::Color3B(v, v, v) : ax::Color3B(150, 150, 150));
    }
}

void Button::setColors(const ax::Color3B& body, const ax::Color3B& shadow) {
    _bodyColor = body;
    _shadowColor = shadow;
    applyBodyColor();
}

void Button::setHollow(bool hollow, bool selected) {
    _hollow = hollow;
    _bodyColor = hollow ? ax::Color3B(0, 0, 0) : _bodyColor;
    if (selected) {
        _shadowColor = ax::Color3B(0, 255, 0);
        _bodyColor = ax::Color3B(0, 0, 0);
    }
    applyBodyColor();
}

void Button::setHollowBorder(bool selected) {
    // `.btn-hollow`: transparent body, 2px white (or green when selected) border
    // on all four sides, and the web selected fill `rgba(0,0,0,.35)`.
    _hollow = true;
    _hollowBorder = true;
    _hollowSelected = selected;
    _bodyColor = ax::Color3B(0, 0, 0);
    const ax::Color3B accent = selected ? ax::Color3B(0, 255, 0) : ax::Color3B(255, 255, 255);
    const ax::Size size = getContentSize();
    // Replace any previously added hollow frame (setHollowBorder is re-entrant
    // when the selection changes).
    for (auto* child : getChildren()) {
        if (child->getTag() == 0xB07E) {
            child->removeFromParent();
        }
    }
    constexpr float kBorder = 2.0f;
    // Anchor is top-left (0,1); edges are laid out in local (y-up) space.
    const ax::Vec2 positions[4] = {
        ax::Vec2(0.0f, -kBorder),                       // bottom
        ax::Vec2(0.0f, 0.0f),                           // left
        ax::Vec2(0.0f, -size.height),                   // top
        ax::Vec2(size.width - kBorder, 0.0f),           // right
    };
    const ax::Size sizes[4] = {
        ax::Size(size.width, kBorder), ax::Size(kBorder, size.height), ax::Size(size.width, kBorder),
        ax::Size(kBorder, size.height),
    };
    for (int i = 0; i < 4; ++i) {
        auto* edge = ax::LayerColor::create(ax::Color4B(accent.r, accent.g, accent.b, 255), sizes[i].width,
                                            sizes[i].height);
        edge->setAnchorPoint(ax::Vec2(0.0f, 1.0f));
        edge->setPosition(positions[i]);
        edge->setIgnoreAnchorPointForPosition(false);
        edge->setTag(0xB07E);
        addChild(edge, -3);
    }
    applyBodyColor();
}

void Button::setBold(bool bold) {
    if (!_title) {
        return;
    }
    _labelBold = bold;
    auto* replacement = makeLabel(_label, _fontSize, bold, _bulkFont);
    if (!replacement || replacement == _title) {
        return;
    }
    replacement->setAnchorPoint(_title->getAnchorPoint());
    replacement->setPosition(_title->getPosition());
    replacement->setTextColor(_title->getTextColor());
    const int z = _title->getLocalZOrder();
    _title->removeFromParent();
    addChild(replacement, z);
    _title = replacement;
}

void Button::setBulkFont(bool bulk) {
    if (_bulkFont == bulk) {
        return;
    }
    _bulkFont = bulk;
    // Rebuild the label on the other font family, preserving layout/style.
    auto* replacement = makeLabel(_label, _fontSize, _labelBold, _bulkFont);
    if (!replacement || replacement == _title) {
        return;
    }
    replacement->setAnchorPoint(_title->getAnchorPoint());
    replacement->setPosition(_title->getPosition());
    replacement->setTextColor(_title->getTextColor());
    const int z = _title->getLocalZOrder();
    _title->removeFromParent();
    addChild(replacement, z);
    _title = replacement;
    layoutLabel();
}

void Button::setCssPosition(float x, float y) {
    _cssPosition = ax::Vec2(x, y);
    _hasCssPosition = true;
    setPosition(kit::fromCss(x, y, parentHeight(this)));
}

void TextField::setCssPosition(float x, float y) {
    _cssPosition = ax::Vec2(x, y);
    _hasCssPosition = true;
    setPosition(kit::fromCss(x, y, parentHeight(this)));
}

void Slider::setCssPosition(float x, float y) {
    _cssPosition = ax::Vec2(x, y);
    _hasCssPosition = true;
    setPosition(kit::fromCss(x, y, parentHeight(this)));
}

void Panel::onEnter() {
    ax::Node::onEnter();
    if (_hasCssPosition) setCssPosition(_cssPosition.x, _cssPosition.y);
}
void Button::onEnter() {
    ax::Node::onEnter();
    if (_hasCssPosition) setCssPosition(_cssPosition.x, _cssPosition.y);
}
void TextField::onEnter() {
    ax::Node::onEnter();
    if (_hasCssPosition) setCssPosition(_cssPosition.x, _cssPosition.y);
}
void Slider::onEnter() {
    ax::Node::onEnter();
    if (_hasCssPosition) setCssPosition(_cssPosition.x, _cssPosition.y);
}

void Button::setEnabled(bool enabled) {
    _enabled = enabled;
    applyBodyColor();
}

void Button::setFontSize(float size) {
    _fontSize = size;
    setLabelSize(_title, size);
}

void Button::setTitleColor(const ax::Color3B& color) {
    if (_title) {
        _title->setColor(color);
    }
}

void Button::useTranslucentBackground(float alpha) {
    _hollow = true;
    _bodyColor = ax::Color3B(0, 0, 0);
    applyBodyColor();
    if (_fallbackBackground) {
        _fallbackBackground->setOpacity(static_cast<uint8_t>(clamp01(alpha) * 255.0f + 0.5f));
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
    // axmol centers text in the field's content box, matching the web
    // `.player-name-input { text-align: center }`.
    _input->setTextHorizontalAlignment(ax::TextHAlignment::CENTER);
    _input->setTextVerticalAlignment(ax::TextVAlignment::CENTER);
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

void TextField::setBold(bool bold) {
    if (!_input) {
        return;
    }
    const std::string face = fontPath(kGuiFamily, bold);
    if (!face.empty() && ax::FileUtils::getInstance()->isFileExist(face)) {
        _input->setFontName(face);
    } else if (bold) {
        _input->setFontName("sans-serif-bold");
    }
}

void TextField::setWhiteBackground(bool white) {
    if (_background) {
        _background->setColor(white ? ax::Color3B::WHITE : ax::Color3B(20, 20, 20));
        _background->setOpacity(white ? 255 : 230);
    }
}

void TextField::setTextColor(const ax::Color3B& color) {
    if (_input) {
        _input->setTextColor(ax::Color4B(color.r, color.g, color.b, 255));
    }
}

void TextField::setFontSize(float size) {
    if (_input) {
        _input->setFontSize(size);
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
    if (!hit(this, touch)) return false;
    const ax::Vec2 p = localPointFromDesign(touch->getLocation(), this);
    const float t = (p.x - 8.0f) / (_trackWidth > 0.0f ? _trackWidth : 1.0f);
    setValue(t, true);
    return true;
}

void Slider::onTouchMoved(ax::Touch* touch, ax::Event*) {
    const ax::Vec2 p = localPointFromDesign(touch->getLocation(), this);
    const float t = (p.x - 8.0f) / (_trackWidth > 0.0f ? _trackWidth : 1.0f);
    setValue(t, true);
}

void Slider::onTouchEnded(ax::Touch*, ax::Event*) {}

// --- Modal -----------------------------------------------------------------
Modal::~Modal() {
    if (_overlay) {
        _overlay->removeFromParent();
        _overlay->release();
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
    _overlay->retain();
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
    close->setColors(ax::Color3B(122, 122, 122), ax::Color3B(62, 62, 62));
    close->onClick = [this] { this->close(); };
    // Position from the axmol bottom-left so it also works before onEnter
    // (a Modal is a plain C++ object, not a node: no onEnter hook at all).
    _panel->addChild(static_cast<ax::Node*>(close), 10);
    close->setPosition(ax::Vec2(_width - 23.0f, 15.0f));
    _close = close;
}

void Modal::setCloseLabel(const std::string& label) {
    if (_close) {
        _close->setLabel(label);
    }
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
    listener->onTouchBegan = [node](ax::Touch* t, ax::Event*) { return hit(node, t); };
    node->getEventDispatcher()->addEventListenerWithSceneGraphPriority(listener, node);
}

} // namespace ui
