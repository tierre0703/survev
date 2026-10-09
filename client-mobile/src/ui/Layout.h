#pragma once
// Engine-independent layout/colour constants shared by the axmol UI toolkit
// (`Ui.h`) and the host tests. `Ui.h` needs axmol for the widget classes, but
// the geometry (the 1280x720 design canvas) and the CSS colour parser are pure
// logic, so they live here and stay linkable without the engine.
//
// IMPORTANT COORDINATES: the web UI lays out in CSS pixels with y growing
// *down*, but axmol nodes use y growing *up*. `kit::fromCssY()` flips them so
// callers can keep authoring geometry exactly like the DOM.
#if defined(SURV_UI_NO_AXMOL)
// Host-test build: no axmol headers. Provide the few tiny types used by the
// colour helpers so this header stays engine-free.
#else
#include "axmol.h"
#endif

#include <string>

#if defined(SURV_UI_NO_AXMOL)
struct SurvUiVec2 {
    float x = 0.0f;
    float y = 0.0f;
};
#endif

namespace ui {

namespace kit {
// The screen is designed at a fixed "CSS" resolution (like the web client's
// window); the scene root is scaled to the real display. All UI geometry is
// authored in these units.
inline constexpr float kDesignWidth = 1280.0f;
inline constexpr float kDesignHeight = 720.0f;

inline float designWidth() { return kDesignWidth; }
inline float designHeight() { return kDesignHeight; }
// CSS y (0 = top) -> axmol y (0 = bottom).
inline float fromCssY(float cssY, float height = kDesignHeight) { return height - cssY; }
#if defined(SURV_UI_NO_AXMOL)
struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};
inline Vec2 fromCss(float cssX, float cssY, float height = kDesignHeight) {
    return Vec2{cssX, fromCssY(cssY, height)};
}
#else
inline ax::Vec2 fromCss(float cssX, float cssY, float height = kDesignHeight) {
    return ax::Vec2(cssX, fromCssY(cssY, height));
}
#endif
} // namespace kit

// Engine-free 8-bit colour value, used by the CSS parser. The toolkit converts
// these to `ax::Color3B`/`ax::Color4B` at the axmol boundary.
struct Rgb {
    unsigned char r = 255, g = 255, b = 255;
};
struct Rgba {
    unsigned char r = 255, g = 255, b = 255, a = 255;
};

// CSS colour string ("#rrggbb", "#rgb", "#rrggbbaa", "rgb(a,b,c)") -> bytes.
// On parse failure the `fallback` is returned unchanged.
Rgb parseHexColorRgb(const std::string& css, const Rgb& fallback = Rgb{255, 255, 255});
Rgba parseHexColorRgba(const std::string& css, const Rgba& fallback = Rgba{255, 255, 255, 255});

// Tag the menu scene puts on its scaled design-space UI root; the in-game
// overlay adopts it so HUD geometry and tap mapping match the menu.
inline constexpr int kUiRootTag = 0x5A17;

} // namespace ui
