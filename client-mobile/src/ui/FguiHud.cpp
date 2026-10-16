#ifdef SURV_UI_NO_AXMOL
// Host build without axmol/FGUI; stubs below are empty.
#else

#include "FguiHud.h"
#include "../game/Game.h"
#include "../render/Defs.h"
#include "../ui/Localization.h"
#include "../core/GameConfig.h"

#include "fairygui/GRoot.h"
#include "fairygui/GComponent.h"
#include "fairygui/GGraph.h"
#include "fairygui/GTextField.h"
#include "fairygui/GObject.h"
#include "fairygui/GButton.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <sstream>

USING_NS_AX;

namespace surv {

// ---------------------------------------------------------------------------
// FguiHud — FairyGUI-based HUD for health/boost bars, weapon/ammo text,
// alive count, kill feed, and game result overlay.
//
// Uses GGraph for bar rendering (rect fills) and GBasicTextField for text,
// since GLabel/GProgressBar require .fui package children to function.
// ---------------------------------------------------------------------------

// Layout constants (pixels in FG design-space, scaled by GRoot).
static constexpr float kBarWidth = 240.0f;
static constexpr float kBarHeight = 16.0f;
static constexpr float kPanelPadding = 10.0f;
static constexpr float kPanelWidth = kBarWidth + 160.0f;  // bar + label margin + text slots
static constexpr float kPanelHeight = 100.0f;             // bars + weapon/ammo + alive

// Boost breakpoints from shared/gameConfig.ts: player.boostBreakpoints = [1, 1, 1.5, 0.5]
static constexpr int kBoostBarCount = 4;
static constexpr float kBoostBreakpoints[kBoostBarCount] = {1.0f, 1.0f, 1.5f, 0.5f};
static constexpr float kBoostBreakpointsTotal = 4.0f;

// Font sizes
static constexpr float kFontSize = 18.0f;
static constexpr float kSmallFontSize = 14.0f;

// Kill feed: max lines, line height, fade time
static constexpr int kKillFeedMaxLines = 6;
static constexpr float kKillFeedLineH = 20.0f;
static constexpr float kKillFeedWidth = 300.0f;
static constexpr float kKillFeedHeight = kKillFeedMaxLines * kKillFeedLineH + 10.0f;

// Result overlay
static constexpr float kResultWidth = 400.0f;
static constexpr float kResultHeight = 300.0f;

// Health colour steps (port of ui2.ts steps arrays for color interpolation).
struct HealthStep { float health; uint8_t r, g, b; };
static const HealthStep kHealthSteps[] = {
    {100, 179, 179, 179},  // grey
    {100, 255, 255, 255},  // white
    { 75, 255, 255, 255},  // white
    { 75, 255, 158, 158},  // light pink
    { 25, 255,  82,  82},  // medium red
    { 25, 255,   0,   0},  // pure red
    {  0, 255,   0,   0},
    {  0, 255,   0,   0},
};

// English-only translate helper (mirrors UiOverlay.cpp tr()).
// Full localization is in Localization.h; for HUD labels we fallback to English.
static std::string tr(const std::string& fallback) {
    return fallback;
}

FguiHud::FguiHud() = default;

FguiHud::~FguiHud() = default;

// ---- Health colour interpolation -----------------------------------------
static uint32_t healthColor(float health, bool downed) {
    if (downed) return 0xffff0000;
    const int nSteps = sizeof(kHealthSteps) / sizeof(kHealthSteps[0]);
    int endIdx = 0;
    const int h = static_cast<int>(std::ceil(health));
    while (endIdx < nSteps - 1 && kHealthSteps[endIdx].health > h) ++endIdx;
    const int aIdx = std::max(endIdx - 1, 0);
    const auto& stepA = kHealthSteps[aIdx];
    const auto& stepB = kHealthSteps[endIdx];
    const float t = (stepB.health - stepA.health) > 0.0f
        ? (health - stepA.health) / (stepB.health - stepA.health)
        : 0.0f;
    const float tClamped = std::clamp(t, 0.0f, 1.0f);
    uint8_t r = static_cast<uint8_t>(std::lerp(tClamped, stepA.r, stepB.r));
    uint8_t g = static_cast<uint8_t>(std::lerp(tClamped, stepA.g, stepB.g));
    uint8_t b = static_cast<uint8_t>(std::lerp(tClamped, stepA.b, stepB.b));
    return 0xff000000 | (static_cast<uint32_t>(r) << 16) |
           (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
}

void FguiHud::build(ax::Scene* scene) {
    fairygui::GRoot* root = fairygui::GRoot::getInstance();
    if (!root) return;

    buildHudPanel(root);
    buildKillFeed(root);
    buildAliveCounter(root);
    buildResultOverlay(root);
}

void FguiHud::buildHudPanel(fairygui::GComponent* root) {
    // Panel background: semi-transparent black rect behind the HUD.
    fairygui::GGraph* panelBg = fairygui::GGraph::create();
    panelBg->drawRect(kPanelWidth, kPanelHeight, 0,
                      Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                      Color4F(0.0f, 0.0f, 0.0f, 0.35f));
    panelBg->setPosition(0.0f, 0.0f);
    panelBg->setPivot(0.0f, 0.0f);
    _hudPanel = panelBg;

    // --- Health bar ---
    // Background (depleted colour, #929292)
    fairygui::GGraph* healthBg = fairygui::GGraph::create();
    healthBg->drawRect(kBarWidth, kBarHeight, 0,
                       Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                       Color4F(0.57f, 0.57f, 0.57f, 1.0f));
    healthBg->setPosition(kPanelPadding, kPanelHeight - kBarHeight - kPanelPadding);
    healthBg->setPivot(0.0f, 0.0f);
    healthBg->setVisible(true);
    panelBg->addChild(healthBg);

    // Foreground fill (redrawn per frame with dynamic width + interpolated color)
    fairygui::GGraph* healthFill = fairygui::GGraph::create();
    healthFill->drawRect(kBarWidth, kBarHeight, 0,
                         Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                         Color4F(0.85f, 0.2f, 0.2f, 1.0f));
    healthFill->setPosition(kPanelPadding, kPanelHeight - kBarHeight - kPanelPadding);
    healthFill->setPivot(0.0f, 0.0f);
    healthFill->setVisible(true);
    panelBg->addChild(healthFill);
    _healthFill = healthFill;

    // Health numeric text
    fairygui::GBasicTextField* healthTf = fairygui::GBasicTextField::create();
    healthTf->setText("");
    healthTf->setColor(Color3B(255, 255, 255));
    healthTf->setFontSize(kFontSize);
    healthTf->setSingleLine(true);
    healthTf->setPosition(kPanelPadding + kBarWidth + 4.0f,
                          kPanelHeight - kBarHeight - kPanelPadding - 2.0f);
    healthTf->setPivot(0.0f, 0.0f);
    panelBg->addChild(healthTf);
    _healthText = healthTf;

    // --- Boost bar ---
    // Background
    const float boostBarY = kPanelPadding + kBarHeight + 4.0f;
    fairygui::GGraph* boostBg = fairygui::GGraph::create();
    boostBg->drawRect(kBarWidth, kBarHeight, 0,
                      Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                      Color4F(0.15f, 0.15f, 0.15f, 0.9f));
    boostBg->setPosition(kPanelPadding, boostBarY);
    boostBg->setPivot(0.0f, 0.0f);
    panelBg->addChild(boostBg);

    // 4 boost segment fills
    static const uint32_t boostColors[] = {
        0xffff901a, 0xffff751a, 0xffff6616, 0xffff5600
    };
    for (int i = 0; i < kBoostBarCount; ++i) {
        const float segmentWidth = kBarWidth * (kBoostBreakpoints[i] / kBoostBreakpointsTotal);
        fairygui::GGraph* seg = fairygui::GGraph::create();
        const uint32_t c = boostColors[i];
        seg->drawRect(segmentWidth, kBarHeight, 0,
                      Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                      Color4F(((c >> 16) & 0xff) / 255.0f,
                              ((c >> 8) & 0xff) / 255.0f,
                              (c & 0xff) / 255.0f,
                              1.0f));
        seg->setPosition(kPanelPadding, boostBarY);
        seg->setPivot(0.0f, 0.0f);
        seg->setVisible(false);
        panelBg->addChild(seg);
        _boostSegments[i] = seg;
    }

    // --- Weapon + Ammo text ---
    // Weapon name label (top area, small font)
    fairygui::GBasicTextField* weaponTf = fairygui::GBasicTextField::create();
    weaponTf->setText("");
    weaponTf->setColor(Color3B(220, 220, 220));
    weaponTf->setFontSize(kSmallFontSize);
    weaponTf->setSingleLine(true);
    weaponTf->setPosition(kPanelPadding + kBarWidth + 4.0f,
                          boostBarY + kBarHeight + 2.0f);
    weaponTf->setPivot(0.0f, 0.0f);
    panelBg->addChild(weaponTf);
    _weaponText = weaponTf;

    // Ammo count (clip / reserve)
    fairygui::GBasicTextField* ammoTf = fairygui::GBasicTextField::create();
    ammoTf->setText("");
    ammoTf->setColor(Color3B(255, 255, 255));
    ammoTf->setFontSize(kFontSize);
    ammoTf->setSingleLine(true);
    ammoTf->setPosition(kPanelPadding, boostBarY + kBarHeight + 2.0f);
    ammoTf->setPivot(0.0f, 0.0f);
    panelBg->addChild(ammoTf);
    _ammoText = ammoTf;

    // Position the entire HUD panel at bottom-left.
    panelBg->setPivot(0.0f, 1.0f);
    panelBg->setPosition(10.0f, -10.0f);
    root->addChild(panelBg);
}

void FguiHud::buildAliveCounter(fairygui::GComponent* root) {
    // Alive count: shown top-right
    fairygui::GBasicTextField* aliveTf = fairygui::GBasicTextField::create();
    aliveTf->setText("");
    aliveTf->setColor(Color3B(255, 255, 255));
    aliveTf->setFontSize(22.0f);
    aliveTf->setSingleLine(true);
    aliveTf->setPosition(-160.0f, 10.0f);  // top-right area
    aliveTf->setPivot(1.0f, 0.0f);          // right-aligned
    root->addChild(aliveTf);
    _aliveText = aliveTf;
}

void FguiHud::buildKillFeed(fairygui::GComponent* root) {
    // Kill feed: panel in top-right area below alive counter
    fairygui::GBasicTextField* killTf = fairygui::GBasicTextField::create();
    killTf->setText("");
    killTf->setColor(Color3B(255, 200, 150));
    killTf->setFontSize(kSmallFontSize);
    killTf->setSingleLine(false);
    killTf->setPosition(-160.0f, 40.0f);
    killTf->setPivot(1.0f, 0.0f);  // right-aligned
    root->addChild(killTf);
    _killFeedText = killTf;
}

void FguiHud::buildResultOverlay(fairygui::GComponent* root) {
    // Result overlay: centered panel, hidden initially
    fairygui::GGraph* panel = fairygui::GGraph::create();
    panel->drawRect(kResultWidth, kResultHeight, 2,
                    Color4F(1.0f, 1.0f, 1.0f, 1.0f),
                    Color4F(0.0f, 0.0f, 0.0f, 0.85f));
    panel->setPivot(0.5f, 0.5f);
    panel->setPosition(0.0f, 0.0f);
    panel->setVisible(false);
    root->addChild(panel);
    _resultPanel = panel;

    // Result text
    fairygui::GBasicTextField* resultTf = fairygui::GBasicTextField::create();
    resultTf->setText("");
    resultTf->setColor(Color3B(255, 220, 100));
    resultTf->setFontSize(24.0f);
    resultTf->setSingleLine(false);
    resultTf->setPosition(-kResultWidth / 2.0f + 20.0f, -kResultHeight / 2.0f + 30.0f);
    resultTf->setPivot(0.0f, 0.0f);
    resultTf->setVisible(false);
    panel->addChild(resultTf);
    _resultText = resultTf;
}

// ---- Refresh methods -----------------------------------------------------

void FguiHud::refreshHealth(float health) {
    if (!_healthFill || !_healthText || !_hudPanel) return;

    const bool downed = health < 0.0f;
    if (downed) health = 0.0f;
    health = std::clamp(health, 0.0f, 100.0f);
    const float fraction = health / 100.0f;
    const float fillWidth = kBarWidth * fraction;

    // Redraw the fill rect with correct width and interpolated color.
    uint32_t color = healthColor(health, downed);
    _healthFill->clear();
    _healthFill->drawRect(std::max(fillWidth, 0.0f), kBarHeight, 0,
                          Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                          Color4F(((color >> 16) & 0xff) / 255.0f,
                                  ((color >> 8) & 0xff) / 255.0f,
                                  (color & 0xff) / 255.0f,
                                  1.0f));
    _healthFill->setVisible(fillWidth > 0.0f);

    // Update health text
    std::string text = std::to_string(static_cast<int>(std::round(health)));
    _healthText->setText(text);
}

void FguiHud::refreshBoost(float boost) {
    if (!_hudPanel) return;

    boost = std::clamp(boost, 0.0f, 100.0f);
    float boostT = boost / 100.0f;

    for (int i = 0; i < kBoostBarCount; ++i) {
        const float breakPointT = kBoostBreakpoints[i] / kBoostBreakpointsTotal;
        const float widthT = std::clamp(boostT / breakPointT, 0.0f, 1.0f);
        boostT = std::max(boostT - breakPointT, 0.0f);

        if (_boostSegments[i]) {
            _boostSegments[i]->setVisible(widthT > 0.001f);
        }
    }
}

void FguiHud::refreshWeapon(const std::string& weapon, int ammo, int reserve) {
    if (!_weaponText || !_ammoText || !_hudPanel) return;

    // Weapon name (mirrors UiOverlay.cpp logic)
    if (!weapon.empty()) {
        _weaponText->setText(weapon);
    } else {
        _weaponText->setText("");
    }

    // Ammo: "clip / reserve" for guns, just "clip" for others, "" for melee
    if (ammo >= 0) {
        std::string ammoStr = std::to_string(ammo);
        if (reserve >= 0) {
            ammoStr += " / " + std::to_string(reserve);
        }
        _ammoText->setText(ammoStr);
    } else {
        _ammoText->setText("");
    }
}

void FguiHud::refreshAlive(int count, int teamCount) {
    if (!_aliveText) return;

    if (teamCount >= 2) {
        // Faction mode or squad: show as "N alive" or faction counts
        _aliveText->setText(std::to_string(count) + " " + tr("alive"));
    } else {
        _aliveText->setText(std::to_string(count) + " " + tr("alive"));
    }
}

void FguiHud::refreshKillFeed(const std::string& text) {
    if (!_killFeedText) return;

    if (!text.empty()) {
        _killFeedText->setText(text);
    } else {
        _killFeedText->setText("");
    }
}

void FguiHud::refreshResult(const std::string& text) {
    if (!_resultText) return;
    _resultText->setText(text);
}

void FguiHud::showResult(bool show) {
    if (_resultPanel) {
        _resultPanel->setVisible(show);
    }
    if (_resultText) {
        _resultText->setVisible(show);
    }
    _resultShown = show;
}

void FguiHud::setVisible(bool visible) {
    _visible = visible;
    if (_hudPanel) {
        _hudPanel->setVisible(visible);
    }
}

bool FguiHud::isVisible() const {
    return _visible;
}

void FguiHud::update(float dt, bool started, bool playing, const Game* game) {
    if (!_hudPanel || !_visible) return;
    if (!started || !playing || !game) return;

    const GameStateSnapshot snap = game->snapshot();
    const ActivePlayerData& apd = game->activePlayerData();

    // Health/boost
    refreshHealth(snap.health);
    refreshBoost(apd.boost);

    // Weapon + ammo
    std::string weaponName;
    int ammo = -1;
    int reserve = -1;
    if (apd.curWeapIdx >= 0 && static_cast<size_t>(apd.curWeapIdx) < apd.weapons.size()) {
        const auto& weapon = apd.weapons[apd.curWeapIdx];
        // Use weapon type directly as display name
        const auto* provider = getDefProvider();
        const auto* def = provider ? provider->gameObject(weapon.type) : nullptr;
        if (def && def->category != "melee") {
            ammo = weapon.ammo;
        }
        weaponName = weapon.type;
        if (def && def->category == "gun") {
            const auto found = std::find(defs::kBagSizeKeys.begin(), defs::kBagSizeKeys.end(), def->ammoType);
            const auto index = static_cast<size_t>(found - defs::kBagSizeKeys.begin());
            if (index < apd.inventory.size()) {
                reserve = static_cast<int>(apd.inventory[index]);
            }
        }
    }
    refreshWeapon(weaponName, ammo, reserve);

    // Alive count
    const auto counts = game->aliveCounts();
    const int totalAlive = std::accumulate(counts.begin(), counts.end(), 0);
    refreshAlive(totalAlive, static_cast<int>(counts.size()));

    // Kill feed
    std::string feedText;
    for (const auto& entry : game->killFeed()) {
        const auto& msg = entry.message;
        const auto verb = msg.downed ? "knocked out" : "killed";
        if (msg.damageType == DamageType_Player) {
            feedText += entry.killerName + " " + verb + " " + entry.targetName;
        } else {
            feedText += entry.targetName + " - " + (msg.damageType == DamageType_Gas ? "gas" : "died");
        }
        if (!msg.itemSourceType.empty()) {
            feedText += " (" + msg.itemSourceType + ")";
        }
        feedText += "\n";
    }
    refreshKillFeed(feedText);

    // Game result
    const auto* over = game->gameOver();
    if (over) {
        std::string result;
        result = "Rank #" + std::to_string(over->teamRank);
        for (const auto& stats : over->playerStats) {
            if (stats.playerId == game->getLocalPlayerId()) {
                result += "\n" + std::to_string(stats.kills) + " Kills"
                    + "   " + std::to_string(stats.damageDealt) + " damage";
            }
        }
        refreshResult(result);
        showResult(!over->teamRank == 0);
    } else {
        showResult(false);
    }
}

} // namespace surv

#endif // SURV_UI_NO_AXMOL