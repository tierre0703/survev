#pragma once
// Port of client/src/camera.ts. Engine-independent (pure view transform), so
// it is covered by the host tests.
#include "../core/MathUtil.h"
#include "../core/Vec2.h"
#include <cmath>
#include <cstdlib>

namespace surv {

class Camera {
public:
    Vec2 m_pos;
    float m_ppu = 16.0f;
    float m_zoom = 1.5f;
    float m_targetZoom = 1.5f;
    float m_screenWidth = 1.0f;
    float m_screenHeight = 1.0f;
    bool m_shakeEnabled = true;
    float m_shakeInt = 0.0f;

    bool m_interpEnabled = true;
    bool m_localRotationEnabled = false;
    float m_interpInterval = 0.0f;

    float m_z() const { return m_ppu * m_zoom; }

    Vec2 m_pointToScreen(const Vec2& point) const {
        return Vec2(m_screenWidth * 0.5f + (point.x - m_pos.x) * m_z(),
                    m_screenHeight * 0.5f - (point.y - m_pos.y) * m_z());
    }

    Vec2 m_screenToPoint(const Vec2& screen) const {
        return Vec2(m_pos.x + (screen.x - m_screenWidth * 0.5f) / m_z(),
                    m_pos.y + (m_screenHeight * 0.5f - screen.y) / m_z());
    }

    float m_pixels(float p) const { return p * m_zoom; }

    float m_scaleToScreen(float s) const { return s * m_z(); }

    void m_setShakeEnabled(bool en) { m_shakeEnabled = en; }
    void m_setInterpEnabled(bool en) { m_interpEnabled = en; }
    void m_setRotationEnabled(bool en) { m_localRotationEnabled = en; }

    void m_addShake(const Vec2& pos, float intensity) {
        const float dist = v2Length(v2Sub(m_pos, pos));
        const float newInt = math::delerp(dist, 40.0f, 10.0f) * intensity;
        m_shakeInt = math::max(m_shakeInt, newInt);
    }

    void m_applyShake() {
        if (m_shakeEnabled) {
            const float angle = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) *
                                2.0f * 3.14159265358979f;
            m_pos = v2Add(m_pos, Vec2(std::cos(angle) * m_shakeInt, std::sin(angle) * m_shakeInt));
        }
        m_shakeInt = 0.0f;
    }
};

} // namespace surv
