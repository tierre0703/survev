#ifdef SURV_UI_NO_AXMOL
// Host build without axmol/FGUI; stubs below are empty.
#else

#include "FguiHud.h"
#include "../game/Game.h"
#include "../game/Map.h"
#include "../game/Gas.h"
#include "../render/Defs.h"
#include "../ui/Localization.h"
#include "../core/GameConfig.h"
#include "../net/Messages.h"

#include "fairygui/GRoot.h"
#include "fairygui/GComponent.h"
#include "fairygui/GGraph.h"
#include "fairygui/GTextField.h"
#include "fairygui/GObject.h"
#include "fairygui/GButton.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

USING_NS_AX;

namespace surv {

// ---------------------------------------------------------------------------
// FguiHud — FairyGUI-based HUD v2: weapon cards, gear, perks, scopes, loot
// counts, pickup/kill messages, interaction prompt, health/boost, kill feed,
// alive count, squad bars, minimap, and game result overlay.
//
// Layout (top to bottom, left-anchored):
//   Health bar + Boost bar          [far left, bottom area]
//   Weapon cards (4)                [bottom right]
//   Gear (helmet/chest/backpack)    [left of weapons]
//   Perk slots (4)                  [left of gear]
//   Scope inventory (5)             [top center]
//   Loot grid (ammo/heals/... )     [right center, collapsed]
//
// Overlays (center screen):
//   Kill message                    [upper center]
//   Pickup message                  [lower center]
//   Interaction prompt              [lower center]
//   Kill feed                       [top right]
//   Alive counter                   [top right]
//   Squad bars                      [top left]
//   Minimap                         [top right corner area]
//   Result overlay                  [center]
// ---------------------------------------------------------------------------

// Layout constants (pixels in FG design-space, scaled by GRoot).
static constexpr float kBarWidth = 220.0f;
static constexpr float kBarHeight = 14.0f;
static constexpr float kPanelPadding = 6.0f;
static constexpr float kFontSize = 16.0f;
static constexpr float kSmallFontSize = 12.0f;
static constexpr float kTinyFontSize = 10.0f;

// Weapon slot dimensions
static constexpr float kWeaponSlotW = 160.0f;
static constexpr float kWeaponSlotH = 30.0f;
static constexpr float kWeaponSlotGap = 2.0f;

// Gear/perk slot dimensions
static constexpr float kGearSlotW = 70.0f;
static constexpr float kGearSlotH = 18.0f;
static constexpr float kPerkSlotW = 70.0f;
static constexpr float kPerkSlotH = 18.0f;

// Scope slot
static constexpr float kScopeSlotW = 40.0f;
static constexpr float kScopeSlotH = 20.0f;

// Loot grid cell
static constexpr float kLootCellW = 50.0f;
static constexpr float kLootCellH = 22.0f;

// Message sizes
static constexpr float kKillMsgW = 320.0f;
static constexpr float kKillMsgH = 50.0f;

// Boost breakpoints from shared/gameConfig.ts: player.boostBreakpoints = [1, 1, 1.5, 0.5]
static constexpr int kBoostBarCount = 4;
static constexpr float kBoostBreakpoints[kBoostBarCount] = {1.0f, 1.0f, 1.5f, 0.5f};
static constexpr float kBoostBreakpointsTotal = 4.0f;

// Kill feed: max lines, line height, fade time
static constexpr int kKillFeedMaxLines = 6;
static constexpr float kKillFeedLineH = 18.0f;
static constexpr float kKillFeedWidth = 280.0f;
static constexpr float kKillFeedHeight = kKillFeedMaxLines * kKillFeedLineH + 8.0f;

// Result overlay
static constexpr float kResultWidth = 380.0f;
static constexpr float kResultHeight = 280.0f;

// Squad bars
static constexpr float kSquadBarWidth = 150.0f;
static constexpr float kSquadBarHeight = 10.0f;
static constexpr float kSquadNameFontSize = 14.0f;
static constexpr float kSquadSlotHeight = 32.0f;
static constexpr float kSquadPanelWidth = kSquadBarWidth + 36.0f;
static constexpr float kSquadPanelHeight = 4 * kSquadSlotHeight + 8.0f;

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
};

// English-only translate helper.
static std::string tr(const std::string& fallback) {
    return fallback;
}

