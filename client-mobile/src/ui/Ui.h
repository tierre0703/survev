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
#include "Layout.h"

#include "axmol.h"
#include "ui/UIScale9Sprite.h"
#include "ui/UITextField.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace ax {
class Event;
class Touch;
} // namespace ax

// Declared before `namespace ui` (defined in Files.cpp) so the UI helpers below
// can call it. The explicit `surv::` scope keeps it distinct from axmol's
// `ax::ui` in translation units that have `using namespace ax`.
namespace surv { namespace ui { void registerGuiFonts(const char* dir); } }

namespace ui {

// --- Typography ------------------------------------------------------------
// The web client renders all UI text in Roboto Condensed. The native build
// ships a TTF copy under `Content/fonts`; `makeLabel` uses it when present and
// falls back to the platform sans-serif otherwise. `bulk = true` marks labels
// laid out in a tight grid (loadout cells), where per-glyph TTF metrics would
// overflow the box, so those stay on the system font.
//
// `makeLabel`/`fontPath`/`setLabelSize`/`setLabelBold` all take an optional
// `bulk` flag so a caller can opt a whole subtree into the system font.
ax::Label* makeLabel(const std::string& text, float size, bool bold = false, bool bulk = false);
// Resolves a family + weight to a staged TTF path ("" -> system font).
std::string fontPath(const std::string& family, bool bold = false);
// Resize/re-bold a label created by `makeLabel`. axmol's `setSystemFontSize` on
// a TTF label corrupts its face size, so these helpers dispatch by label type.
void setLabelSize(ax::Label* label, float size);
void setLabelBold(ax::Label* label, bool bold, bool bulk = false);

// --- coordinate helpers ----------------------------------------------------
// `ui::kit` (the 1280x720 design canvas and the CSS y-flip) lives in Layout.h so
// it can be shared with the host tests; it is included above.

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
    void onEnter() override;
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
    ax::Vec2 _cssPosition;
    bool _hasCssPosition = false;
    ax::LayerColor* _background = nullptr;
    bool _interactive = false;
};

// --- Button ----------------------------------------------------------------
// Native counterpart of the web client's `.btn-green` / `.btn-darken` /
// `.btn-hollow` anchors: a filled body with the CSS bottom-border/shadow, a
// centered label, an optional high-res PNG icon from the generated GUI sheet
// (`gui/<name>.png`, see tools/build-gui-icons.mjs) and an optional alert badge.
// `onClick` fires on tap.
class Button : public ax::Node {
public:
    static Button* create(const std::string& label, float width, float height,
                          const std::string& bgFrame = "");

    ~Button() override;
    bool init(const std::string& label, float width, float height, const std::string& bgFrame);

    void setLabel(const std::string& text);
    const std::string& label() const { return _label; }
    void setEnabled(bool enabled);
    bool isEnabled() const { return _enabled; }
    void setFontSize(float size);
    // Web `.player-name-input` is bold white-on-white; most buttons inherit the
    // white UI text colour (`.btn-green`/`.btn-darken` set `color: #fff`).
    void setTitleColor(const ax::Color3B& color);
    // Switches the label to the bold TTF face (or the system bold face) and the
    // button's primary/secondary CSS colours in one call.
    void setBold(bool bold);
    // Opts the label out of the bundled TTF and onto the system font (used by
    // tightly-packed grids whose cells are sized for system-font metrics).
    void setBulkFont(bool bulk);
    void setColors(const ax::Color3B& body, const ax::Color3B& shadow);
    void setHollow(bool hollow, bool selected = false);
    // Web `.btn-hollow`: a 2px border with a transparent body. `selected` uses
    // the web green `#00ff00` border + `rgba(0,0,0,.35)` fill
    // (`.btn-hollow-selected`).
    void setHollowBorder(bool selected = false);
    // Web `.btn-hollow-selected` toggle: `selected` -> green border + dark fill,
    // otherwise the plain `.btn-hollow` 2px white border with a clear body.
    void setToggle(bool on);
    bool isToggleOn() const { return _toggleOn; }
    // Web `.btn-darken:active { filter: brightness(80%) }`: fades brightness
    // over 0.25s while held (matches the CSS transition).
    void setPressed(bool pressed);
    bool isPressed() const { return _pressed; }
    // axmol ActionTween driver for the brightness animation.
    void update(float dt) override;
    // High-res PNG icon from `Content/gui/<name>.png`; empty removes it.
    void setIcon(const std::string& name);
    void setIconScale(float scale);
    // Layout: icon left of the label (web `.btn-social`), or label only.
    void setIconLayout(bool left);
    // Optional translucent black instead of an atlas 9-slice.
    void useTranslucentBackground(float alpha);
    void showBadge(bool show);
    void setCssPosition(float x, float y);
    void onEnter() override;

