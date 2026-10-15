#pragma once
// Minimal layout constants kept for the host tests and remaining C++ code.
// The CSS colour parser moved to CssColor.* in S7 cleanup.
#include <string>

namespace ui {

namespace kit {
inline constexpr float kDesignWidth = 1280.0f;
inline constexpr float kDesignHeight = 720.0f;
inline float designWidth() { return kDesignWidth; }
inline float designHeight() { return kDesignHeight; }
inline float fromCssY(float cssY, float height = kDesignHeight) { return height - cssY; }
struct Vec2 { float x = 0.0f; float y = 0.0f; };
inline Vec2 fromCss(float cssX, float cssY, float height = kDesignHeight) {
    return Vec2{cssX, fromCssY(cssY, height)};
}
} // namespace kit

// The tag the menu scene puts on its scaled design-space UI root; the in-game
// overlay adopts it so HUD geometry and tap mapping match the menu.
inline constexpr int kUiRootTag = 0x5A17;

} // namespace ui