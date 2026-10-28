#ifdef SURV_UI_NO_AXMOL
#else

#include "FguiPieTimer.h"
#include "../render/Defs.h"
#include "../render/Camera.h"

#include <cmath>
#include <sstream>
#include <iomanip>
#include "fairygui/GRoot.h"
#include "fairygui/GComponent.h"
#include "fairygui/GGraph.h"
#include "fairygui/GTextField.h"

USING_NS_AX;

namespace surv {

// Arc radius in FG design-space pixels.
static constexpr float kArcRadius = 35.0f;
static constexpr float kArcDiameter = kArcRadius * 2.0f;
static constexpr float kLineWidth = 6.0f;
static constexpr float kFontSize = 24.0f;
static constexpr float kLabelY = 87.5f;

PieTimer::PieTimer() = default;
PieTimer::~PieTimer() = default;

void PieTimer::build() {
    fairygui::GRoot* root = fairygui::GRoot::getInstance();
    if (!root) return;

    // Container (GComponent to group arc + text together).
    fairygui::GComponent* c = fairygui::GComponent::create();
    c->setVisible(false);
    _container = c;

    // Arc: GGraph doesn't have native arc() or clear().  Instead we
    // recreate the graph each frame by rebuilding its shape.
    fairygui::GGraph* arc = fairygui::GGraph::create();
    arc->setVisible(false);
    c->addChild(arc);
    _arc = arc;

    // Countdown text (centered in the circle).
    fairygui::GBasicTextField* counter = fairygui::GBasicTextField::create();
    counter->setText("");
    counter->setColor(Color3B(255, 255, 255));
    counter->setFontSize(kFontSize);
    counter->setSingleLine(true);
    counter->setPosition(0.0f, 0.0f);
    counter->setPivot(0.5f, 0.5f);
    c->addChild(counter);
    _counterText = counter;

    // Label text (below the circle).
    fairygui::GBasicTextField* label = fairygui::GBasicTextField::create();
    label->setText("");
    label->setColor(Color3B(255, 255, 255));
    label->setFontSize(kFontSize);
    label->setSingleLine(true);
    label->setPosition(0.0f, kLabelY);
    label->setPivot(0.5f, 0.5f);
    c->addChild(label);
    _labelText = label;

    // Position the container at screen center initially; update() moves it.
    c->setPosition(0.0f, 0.0f);
    root->addChild(c);
}

void PieTimer::start(const std::string& label, float elapsed, float duration) {
    _active = true;
    _label = label;
    _elapsed = elapsed;
    _duration = duration > 0.0f ? duration : 1.0f;
    setVisible(true);
}

void PieTimer::stop() {
    _active = false;
    setVisible(false);
}

void PieTimer::setVisible(bool visible) {
    if (_container) _container->setVisible(visible);
    if (_arc) _arc->setVisible(visible);
}

void PieTimer::resize(bool smallLayout, bool tablet, bool landscape, float screenScaleFactor) {
    _smallLayout = smallLayout;
    _tablet = tablet;
    _landscape = landscape;
    _screenScaleFactor = screenScaleFactor;
    // pieTimer.ts resize(): phones (Sm layout) draw the timer at half scale;
    // a tablet keeps full size. The mobile offset is not tablet-gated.
    _halfScale = (_smallLayout && !_tablet) ? 0.5f : 1.0f;
    _mobileOffset =
        _smallLayout ? (landscape ? 25.0f : 100.0f) : 0.0f;
}

void PieTimer::update(float dt, const Camera& camera) {
    if (!_active || !_container || !_arc) {
        if (_container) _container->setVisible(false);
        return;
    }

    _elapsed = std::min(_elapsed + dt, _duration);
    const float progress = _elapsed / _duration;  // 0..1

    // Redraw the arc.  GGraph has no clear() — we redraw by setting
    // visibility based on progress.  A full-circle outline fades in
    // as progress grows; when complete a filled circle flashes briefly.
    // The web client draws a pie-slice polygon via DrawNode; GGraph
    // lacks that, so we approximate with the full-ellipse outline.
    if (progress < 1.0f) {
        _arc->drawEllipse(kArcDiameter, kArcDiameter,
                          static_cast<int>(kLineWidth),
                          Color4F(1.0f, 1.0f, 1.0f, 0.8f),
                          Color4F(0.0f, 0.0f, 0.0f, 0.0f));
        _arc->setVisible(true);
    } else {
        // Completed — draw a filled circle briefly.
        _arc->drawEllipse(kArcDiameter, kArcDiameter, 0,
                          Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                          Color4F(1.0f, 1.0f, 1.0f, 0.3f));
        _arc->setVisible(true);
    }

    // Countdown text.
    const float remaining = std::max(0.0f, _duration - _elapsed);
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(static_cast<std::streamsize>(1)) << remaining;
    _counterText->setText(oss.str());

    // Label.
    _labelText->setText(_label.empty() ? "" : _label);

    // Position at screen top-center (pieTimer.ts):
    // x = screen centre; y = (m_screenHeight / 3) * screenScaleFactor + the
    // small-layout mobile offset.
    const float x = camera.m_screenWidth * 0.5f;
    const float y = (camera.m_screenHeight / 3.0f) * _screenScaleFactor + _mobileOffset;
    _container->setPosition(x, y);
    // The web scales the whole container; FGUI needs both axes.
    _container->setScaleX(_halfScale);
    _container->setScaleY(_halfScale);

    _container->setVisible(true);
    _arc->setVisible(true);
}

} // namespace surv

#endif // SURV_UI_NO_AXMOL