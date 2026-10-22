#ifdef SURV_UI_NO_AXMOL
// Host build without axmol/FGUI; stubs below are empty.
#else

#include "FguiHud.h"
#include "../game/Game.h"
#include "../game/Map.h"
#include "../game/Gas.h"
#include "../game/objects/MapIndicator.h"
#include "../render/Defs.h"
#include "../ui/Localization.h"
#include "../app/DevConfig.h"
#include "Layout.h"
#include "../app/GameScene.h"
#include "FguiRoot.h"
#include "FguiText.h"

namespace {
using fairygui::GComponent;
} // namespace
#include "../core/GameConfig.h"
#include "../net/Messages.h"

#include "fairygui/GRoot.h"
#include "fairygui/GComponent.h"
#include "fairygui/GGraph.h"
#include "fairygui/GTextField.h"
#include "fairygui/GRichTextField.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <numeric>
#include <cstdlib>
#include <cstdlib>
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
FguiHud::~FguiHud() { freeTree(); }

// Detaches the whole FairyGUI tree: every widget created in build() (the
// top-level blocks, plus the retained frame) is a child of _hudPanel, which the
// HUD retains, so dropping that reference releases all of them.
void FguiHud::freeTree() {
    _emoteWheel.reset();
    for (auto*& obj : _fgOwned) {
        if (obj) {
            auto* g = dynamic_cast<fairygui::GObject*>(obj);
            if (g && g->getParent()) {
                g->removeFromParent();
            }
            obj->release();
            obj = nullptr;
        }
    }
    _fgOwned.clear();
    if (_hudPanel) {
        if (_hudPanel->getParent()) {
            _hudPanel->removeFromParent();
        }
        _hudPanel->release();
        _hudPanel = nullptr;
    }
    _healthText = nullptr;
    _healthFill = nullptr;
    _healthDepleted = nullptr;
    for (auto*& seg : _boostSegments) {
        seg = nullptr;
    }
    _weaponText = nullptr;
    _ammoText = nullptr;
    _aliveText = nullptr;
    _killFeedText = nullptr;
    _killMessageBg = nullptr;
    _killMessageText = nullptr;
    _killMessageCount = nullptr;
    _pickupMessageText = nullptr;
    _interactionBg = nullptr;
    _interactionKeyText = nullptr;
    _interactionDescText = nullptr;
    _resultText = nullptr;
    _resultPanel = nullptr;
    _minimapContainer = nullptr;
    _minimapFrame = nullptr;
    _minimapGasRing = nullptr;
    _minimapSafeZone = nullptr;
    _minimapDots.clear();
    _bottomCenter = nullptr;
    _scopePanel = nullptr;
    _weaponPanel = nullptr;
    _gearPanel = nullptr;
    _perkPanel = nullptr;
    _lootPanel = nullptr;
    for (auto& ws : _weaponSlots) ws = {};
    for (auto& gs : _gearSlots) gs = {};
    for (auto& ps : _perkSlots) ps = {};
    for (auto& ss : _scopeSlots) ss = {};
    for (auto& ls : _lootSlots) ls = {};
    for (auto& m : _squadMembers) m = {};
}

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

// FairyGUI ownership (FguiOwnership.h): a GComponent does not release its
// children, so addChild() takes over the create() reference and the tree is
// owned by `_hudPanel`. Without this the widgets were freed when the frame
// that built them ended, and the first HUD update crashed in
// GObject::setVisible().
fairygui::GObject* FguiHud::addChild(fairygui::GComponent* parent, fairygui::GObject* child) {
    fgui::adopt(parent, child);
    return child;
}

// Retains an object that is not a child of the HUD root (e.g. the minimap
// frame, positioned next to the minimap container).
fairygui::GObject* FguiHud::keep(fairygui::GObject* obj) {
    _fgOwned.push_back(obj);
    return obj;
}

void FguiHud::build(ax::Scene* scene) {
    // The scene's GRoot (GameScene: GRoot::create(this, 500)).
    // GRoot::getInstance() is only valid while the root is running, which it is
    // not yet when the HUD is built from enterWithJoin().
    fairygui::GRoot* root = ::surv::fguiRoot();
    if (!root) return;

    // HUD-disable switch on debug builds (`surv_debug` == 1, see dev::kKeyDebug).
    // The HUD build is the last known device crash site; this allows a match
    // without it while that is being fixed.
    if (ax::UserDefault::getInstance()->getStringForKey(dev::kKeyDebug, "0") == "1") {
        return;
    }

    // An empty GComponent as the HUD root. Must not be attached to the scene:
    // on the Android target the GRoot's scene-graph queue crashes when a new
    // FairyGUI component enters it at runtime (see M4_REMAINING.md).
    freeTree();
    _hudPanel = fairygui::GComponent::create();
    _hudPanel->retain();

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

    // Anchor every top-level block the way the web HUD's CSS boxes are placed.
    if (_hudPanel) {
        layoutAnchors(_hudPanel);
    }

    // Emote wheel (built once, shown/hidden on trigger).
    _emoteWheel = std::make_unique<surv::FguiEmoteWheel>();
    _emoteWheel->build(scene);

    _hudPanel->setVisible(false);
}

// ---- Health + Boost (ui-bottom-center-0 in the web HUD) -------------------
// Web sizes (client/css/game.css): #ui-health-container is 400px wide and
// #ui-health-counter is 32px tall with 6px padding; #ui-health-actual is the
// health fill and #ui-health-depleted (behind it) the lost-health trail.
static constexpr float kHealthBarWidth = 400.0f;
static constexpr float kHealthBarHeight = 32.0f;
static constexpr float kHealthBarPadding = 6.0f;
// #ui-boost-counter segments in game.css (mobile) are 8px tall.
static constexpr float kBoostBarHeight = 8.0f;

