#pragma once
// Helper for stitching the HUD's FairyGUI component under the scene's GRoot.

#include "FguiOwnership.h"
#include "fairygui/GRoot.h"

namespace surv {
namespace fgui {

// Attaches an owning HUD component to the root: the component keeps its create()
// reference (caller retains it), and only its display object joins the root.
// GRoot.addChild() would release() the component itself.
inline void attachToRoot(fairygui::GRoot* root, fairygui::GComponent* component) {
    if (!root || !component || !root->displayObject()) {
        return;
    }
    ax::Node* display = component->displayObject();
    if (display && display->getParent() == nullptr) {
        root->displayObject()->addChild(display);
    }
}

} // namespace fgui
} // namespace surv
