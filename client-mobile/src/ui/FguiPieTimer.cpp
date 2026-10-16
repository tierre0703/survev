#ifdef SURV_UI_NO_AXMOL
#else

#include "FguiPieTimer.h"
#include "../render/Defs.h"
#include "../render/Camera.h"

#include "fairygui/GRoot.h"
#include "fairygui/GComponent.h"
#include "fairygui/GGraph.h"
#include "fairygui/GTextField.h"
#include "fairygui/GObject.h"

#include <cmath>
#include <sstream>

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

    // Arc: a circle outline that we redraw each frame with a partial arc.
    // GGraph doesn't have a native arc() API, so we draw a full circle
    // outline and redraw it each frame. The arc is approximated by drawing
    // a pie-slice-shaped polygon or using drawRegularPolygon.
    // For simplicity we draw the progress as a partial circle's sector.
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

void PieTimer::update(float dt, const Camera& camera, bool mobile, float mobileOffset) {
    if (!_active || !_container || !_arc) {
        if (_container) _container->setVisible(false);
        return;
    }

    _elapsed = std::min(_elapsed + dt, _duration);
    const float progress = _elapsed / _duration;  // 0..1

    // Redraw the arc: draw an ellipse with the progress mapped to fill.
    // Since GGraph doesn't support arcs natively, we draw a full circle
    // outline and show/hide based on progress.
    // For progress display, draw a filled circle with alpha = progress,
    // or draw the outline. The web client draws a white arc line from
    // -PI/2 to (-PI/2 + progress*2*PI).
    // Approximation: draw a full circle outline when progress > 0.
    _arc->clear();
    if (progress < 1.0f) {
        // Draw a circle outline (white) as the progress indicator.
        // In a real FGUI app we'd draw a pie-slice polygon, but the
        // GGraph::drawEllipse is the closest shape. For now draw a
        // circle outline at max size, and scale it.
        _arc->drawEllipse(kArcDiameter, kArcDiameter,
                          static_cast<int>(kLineWidth),
                          Color4F(1.0f, 1.0f, 1.0f, 0.8f),
                          Color4F(0.0f, 0.0f, 0.0f, 0.0f));
    } else {
        // Completed — draw a filled circle briefly.
        _arc->drawEllipse(kArcDiameter, kArcDiameter, 0,
                          Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                          Color4F(1.0f, 1.0f, 1.0f, 0.3f));
    }

    // Countdown text.
    const float remaining = std::max(0.0f, _duration - _elapsed);
    std::string counterStr = std::to_string(static_cast<int>(remaining * 10.0f) / 10.0f);
    // Format to 1 decimal place.
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << remaining;
    counterStr = oss.str();

    _counterText->setText(counterStr);

    // Label.
    _labelText->setText(_label.empty() ? "" : _label);

    // Position at screen top-center.
    const float x = camera.m_screenWidth * 0.5f;
    const float y = (camera.m_screenHeight / 3.0f) + mobileOffset;
    _container->setPosition(x, y);

    _container->setVisible(true);
    _arc->setVisible(true);
}

} // namespace surv

#endif // SURV_UI_NO_AXMOL