void FguiHud::buildHudPanel(fairygui::GComponent* root) {
    const float hudW = kHealthBarWidth + kHealthBarPadding * 2.0f;
    const float hudH = kHealthBarHeight + kHealthBarPadding * 2.0f;

    // ui-bottom-center-0 container: health counter + boost counter.
    fairygui::GComponent* bottomCenter = surv::fgui::created(fairygui::GComponent::create());
    bottomCenter->setSize(hudW, hudH);
    _hudPanel = bottomCenter;

    // #ui-health-counter background (rgba(0,0,0,0.4), 5px radius).
    fairygui::GGraph* counterBg = surv::fgui::created(fairygui::GGraph::create());
    counterBg->drawRect(hudW, hudH, 0,
                        Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                        Color4F(0.0f, 0.0f, 0.0f, 0.4f));
    counterBg->setPivot(0.0f, 0.0f);
    counterBg->setPosition(0.0f, 0.0f);
    fgui::adopt(bottomCenter, counterBg);
    fgui::releaseCreated(counterBg);

    // #ui-health-depleted: the trail left behind by damage.
    fairygui::GGraph* depleted = surv::fgui::created(fairygui::GGraph::create());
    depleted->drawRect(kHealthBarWidth, kHealthBarHeight, 0,
                       Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                       Color4F(0.55f, 0.55f, 0.55f, 0.9f));
    depleted->setPivot(0.0f, 0.0f);
    depleted->setPosition(kHealthBarPadding, kHealthBarPadding);
    fgui::adopt(bottomCenter, depleted);
    fgui::releaseCreated(depleted);
    _healthDepleted = depleted;

    // #ui-health-actual: the health fill.
    fairygui::GGraph* healthFill = surv::fgui::created(fairygui::GGraph::create());
    healthFill->drawRect(kHealthBarWidth, kHealthBarHeight, 0,
                         Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                         Color4F(0.7f, 0.7f, 0.7f, 1.0f));
    healthFill->setPivot(0.0f, 0.0f);
    healthFill->setPosition(kHealthBarPadding, kHealthBarPadding);
    fgui::adopt(bottomCenter, healthFill);
    fgui::releaseCreated(healthFill);
    _healthFill = healthFill;

    // Health number centred over the bar.
    surv::fgui::TextField* healthTf = surv::fgui::created(surv::fgui::TextField::create());
    healthTf->setText("100");
    healthTf->setColor(Color3B(255, 255, 255));
    healthTf->setFontSize(18.0f);
    healthTf->setSingleLine(true);
    healthTf->setPosition(hudW * 0.5f - (healthTf->label() ? healthTf->label()->getContentSize().width * 0.5f : 0.0f),
                          hudH * 0.5f + (healthTf->label() ? healthTf->label()->getContentSize().height * 0.5f : 0.0f));
    fgui::adopt(bottomCenter, healthTf);
    fgui::releaseCreated(healthTf);
    _healthText = healthTf;

    // Boost counter: 4 segments (boostBreakpoints 1 / 1 / 1.5 / 0.5, total 4)
    // drawn above the health bar like #ui-boost-counter.
    for (int i = 0; i < kBoostBarCount; ++i) {
        fairygui::GGraph* seg = surv::fgui::created(fairygui::GGraph::create());
        seg->setPivot(0.0f, 0.0f);
        seg->setPosition(kHealthBarPadding, -8.0f - kBoostBarHeight);
        seg->setVisible(false);
        fgui::adopt(bottomCenter, seg);
        fgui::releaseCreated(seg);
        _boostSegments[i] = seg;
    }

    // Compact weapon/ammo text: the web HUD shows these in #ui-equipped-ammo
    // above the health bar; keep them in the same block.
    surv::fgui::TextField* weaponTf = surv::fgui::created(surv::fgui::TextField::create());
    weaponTf->setText("");
    weaponTf->setColor(Color3B(220, 220, 220));
    weaponTf->setFontSize(kSmallFontSize);
    weaponTf->setSingleLine(true);
    weaponTf->setPivot(0.5f, 1.0f);
    weaponTf->setPosition(hudW * 0.5f, -30.0f);
    fgui::adopt(bottomCenter, weaponTf);
    fgui::releaseCreated(weaponTf);
    _weaponText = weaponTf;

    surv::fgui::TextField* ammoTf = surv::fgui::created(surv::fgui::TextField::create());
    ammoTf->setText("");
    ammoTf->setColor(Color3B(255, 255, 255));
    ammoTf->setFontSize(16.0f);
    ammoTf->setSingleLine(true);
    ammoTf->setPivot(0.5f, 1.0f);
    ammoTf->setPosition(hudW * 0.5f, -48.0f);
    fgui::adopt(bottomCenter, ammoTf);
    fgui::releaseCreated(ammoTf);
    _ammoText = ammoTf;

    fgui::adopt(_hudPanel, bottomCenter);
    _bottomCenter = bottomCenter;
}

