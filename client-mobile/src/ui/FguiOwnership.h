#pragma once
// FairyGUI object ownership helpers.
//
// FairyGUI objects are ref-counted (CREATE_FUNC -> autorelease) and a
// GComponent does NOT release its children, while GRoot::addChild() *does*
// release the object it is given. A widget created with create() and added to a
// parent must therefore balance that reference itself. Doing it in the wrong
// order (as the HUD/pads/wheel did) released the object inside addChild, so the
// tree died in the frame that built it - the GObject::setVisible() crash.
#ifndef SURV_FGUI_OWNERSHIP_H
#define SURV_FGUI_OWNERSHIP_H

#include "fairygui/GComponent.h"
#include "fairygui/GObject.h"

namespace surv {
namespace fgui {

// Adds `child` to `parent` and owns it (the addChild reference).
inline void adopt(fairygui::GComponent* parent, fairygui::GObject* child) {
    if (!parent || !child) {
        return;
    }
    child->retain();
    parent->addChild(child);
    child->release();
}

// Drops the extra owning reference taken by created() once a child has been
// adopted. The widget may be gone at this point (it was only held by that
// reference), so the pointer must not be touched again.
inline void releaseCreated(fairygui::GObject* child) {
    if (child) {
        child->release();
    }
}

// Takes an owning reference to a freshly created widget (its create() reference
// is autoreleased). The HUD's builders hand raw pointers to adopt() below
// without retaining first, so the object must be kept alive from here until it
// is added to its parent or explicitly released.
template <class T>
inline T* created(T* obj) {
    if (obj) {
        obj->retain();
    }
    return obj;
}

} // namespace fgui
} // namespace surv

#endif // SURV_FGUI_OWNERSHIP_H
