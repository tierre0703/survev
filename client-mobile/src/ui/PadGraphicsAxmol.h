#pragma once
// axmol implementation of the touch-pad renderer (Touch::PadGraphics).
//
// The web client draws each pad with the `pad.img` texture (a grey ring) and a
// white dot, both ordinary PIXI sprites on the stage. This port uses the same
// two sprites from `Content/gui/pad.png` (the bundled 4x copy of `pad.img`) on a
// dedicated axmol node. Plain axmol nodes have a stable ownership model (the
// scene retains its children), so the pad can be updated every frame; the
// previous FairyGUI GGraph widgets could not.
#include "Touch.h"

namespace ax {
class Node;
class Sprite;
class DrawNode;
class Vec2;
} // namespace ax

namespace surv {

class PadGraphicsAxmol : public PadGraphics {
public:
    explicit PadGraphicsAxmol(ax::Node* parent, bool isMovePad);
    ~PadGraphicsAxmol() override = default;

    void setVisible(bool visible) override;
    void setCenterPos(float x, float y) override;
    void setTouchPos(float x, float y) override;
    void setCenterScale(float s) override;
    void setTouchScale(float s) override;
    void update() override;

private:
    // Touch uses web screen coordinates (y down); axmol nodes are y up.
    ax::Vec2 toNodePos(float x, float y) const;

    ax::Node* _root = nullptr;
    ax::Sprite* _center = nullptr;
    ax::DrawNode* _dot = nullptr;
    bool _isMovePad = false;
    float _screenHeight = 720.0f;
};

} // namespace surv