// Pickup message text mapping (mirrors ui2.ts getPickupMessageText)
static std::string getPickupText(uint8_t type) {
    switch (type) {
        case PickupMsgType_Full:             return "Not enough space";
        case PickupMsgType_AlreadyOwned:     return "Item already owned";
        case PickupMsgType_AlreadyEquipped:  return "Item already equipped";
        case PickupMsgType_BetterItemEquipped: return "Better item equipped";
        case PickupMsgType_GunCannotFire:    return "Gun cannot fire";
        case PickupMsgType_MaxPerks:         return "Max perks reached";
        default:                             return "";
    }
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

// ---- Gear item display helpers -------------------------------------------
static std::string gearLevelStr(const std::string& item, const DefProvider* provider) {
    if (item.empty() || !provider) return "";
    const auto* def = provider->gameObject(item);
    if (def && def->level > 0) {
        if (def->level == 4) return "Lv.4";
        if (def->level == 3) return "Lv.3";
        if (def->level == 2) return "Lv.2";
        return "Lv.1";
    }
    return "";
}

static std::string gearDisplayName(const std::string& item) {
    if (item.empty()) return "";
    // Trim item type prefix for display
    if (item.find("chest") == 0) return std::string("Vest Lv.") + item.back();
    if (item.find("helmet") == 0) return std::string("Helm Lv.") + item.back();
    if (item.find("backpack") == 0) {
        if (item == "backpack00") return "Bag Lv.0";
        return std::string("Bag Lv.") + item.back();
    }
    return item;
}

// ---- Build: creates all FGUI widgets -------------------------------------
void FguiHud::build(ax::Scene* scene) {
    fairygui::GRoot* root = fairygui::GRoot::getInstance();
    if (!root) return;

    buildHudPanel(root);         // health, boost, weapon/ammo compact text
    buildWeaponSlots(root);      // 4 weapon cards (bottom right)
    buildGearSlots(root);        // helmet, chest, backpack
    buildPerkSlots(root);        // up to 4 perks
    buildScopeSlots(root);       // 5 scope slots
    buildLootGrid(root);         // inventory item counts
    buildKillFeed(root);
    buildKillMessage(root);      // center kill notification
    buildPickupMessage(root);    // pickup feedback
    buildInteractionPrompt(root);
    buildAliveCounter(root);
    buildResultOverlay(root);
    buildSquadBars(root);
    buildMinimap(root);
}

// ---- Health + Boost + Compact Weapon/Ammo text (legacy location) ---------
void FguiHud::buildHudPanel(fairygui::GComponent* root) {
    // Panel background: semi-transparent black rect behind the HUD
    const float hudW = kBarWidth + 180.0f;
    const float hudH = 90.0f;
    fairygui::GGraph* panelBg = fairygui::GGraph::create();
    panelBg->drawRect(hudW, hudH, 0,
                      Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                      Color4F(0.0f, 0.0f, 0.0f, 0.30f));
    panelBg->setPosition(0.0f, 0.0f);
    panelBg->setPivot(0.0f, 0.0f);
    _hudPanel = panelBg;

    // --- Health bar ---
    // Background
    const float healthBarY = hudH - kBarHeight - kPanelPadding;
    fairygui::GGraph* healthBg = fairygui::GGraph::create();
    healthBg->drawRect(kBarWidth, kBarHeight, 0,
                       Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                       Color4F(0.57f, 0.57f, 0.57f, 1.0f));
    healthBg->setPosition(kPanelPadding, healthBarY);
    healthBg->setPivot(0.0f, 0.0f);
    panelBg->addChild(healthBg);

    // Foreground fill
    fairygui::GGraph* healthFill = fairygui::GGraph::create();
    healthFill->drawRect(kBarWidth, kBarHeight, 0,
                         Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                         Color4F(0.85f, 0.2f, 0.2f, 1.0f));
    healthFill->setPosition(kPanelPadding, healthBarY);
    healthFill->setPivot(0.0f, 0.0f);
    panelBg->addChild(healthFill);
    _healthFill = healthFill;

    // Health numeric text
    fairygui::GBasicTextField* healthTf = fairygui::GBasicTextField::create();
    healthTf->setText("");
    healthTf->setColor(Color3B(255, 255, 255));
    healthTf->setFontSize(kSmallFontSize);
    healthTf->setSingleLine(true);
    healthTf->setPosition(kPanelPadding + kBarWidth + 4.0f, healthBarY - 1.0f);
    healthTf->setPivot(0.0f, 0.0f);
    panelBg->addChild(healthTf);
    _healthText = healthTf;

    // --- Boost bar ---
    const float boostBarY = kPanelPadding + kBarHeight + 4.0f;
    fairygui::GGraph* boostBg = fairygui::GGraph::create();
    boostBg->drawRect(kBarWidth, kBarHeight, 0,
                      Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                      Color4F(0.15f, 0.15f, 0.15f, 0.9f));
    boostBg->setPosition(kPanelPadding, boostBarY);
    boostBg->setPivot(0.0f, 0.0f);
    panelBg->addChild(boostBg);

    // 4 boost segment fills
    static const uint32_t boostColors[] = {0xffff901a, 0xffff751a, 0xffff6616, 0xffff5600};
    for (int i = 0; i < kBoostBarCount; ++i) {
        const float segmentWidth = kBarWidth * (kBoostBreakpoints[i] / kBoostBreakpointsTotal);
        fairygui::GGraph* seg = fairygui::GGraph::create();
        const uint32_t c = boostColors[i];
        seg->drawRect(segmentWidth, kBarHeight, 0,
                      Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                      Color4F(((c >> 16) & 0xff) / 255.0f,
                              ((c >> 8) & 0xff) / 255.0f,
                              (c & 0xff) / 255.0f, 1.0f));
        seg->setPosition(kPanelPadding, boostBarY);
        seg->setPivot(0.0f, 0.0f);
        seg->setVisible(false);
        panelBg->addChild(seg);
        _boostSegments[i] = seg;
    }

    // Compact weapon name + ammo text (top of HUD panel)
    fairygui::GBasicTextField* weaponTf = fairygui::GBasicTextField::create();
    weaponTf->setText("");
    weaponTf->setColor(Color3B(220, 220, 220));
    weaponTf->setFontSize(kSmallFontSize);
    weaponTf->setSingleLine(true);
    weaponTf->setPosition(kPanelPadding + kBarWidth + 4.0f, boostBarY + kBarHeight + 2.0f);
    weaponTf->setPivot(0.0f, 0.0f);
    panelBg->addChild(weaponTf);
    _weaponText = weaponTf;

    fairygui::GBasicTextField* ammoTf = fairygui::GBasicTextField::create();
    ammoTf->setText("");
    ammoTf->setColor(Color3B(255, 255, 255));
    ammoTf->setFontSize(kSmallFontSize);
    ammoTf->setSingleLine(true);
    ammoTf->setPosition(kPanelPadding, boostBarY + kBarHeight + 2.0f);
    ammoTf->setPivot(0.0f, 0.0f);
    panelBg->addChild(ammoTf);
    _ammoText = ammoTf;

    // Position the entire HUD panel at bottom-left
    panelBg->setPivot(0.0f, 1.0f);
    panelBg->setPosition(4.0f, -4.0f);
    root->addChild(panelBg);
}

// ---- Weapon slots (4 cards: primary, secondary, melee, throwable) --------
void FguiHud::buildWeaponSlots(fairygui::GComponent* root) {
    const float panelW = kWeaponSlotW + kPanelPadding * 2;
    const float panelH = kWeaponSlotCount * (kWeaponSlotH + kWeaponSlotGap) + kPanelPadding * 2;

    fairygui::GGraph* panel = fairygui::GGraph::create();
    panel->drawRect(panelW, panelH, 0,
                    Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                    Color4F(0.0f, 0.0f, 0.0f, 0.30f));
    panel->setPivot(1.0f, 1.0f); // bottom-right anchored
    panel->setPosition(-4.0f, -4.0f);

    for (int i = 0; i < kWeaponSlotCount; ++i) {
        const float y = kPanelPadding + static_cast<float>(i) * (kWeaponSlotH + kWeaponSlotGap);

        // Background rect
        fairygui::GGraph* bg = fairygui::GGraph::create();
        bg->drawRect(kWeaponSlotW, kWeaponSlotH, 0,
                     Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                     Color4F(0.08f, 0.08f, 0.08f, 0.7f));
        bg->setPosition(kPanelPadding, y);
        bg->setPivot(0.0f, 0.0f);
        panel->addChild(bg);
        _weaponSlots[i].bg = bg;

        // Equipped highlight bar (left edge, shown when equipped)
        fairygui::GGraph* hl = fairygui::GGraph::create();
        hl->drawRect(4.0f, kWeaponSlotH, 0,
                     Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                     Color4F(0.0f, 0.7f, 1.0f, 0.9f));
        hl->setPosition(kPanelPadding, y);
        hl->setPivot(0.0f, 0.0f);
        hl->setVisible(false);
        panel->addChild(hl);
        _weaponSlots[i].equippedHighlight = hl;

        // Keybind (slot number)
        fairygui::GBasicTextField* kbTf = fairygui::GBasicTextField::create();
        kbTf->setText(std::to_string(i + 1));
        kbTf->setColor(Color3B(180, 180, 180));
        kbTf->setFontSize(kTinyFontSize);
        kbTf->setSingleLine(true);
        kbTf->setPosition(kPanelPadding + 6.0f, y + 1.0f);
        kbTf->setPivot(0.0f, 0.0f);
        panel->addChild(kbTf);
        _weaponSlots[i].keybindText = kbTf;

        // Weapon name
        fairygui::GBasicTextField* nameTf = fairygui::GBasicTextField::create();
        nameTf->setText("");
        nameTf->setColor(Color3B(220, 220, 220));
        nameTf->setFontSize(kSmallFontSize);
        nameTf->setSingleLine(true);
        nameTf->setPosition(kPanelPadding + 18.0f, y + 2.0f);
        nameTf->setPivot(0.0f, 0.0f);
        panel->addChild(nameTf);
        _weaponSlots[i].nameText = nameTf;

        // Ammo count
        fairygui::GBasicTextField* ammoTf = fairygui::GBasicTextField::create();
        ammoTf->setText("");
        ammoTf->setColor(Color3B(255, 255, 200));
        ammoTf->setFontSize(kTinyFontSize);
        ammoTf->setSingleLine(true);
        ammoTf->setPosition(kPanelPadding + kWeaponSlotW - 35.0f, y + 2.0f);
        ammoTf->setPivot(0.0f, 0.0f);
        panel->addChild(ammoTf);
        _weaponSlots[i].ammoText = ammoTf;
    }

    panel->setPivot(1.0f, 1.0f);
    panel->setPosition(-4.0f, -4.0f);
    root->addChild(panel);
}

// ---- Gear slots (helmet, chest, backpack) --------------------------------
void FguiHud::buildGearSlots(fairygui::GComponent* root) {
    const float panelW = kGearSlotW + kPanelPadding * 2 + 10.0f;
    const float panelH = kGearSlotCount * (kGearSlotH + 2) + kPanelPadding * 2;

    fairygui::GGraph* panel = fairygui::GGraph::create();
    panel->drawRect(panelW, panelH, 0,
                    Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                    Color4F(0.0f, 0.0f, 0.0f, 0.25f));
    panel->setPivot(1.0f, 1.0f);

    for (int i = 0; i < kGearSlotCount; ++i) {
        const float y = kPanelPadding + static_cast<float>(i) * (kGearSlotH + 2.0f);

        fairygui::GGraph* bg = fairygui::GGraph::create();
        bg->drawRect(kGearSlotW, kGearSlotH, 0,
                     Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                     Color4F(0.1f, 0.1f, 0.1f, 0.7f));
        bg->setPosition(kPanelPadding, y);
        bg->setPivot(0.0f, 0.0f);
        panel->addChild(bg);
        _gearSlots[i].bg = bg;

        // Level indicator
        fairygui::GBasicTextField* lvlTf = fairygui::GBasicTextField::create();
        lvlTf->setText("");
        lvlTf->setColor(Color3B(255, 255, 255));
        lvlTf->setFontSize(kTinyFontSize);
        lvlTf->setSingleLine(true);
        lvlTf->setPosition(kPanelPadding + 2.0f, y + 1.0f);
        lvlTf->setPivot(0.0f, 0.0f);
        panel->addChild(lvlTf);
        _gearSlots[i].levelText = lvlTf;

        // Name
        fairygui::GBasicTextField* nameTf = fairygui::GBasicTextField::create();
        nameTf->setText("");
        nameTf->setColor(Color3B(200, 200, 200));
        nameTf->setFontSize(kSmallFontSize);
        nameTf->setSingleLine(true);
        nameTf->setPosition(kPanelPadding + 22.0f, y + 1.0f);
        nameTf->setPivot(0.0f, 0.0f);
        panel->addChild(nameTf);
        _gearSlots[i].nameText = nameTf;
    }

    // Position relative to weapon panel (left of weapons)
    panel->setPivot(1.0f, 1.0f);
    panel->setPosition(-kWeaponSlotW - 2 * kPanelPadding - 12.0f, -4.0f);
    root->addChild(panel);
}

// ---- Perk slots (up to 4) ------------------------------------------------
void FguiHud::buildPerkSlots(fairygui::GComponent* root) {
    const float panelW = kPerkSlotW + kPanelPadding * 2;
    const float panelH = kPerkSlotCount * (kPerkSlotH + 2) + kPanelPadding * 2;

    fairygui::GGraph* panel = fairygui::GGraph::create();
    panel->drawRect(panelW, panelH, 0,
                    Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                    Color4F(0.0f, 0.0f, 0.0f, 0.25f));
    panel->setPivot(1.0f, 1.0f);

    for (int i = 0; i < kPerkSlotCount; ++i) {
        const float y = kPanelPadding + static_cast<float>(i) * (kPerkSlotH + 2.0f);

        fairygui::GGraph* bg = fairygui::GGraph::create();
        bg->drawRect(kPerkSlotW, kPerkSlotH, 0,
                     Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                     Color4F(0.12f, 0.12f, 0.12f, 0.7f));
        bg->setPosition(kPanelPadding, y);
        bg->setPivot(0.0f, 0.0f);
        panel->addChild(bg);
        _perkSlots[i].bg = bg;

        fairygui::GBasicTextField* nameTf = fairygui::GBasicTextField::create();
        nameTf->setText("");
        nameTf->setColor(Color3B(200, 200, 100));
        nameTf->setFontSize(kTinyFontSize);
        nameTf->setSingleLine(true);
        nameTf->setPosition(kPanelPadding + 2.0f, y + 1.0f);
        nameTf->setPivot(0.0f, 0.0f);
        panel->addChild(nameTf);
        _perkSlots[i].nameText = nameTf;
    }

    // Position left of gear panel
    panel->setPivot(1.0f, 1.0f);
    const float gearOffset = kGearSlotW + 2.0f * kPanelPadding + 12.0f;
    const float weaponOffset = kWeaponSlotW + 2.0f * kPanelPadding;
    panel->setPosition(-(weaponOffset + gearOffset + kPerkSlotW + 4.0f), -4.0f);
    root->addChild(panel);
}

// ---- Scope slots (5: 1x–15x) ---------------------------------------------
void FguiHud::buildScopeSlots(fairygui::GComponent* root) {
    const float totalW = kScopeSlotCount * (kScopeSlotW + 2.0f) + kPanelPadding * 2;
    const float panelH = kScopeSlotH + kPanelPadding * 2;

    fairygui::GGraph* panel = fairygui::GGraph::create();
    panel->drawRect(totalW, panelH, 0,
                    Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                    Color4F(0.0f, 0.0f, 0.0f, 0.20f));
    panel->setPivot(0.5f, 1.0f);
    panel->setPosition(0.0f, 0.0f);

    for (int i = 0; i < kScopeSlotCount; ++i) {
        const float x = kPanelPadding + static_cast<float>(i) * (kScopeSlotW + 2.0f);

        fairygui::GGraph* bg = fairygui::GGraph::create();
        bg->drawRect(kScopeSlotW, kScopeSlotH, 0,
                     Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                     Color4F(0.1f, 0.1f, 0.1f, 0.6f));
        bg->setPosition(x, kPanelPadding);
        bg->setPivot(0.0f, 0.0f);
        bg->setVisible(false);
        panel->addChild(bg);
        _scopeSlots[i].bg = bg;

        fairygui::GBasicTextField* labelTf = fairygui::GBasicTextField::create();
        labelTf->setText("");
        labelTf->setColor(Color3B(200, 200, 200));
        labelTf->setFontSize(kTinyFontSize);
        labelTf->setSingleLine(true);
        labelTf->setPosition(x + 1.0f, kPanelPadding + 1.0f);
        labelTf->setPivot(0.0f, 0.0f);
        labelTf->setVisible(false);
        panel->addChild(labelTf);
        _scopeSlots[i].labelText = labelTf;
    }

    // Position at top center
    panel->setPivot(0.5f, 0.0f);
    panel->setPosition(0.0f, 4.0f);
    root->addChild(panel);
}

// ---- Loot grid (ammo/heals/throwables) -----------------------------------
void FguiHud::buildLootGrid(fairygui::GComponent* root) {
    const int cols = 5;
    const int rows = (kLootGridCount + cols - 1) / cols;
    const float panelW = static_cast<float>(cols) * (kLootCellW + 2.0f) + kPanelPadding * 2;
    const float panelH = static_cast<float>(rows) * (kLootCellH + 1.0f) + kPanelPadding * 2;

    fairygui::GGraph* panel = fairygui::GGraph::create();
    panel->drawRect(panelW, panelH, 0,
                    Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                    Color4F(0.0f, 0.0f, 0.0f, 0.20f));
    panel->setPivot(1.0f, 0.0f);

    for (int i = 0; i < kLootGridCount; ++i) {
        const int col = i % cols;
        const int row = i / cols;
        const float x = kPanelPadding + static_cast<float>(col) * (kLootCellW + 2.0f);
        const float y = kPanelPadding + static_cast<float>(row) * (kLootCellH + 1.0f);

        fairygui::GGraph* bg = fairygui::GGraph::create();
        bg->drawRect(kLootCellW, kLootCellH, 0,
                     Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                     Color4F(0.08f, 0.08f, 0.08f, 0.5f));
        bg->setPosition(x, y);
        bg->setPivot(0.0f, 0.0f);
        bg->setVisible(false);
        panel->addChild(bg);
        _lootSlots[i].bg = bg;

        fairygui::GBasicTextField* nameTf = fairygui::GBasicTextField::create();
        nameTf->setText("");
        nameTf->setColor(Color3B(180, 180, 180));
        nameTf->setFontSize(kTinyFontSize);
        nameTf->setSingleLine(true);
        nameTf->setPosition(x + 1.0f, y);
        nameTf->setPivot(0.0f, 0.0f);
        nameTf->setVisible(false);
        panel->addChild(nameTf);
        _lootSlots[i].nameText = nameTf;

        fairygui::GBasicTextField* countTf = fairygui::GBasicTextField::create();
        countTf->setText("");
        countTf->setColor(Color3B(255, 255, 255));
        countTf->setFontSize(kTinyFontSize);
        countTf->setSingleLine(true);
        countTf->setPosition(x + kLootCellW - 18.0f, y);
        countTf->setPivot(0.0f, 0.0f);
        countTf->setVisible(false);
        panel->addChild(countTf);
        _lootSlots[i].countText = countTf;
    }

    // Position right side, below scope slots
    panel->setPivot(1.0f, 0.0f);
    panel->setPosition(-4.0f, 30.0f);
    root->addChild(panel);
}

// ---- Kill message (center screen) ----------------------------------------
void FguiHud::buildKillMessage(fairygui::GComponent* root) {
    fairygui::GGraph* bg = fairygui::GGraph::create();
    bg->drawRect(kKillMsgW, kKillMsgH, 0,
                 Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                 Color4F(0.0f, 0.0f, 0.0f, 0.0f)); // fully transparent, just a container
    bg->setPivot(0.5f, 0.5f);
    bg->setPosition(0.0f, 0.0f);
    bg->setVisible(false);
    root->addChild(bg);
    _killMessageBg = bg;

    fairygui::GBasicTextField* textTf = fairygui::GBasicTextField::create();
    textTf->setText("");
    textTf->setColor(Color3B(255, 200, 50));
    textTf->setFontSize(22.0f);
    textTf->setSingleLine(false);
    textTf->setPosition(-kKillMsgW * 0.5f + 10.0f, -kKillMsgH * 0.5f + 4.0f);
    textTf->setPivot(0.0f, 0.0f);
    textTf->setVisible(false);
    bg->addChild(textTf);
    _killMessageText = textTf;

    fairygui::GBasicTextField* countTf = fairygui::GBasicTextField::create();
    countTf->setText("");
    countTf->setColor(Color3B(255, 255, 255));
    countTf->setFontSize(28.0f);
    countTf->setSingleLine(true);
    countTf->setPosition(kKillMsgW * 0.5f - 50.0f, -kKillMsgH * 0.5f + 4.0f);
    countTf->setPivot(1.0f, 0.0f);
    countTf->setVisible(false);
    bg->addChild(countTf);
    _killMessageCount = countTf;
}

// ---- Pickup message (below center) ---------------------------------------
void FguiHud::buildPickupMessage(fairygui::GComponent* root) {
    fairygui::GBasicTextField* tf = fairygui::GBasicTextField::create();
    tf->setText("");
    tf->setColor(Color3B(255, 200, 100));
    tf->setFontSize(kSmallFontSize);
    tf->setSingleLine(true);
    tf->setPivot(0.5f, 1.0f);
    tf->setPosition(0.0f, -70.0f);
    tf->setVisible(false);
    root->addChild(tf);
    _pickupMessageText = tf;
}

// ---- Interaction prompt --------------------------------------------------
void FguiHud::buildInteractionPrompt(fairygui::GComponent* root) {
    const float promptW = 200.0f;
    const float promptH = 26.0f;

    fairygui::GGraph* bg = fairygui::GGraph::create();
    bg->drawRect(promptW, promptH, 0,
                 Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                 Color4F(0.0f, 0.0f, 0.0f, 0.0f));
    bg->setPivot(0.5f, 1.0f);
    bg->setPosition(0.0f, -100.0f);
    bg->setVisible(false);
    root->addChild(bg);
    _interactionBg = bg;

    fairygui::GBasicTextField* keyTf = fairygui::GBasicTextField::create();
    keyTf->setText("");
    keyTf->setColor(Color3B(255, 200, 50));
    keyTf->setFontSize(kSmallFontSize);
    keyTf->setSingleLine(true);
    keyTf->setPosition(-promptW * 0.5f + 4.0f, 2.0f);
    keyTf->setPivot(0.0f, 0.0f);
    keyTf->setVisible(false);
    bg->addChild(keyTf);
    _interactionKeyText = keyTf;

    fairygui::GBasicTextField* descTf = fairygui::GBasicTextField::create();
    descTf->setText("");
    descTf->setColor(Color3B(200, 200, 200));
    descTf->setFontSize(kSmallFontSize);
    descTf->setSingleLine(true);
    descTf->setPosition(-promptW * 0.5f + 30.0f, 2.0f);
    descTf->setPivot(0.0f, 0.0f);
    descTf->setVisible(false);
    bg->addChild(descTf);
    _interactionDescText = descTf;
}

// ---- Alive counter -------------------------------------------------------
void FguiHud::buildAliveCounter(fairygui::GComponent* root) {
    fairygui::GBasicTextField* aliveTf = fairygui::GBasicTextField::create();
    aliveTf->setText("");
    aliveTf->setColor(Color3B(255, 255, 255));
    aliveTf->setFontSize(20.0f);
    aliveTf->setSingleLine(true);
    aliveTf->setPosition(-160.0f, 6.0f);
    aliveTf->setPivot(1.0f, 0.0f);
    root->addChild(aliveTf);
    _aliveText = aliveTf;
}

// ---- Kill feed (multi-line) ----------------------------------------------
void FguiHud::buildKillFeed(fairygui::GComponent* root) {
    fairygui::GBasicTextField* killTf = fairygui::GBasicTextField::create();
    killTf->setText("");
    killTf->setColor(Color3B(255, 200, 150));
    killTf->setFontSize(kSmallFontSize);
    killTf->setSingleLine(false);
    killTf->setPosition(-160.0f, 34.0f);
    killTf->setPivot(1.0f, 0.0f);
    root->addChild(killTf);
    _killFeedText = killTf;
}

// ---- Result overlay ------------------------------------------------------
void FguiHud::buildResultOverlay(fairygui::GComponent* root) {
    fairygui::GGraph* panel = fairygui::GGraph::create();
    panel->drawRect(kResultWidth, kResultHeight, 2,
                    Color4F(1.0f, 1.0f, 1.0f, 1.0f),
                    Color4F(0.0f, 0.0f, 0.0f, 0.85f));
    panel->setPivot(0.5f, 0.5f);
    panel->setPosition(0.0f, 0.0f);
    panel->setVisible(false);
    root->addChild(panel);
    _resultPanel = panel;

    fairygui::GBasicTextField* resultTf = fairygui::GBasicTextField::create();
    resultTf->setText("");
    resultTf->setColor(Color3B(255, 220, 100));
    resultTf->setFontSize(22.0f);
    resultTf->setSingleLine(false);
    resultTf->setPosition(-kResultWidth / 2.0f + 20.0f, -kResultHeight / 2.0f + 30.0f);
    resultTf->setPivot(0.0f, 0.0f);
    resultTf->setVisible(false);
    panel->addChild(resultTf);
    _resultText = resultTf;
}

// ---- Squad health bars ---------------------------------------------------
void FguiHud::buildSquadBars(fairygui::GComponent* root) {
    fairygui::GGraph* panel = fairygui::GGraph::create();
    panel->drawRect(kSquadPanelWidth, kSquadPanelHeight, 0,
                    Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                    Color4F(0.0f, 0.0f, 0.0f, 0.4f));
    panel->setPosition(6.0f, 6.0f);
    panel->setPivot(0.0f, 0.0f);
    panel->setVisible(false);
    root->addChild(panel);

    for (int i = 0; i < 4; ++i) {
        const float y = 4.0f + static_cast<float>(i) * kSquadSlotHeight;

        fairygui::GComponent* slot = fairygui::GComponent::create();
        slot->setPosition(0.0f, y);
        slot->setPivot(0.0f, 0.0f);
        panel->addChild(slot);

        fairygui::GBasicTextField* nameTf = fairygui::GBasicTextField::create();
        nameTf->setText("");
        nameTf->setColor(Color3B(200, 200, 200));
        nameTf->setFontSize(kSquadNameFontSize);
        nameTf->setSingleLine(true);
        nameTf->setPosition(4.0f, 0.0f);
        nameTf->setPivot(0.0f, 0.0f);
        slot->addChild(nameTf);

        fairygui::GGraph* barBg = fairygui::GGraph::create();
        barBg->drawRect(kSquadBarWidth, kSquadBarHeight, 0,
                        Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                        Color4F(0.2f, 0.2f, 0.2f, 0.8f));
        barBg->setPosition(4.0f, kSquadNameFontSize + 2.0f);
        barBg->setPivot(0.0f, 0.0f);
        slot->addChild(barBg);

        fairygui::GGraph* healthFill = fairygui::GGraph::create();
        healthFill->drawRect(kSquadBarWidth, kSquadBarHeight, 0,
                             Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                             Color4F(0.3f, 0.8f, 0.3f, 1.0f));
        healthFill->setPosition(4.0f, kSquadNameFontSize + 2.0f);
        healthFill->setPivot(0.0f, 0.0f);
        slot->addChild(healthFill);

        fairygui::GBasicTextField* statusTf = fairygui::GBasicTextField::create();
        statusTf->setText("");
        statusTf->setColor(Color3B(255, 80, 80));
        statusTf->setFontSize(kSquadNameFontSize);
        statusTf->setSingleLine(true);
        statusTf->setPosition(4.0f + kSquadBarWidth + 4.0f, 0.0f);
        statusTf->setPivot(0.0f, 0.0f);
        slot->addChild(statusTf);

        _squadMembers[i].container = slot;
        _squadMembers[i].nameText = nameTf;
        _squadMembers[i].healthBar = healthFill;
        _squadMembers[i].statusText = statusTf;
        _squadMembers[i].prevHealth = -1;
        _squadMembers[i].prevDead = false;
        _squadMembers[i].prevDowned = false;
        _squadMembers[i].prevDisconnected = false;
    }
}

// ---- Minimap (stub) ------------------------------------------------------
void FguiHud::buildMinimap(fairygui::GComponent* root) {
    // Stub: minimap implementation deferred
    (void)root;
}

// ---- Refresh methods ----------------------------------------------------

void FguiHud::refreshHealth(float health) {
    if (!_healthFill || !_healthText || !_hudPanel) return;

    const bool downed = health < 0.0f;
    if (downed) health = 0.0f;
    health = std::clamp(health, 0.0f, 100.0f);
    const float fraction = health / 100.0f;
    const float fillWidth = kBarWidth * fraction;

    uint32_t color = healthColor(health, downed);
    _healthFill->clear();
    _healthFill->drawRect(std::max(fillWidth, 0.0f), kBarHeight, 0,
                          Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                          Color4F(((color >> 16) & 0xff) / 255.0f,
                                  ((color >> 8) & 0xff) / 255.0f,
                                  (color & 0xff) / 255.0f, 1.0f));
    _healthFill->setVisible(fillWidth > 0.0f);

    std::string text = std::to_string(static_cast<int>(std::round(health)));
    _healthText->setText(text);
}

void FguiHud::refreshBoost(float boost) {
    if (!_hudPanel) return;

    boost = std::clamp(boost, 0.0f, 100.0f);
    float boostT = boost / 100.0f;

    for (int i = 0; i < kBoostBarCount; ++i) {
        const float breakPointT = kBoostBreakpoints[i] / kBoostBreakpointsTotal;
        boostT = std::max(boostT - breakPointT, 0.0f);
        // Simple visibility toggle (web client uses proportional width)
        if (_boostSegments[i]) {
            _boostSegments[i]->setVisible(boostT > 0.001f);
        }
    }
}

void FguiHud::refreshWeapon(const std::string& weapon, int ammo, int reserve) {
    if (!_weaponText || !_ammoText || !_hudPanel) return;

    if (!weapon.empty()) {
        _weaponText->setText(weapon);
    } else {
        _weaponText->setText("");
    }

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

// ---- Inventory v2: refresh methods ---------------------------------------

void FguiHud::refreshWeaponSlots(const std::vector<WeaponData>& weapons, int curWeapIdx) {
    for (int i = 0; i < kWeaponSlotCount; ++i) {
        const auto& ws = _weaponSlots[i];

        bool hasWeapon = false;
        std::string wName;
        int wAmmo = 0;

        if (i < static_cast<int>(weapons.size()) && !weapons[i].type.empty()) {
            hasWeapon = true;
            wName = weapons[i].type;
            wAmmo = weapons[i].ammo;
        }

        // Equipped highlight
        bool equipped = hasWeapon && (i == curWeapIdx);
        if (ws.equippedHighlight) {
            ws.equippedHighlight->setVisible(equipped);
        }

        // Name text
        if (ws.nameText) {
            ws.nameText->setText(hasWeapon ? wName : "");
            // Get parent panel's visibility — the bg is the parent panel
            if (ws.bg) {
                ws.bg->setVisible(hasWeapon || i == WeaponSlot_Primary || i == WeaponSlot_Secondary);
            }
        }

        // Ammo count
        if (ws.ammoText) {
            ws.ammoText->setText(hasWeapon && wAmmo > 0 ? std::to_string(wAmmo) : "");
        }

        // Slot visibility
        if (ws.bg) {
            ws.bg->setVisible(hasWeapon || i == WeaponSlot_Primary || i == WeaponSlot_Secondary);
        }
    }
}

void FguiHud::refreshLootCounts(const std::vector<uint16_t>& inventory) {
    for (int i = 0; i < kLootGridCount && i < static_cast<int>(inventory.size()); ++i) {
        const uint16_t count = inventory[i];
        const auto& ls = _lootSlots[i];

        if (ls.bg) ls.bg->setVisible(count > 0);
        if (ls.nameText) {
            if (i < static_cast<int>(defs::kBagSizeKeys.size())) {
                ls.nameText->setText(count > 0 ? std::string(defs::kBagSizeKeys[i]) : "");
            }
            ls.nameText->setVisible(count > 0);
        }
        if (ls.countText) {
            ls.countText->setText(count > 0 ? std::to_string(count) : "");
            ls.countText->setVisible(count > 0);
        }
    }
}

void FguiHud::refreshGear(const std::string& chest, const std::string& helmet, const std::string& backpack) {
    const std::array<std::string, kGearSlotCount> items = {helmet, chest, backpack};
    const auto* provider = getDefProvider();

    for (int i = 0; i < kGearSlotCount; ++i) {
        const auto& gs = _gearSlots[i];
        const std::string& item = items[i];
        const bool hasItem = !item.empty() && item != "backpack00";

        if (gs.bg) gs.bg->setVisible(hasItem);

        // Level
        if (gs.levelText) {
            gs.levelText->setText(hasItem ? gearLevelStr(item, provider) : "");
            gs.levelText->setVisible(hasItem);
        }

        // Name
        if (gs.nameText) {
            gs.nameText->setText(hasItem ? gearDisplayName(item) : "");
            gs.nameText->setVisible(hasItem);
        }
    }
}

void FguiHud::refreshPerks(const std::vector<PerkData>& perks) {
    for (int i = 0; i < kPerkSlotCount; ++i) {
        const auto& ps = _perkSlots[i];
        const bool hasPerk = i < static_cast<int>(perks.size()) && !perks[i].type.empty();

        if (ps.bg) ps.bg->setVisible(hasPerk);
        if (ps.nameText) {
            ps.nameText->setText(hasPerk ? perks[i].type : "");
            ps.nameText->setVisible(hasPerk);
        }
    }
}

void FguiHud::refreshScopes(const std::string& scope, const std::vector<uint16_t>& inventory) {
    for (int i = 0; i < kScopeSlotCount; ++i) {
        const std::string scopeType(kScopeTypeNames[i]);
        int count = 0;
        // Scopes are at indices 20-24 in the inventory (kBagSizeKeys order)
        for (int j = 0; j < static_cast<int>(defs::kBagSizeKeys.size()) && j < static_cast<int>(inventory.size()); ++j) {
            if (defs::kBagSizeKeys[j] == scopeType) {
                count = inventory[j];
                break;
            }
        }

        const bool visible = count > 0;
        const bool equipped = visible && scope == scopeType;

        const auto& ss = _scopeSlots[i];
        if (ss.bg) {
            ss.bg->setVisible(visible);
        }
        if (ss.labelText) {
            // Show scope zoom level as label
            std::string label = scopeType;
            if (scopeType == "1xscope") label = "1x";
            else if (scopeType == "2xscope") label = "2x";
            else if (scopeType == "4xscope") label = "4x";
            else if (scopeType == "8xscope") label = "8x";
            else if (scopeType == "15xscope") label = "15x";
            if (equipped) label += "*";
            ss.labelText->setText(visible ? label : "");
            ss.labelText->setVisible(visible);
        }
    }
}

void FguiHud::refreshKillMessage(const std::string& text, const std::string& count) {
    // Text
    if (_killMessageText) {
        _killMessageText->setText(text);
    }
    if (_killMessageCount) {
        _killMessageCount->setText(count);
    }
    const bool visible = !text.empty() || !count.empty();
    if (_killMessageBg) _killMessageBg->setVisible(visible);
    if (_killMessageText) _killMessageText->setVisible(visible);
    if (_killMessageCount) _killMessageCount->setVisible(visible);
}

void FguiHud::refreshPickupMessage(const std::string& text) {
    if (_pickupMessageText) {
        _pickupMessageText->setText(text);
        _pickupMessageText->setVisible(!text.empty());
    }
}

void FguiHud::displayPickupMessage(uint8_t type) {
    _pickupMessage = getPickupText(type);
    _pickupTicker = 0.0f;
    _pickupDuration = 3.0f;
}

void FguiHud::displayKillMessage(const std::string& text, const std::string& count) {
    _killMsgText = text;
    _killMsgCount = count;
    _killMsgTicker = 0.0f;
    _killMsgDuration = 7.0f;
}

void FguiHud::refreshAlive(int count, int teamCount) {
    if (!_aliveText) return;
    std::string text = std::to_string(count) + " alive";
    _aliveText->setText(text);
}

void FguiHud::refreshKillFeed(const std::string& text) {
    if (!_killFeedText) return;
    _killFeedText->setText(text);
}

void FguiHud::refreshResult(const std::string& text) {
    if (!_resultText) return;
    _resultText->setText(text);
}

void FguiHud::showResult(bool show) {
    if (_resultPanel) _resultPanel->setVisible(show);
    if (_resultText) _resultText->setVisible(show);
    _resultShown = show;
}

void FguiHud::setVisible(bool visible) {
    _visible = visible;
    if (_hudPanel) _hudPanel->setVisible(visible);
}

bool FguiHud::isVisible() const {
    return _visible;
}

// ---- Main update: called each frame -------------------------------------
void FguiHud::update(float dt, bool started, bool playing, const Game* game) {
    if (!_hudPanel || !_visible) return;
    if (!started || !playing || !game) return;

    const GameStateSnapshot snap = game->snapshot();
    const ActivePlayerData& apd = game->activePlayerData();

    // --- Message tickers ---

    // Pickup message timer
    if (!_pickupMessage.empty()) {
        _pickupTicker += dt;
        if (_pickupTicker >= _pickupDuration) {
            _pickupMessage.clear();
            refreshPickupMessage("");
        } else {
            refreshPickupMessage(_pickupMessage);
        }
    }

    // Kill message timer
    if (!_killMsgText.empty()) {
        _killMsgTicker += dt;
        if (_killMsgTicker >= _killMsgDuration) {
            _killMsgText.clear();
            _killMsgCount.clear();
            refreshKillMessage("", "");
        } else {
            refreshKillMessage(_killMsgText, _killMsgCount);
        }
    }

    // --- Health/boost ---
    refreshHealth(snap.health);
    refreshBoost(apd.boost);

    // --- Weapon + ammo (compact) ---
    std::string weaponName;
    int ammo = -1;
    int reserve = -1;
    if (apd.curWeapIdx >= 0 && static_cast<size_t>(apd.curWeapIdx) < apd.weapons.size()) {
        const auto& weapon = apd.weapons[apd.curWeapIdx];
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

    // --- Inventory v2: weapon cards, gear, perks, scopes, loot ---
    refreshWeaponSlots(apd.weapons, apd.curWeapIdx);
    refreshGear(apd.chest, apd.helmet, apd.backpack);
    refreshPerks(apd.perks);
    refreshScopes(apd.scope, apd.inventory);
    refreshLootCounts(apd.inventory);

    // --- Alive count ---
    const auto counts = game->aliveCounts();
    const int totalAlive = std::accumulate(counts.begin(), counts.end(), 0);
    refreshAlive(totalAlive, static_cast<int>(counts.size()));

    // --- Kill feed ---
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

    // --- Squad health bars ---
    refreshSquad(game);

    // --- Game result ---
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
        showResult(over->teamRank > 0);
    } else {
        showResult(false);
    }
}

// ---- Squad refresh (kept from previous) -----------------------------------
void FguiHud::refreshSquad(const Game* game) {
    if (!game) return;
    const std::vector<uint16_t> teamIds = game->getTeamPlayerIds();
    const auto& playerStatusMap = game->allPlayerStatus();
    const auto& playerInfoMap = game->allPlayerInfo();
    const auto& groupStatusVec = game->groupStatus();

    const bool hasTeam = teamIds.size() > 1;
    if (!_squadMembers.empty() && _squadMembers[0].container) {
        auto* parent = _squadMembers[0].container->getParent();
        if (parent) parent->setVisible(hasTeam && game->isPlaying());
    }

    for (size_t i = 0; i < _squadMembers.size(); ++i) {
        auto& m = _squadMembers[i];
        if (!m.container) continue;

        if (!hasTeam || i >= teamIds.size()) {
            m.container->setVisible(false);
            continue;
        }

        m.container->setVisible(true);
        const uint16_t pid = teamIds[i];

        const auto* info = playerInfoMap.count(pid) ? &playerInfoMap.at(pid) : nullptr;
        const std::string& name = info ? info->name : "";
        m.nameText->setText(name.empty() ? "Player" : name);

        float health = 100.0f;
        bool disconnected = false;
        if (i < groupStatusVec.size()) {
            health = groupStatusVec[i].health;
            disconnected = groupStatusVec[i].disconnected;
        }
        const auto* ps = playerStatusMap.count(pid) ? &playerStatusMap.at(pid) : nullptr;
        const bool dead = ps ? ps->dead : false;
        const bool downed = ps ? ps->downed : false;

        const float fillWidth = kSquadBarWidth * std::clamp(health / 100.0f, 0.0f, 1.0f);
        m.healthBar->clear();
        const uint32_t barColor = dead    ? 0xff444444 :
                                   downed ? 0xffffaa00 :
                                            0xff4caf50;
        m.healthBar->drawRect(std::max(fillWidth, 0.0f), kSquadBarHeight, 0,
                              Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                              Color4F(((barColor >> 16) & 0xff) / 255.0f,
                                      ((barColor >> 8) & 0xff) / 255.0f,
                                      (barColor & 0xff) / 255.0f, 1.0f));
        m.healthBar->setVisible(fillWidth > 0.0f);

        std::string status;
        if (disconnected) status = "DC";
        else if (dead) status = "Dead";
        else if (downed) status = "Down";
        m.statusText->setText(status);
        m.statusText->setVisible(!status.empty());

        m.prevHealth = static_cast<int>(health);
        m.prevDead = dead;
        m.prevDowned = downed;
        m.prevDisconnected = disconnected;
    }
}

} // namespace surv

#endif // SURV_UI_NO_AXMOL