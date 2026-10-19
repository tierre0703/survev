#pragma once
// FairyGUI-based touch pad graphics (M7/S2): replaces the axmol Sprite-based
// TouchPadGfx with programmatic FairyGUI GGraph/GObject components.
//
// This is an incremental step: the pads are built from code rather than an
// authored .fui package. Once Touch_pkg exists in the FairyGUI Editor, this
// file is replaced by loading the package.
#include "../ui/Touch.h"

namespace ax {
class Scene;
}

namespace fairygui {
class GComponent;
class GGraph;
}

namespace surv {

// FairyGUI implementation of PadGraphics: renders the joystick rings using
// FGUI objects (GGraph circles) instead of axmol sprites.
class FguiPadGraphics : public PadGraphics {
public:
    explicit FguiPadGraphics(ax::Scene* scene, bool isMovePad);
    ~FguiPadGraphics() override;

    void setVisible(bool visible) override;
    void setCenterPos(float x, float y) override;
    void setTouchPos(float x, float y) override;
    void setCenterScale(float s) override;
    void setTouchScale(float s) override;
    void update() override;

private:
    void ensureFx(); // deferred creation of FairyGUI widgets
    ax::Scene* _scene = nullptr;
    fairygui::GComponent* _parent = nullptr;
    fairygui::GGraph* _centerRing = nullptr;
    fairygui::GGraph* _touchDot = nullptr;
    bool _isMovePad = false;
    float _centerScale = 1.0f;
    float _touchScale = 1.0f;
};

} // namespace surv