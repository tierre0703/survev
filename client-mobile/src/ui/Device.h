#pragma once
// Port of the parts of client/src/device.ts the native UI depends on.
//
// The native app is always a touch device in landscape, so the web client's
// detection becomes constants. `uiLayout` mirrors device.ts: the small layout
// is used on phones (and narrow screens), the large one on tablets/desktop.
#include <string>

namespace ui {

class Device {
public:
    enum Layout { Lg = 0, Sm = 1 };

    // Android is the only target; iOS would be the other mobile OS.
    const std::string os = "android";
    const bool mobile = true;
    const bool tablet = false;
    const bool touch = true;

    // Re-derived in `resize` (mirrors device.onResize()).
    bool isLandscape = true;
    float screenWidth = 0.0f;
    float screenHeight = 0.0f;
    float pixelRatio = 1.0f;
    Layout uiLayout = Sm;

    static Device& get();

    void resize(float width, float height, float pixelRatio_ = 1.0f) {
        screenWidth = width;
        screenHeight = height;
        pixelRatio = pixelRatio_;
        isLandscape = width > height;
        const float layoutDim = isLandscape ? screenWidth : screenHeight;
        uiLayout = (mobile || layoutDim <= 850.0f || (layoutDim <= 900.0f && pixelRatio >= 3.0f))
                       ? Sm
                       : Lg;
    }

    bool isSmallLayout() const { return uiLayout == Sm; }
};

} // namespace ui
