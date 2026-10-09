#include "Layout.h"

#include <cctype>
#include <cstdint>
#include <vector>

namespace ui {

namespace {

int hexVal(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool parseCss(const std::string& css, float& r, float& g, float& b, float& a) {
    std::string s = css;
    // Trim whitespace.
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
    if (s.rfind("rgb(", 0) == 0 || s.rfind("rgba(", 0) == 0) {
        const bool hasAlpha = s[3] == 'a';
        const std::size_t start = hasAlpha ? 5 : 4;
        const std::size_t end = s.find(')');
        const std::string inner = s.substr(start, end == std::string::npos ? std::string::npos : end - start);
        std::vector<float> parts;
        std::string cur;
        for (char c : inner + ",") {
            if (c == ',') {
                if (!cur.empty()) {
                    parts.push_back(std::stof(cur));
                    cur.clear();
                }
            } else {
                cur.push_back(c);
            }
        }
        if (parts.size() < 3) {
            return false;
        }
        r = parts[0] / 255.0f;
        g = parts[1] / 255.0f;
        b = parts[2] / 255.0f;
        a = parts.size() > 3 ? parts[3] : 1.0f;
        return true;
    }
    if (s.empty() || s[0] != '#') {
        return false;
    }
    s.erase(s.begin());
    if (s.size() == 3 || s.size() == 4) {
        std::string expanded;
        for (char c : s) {
            expanded.push_back(c);
            expanded.push_back(c);
        }
        s = expanded;
    }
    if (s.size() != 6 && s.size() != 8) {
        return false;
    }
    const int rr = hexVal(s[0]) * 16 + hexVal(s[1]);
    const int gg = hexVal(s[2]) * 16 + hexVal(s[3]);
    const int bb = hexVal(s[4]) * 16 + hexVal(s[5]);
    const int aa = s.size() == 8 ? hexVal(s[6]) * 16 + hexVal(s[7]) : 255;
    if (rr < 0 || gg < 0 || bb < 0 || aa < 0) {
        return false;
    }
    r = rr / 255.0f;
    g = gg / 255.0f;
    b = bb / 255.0f;
    a = aa / 255.0f;
    return true;
}

unsigned char toByte(float v) {
    return static_cast<unsigned char>(v * 255.0f + 0.5f);
}

} // namespace

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
