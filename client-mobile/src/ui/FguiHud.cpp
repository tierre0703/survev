#ifdef SURV_UI_NO_AXMOL
// Host build without axmol/FGUI; stubs below are empty.
#else

#include "FguiHud.h"
#include "../game/Game.h"
#include "../render/Defs.h"
#include "../ui/Localization.h"

#include "fairygui/GRoot.h"
#include "fairygui/GComponent.h"
#include "fairygui/GProgressBar.h"
#include "fairygui/GLabel.h"
#include "fairygui/GObject.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <sstream>

USING_NS_AX;

namespace surv {

// ---------------------------------------------------------------------------
// FguiHud implementation — commented out to avoid null-create crashes.
// FairyGUI GTextField has no public create() macro; the HUD will be replaced
// by a real .fui package (UIPackage::createObject) in the next pass.
// For now, the C++ UiOverlay HUD remains active.
// ---------------------------------------------------------------------------

FguiHud::FguiHud() = default;

FguiHud::~FguiHud() = default;

void FguiHud::build(ax::Scene* scene) {
    // Stub — HUD will be loaded from a .fui package when available.
    // FairyGUI programmatic object creation is unreliable without the
    // package system. The C++ UiOverlay handles the HUD in the meantime.
}

void FguiHud::buildHudPanel(fairygui::GComponent* root) {
    // Stub — see build().
    (void)root;
}

void FguiHud::refreshHealth(float health) {
    (void)health;
}

void FguiHud::refreshBoost(float boost) {
    (void)boost;
}

void FguiHud::refreshWeapon(const std::string& weapon, int ammo, int reserve) {
    (void)weapon; (void)ammo; (void)reserve;
}

void FguiHud::refreshAlive(int count, int teamCount) {
    (void)count; (void)teamCount;
}

void FguiHud::refreshKillFeed(const std::string& text) {
    (void)text;
}

void FguiHud::refreshResult(const std::string& text) {
    (void)text;
}

void FguiHud::showResult(bool show) {
    (void)show;
}

void FguiHud::setVisible(bool visible) {
    _visible = visible;
}

bool FguiHud::isVisible() const {
    return _visible;
}

void FguiHud::update(float dt, bool started, bool playing, const Game* game) {
    // Stub — no-op until the .fui package is ready.
}

} // namespace surv

#endif // SURV_UI_NO_AXMOL