// ---- Weapon slots (4 cards: primary, secondary, melee, throwable) --------
void FguiHud::buildWeaponSlots(fairygui::GComponent* root) {
    const float panelW = kWeaponSlotW + kPanelPadding * 2;
    const float panelH = kWeaponSlotCount * (kWeaponSlotH + kWeaponSlotGap) + kPanelPadding * 2;

    fairygui::GComponent* panel = surv::fgui::created(fairygui::GComponent::create());
    fairygui::GGraph* panelBg = surv::fgui::created(fairygui::GGraph::create());
    panelBg->drawRect(panelW, panelH, 0,
                      Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                      Color4F(0.0f, 0.0f, 0.0f, 0.30f));
    panelBg->setPivot(0.0f, 0.0f);
    panelBg->setPosition(0.0f, 0.0f);
    fgui::adopt(panel, panelBg);
    fgui::releaseCreated(panelBg);
    panel->setPivot(1.0f, 1.0f); // bottom-right anchored
    panel->setPosition(-4.0f, -4.0f);
    _weaponPanel = panel;

    for (int i = 0; i < kWeaponSlotCount; ++i) {
        const float y = kPanelPadding + static_cast<float>(i) * (kWeaponSlotH + kWeaponSlotGap);

        // Background rect
        fairygui::GGraph* bg = surv::fgui::created(fairygui::GGraph::create());
        bg->drawRect(kWeaponSlotW, kWeaponSlotH, 0,
                     Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                     Color4F(0.08f, 0.08f, 0.08f, 0.7f));
        bg->setPosition(kPanelPadding, y);
        bg->setPivot(0.0f, 0.0f);
        fgui::adopt(panel, bg);
        fgui::releaseCreated(bg);
        _weaponSlots[i].bg = bg;

        // Equipped highlight bar (left edge, shown when equipped)
        fairygui::GGraph* hl = surv::fgui::created(fairygui::GGraph::create());
        hl->drawRect(4.0f, kWeaponSlotH, 0,
                     Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                     Color4F(0.0f, 0.7f, 1.0f, 0.9f));
        hl->setPosition(kPanelPadding, y);
        hl->setPivot(0.0f, 0.0f);
        hl->setVisible(false);
        fgui::adopt(panel, hl);
        fgui::releaseCreated(hl);
        _weaponSlots[i].equippedHighlight = hl;

        // Keybind (slot number)
    surv::fgui::TextField* kbTf = surv::fgui::created(surv::fgui::TextField::create());
    kbTf->setText(std::to_string(i + 1));
    kbTf->setColor(Color3B(180, 180, 180));
    kbTf->setFontSize(kTinyFontSize);
    kbTf->setSingleLine(true);
        kbTf->setPosition(kPanelPadding + 6.0f, y + 1.0f);
        kbTf->setPivot(0.0f, 0.0f);
        fgui::adopt(panel, kbTf);
        fgui::releaseCreated(kbTf);
        _weaponSlots[i].keybindText = kbTf;

        // Weapon name
    surv::fgui::TextField* nameTf = surv::fgui::created(surv::fgui::TextField::create());
    nameTf->setText("");
    nameTf->setColor(Color3B(220, 220, 220));
    nameTf->setFontSize(kSmallFontSize);
    nameTf->setSingleLine(true);
        nameTf->setPosition(kPanelPadding + 18.0f, y + 2.0f);
        nameTf->setPivot(0.0f, 0.0f);
        fgui::adopt(panel, nameTf);
        fgui::releaseCreated(nameTf);
        _weaponSlots[i].nameText = nameTf;

        // Ammo count
    surv::fgui::TextField* ammoTf = surv::fgui::created(surv::fgui::TextField::create());
    ammoTf->setText("");
    ammoTf->setColor(Color3B(255, 255, 200));
    ammoTf->setFontSize(kTinyFontSize);
    ammoTf->setSingleLine(true);
        ammoTf->setPosition(kPanelPadding + kWeaponSlotW - 35.0f, y + 2.0f);
        ammoTf->setPivot(0.0f, 0.0f);
        fgui::adopt(panel, ammoTf);
        fgui::releaseCreated(ammoTf);
        _weaponSlots[i].ammoText = ammoTf;
    }

    panel->setPivot(1.0f, 1.0f);
    panel->setPosition(-4.0f, -4.0f);
    fgui::adopt(_hudPanel, panel);
}

// ---- Gear slots (helmet, chest, backpack) --------------------------------
void FguiHud::buildGearSlots(fairygui::GComponent* root) {
    const float panelW = kGearSlotW + kPanelPadding * 2 + 10.0f;
    const float panelH = kGearSlotCount * (kGearSlotH + 2) + kPanelPadding * 2;

    fairygui::GComponent* panel = surv::fgui::created(fairygui::GComponent::create());
    fairygui::GGraph* panelBg = surv::fgui::created(fairygui::GGraph::create());
    panelBg->drawRect(panelW, panelH, 0,
                      Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                      Color4F(0.0f, 0.0f, 0.0f, 0.25f));
    panelBg->setPivot(0.0f, 0.0f);
    panelBg->setPosition(0.0f, 0.0f);
    fgui::adopt(panel, panelBg);
    fgui::releaseCreated(panelBg);
    panel->setPivot(1.0f, 1.0f);

    for (int i = 0; i < kGearSlotCount; ++i) {
        const float y = kPanelPadding + static_cast<float>(i) * (kGearSlotH + 2.0f);

        fairygui::GGraph* bg = surv::fgui::created(fairygui::GGraph::create());
        bg->drawRect(kGearSlotW, kGearSlotH, 0,
                     Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                     Color4F(0.1f, 0.1f, 0.1f, 0.7f));
        bg->setPosition(kPanelPadding, y);
        bg->setPivot(0.0f, 0.0f);
        fgui::adopt(panel, bg);
        fgui::releaseCreated(bg);
        _gearSlots[i].bg = bg;

        // Level indicator
    surv::fgui::TextField* lvlTf = surv::fgui::created(surv::fgui::TextField::create());
    lvlTf->setText("");
    lvlTf->setColor(Color3B(255, 255, 255));
    lvlTf->setFontSize(kTinyFontSize);
    lvlTf->setSingleLine(true);
        lvlTf->setPosition(kPanelPadding + 2.0f, y + 1.0f);
        lvlTf->setPivot(0.0f, 0.0f);
        fgui::adopt(panel, lvlTf);
        fgui::releaseCreated(lvlTf);
        _gearSlots[i].levelText = lvlTf;

        // Name
    surv::fgui::TextField* nameTf = surv::fgui::created(surv::fgui::TextField::create());
    nameTf->setText("");
    nameTf->setColor(Color3B(200, 200, 200));
    nameTf->setFontSize(kSmallFontSize);
    nameTf->setSingleLine(true);
        nameTf->setPosition(kPanelPadding + 22.0f, y + 1.0f);
        nameTf->setPivot(0.0f, 0.0f);
        fgui::adopt(panel, nameTf);
        fgui::releaseCreated(nameTf);
        _gearSlots[i].nameText = nameTf;
    }

    // Position relative to weapon panel (left of weapons)
    panel->setPivot(1.0f, 1.0f);
    panel->setPosition(-kWeaponSlotW - 2 * kPanelPadding - 12.0f, -4.0f);
    fgui::adopt(_hudPanel, panel);
    _gearPanel = panel;
}

