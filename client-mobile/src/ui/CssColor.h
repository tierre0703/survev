#pragma once
// Engine-free CSS colour parser (extracted from Layout.cpp in S7 cleanup).
// The widget toolkit `Ui.*` is deleted; only the colour parser is kept because
// the host tests still exercise it.
#include <cstdint>
#include <string>

namespace ui {

struct Rgb {
    unsigned char r = 255, g = 255, b = 255;
};
struct Rgba {
    unsigned char r = 255, g = 255, b = 255, a = 255;
};

Rgb parseHexColorRgb(const std::string& css, const Rgb& fallback = Rgb{255, 255, 255});
Rgba parseHexColorRgba(const std::string& css, const Rgba& fallback = Rgba{255, 255, 255, 255});

} // namespace ui