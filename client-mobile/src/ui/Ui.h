#pragma once
// Native UI toolkit (axmol) used by the menu / loadout / team / in-game
// overlay. It is the counterpart of the web client's HTML+CSS UI:
//
//   web                          -> native
//   <div class="menu-block">     -> ui::Panel      (ui::Layout + background)
//   <a class="btn-darken">       -> ui::Button     (9-slice + label)
//   <input type="text">          -> ui::TextField  (ui::TextField wrapper)
//   <input type="range">         -> ui::Slider     (track + knob, drag)
//   <div class="account-alert">  -> ui::badge      (notify dot on a button)
//   #modal-... + overlay         -> ui::Modal      (dim + centered panel)
//
// IMPORTANT COORDINATES: the web UI lays out in CSS pixels with y growing
// *down*, but axmol nodes use y growing *up*. Every helper here therefore
// takes y-down coordinates (like the web client) and flips them with
// `ui::kit::fromCssY()`. Callers place children from the top-left, exactly
// like the DOM, and the toolkit keeps the axmol scene graph consistent.
#include "Localization.h"

#include "axmol.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace ui {

// --- coordinate helpers ----------------------------------------------------
namespace kit {
// The screen is designed at a fixed "CSS" resolution (like the web client's
// window); the scene root is scaled to the real display. All UI geometry is
// authored in these units.
inline constexpr float kDesignWidth = 1280.0f;
inline constexpr float kDesignHeight = 720.0f;

inline float designWidth() { return kDesignWidth; }
inline float designHeight() { return kDesignHeight; }
// CSS y (0 = top) -> axmol y (0 = bottom).
inline float fromCssY(float cssY, float height = kDesignHeight) { return height - cssY; }
inline ax::Vec2 fromCss(float cssX, float cssY, float height = kDesignHeight) {
    return ax::Vec2(cssX, fromCssY(cssY, height));
}
} // namespace kit

// --- colours ---------------------------------------------------------------
// CSS hex string ("#rrggbb", "#rgb", "#rrggbbaa", "rgb(a,b,c)") -> axmol.
ax::Color3B parseHexColor(const std::string& css, const ax::Color3B& fallback = ax::Color3B::WHITE);
ax::Color4B parseHexColorA(const std::string& css, const ax::Color4B& fallback = ax::Color4B::WHITE);

// --- Panel -----------------------------------------------------------------
// A positioned, optionally-clipped container. Anchor is top-left (web `div`
// default): `setPosition` takes CSS coordinates.
class Panel : public ax::Node {
public:
    static Panel* create(float width, float height);

    bool init(float width, float height);
    void setBackgroundColor(const ax::Color3B& color, float alpha);
    void setCssPosition(float x, float y);
    // Anchored helpers (top-right / bottom-left / bottom-right / center).
    void placeTopLeft(float x, float y) { setCssPosition(x, y); }
    void placeTopRight(float x, float y) { setCssPosition(kit::designWidth() - getContentSize().width - x, y); }
    void placeBottomLeft(float x, float y) { setCssPosition(x, kit::designHeight() - getContentSize().height - y); }
    void placeBottomRight(float x, float y) { setCssPosition(kit::designWidth() - getContentSize().width - x, kit::designHeight() - getContentSize().height - y); }
    void placeCenter(float offsetX = 0.0f, float offsetY = 0.0f) {
        setCssPosition((kit::designWidth() - getContentSize().width) * 0.5f + offsetX,
                       (kit::designHeight() - getContentSize().height) * 0.5f + offsetY);
    }

    void setInteractive(bool interactive);
    bool isInteractive() const { return _interactive; }

protected:
    ax::LayerColor* _background = nullptr;
    bool _interactive = false;
};

// --- Button ----------------------------------------------------------------
// 9-slice background (from the atlas) plus a centered label, matching the web
// `.btn-darken`/`.btn-green` look. `onClick` fires on tap.
class Button : public ax::Node {
public:
    static Button* create(const std::string& label, float width, float height,
                          const std::string& bgFrame = "");

    bool init(const std::string& label, float width, float height, const std::string& bgFrame);