// ---- Perk slots (up to 4) ------------------------------------------------
void FguiHud::buildPerkSlots(fairygui::GComponent* root) {
    const float panelW = kPerkSlotW + kPanelPadding * 2;
    const float panelH = kPerkSlotCount * (kPerkSlotH + 2) + kPanelPadding * 2;

    fairygui::GComponent* panel = surv::fgui::created(fairygui::GComponent::create());
    fairygui::GGraph* panelBg = surv::fgui::created(fairygui::GGraph::create());
    panelBg->drawRect(panelW, panelH, 0,
                      Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                      Color4F(0.0f, 0.0f, 0.0f, 0.25f));
    panelBg->setPivot(0.0f, 0.0f);
    panelBg->setPosition(0.0f, 0.0f);
    fgui::adopt(panel, panelBg);
    fgui::releaseCreated(panelBg);
    panel->setPivot(1.0f, 1.0f);

    for (int i = 0; i < kPerkSlotCount; ++i) {
        const float y = kPanelPadding + static_cast<float>(i) * (kPerkSlotH + 2.0f);

        fairygui::GGraph* bg = surv::fgui::created(fairygui::GGraph::create());
        bg->drawRect(kPerkSlotW, kPerkSlotH, 0,
                     Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                     Color4F(0.12f, 0.12f, 0.12f, 0.7f));
        bg->setPosition(kPanelPadding, y);
        bg->setPivot(0.0f, 0.0f);
        fgui::adopt(panel, bg);
        fgui::releaseCreated(bg);
        _perkSlots[i].bg = bg;

    surv::fgui::TextField* nameTf = surv::fgui::created(surv::fgui::TextField::create());
    nameTf->setText("");
    nameTf->setColor(Color3B(200, 200, 100));
    nameTf->setFontSize(kTinyFontSize);
    nameTf->setSingleLine(true);
        nameTf->setPosition(kPanelPadding + 2.0f, y + 1.0f);
        nameTf->setPivot(0.0f, 0.0f);
        fgui::adopt(panel, nameTf);
        fgui::releaseCreated(nameTf);
        _perkSlots[i].nameText = nameTf;
    }

    // Position left of gear panel
    panel->setPivot(1.0f, 1.0f);
    const float gearOffset = kGearSlotW + 2.0f * kPanelPadding + 12.0f;
    const float weaponOffset = kWeaponSlotW + 2.0f * kPanelPadding;
    panel->setPosition(-(weaponOffset + gearOffset + kPerkSlotW + 4.0f), -4.0f);
    fgui::adopt(_hudPanel, panel);
    _perkPanel = panel;
}

// ---- Scope slots (5: 1x–15x) ---------------------------------------------
void FguiHud::buildScopeSlots(fairygui::GComponent* root) {
    const float totalW = kScopeSlotCount * (kScopeSlotW + 2.0f) + kPanelPadding * 2;
    const float panelH = kScopeSlotH + kPanelPadding * 2;

    fairygui::GComponent* panel = surv::fgui::created(fairygui::GComponent::create());
    fairygui::GGraph* panelBg = surv::fgui::created(fairygui::GGraph::create());
    panelBg->drawRect(totalW, panelH, 0,
                      Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                      Color4F(0.0f, 0.0f, 0.0f, 0.20f));
    panelBg->setPivot(0.0f, 0.0f);
    panelBg->setPosition(0.0f, 0.0f);
    fgui::adopt(panel, panelBg);
    fgui::releaseCreated(panelBg);
    panel->setPivot(0.5f, 1.0f);
    panel->setPosition(0.0f, 0.0f);

    for (int i = 0; i < kScopeSlotCount; ++i) {
        const float x = kPanelPadding + static_cast<float>(i) * (kScopeSlotW + 2.0f);

        fairygui::GGraph* bg = surv::fgui::created(fairygui::GGraph::create());
        bg->drawRect(kScopeSlotW, kScopeSlotH, 0,
                     Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                     Color4F(0.1f, 0.1f, 0.1f, 0.6f));
        bg->setPosition(x, kPanelPadding);
        bg->setPivot(0.0f, 0.0f);
        bg->setVisible(false);
        fgui::adopt(panel, bg);
        fgui::releaseCreated(bg);
        _scopeSlots[i].bg = bg;

    surv::fgui::TextField* labelTf = surv::fgui::created(surv::fgui::TextField::create());
    labelTf->setText("");
    labelTf->setColor(Color3B(200, 200, 200));
    labelTf->setFontSize(kTinyFontSize);
    labelTf->setSingleLine(true);
        labelTf->setPosition(x + 1.0f, kPanelPadding + 1.0f);
        labelTf->setPivot(0.0f, 0.0f);
        labelTf->setVisible(false);
        fgui::adopt(panel, labelTf);
        fgui::releaseCreated(labelTf);
        _scopeSlots[i].labelText = labelTf;
    }

    // Position at top center
    panel->setPivot(0.5f, 0.0f);
    panel->setPosition(0.0f, 4.0f);
    fgui::adopt(_hudPanel, panel);
    _scopePanel = panel;
}

// ---- Loot grid (ammo/heals/throwables) -----------------------------------
void FguiHud::buildLootGrid(fairygui::GComponent* root) {
    const int cols = 5;
    const int rows = (kLootGridCount + cols - 1) / cols;
    const float panelW = static_cast<float>(cols) * (kLootCellW + 2.0f) + kPanelPadding * 2;
    const float panelH = static_cast<float>(rows) * (kLootCellH + 1.0f) + kPanelPadding * 2;

    fairygui::GComponent* panel = surv::fgui::created(fairygui::GComponent::create());
    fairygui::GGraph* panelBg = surv::fgui::created(fairygui::GGraph::create());
    panelBg->drawRect(panelW, panelH, 0,
                      Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                      Color4F(0.0f, 0.0f, 0.0f, 0.20f));
    panelBg->setPivot(0.0f, 0.0f);
    panelBg->setPosition(0.0f, 0.0f);
    fgui::adopt(panel, panelBg);
    fgui::releaseCreated(panelBg);
    panel->setPivot(1.0f, 0.0f);

    for (int i = 0; i < kLootGridCount; ++i) {
        const int col = i % cols;
        const int row = i / cols;
        const float x = kPanelPadding + static_cast<float>(col) * (kLootCellW + 2.0f);
        const float y = kPanelPadding + static_cast<float>(row) * (kLootCellH + 1.0f);

        fairygui::GGraph* bg = surv::fgui::created(fairygui::GGraph::create());
        bg->drawRect(kLootCellW, kLootCellH, 0,
                     Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                     Color4F(0.08f, 0.08f, 0.08f, 0.5f));
        bg->setPosition(x, y);
        bg->setPivot(0.0f, 0.0f);
        bg->setVisible(false);
        fgui::adopt(panel, bg);
        fgui::releaseCreated(bg);
        _lootSlots[i].bg = bg;

    surv::fgui::TextField* nameTf = surv::fgui::created(surv::fgui::TextField::create());
    nameTf->setText("");
    nameTf->setColor(Color3B(180, 180, 180));
    nameTf->setFontSize(kTinyFontSize);
    nameTf->setSingleLine(true);
        nameTf->setPosition(x + 1.0f, y);
        nameTf->setPivot(0.0f, 0.0f);
        nameTf->setVisible(false);
        fgui::adopt(panel, nameTf);
        fgui::releaseCreated(nameTf);
        _lootSlots[i].nameText = nameTf;

    surv::fgui::TextField* countTf = surv::fgui::created(surv::fgui::TextField::create());
    countTf->setText("");
    countTf->setColor(Color3B(255, 255, 255));
    countTf->setFontSize(kTinyFontSize);
    countTf->setSingleLine(true);
        countTf->setPosition(x + kLootCellW - 18.0f, y);
        countTf->setPivot(0.0f, 0.0f);
        countTf->setVisible(false);
        fgui::adopt(panel, countTf);
        fgui::releaseCreated(countTf);
        _lootSlots[i].countText = countTf;
    }

    // Position right side, below scope slots
    panel->setPivot(1.0f, 0.0f);
    panel->setPosition(-4.0f, 30.0f);
    fgui::adopt(_hudPanel, panel);
    _lootPanel = panel;
}

