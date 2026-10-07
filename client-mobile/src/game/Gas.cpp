#include "Gas.h"

#include "../net/Messages.h"
#include "../net/Net.h"
#include "../render/Camera.h"
#include <algorithm>
#include <cmath>

namespace surv {

namespace {
constexpr int kSegments = 512;
constexpr float kOverdraw = 100.0f * 1000.0f;

// gas.ts GasRenderer (non-canvas branch): a huge quad with a unit circle hole;
// the node is then scaled by the circle radius.
void buildGasGeometry(pix::Graphics* g, uint32_t gasColor) {
    g->clear();
    g->beginFill(gasColor, 0.6f);
    g->moveTo(-kOverdraw, -kOverdraw);
    g->lineTo(kOverdraw, -kOverdraw);
    g->lineTo(kOverdraw, kOverdraw);
    g->lineTo(-kOverdraw, kOverdraw);
    g->closePath();
    g->beginHole();
    g->moveTo(0.0f, 1.0f);
    for (int i = 1; i < kSegments; i++) {
        const float theta = static_cast<float>(i) / static_cast<float>(kSegments);
        g->lineTo(std::sin(3.14159265358979f * 2.0f * theta),
                  std::cos(3.14159265358979f * 2.0f * theta));
    }
    g->endHole();
    g->closePath();
    g->endFill();
}
} // namespace

void GasRenderer::m_init(pix::Factory* factory) {
    display = factory->createGraphics();
    buildGasGeometry(display, gasColor);
    display->setVisible(false);
}

void GasRenderer::m_free() {
    if (display) {
        display->setVisible(false);
    }
}

void GasRenderer::render(pix::Factory* factory, const Vec2& gasPos, float gasRad, bool isActive) {
    (void)factory;
    if (!display) {
        return;
    }
    Vec2 center = gasPos;
    float rad = gasRad;
    // Once the hole is small enough, fill the entire screen with a random part
    // of the geometry (gas.ts behaviour).
    if (rad < 0.1f) {
        rad = 1.0f;
        center.x += kOverdraw * 0.5f;
    }
    display->setPosition(center.x, center.y);
    display->setScale(rad, rad);
    display->setVisible(isActive);
}

void GasSafeZoneRenderer::m_init(pix::Factory* factory) {
    display = factory->createContainer();
    circleGfx = factory->createGraphics();
    lineGfx = factory->createGraphics();
    display->addChild(circleGfx);
    display->addChild(lineGfx);
    circleGfx->setVisible(false);
    lineGfx->setVisible(false);
}

void GasSafeZoneRenderer::render(const Vec2& safePos_, float safeRad_, const Vec2& playerPos_,
                                 bool drawCircle, bool drawLine) {
    if (!circleGfx || !lineGfx) {
        return;
    }
    circleGfx->setVisible(drawCircle);
    lineGfx->setVisible(drawLine);
    if (!drawCircle && !drawLine) {
        return;
    }
    const bool safePosChanged = !hasSafePos || !v2Eq(safePos, safePos_);
    const bool safeRadChanged = !hasSafeRad || std::fabs(safeRad - safeRad_) > 0.0001f;
    const bool playerPosChanged = !hasPlayerPos || !v2Eq(playerPos, playerPos_);
    safePos = safePos_;
    safeRad = safeRad_;
    playerPos = playerPos_;
    hasSafePos = hasSafeRad = hasPlayerPos = true;

    if (safePosChanged) {
        circleGfx->setPosition(safePos.x, safePos.y);
    }
    if (safeRadChanged) {
        circleGfx->clear();
        circleGfx->lineStyle(1.5f, 0xffffff);
        circleGfx->drawCircle(0.0f, 0.0f, safeRad);
    }
    if (safePosChanged || safeRadChanged || playerPosChanged) {
        const bool isSafe = v2Length(v2Sub(playerPos, safePos)) < safeRad;
        const float alpha = isSafe ? 0.5f : 1.0f;
        lineGfx->clear();
        lineGfx->lineStyle(2.0f, 0x00ff00, alpha);
        lineGfx->moveTo(playerPos.x, playerPos.y);
        lineGfx->lineTo(safePos.x, safePos.y);
    }
}

void Gas::m_init(pix::Factory* factory) {
    const float startRad = (std::sqrt(2.0f) + 0.01f) * Constants::MaxPosition;
    circleOld = {Vec2(), startRad};
    circleNew = {Vec2(), startRad};
    mode = GasMode_Inactive;
    circleT = circleTOld = 0.0f;
    duration = 0.0f;
    interpolationT = 0.0f;
    gasRenderer.m_init(factory);
    safeZone_.m_init(factory);
}

void Gas::m_free() { gasRenderer.m_free(); }

Gas::Circle Gas::getCircle(float interpT) const {
    const float t = mode == GasMode_Moving ? math::lerp(interpT, circleTOld, circleT) : 0.0f;
    return {v2Lerp(t, circleOld.pos, circleNew.pos),
            math::lerp(t, circleOld.rad, circleNew.rad)};
}

void Gas::setProgress(float circleT_) {
    circleTOld = circleT;
    circleT = circleT_;
    interpolationT = 0.0f;
}

void Gas::setFullState(float circleT_, const GasData& data) {
    mode = data.mode;
    duration = data.duration;
    setProgress(circleT_);
    circleOld.pos = data.posOld;
    circleOld.rad = data.radOld;
    circleNew.pos = data.posNew;
    circleNew.rad = data.radNew;
}

void Gas::m_render(pix::Factory* factory, float dt, const Camera& camera) {
    interpolationT += dt;
    float interpT = 1.0f;
    if (camera.m_interpEnabled && camera.m_interpInterval > 0.0f) {
        interpT = math::clamp(interpolationT / camera.m_interpInterval, 0.0f, 1.0f);
    }
    const Circle circle = getCircle(interpT);
    const Vec2 pos = camera.m_pointToScreen(circle.pos);
    const float scale = camera.m_scaleToScreen(circle.rad);
    gasRenderer.render(factory, pos, scale, isActive());
}

} // namespace surv