    void setLabel(const std::string& text);
    const std::string& label() const { return _label; }
    void setEnabled(bool enabled);
    bool isEnabled() const { return _enabled; }
    void setFontSize(float size);
    void setTitleColor(const ax::Color3B& color);
    // Optional translucent black instead of an atlas 9-slice.
    void useTranslucentBackground(float alpha);
    void showBadge(bool show);
    void setCssPosition(float x, float y) { ax::Node::setPosition(kit::fromCss(x, y)); }

    std::function<void()> onClick;

protected:
    std::function<void(ax::Object*)> _handler;
    std::string _label;
    ax::ui::Scale9Sprite* _background = nullptr;
    ax::Label* _title = nullptr;
    ax::DrawNode* _badge = nullptr;
    bool _enabled = true;
    float _fontSize = 18.0f;
};

// --- TextField -------------------------------------------------------------
class TextField : public ax::Node {
public:
    static TextField* create(const std::string& placeholder, float width, float height);

    bool init(const std::string& placeholder, float width, float height);
    std::string getText() const;
    void setText(const std::string& text);
    void setMaxLength(int length) { _input->setMaxLength(length); }
    void setCssPosition(float x, float y) { ax::Node::setPosition(kit::fromCss(x, y)); }
    void setOnChanged(std::function<void(const std::string&)> cb) { _onChanged = std::move(cb); }
    ax::ui::TextField* input() const { return _input; }

protected:
    ax::ui::TextField* _input = nullptr;
    ax::LayerColor* _background = nullptr;
    std::function<void(const std::string&)> _onChanged;
};

// --- Slider ----------------------------------------------------------------
// Range slider with drag (mouse/touch), matching `<input type="range">`.
class Slider : public ax::Node {
public:
    static Slider* create(float width, float value = 1.0f);

    bool init(float width, float value);
    float value() const { return _value; }
    void setValue(float value, bool fire = true);
    void setCssPosition(float x, float y) { ax::Node::setPosition(kit::fromCss(x, y)); }
    std::function<void(float)> onChanged;

protected:
    bool onTouchBegan(ax::Touch* touch, ax::Event* event);
    void onTouchMoved(ax::Touch* touch, ax::Event* event);
    void onTouchEnded(ax::Touch* touch, ax::Event* event);
    void layoutKnob();

    float _value = 1.0f;
    float _trackWidth = 200.0f;
    ax::DrawNode* _track = nullptr;
    ax::DrawNode* _fill = nullptr;
    ax::DrawNode* _knob = nullptr;
};

// --- Modal -----------------------------------------------------------------
// A dimmed full-screen overlay with a centered panel, the native equivalent of
// the web modal + `#modal-screen-block`.
class Modal {
public:
    Modal() = default;
    ~Modal();

    Panel* build(float width, float height, const std::string& title = "");
    void show();
    void hide();
    bool isOpen() const { return _visible; }

    Panel* panel() const { return _panel; }
    ax::Node* overlay() const { return _overlay; }
    void setOnClosed(std::function<void()> cb) { _onClosed = std::move(cb); }
    void close() { hide(); if (_onClosed) _onClosed(); }

    std::function<void()> onShow;
    std::function<void()> onHide;

protected:
    void addCloseButton();
    void refreshSize();

    ax::LayerColor* _overlay = nullptr;
    Panel* _panel = nullptr;
    ax::Node* _root = nullptr;
    bool _visible = false;
    float _width = 400.0f;
    float _height = 300.0f;
    std::function<void()> _onClosed;
};

// --- shared scene plumbing -------------------------------------------------
// Creates (once) the UI root node under `scene` at the given z order and
// returns it. The root is sized to the design resolution.
ax::Node* uiRoot(ax::Scene* scene, int zOrder);
// Re-positions/resizes the UI root after a resolution change.
void layoutUiRoot(ax::Node* root, const ax::Vec2& visibleSize, const ax::Vec2& designSize);

// Makes an ax::Node swallow touches (used by overlays/panels).
void swallowTouches(ax::Node* node);

} // namespace ui