// ---- Kill message (center screen) ----------------------------------------
void FguiHud::buildKillMessage(fairygui::GComponent* root) {
    fairygui::GComponent* bg = surv::fgui::created(fairygui::GComponent::create());
    fairygui::GGraph* bgGraph = surv::fgui::created(fairygui::GGraph::create());
    bgGraph->drawRect(kKillMsgW, kKillMsgH, 0,
                      Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                      Color4F(0.0f, 0.0f, 0.0f, 0.0f)); // fully transparent, just a container
    bgGraph->setPivot(0.0f, 0.0f);
    bgGraph->setPosition(0.0f, 0.0f);
    fgui::adopt(bg, bgGraph);
    fgui::releaseCreated(bgGraph);
    bg->setPivot(0.5f, 0.5f);
    bg->setPosition(0.0f, 0.0f);
    bg->setVisible(false);
    fgui::adopt(_hudPanel, bg);
    _killMessageBg = bg;

    surv::fgui::TextField* textTf = surv::fgui::created(surv::fgui::TextField::create());
    textTf->setText("");
    textTf->setColor(Color3B(255, 200, 50));
    textTf->setFontSize(22.0f);
    textTf->setSingleLine(false);
    textTf->setPosition(-kKillMsgW * 0.5f + 10.0f, -kKillMsgH * 0.5f + 4.0f);
    textTf->setPivot(0.0f, 0.0f);
    textTf->setVisible(false);
    fgui::adopt(bg, textTf);
    fgui::releaseCreated(textTf);
    _killMessageText = textTf;

    surv::fgui::TextField* countTf = surv::fgui::created(surv::fgui::TextField::create());
    countTf->setText("");
    countTf->setColor(Color3B(255, 255, 255));
    countTf->setFontSize(28.0f);
    countTf->setSingleLine(true);
    countTf->setPosition(kKillMsgW * 0.5f - 50.0f, -kKillMsgH * 0.5f + 4.0f);
    countTf->setPivot(1.0f, 0.0f);
    countTf->setVisible(false);
    fgui::adopt(bg, countTf);
    fgui::releaseCreated(countTf);
    _killMessageCount = countTf;
}

// ---- Pickup message (below center) ---------------------------------------
void FguiHud::buildPickupMessage(fairygui::GComponent* root) {
    surv::fgui::TextField* tf = surv::fgui::created(surv::fgui::TextField::create());
    tf->setText("");
    tf->setColor(Color3B(255, 200, 100));
    tf->setFontSize(kSmallFontSize);
    tf->setSingleLine(true);
    tf->setPivot(0.5f, 1.0f);
    tf->setPosition(0.0f, -70.0f);
    tf->setVisible(false);
    fgui::adopt(_hudPanel, tf);
    _pickupMessageText = tf;
}

// ---- Interaction prompt --------------------------------------------------
void FguiHud::buildInteractionPrompt(fairygui::GComponent* root) {
    const float promptW = 200.0f;
    const float promptH = 26.0f;

    fairygui::GComponent* bg = surv::fgui::created(fairygui::GComponent::create());
    fairygui::GGraph* bgGraph = surv::fgui::created(fairygui::GGraph::create());
    bgGraph->drawRect(promptW, promptH, 0,
                      Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                      Color4F(0.0f, 0.0f, 0.0f, 0.0f));
    bgGraph->setPivot(0.0f, 0.0f);
    bgGraph->setPosition(0.0f, 0.0f);
    fgui::adopt(bg, bgGraph);
    fgui::releaseCreated(bgGraph);
    bg->setPivot(0.5f, 1.0f);
    bg->setPosition(0.0f, -100.0f);
    bg->setVisible(false);
    fgui::adopt(_hudPanel, bg);
    _interactionBg = bg;

    surv::fgui::TextField* keyTf = surv::fgui::created(surv::fgui::TextField::create());
    keyTf->setText("");
    keyTf->setColor(Color3B(255, 200, 50));
    keyTf->setFontSize(kSmallFontSize);
    keyTf->setSingleLine(true);
    keyTf->setPosition(-promptW * 0.5f + 4.0f, 2.0f);
    keyTf->setPivot(0.0f, 0.0f);
    keyTf->setVisible(false);
    fgui::adopt(bg, keyTf);
    fgui::releaseCreated(keyTf);
    _interactionKeyText = keyTf;

    surv::fgui::TextField* descTf = surv::fgui::created(surv::fgui::TextField::create());
    descTf->setText("");
    descTf->setColor(Color3B(200, 200, 200));
    descTf->setFontSize(kSmallFontSize);
    descTf->setSingleLine(true);
    descTf->setPosition(-promptW * 0.5f + 30.0f, 2.0f);
    descTf->setPivot(0.0f, 0.0f);
    descTf->setVisible(false);
    fgui::adopt(bg, descTf);
    fgui::releaseCreated(descTf);
    _interactionDescText = descTf;
}

// ---- Alive counter -------------------------------------------------------
void FguiHud::buildAliveCounter(fairygui::GComponent* root) {
    surv::fgui::TextField* aliveTf = surv::fgui::created(surv::fgui::TextField::create());
    aliveTf->setText("");
    aliveTf->setColor(Color3B(255, 255, 255));
    aliveTf->setFontSize(20.0f);
    aliveTf->setSingleLine(true);
    aliveTf->setPosition(-160.0f, 6.0f);
    aliveTf->setPivot(1.0f, 0.0f);
    fgui::adopt(_hudPanel, aliveTf);
    _aliveText = aliveTf;
}

