#pragma once
// FairyGUI-based pie timer (action progress arc): port of client/src/ui/pieTimer.ts.
// Renders a countdown arc + label + duration text for reload/use/revive actions.
#ifndef SURV_FGUI_PIE_TIMER_H
#define SURV_FGUI_PIE_TIMER_H

#include <string>

#include "core/Vec2.h"

namespace ax {
class Scene;
}

namespace fairygui {
class GBasicTextField;
class GComponent;
class GGraph;
}

namespace surv {
class Camera;

class PieTimer {
public:
    PieTimer();
    ~PieTimer();

    // Builds FGUI objects under the root.
    void build();

    // Start showing a timer for the given action.
    void start(const std::string& label, float elapsed, float duration);

    // Hide the timer.
    void stop();

    // pieTimer.ts resize(): re-derives the web layout state — on the small
    // layout the timer is drawn at half scale (unless `tablet`) and drops
    // `mobileOffset` (touch.mobileOffsetLandscape/Portrait) below the
    // (screenHeight / 3) * screenScaleFactor anchor.
    void resize(bool smallLayout, bool tablet, bool landscape, float screenScaleFactor);

    // Per-frame update. `camera` provides screen dimensions for positioning.
    void update(float dt, const Camera& camera);

    bool isActive() const { return _active; }

private:
    void setVisible(bool visible);

    // Whole timer container (to position as a group).
    fairygui::GComponent* _container = nullptr;

    // Arc graph showing progress.
    fairygui::GGraph* _arc = nullptr;

    // Countdown text (e.g. "2.5")
    fairygui::GBasicTextField* _counterText = nullptr;

    // Label text (e.g. "Reviving")
    fairygui::GBasicTextField* _labelText = nullptr;

    bool _active = false;
    std::string _label;
    float _elapsed = 0.0f;
    float _duration = 1.0f;

    // Web pieTimer.ts layout state (see resize()).
    bool _smallLayout = true;
    bool _tablet = false;
    bool _landscape = true;
    float _screenScaleFactor = 1.0f;
    float _halfScale = 1.0f;   // container.scale.set(0.5) on phones
    float _mobileOffset = 0.0f;
};

} // namespace surv

#endif // SURV_FGUI_PIE_TIMER_H