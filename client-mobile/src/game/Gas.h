#pragma once
// Port of client/src/gas.ts: the gas/safe-zone overlay. The overlay is a
// screen-covering quad with a circular hole punched at the (interpolated) gas
// centre; when the safe radius gets small the hole is dropped and the overlay
// covers the whole screen.
#include "../core/GameConfig.h"
#include "../core/MathUtil.h"
#include "../core/Vec2.h"
#include "../render/PixiLike.h"
#include <cmath>

namespace surv {

class Camera;
struct GasData;

// GasRenderer (gas.ts). The non-canvas PIXI.Graphics branch.
class GasRenderer {
public:
    pix::Graphics* display = nullptr;
    bool active = false;
    uint32_t gasColor = 16711680;

    void m_init(pix::Factory* factory);
    void m_free();
    void render(pix::Factory* factory, const Vec2& gasPos, float gasRad, bool active, float zoom);
};

class GasSafeZoneRenderer {
public:
    pix::Container* display = nullptr;
    pix::Graphics* circleGfx = nullptr;
    pix::Graphics* lineGfx = nullptr;
    Vec2 safePos;
    float safeRad = 0.0f;
    Vec2 playerPos;
    bool hasSafePos = false;
    bool hasSafeRad = false;
    bool hasPlayerPos = false;

    void m_init(pix::Factory* factory);
    void render(const Vec2& safePos_, float safeRad_, const Vec2& playerPos_, bool drawCircle,
                bool drawLine);
};

class Gas {
public:
    uint8_t mode = GasMode_Inactive;
    float circleT = 0.0f;
    float circleTOld = 0.0f;
    float duration = 0.0f;
    float interpolationT = 0.0f;

    GasRenderer gasRenderer;
    struct Circle {
        Vec2 pos;
        float rad = 0.0f;
    };
    Circle circleOld;
    Circle circleNew;

    void m_init(pix::Factory* factory);
    void m_free();
    bool isActive() const { return mode != GasMode_Inactive; }
    Circle getCircle(float interpT) const;
    void setProgress(float circleT_);
    void setFullState(float circleT_, const GasData& data);
    void m_render(pix::Factory* factory, float dt, const Camera& camera);

    // Kept private-ish: the safe zone renderer is a UI-only feature; the port
    // still tracks it so the renderer can be extended without another pass.
    GasSafeZoneRenderer& safeZone() { return safeZone_; }

private:
    GasSafeZoneRenderer safeZone_;
};

} // namespace surv
