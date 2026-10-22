#include "FguiText.h"

#include "cocos2d.h"

USING_NS_AX;

namespace surv {
namespace fgui {

namespace {
// The web HUD's family is Roboto Condensed; the app stages the TTF under
// Content/fonts.
std::string hudFontPath() {
    static const std::string path = [] {
        auto* files = FileUtils::getInstance();
        if (files->isFileExist("fonts/RobotoCondensed-Normal.ttf")) {
            return files->fullPathForFilename("fonts/RobotoCondensed-Normal.ttf");
        }
        return std::string();
    }();
    return path;
}
} // namespace

TextField::TextField() = default;

TextField::~TextField() {
    if (_label) {
        _label->removeFromParent();
        _label->release();
        _label = nullptr;
    }
}

void TextField::ensureLabel() {
    if (_label) {
        return;
    }
    buildTexture();
    if (!_label) {
        // Nothing to measure yet (empty/failed raster): use a tiny placeholder so
        // the component still has a display node.
        _label = Node::create();
        if (!_label) {
            return;
        }
        _label->retain();
    }
    _label->setAnchorPoint(Vec2(0.0f, 1.0f));
    // The text node rides on this component's display object, below any
    // component children of its own.
    if (displayObject() && _label->getParent() == nullptr) {
        displayObject()->addChild(_label, -1);
        displayObject()->setCascadeOpacityEnabled(false);
    }
}

void TextField::buildTexture() {
    // The text is rasterised as a texture with the bundled TTF. axmol's Label
    // TTF layer (createWithTTF / setTTFConfig) faults on this target, and the
    // string-texture path is the one the menu UI already uses, so the HUD text
    // goes through Texture2D::initWithString() here.
    FontDefinition def;
    const std::string face = hudFontPath();
    def._fontName = face.empty() ? std::string("sans-serif") : face;
    def._fontSize = _size;
    def._fontFillColor = _color;

    auto* text = new Texture2D();
    text->initWithString(_value, def);
    auto* sprite = Sprite::createWithTexture(text);
    text->release();
    if (!sprite) {
        return;
    }
    sprite->setColor(_color);
    sprite->setAnchorPoint(Vec2(0.0f, 1.0f));
    sprite->retain();
    if (_label && _label->getParent() && displayObject()) {
        displayObject()->removeAllChildren();
    }
    if (_label) {
        _label->release();
    }
    _label = sprite;
    _label->setVisible(_visible);
}

void TextField::rebuild() {
    buildTexture();
    ensureLabel();
}

void TextField::setText(const std::string& value) {
    if (_value == value) {
        return;
    }
    _value = value;
    rebuild();
}

void TextField::setFontSize(float size) {
    if (_size == size) {
        return;
    }
    _size = size;
    rebuild();
}

void TextField::setColor(const ax::Color3B& value) {
    _color = value;
    rebuild();
}

void TextField::setSingleLine(bool value) {
    if (_singleLine == value) {
        return;
    }
    _singleLine = value;
    rebuild();
}

void TextField::setVisible(bool visible) {
    _visible = visible;
    if (_label) {
        _label->setVisible(visible);
    }
}

bool TextField::isVisible() const { return _visible; }

void TextField::setLineBreakWithoutSpace(bool value) { (void)value; }

void TextField::setDimensions(float width, float height) {
    (void)width;
    (void)height;
}

} // namespace fgui
} // namespace surv