    std::function<void()> onClick;

protected:
    void applyBodyColor();
    void layoutLabel();

    ax::Vec2 _cssPosition;
    bool _hasCssPosition = false;
    std::function<void(ax::Object*)> _handler;
    std::string _label;
    ax::ui::Scale9Sprite* _background = nullptr;
    ax::LayerColor* _fallbackBackground = nullptr;
    ax::Sprite* _icon = nullptr;
    ax::Label* _title = nullptr;
    ax::DrawNode* _badge = nullptr;
    bool _enabled = true;
    bool _hollow = false;
    bool _hollowBorder = false;
    bool _hollowSelected = false;
    bool _iconLeft = false;
    bool _labelBold = false;
    bool _bulkFont = false;
    bool _pressed = false;
    bool _toggleOn = false;
    // CSS brightness tween shared by the body, border, icon and label.
    float* _brightness = nullptr;
    float _lastBrightness = 1.0f;
    ax::Color3B _cachedBodyColor{40, 40, 40};
    ax::Color3B _cachedShadowColor{28, 28, 28};
    float _fontSize = 18.0f;
    ax::Color3B _bodyColor{40, 40, 40};
    ax::Color3B _shadowColor{28, 28, 28};
};

// --- TextField -------------------------------------------------------------
class TextField : public ax::Node {
public:
    static TextField* create(const std::string& placeholder, float width, float height);

    bool init(const std::string& placeholder, float width, float height);
    std::string getText() const;
    void setText(const std::string& text);
    void setMaxLength(int length) { _input->setMaxLength(length); }
    // Web `.player-name-input { background:#fff; color:#000; font-weight:700 }`
    void setWhiteBackground(bool white);
    void setTextColor(const ax::Color3B& color);
    void setFontSize(float size);
    void setBold(bool bold);
    void setCssPosition(float x, float y);
    void onEnter() override;
    void setOnChanged(std::function<void(const std::string&)> cb) { _onChanged = std::move(cb); }
    ax::ui::TextField* input() const { return _input; }

protected:
    ax::Vec2 _cssPosition;
    bool _hasCssPosition = false;
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
    void setCssPosition(float x, float y);
    void onEnter() override;
    std::function<void(float)> onChanged;

protected:
    ax::Vec2 _cssPosition;
    bool _hasCssPosition = false;
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
    ax::Node* root() const { return _root; }
    // Overrides the corner close button's label (default "×"); pass an l10n
    // title for modals that need a readable close control.
    void setCloseLabel(const std::string& label);
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
    Button* _close = nullptr;
    bool _visible = false;
    float _width = 400.0f;
    float _height = 300.0f;
    std::function<void()> _onClosed;
};

// --- shared scene plumbing -------------------------------------------------
// Tag the menu scene puts on its scaled design-space UI root; the in-game
// overlay adopts it so HUD geometry and tap mapping match the menu.
// (`kUiRootTag` is declared in Layout.h.)

// Creates (once) the UI root node under `scene` at the given z order and
// returns it. The root is sized to the design resolution.
ax::Node* uiRoot(ax::Scene* scene, int zOrder);
// Re-positions/resizes the UI root after a resolution change.
void layoutUiRoot(ax::Node* root, const ax::Vec2& visibleSize, const ax::Vec2& designSize);

// Makes an ax::Node swallow touches (used by overlays/panels).
void swallowTouches(ax::Node* node);

} // namespace ui