// ---- HUD anchoring -------------------------------------------------------
// Every top-level element is placed with the uiRoot's pivot on its anchor, i.e.
// position (0,0) is the element's top-left corner inside the 1280x720 design
// rect (the panel's own pivot picks which corner it hangs from). This mirrors
// the web client's absolute CSS boxes (#ui-top-left, #ui-bottom-center-0, ...).
void FguiHud::layoutAnchors(fairygui::GComponent* hudRoot) {
    const float vw = ::ui::kit::designWidth();
    const float vh = ::ui::kit::designHeight();
    auto place = [vw, vh](fairygui::GComponent* c, float px, float py) {
        if (!c) return;
        c->setPivot(px, py);
        c->setPosition(px * vw, py * vh);
    };

    // ui-bottom-center-0: health + boost, 12px above the bottom edge (the
    // game.css mobile rule scales it to 0.85 from the bottom centre).
    if (_hudPanel) {
        _hudPanel->setPivot(0.5f, 1.0f);
        _hudPanel->setPosition(vw * 0.5f, vh - 12.0f);
        _hudPanel->setScale(0.85f, 0.85f);
    }
    // ui-top-left: squad bars (only shown for teams).
    for (auto& m : _squadMembers) {
        if (m.container && m.container->getParent()) {
            m.container->getParent()->setPivot(0.0f, 0.0f);
            m.container->getParent()->setPosition(12.0f, 12.0f);
        }
        break;
    }
    // ui-top-center: scope ladder.
    if (_scopePanel) {
        _scopePanel->setPivot(0.5f, 0.0f);
        _scopePanel->setPosition(vw * 0.5f, 8.0f);
    }
    // ui-alive counter sits left of the minimap (ui-top-right area).
    if (_aliveText) {
        _aliveText->setPivot(1.0f, 0.0f);
        _aliveText->setPosition(vw - 160.0f, 40.0f);
    }
    // ui-killfeed: below the alive counter, right-aligned.
    if (_killFeedText) {
        _killFeedText->setPivot(1.0f, 0.0f);
        _killFeedText->setPosition(vw - 20.0f, 70.0f);
    }
    // ui-bottom-right: weapon cards, 4px off the corner.
    if (_weaponPanel) {
        _weaponPanel->setPivot(1.0f, 1.0f);
        _weaponPanel->setPosition(vw - 8.0f, vh - 8.0f);
    }
    // ui-bottom-center-right: gear (left of the weapon cards).
    if (_gearPanel) {
        _gearPanel->setPivot(1.0f, 1.0f);
        _gearPanel->setPosition(vw - 8.0f - (kWeaponSlotW + 2.0f * kPanelPadding) - 8.0f, vh - 8.0f);
    }
    if (_perkPanel) {
        _perkPanel->setPivot(1.0f, 1.0f);
        _perkPanel->setPosition(
            vw - 8.0f - (kWeaponSlotW + 2.0f * kPanelPadding) - (kGearSlotW + 2.0f * kPanelPadding + 10.0f) - 16.0f,
            vh - 8.0f);
    }
    // ui-right-center: loot grid, right edge, vertically centred.
    if (_lootPanel) {
        _lootPanel->setPivot(1.0f, 0.5f);
        _lootPanel->setPosition(vw - 8.0f, vh * 0.5f);
    }
    (void)place;
}

// ---- Kill feed (multi-line) ----------------------------------------------
void FguiHud::buildKillFeed(fairygui::GComponent* root) {
    surv::fgui::TextField* killTf = surv::fgui::created(surv::fgui::TextField::create());
    killTf->setText("");
    killTf->setColor(Color3B(255, 200, 150));
    killTf->setFontSize(kSmallFontSize);
    killTf->setSingleLine(false);
    killTf->setPosition(-160.0f, 34.0f);
    killTf->setPivot(1.0f, 0.0f);
    fgui::adopt(_hudPanel, killTf);
    _killFeedText = killTf;
}

// ---- Result overlay ------------------------------------------------------
void FguiHud::buildResultOverlay(fairygui::GComponent* root) {
    fairygui::GComponent* panel = surv::fgui::created(fairygui::GComponent::create());
    fairygui::GGraph* panelBg = surv::fgui::created(fairygui::GGraph::create());
    panelBg->drawRect(kResultWidth, kResultHeight, 2,
                      Color4F(1.0f, 1.0f, 1.0f, 1.0f),
                      Color4F(0.0f, 0.0f, 0.0f, 0.85f));
    panelBg->setPivot(0.0f, 0.0f);
    panelBg->setPosition(0.0f, 0.0f);
    fgui::adopt(panel, panelBg);
    fgui::releaseCreated(panelBg);
    panel->setPivot(0.5f, 0.5f);
    panel->setPosition(0.0f, 0.0f);
    panel->setVisible(false);
    fgui::adopt(_hudPanel, panel);
    _resultPanel = panel;

    surv::fgui::TextField* resultTf = surv::fgui::created(surv::fgui::TextField::create());
    resultTf->setText("");
    resultTf->setColor(Color3B(255, 220, 100));
    resultTf->setFontSize(22.0f);
    resultTf->setSingleLine(false);
    resultTf->setPosition(-kResultWidth / 2.0f + 20.0f, -kResultHeight / 2.0f + 30.0f);
    resultTf->setPivot(0.0f, 0.0f);
    resultTf->setVisible(false);
    fgui::adopt(panel, resultTf);
    fgui::releaseCreated(resultTf);
    _resultText = resultTf;
}

