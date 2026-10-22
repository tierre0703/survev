#pragma once
// Label helper for the in-game HUD.
//
// GBasicTextField wraps an ax::Label and, the first time it measures itself,
// rasterises the text as a system-font string texture. On the Android target
// axmol's Label::createSpriteForSystemFont() frees a garbage Timer the first
// time that path runs (`~Timer` SIGILL), so the HUD cannot use it.
//
// This class is an empty FairyGUI component (GComponent) whose display label is
// a plain ax::Label on the bundled TTF (Roboto Condensed), so it still travels
// in the HUD tree like any other GObject (addChild/pivot/position/visible) while
// the glyphs rasterise through the TTF path.
#include "fairygui/GComponent.h"

#include <string>

namespace ax {
class Label;
}

namespace fairygui {
class GRoot;
}

namespace surv {
namespace fgui {

class TextField : public fairygui::GComponent {
public:
    // CREATE_FUNC-generated create() calls init(), which runs GObject::handleInit.
    CREATE_FUNC(TextField);

    TextField();
    ~TextField() override;

    void setText(const std::string& value);
    const std::string& getText() const { return _value; }
    void setFontSize(float size);
    void setColor(const ax::Color3B& value);

    // ---- text-only conveniences (single-line by default) ----
    void setSingleLine(bool value);

    ax::Node* label() const { return _label; }

    void setVisible(bool visible);
    bool isVisible() const;

    // ax::Label-compatible no-ops for the HUD's layout calls.
    void setLineBreakWithoutSpace(bool value);
    void setDimensions(float width, float height);

private:
    void ensureLabel();
    void rebuild();

    void buildTexture();

    ax::Node* _label = nullptr;
    bool _visible = true;
    std::string _value;
    float _size = 16.0f;
    bool _singleLine = true;
    ax::Color3B _color = ax::Color3B::WHITE;
};

} // namespace fgui
} // namespace surv
