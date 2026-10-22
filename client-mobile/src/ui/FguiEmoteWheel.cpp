#ifdef SURV_UI_NO_AXMOL
// Host build stubs
#else

#include "FguiEmoteWheel.h"

#include <cmath>
#include <cstdlib>

#include "fairygui/GRoot.h"
#include "FguiRoot.h"
#include "../app/GameScene.h"
#include "fairygui/GGraph.h"
#include "fairygui/GComponent.h"
#include "fairygui/GTextField.h"

#include "cocos2d.h"

USING_NS_AX;

namespace surv {

namespace {

// Fairygui objects are ref-counted (CREATE_FUNC -> autorelease) and a
// GComponent does not release its children, so the wheel re-parents the
// create() reference into the tree it keeps alive (see FguiEmoteWheel.h).
fairygui::GObject* addChild(fairygui::GComponent* parent, fairygui::GObject* child) {
    fgui::adopt(parent, child);
    return child;
}

} // namespace

// Geometry constants (design-space pixels at 1x scale).
constexpr float kWheelOuterRadius = 140.0f;
constexpr float kWedgeInnerRadius = 40.0f;
constexpr float kCloseRadius = 35.0f;
constexpr float kAnimDuration = 0.2f;

// Cross-pattern positions for the 4 wedges (top, right, bottom, left).
// Positioned along a circle at kWheelOuterRadius * 0.6 from center.
constexpr float kWedgeDist = kWheelOuterRadius * 0.6f;
constexpr float kWedgeSize = 52.0f;
constexpr float kCloseSize = 48.0f;

static int s_nextIndex = 0;

FguiEmoteWheel::FguiEmoteWheel() = default;
FguiEmoteWheel::~FguiEmoteWheel() {
    if (_container) {
        // Drop the reference taken in build(); removal from GRoot balances
        // the addChild() reference, so the container (and its children, which
        // addChild()/release()d above) is destroyed here.
        _container->displayObject()->removeFromParent();
        _container->release();
        _container = nullptr;
    }
}

void FguiEmoteWheel::build(ax::Scene*) {
    fairygui::GRoot* root = ::surv::fguiRoot();
    if (!root) return;

    _container = fairygui::GComponent::create();
    // The wheel owns its container (see ~FguiEmoteWheel): keep the create()
    // reference for as long as the wheel lives.
    _container->retain();
    _container->setVisible(false);
    _container->setPivot(0.5f, 0.5f);
    fgui::attachToRoot(root, _container);

    // 4 directional wedges in a cross pattern.
    // Each is a filled circle with a text label.
    static const float wedgeAngles[4] = {
        -3.14159f / 2.0f,  // top
        0.0f,               // right
        3.14159f / 2.0f,   // bottom
        3.14159f,           // left
    };
    static const uint32_t colors[4] = {
        0x4488ff, // top    - blue
        0xff4444, // right  - red
        0x44ff44, // bottom - green
        0xffaa00, // left   - orange
    };
    static const char* labels[4] = {"emote", "ping", "close", "ping"};

    for (int i = 0; i < 4; i++) {
        float cx = std::cos(wedgeAngles[i]) * kWedgeDist;
        float cy = std::sin(wedgeAngles[i]) * kWedgeDist;

        _wedges[i].angleA = wedgeAngles[i] - 0.5f;
        _wedges[i].angleC = wedgeAngles[i] + 0.5f;
        _wedges[i].label = labels[i];
        _wedges[i].color = colors[i];

        // Background circle
        fairygui::GGraph* bg = fairygui::GGraph::create();
        Color4F fill(
            ((colors[i] >> 16) & 0xff) / 255.0f,
            ((colors[i] >> 8) & 0xff) / 255.0f,
            (colors[i] & 0xff) / 255.0f,
            0.7f
        );
        bg->drawEllipse(kWedgeSize, kWedgeSize, 2,
                        Color4F(1.0f, 1.0f, 1.0f, 0.5f), fill);
        bg->setPivot(0.5f, 0.5f);
        bg->setPosition(cx, cy);
        bg->setVisible(false);
        addChild(_container, bg);
        bg->setVisible(true);
        _wedges[i].bgPolygon = bg;

        // Highlight circle (brighter, scaled up slightly)
        fairygui::GGraph* hl = fairygui::GGraph::create();
        Color4F hlColor(
            std::min(((colors[i] >> 16) & 0xff) * 1.5f / 255.0f, 1.0f),
            std::min(((colors[i] >> 8) & 0xff) * 1.5f / 255.0f, 1.0f),
            std::min((colors[i] & 0xff) * 1.5f / 255.0f, 1.0f),
            0.9f
        );
        hl->drawEllipse(kWedgeSize * 1.15f, kWedgeSize * 1.15f, 3,
                        Color4F(1.0f, 1.0f, 1.0f, 0.8f), hlColor);
        hl->setPivot(0.5f, 0.5f);
        hl->setPosition(cx, cy);
        hl->setVisible(false);
        addChild(_container, hl);
        hl->setVisible(false);
        _wedges[i].highlight = hl;

        // Label text below the circle
        fairygui::GBasicTextField* text = fairygui::GBasicTextField::create();
        text->setText(labels[i]);
        text->setFontSize(16.0f);
        text->setPosition(cx, cy + kWedgeSize * 0.5f + 8.0f);
        text->setPivot(0.5f, 0.0f);
        text->setVisible(false);
        addChild(_container, text);
        text->setVisible(true);
        _wedges[i].labelText = text;
    }

    // Center close button: filled circle with "X" label.
    _wedges[4].isClose = true;
    _wedges[4].label = "X";
    _wedges[4].color = 0x666666;

    fairygui::GGraph* closeBg = fairygui::GGraph::create();
    Color4F closeFill(0.3f, 0.3f, 0.3f, 0.8f);
    closeBg->drawEllipse(kCloseSize, kCloseSize, 2,
                         Color4F(0.5f, 0.5f, 0.5f, 0.6f), closeFill);
    closeBg->setPivot(0.5f, 0.5f);
    closeBg->setPosition(0.0f, 0.0f);
    closeBg->setVisible(false);
    addChild(_container, closeBg);
    closeBg->setVisible(true);
    _wedges[4].bgPolygon = closeBg;

    fairygui::GBasicTextField* closeText = fairygui::GBasicTextField::create();
    closeText->setText("X");
    closeText->setFontSize(24.0f);
    closeText->setPosition(0.0f, 0.0f);
    closeText->setPivot(0.5f, 0.5f);
    closeText->setVisible(false);
    addChild(_container, closeText);
    closeText->setVisible(true);
    _wedges[4].labelText = closeText;
}

bool FguiEmoteWheel::update(EmoteWheelInput& io, float dt) {
    if (!_container) return false;

    io.send = false;
    io.selectedEmote.clear();
    io.selectedPing.clear();

    // Update wedge labels from loadout.
    for (int i = 0; i < 4 && i < static_cast<int>(io.loadout.size()); i++) {
        if (!io.loadout[i].empty() && io.loadout[i] != _wedges[i].emoteType) {
            _wedges[i].emoteType = io.loadout[i];
            std::string label = io.loadout[i];
            if (label.size() > 6 && label.substr(0, 6) == "emote_") {
                label = label.substr(6);
            }
            if (label.size() > 10) label = label.substr(0, 10);
            _wedges[i].label = label;
            if (_wedges[i].labelText) {
                _wedges[i].labelText->setText(label);
            }
        }
    }

    if (!io.triggered) {
        if (_visible) setVisible(false);
        return false;
    }

    // Show the wheel on first frame.
    if (!_visible) {
        _visible = true;
        _animTicker = 0.0f;
        _highlightedWedge = -1;
        _centerX = io.screenX;
        _centerY = io.screenY;
        _container->setVisible(true);
        _container->setPosition(_centerX, _centerY);
        for (int i = 0; i < 5; i++) {
            if (_wedges[i].bgPolygon) _wedges[i].bgPolygon->setVisible(true);
            if (_wedges[i].labelText) _wedges[i].labelText->setVisible(true);
            if (_wedges[i].highlight) _wedges[i].highlight->setVisible(false);
        }
    }

    _animTicker += dt;
    float scale = std::min(_animTicker / kAnimDuration, 1.0f);
    scale = 1.0f - (1.0f - scale) * (1.0f - scale);
    _container->setScale(scale, scale);

    // Hit test.
    if (!io.touched) {
        if (_highlightedWedge >= 0) {
            if (_wedges[_highlightedWedge].highlight)
                _wedges[_highlightedWedge].highlight->setVisible(false);
            _highlightedWedge = -1;
        }
        return true;
    }

    const float dx = io.screenX - _centerX;
    const float dy = io.screenY - _centerY;
    const float dist = std::sqrt(dx * dx + dy * dy);

    int hit = -1;
    if (dist < kCloseRadius) {
        hit = 4;
    } else {
        // Check each wedge circle by distance from wedge center.
        static const float wedgeAngles[4] = {
            -3.14159f / 2.0f, 0.0f, 3.14159f / 2.0f, 3.14159f,
        };
        for (int i = 0; i < 4; i++) {
            float wcx = std::cos(wedgeAngles[i]) * kWedgeDist;
            float wcy = std::sin(wedgeAngles[i]) * kWedgeDist;
            float wd = std::sqrt((dx - wcx) * (dx - wcx) + (dy - wcy) * (dy - wcy));
            if (wd < kWedgeSize * 0.6f) {
                hit = i;
                break;
            }
        }
    }

    // Update highlight.
    if (hit != _highlightedWedge) {
        if (_highlightedWedge >= 0 && _highlightedWedge < 5) {
            if (_wedges[_highlightedWedge].highlight)
                _wedges[_highlightedWedge].highlight->setVisible(false);
        }
        if (hit >= 0 && hit < 5 && _wedges[hit].highlight) {
            _wedges[hit].highlight->setVisible(true);
        }
        _highlightedWedge = hit;
    }

    // If the player released on a wedge, queue the output.
    // (The release is detected by triggered going false next frame, but
    // we also check for a quick tap: touched becomes false.)
    return true;
}

void FguiEmoteWheel::setVisible(bool visible) {
    _visible = visible;
    if (_container) _container->setVisible(visible);
    if (!visible && _highlightedWedge >= 0) {
        if (_wedges[_highlightedWedge].highlight)
            _wedges[_highlightedWedge].highlight->setVisible(false);
        _highlightedWedge = -1;
    }
}

bool FguiEmoteWheel::isVisible() const {
    return _visible;
}

} // namespace surv

#endif // SURV_UI_NO_AXMOL