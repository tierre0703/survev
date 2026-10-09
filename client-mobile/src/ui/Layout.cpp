#include "Layout.h"

#include <cctype>
#include <cstdint>
#include <vector>

namespace ui {

Rgb parseHexColorRgb(const std::string& css, const Rgb& fallback) {
    float r = 0, g = 0, b = 0, a = 1;
    if (!parseCss(css, r, g, b, a)) {
        return fallback;
    }
    return Rgb{toByte(r), toByte(g), toByte(b)};
}

Rgba parseHexColorRgba(const std::string& css, const Rgba& fallback) {
    float r = 0, g = 0, b = 0, a = 1;
    if (!parseCss(css, r, g, b, a)) {
        return fallback;
    }
    return Rgba{toByte(r), toByte(g), toByte(b), toByte(a)};
}

} // namespace ui