// ---- Squad health bars ---------------------------------------------------
void FguiHud::buildSquadBars(fairygui::GComponent* root) {
    fairygui::GComponent* panel = surv::fgui::created(fairygui::GComponent::create());
    fairygui::GGraph* panelBg = surv::fgui::created(fairygui::GGraph::create());
    panelBg->drawRect(kSquadPanelWidth, kSquadPanelHeight, 0,
                      Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                      Color4F(0.0f, 0.0f, 0.0f, 0.4f));
    panelBg->setPivot(0.0f, 0.0f);
    panelBg->setPosition(0.0f, 0.0f);
    fgui::adopt(panel, panelBg);
    fgui::releaseCreated(panelBg);
    panel->setPosition(6.0f, 6.0f);
    panel->setPivot(0.0f, 0.0f);
    panel->setVisible(false);
    fgui::adopt(_hudPanel, panel);

    for (int i = 0; i < 4; ++i) {
        const float y = 4.0f + static_cast<float>(i) * kSquadSlotHeight;

        fairygui::GComponent* slot = surv::fgui::created(fairygui::GComponent::create());
        slot->setPosition(0.0f, y);
        slot->setPivot(0.0f, 0.0f);
        fgui::adopt(panel, slot);
        fgui::releaseCreated(slot);

    surv::fgui::TextField* nameTf = surv::fgui::created(surv::fgui::TextField::create());
    nameTf->setText("");
    nameTf->setColor(Color3B(200, 200, 200));
    nameTf->setFontSize(kSquadNameFontSize);
    nameTf->setSingleLine(true);
        nameTf->setPosition(4.0f, 0.0f);
        nameTf->setPivot(0.0f, 0.0f);
        fgui::adopt(slot, nameTf);
        fgui::releaseCreated(nameTf);

        fairygui::GGraph* barBg = surv::fgui::created(fairygui::GGraph::create());
        barBg->drawRect(kSquadBarWidth, kSquadBarHeight, 0,
                        Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                        Color4F(0.2f, 0.2f, 0.2f, 0.8f));
        barBg->setPosition(4.0f, kSquadNameFontSize + 2.0f);
        barBg->setPivot(0.0f, 0.0f);
        fgui::adopt(slot, barBg);
        fgui::releaseCreated(barBg);

        fairygui::GGraph* healthFill = surv::fgui::created(fairygui::GGraph::create());
        healthFill->drawRect(kSquadBarWidth, kSquadBarHeight, 0,
                             Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                             Color4F(0.3f, 0.8f, 0.3f, 1.0f));
        healthFill->setPosition(4.0f, kSquadNameFontSize + 2.0f);
        healthFill->setPivot(0.0f, 0.0f);
        fgui::adopt(slot, healthFill);
        fgui::releaseCreated(healthFill);

    surv::fgui::TextField* statusTf = surv::fgui::created(surv::fgui::TextField::create());
    statusTf->setText("");
    statusTf->setColor(Color3B(255, 80, 80));
    statusTf->setFontSize(kSquadNameFontSize);
    statusTf->setSingleLine(true);
        statusTf->setPosition(4.0f + kSquadBarWidth + 4.0f, 0.0f);
        statusTf->setPivot(0.0f, 0.0f);
        fgui::adopt(slot, statusTf);
        fgui::releaseCreated(statusTf);

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

// ---- Minimap -------------------------------------------------------------
void FguiHud::buildMinimap(fairygui::GComponent* root) {
    // Container at top-right corner
    // ui2.ts divides the CSS sizes by the device scale (points per CSS px);
    // the same factor is applied to the minimap frame.
    const float ds = _deviceScale;
    const float mmSize = 140.0f / ds;
    const float margin = 8.0f / ds;
    (void)ds;


    fairygui::GComponent* container = surv::fgui::created(fairygui::GComponent::create());
    fairygui::GGraph* containerBg = surv::fgui::created(fairygui::GGraph::create());
    containerBg->drawRect(mmSize, mmSize, 2,
                          Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                          Color4F(0.8f, 0.8f, 0.8f, 1.0f));
    containerBg->setPivot(0.0f, 0.0f);
    containerBg->setPosition(0.0f, 0.0f);
    fgui::adopt(container, containerBg);
    fgui::releaseCreated(containerBg);
    container->setPosition(root->getWidth() - mmSize - margin, margin);
    container->setPivot(0.0f, 0.0f);
    fgui::adopt(_hudPanel, container);
    _minimapContainer = container;

    // Frame border (white outline)
    fairygui::GGraph* frame = surv::fgui::created(fairygui::GGraph::create());
    frame->drawRect(mmSize, mmSize, 2,
                    Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                    Color4F(1.0f, 1.0f, 1.0f, 9.0f / 15.0f));
    frame->setPosition(container->getX(), container->getY());
    frame->setPivot(0.0f, 0.0f);
    fgui::adopt(_hudPanel, frame);
    keep(frame);
    _minimapFrame = frame;

    // Gas ring (dashed circle overlay, same size as minimap)
    fairygui::GGraph* gasRing = surv::fgui::created(fairygui::GGraph::create());
    gasRing->setPosition(mmSize * 0.5f, mmSize * 0.5f);
    gasRing->setVisible(false);
    fgui::adopt(container, gasRing);
    fgui::releaseCreated(gasRing);
    _minimapGasRing = gasRing;

    // Safe zone (solid circle, positioned at safe centre on the minimap)
    fairygui::GGraph* safeZone = surv::fgui::created(fairygui::GGraph::create());
    safeZone->setVisible(false);
    fgui::adopt(container, safeZone);
    fgui::releaseCreated(safeZone);
    _minimapSafeZone = safeZone;
}

// ---- Refresh methods ----------------------------------------------------

void FguiHud::refreshHealth(float health) {
    if (!_healthFill || !_healthText || !_hudPanel) return;

    const bool downed = health < 0.0f;
    if (downed) health = 0.0f;
    health = std::clamp(health, 0.0f, 100.0f);
    const float fraction = health / 100.0f;
    const float fillWidth = kHealthBarWidth * fraction;

    uint32_t color = healthColor(health, downed);
    // GGraph has no clear(); redraw replaces the internal shape.
    _healthFill->setVisible(false);
    _healthFill->drawRect(std::max(fillWidth, 0.0f), kHealthBarHeight, 0,
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

    // 4 segments laid out end to end above the health bar.
    const float segH = kBoostBarHeight;
    const float barTop = -8.0f - kBoostBarHeight;
    float x = kHealthBarPadding;
    const float segWidths[kBoostBarCount] = {
        kHealthBarWidth * (1.0f / kBoostBreakpointsTotal),
        kHealthBarWidth * (1.0f / kBoostBreakpointsTotal),
        kHealthBarWidth * (1.5f / kBoostBreakpointsTotal),
        kHealthBarWidth * (0.5f / kBoostBreakpointsTotal),
    };
    static const uint32_t boostColors[] = {0xffff901a, 0xffff751a, 0xffff6616, 0xffff5600};
    for (int i = 0; i < kBoostBarCount; ++i) {
        const float breakPointT = kBoostBreakpoints[i] / kBoostBreakpointsTotal;
        const float filled = std::min(boostT, breakPointT);
        boostT = std::max(boostT - breakPointT, 0.0f);
        fairygui::GGraph* seg = _boostSegments[i];
        if (!seg) continue;
        const float w = segWidths[i] * (filled / breakPointT);
        const uint32_t c = boostColors[i];
        seg->setVisible(w > 0.5f);
        if (w > 0.5f) {
            seg->drawRect(w, segH, 0, Color4F(0.0f, 0.0f, 0.0f, 0.0f),
                          Color4F(((c >> 16) & 0xff) / 255.0f,
                                  ((c >> 8) & 0xff) / 255.0f,
                                  (c & 0xff) / 255.0f, 1.0f));
        }
        seg->setPosition(x, barTop);
        x += segWidths[i];
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
    if (!_hudPanel) return;
    // The HUD follows the web client's visibility rules: the tree is hidden
    // until the match starts (game.ts `stale`) and stays hidden after leaving.
    const bool show = _visible && started && playing && game;
    if (!show) {
        if (_hudPanel) _hudPanel->setVisible(false);
        return;
    }
    // Each block has its own container in the web HUD; in the native HUD they
    // share the uiRoot, so visibility is driven block by block (setVisible on a
    // GComponent propagates to its children).
    for (auto* block : {_bottomCenter, _scopePanel, _weaponPanel, _gearPanel, _perkPanel, _lootPanel,
                        _minimapContainer}) {
        if (block) block->setVisible(true);
    }



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

    // --- Minimap overlay (gas ring + safe zone + player dots) ---
    refreshMinimap(game);

    // --- Emote wheel per-frame update ---
    if (_emoteWheel) {
        _emoteWheelInput.touched = false; // reset each frame; GameScene sets it
        _emoteWheel->update(_emoteWheelInput, dt);
    }

    // --- Minimap indicators from MapIndicatorBarn ---
    // Render dots on the minimap for indicators in the world.
    // This is a simplified pass: the full port would use MapSpriteBarn
    // with proper positioning via the minimap transform.
}

// ---- Minimap refresh ----------------------------------------------------
void FguiHud::refreshMinimap(const Game* game) {
    if (!_minimapContainer || !_minimapGasRing || !_minimapSafeZone) return;
    if (!game) return;

    // The minimap shows team-player dots by reading game state each frame.
    // The map background is baked into the axmol RenderTexture by
    // Map::generateMinimapTexture; the FGUI layer overlays dynamic shapes.

    // Clear old player dots. removeFromParent() balances the create() retain,
    // so the dots are destroyed here instead of being re-parented.
    for (auto* dot : _minimapDots) {
        dot->removeFromParent();
    }
    _minimapDots.clear();

    // Team player dots (small coloured circles at each team-mate's position).
    const auto& teamIds = game->getTeamPlayerIds();
    const auto& playerStatus = game->allPlayerStatus();
    const uint16_t activeId = game->getActivePlayerId();
    for (uint16_t pid : teamIds) {
        auto it = playerStatus.find(pid);
        if (it == playerStatus.end() || !it->second.visible) continue;

        auto* dot = surv::fgui::created(fairygui::GGraph::create());
        const float dotDiam = 6.0f;
        const uint32_t dotColor = pid == activeId ? 0xff00ff00 : 0xffffffff;
        dot->drawEllipse(dotDiam, dotDiam, 0,
                         Color4F(((dotColor >> 16) & 0xff) / 255.0f,
                                 ((dotColor >> 8) & 0xff) / 255.0f,
                                 (dotColor & 0xff) / 255.0f, 1.0f),
                         Color4F(0.0f, 0.0f, 0.0f, 0.0f));
        fgui::adopt(_minimapContainer, dot);
        fgui::releaseCreated(dot);
        _minimapDots.push_back(dot);
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

        // playerInfoMap containment check
        const surv::PlayerInfo* info = nullptr;
        {
            auto it = playerInfoMap.find(pid);
            if (it != playerInfoMap.end()) {
                info = &it->second;
            }
        }
        const std::string& name = info ? info->name : "";
        m.nameText->setText(name.empty() ? "Player" : name);

        float health = 100.0f;
        bool disconnected = false;
        if (i < groupStatusVec.size()) {
            health = groupStatusVec[i].health;
            disconnected = groupStatusVec[i].disconnected;
        }
        const surv::PlayerStatus* ps = nullptr;
        {
            auto it = playerStatusMap.find(pid);
            if (it != playerStatusMap.end()) {
                ps = &it->second;
            }
        }
        const bool dead = ps ? ps->dead : false;
        const bool downed = ps ? ps->downed : false;

        const float fillWidth = kSquadBarWidth * std::clamp(health / 100.0f, 0.0f, 1.0f);
        // GGraph has no clear(); redraw replaces the internal shape.
        m.healthBar->setVisible(false);
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

// ---- Emote wheel bridge -----------------------------------------------

bool FguiHud::feedEmoteWheel(float screenX, float screenY, bool touched) {
    if (_emoteWheel && _emoteWheel->isVisible()) {
        _emoteWheelInput.screenX = screenX;
        _emoteWheelInput.screenY = screenY;
        _emoteWheelInput.touched = touched;
        return true;
    }
    return false;
}

void FguiHud::setEmoteLoadout(const std::array<std::string, 4>& emotes) {
    for (int i = 0; i < 4; i++) {
        _emoteWheelInput.loadout[i] = emotes[i];
    }
}

bool FguiHud::emoteWheelActive() const {
    return _emoteWheel && _emoteWheel->isVisible();
}

bool FguiHud::consumeEmoteOutput(std::string& outType, std::string& outId, Vec2* worldPos) {
    (void)worldPos;
    if (_emoteWheelInput.send) {
        outType = _emoteWheelInput.selectedEmote.empty() ? "ping" : "emote";
        outId = _emoteWheelInput.selectedEmote.empty() ? _emoteWheelInput.selectedPing
                                                       : _emoteWheelInput.selectedEmote;
        _emoteWheelInput.send = false;
        return !outId.empty();
    }
    return false;
}

} // namespace surv

#endif // SURV_UI_NO_AXMOL







