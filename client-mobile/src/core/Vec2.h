#pragma once
#include <cmath>

namespace surv {

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    Vec2() = default;
    Vec2(float x_, float y_) : x(x_), y(y_) {}
};

inline Vec2 v2Create(float x, float y = 0.0f) {
    return Vec2(x, y == 0.0f ? x : y);
}

inline Vec2 v2Copy(const Vec2& a) {
    return Vec2(a.x, a.y);
}

inline Vec2 v2Add(const Vec2& a, const Vec2& b) {
    return Vec2(a.x + b.x, a.y + b.y);
}

inline Vec2 v2Sub(const Vec2& a, const Vec2& b) {
    return Vec2(a.x - b.x, a.y - b.y);
}

inline Vec2 v2Mul(const Vec2& a, float s) {
    return Vec2(a.x * s, a.y * s);
}

inline Vec2 v2Div(const Vec2& a, float s) {
    return Vec2(a.x / s, a.y / s);
}

inline Vec2 v2Neg(const Vec2& a) {
    return Vec2(-a.x, -a.y);
}

inline float v2LengthSqr(const Vec2& a) {
    return a.x * a.x + a.y * a.y;
}

inline float v2Length(const Vec2& a) {
    return std::sqrt(v2LengthSqr(a));
}

inline Vec2 v2Normalize(const Vec2& a) {
    constexpr float eps = 0.000001f;
    const float len = v2Length(a);
    return Vec2(len > eps ? a.x / len : a.x, len > eps ? a.y / len : a.y);
}

inline Vec2 v2NormalizeSafe(const Vec2& a, const Vec2& v = Vec2(1.0f, 0.0f)) {
    constexpr float eps = 0.000001f;
    const float len = v2Length(a);
    return Vec2(len > eps ? a.x / len : v.x, len > eps ? a.y / len : v.y);
}

inline float v2Distance(const Vec2& a, const Vec2& b) {
    return v2Length(v2Sub(a, b));
}

inline float v2ManhattanDistance(const Vec2& a, const Vec2& b) {
    return std::fabs(a.x - b.x) + std::fabs(a.y - b.y);
}

inline Vec2 v2Midpoint(const Vec2& a, const Vec2& b) {
    return Vec2((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f);
}

inline Vec2 v2DirectionNormalized(const Vec2& a, const Vec2& b) {
    return v2Normalize(v2Sub(b, a));
}

inline float v2Dot(const Vec2& a, const Vec2& b) {
    return a.x * b.x + a.y * b.y;
}

inline Vec2 v2Perp(const Vec2& a) {
    return Vec2(-a.y, a.x);
}

inline Vec2 v2Proj(const Vec2& a, const Vec2& b) {
    return v2Mul(b, v2Dot(a, b) / v2Dot(b, b));
}

inline Vec2 v2Rotate(const Vec2& a, float rad) {
    const float cosr = std::cos(rad);
    const float sinr = std::sin(rad);
    return Vec2(a.x * cosr - a.y * sinr, a.x * sinr + a.y * cosr);
}

inline Vec2 v2MulElems(const Vec2& a, const Vec2& b) {
    return Vec2(a.x * b.x, a.y * b.y);
}

inline Vec2 v2DivElems(const Vec2& a, const Vec2& b) {
    return Vec2(a.x / b.x, a.y / b.y);
}

inline Vec2 v2MinElems(const Vec2& a, const Vec2& b) {
    return Vec2(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y);
}

inline Vec2 v2MaxElems(const Vec2& a, const Vec2& b) {
    return Vec2(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y);
}

inline Vec2 v2RandomUnit(float length = 1.0f, float (*rand)() = nullptr) {
    const float angle = 2.0f * (rand ? rand() : 0.0f) * 3.14159265358979f;
    return v2Rotate(Vec2(length, 0.0f), angle);
}

inline Vec2 v2Lerp(float t, const Vec2& a, const Vec2& b) {
    return v2Add(v2Mul(a, 1.0f - t), v2Mul(b, t));
}

inline bool v2Eq(const Vec2& a, const Vec2& b, float epsilon = 0.0001f) {
    return std::fabs(a.x - b.x) <= epsilon && std::fabs(a.y - b.y) <= epsilon;
}

} // namespace surv