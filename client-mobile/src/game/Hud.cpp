#include "Hud.h"

#include "Game.h"
#include "EmoteManager.h"
#include "../net/generatedDefs.inc"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <numeric>

namespace surv {

namespace {
// Health colour steps (port of ui2.ts healthColourSteps).
struct HealthStep {
    float health;
    uint8_t r, g, b;
};
const HealthStep kHealthSteps[] = {
    {100, 179, 179, 179},
    {100, 255, 255, 255},
    {75, 255, 255, 255},
    {75, 255, 158, 158},
    {25, 255, 82, 82},
    {25, 255, 0, 0},
    {0, 255, 0, 0},
};
// gameConfig.player.boostBreakpoints = [1, 1, 1.5, 0.5] (0..4 span).
const float kBoostBreakpoints[4] = {1.0f, 1.0f, 1.5f, 0.5f};
constexpr float kBoostBreakpointsTotal = 4.0f;
const uint32_t kBoostColors[4] = {0xff901a, 0xff751a, 0xff6616, 0xff5600};
} // namespace

uint32_t Hud::healthColor(float health, bool downed) {
    if (downed) {
        return 0xff0000;
    }
    const int n = static_cast<int>(sizeof(kHealthSteps) / sizeof(kHealthSteps[0]));
    const int h = static_cast<int>(std::ceil(health));
    // ui2.ts walks the steps from the top: `m_healthColourSteps` starts at the
    // 75% entry (white), then 75 pink, 25 red, 0 red.
    int end = 0;
    while (end < n - 1 && kHealthSteps[end].health >= h) {
        ++end;
    }
    const int first = end > 0 ? end - 1 : 0;
    const auto& a = kHealthSteps[first];
    const auto& b = kHealthSteps[end];
    const float t = b.health > a.health
        ? std::clamp((health - a.health) / (b.health - a.health), 0.0f, 1.0f)
        : 0.0f;
    const auto mix = [t](uint8_t x, uint8_t y) {
        return static_cast<uint8_t>(static_cast<float>(x) + (static_cast<float>(y) - x) * t + 0.5f);
    };
    return (static_cast<uint32_t>(mix(a.r, b.r)) << 16) |
           (static_cast<uint32_t>(mix(a.g, b.g)) << 8) |
           static_cast<uint32_t>(mix(a.b, b.b));
}

int Hud::bagCount(const std::vector<uint16_t>& inventory, const char* type) {
    if (!type || type[0] == '\0') {
        return 0;
    }
    for (size_t i = 0; i < defs::kBagSizeKeys.size(); ++i) {
        if (defs::kBagSizeKeys[i] == type) {
            return i < inventory.size() ? static_cast<int>(inventory[i]) : 0;
        }
    }
    return 0;
}

std::string Hud::healItemFor(const std::vector<uint16_t>& inventory, float health) {
    const int bandage = bagCount(inventory, "bandage");
    const int healthkit = bagCount(inventory, "healthkit");
    // One quick-use pill for both heal items (the web shows a loot grid; touch
    // gets a single best-item button until the inventory v2 HUD lands). Prefer
    // the small bandage for a top-up, the med kit when badly hurt.
    if (healthkit > 0 && (health < 50.0f || bandage == 0)) {
        return "healthkit";
    }
    if (bandage > 0) {
        return "bandage";
    }
    return healthkit > 0 ? "healthkit" : std::string();
}

std::string Hud::boostItemFor(const std::vector<uint16_t>& inventory, float boost) {
    const int soda = bagCount(inventory, "soda");
    const int pills = bagCount(inventory, "painkiller");
    // Soda is the cheap top-up; pills are worth it at low boost (ui2.ts shows
    // both strips and lets the player pick, the touch HUD uses one button).
    if (pills > 0 && (boost < 50.0f || soda == 0)) {
        return "painkiller";
    }
    if (soda > 0) {
        return "soda";
    }
    return pills > 0 ? "painkiller" : std::string();
}

std::string Hud::scopeLabel(const std::string& scope) {
    if (scope.empty()) {
        return "SCOPE";
    }
    std::string out = scope;
    const std::string suffix = "scope";
    if (out.size() > suffix.size() &&
        out.compare(out.size() - suffix.size(), suffix.size(), suffix) == 0) {
        out.resize(out.size() - suffix.size());
    }
    for (char& c : out) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return "SCOPE " + out;
}

const std::string& Hud::weaponType(int slot) const {
    static const std::string kEmpty;
    return (slot >= 0 && slot < 4) ? _weaponTypes[static_cast<size_t>(slot)] : kEmpty;
}

Hud::TapResult Hud::hitTest(float designX, float designY) const {
    if (!_built || !_visible || _overShown || _deathShown || _spectating || _bigMapVisible) {
        return TapResult{};
    }
    // Rects are stored in the centred HUD container.
    const float px = designX - _design.x * 0.5f;
    const float py = designY - _design.y * 0.5f;
    if (_reloadRect.contains(px, py)) {
        return TapResult{TapKind::Reload, -1};
    }
    if (_interactRect.contains(px, py)) {
        return TapResult{TapKind::Interact, -1};
    }
    // Item pills are only tappable while the pill is shown (empty bag hides it).
    if (!_healItem.empty() && _healRect.contains(px, py)) {
        return TapResult{TapKind::Heal, -1};
    }
    if (!_boostItem.empty() && _boostRect.contains(px, py)) {
        return TapResult{TapKind::Boost, -1};
    }
    if (!_scopeType.empty() && _scopeRect.contains(px, py)) {
        return TapResult{TapKind::ScopeNext, -1};
    }
    if (_swapRect.contains(px, py)) {
        return TapResult{TapKind::SwapWeapons, -1};
    }
    for (int i = 0; i < 4; ++i) {
        if (_weaponRect[i].contains(px, py)) {
            return TapResult{TapKind::WeaponSlot, i};
        }
    }
    return TapResult{};
}

void Hud::build(pix::Factory* factory, pix::Container* root, const Vec2& design) {
    _factory = factory;
    _root = root;
    _design = design;
    if (!_factory || !_root) {
        return;
    }

    // #ui-bottom-center-0: health counter (game.css: 400x32 bar, 6px padding,
    // rgba(0,0,0,0.4) background).
    _healthBg = _factory->createGraphics();
    _root->addChild(_healthBg);
    _healthFill = _factory->createGraphics();
    _root->addChild(_healthFill);
    _healthText = _factory->createText();
    _healthText->setFontSize(18.0f);
    _healthText->setColor(0xffffff, 0x000000, 2.0f, true);
    _healthText->setAnchor(0.5f, 0.5f);
    _root->addChild(_healthText);

    // #ui-boost-counter: four segments above the health bar.
    for (auto*& seg : _boostSegments) {
        seg = _factory->createGraphics();
        seg->setAnchor(0.0f, 1.0f);
        _root->addChild(seg);
    }

    // #ui-equipped-ammo: clip / reserve, right of the health bar.
    _ammoText = _factory->createText();
    _ammoText->setFontSize(20.0f);
    _ammoText->setColor(0xffffff, 0x000000, 2.0f, true);
    _ammoText->setAnchor(0.0f, 1.0f);
    _root->addChild(_ammoText);

    // #ui-map-wrapper: minimap frame + alive / kill counters (top right).
    // The baked map texture is attached by setMinimap() once the MapMsg lands.
    _mapFrame = _factory->createGraphics();
    _root->addChild(_mapFrame);
    _aliveText = _factory->createText();
    _aliveText->setFontSize(16.0f);
    _aliveText->setColor(0xffffff, 0x000000, 2.0f, true);
    _aliveText->setAnchor(1.0f, 0.0f);
    _root->addChild(_aliveText);
    _killsText = _factory->createText();
    _killsText->setFontSize(16.0f);
    _killsText->setColor(0xffffff, 0x000000, 2.0f, true);
    _killsText->setAnchor(1.0f, 0.0f);
    _root->addChild(_killsText);
    // #ui-kill-leader-wrapper: name + kill count, top centre (game.ts
    // msg.killLeaderDirty -> uiManager.updateKillLeader).
    _killLeaderText = _factory->createText();
    _killLeaderText->setFontSize(15.0f);
    _killLeaderText->setColor(0xffd060, 0x000000, 2.0f, true);
    _killLeaderText->setAnchor(0.5f, 1.0f);
    _root->addChild(_killLeaderText);

    // #ui-gas-timer: countdown under the minimap (ui.ts updateGasDisplay).
    _gasText = _factory->createText();
    _gasText->setFontSize(14.0f);
    _gasText->setColor(0xffffff, 0x000000, 2.0f, true);
    _gasText->setAnchor(1.0f, 0.0f);
    _gasText->setVisible(false);
    _root->addChild(_gasText);

    // Result screen (ui.ts displayStats): dim backdrop + centred panel.
    _overBackdrop = _factory->createGraphics();
    _overBackdrop->beginFill(0x000000, 0.55f);
    _overBackdrop->drawRect(-_design.x * 0.5f, -_design.y * 0.5f, _design.x, _design.y);
    _overBackdrop->endFill();
    _overBackdrop->setLocalZOrder(950);
    _overBackdrop->setVisible(false);
    _root->addChild(_overBackdrop);
    const float panelW = 460.0f;
    const float panelH = 230.0f;
    _overPanel = _factory->createGraphics();
    _overPanel->beginFill(0x101010, 0.85f);
    _overPanel->drawRect(-panelW * 0.5f, -panelH * 0.5f, panelW, panelH);
    _overPanel->endFill();
    _overPanel->lineStyle(2.0f, 0xffffff, 0.6f);
    _overPanel->drawRect(-panelW * 0.5f, -panelH * 0.5f, panelW, panelH);
    _overPanel->setLocalZOrder(951);
    _overPanel->setVisible(false);
    _root->addChild(_overPanel);
    _overTitle = _factory->createText();
    _overTitle->setFontSize(30.0f);
    _overTitle->setColor(0xffffff, 0x000000, 2.0f, true);
    _overTitle->setAnchor(0.5f, 1.0f);
    _overTitle->setPosition(0.0f, panelH * 0.5f - 18.0f);
    _overTitle->setLocalZOrder(952);
    _overTitle->setVisible(false);
    _root->addChild(_overTitle);
    _overRank = _factory->createText();
    _overRank->setFontSize(20.0f);
    _overRank->setColor(0xffd060, 0x000000, 2.0f, true);
    _overRank->setAnchor(0.5f, 1.0f);
    _overRank->setPosition(0.0f, panelH * 0.5f - 58.0f);
    _overRank->setLocalZOrder(952);
    _overRank->setVisible(false);
    _root->addChild(_overRank);
    _overStats = _factory->createText();
    _overStats->setFontSize(16.0f);
    _overStats->setColor(0xffffff, 0x000000, 1.0f, false);
    _overStats->setAnchor(0.5f, 1.0f);
    _overStats->setPosition(0.0f, panelH * 0.5f - 96.0f);
    _overStats->setLocalZOrder(952);
    _overStats->setVisible(false);
    _root->addChild(_overStats);

    // P0.4 overlay buttons: the final result has LEAVE; the team-death panel has
    // SPECTATE + LEAVE. Positions are panel-local (the panel is centred).
    const auto makeOverlayButton = [this](pix::Graphics*& bg, pix::Text*& text,
                                          float fontSize) {
        bg = _factory->createGraphics();
        bg->setLocalZOrder(953);
        _root->addChild(bg);
        text = _factory->createText();
        text->setFontSize(fontSize);
        text->setColor(0xffffff, 0x000000, 1.5f, false);
        text->setAnchor(0.5f, 0.5f);
        text->setLocalZOrder(954);
        _root->addChild(text);
    };
    makeOverlayButton(_overSpectateBg, _overSpectateText, 18.0f);
    makeOverlayButton(_overLeaveBg, _overLeaveText, 18.0f);
    _overSpectateText->setText("SPECTATE");
    _overLeaveText->setText("LEAVE GAME");
    _overSpectateRect = Rect{-220.0f, -118.0f, 200.0f, 48.0f};
    _overLeaveRect = Rect{20.0f, -118.0f, 200.0f, 48.0f};
    _overSpectateText->setPosition(-120.0f, -94.0f);
    _overLeaveText->setPosition(120.0f, -94.0f);

    // Spectating overlay: label/count at the top centre and the touch options
    // stacked bottom-left (game.css #ui-spectate-options mobile).
    _specLabel = _factory->createText();
    _specLabel->setFontSize(22.0f);
    _specLabel->setColor(0x66ffff, 0x000000, 2.0f, true);
    _specLabel->setAnchor(0.5f, 0.5f);
    _specLabel->setLocalZOrder(920);
    _root->addChild(_specLabel);
    _specCountText = _factory->createText();
    _specCountText->setFontSize(14.0f);
    _specCountText->setColor(0xffd060, 0x000000, 2.0f, true);
    _specCountText->setAnchor(0.5f, 0.5f);
    _specCountText->setLocalZOrder(920);
    _root->addChild(_specCountText);
    makeOverlayButton(_specPrevBg, _specPrevText, 15.0f);
    makeOverlayButton(_specNextBg, _specNextText, 15.0f);
    makeOverlayButton(_specStatsBg, _specStatsTextBtn, 15.0f);
    makeOverlayButton(_specLeaveBg, _specLeaveText, 15.0f);
    _specPrevText->setText("PREV");
    _specNextText->setText("NEXT");
    _specStatsTextBtn->setText("MATCH STATS");
    _specLeaveText->setText("LEAVE GAME");
    _specStatsPanel = _factory->createGraphics();
    _specStatsPanel->setLocalZOrder(920);
    _root->addChild(_specStatsPanel);
    _specStatsText = _factory->createText();
    _specStatsText->setFontSize(15.0f);
    _specStatsText->setColor(0xffffff, 0x000000, 1.0f, false);
    _specStatsText->setAnchor(0.0f, 1.0f);
    _specStatsText->setLocalZOrder(921);
    _root->addChild(_specStatsText);

    // #ui-bottom-right: four weapon cards.
    _weaponBoxBg = _factory->createGraphics();
    _root->addChild(_weaponBoxBg);
    for (int i = 0; i < 4; ++i) {
        _weaponSlotBg[i] = _factory->createGraphics();
        _root->addChild(_weaponSlotBg[i]);
        _weaponSlots[i] = _factory->createText();
        _weaponSlots[i]->setFontSize(13.0f);
        _weaponSlots[i]->setColor(0xdcdcdc, 0x000000, 1.0f, false);
        _weaponSlots[i]->setAnchor(0.0f, 0.0f);
        _root->addChild(_weaponSlots[i]);
    }

    // P0.1 touch action controls: reload, interact, quick-use heal/boost, scope
    // cycle and weapon-slot swap. Labels are written by updateActionButtons().
    const auto makeButton = [this](pix::Graphics*& bg, pix::Text*& text, float fontSize) {
        bg = _factory->createGraphics();
        _root->addChild(bg);
        text = _factory->createText();
        text->setFontSize(fontSize);
        text->setColor(0xffffff, 0x000000, 1.5f, false);
        text->setAnchor(0.5f, 0.5f);
        _root->addChild(text);
    };
    makeButton(_reloadBg, _reloadText, 15.0f);
    makeButton(_interactBg, _interactText, 18.0f);
    makeButton(_healBg, _healText, 13.0f);
    makeButton(_boostBg, _boostText, 13.0f);
    makeButton(_scopeBg, _scopeText, 13.0f);
    makeButton(_swapBg, _swapText, 13.0f);
    _reloadText->setText("RELOAD");
    _interactText->setText("USE");
    _swapText->setText("SWAP");

    // P0.1 action controls follow the HUD; updateActionButtons() hides the
    // pills whose item is not in the bag.
    _reloadBg->setVisible(false);
    _reloadText->setVisible(false);
    _interactBg->setVisible(false);
    _interactText->setVisible(false);
    _healBg->setVisible(false);
    _healText->setVisible(false);
    _boostBg->setVisible(false);
    _boostText->setVisible(false);
    _scopeBg->setVisible(false);
    _scopeText->setVisible(false);
    _swapBg->setVisible(false);
    _swapText->setVisible(false);

    // #ui-bottom-center-2 pickup message and #ui-killfeed.
    _pickupText = _factory->createText();
    _pickupText->setFontSize(14.0f);
    _pickupText->setColor(0xffc864, 0x000000, 2.0f, false);
    _pickupText->setAnchor(0.5f, 0.0f);
    _root->addChild(_pickupText);
    _killFeedText = _factory->createText();
    _killFeedText->setFontSize(13.0f);
    _killFeedText->setColor(0xffc896, 0x000000, 1.0f, false);
    _killFeedText->setAnchor(1.0f, 0.0f);
    _root->addChild(_killFeedText);

    // One static layout pass (the web CSS boxes do not move during a match).
    // The HUD container is centred on the viewport (GameScene::resetWorld), and
    // axmol's scene graph is y-up while the CSS boxes are y-down. Work in
    // design-space bottom-left coordinates here and convert to the centred
    // container: X/Y offset from the design centre.
    const float vw = _design.x;
    const float vh = _design.y;
    const auto X = [vw](float x) { return x - vw * 0.5f; };
    const auto Y = [vh](float y) { return y - vh * 0.5f; };

    // #ui-bottom-center-0: 400x32 bar in a 412x44 box, 12px above the bottom.
    const float hudW = kHealthBarWidth + kHealthBarPadding * 2.0f;
    const float hudH = kHealthBarHeight + kHealthBarPadding * 2.0f;
    const float hudX = (vw - hudW) * 0.5f;
    const float hudY = 12.0f;

    _healthBg->clear();
    _healthBg->beginFill(0x000000, 0.4f);
    _healthBg->drawRect(X(hudX), Y(hudY), hudW, hudH);
    _healthBg->endFill();

    _healthFill->setPosition(X(hudX + kHealthBarPadding), Y(hudY + kHealthBarPadding));
    _healthText->setAnchor(0.5f, 0.5f);
    _healthText->setPosition(X(vw * 0.5f), Y(hudY + hudH * 0.5f));
    _ammoText->setAnchor(0.0f, 1.0f);
    _ammoText->setPosition(X(hudX + hudW + 12.0f), Y(hudY + hudH));

    // #ui-map-wrapper: minimap frame + alive/kill counters, top right.
    const float mm = kMinimapSize;
    const float mmX = vw - mm - 12.0f;
    const float mmY = vh - mm - 12.0f;
    _minimapPos = Vec2(X(mmX), Y(mmY));
    _mapFrame->clear();
    _mapFrame->lineStyle(2.0f, 0xffffff, 0.6f);
    _mapFrame->drawRect(X(mmX), Y(mmY), mm, mm);
    _aliveText->setAnchor(1.0f, 1.0f);
    _aliveText->setPosition(X(mmX - 8.0f), Y(vh - 12.0f));
    _killsText->setAnchor(1.0f, 1.0f);
    _killsText->setPosition(X(mmX - 8.0f), Y(vh - 36.0f));
    _killLeaderText->setAnchor(0.5f, 1.0f);
    _killLeaderText->setPosition(X(vw * 0.5f), Y(vh - 8.0f));
    _gasText->setAnchor(1.0f, 0.0f);
    _gasText->setPosition(X(mmX + mm), Y(mmY - 18.0f));

    // #ui-bottom-right: four weapon cards, bottom right.
    const float boxW = kWeaponSlotWidth + 16.0f;
    const float boxH = 4.0f * (kWeaponSlotHeight + 2.0f) + 12.0f;
    const float boxX = vw - boxW - 8.0f;
    const float boxY = 8.0f;
    _weaponBoxBg->clear();
    _weaponBoxBg->beginFill(0x000000, 0.25f);
    _weaponBoxBg->drawRect(X(boxX), Y(boxY), boxW, boxH);
    _weaponBoxBg->endFill();
    for (int i = 0; i < 4; ++i) {
        // CSS order is top -> bottom, so slot 0 sits highest inside the box.
        const float y = boxY + boxH - 6.0f - static_cast<float>(i) * (kWeaponSlotHeight + 2.0f);
        _weaponSlots[i]->setAnchor(0.0f, 1.0f);
        _weaponSlots[i]->setPosition(X(boxX + 8.0f), Y(y));
        // Tap/long-press target: the full card row (ui2.ts ui-weapon-id-N).
        _weaponRect[i] = Rect{X(boxX + 2.0f),
                              Y(y - kWeaponSlotHeight * 0.5f),
                              boxW - 4.0f,
                              kWeaponSlotHeight};
    }

    // P0.1 touch action controls. (cx, cy) are design-space bottom-left points.
    const auto placeButton = [&](pix::Graphics* bg, pix::Text* text, Rect& rect,
                                 float cx, float cy, float w, float h) {
        rect = Rect{X(cx - w * 0.5f), Y(cy - h * 0.5f), w, h};
        text->setPosition(X(cx), Y(cy));
        if (bg) {
            bg->setVisible(false);
        }
        if (text) {
            text->setVisible(false);
        }
    };
    // Reload sits right of the health bar (#ui-reload-button-container).
    placeButton(_reloadBg, _reloadText, _reloadRect, hudX + hudW + 54.0f, 90.0f,
                kReloadButtonWidth, kReloadButtonHeight);
    // Interaction/revive/loot tap target (#ui-interaction, touch layout).
    placeButton(_interactBg, _interactText, _interactRect, vw * 0.3125f, vh * 0.4167f,
                kInteractButtonWidth, kInteractButtonHeight);
    // Quick-use consumables and scope/swap pills, above the left pad and clear
    // of the emote/ping buttons on the right.
    placeButton(_healBg, _healText, _healRect, vw * 0.4766f, vh * 0.4528f,
                kPillWidth, kPillHeight);
    placeButton(_boostBg, _boostText, _boostRect, vw * 0.4766f, vh * 0.3833f,
                kPillWidth, kPillHeight);
    placeButton(_scopeBg, _scopeText, _scopeRect, vw * 0.6016f, vh * 0.4528f,
                kSmallPillWidth, kPillHeight);
    placeButton(_swapBg, _swapText, _swapRect, vw * 0.6016f, vh * 0.3833f,
                kSmallPillWidth, kPillHeight);

    // P0.4 spectating overlay: label/count top-centre and a bottom-left option
    // stack over the (now hidden) movement pad.
    _specLabel->setPosition(X(vw * 0.5f), Y(vh - 64.0f));
    _specCountText->setPosition(X(vw * 0.5f), Y(vh - 94.0f));
    const auto placeOverlayButton = [&](pix::Graphics* bg, pix::Text* text, Rect& rect,
                                        float cx, float cy, float w, float h) {
        rect = Rect{X(cx - w * 0.5f), Y(cy - h * 0.5f), w, h};
        text->setPosition(X(cx), Y(cy));
        if (bg) bg->setVisible(false);
        if (text) text->setVisible(false);
    };
    placeOverlayButton(_specPrevBg, _specPrevText, _specPrevRect, 95.0f, 250.0f, 150.0f, 42.0f);
    placeOverlayButton(_specNextBg, _specNextText, _specNextRect, 95.0f, 200.0f, 150.0f, 42.0f);
    placeOverlayButton(_specStatsBg, _specStatsTextBtn, _specStatsRect, 95.0f, 150.0f, 150.0f,
                       42.0f);
    placeOverlayButton(_specLeaveBg, _specLeaveText, _specLeaveRect, 95.0f, 100.0f, 150.0f,
                       42.0f);
    _specStatsPanel->setVisible(false);
    _specStatsPanel->setPosition(X(185.0f), Y(70.0f));
    _specStatsText->setVisible(false);
    _specStatsText->setPosition(X(195.0f), Y(258.0f));

    const auto paintOverlayButton = [](pix::Graphics* bg, const Rect& rect, uint32_t color) {
        if (!bg) return;
        bg->clear();
        bg->beginFill(color, 0.92f);
        bg->drawRect(rect.x, rect.y, rect.w, rect.h);
        bg->endFill();
        bg->lineStyle(2.0f, 0xffffff, 0.45f);
        bg->drawRect(rect.x, rect.y, rect.w, rect.h);
        bg->setPosition(0.0f, 0.0f);
        bg->setVisible(false);
    };
    paintOverlayButton(_overSpectateBg, _overSpectateRect, 0x50afab);
    paintOverlayButton(_overLeaveBg, _overLeaveRect, 0xcc3333);
    paintOverlayButton(_specPrevBg, _specPrevRect, 0x50afab);
    paintOverlayButton(_specNextBg, _specNextRect, 0x50afab);
    paintOverlayButton(_specStatsBg, _specStatsRect, 0x50afab);
    paintOverlayButton(_specLeaveBg, _specLeaveRect, 0xcc3333);
    if (_specStatsPanel) {
        _specStatsPanel->clear();
        _specStatsPanel->beginFill(0x000000, 0.7f);
        _specStatsPanel->drawRect(0.0f, 0.0f, 250.0f, 190.0f);
        _specStatsPanel->endFill();
        _specStatsPanel->lineStyle(2.0f, 0xffffff, 0.4f);
        _specStatsPanel->drawRect(0.0f, 0.0f, 250.0f, 190.0f);
    }

    // #ui-bottom-center-2: pickup message just below the centre.
    _pickupText->setAnchor(0.5f, 0.0f);
    _pickupText->setPosition(X(vw * 0.5f), Y(vh * 0.12f));
    // #ui-killfeed: right side, above the weapon cards.
    _killFeedText->setAnchor(1.0f, 1.0f);
    _killFeedText->setPosition(X(boxX - 8.0f), Y(boxY + boxH + 8.0f));

    // The boost segments sit above the health block; update() repaints width.
    {
        float x = hudX + kHealthBarPadding;
        for (int i = 0; i < 4; ++i) {
            const float span = kBoostBreakpoints[i] / kBoostBreakpointsTotal;
            _boostSegments[i]->setAnchor(0.0f, 0.0f);
            _boostSegments[i]->setPosition(X(x), Y(hudY + hudH + 4.0f));
            x += kHealthBarWidth * span;
        }
    }

    // --- Minimap contents (map.ts renderMap + ui.ts map sprites) ---
    _miniMap.build(_factory, _root, _minimapPos, kMinimapSize);
    _miniMap.setZoomed(true);
    // #ui-map-expand-mobile: small expand affordance inside the frame.
    _expandPos = Vec2(X(mmX + mm - kMinimapExpandSize - 4.0f), Y(mmY + 4.0f));
    _mapExpandBg = _factory->createGraphics();
    _mapExpandBg->beginFill(0x000000, 0.45f);
    _mapExpandBg->drawRect(0.0f, 0.0f, kMinimapExpandSize, kMinimapExpandSize);
    _mapExpandBg->endFill();
    _mapExpandBg->setPosition(_expandPos.x, _expandPos.y);
    _mapExpandBg->setLocalZOrder(20);
    _root->addChild(_mapExpandBg);
    _mapExpandIcon = _factory->createGraphics();
    _mapExpandIcon->lineStyle(2.0f, 0xffffff, 0.9f);
    _mapExpandIcon->drawRect(5.0f, 5.0f, 12.0f, 12.0f);
    _mapExpandIcon->drawRect(8.0f, 8.0f, 6.0f, 6.0f);
    _mapExpandIcon->setPosition(_expandPos.x, _expandPos.y);
    _mapExpandIcon->setLocalZOrder(21);
    _root->addChild(_mapExpandIcon);

    // --- Full-screen big map (#big-map) ---
    _bigMapPos = Vec2(X((_design.x - kBigMapSize) * 0.5f), Y((_design.y - kBigMapSize) * 0.5f));
    _bigMapBackdrop = _factory->createGraphics();
    _bigMapBackdrop->beginFill(0x000000, 0.62f);
    _bigMapBackdrop->drawRect(-_design.x * 0.5f, -_design.y * 0.5f, _design.x, _design.y);
    _bigMapBackdrop->endFill();
    _bigMapBackdrop->setLocalZOrder(900);
    _root->addChild(_bigMapBackdrop);
    _bigMap.build(_factory, _root, _bigMapPos, kBigMapSize);
    if (_bigMap.root()) {
        _bigMap.root()->setLocalZOrder(901);
    }
    const Vec2 closePos = Vec2(_bigMapPos.x + kBigMapSize + 10.0f, _bigMapPos.y + kBigMapSize - 10.0f);
    _bigMapCloseBg = _factory->createGraphics();
    _bigMapCloseBg->beginFill(0x000000, 0.7f);
    _bigMapCloseBg->drawCircle(0.0f, 0.0f, 18.0f);
    _bigMapCloseBg->endFill();
    _bigMapCloseBg->lineStyle(2.0f, 0xffffff, 0.8f);
    _bigMapCloseBg->drawCircle(0.0f, 0.0f, 18.0f);
    _bigMapCloseBg->setPosition(closePos.x, closePos.y);
    _bigMapCloseBg->setLocalZOrder(902);
    _root->addChild(_bigMapCloseBg);
    _bigMapCloseText = _factory->createText();
    _bigMapCloseText->setFontSize(22.0f);
    _bigMapCloseText->setColor(0xffffff, 0x000000, 0.0f, true);
    _bigMapCloseText->setAnchor(0.5f, 0.5f);
    _bigMapCloseText->setText("X");
    _bigMapCloseText->setPosition(closePos.x, closePos.y);
    _bigMapCloseText->setLocalZOrder(903);
    _root->addChild(_bigMapCloseText);
    setBigMapVisible(false);

    // Gear/perk slots (ui2.ts ui-armor-*/ui-perk-*): item atlas frames along
    // the top-left, above the squad bars.
    for (int i = 0; i < kGearIconCount; ++i) {
        pix::Sprite* icon = _factory->createSprite();
        icon->setAnchor(0.0f, 1.0f);
        icon->setPosition(X(12.0f + static_cast<float>(i) * 32.0f), Y(vh - 12.0f));
        icon->setVisible(false);
        _root->addChild(icon);
        _gearIcons[static_cast<size_t>(i)] = icon;
    }

    // Squad bars (#ui-top-left / FguiHud::buildSquadBars): name, health bar,
    // Dead/Down/DC. Shown only when GameScene supplies team members.
    const float squadX = 12.0f;
    const float squadTop = vh - 54.0f;
    for (int i = 0; i < 4; ++i) {
        SquadRow& row = _squad[static_cast<size_t>(i)];
        const float rowY = squadTop - static_cast<float>(i) * 32.0f;
        row.name = _factory->createText();
        row.name->setFontSize(12.0f);
        row.name->setColor(0xffffff, 0x000000, 1.0f, false);
        row.name->setAnchor(0.0f, 1.0f);
        row.name->setPosition(X(squadX), Y(rowY));
        _root->addChild(row.name);
        row.barBg = _factory->createGraphics();
        row.barBg->beginFill(0x000000, 0.4f);
        row.barBg->drawRect(X(squadX), Y(rowY - 20.0f), 120.0f, 8.0f);
        row.barBg->endFill();
        _root->addChild(row.barBg);
        row.barFill = _factory->createGraphics();
        row.barFill->setPosition(X(squadX), Y(rowY - 20.0f));
        _root->addChild(row.barFill);
        row.status = _factory->createText();
        row.status->setFontSize(11.0f);
        row.status->setColor(0xffd060, 0x000000, 1.0f, false);
        row.status->setAnchor(0.0f, 1.0f);
        row.status->setPosition(X(squadX + 124.0f), Y(rowY));
        _root->addChild(row.status);
        row.name->setVisible(false);
        row.barBg->setVisible(false);
        row.barFill->setVisible(false);
        row.status->setVisible(false);
    }

    _built = true;
    setVisible(false);
}

void Hud::setVisible(bool visible) {
    if (_visible != visible) {
        _actionKey.clear();
    }
    if (!_built) {
        _visible = visible;
        return;
    }
    _visible = visible;
    if (!visible) {
        _bigMapVisible = false;
    }
    const auto show = [visible](pix::Node* node) {
        if (node) {
            node->setVisible(visible);
        }
    };
    show(_healthBg);
    show(_healthFill);
    show(_healthText);
    show(_ammoText);
    show(_mapFrame);
    show(_aliveText);
    show(_killsText);
    show(_killLeaderText);
    if (_gasText) {
        _gasText->setVisible(visible && _gas.active);
    }
    show(_weaponBoxBg);
    show(_killFeedText);
    for (auto* seg : _boostSegments) {
        show(seg);
    }
    for (auto* slot : _weaponSlots) {
        show(slot);
    }
    for (auto* bg : _weaponSlotBg) {
        show(bg);
    }
    // P0.1 action controls; the item pills own their visibility in
    // updateActionButtons() (empty bag = hidden).
    show(_reloadBg);
    show(_reloadText);
    show(_interactBg);
    show(_interactText);
    show(_swapBg);
    show(_swapText);
    if (!visible) {
        show(_pickupText);
    }
    const bool big = visible && _bigMapVisible;
    _miniMap.setVisible(visible && !big);
    _bigMap.setVisible(big);
    if (_bigMapBackdrop) {
        _bigMapBackdrop->setVisible(big);
    }
    if (_bigMapCloseBg) {
        _bigMapCloseBg->setVisible(big);
    }
    if (_bigMapCloseText) {
        _bigMapCloseText->setVisible(big);
    }
    if (_mapExpandBg) {
        _mapExpandBg->setVisible(visible && !big);
    }
    if (_mapExpandIcon) {
        _mapExpandIcon->setVisible(visible && !big);
    }
    for (size_t i = 0; i < _squad.size(); ++i) {
        const bool showRow = visible && i < _squadState.size();
        SquadRow& row = _squad[i];
        if (row.name) {
            row.name->setVisible(showRow);
        }
        if (row.barBg) {
            row.barBg->setVisible(showRow);
        }
        if (row.barFill) {
            row.barFill->setVisible(showRow && _squadState[i].health > 0.0f);
        }
        if (row.status) {
            row.status->setVisible(showRow);
        }
    }
    for (size_t i = 0; i < _gearIcons.size(); ++i) {
        if (_gearIcons[i]) {
            _gearIcons[i]->setVisible(visible && !_gearFrames[i].empty());
        }
    }
    const bool over = visible && (_overShown || _deathShown);
    if (_overBackdrop) {
        _overBackdrop->setVisible(over);
    }
    if (_overPanel) {
        _overPanel->setVisible(over);
    }
    if (_overTitle) {
        _overTitle->setVisible(over);
    }
    if (_overRank) {
        _overRank->setVisible(visible && _overShown);
    }
    if (_overStats) {
        _overStats->setVisible(over);
    }
    if (_overSpectateBg) {
        _overSpectateBg->setVisible(visible && _deathShown && !_overShown);
    }
    if (_overSpectateText) {
        _overSpectateText->setVisible(visible && _deathShown && !_overShown);
    }
    if (_overLeaveBg) {
        _overLeaveBg->setVisible(over);
    }
    if (_overLeaveText) {
        _overLeaveText->setVisible(over);
    }
    // The final result panel has only LEAVE, centred; the death panel has
    // SPECTATE + LEAVE side by side.
    if (_overLeaveText) {
        _overLeaveText->setPosition(_overShown ? 0.0f : 120.0f, -94.0f);
    }
    if (_overLeaveBg) {
        _overLeaveBg->setPosition(_overShown ? -120.0f : 0.0f, 0.0f);
    }
    const bool spec = visible && _spectating && !_overShown && !_deathShown;
    if (_specLabel) {
        _specLabel->setVisible(spec);
    }
    if (_specCountText) {
        _specCountText->setVisible(spec && _specCount > 0);
    }
    if (_specPrevBg) {
        _specPrevBg->setVisible(spec && _specCanCycle);
    }
    if (_specPrevText) {
        _specPrevText->setVisible(spec && _specCanCycle);
    }
    if (_specNextBg) {
        _specNextBg->setVisible(spec && _specCanCycle);
    }
    if (_specNextText) {
        _specNextText->setVisible(spec && _specCanCycle);
    }
    if (_specStatsBg) {
        _specStatsBg->setVisible(spec);
    }
    if (_specStatsTextBtn) {
        _specStatsTextBtn->setVisible(spec);
    }
    if (_specLeaveBg) {
        _specLeaveBg->setVisible(spec);
    }
    if (_specLeaveText) {
        _specLeaveText->setVisible(spec);
    }
    if (_specStatsPanel) {
        _specStatsPanel->setVisible(spec && _specStatsShown);
    }
    if (_specStatsText) {
        _specStatsText->setVisible(spec && _specStatsShown);
    }
    // Item pills own their visibility (empty inventory hides them); this also
    // runs after the big-map/result state changes so they never linger.
    updateActionButtons();
}

void Hud::setMinimap(pix::RenderTexture* texture, float mapWidth, float mapHeight) {
    if (!_built || !texture) {
        return;
    }
    _miniMap.setMapSize(mapWidth, mapHeight);
    _miniMap.setTexture(texture);
    _bigMap.setMapSize(mapWidth, mapHeight);
    _bigMap.setTexture(texture);
    _miniMap.setVisible(_visible && !_bigMapVisible);
}

void Hud::updateMapPings(const std::vector<MapPing>& pings) {
    if (!_built) {
        return;
    }
    _miniMap.setPings(pings);
    _bigMap.setPings(pings);
}

void Hud::updateMapOverlay(const std::vector<MapDot>& dots, const MapGas& gas,
                           const std::vector<MapAirstrike>& strikes) {
    if (!_built) {
        return;
    }
    _miniMap.setDots(dots);
    _bigMap.setDots(dots);
    _miniMap.setGas(gas);
    _bigMap.setGas(gas);
    _miniMap.setAirstrikes(strikes);
    _bigMap.setAirstrikes(strikes);
}

void Hud::setMapLabels(const std::vector<MapPlace>& places) {
    if (!_built) {
        return;
    }
    _miniMap.setLabels(places);
    _bigMap.setLabels(places);
}

void Hud::setMapCenter(const Vec2& worldPos) {
    if (!_built) {
        return;
    }
    _miniMap.setCenter(worldPos);
}

void Hud::setBigMapVisible(bool visible) {
    _bigMapVisible = visible;
    if (!_built) {
        return;
    }
    // Re-run the visibility pass so the minimap/expand button swap with the
    // big map and its backdrop.
    setVisible(_visible);
}

bool Hud::minimapExpandHit(float designX, float designY) const {
    if (!_visible || _bigMapVisible || !_miniMap.visible()) {
        return false;
    }
    // Callers pass design-space coordinates (0..design); the widget positions
    // are in the centred HUD container.
    const Vec2 p(designX - _design.x * 0.5f, designY - _design.y * 0.5f);
    const float pad = 4.0f;
    return p.x >= _expandPos.x - pad && p.x <= _expandPos.x + kMinimapExpandSize + pad &&
           p.y >= _expandPos.y - pad && p.y <= _expandPos.y + kMinimapExpandSize + pad;
}

bool Hud::bigMapContains(float designX, float designY) const {
    return _bigMapVisible &&
           _bigMap.contains(Vec2(designX - _design.x * 0.5f, designY - _design.y * 0.5f));
}

bool Hud::bigMapCloseHit(float designX, float designY) const {
    if (!_bigMapVisible) {
        return false;
    }
    const Vec2 p(designX - _design.x * 0.5f, designY - _design.y * 0.5f);
    const Vec2 c(_bigMapPos.x + kBigMapSize + 10.0f, _bigMapPos.y + kBigMapSize - 10.0f);
    const float dx = p.x - c.x;
    const float dy = p.y - c.y;
    return dx * dx + dy * dy <= 24.0f * 24.0f;
}

void Hud::setSquadState(const std::vector<SquadMember>& members) {
    _squadState = members;
    if (!_built) {
        return;
    }
    for (size_t i = 0; i < _squad.size(); ++i) {
        SquadRow& row = _squad[i];
        if (i >= members.size()) {
            row.name->setVisible(false);
            row.barBg->setVisible(false);
            row.barFill->setVisible(false);
            row.status->setVisible(false);
            row.key.clear();
            continue;
        }
        const SquadMember& m = members[i];
        std::string status;
        if (m.disconnected) {
            status = "DC";
        } else if (m.dead) {
            status = "Dead";
        } else if (m.downed) {
            status = "Down";
        }
        const int health = static_cast<int>(std::clamp(m.health, 0.0f, 100.0f));
        const std::string key = m.name + "|" + std::to_string(health) + "|" + status +
                                "|" + (m.active ? "1" : "0");
        if (key == row.key) {
            continue;
        }
        row.key = key;
        row.name->setText(m.name.empty() ? "Player" : m.name);
        row.status->setText(status);
        const uint32_t color = m.dead ? 0x444444 : (m.downed ? 0xffaa00 : 0x4caf50);
        const float fillWidth = 120.0f * static_cast<float>(health) / 100.0f;
        row.barFill->clear();
        if (fillWidth > 0.0f) {
            row.barFill->beginFill(color, 1.0f);
            row.barFill->drawRect(0.0f, 0.0f, fillWidth, 8.0f);
            row.barFill->endFill();
        }
        const bool showRow = _visible;
        row.name->setVisible(showRow);
        row.barBg->setVisible(showRow);
        row.barFill->setVisible(showRow && fillWidth > 0.0f);
        row.status->setVisible(showRow);
    }
}

void Hud::setGearState(const std::vector<std::string>& frames) {
    if (!_built) {
        return;
    }
    for (size_t i = 0; i < _gearIcons.size(); ++i) {
        const std::string frame = i < frames.size() ? frames[i] : std::string();
        const bool changed = frame != _gearFrames[i];
        _gearFrames[i] = frame;
        pix::Sprite* icon = _gearIcons[i];
        if (!icon) {
            continue;
        }
        if (frame.empty()) {
            icon->setVisible(false);
            continue;
        }
        if (changed) {
            icon->setFrame(frame);
            const float source = std::max(icon->sourceWidth(), icon->sourceHeight());
            const float size = source > 0.0f ? 26.0f / source : 1.0f;
            icon->setScale(size, size);
        }
        icon->setVisible(_visible);
    }
}

void Hud::setGasState(const GasHudState& gas) {
    _gas = gas;
    if (!_built || !_gasText) {
        return;
    }
    std::string text;
    if (gas.active) {
        const int sec = gas.secondsLeft > 0.0f ? static_cast<int>(gas.secondsLeft) : 0;
        text = formatTime(static_cast<uint16_t>(sec));
    }
    if (text != _lastGasText) {
        _lastGasText = text;
        _gasText->setText(text);
        _gasText->setColor(gas.moving ? 0xff6060 : 0xffffff, 0x000000, 2.0f, true);
    }
    _gasText->setVisible(_visible && gas.active);
}

std::string Hud::formatTime(uint16_t seconds) {
    const uint16_t minutes = seconds / 60;
    const uint16_t secs = seconds % 60;
    std::string out = std::to_string(minutes) + ":";
    if (secs < 10) {
        out += "0";
    }
    out += std::to_string(secs);
    return out;
}

void Hud::showGameOver(const GameOverMsg& over, uint8_t localTeamId, uint16_t localPlayerId) {
    if (!_built) {
        return;
    }
    const bool victory = localTeamId != 0 && over.winningTeamId == localTeamId;
    _overTitle->setText(victory ? "VICTORY" : "DEFEAT");
    _overTitle->setColor(victory ? 0x8fff6a : 0xff6a6a, 0x000000, 2.0f, true);
    _overRank->setText("Rank #" + std::to_string(over.teamRank));

    int teamKills = 0;
    const PlayerStatsMsg* local = nullptr;
    for (const auto& stats : over.playerStats) {
        teamKills += stats.kills;
        if (stats.playerId == localPlayerId) {
            local = &stats;
        }
    }
    std::string text = "Team kills: " + std::to_string(teamKills);
    if (local) {
        text += "\nKills: " + std::to_string(local->kills) + "   Damage: " + std::to_string(local->damageDealt);
        text += "\nSurvived: " + formatTime(local->timeAlive);
    }
    _overStats->setText(text);

    _overShown = true;
    _deathShown = false;
    _spectating = false;
    if (_overLeaveText) _overLeaveText->setText("LEAVE GAME");
    _bigMapVisible = false;
    setVisible(_visible);
}

void Hud::showDeathPanel(const PlayerStatsMsg& stats) {
    if (!_built) {
        return;
    }
    _localStats = stats;
    _hasLocalStats = true;
    _overShown = false;
    _deathShown = true;
    _spectating = false;
    if (_overTitle) {
        _overTitle->setText("YOU DIED");
        _overTitle->setColor(0xff6a6a, 0x000000, 2.0f, true);
    }
    if (_overRank) {
        _overRank->setText("");
    }
    std::string text = "Kills: " + std::to_string(stats.kills);
    text += "   Damage: " + std::to_string(stats.damageDealt);
    text += "\nDamage taken: " + std::to_string(stats.damageTaken);
    text += "\nSurvived: " + formatTime(stats.timeAlive);
    if (_overStats) {
        _overStats->setText(text);
    }
    if (_overLeaveText) _overLeaveText->setText("LEAVE GAME");
    _bigMapVisible = false;
    setVisible(_visible);
}

void Hud::toggleSpectateStats() {
    _specStatsShown = !_specStatsShown;
    if (_specStatsText && _hasLocalStats) {
        std::string text = "KILLS  " + std::to_string(_localStats.kills);
        text += "\nDAMAGE  " + std::to_string(_localStats.damageDealt);
        text += "\nTAKEN  " + std::to_string(_localStats.damageTaken);
        text += "\nSURVIVED  " + formatTime(_localStats.timeAlive);
        _specStatsText->setText(text);
    }
    setVisible(_visible);
}

Hud::OverlayAction Hud::overlayActionAt(float designX, float designY) const {
    if (!_built || !_visible) {
        return OverlayAction::None;
    }
    const float px = designX - _design.x * 0.5f;
    const float py = designY - _design.y * 0.5f;
    if (_overShown) {
        const Rect leave{-100.0f, -118.0f, 200.0f, 48.0f};
        return leave.contains(px, py) ? OverlayAction::Leave : OverlayAction::None;
    }
    if (_deathShown) {
        if (_overSpectateRect.contains(px, py)) return OverlayAction::SpectateBegin;
        if (_overLeaveRect.contains(px, py)) return OverlayAction::Leave;
        return OverlayAction::None;
    }
    if (_spectating) {
        if (_specCanCycle && _specPrevRect.contains(px, py)) return OverlayAction::SpectatePrev;
        if (_specCanCycle && _specNextRect.contains(px, py)) return OverlayAction::SpectateNext;
        if (_specStatsRect.contains(px, py)) return OverlayAction::ToggleStats;
        if (_specLeaveRect.contains(px, py)) return OverlayAction::Leave;
    }
    return OverlayAction::None;
}

void Hud::displayPickupMessage(const std::string& text) {
    _lastPickup = text;
    _pickupTicker = text.empty() ? 0.0f : 3.0f;
    if (_pickupText) {
        _pickupText->setText(text);
        _pickupText->setVisible(!text.empty() && _visible);
    }
}

void Hud::updateActionButtons() {
    if (!_built) {
        return;
    }
    const bool base = _visible && !_overShown && !_deathShown && !_spectating &&
        !_bigMapVisible;
    const auto itemLabel = [](const std::string& item) -> std::string {
        if (item == "healthkit") return "MEDKIT";
        if (item == "painkiller") return "PILLS";
        std::string out = item;
        for (char& c : out) {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        return out;
    };
    const std::string healLabel = _healItem.empty()
        ? std::string()
        : itemLabel(_healItem) + " x" + std::to_string(_healCount);
    const std::string boostLabel = _boostItem.empty()
        ? std::string()
        : itemLabel(_boostItem) + " x" + std::to_string(_boostCount);
    const std::string scopeText = scopeLabel(_scopeType);

    std::string key = base ? "1" : "0";
    key += "|" + healLabel + "|" + boostLabel + "|" + scopeText + "|" +
        std::to_string(_equippedSlot);
    if (key == _actionKey && base) {
        return;
    }
    _actionKey = key;

    const auto paint = [this](pix::Graphics* bg, pix::Text* text, const Rect& rect,
                              bool shown, uint32_t fill, float alpha,
                              const std::string& label) {
        if (!bg || !text) {
            return;
        }
        bg->setVisible(shown);
        text->setVisible(shown);
        if (!shown) {
            return;
        }
        bg->clear();
        bg->beginFill(fill, alpha);
        bg->drawRect(0.0f, 0.0f, rect.w, rect.h);
        bg->endFill();
        bg->lineStyle(2.0f, 0xffffff, 0.35f);
        bg->drawRect(0.0f, 0.0f, rect.w, rect.h);
        bg->setPosition(rect.x, rect.y);
        text->setText(label);
    };

    paint(_reloadBg, _reloadText, _reloadRect, base, 0x000000, 0.4f, "RELOAD");
    paint(_interactBg, _interactText, _interactRect, base, 0x000000, 0.4f, "USE");
    paint(_healBg, _healText, _healRect, base && !_healItem.empty(), 0x000000, 0.4f, healLabel);
    paint(_boostBg, _boostText, _boostRect, base && !_boostItem.empty(), 0x000000, 0.4f,
          boostLabel);
    paint(_scopeBg, _scopeText, _scopeRect, base && !_scopeType.empty(), 0x000000, 0.4f,
          scopeText);
    paint(_swapBg, _swapText, _swapRect, base, 0x000000, 0.4f, "SWAP");

    // Weapon cards: dim background, brighter for the equipped slot (ui2.ts
    // weapon-state opacity). The text itself is painted by update().
    for (int i = 0; i < 4; ++i) {
        pix::Graphics* bg = _weaponSlotBg[i];
        if (!bg) {
            continue;
        }
        bg->setVisible(base);
        if (!base) {
            continue;
        }
        const bool equipped = i == _equippedSlot;
        bg->clear();
        bg->beginFill(equipped ? 0xffffff : 0x000000, equipped ? 0.2f : 0.22f);
        bg->drawRect(0.0f, 0.0f, _weaponRect[i].w, _weaponRect[i].h);
        bg->endFill();
        if (equipped) {
            bg->lineStyle(2.0f, 0xffffff, 0.55f);
            bg->drawRect(0.0f, 0.0f, _weaponRect[i].w, _weaponRect[i].h);
        }
        bg->setPosition(_weaponRect[i].x, _weaponRect[i].y);
    }
}

void Hud::update(float dt, const Game& game, bool visible) {
    if (!_built) {
        return;
    }
    setVisible(visible);

    // Result / death / spectate state (ui.ts displayStats, showTeamAd and
    // setSpectating). GameOverMsg latches until the next match resets the HUD.
    const GameOverMsg* over = game.gameOver();
    if (over) {
        if (!_overShown) {
            showGameOver(*over, game.teamIdForPlayer(game.getLocalPlayerId()),
                         game.getLocalPlayerId());
        }
        _deathShown = false;
        _spectating = false;
    } else {
        if (_overShown) {
            _overShown = false;
        }
        const PlayerStatsMsg* stats = game.localStats();
        if (stats && stats->dead) {
            _localStats = *stats;
            _hasLocalStats = true;
        }
        const bool wasSpectating = _spectating;
        const bool spectating = game.isSpectating();
        if (spectating) {
            _spectating = true;
            _deathShown = false;
            if (!wasSpectating) {
                _specStatsShown = false;
            }
            std::string name = "Player";
            const auto it = game.allPlayerInfo().find(game.getActivePlayerId());
            if (it != game.allPlayerInfo().end() && !it->second.name.empty()) {
                name = it->second.name;
            }
            if (name != _specName) {
                _specName = name;
                if (_specLabel) {
                    _specLabel->setText("SPECTATING " + name);
                }
            }
            const uint8_t count = game.activePlayerData().spectatorCount;
            if (count != _specCount) {
                _specCount = count;
                if (_specCountText) {
                    _specCountText->setText(count > 0
                            ? std::to_string(count) + " SPECTATING"
                            : std::string());
                }
            }
            _specCanCycle = game.teamMode() != 1;
        } else {
            _spectating = false;
            if (stats && stats->dead) {
                if (!_deathShown) {
                    showDeathPanel(*stats);
                }
            } else if (_deathShown) {
                _deathShown = false;
            }
        }
    }
    setVisible(_visible);

    // Transient messages fade on their own timer, even while hidden (the web
    // client keeps the state and only hides the DOM).
    if (_pickupTicker > 0.0f) {
        _pickupTicker -= dt;
        if (_pickupTicker <= 0.0f) {
            _lastPickup.clear();
            if (_pickupText) {
                _pickupText->setText(std::string());
            }
        }
    }
    if (!visible) {
        return;
    }

    const GameStateSnapshot snap = game.snapshot();
    const ActivePlayerData& apd = game.activePlayerData();

    // --- Health: fill width = health%, colour from the ui2.ts steps ---
    const bool downed = snap.health < 0.0f;
    const float health = downed ? 0.0f : std::clamp(snap.health, 0.0f, 100.0f);
    if (health != _lastHealth || downed) {
        _lastHealth = health;
        const float fraction = health / 100.0f;
        const uint32_t color = healthColor(health, downed);
        _healthFill->clear();
        if (fraction > 0.0f) {
            _healthFill->beginFill(color, 1.0f);
            _healthFill->drawRect(0.0f, 0.0f, kHealthBarWidth * fraction, kHealthBarHeight);
            _healthFill->endFill();
        }
        _healthText->setText(std::to_string(static_cast<int>(std::round(health))));
    }

    // --- Boost: four segments, filled by their share of 100 ---
    const float boost = std::clamp(apd.boost, 0.0f, 100.0f);
    if (boost != _lastBoost) {
        _lastBoost = boost;
        float left = boost / 100.0f;
        for (int i = 0; i < 4; ++i) {
            const float span = kBoostBreakpoints[i] / kBoostBreakpointsTotal;
            const float filled = std::min(left, span);
            left = std::max(left - span, 0.0f);
            const float w = kHealthBarWidth * filled;
            pix::Graphics* seg = _boostSegments[i];
            seg->clear();
            if (w > 0.5f) {
                seg->beginFill(kBoostColors[i], 1.0f);
                seg->drawRect(0.0f, 0.0f, w, kBoostBarHeight);
                seg->endFill();
            }
        }
    }

    // --- Weapon cards: "N. type" per slot; ammo of the equipped gun ---
    std::string weaponKey;
    std::string ammoText;
    const int slotCount = std::min<int>(4, static_cast<int>(apd.weapons.size()));
    for (int i = 0; i < 4; ++i) {
        if (i < slotCount && !apd.weapons[i].type.empty()) {
            weaponKey += std::to_string(i + 1) + ". " + apd.weapons[i].type;
            if (i == apd.curWeapIdx && apd.weapons[i].ammo >= 0) {
                ammoText = std::to_string(apd.weapons[i].ammo);
            }
        }
        weaponKey += '\n';
    }
    if (weaponKey != _lastWeapon) {
        _lastWeapon = weaponKey;
        size_t start = 0;
        for (int i = 0; i < 4; ++i) {
            const size_t end = weaponKey.find('\n', start);
            _weaponSlots[i]->setText(
                weaponKey.substr(start, end == std::string::npos ? std::string::npos : end - start));
            start = end == std::string::npos ? weaponKey.size() : end + 1;
        }
    }
    _ammoText->setText(ammoText);

    // --- P0.1 action controls: equipped slot, quick-use items, scopes ---
    _weaponTypes.fill(std::string());
    for (int i = 0; i < 4; ++i) {
        if (i < slotCount) {
            _weaponTypes[static_cast<size_t>(i)] = apd.weapons[static_cast<size_t>(i)].type;
        }
    }
    _equippedSlot = apd.curWeapIdx;
    _healItem = healItemFor(apd.inventory, health);
    _boostItem = boostItemFor(apd.inventory, boost);
    _healCount = _healItem.empty() ? 0 : bagCount(apd.inventory, _healItem.c_str());
    _boostCount = _boostItem.empty() ? 0 : bagCount(apd.inventory, _boostItem.c_str());
    _scopeType.clear();
    if (!apd.scope.empty()) {
        _scopeType = apd.scope;
    } else {
        for (const auto& key : defs::kBagSizeKeys) {
            const std::string name(key);
            if (name.size() > 5 && name.compare(name.size() - 5, 5, "scope") == 0 &&
                bagCount(apd.inventory, name.c_str()) > 0) {
                _scopeType = name;
                break;
            }
        }
    }
    updateActionButtons();

    // --- Alive counter and kill counter (#ui-leaderboard-wrapper) ---
    const auto counts = game.aliveCounts();
    const int alive = std::accumulate(counts.begin(), counts.end(), 0);
    if (alive != _lastAlive) {
        _lastAlive = alive;
        _aliveText->setText(std::to_string(alive));
    }
    int kills = 0;
    if (const GameOverMsg* over = game.gameOver()) {
        for (const auto& stats : over->playerStats) {
            if (stats.playerId == game.getLocalPlayerId()) {
                kills = stats.kills;
            }
        }
    }
    if (kills != _lastKills) {
        _lastKills = kills;
        _killsText->setText(std::to_string(kills) + " K");
    }

    // --- Kill leader (game.ts msg.killLeaderDirty) ---
    std::string leaderText;
    if (const uint16_t leaderId = game.killLeaderId()) {
        const auto& infos = game.allPlayerInfo();
        const auto it = infos.find(leaderId);
        const std::string name = it != infos.end() ? it->second.name : std::string("Kill Leader");
        leaderText = "Kill Leader: " + name + " (" + std::to_string(game.killLeaderKills()) + ")";
    }
    if (leaderText != _lastKillLeader) {
        _lastKillLeader = leaderText;
        _killLeaderText->setText(leaderText);
    }

    // --- Kill feed (#ui-killfeed) ---
    std::string feed;
    for (const auto& entry : game.killFeed()) {
        feed += entry.killerName.empty() ? entry.targetName
                                         : entry.killerName + " -> " + entry.targetName;
        feed += '\n';
    }
    if (feed != _lastFeed) {
        _lastFeed = feed;
        _killFeedText->setText(feed);
    }
}

} // namespace surv
