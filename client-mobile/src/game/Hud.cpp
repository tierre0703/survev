#include "Hud.h"

#include "Game.h"
#include "EmoteManager.h"
#include "../core/GameConfig.h"
#include "../net/generatedDefs.inc"
#include "../render/Defs.h"

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

// Top bag counters (#ui-medical-interactive + #ui-loot-soda/#ui-loot-painkiller).
const char* kTopLootTypes[Hud::kTopLootCount] = {"bandage", "healthkit", "soda", "painkiller"};

// #ui-ammo-interactive rows, game.css overlay colours per ammo type.
struct AmmoTypeStyle {
    const char* type;
    uint32_t color;
};
const AmmoTypeStyle kAmmoTypes[Hud::kAmmoRowCount] = {
    {"12gauge", 0xff0000},
    {"9mm", 0xff9900},
    {"762mm", 0x0066ff},
    {"556mm", 0x037b00},
    {"45acp", 0x7900ff},
    {"308sub", 0x8e8e00},
    {"50AE", 0x606060},
    {"flare", 0xff5500},
};

// Scope strip (#ui-top-center-scopes-wrapper): one pill per scope level.
const char* kScopeTypes[Hud::kScopeSlotCount] = {
    "1xscope", "2xscope", "4xscope", "8xscope", "15xscope"};

// ui.ts updateHealthBar squad palette (the team bars, not the main bar):
// grey at 100, white at 75..99, red for downed/<=25.
constexpr uint32_t kSquadHealthGrey = 0xb3b3b3; // rgb(179,179,179)
constexpr uint32_t kSquadHealthWhite = 0xffffff;
constexpr uint32_t kSquadHealthRed = 0xff0000;

std::string uppercase(std::string out) {
    for (char& c : out) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return out;
}

// ui2.ts item label ("MEDKIT" for healthkit, "PILLS" for painkiller).
std::string itemLabel(const std::string& item) {
    if (item == "healthkit") return "MEDKIT";
    if (item == "painkiller") return "PILLS";
    return uppercase(item);
}

// GameObjRenderDef img sprite for inventory/loot icons ("" without defs).
std::string itemIconFrame(const std::string& type) {
    const DefProvider* defs = getDefProvider();
    const GameObjRenderDef* def = defs ? defs->gameObject(type) : nullptr;
    return def ? def->img.sprite : std::string();
}
} // namespace

uint32_t Hud::squadHealthColor(float health, bool dead, bool downed) {
    if (dead) {
        return 0x444444;
    }
    if (health > 25.0f) {
        if (downed) {
            return kSquadHealthRed;
        }
        if (std::fabs(health - 100.0f) < 0.2f) {
            return kSquadHealthGrey;
        }
        if (std::fabs(health - 75.0f) < 0.2f || health >= 75.0f) {
            return kSquadHealthWhite;
        }
        // ui.ts Interpolate(darkpink(255,45,45), lightpink(255,112,112), 45, health).
        const int g = static_cast<int>(std::floor(45.0f + ((112.0f - 45.0f) / 45.0f) * health));
        const uint32_t channel = static_cast<uint32_t>(std::clamp(g, 0, 255));
        return (255u << 16) | (channel << 8) | channel;
    }
    return 0x000000;
}

// CSS `@keyframes pulse` (black 0.8 -> red 0.8 -> black 0.8 over 1 s): `mix`
// is 0 at black and 1 at red.
uint32_t Hud::healthDangerColor(float mix) {
    const float t = std::clamp(mix, 0.0f, 1.0f);
    return static_cast<uint32_t>(std::lround(255.0f * t)) << 16;
}

uint32_t Hud::healthColor(float health, bool downed) {
    if (downed) {
        return 0xff0000;
    }
    const int n = static_cast<int>(sizeof(kHealthSteps) / sizeof(kHealthSteps[0]));
    const int h = static_cast<int>(std::ceil(health));
    // ui2.ts walks the steps with a strict `>` on the ceiled health; equal
    // bounds collapse to the upper step (math.delerp's clamp(NaN) = 1).
    int end = 0;
    while (end < n - 1 && kHealthSteps[end].health > h) {
        ++end;
    }
    const int first = std::max(end - 1, 0);
    const auto& a = kHealthSteps[first];
    const auto& b = kHealthSteps[end];
    const double value = static_cast<double>(health);
    const double t = b.health != a.health
        ? std::clamp((value - a.health) / (static_cast<double>(b.health) - a.health), 0.0, 1.0)
        : 1.0;
    const auto mix = [t](uint8_t x, uint8_t y) {
        // math.lerp(t, a, b) = a * (1 - t) + b * t, then Math.floor (double
        // precision, like the web).
        return static_cast<uint8_t>(
            std::floor(static_cast<double>(x) * (1.0 - t) + static_cast<double>(y) * t));
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

// Best quick-use heal from the bag: the small bandage for a top-up, the med
// kit when badly hurt (ui2.ts shows both strips and lets the player pick; the
// mockup HUD shows the four fixed bag counters instead).
std::string Hud::healItemFor(const std::vector<uint16_t>& inventory, float health) {
    const int bandage = bagCount(inventory, "bandage");
    const int healthkit = bagCount(inventory, "healthkit");
    if (healthkit > 0 && (health < 50.0f || bandage == 0)) {
        return "healthkit";
    }
    if (bandage > 0) {
        return "bandage";
    }
    return healthkit > 0 ? "healthkit" : std::string();
}

// Best quick-use boost: soda is the cheap top-up; pills are worth it below 50.
std::string Hud::boostItemFor(const std::vector<uint16_t>& inventory, float boost) {
    const int soda = bagCount(inventory, "soda");
    const int pills = bagCount(inventory, "painkiller");
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
    return "SCOPE " + uppercase(out);
}

// ui.ts scope strip label: "4xscope" -> "4x".
static std::string scopeStripLabel(const std::string& type) {
    std::string out = type;
    const std::string suffix = "scope";
    if (out.size() > suffix.size() &&
        out.compare(out.size() - suffix.size(), suffix.size(), suffix) == 0) {
        out.resize(out.size() - suffix.size());
    }
    return out;
}

const std::string& Hud::weaponType(int slot) const {
    static const std::string kEmpty;
    return (slot >= 0 && slot < 4) ? _weaponTypes[static_cast<size_t>(slot)] : kEmpty;
}

const std::string& Hud::topLootType(int idx) const {
    static const std::string kEmpty;
    return (idx >= 0 && idx < kTopLootCount) ? _topLoot[static_cast<size_t>(idx)].type : kEmpty;
}

const std::string& Hud::ammoRowType(int idx) const {
    static const std::string kEmpty;
    return (idx >= 0 && idx < kAmmoRowCount) ? _ammoRows[static_cast<size_t>(idx)].type : kEmpty;
}

const std::string& Hud::scopeSlotType(int idx) const {
    static const std::string kEmpty;
    return (idx >= 0 && idx < kScopeSlotCount) ? _scopeSlots[static_cast<size_t>(idx)].type : kEmpty;
}

Hud::TapResult Hud::hitTest(float designX, float designY) const {
    if (!_built || !_visible || _overShown || _deathShown || _spectating || _bigMapVisible ||
        _roleMenuActive || _menuShown) {
        return TapResult{};
    }
    // Rects are stored in the centred HUD container.
    const float px = designX - _design.x * 0.5f;
    const float py = designY - _design.y * 0.5f;
    if (_reloadRect.contains(px, py)) {
        return TapResult{TapKind::Reload, -1};
    }
    // ui.ts binds touchstart to #ui-interaction (the prompt itself), so
    // tapping the finger-press button or the item name picks the loot up.
    if (!_lastPrompt.empty() && _promptRect.w > 0.0f && _promptRect.contains(px, py)) {
        return TapResult{TapKind::Interact, -1};
    }
    if (_settingsRect.contains(px, py)) {
        return TapResult{TapKind::Settings, -1};
    }
    for (int i = 0; i < kTopLootCount; ++i) {
        const TopLootRow& row = _topLoot[static_cast<size_t>(i)];
        if (!row.type.empty() && row.countValue > 0 && row.rect.contains(px, py)) {
            return TapResult{TapKind::LootUse, i};
        }
    }
    for (int i = 0; i < kScopeSlotCount; ++i) {
        const ScopeSlot& slot = _scopeSlots[static_cast<size_t>(i)];
        if (!slot.type.empty() && slot.rect.contains(px, py)) {
            return TapResult{TapKind::ScopeSlot, i};
        }
    }
    if (_swapRect.contains(px, py)) {
        return TapResult{TapKind::SwapWeapons, -1};
    }
    // ui2.ts gear/perk tiles: tapping a droppable slot sends DropItemMsg
    // (the web touch listener drops on tap; backpack is not droppable).
    for (int i = 0; i < kGearIconCount; ++i) {
        if (gearDroppable(i) && _gearRects[static_cast<size_t>(i)].contains(px, py)) {
            return TapResult{TapKind::GearSlot, i};
        }
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
    _designPos = Vec2(_design.x, _design.y);
    if (!_factory || !_root) {
        return;
    }

    const float vw = _design.x;
    const float vh = _design.y;
    const auto X = [vw](float x) { return x - vw * 0.5f; };
    const auto Y = [vh](float y) { return y - vh * 0.5f; };
    // CSS top-based positions -> container Y (the design space is y up).
    // yTop(t) is the TOP edge (container Y) of an element whose CSS top is
    // `t` px from the top of the screen; a box's bottom edge is yTop(t + h).
    const auto yTop = [&](float top) { return Y(vh - top); };

    // #ui-bottom-center-0: 400x32 bar in a 412x44 box, 12px above the bottom.
    const float hudW = kHealthBarWidth + kHealthBarPadding * 2.0f;
    const float hudH = kHealthBarHeight + kHealthBarPadding * 2.0f;
    const float hudX = (vw - hudW) * 0.5f;
    const float hudY = 12.0f;

    // #ui-bottom-center-0: health counter (game.css: 400x32 bar, 6px padding,
    // rgba(0,0,0,0.4) background).
    _healthBg = _factory->createGraphics();
    _root->addChild(_healthBg);
    // #ui-health-depleted: grey #929292 trail behind the fill (game.css 1 s
    // ease-out width transition); drawn before the fill so it stays behind.
    _healthDepleted = _factory->createGraphics();
    _root->addChild(_healthDepleted);
    _healthFill = _factory->createGraphics();
    _root->addChild(_healthFill);
    _healthText = _factory->createText();
    _healthText->setFontSize(18.0f);
    _healthText->setColor(0xffffff, 0x000000, 2.0f, true);
    _healthText->setAnchor(0.5f, 0.5f);
    _root->addChild(_healthText);

    // ui.ts faction flair: 64px red/blue player-patch sprites on the left/right
    // of the health counter (only shown in faction mode).
    _flairLeft = _factory->createSprite();
    _flairLeft->setAnchor(0.5f, 0.5f);
    _flairLeft->setVisible(false);
    _root->addChild(_flairLeft);
    _flairRight = _factory->createSprite();
    _flairRight->setAnchor(0.5f, 0.5f);
    _flairRight->setVisible(false);
    _root->addChild(_flairRight);

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
    _root->addChild(_ammoText);

    _healthBg->clear();
    _healthBg->beginFill(0x000000, 0.4f);
    _healthBg->drawRect(X(hudX), yTop(vh - hudY), hudW, hudH);
    _healthBg->endFill();

    _healthFill->setPosition(X(hudX + kHealthBarPadding), Y(hudY + kHealthBarPadding));
    _healthDepleted->setPosition(X(hudX + kHealthBarPadding), Y(hudY + kHealthBarPadding));
    _healthText->setPosition(X(vw * 0.5f), Y(hudY + hudH * 0.5f));
    // game.css .ui-health-flair: 64x64, left:-32/top:13 (rotated 270deg) and
    // right:-32/top:13 (90deg); the counter is hudH tall.
    if (_flairLeft) {
        _flairLeft->setPosition(X(hudX), Y(hudY + hudH - 13.0f - 32.0f));
        _flairLeft->setRotation(-3.14159265358979f * 0.5f);
        const float source = std::max(_flairLeft->sourceWidth(), _flairLeft->sourceHeight());
        const float scale = source > 0.0f ? 64.0f / source : 1.0f;
        _flairLeft->setScale(scale, scale);
    }
    if (_flairRight) {
        _flairRight->setPosition(X(hudX + hudW), Y(hudY + hudH - 13.0f - 32.0f));
        _flairRight->setRotation(3.14159265358979f * 0.5f);
        const float source = std::max(_flairRight->sourceWidth(), _flairRight->sourceHeight());
        const float scale = source > 0.0f ? 64.0f / source : 1.0f;
        _flairRight->setScale(scale, scale);
    }
    {
        float x = hudX + kHealthBarPadding;
        for (int i = 0; i < 4; ++i) {
            const float span = kBoostBreakpoints[i] / kBoostBreakpointsTotal;
            _boostSegments[i]->setAnchor(0.0f, 0.0f);
            _boostSegments[i]->setPosition(X(x), Y(hudY + hudH + 4.0f));
            x += kHealthBarWidth * span;
        }
    }
    _ammoText->setAnchor(0.0f, 1.0f);
    _ammoText->setPosition(X(hudX + hudW + 12.0f), Y(hudY + hudH));

    // #ui-approxominator top-right (ui2.ts #ui-ammo-interactive): one row per
    // owned ammo type.
    for (int i = 0; i < kAmmoRowCount; ++i) {
        AmmoRow& row = _ammoRows[static_cast<size_t>(i)];
        const float top = 14.0f + static_cast<float>(i) * kAmmoRowPitch;
        row.rect = Rect{X(vw - kAmmoRowW - 12.0f), yTop(top + kAmmoRowH), kAmmoRowW,
                        kAmmoRowH};
        row.bg = _factory->createGraphics();
        row.chip = _factory->createGraphics();
        row.icon = _factory->createSprite("bullets.img");
        row.icon->setAnchor(0.5f, 0.5f);
        row.count = _factory->createText();
        row.count->setFontSize(14.0f);
        row.count->setColor(0xffffff, 0x000000, 1.5f, true);
        row.count->setAnchor(0.0f, 0.5f);
        row.type = kAmmoTypes[i].type;
        row.bg->setVisible(false);
        row.chip->setVisible(false);
        row.icon->setVisible(false);
        row.count->setVisible(false);
        _root->addChild(row.bg);
        _root->addChild(row.chip);
        _root->addChild(row.icon);
        _root->addChild(row.count);
        row.count->setPosition(X(vw - kAmmoRowW - 12.0f + 8.0f),
                               yTop(top + kAmmoRowH * 0.5f));
        row.icon->setPosition(X(vw - 12.0f - kAmmoRowW * 0.5f + 20.0f),
                              yTop(top + kAmmoRowH * 0.5f));
    }

    // --- Minimap + settings + info row (web mobile hud top-left) ---
    // #ui-map-wrapper-mobile: left 6px, top 46px; the map is 140px here.
    const float mm = kMinimapSize;
    const float mmX = kMinimapLeft;
    const float mmTop = kMinimapTop;
    _minimapPos = Vec2(X(mmX), yTop(mmTop + mm));
    _mapFrame = _factory->createGraphics();
    _mapFrame->lineStyle(2.0f, 0xffffff, 0.6f);
    _mapFrame->drawRect(_minimapPos.x, _minimapPos.y, mm, mm);
    _root->addChild(_mapFrame);
    _minimapTapRect = Rect{_minimapPos.x - 4.0f, _minimapPos.y - 4.0f, mm + 8.0f, mm + 8.0f};

    // Settings gear (#ui-menu-display) is created after the minimap contents
    // below so it draws inside the map's top-left corner (0,0) instead of
    // being covered by the map texture.

    // Info row under the minimap: gas timer + alive count + kill count.
    const float infoTop = kInfoBarTop;
    const float infoY = yTop(infoTop + kBoxHeight);
    const float gasW = kGasBoxWidth;
    const float countW = kCountBoxWidth;
    const float killW = kKillBoxWidth;
    _mapInfoBg = _factory->createGraphics();
    _mapInfoBg->beginFill(0x000000, 0.4f);
    _mapInfoBg->drawRect(X(mmX), infoY, gasW, kBoxHeight);
    _mapInfoBg->drawRect(X(mmX + gasW + 6.0f), infoY, countW, kBoxHeight);
    _mapInfoBg->drawRect(X(mmX + gasW + countW + 14.0f), infoY, killW, kBoxHeight);
    _mapInfoBg->endFill();
    _root->addChild(_mapInfoBg);
    const float infoCenterY = infoY + kBoxHeight * 0.5f;
    _gasIcon = _factory->createSprite("gui/gas.png");
    _gasIcon->setAnchor(0.5f, 0.5f);
    {
        const float source = std::max(_gasIcon->sourceWidth(), _gasIcon->sourceHeight());
        const float scale = source > 0.0f ? 18.0f / source : 1.0f;
        _gasIcon->setScale(scale, scale);
    }
    _gasIcon->setPosition(X(mmX + 18.0f), infoCenterY);
    _root->addChild(_gasIcon);
    _gasText = _factory->createText();
    _gasText->setFontSize(14.0f);
    _gasText->setColor(0xffffff, 0x000000, 2.0f, true);
    _gasText->setAnchor(0.5f, 0.5f);
    _gasText->setPosition(X(mmX + 18.0f + (gasW - 36.0f) * 0.5f + 18.0f), infoCenterY);
    _root->addChild(_gasText);
    _aliveIcon = _factory->createSprite("gui/player-gui.png");
    _aliveIcon->setAnchor(0.5f, 0.5f);
    {
        const float source = std::max(_aliveIcon->sourceWidth(), _aliveIcon->sourceHeight());
        const float scale = source > 0.0f ? 16.0f / source : 1.0f;
        _aliveIcon->setScale(scale, scale);
    }
    _aliveIcon->setPosition(X(mmX + gasW + 6.0f + 16.0f), infoCenterY);
    _root->addChild(_aliveIcon);
    _aliveText = _factory->createText();
    _aliveText->setFontSize(14.0f);
    _aliveText->setColor(0xffffff, 0x000000, 2.0f, true);
    _aliveText->setAnchor(0.5f, 0.5f);
    _aliveText->setPosition(X(mmX + gasW + 6.0f + 34.0f), infoCenterY);
    _root->addChild(_aliveText);
    _killsText = _factory->createText();
    _killsText->setFontSize(13.0f);
    _killsText->setColor(0xffd060, 0x000000, 2.0f, true);
    _killsText->setAnchor(0.5f, 0.5f);
    _killsText->setPosition(X(mmX + gasW + countW + 14.0f + killW * 0.5f), infoCenterY);
    _root->addChild(_killsText);

    // --- Scope strip (web #ui-top-center-scopes-wrapper, mobile layout) ---
    for (int i = 0; i < kScopeSlotCount; ++i) {
        ScopeSlot& slot = _scopeSlots[static_cast<size_t>(i)];
        const float top = kScopeStripTop + static_cast<float>(i) * kScopePitch;
        slot.rect = Rect{X(mmX), yTop(top + kScopeButtonH), kScopeButtonW, kScopeButtonH};
        slot.bg = _factory->createGraphics();
        slot.bg->setVisible(false);
        _root->addChild(slot.bg);
        slot.label = _factory->createText();
        slot.label->setFontSize(15.0f);
        slot.label->setColor(0xffffff, 0x000000, 1.5f, true);
        slot.label->setAnchor(0.5f, 0.5f);
        slot.label->setVisible(false);
        _root->addChild(slot.label);
        slot.label->setPosition(slot.rect.x + kScopeButtonW * 0.5f,
                                slot.rect.y + kScopeButtonH * 0.5f);
        // The per-update loop owns the type (owned scopes only).
        slot.type = std::string();
    }

    // --- In-game messages + announcements + the reload progress bar ---
    _announcementText = _factory->createText();
    _announcementText->setFontSize(18.0f);
    _announcementText->setColor(0xffd060, 0x000000, 2.0f, true);
    _announcementText->setAnchor(0.5f, 0.5f);
    _announcementText->setPosition(X(vw * 0.5f), yTop(kMessageTop));
    _announcementText->setVisible(false);
    _root->addChild(_announcementText);
    _waitingText = _factory->createText();
    _waitingText->setText("Waiting for players...");
    _waitingText->setFontSize(20.0f);
    _waitingText->setColor(0xffffff, 0x000000, 2.0f, true);
    _waitingText->setAnchor(0.5f, 0.5f);
    _waitingText->setPosition(X(vw * 0.5f), yTop(kMessageTop));
    _waitingText->setVisible(false);
    _root->addChild(_waitingText);

    // Reload/use/revive progress bar above the messages (pieTimer.ts port).
    _actionBarBg = _factory->createGraphics();
    _root->addChild(_actionBarBg);
    _actionBarFill = _factory->createGraphics();
    _root->addChild(_actionBarFill);
    _actionBarBg->setPosition(X((vw - kActionWidth) * 0.5f), yTop(kActionTop + kActionHeight));
    _actionBarFill->setPosition(X((vw - kActionWidth) * 0.5f), yTop(kActionTop + kActionHeight));
    _actionBarText = _factory->createText();
    _actionBarText->setFontSize(11.0f);
    _actionBarText->setColor(0xffffff, 0x000000, 1.0f, false);
    _actionBarText->setAnchor(0.5f, 0.5f);
    _actionBarText->setPosition(X(vw * 0.5f), yTop(kActionTop + kActionHeight * 0.5f));
    _actionBarText->setVisible(false);
    _root->addChild(_actionBarText);
    _pickupText = _factory->createText();
    _pickupText->setFontSize(14.0f);
    _pickupText->setColor(0xffc864, 0x000000, 2.0f, false);
    _pickupText->setAnchor(0.5f, 0.0f);
    _pickupText->setPosition(X(vw * 0.5f), yTop(162.0f));
    _root->addChild(_pickupText);

    // --- Top bag counters (bandage / medkit / soda / pills) ---
    for (int i = 0; i < kTopLootCount; ++i) {
        TopLootRow& row = _topLoot[static_cast<size_t>(i)];
        const float top = 12.0f;
        row.rect = Rect{X(614.0f + static_cast<float>(i) * kTopLootPitch),
                        yTop(top + kTopLootH), kTopLootW, kTopLootH};
        row.bg = _factory->createGraphics();
        row.icon = _factory->createSprite();
        row.icon->setAnchor(0.5f, 0.5f);
        row.count = _factory->createText();
        row.count->setFontSize(15.0f);
        row.count->setColor(0xffffff, 0x000000, 1.5f, true);
        row.count->setAnchor(0.5f, 0.5f);
        row.type = kTopLootTypes[i];
        row.bg->setVisible(false);
        row.icon->setVisible(false);
        row.count->setVisible(false);
        _root->addChild(row.bg);
        _root->addChild(row.icon);
        _root->addChild(row.count);
        row.icon->setPosition(row.rect.x + 11.0f, row.rect.y + kTopLootH * 0.5f);
        row.count->setPosition(row.rect.x + kTopLootW - 12.0f, row.rect.y + kTopLootH * 0.5f);
    }

    // --- Interact prompt: finger-press button + loot name next to the item ---
    _promptBox = _factory->createGraphics();
    _root->addChild(_promptBox);
    // Content/gui/tap.png loads without an atlas (the loadout atlas is only
    // registered on MapMsg), so the finger-press icon never gets missed.
    _promptIcon = _factory->createSprite("gui/tap.png");
    _promptIcon->setAnchor(0.5f, 0.5f);
    {
        const float source = std::max(_promptIcon->sourceWidth(), _promptIcon->sourceHeight());
        const float scale = source > 0.0f ? 36.0f / source : 1.0f;
        _promptIcon->setScale(scale, scale);
    }
    _promptIcon->setVisible(false);
    _root->addChild(_promptIcon);
    _promptText = _factory->createText();
    _promptText->setFontSize(16.0f);
    _promptText->setColor(0xffe08a, 0x000000, 2.0f, true);
    _promptText->setAnchor(0.0f, 0.5f);
    _promptText->setVisible(false);
    _root->addChild(_promptText);
    _promptPos = Vec2(vw * 0.5f, vh * 0.5f);

    // ui.ts off-screen teammate arrows: `.ui-team-indicator` uses the
    // 64x64 `/img/gui/indicator.svg` art plus a 32x32 group-colour dot at
    // `.ui-team-indicator-pos`. The small layout applies `scale(0.5)` to the
    // whole 64px box, hence the 32px art and 8px dot radius here.
    for (int i = 0; i < kTeamArrowCount; ++i) {
        pix::Container* arrow = _factory->createContainer();
        pix::Sprite* art = _factory->createSprite("gui/indicator.png");
        art->setAnchor(0.5f, 0.5f);
        const float source = std::max(art->sourceWidth(), art->sourceHeight());
        if (source > 0.0f) {
            const float scale = kTeamArrowArtSize / source;
            art->setScale(scale, scale);
        }
        arrow->addChild(art);

        // index.html gives the five main indicators fixed group colours
        // (yellow, magenta, cyan, orange, yellow again for slot 4).
        pix::Graphics* dot = _factory->createGraphics();
        dot->beginFill(kGroupColors[i % 4], 1.0f);
        dot->drawCircle(0.0f, kTeamArrowDotOffsetY, kTeamArrowDotRadius);
        dot->endFill();
        dot->setAlpha(kTeamArrowDotOpacity);
        arrow->addChild(dot);

        arrow->setVisible(false);
        _root->addChild(arrow);
        _teamArrows[static_cast<size_t>(i)] = arrow;
        _teamArrowDots[static_cast<size_t>(i)] = dot;
    }

    // --- Weapon cards (right column, tap-to-equip / hold-to-drop) ---
    const float cardW = kWeaponSlotWidth;
    const float cardH = kWeaponSlotHeight;
    const float cardX = vw - cardW - 8.0f;
    const float cardTop = 300.0f;
    for (int i = 0; i < 4; ++i) {
        const float top = cardTop + static_cast<float>(i) * kWeaponCardPitch;
        _weaponRect[i] = Rect{X(cardX), yTop(top + cardH), cardW, cardH};
        _weaponSlotBg[i] = _factory->createGraphics();
        _weaponSlotBg[i]->setVisible(false);
        _root->addChild(_weaponSlotBg[i]);
        _weaponIcons[i] = _factory->createSprite();
        _weaponIcons[i]->setAnchor(0.5f, 0.5f);
        _weaponIcons[i]->setVisible(false);
        _root->addChild(_weaponIcons[i]);
        _weaponSlots[i] = _factory->createText();
        _weaponSlots[i]->setFontSize(13.0f);
        _weaponSlots[i]->setColor(0xdcdcdc, 0x000000, 1.5f, true);
        _weaponSlots[i]->setAnchor(0.0f, 0.5f);
        _weaponSlots[i]->setVisible(false);
        _root->addChild(_weaponSlots[i]);
        _weaponIcons[i]->setPosition(_weaponRect[i].x + 22.0f, _weaponRect[i].y + cardH * 0.5f);
        _weaponSlots[i]->setPosition(_weaponRect[i].x + 44.0f, _weaponRect[i].y + cardH * 0.5f);
    }

    // Kill feed: rows drop down from above the weapon cards on the right.
    for (int i = 0; i < 6; ++i) {
        _killFeedRows[i] = _factory->createText();
        _killFeedRows[i]->setFontSize(13.0f);
        _killFeedRows[i]->setColor(0xffc896, 0x000000, 1.0f, false);
        _killFeedRows[i]->setAnchor(1.0f, 1.0f);
        _killFeedRows[i]->setPosition(X(cardX - 8.0f), yTop(cardTop - 10.0f + 16.0f * static_cast<float>(i)));
        _killFeedRows[i]->setVisible(false);
        _root->addChild(_killFeedRows[i]);
    }

    // P0.1 remaining pills: RELOAD right of the health bar, SWAP above it.
    _reloadRect = Rect{X(hudX + hudW + 54.0f - kReloadButtonWidth * 0.5f),
                       Y(90.0f - kReloadButtonHeight * 0.5f), kReloadButtonWidth,
                       kReloadButtonHeight};
    _swapRect = Rect{X(hudX + hudW + 54.0f - kPillWidth * 0.5f),
                     Y(150.0f - kPillHeight * 0.5f), kPillWidth, kPillHeight};
    _reloadBg = _factory->createGraphics();
    _root->addChild(_reloadBg);
    _reloadText = _factory->createText();
    _reloadText->setFontSize(15.0f);
    _reloadText->setColor(0xffffff, 0x000000, 1.5f, false);
    _reloadText->setAnchor(0.5f, 0.5f);
    _reloadText->setText("RELOAD");
    _reloadText->setPosition(_reloadRect.x + _reloadRect.w * 0.5f,
                             _reloadRect.y + _reloadRect.h * 0.5f);
    _reloadBg->setVisible(false);
    _reloadText->setVisible(false);
    _root->addChild(_reloadText);
    _swapBg = _factory->createGraphics();
    _root->addChild(_swapBg);
    _swapText = _factory->createText();
    _swapText->setFontSize(13.0f);
    _swapText->setColor(0xffffff, 0x000000, 1.5f, false);
    _swapText->setAnchor(0.5f, 0.5f);
    _swapText->setText("SWAP");
    _swapText->setPosition(_swapRect.x + _swapRect.w * 0.5f, _swapRect.y + _swapRect.h * 0.5f);
    _swapBg->setVisible(false);
    _swapText->setVisible(false);
    _root->addChild(_swapText);

    // Gear/perk slots: item atlas frames on the left of the health bar
    // (ui2.ts #ui-bottom-center-left). Each slot also carries a small
    // "Lvl. N" badge (gear only) and a tap-to-drop rect.
    for (int i = 0; i < kGearIconCount; ++i) {
        const float slotX = hudX - 34.0f - static_cast<float>(i) * 32.0f;
        pix::Sprite* icon = _factory->createSprite();
        icon->setAnchor(0.5f, 0.5f);
        icon->setPosition(X(slotX), Y(hudY + 16.0f));
        icon->setVisible(false);
        _root->addChild(icon);
        _gearIcons[static_cast<size_t>(i)] = icon;

        pix::Text* level = _factory->createText();
        level->setFontSize(9.0f);
        level->setColor(0xffffff, 0x000000, 1.0f, false);
        level->setAnchor(0.5f, 0.0f);
        level->setPosition(X(slotX), Y(hudY - 2.0f));
        level->setVisible(false);
        _root->addChild(level);
        _gearLevelText[static_cast<size_t>(i)] = level;

        // Root-space hit rect (icons are 26 px atlas art, 32 px pitch).
        _gearRects[static_cast<size_t>(i)] =
            Rect{X(slotX - 15.0f), Y(hudY - 4.0f), 30.0f, 30.0f};
    }

    // Squad bars: to the right of the minimap, top-aligned with it (team
    // modes). The block follows the web's `.ui-team-member` proportions
    // (200px bar, 4px padding, 8px gap below), with room for the 16px name.
    const float squadX = kMinimapLeft + kMinimapSize + 8.0f;
    const float barX = squadX + kSquadRowPadding;
    const float barTop = 28.0f; // name row + gap inside the 44px block
    for (int i = 0; i < 4; ++i) {
        SquadRow& row = _squad[static_cast<size_t>(i)];
        const float top = kMinimapTop + static_cast<float>(i) * kSquadRowPitch;
        row.bg = _factory->createGraphics();
        row.bg->beginFill(0x000000, 0.35f);
        row.bg->drawRect(X(squadX), yTop(top + kSquadRowHeight), kSquadRowWidth, kSquadRowHeight);
        row.bg->endFill();
        row.bg->setVisible(false);
        _root->addChild(row.bg);

        row.name = _factory->createText();
        row.name->setFontSize(16.0f);
        row.name->setColor(0xffffff, 0x000000, 1.5f, true);
        row.name->setAnchor(0.0f, 1.0f);
        // Text anchors are axmol anchors (0,1 = top-left), so position the
        // top edge 4px into the block; the 16px name then ends above the bar.
        row.name->setPosition(X(squadX + kSquadRowPadding + 16.0f), yTop(top + 4.0f));
        row.name->setVisible(false);
        _root->addChild(row.name);

        // Offline/dead/downed state icons match the web's
        // `ui-team-member-status-*` classes (close/skull/down art).
        row.statusIcon = _factory->createSprite();
        row.statusIcon->setAnchor(0.5f, 0.5f);
        row.statusIcon->setPosition(X(squadX + kSquadRowWidth + 6.0f + kSquadStatusSize * 0.5f),
                                    yTop(top + 20.0f));
        row.statusIcon->setVisible(false);
        _root->addChild(row.statusIcon);

        row.barBg = _factory->createGraphics();
        row.barBg->beginFill(0x000000, 0.5f);
        row.barBg->drawRect(X(barX), yTop(top + barTop + 10.0f), kSquadBarWidth, 10.0f);
        row.barBg->endFill();
        row.barBg->setVisible(false);
        _root->addChild(row.barBg);

        row.barFill = _factory->createGraphics();
        row.barFill->setPosition(X(barX), yTop(top + barTop + 10.0f));
        row.barFill->setVisible(false);
        _root->addChild(row.barFill);

        // ui-team-member-color: fixed group color for this squad slot
        // (GameConfig.groupColors: yellow, magenta, cyan, orange). It sits at
        // the block's left, clear of the bar's right end.
        row.colorDot = _factory->createGraphics();
        row.colorDot->beginFill(kGroupColors[i], 1.0f);
        row.colorDot->drawCircle(X(squadX + kSquadRowPadding + 7.0f), yTop(top + 12.0f), 7.0f);
        row.colorDot->endFill();
        row.colorDot->setVisible(false);
        _root->addChild(row.colorDot);
    }

    // --- Result screen (ui.ts displayStats): dim backdrop + centred panel ---
    _overBackdrop = _factory->createGraphics();
    _overBackdrop->beginFill(0x000000, 0.55f);
    _overBackdrop->drawRect(-_design.x * 0.5f, -_design.y * 0.5f, _design.x, _design.y);
    _overBackdrop->endFill();
    _overBackdrop->setLocalZOrder(950);
    _overBackdrop->setVisible(false);
    _root->addChild(_overBackdrop);
    const float panelW = 460.0f;
    const float panelH = 300.0f;
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

    // ui.ts showStats: per-player kills/damage cards (compact list here).
    for (int i = 0; i < kOverStatRows; ++i) {
        pix::Text* row = _factory->createText();
        row->setFontSize(13.0f);
        row->setColor(0xffffff, 0x000000, 1.0f, false);
        row->setAnchor(0.5f, 1.0f);
        row->setPosition(0.0f, panelH * 0.5f - 145.0f - static_cast<float>(i) * 18.0f);
        row->setLocalZOrder(952);
        row->setVisible(false);
        _root->addChild(row);
        _overStatRow[static_cast<size_t>(i)] = row;
    }

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

    // --- Minimap contents (map.ts renderMap + ui.ts map sprites) ---
    _miniMap.build(_factory, _root, _minimapPos, kMinimapSize);
    _miniMap.setZoomed(true);

    // Settings gear (#ui-menu-display) inside the minimap's top-left corner.
    // The icon uses the direct file (Content/gui/cog.png) instead of a
    // loadout-atlas frame: resetWorld() runs before the map atlases are loaded.
    _settingsRect = Rect{X(kMinimapLeft), yTop(kMinimapTop + kSettingsSize), kSettingsSize, kSettingsSize};
    _settingsBg = _factory->createGraphics();
    paintBox(_settingsBg, _settingsRect, 0x000000, 0.4f, 0.5f);
    _root->addChild(_settingsBg);
    _settingsIcon = _factory->createSprite("gui/cog.png");
    _settingsIcon->setAnchor(0.5f, 0.5f);
    _root->addChild(_settingsIcon);
    {
        const float source = std::max(_settingsIcon->sourceWidth(), _settingsIcon->sourceHeight());
        const float scale = source > 0.0f ? 32.0f / source : 1.0f;
        _settingsIcon->setScale(scale, scale);
        _settingsIcon->setPosition(_settingsRect.x + kSettingsSize * 0.5f,
                                   _settingsRect.y + kSettingsSize * 0.5f);
    }

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

    // --- In-game menu (gear button) ---
    _menuBackdrop = _factory->createGraphics();
    _menuBackdrop->beginFill(0x000000, 0.62f);
    _menuBackdrop->drawRect(-vw * 0.5f, -vh * 0.5f, vw, vh);
    _menuBackdrop->setLocalZOrder(960);
    _menuBackdrop->setVisible(false);
    _root->addChild(_menuBackdrop);
    const float menuW = 340.0f;
    const float menuH = 220.0f;
    _menuPanel = _factory->createGraphics();
    _menuPanel->beginFill(0x1a1a1a, 0.92f);
    _menuPanel->drawRect(-menuW * 0.5f, -menuH * 0.5f, menuW, menuH);
    _menuPanel->lineStyle(2.0f, 0xffffff, 0.5f);
    _menuPanel->drawRect(-menuW * 0.5f, -menuH * 0.5f, menuW, menuH);
    _menuPanel->setLocalZOrder(961);
    _menuPanel->setVisible(false);
    _root->addChild(_menuPanel);
    _menuTitle = _factory->createText();
    _menuTitle->setFontSize(22.0f);
    _menuTitle->setColor(0xffffff, 0x000000, 2.0f, true);
    _menuTitle->setAnchor(0.5f, 0.5f);
    _menuTitle->setText("GAME MENU");
    _menuTitle->setPosition(0.0f, 62.0f);
    _menuTitle->setLocalZOrder(962);
    _menuTitle->setVisible(false);
    _root->addChild(_menuTitle);
    const auto makeMenuButton = [&](pix::Graphics*& bg, pix::Text*& text, Rect& rect,
                                    float y) {
        rect = Rect{X(-140.0f), Y(y - 21.0f), 280.0f, 42.0f};
        bg = _factory->createGraphics();
        bg->setLocalZOrder(963);
        bg->setVisible(false);
        _root->addChild(bg);
        text = _factory->createText();
        text->setFontSize(17.0f);
        text->setColor(0xffffff, 0x000000, 1.5f, false);
        text->setAnchor(0.5f, 0.5f);
        text->setLocalZOrder(964);
        text->setPosition(X(0.0f), Y(y));
        text->setVisible(false);
        _root->addChild(text);
    };
    makeMenuButton(_menuResumeBg, _menuResumeText, _menuResumeRect, 24.0f);
    makeMenuButton(_menuLeaveBg, _menuLeaveText, _menuLeaveRect, -30.0f);
    _menuResumeText->setText("RESUME");
    _menuLeaveText->setText("LEAVE GAME");
    paintOverlayButton(_menuResumeBg, _menuResumeRect, 0x50afab);
    paintOverlayButton(_menuLeaveBg, _menuLeaveRect, 0xcc3333);

    // --- Perk-mode class picker (#ui-role-menu-wrapper, ui.ts) ---
    // The web panel is 1200x770 px and scales by min(1, w/1200, h/900); the
    // HUD design rect is 1280x720, so the native panel is 0.8x (960x616).
    _roleScale = std::min(1.0f, std::min(vw / kRoleMenuWidth, vh / 900.0f));
    _roleBackdrop = _factory->createGraphics();
    _roleBackdrop->beginFill(0x000000, 0.75f);
    _roleBackdrop->drawRect(-vw * 0.5f, -vh * 0.5f, vw, vh);
    _roleBackdrop->endFill();
    _roleBackdrop->setLocalZOrder(939);
    _roleBackdrop->setVisible(false);
    _root->addChild(_roleBackdrop);

    _rolePanel = _factory->createContainer();
    _rolePanel->setPosition(0.0f, 0.0f);
    _rolePanel->setScale(_roleScale, _roleScale);
    _rolePanel->setLocalZOrder(940);
    _rolePanel->setVisible(false);
    _root->addChild(_rolePanel);

    const float rw = kRoleMenuWidth;
    const float rh = kRoleMenuHeight;
    _rolePanelBg = _factory->createGraphics();
    _rolePanelBg->beginFill(0x7692b0, 0.75f);
    _rolePanelBg->drawRect(-rw * 0.5f, -rh * 0.5f, rw, rh);
    _rolePanelBg->endFill();
    _rolePanel->addChild(_rolePanelBg);

    _roleHeaderBg = _factory->createGraphics();
    _roleHeaderBg->beginFill(0x000000, 0.6f);
    _roleHeaderBg->drawRect(-rw * 0.5f, rh * 0.5f - 200.0f, rw, 200.0f);
    _roleHeaderBg->endFill();
    _rolePanel->addChild(_roleHeaderBg);

    _roleFooterBg = _factory->createGraphics();
    _roleFooterBg->beginFill(0x000000, 0.6f);
    _roleFooterBg->drawRect(-rw * 0.5f, -rh * 0.5f, rw, 120.0f);
    _roleFooterBg->endFill();
    _rolePanel->addChild(_roleFooterBg);

    for (int i = 0; i < kRoleOptionMax; ++i) {
        pix::Sprite* option = _factory->createSprite();
        option->setAnchor(0.5f, 0.5f);
        option->setVisible(false);
        _rolePanel->addChild(option);
        _roleOptionSprites[static_cast<size_t>(i)] = option;
    }

    // Left column: class name + guiImg art (240 px, web .ui-role-body-image).
    _roleName = _factory->createText();
    _roleName->setFontSize(80.0f);
    _roleName->setColor(0xffffff, 0x000000, 3.0f, true);
    _roleName->setAnchor(0.5f, 0.5f);
    _roleName->setPosition(-rw * 0.3f, 120.0f);
    _rolePanel->addChild(_roleName);
    _roleImage = _factory->createSprite();
    _roleImage->setAnchor(0.5f, 0.5f);
    _roleImage->setPosition(-rw * 0.3f, -90.0f);
    _roleImage->setVisible(false);
    _rolePanel->addChild(_roleImage);

    // Right column: perk ring + loot icon + name (web .ui-role-body-perk).
    for (int i = 0; i < kRolePerkMax; ++i) {
        pix::Sprite* ring = _factory->createSprite("loot-circle-outer-03.img");
        ring->setAnchor(0.5f, 0.5f);
        ring->setVisible(false);
        _rolePanel->addChild(ring);
        _rolePerkRing[static_cast<size_t>(i)] = ring;
        pix::Sprite* icon = _factory->createSprite();
        icon->setAnchor(0.5f, 0.5f);
        icon->setVisible(false);
        _rolePanel->addChild(icon);
        _rolePerkIcon[static_cast<size_t>(i)] = icon;
        pix::Text* name = _factory->createText();
        name->setFontSize(56.0f);
        name->setColor(0xffffff, 0x000000, 3.0f, true);
        name->setAnchor(0.0f, 0.5f);
        name->setVisible(false);
        _rolePanel->addChild(name);
        _rolePerkName[static_cast<size_t>(i)] = name;
    }

    _roleDesc = _factory->createText();
    _roleDesc->setText("SELECT A CLASS");
    _roleDesc->setFontSize(64.0f);
    _roleDesc->setColor(0xffffff, 0x000000, 3.0f, true);
    _roleDesc->setAnchor(0.5f, 0.5f);
    _roleDesc->setPosition(-rw * 0.2f, -rh * 0.5f + 60.0f);
    _rolePanel->addChild(_roleDesc);

    _roleConfirmBg = _factory->createGraphics();
    _roleConfirmBg->beginFill(0x000000, 0.75f);
    _roleConfirmBg->drawRect(rw * 0.12f, -rh * 0.5f + 24.0f, rw * 0.36f, 72.0f);
    _roleConfirmBg->endFill();
    _rolePanel->addChild(_roleConfirmBg);
    // #ui-role-footer-enter: "ENTER GAME (20)"; the same element is the
    // confirm button and the countdown label (ui.ts roleMenuFooterEnterElem).
    _roleFooter = _factory->createText();
    _roleFooter->setFontSize(40.0f);
    _roleFooter->setColor(0x00ff00, 0x000000, 3.0f, true);
    _roleFooter->setAnchor(0.5f, 0.5f);
    _roleFooter->setPosition(rw * 0.3f, -rh * 0.5f + 60.0f);
    _rolePanel->addChild(_roleFooter);

    // Hit rects are design-space (the panel container is scaled around the
    // design centre). The confirm button is static; option rects follow the
    // map's class count in applyRoleSelection().
    {
        const float cx = _design.x * 0.5f;
        const float cy = _design.y * 0.5f;
        _roleConfirmRect = Rect{
            cx + rw * 0.12f * _roleScale,
            cy + (-rh * 0.5f + 24.0f) * _roleScale,
            rw * 0.36f * _roleScale,
            72.0f * _roleScale,
        };
    }

    _built = true;
    setVisible(false);
}

void Hud::paintBox(pix::Graphics* g, const Rect& rect, uint32_t fill, float alpha,
                   float borderAlpha) const {
    if (!g) {
        return;
    }
    g->clear();
    g->beginFill(fill, alpha);
    g->drawRect(0.0f, 0.0f, rect.w, rect.h);
    g->endFill();
    if (borderAlpha > 0.0f) {
        g->lineStyle(2.0f, 0xffffff, borderAlpha);
        g->drawRect(0.0f, 0.0f, rect.w, rect.h);
    }
    g->setPosition(rect.x, rect.y);
}

void Hud::setVisible(bool visible) {
    if (_visible != visible) {
        _actionKey.clear();
        _lastActionProgress = -1.0f;
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
    show(_healthDepleted);
    show(_healthFill);
    show(_healthText);
    show(_ammoText);
    show(_mapFrame);
    show(_mapInfoBg);
    show(_aliveIcon);
    if (_gasText) {
        _gasText->setVisible(visible && _gas.active);
    }
    if (_gasIcon) {
        _gasIcon->setVisible(visible && _gas.active);
    }
    show(_aliveText);
    show(_killsText);
    if (_announcementText) {
        _announcementText->setVisible(visible && _announcementTicker > 0.0f);
    }
    for (size_t i = 0; i < _killFeedRows.size(); ++i) {
        if (_killFeedRows[i]) {
            _killFeedRows[i]->setVisible(visible && _killFeedHas[i]);
        }
    }
    for (size_t i = 0; i < _teamArrows.size(); ++i) {
        if (_teamArrows[i]) {
            // Only slots GameScene actually populated may show; otherwise a
            // big-map toggle would revive stale arrow nodes.
            const bool populated = std::any_of(_teamArrowState.begin(), _teamArrowState.end(),
                [i](const TeammateArrow& arrow) { return arrow.slot == static_cast<int>(i); });
            _teamArrows[i]->setVisible(visible && !_bigMapVisible && populated);
        }
    }
    for (auto* seg : _boostSegments) {
        show(seg);
    }
    for (auto* slot : _weaponSlots) {
        show(slot);
    }
    for (auto* icon : _weaponIcons) {
        if (icon) {
            icon->setVisible(visible && icon->sourceWidth() > 0.0f);
        }
    }
    for (auto* bg : _weaponSlotBg) {
        show(bg);
    }
    // Remaining pills + counters own their visibility in updateActionButtons()
    // (empty bag = hidden), so nothing to do here.
    _miniMap.setVisible(visible && !_bigMapVisible && !_minimapHidden);
    _bigMap.setVisible(visible && _bigMapVisible);
    if (_bigMapBackdrop) {
        _bigMapBackdrop->setVisible(visible && _bigMapVisible);
    }
    if (_bigMapCloseBg) {
        _bigMapCloseBg->setVisible(visible && _bigMapVisible);
    }
    if (_bigMapCloseText) {
        _bigMapCloseText->setVisible(visible && _bigMapVisible);
    }
    // Settings gear + top counters: hidden with the result/death overlays.
    const bool over = _overShown || _deathShown;
    if (_settingsBg) {
        _settingsBg->setVisible(visible && !over);
    }
    if (_settingsIcon) {
        _settingsIcon->setVisible(visible && !over);
    }
    for (auto* node : std::initializer_list<pix::Node*>{
             _menuBackdrop, _menuPanel, _menuTitle, _menuResumeBg, _menuResumeText, _menuLeaveBg,
             _menuLeaveText}) {
        if (node) {
            node->setVisible(false);
        }
    }
    if (_menuShown) {
        if (_menuBackdrop) _menuBackdrop->setVisible(true);
        if (_menuPanel) _menuPanel->setVisible(true);
        if (_menuTitle) _menuTitle->setVisible(true);
        if (_menuResumeBg) _menuResumeBg->setVisible(true);
        if (_menuResumeText) _menuResumeText->setVisible(true);
        if (_menuLeaveBg) _menuLeaveBg->setVisible(true);
        if (_menuLeaveText) _menuLeaveText->setVisible(true);
    }
    for (size_t i = 0; i < _squad.size(); ++i) {
        const bool showRow = visible && i < _squadState.size();
        SquadRow& row = _squad[i];
        if (row.bg) {
            row.bg->setVisible(showRow);
        }
        if (row.name) {
            row.name->setVisible(showRow);
        }
        if (row.barBg) {
            row.barBg->setVisible(showRow);
        }
        if (row.barFill) {
            row.barFill->setVisible(showRow && _squadState[i].health > 0.0f);
        }
        if (row.statusIcon) {
            row.statusIcon->setVisible(showRow && row.hasStatus);
        }
        if (row.colorDot) {
            row.colorDot->setVisible(showRow);
        }
    }
    for (size_t i = 0; i < _gearIcons.size(); ++i) {
        if (_gearIcons[i]) {
            _gearIcons[i]->setVisible(visible && !_gearFrames[i].empty());
        }
        if (_gearLevelText[i]) {
            _gearLevelText[i]->setVisible(
                visible && !_gearFrames[i].empty() && _gearSlots[i].level > 0);
        }
    }
    if (_overBackdrop) {
        _overBackdrop->setVisible(visible && over);
    }
    if (_overPanel) {
        _overPanel->setVisible(visible && over);
    }
    if (_overTitle) {
        _overTitle->setVisible(visible && over);
    }
    if (_overRank) {
        _overRank->setVisible(visible && _overShown);
    }
    if (_overStats) {
        _overStats->setVisible(visible && over);
    }
    for (size_t i = 0; i < _overStatRow.size(); ++i) {
        if (_overStatRow[i]) {
            _overStatRow[i]->setVisible(visible && _overShown && _overStatRowHas[i]);
        }
    }
    if (_overLeaveText) {
        _overLeaveText->setVisible(visible && over);
    }
    if (_overLeaveBg) {
        _overLeaveBg->setVisible(visible && over);
    }
    if (_overSpectateBg) {
        _overSpectateBg->setVisible(_deathShown && !_overShown);
    }
    if (_overSpectateText) {
        _overSpectateText->setVisible(_deathShown && !_overShown);
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
    // ui.ts #ui-waiting-text is independent of the normal HUD visibility: it
    // shows after joining, before the match starts.
    if (_waitingText) {
        _waitingText->setVisible(_waiting && !_overShown && !_deathShown);
    }
    // Perk-mode class picker (#ui-role-menu-wrapper): the backdrop + panel are
    // shown only while the picker is active; option art follows the class list.
    const bool role = visible && _roleMenuActive;
    if (_roleBackdrop) {
        _roleBackdrop->setVisible(role);
    }
    if (_rolePanel) {
        _rolePanel->setVisible(role);
    }
    for (int i = 0; i < kRoleOptionMax; ++i) {
        if (_roleOptionSprites[static_cast<size_t>(i)]) {
            _roleOptionSprites[static_cast<size_t>(i)]->setVisible(
                role && i < static_cast<int>(_roleOptionsState.size()));
        }
    }
    if (_promptBox) {
        _promptBox->setVisible(_visible && !_lastPrompt.empty());
    }
    if (_promptIcon) {
        _promptIcon->setVisible(_visible && !_lastPrompt.empty() &&
                                _promptIcon->sourceWidth() > 0.0f);
    }
    if (_promptText) {
        _promptText->setVisible(_visible && !_lastPrompt.empty());
    }
    // Action bar visibility is owned by setActionProgress().
    if (!_lastAction.empty() && _lastActionProgress >= 0.0f) {
        if (_actionBarBg) _actionBarBg->setVisible(_visible);
        if (_actionBarFill) _actionBarFill->setVisible(_visible);
        if (_actionBarText) _actionBarText->setVisible(_visible);
    } else {
        if (_actionBarBg) _actionBarBg->setVisible(false);
        if (_actionBarFill) _actionBarFill->setVisible(false);
        if (_actionBarText) _actionBarText->setVisible(false);
    }
    // Chip/counter rows plus action pills are repainted by
    // updateActionButtons().
    updateActionButtons();
    updatePrompt();
}

void Hud::setMinimap(pix::RenderTexture* texture, float mapWidth, float mapHeight) {
    if (!_built || !texture) {
        return;
    }
    _miniMap.setMapSize(mapWidth, mapHeight);
    _miniMap.setTexture(texture);
    _bigMap.setMapSize(mapWidth, mapHeight);
    _bigMap.setTexture(texture);
    _miniMap.setVisible(_visible && !_bigMapVisible && !_minimapHidden);
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

void Hud::updateMapIndicators(const std::vector<MapIndicatorMarker>& indicators) {
    if (!_built) {
        return;
    }
    _miniMap.setIndicators(indicators);
    _bigMap.setIndicators(indicators);
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
    // Re-run the visibility pass so the minimap and the big map swap.
    setVisible(_visible);
}

void Hud::setMinimapHidden(bool hidden) {
    _minimapHidden = hidden;
    if (!_built) {
        return;
    }
    setVisible(_visible);
}

bool Hud::minimapTapHit(float designX, float designY) const {
    if (!_visible || _bigMapVisible || _minimapHidden || !_miniMap.visible()) {
        return false;
    }
    // Callers pass design-space coordinates (0..design); the widget positions
    // are in the centred HUD container.
    const Vec2 p(designX - _design.x * 0.5f, designY - _design.y * 0.5f);
    // The settings gear is drawn inside the minimap's top-left corner; let it
    // win the touch instead of opening the big map underneath it.
    if (_settingsRect.contains(p.x, p.y)) {
        return false;
    }
    return _minimapTapRect.contains(p.x, p.y);
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

void Hud::showGameMenu(bool visible) {
    _menuShown = visible;
    if (!_built) {
        return;
    }
    setVisible(_visible);
}

Hud::MenuAction Hud::menuActionAt(float designX, float designY) const {
    if (!_menuShown) {
        return MenuAction::None;
    }
    const float px = designX - _design.x * 0.5f;
    const float py = designY - _design.y * 0.5f;
    if (_menuResumeRect.contains(px, py)) {
        return MenuAction::Resume;
    }
    if (_menuLeaveRect.contains(px, py)) {
        return MenuAction::Leave;
    }
    return MenuAction::None;
}

void Hud::setSquadState(const std::vector<SquadMember>& members) {
    _squadState = members;
    if (!_built) {
        return;
    }
    for (size_t i = 0; i < _squad.size(); ++i) {
        SquadRow& row = _squad[i];
        if (i >= members.size()) {
            if (row.bg) {
                row.bg->setVisible(false);
            }
            row.name->setVisible(false);
            row.barBg->setVisible(false);
            row.barFill->setVisible(false);
            row.hasStatus = false;
            if (row.statusIcon) {
                row.statusIcon->setVisible(false);
            }
            if (row.colorDot) {
                row.colorDot->setVisible(false);
            }
            row.key.clear();
            continue;
        }
        const SquadMember& m = members[i];
        std::string status;
        std::string statusFrame;
        float statusAlpha = 1.0f;
        if (m.disconnected) {
            // ui-team-member-status-disconnected: close.svg at 50% opacity.
            status = "DC";
            statusFrame = "gui/close.png";
            statusAlpha = 0.5f;
        } else if (m.dead) {
            // ui-team-member-status-dead: skull-team.svg at 50% opacity.
            status = "Dead";
            statusFrame = "gui/skull-team.png";
            statusAlpha = 0.5f;
        } else if (m.downed) {
            // ui-team-member-status-downed: down.svg with icon-pulse.
            status = "Down";
            statusFrame = "gui/down.png";
        }
        const int health = static_cast<int>(std::clamp(m.health, 0.0f, 100.0f));
        const std::string key = m.name + "|" + std::to_string(health) + "|" + status +
                                "|" + (m.active ? "1" : "0");
        if (key == row.key) {
            continue;
        }
        row.key = key;
        row.name->setText(m.name.empty() ? "Player" : m.name);
        // ui.ts: disconnected/dead names fade to 30%; downed status pulses
        // (the pulse itself is advanced in update()).
        row.name->setAlpha(m.disconnected || m.dead ? 0.3f : 1.0f);
        row.hasStatus = !statusFrame.empty();
        if (row.statusIcon) {
            if (row.hasStatus) {
                row.statusIcon->setFrame(statusFrame);
                const float source = std::max(row.statusIcon->sourceWidth(),
                                              row.statusIcon->sourceHeight());
                const float scale = source > 0.0f ? kSquadStatusSize / source : 1.0f;
                row.statusIcon->setScale(scale, scale);
                row.statusIcon->setAlpha(statusAlpha);
            }
            row.statusIcon->setVisible(_visible && row.hasStatus);
        }
        if (row.colorDot) {
            row.colorDot->setAlpha(1.0f);
            row.colorDot->setVisible(_visible);
        }
        // ui.ts updateHealthBar: grey/white/pink palette; the <=25 pulse is
        // advanced per frame in update().
        const uint32_t color = squadHealthColor(m.health, m.dead, m.downed);
        const float fillWidth = m.dead
            ? 0.0f
            : std::max(1.0f, kSquadBarWidth * std::clamp(m.health, 0.0f, 100.0f) / 100.0f);
        row.barFill->clear();
        if (fillWidth > 0.0f) {
            row.barFill->beginFill(color, m.health <= 25.0f && !m.dead ? 0.8f : 1.0f);
            row.barFill->drawRect(0.0f, 0.0f, fillWidth, 10.0f);
            row.barFill->endFill();
        }
        const bool showRow = _visible;
        if (row.bg) {
            row.bg->setVisible(showRow);
        }
        row.name->setVisible(showRow);
        row.barBg->setVisible(showRow);
        row.barFill->setVisible(showRow && fillWidth > 0.0f);
        if (row.statusIcon) {
            row.statusIcon->setVisible(showRow && row.hasStatus);
        }
        if (row.colorDot) {
            row.colorDot->setVisible(showRow);
        }
    }
}

void Hud::setGearState(const std::vector<std::string>& frames) {
    std::vector<GearSlot> slots;
    slots.reserve(frames.size());
    for (const auto& frame : frames) {
        GearSlot slot;
        slot.frame = frame;
        slots.push_back(std::move(slot));
    }
    setGearSlots(slots);
}

void Hud::setGearSlots(const std::vector<GearSlot>& slots) {
    if (!_built) {
        return;
    }
    for (size_t i = 0; i < _gearIcons.size(); ++i) {
        GearSlot slot = i < slots.size() ? slots[i] : GearSlot{};
        const bool changed = slot.frame != _gearFrames[i];
        _gearFrames[i] = slot.frame;
        _gearSlots[i] = slot;
        pix::Sprite* icon = _gearIcons[i];
        pix::Text* level = _gearLevelText[i];
        if (level) {
            if (slot.level > 0 && !slot.frame.empty()) {
                level->setText("Lvl. " + std::to_string(slot.level));
                const uint32_t color =
                    slot.level >= 4 ? 0xb30000u : (slot.level == 3 ? 0xff9900u : 0xffffffu);
                level->setColor(color, 0x000000, 1.0f, false);
                level->setVisible(_visible);
            } else {
                level->setVisible(false);
            }
        }
        if (!icon) {
            continue;
        }
        if (slot.frame.empty()) {
            icon->setVisible(false);
            continue;
        }
        if (changed) {
            icon->setFrame(slot.frame);
            const float source = std::max(icon->sourceWidth(), icon->sourceHeight());
            const float size = source > 0.0f ? 26.0f / source : 1.0f;
            icon->setScale(size, size);
        }
        icon->setVisible(_visible);
    }
}

const std::string& Hud::gearType(int idx) const {
    static const std::string empty;
    return (idx >= 0 && idx < kGearIconCount) ? _gearSlots[static_cast<size_t>(idx)].type : empty;
}

bool Hud::gearDroppable(int idx) const {
    return idx >= 0 && idx < kGearIconCount && _gearSlots[static_cast<size_t>(idx)].droppable &&
        !_gearSlots[static_cast<size_t>(idx)].type.empty() &&
        !_gearFrames[static_cast<size_t>(idx)].empty();
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
    if (_gasIcon) {
        // gas.svg while waiting, danger.svg while the red zone moves.
        _gasIcon->setFrame(gas.moving ? "gui/danger.png" : "gui/gas.png");
        _gasIcon->setVisible(_visible && gas.active);
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

void Hud::showGameOver(const GameOverMsg& over, uint8_t localTeamId, uint16_t localPlayerId,
                       const Game* game) {
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

    // ui.ts showStats: one row per teammate (name, kills, damage), local row green.
    std::vector<const PlayerStatsMsg*> rows;
    rows.reserve(over.playerStats.size());
    for (const auto& stats : over.playerStats) {
        rows.push_back(&stats);
    }
    std::sort(rows.begin(), rows.end(), [](const PlayerStatsMsg* a, const PlayerStatsMsg* b) {
        if (a->kills != b->kills) return a->kills > b->kills;
        return a->damageDealt > b->damageDealt;
    });
    for (int i = 0; i < kOverStatRows; ++i) {
        pix::Text* row = _overStatRow[static_cast<size_t>(i)];
        if (!row) continue;
        if (i >= static_cast<int>(rows.size())) {
            _overStatRowHas[static_cast<size_t>(i)] = false;
            row->setVisible(false);
            continue;
        }
        const PlayerStatsMsg* stats = rows[static_cast<size_t>(i)];
        const std::string name = game ? game->displayName(stats->playerId)
                                      : "Player " + std::to_string(stats->playerId);
        row->setText(name + "   " + std::to_string(stats->kills) + " K   " +
                     std::to_string(stats->damageDealt) + " DMG");
        uint32_t color = 0xffffff;
        if (stats->playerId == localPlayerId) {
            color = 0x8fff6a;
        } else if (game) {
            const uint8_t team = game->playerTeamId(stats->playerId);
            if (team >= 1 && team <= 2) color = kTeamColors[team - 1];
        }
        row->setColor(color, 0x000000, 1.0f, false);
        _overStatRowHas[static_cast<size_t>(i)] = true;
        row->setVisible(true);
    }

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
    for (auto* row : _overStatRow) {
        if (row) row->setVisible(false);
    }
    _overStatRowHas = {};
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

void Hud::setWaitingForPlayers(bool waiting) {
    _waiting = waiting;
    if (_waitingText) {
        _waitingText->setVisible(waiting && !_overShown && !_deathShown);
    }
}

void Hud::setActionProgress(const std::string& label, float elapsed, float duration) {
    const std::string next = duration > 0.0f && elapsed < duration ? label : std::string();
    const float progress = duration > 0.0f && elapsed < duration
        ? std::clamp(elapsed / duration, 0.0f, 1.0f)
        : 0.0f;
    if (!_built) {
        _lastAction = next;
        _lastActionProgress = progress;
        return;
    }
    if (next.empty()) {
        if (!_lastAction.empty() || _lastActionProgress != 0.0f) {
            _lastAction.clear();
            _lastActionProgress = 0.0f;
            if (_actionBarBg) _actionBarBg->setVisible(false);
            if (_actionBarFill) _actionBarFill->setVisible(false);
            if (_actionBarText) _actionBarText->setVisible(false);
        }
        return;
    }
    // The fill repaints every frame while the action runs; Graphics relies on
    // DrawNode buffers (not text rasterization) so this is cheap.
    const bool bgChanged = _lastAction != next;
    _lastAction = next;
    _lastActionProgress = progress;
    if (bgChanged) {
        if (_actionBarBg) {
            paintBox(_actionBarBg, Rect{0.0f, 0.0f, kActionWidth, kActionHeight}, 0x000000, 0.45f,
                     0.5f);
            _actionBarBg->setVisible(_visible);
        }
        if (_actionBarText) {
            _actionBarText->setText(next);
            _actionBarText->setVisible(_visible);
        }
    }
    if (_actionBarFill) {
        _actionBarFill->clear();
        if (_visible && progress > 0.01f) {
            _actionBarFill->beginFill(0xffffff, 0.85f);
            _actionBarFill->drawRect(2.0f, 2.0f, (kActionWidth - 4.0f) * progress,
                                     kActionHeight - 4.0f);
            _actionBarFill->endFill();
        }
        _actionBarFill->setVisible(_visible);
    }
}

void Hud::setInteractionPrompt(const std::string& text, const Vec2& designPos) {
    if (!_built) {
        return;
    }
    const auto samePos = [](const Vec2& a, const Vec2& b) {
        return std::fabs(a.x - b.x) < 0.5f && std::fabs(a.y - b.y) < 0.5f;
    };
    if (text != _lastPrompt || !samePos(designPos, _promptPos)) {
        _lastPrompt = text;
        _promptPos = designPos;
        updatePrompt();
    }
}

void Hud::updatePrompt() {
    if (!_promptBox || !_promptIcon || !_promptText || !_built) {
        return;
    }
    if (_lastPrompt.empty()) {
        _promptBox->setVisible(false);
        _promptIcon->setVisible(false);
        _promptText->setVisible(false);
        _promptRect = Rect{};
        return;
    }
    // Place the finger-press box right beside the loot, clamped into the
    // design space. The item name sits on the right of the box, or flipped to
    // the left when the loot is close to the right screen edge. The fed
    // position is design-space (y up); nodes/rects are in the centred container.
    const float vw = _design.x;
    const float vh = _design.y;
    const float cx = std::clamp(_promptPos.x - vw * 0.5f,
                                kPromptSize * 0.5f, vw * 0.5f - kPromptSize * 0.5f);
    const float cy =
        std::clamp(_promptPos.y - vh * 0.5f, kPromptSize * 0.5f,
                   vh * 0.5f - kPromptSize * 0.5f);
    const Rect box{cx - kPromptSize * 0.5f, cy - kPromptSize * 0.5f, kPromptSize, kPromptSize};
    const float textWidth = static_cast<float>(_lastPrompt.size()) * 9.0f;
    const bool flipLeft = cx + kPromptSize * 0.5f + 12.0f + textWidth > vw * 0.5f - 12.0f;
    if (flipLeft) {
        _promptRect = Rect{cx - kPromptSize * 0.5f - 16.0f - textWidth, cy - kPromptSize * 0.5f - 4.0f,
                           kPromptSize + 20.0f + textWidth, kPromptSize + 8.0f};
    } else {
        _promptRect = Rect{cx - kPromptSize * 0.5f - 4.0f, cy - kPromptSize * 0.5f - 4.0f,
                           kPromptSize + 12.0f + textWidth, kPromptSize + 8.0f};
    }
    paintBox(_promptBox, box, 0x000000, 0.5f, 0.6f);
    _promptBox->setVisible(_visible);
    const float source = std::max(_promptIcon->sourceWidth(), _promptIcon->sourceHeight());
    if (source > 0.0f) {
        const float scale = 36.0f / source;
        _promptIcon->setScale(scale, scale);
        _promptIcon->setVisible(_visible);
    } else {
        _promptIcon->setVisible(false);
    }
    _promptIcon->setPosition(cx, cy);
    _promptText->setText(_lastPrompt);
    if (flipLeft) {
        _promptText->setAnchor(1.0f, 0.5f);
        _promptText->setPosition(box.x - 12.0f, cy);
    } else {
        _promptText->setAnchor(0.0f, 0.5f);
        _promptText->setPosition(box.x + box.w + 12.0f, cy);
    }
    _promptText->setVisible(_visible);
}

void Hud::showAnnouncement(const std::string& text, float seconds) {
    _lastAnnouncement = text;
    _announcementTicker = text.empty() ? 0.0f : std::max(seconds, 0.1f);
    if (_announcementText) {
        _announcementText->setText(text);
        _announcementText->setVisible(!text.empty() && _visible);
    }
}

float Hud::teamArrowRotation(const Vec2& worldDir) {
    // ui.ts: `Math.atan2(dir.y, -dir.x) - Math.PI * 0.5`. `dir` is a world
    // vector and indicator.svg points up, and both CSS and axmol treat a
    // positive rotation as clockwise on screen, so no extra flip is needed.
    return std::atan2(worldDir.y, -worldDir.x) - 1.5707963267948966f;
}

void Hud::setTeammateArrows(const std::vector<TeammateArrow>& arrows) {
    _teamArrowState = arrows;
    if (!_built) {
        return;
    }
    for (size_t i = 0; i < _teamArrows.size(); ++i) {
        pix::Container* node = _teamArrows[i];
        if (!node) {
            continue;
        }
        // The web keeps one element per group slot; unset slots stay hidden.
        const auto it = std::find_if(arrows.begin(), arrows.end(),
            [i](const TeammateArrow& arrow) { return arrow.slot == static_cast<int>(i); });
        const bool show = _visible && !_bigMapVisible && it != arrows.end();
        node->setVisible(show);
        if (!show) {
            continue;
        }
        const TeammateArrow& arrow = *it;
        node->setPosition(arrow.pos.x, arrow.pos.y);
        node->setRotation(arrow.rot);
    }
}

void Hud::displayPickupMessage(const std::string& text) {
    _lastPickup = text;
    _pickupTicker = text.empty() ? 0.0f : 3.0f;
    if (_pickupText) {
        _pickupText->setText(text);
        _pickupText->setVisible(!text.empty() && _visible);
    }
}

void Hud::setRoleMenuOptions(const std::string& preferred, const std::vector<std::string>& roles) {
    _roleOptionsState.clear();
    for (const auto& role : roles) {
        if (role.empty()) {
            continue;
        }
        if (static_cast<int>(_roleOptionsState.size()) >= kRoleOptionMax) {
            break;
        }
        _roleOptionsState.push_back(role);
    }
    _roleSelectedPending.clear();
    int selected = 0;
    if (!preferred.empty()) {
        const auto it = std::find(_roleOptionsState.begin(), _roleOptionsState.end(), preferred);
        if (it != _roleOptionsState.end()) {
            selected = static_cast<int>(it - _roleOptionsState.begin());
        }
    }
    if (!_roleOptionsState.empty()) {
        selectRole(selected);
    }
}

void Hud::setRoleMenuActive(bool active) {
    _roleMenuActive = active && !_roleOptionsState.empty();
    if (_roleMenuActive) {
        _roleTicker = kRoleSelectDuration;
        _lastRoleFooter.clear();
        tickRoleMenu(0.0f);
    } else {
        _lastRoleFooter.clear();
        _roleTicker = 0.0f;
    }
    if (_built) {
        setVisible(_visible);
    }
}

void Hud::resetRoleMenu() {
    _roleMenuActive = false;
    _roleOptionsState.clear();
    _roleDisplayed.clear();
    _roleSelectedPending.clear();
    _roleTicker = 0.0f;
    _lastRoleFooter.clear();
    if (_built) {
        setVisible(_visible);
    }
}

void Hud::tickRoleMenu(float dt) {
    if (!_roleMenuActive) {
        return;
    }
    _roleTicker = std::max(0.0f, _roleTicker - dt);
    // ui.ts: `${translate("game-enter-game")} (${Math.ceil(ticker)})`.
    const std::string text =
        "ENTER GAME (" + std::to_string(static_cast<int>(std::ceil(_roleTicker))) + ")";
    if (text != _lastRoleFooter) {
        _lastRoleFooter = text;
        if (_roleFooter) {
            _roleFooter->setText(text);
        }
    }
    if (_roleTicker <= 0.0f) {
        _roleSelectedPending = _roleDisplayed;
        setRoleMenuActive(false);
    }
}

void Hud::confirmRoleSelection() {
    if (!_roleMenuActive) {
        return;
    }
    _roleSelectedPending = _roleDisplayed;
    setRoleMenuActive(false);
}

bool Hud::consumeRoleSelection(std::string& role) {
    if (_roleSelectedPending.empty()) {
        return false;
    }
    role = std::move(_roleSelectedPending);
    _roleSelectedPending.clear();
    return true;
}

Hud::RoleTap Hud::roleMenuActionAt(float designX, float designY, int& option) const {
    option = -1;
    if (!_built || !_roleMenuActive) {
        return RoleTap::None;
    }
    for (size_t i = 0; i < _roleOptionRects.size(); ++i) {
        if (static_cast<int>(i) >= static_cast<int>(_roleOptionsState.size())) {
            break;
        }
        if (_roleOptionRects[i].contains(designX, designY)) {
            option = static_cast<int>(i);
            return RoleTap::Option;
        }
    }
    if (_roleConfirmRect.contains(designX, designY)) {
        return RoleTap::Confirm;
    }
    // The web wrapper covers the viewport (pointer-events: all): a tap outside
    // the panel is swallowed rather than reaching the pads.
    return RoleTap::None;
}

void Hud::selectRole(int index) {
    if (index < 0 || index >= static_cast<int>(_roleOptionsState.size())) {
        return;
    }
    const std::string& role = _roleOptionsState[static_cast<size_t>(index)];
    _roleDisplayed = role;
    const DefProvider* defs = getDefProvider();
    const RoleRenderDef* def = defs ? defs->role(role) : nullptr;

    const auto fitSprite = [](pix::Sprite* sprite, float target) {
        if (!sprite) {
            return;
        }
        const float source = std::max(sprite->sourceWidth(), sprite->sourceHeight());
        if (source > 0.0f) {
            const float scale = target / source;
            sprite->setScale(scale, scale);
        }
    };

    // Header class options: selected art is 164 px, the rest 132 px at 50%
    // opacity (ui.ts setRoleMenuInfo).
    const float rw = kRoleMenuWidth;
    const float total = static_cast<float>(_roleOptionsState.size()) * 160.0f +
        static_cast<float>(static_cast<int>(_roleOptionsState.size()) - 1) * 20.0f;
    const float cx = _design.x * 0.5f;
    const float cy = _design.y * 0.5f;
    for (int i = 0; i < kRoleOptionMax; ++i) {
        pix::Sprite* sprite = _roleOptionSprites[static_cast<size_t>(i)];
        if (!sprite) {
            continue;
        }
        if (i >= static_cast<int>(_roleOptionsState.size())) {
            sprite->setVisible(false);
            _roleOptionRects[static_cast<size_t>(i)] = Rect{};
            continue;
        }
        const std::string& type = _roleOptionsState[static_cast<size_t>(i)];
        const RoleRenderDef* optionDef = defs ? defs->role(type) : nullptr;
        sprite->setFrame(optionDef ? optionDef->guiImg : std::string());
        const bool selected = i == index;
        const float localX = -total * 0.5f + 80.0f + static_cast<float>(i) * 180.0f;
        const float localY = kRoleMenuHeight * 0.5f - 100.0f;
        sprite->setPosition(localX, localY);
        fitSprite(sprite, selected ? 164.0f : 132.0f);
        sprite->setAlpha(selected ? 1.0f : 0.5f);
        sprite->setVisible(_roleMenuActive);
        _roleOptionRects[static_cast<size_t>(i)] = Rect{
            cx + (localX - 80.0f) * _roleScale,
            cy + (localY - 80.0f) * _roleScale,
            160.0f * _roleScale,
            160.0f * _roleScale,
        };
    }

    if (_roleName) {
        _roleName->setText(def && !def->name.empty() ? def->name : role);
    }
    if (_roleImage) {
        const std::string art = def ? def->guiImg : std::string();
        _roleImage->setFrame(art);
        fitSprite(_roleImage, 240.0f);
        _roleImage->setVisible(!art.empty());
    }
    // Border colour: roleDef.color, else the web default #191919.
    if (_rolePanelBg) {
        const float rh = kRoleMenuHeight;
        _rolePanelBg->clear();
        _rolePanelBg->beginFill(0x7692b0, 0.75f);
        _rolePanelBg->drawRect(-rw * 0.5f, -rh * 0.5f, rw, rh);
        _rolePanelBg->endFill();
        _rolePanelBg->lineStyle(8.0f, def && def->hasColor ? def->color : 0x191919, 1.0f);
        _rolePanelBg->drawRect(-rw * 0.5f, -rh * 0.5f, rw, rh);
    }

    const int perkCount = def
        ? std::min(kRolePerkMax, static_cast<int>(def->perks.size()))
        : 0;
    const float spacing = perkCount <= 1
        ? 144.0f
        : std::min(144.0f, 400.0f / static_cast<float>(perkCount - 1));
    const float firstY = -40.0f + spacing * static_cast<float>(perkCount - 1) * 0.5f;
    for (int i = 0; i < kRolePerkMax; ++i) {
        pix::Sprite* ring = _rolePerkRing[static_cast<size_t>(i)];
        pix::Sprite* icon = _rolePerkIcon[static_cast<size_t>(i)];
        pix::Text* name = _rolePerkName[static_cast<size_t>(i)];
        if (!ring || !icon || !name) {
            continue;
        }
        if (i >= perkCount) {
            ring->setVisible(false);
            icon->setVisible(false);
            name->setVisible(false);
            continue;
        }
        const std::string& perk = def->perks[static_cast<size_t>(i)];
        const GameObjRenderDef* perkDef = defs ? defs->gameObject(perk) : nullptr;
        const std::string frame = perkDef ? perkDef->img.sprite : std::string();
        const float y = firstY - spacing * static_cast<float>(i);
        ring->setPosition(-80.0f, y);
        fitSprite(ring, 96.0f);
        ring->setVisible(true);
        icon->setPosition(-80.0f, y);
        icon->setFrame(frame);
        fitSprite(icon, 64.0f);
        icon->setVisible(!frame.empty());
        name->setPosition(-18.0f, y);
        name->setText(def->perkNames.size() > static_cast<size_t>(i)
                ? def->perkNames[static_cast<size_t>(i)]
                : perk);
        name->setVisible(true);
    }
}

void Hud::updateActionButtons() {
    if (!_built) {
        return;
    }
    const bool base = _visible && !_overShown && !_deathShown && !_spectating &&
        !_bigMapVisible && !_menuShown && !_roleMenuActive;

    // Repaint only when the underlying state moved (row types, counts, the
    // equipped slot and the HUD input gating are all part of the key).
    std::string key = base ? "1" : "0";
    key += "|" + _healItem + "|" + _boostItem + "|" + _scopeEquipped + "|" +
        std::to_string(_equippedSlot);
    for (const auto& row : _topLoot) {
        key += "|" + row.type + ":" + std::to_string(row.countValue);
    }
    for (const auto& row : _ammoRows) {
        key += "|" + row.type + ":" + std::to_string(row.countValue);
    }
    for (const auto& slot : _scopeSlots) {
        key += "|" + slot.type;
    }
    if (base && key == _actionKey) {
        return;
    }
    _actionKey = key;

    // Remaining pills.
    paintBox(_reloadBg, _reloadRect, 0x000000, 0.4f, 0.35f);
    _reloadBg->setVisible(base);
    _reloadText->setVisible(base);
    paintBox(_swapBg, _swapRect, 0x000000, 0.4f, 0.35f);
    _swapBg->setVisible(base);
    _swapText->setVisible(base);

    // Top bag counters: bandage / medkit / soda / pills (tap = use, hold = drop).
    for (int i = 0; i < kTopLootCount; ++i) {
        TopLootRow& row = _topLoot[static_cast<size_t>(i)];
        const bool shown = base && row.countValue > 0;
        paintBox(row.bg, row.rect, 0x000000, 0.4f, 0.35f);
        row.bg->setVisible(shown);
        const std::string frame = itemIconFrame(row.type);
        if (shown && !frame.empty()) {
            if (frame != row.lastFrame) {
                row.lastFrame = frame;
                row.icon->setFrame(frame);
                const float source = std::max(row.icon->sourceWidth(), row.icon->sourceHeight());
                const float scale = source > 0.0f ? 22.0f / source : 1.0f;
                row.icon->setScale(scale, scale);
            }
            row.icon->setVisible(_visible);
        } else {
            row.icon->setVisible(false);
            row.lastFrame.clear();
        }
        row.count->setText(std::to_string(row.countValue));
        row.count->setVisible(shown);
    }

    // Ammo column: one row per ammo type with a count in the bag.
    for (int i = 0; i < kAmmoRowCount; ++i) {
        AmmoRow& row = _ammoRows[static_cast<size_t>(i)];
        const bool shown = base && row.countValue > 0;
        paintBox(row.bg, row.rect, 0x000000, 0.4f, 0.35f);
        row.bg->setVisible(shown);
        row.chip->clear();
        row.chip->beginFill(kAmmoTypes[i].color, 0.85f);
        row.chip->drawRect(0.0f, 0.0f, 20.0f, 20.0f);
        row.chip->endFill();
        row.chip->setPosition(row.rect.x + row.rect.w - 30.0f, row.rect.y + kAmmoRowH * 0.5f);
        row.chip->setVisible(shown);
        const float source = std::max(row.icon->sourceWidth(), row.icon->sourceHeight());
        if (source > 0.0f) {
            const float scale = 16.0f / source;
            row.icon->setScale(scale, scale);
            row.icon->setVisible(shown);
        } else {
            row.icon->setVisible(false);
        }
        row.count->setText(std::to_string(row.countValue));
        row.count->setVisible(shown);
    }

    // Scope strip: owned scopes only; the equipped one is highlighted.
    for (int i = 0; i < kScopeSlotCount; ++i) {
        ScopeSlot& slot = _scopeSlots[static_cast<size_t>(i)];
        const bool equipped = !slot.type.empty() && slot.type == _scopeEquipped;
        const bool shown = base && !slot.type.empty();
        if (shown) {
            paintBox(slot.bg, slot.rect, equipped ? 0xffffff : 0x000000,
                     equipped ? 0.3f : 0.4f, 0.35f);
        }
        slot.bg->setVisible(shown);
        slot.label->setText(slot.type.empty() ? std::string() : scopeStripLabel(slot.type));
        slot.label->setVisible(shown);
    }

    // Weapon cards: dim background, brighter for the equipped slot (ui2.ts
    // weapon-state opacity). Text/icons are painted by update().
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
    if (_announcementTicker > 0.0f) {
        _announcementTicker -= dt;
        if (_announcementTicker <= 0.0f) {
            _lastAnnouncement.clear();
            if (_announcementText) {
                _announcementText->setText(std::string());
                _announcementText->setVisible(false);
            }
        }
    }

    // Result / death / spectate state (ui.ts displayStats, showTeamAd and
    // setSpectating). GameOverMsg latches until the next match resets the HUD.
    const GameOverMsg* over = game.gameOver();
    if (over) {
        // The result panel outranks the class picker; never leave the picker
        // covering the final screen.
        if (_roleMenuActive) {
            setRoleMenuActive(false);
        }
        if (!_overShown) {
            showGameOver(*over, game.teamIdForPlayer(game.getLocalPlayerId()),
                         game.getLocalPlayerId(), &game);
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
            std::string name = game.displayName(game.getActivePlayerId());
            if (name.empty()) {
                name = "Player";
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

    // Perk-mode class picker countdown (ui.ts roleMenuTicker): only runs once
    // the match is actually on screen, so a slow join cannot eat the 20 s.
    tickRoleMenu(dt);

    const GameStateSnapshot snap = game.snapshot();
    const ActivePlayerData& apd = game.activePlayerData();

    // ui2.ts perk acquisition toast. Suppress the initial join inventory.
    {
        std::vector<std::string> perks;
        perks.reserve(apd.perks.size());
        for (const auto& perk : apd.perks) {
            perks.push_back(perk.type);
        }
        if (_perksInitialized) {
            for (const auto& type : perks) {
                if (std::find(_lastPerkTypes.begin(), _lastPerkTypes.end(), type) ==
                    _lastPerkTypes.end()) {
                    displayPickupMessage("PERK: " + type);
                }
            }
        }
        _lastPerkTypes = std::move(perks);
        _perksInitialized = true;
    }

    // --- Health: fill width = health%, colour from the ui2.ts steps; at 25 or
    // less the CSS `ui-bar-danger` pulse (black <-> red, 1 s) overrides it.
    const bool downed = snap.health < 0.0f;
    const float health = downed ? 0.0f : std::clamp(snap.health, 0.0f, 100.0f);
    const bool healthChanged = health != _lastHealth || downed;
    const bool healthDanger = health > 0.0f && health <= 25.0f;
    const float healthDangerMix =
        0.5f - 0.5f * std::cos(2.0f * 3.14159265358979f * _squadPulseTicker);
    // #ui-health-depleted: a new target starts the web's 1 s ease-out width
    // transition from the currently rendered trail.
    if (healthChanged) {
        _healthDepletedFrom = _healthDepletedWidth;
        _healthDepletedTo = health;
        _healthDepletedTicker = 0.0f;
    }
    if (healthChanged || healthDanger) {
        _lastHealth = health;
        const float fraction = health / 100.0f;
        const uint32_t color = healthDanger ? healthDangerColor(healthDangerMix)
                                            : healthColor(health, downed);
        const float alpha = healthDanger ? 0.8f : 1.0f;
        _healthFill->clear();
        if (fraction > 0.0f) {
            _healthFill->beginFill(color, alpha);
            _healthFill->drawRect(0.0f, 0.0f, kHealthBarWidth * fraction, kHealthBarHeight);
            _healthFill->endFill();
        }
        const std::string healthText = std::to_string(static_cast<int>(std::round(health)));
        if (healthText != _lastHealthText) {
            _lastHealthText = healthText;
            _healthText->setText(healthText);
        }
    }
    if (_healthDepletedTicker < 1.0f && _healthDepleted) {
        _healthDepletedTicker = std::min(1.0f, _healthDepletedTicker + dt);
        const float t = _healthDepletedTicker;
        const float ease = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t); // ease-out
        _healthDepletedWidth =
            _healthDepletedFrom + (_healthDepletedTo - _healthDepletedFrom) * ease;
        _healthDepleted->clear();
        if (_healthDepletedTo > 0.0f && _healthDepletedWidth > 0.01f) {
            _healthDepleted->beginFill(0x929292, 1.0f);
            _healthDepleted->drawRect(0.0f, 0.0f,
                kHealthBarWidth * _healthDepletedWidth / 100.0f, kHealthBarHeight);
            _healthDepleted->endFill();
        }
    }

    // ui.ts faction flair: the red/blue player patch on both sides of the
    // health counter (map.factionMode only; team 1 = red, 2 = blue).
    if (_flairLeft && _flairRight) {
        const bool factionTeam = _factionMode &&
            (_localTeamId == FactionTeam_Red || _localTeamId == FactionTeam_Blue);
        if (factionTeam) {
            const std::string frame = _localTeamId == FactionTeam_Red
                ? "gui/player-patch-red.png"
                : "gui/player-patch-blue.png";
            if (_flairFrame != frame) {
                _flairLeft->setFrame(frame);
                _flairRight->setFrame(frame);
                _flairFrame = frame;
                const float source = std::max(_flairLeft->sourceWidth(), _flairLeft->sourceHeight());
                const float scale = source > 1.0f ? 64.0f / source : 1.0f;
                _flairLeft->setScale(scale, scale);
                _flairRight->setScale(scale, scale);
            }
        }
        _flairLeft->setVisible(_visible && factionTeam);
        _flairRight->setVisible(_visible && factionTeam);
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

    // --- Weapon cards: name + loot-img icon per slot; ammo of the equipped gun.
    std::string weaponKey;
    std::string ammoText;
    const int slotCount = std::min<int>(4, static_cast<int>(apd.weapons.size()));
    _weaponTypes.fill(std::string());
    for (int i = 0; i < 4; ++i) {
        if (i < slotCount && !apd.weapons[i].type.empty()) {
            weaponKey += apd.weapons[i].type;
            _weaponTypes[static_cast<size_t>(i)] = apd.weapons[i].type;
        }
        weaponKey += '\n';
    }
    if (weaponKey != _lastWeapon) {
        _lastWeapon = weaponKey;
        size_t start = 0;
        for (int i = 0; i < 4; ++i) {
            const size_t end = weaponKey.find('\n', start);
            const std::string type =
                weaponKey.substr(start, end == std::string::npos ? std::string::npos : end - start);
            if (!type.empty()) {
                _weaponSlots[i]->setText(itemLabel(type));
                const std::string frame = itemIconFrame(type);
                if (!frame.empty()) {
                    _weaponIcons[i]->setFrame(frame);
                    const float source =
                        std::max(_weaponIcons[i]->sourceWidth(), _weaponIcons[i]->sourceHeight());
                    const float scale = source > 0.0f ? 30.0f / source : 1.0f;
                    _weaponIcons[i]->setScale(scale, scale);
                    _weaponIcons[i]->setVisible(_visible);
                } else {
                    _weaponIcons[i]->setVisible(false);
                }
                _weaponSlots[i]->setVisible(_visible);
            } else {
                _weaponSlots[i]->setText(std::string());
                _weaponSlots[i]->setVisible(false);
                _weaponIcons[i]->setVisible(false);
            }
            start = end == std::string::npos ? weaponKey.size() : end + 1;
        }
    }
    // ui2.ts reserve ammo: equipped clip + the bag count for its ammo type.
    if (apd.curWeapIdx >= 0 && apd.curWeapIdx < slotCount) {
        const auto& weap = apd.weapons[static_cast<size_t>(apd.curWeapIdx)];
        const GameObjRenderDef* def = getDefProvider() ? getDefProvider()->gameObject(weap.type) : nullptr;
        if (def && !def->ammoType.empty() && weap.ammo >= 0) {
            std::string text = std::to_string(weap.ammo);
            text += "/" + std::to_string(bagCount(apd.inventory, def->ammoType.c_str()));
            if (text != _lastAmmoText) {
                _lastAmmoText = text;
                _ammoText->setText(text);
            }
        } else if (!_lastAmmoText.empty()) {
            _lastAmmoText.clear();
            _ammoText->setText(std::string());
        }
    } else if (!_lastAmmoText.empty()) {
        _lastAmmoText.clear();
        _ammoText->setText(std::string());
    }

    // --- Top bag counters + ammo column (ui2.ts loot strips) ---
    for (int i = 0; i < kTopLootCount; ++i) {
        TopLootRow& row = _topLoot[static_cast<size_t>(i)];
        row.countValue = bagCount(apd.inventory, row.type.c_str());
    }
    for (int i = 0; i < kAmmoRowCount; ++i) {
        AmmoRow& row = _ammoRows[static_cast<size_t>(i)];
        row.countValue = bagCount(apd.inventory, row.type.c_str());
    }
    // --- Equipped scope (the strip highlights it) ---
    _scopeEquipped = apd.scope;
    for (int i = 0; i < kScopeSlotCount; ++i) {
        ScopeSlot& slot = _scopeSlots[static_cast<size_t>(i)];
        const int count = bagCount(apd.inventory, kScopeTypes[i]);
        slot.type = count > 0 ? kScopeTypes[i] : std::string();
    }

    // --- P0.1 action controls: quick-use items, scopes ---
    _healItem = healItemFor(apd.inventory, health);
    _boostItem = boostItemFor(apd.inventory, boost);
    updateActionButtons();

    // --- Alive counter and kill counter (info bar under the minimap) ---
    const auto counts = game.aliveCounts();
    const int alive = std::accumulate(counts.begin(), counts.end(), 0);
    std::string aliveText;
    if (_factionMode && counts.size() >= 2) {
        // ui.ts faction mode: red/blue counts instead of one total.
        aliveText = std::to_string(counts[0]) + ":" + std::to_string(counts[1]);
    } else {
        aliveText = std::to_string(alive);
    }
    if (aliveText != _lastAliveText) {
        _lastAliveText = aliveText;
        _aliveText->setText(aliveText);
    }
    int kills = game.localKills();
    if (const GameOverMsg* overMsg = game.gameOver()) {
        for (const auto& stats : overMsg->playerStats) {
            if (stats.playerId == game.getLocalPlayerId()) {
                kills = stats.kills;
            }
        }
    }
    if (kills != _lastKills) {
        _lastKills = kills;
        const std::string text = std::to_string(kills) + "K";
        if (text != _lastKillsText) {
            _lastKillsText = text;
            _killsText->setText(text);
        }
    }
    // ui.ts "You killed X" feedback. Game::killFeed keeps the newest entry at
    // the back; only show a toast when the local credited kill count grows.
    if (!over && _lastLocalKills >= 0 && kills > _lastLocalKills) {
        const uint16_t localId = game.getLocalPlayerId();
        for (auto it = game.killFeed().rbegin(); it != game.killFeed().rend(); ++it) {
            const uint16_t credit = it->message.killCreditId != 0 ? it->message.killCreditId
                                                                  : it->message.killerId;
            if (credit == localId && it->message.killed) {
                const std::string target =
                    it->targetName.empty() ? std::string("PLAYER") : it->targetName;
                displayPickupMessage("YOU KILLED " + target);
                break;
            }
        }
    }
    _lastLocalKills = kills;

    // --- Kill feed (#ui-killfeed): one coloured row per entry ---
    const auto& feed = game.killFeed();
    std::string feedKey;
    for (const auto& entry : feed) {
        if (entry.custom) {
            feedKey += "role:" + entry.text + "|" + std::to_string(entry.color) + ";";
            continue;
        }
        feedKey += std::to_string(entry.message.killerId) + ">" +
            std::to_string(entry.message.targetId) + "|" + entry.killerName + "|" +
            entry.targetName + ";";
    }
    if (feedKey != _lastFeed) {
        _lastFeed = feedKey;
        const uint16_t localId = game.getLocalPlayerId();
        for (int i = 0; i < 6; ++i) {
            pix::Text* row = _killFeedRows[static_cast<size_t>(i)];
            if (!row) {
                continue;
            }
            if (i >= static_cast<int>(feed.size())) {
                row->setText(std::string());
                row->setVisible(false);
                _killFeedHas[static_cast<size_t>(i)] = false;
                continue;
            }
            const Game::KillFeedEntry& entry = feed[static_cast<size_t>(i)];
            if (entry.custom) {
                row->setText(entry.text);
                row->setColor(entry.color, 0x000000, 1.0f, false);
                row->setVisible(_visible && !_bigMapVisible);
                _killFeedHas[static_cast<size_t>(i)] = true;
                continue;
            }
            const std::string text = entry.killerName.empty()
                ? entry.targetName
                : entry.killerName + " -> " + entry.targetName;
            row->setText(text);
            if (localId != 0 && entry.message.killerId == localId) {
                row->setColor(0x8fff8f, 0x000000, 1.0f, false);
            } else if (localId != 0 && entry.message.targetId == localId) {
                row->setColor(0xff8585, 0x000000, 1.0f, false);
            } else {
                row->setColor(0xffc896, 0x000000, 1.0f, false);
            }
            row->setVisible(_visible && !_bigMapVisible);
            _killFeedHas[static_cast<size_t>(i)] = true;
        }
    }

    // ui-gas-icon danger pulse while the red zone is moving (2 s cycle).
    _gasPulseTicker += dt;
    if (_gasIcon) {
        if (_gas.active && _gas.moving) {
            const float t = 0.5f - 0.5f * std::cos(3.14159265358979f * _gasPulseTicker);
            _gasIcon->setAlpha(0.55f + 0.45f * t);
        } else {
            _gasIcon->setAlpha(1.0f);
        }
    }

    // ui-team-member-status-downed `icon-pulse` (2s black -> red -> black).
    _squadPulseTicker += dt;
    const float pulseT = 0.5f - 0.5f * std::cos(3.14159265358979f * _squadPulseTicker);
    const float squadDangerMix =
        0.5f - 0.5f * std::cos(2.0f * 3.14159265358979f * _squadPulseTicker);
    for (size_t i = 0; i < _squad.size(); ++i) {
        SquadRow& row = _squad[i];
        if (i < _squadState.size() && _squadState[i].downed) {
            if (row.statusIcon) {
                row.statusIcon->setAlpha(0.55f + 0.45f * pulseT);
            }
            if (row.colorDot) {
                row.colorDot->setAlpha(0.55f + 0.45f * pulseT);
            }
        }
        // ui.ts `.ui-team-member-health .ui-bar-danger`: a living teammate at
        // 25 health or less pulses black <-> red (1 s) like the main bar.
        if (i < _squadState.size()) {
            const SquadMember& m = _squadState[i];
            if (!m.dead && m.health <= 25.0f && row.barFill) {
                const float fillWidth =
                    std::max(1.0f, kSquadBarWidth * std::clamp(m.health, 0.0f, 100.0f) / 100.0f);
                row.barFill->clear();
                row.barFill->beginFill(healthDangerColor(squadDangerMix), 0.8f);
                row.barFill->drawRect(0.0f, 0.0f, fillWidth, 10.0f);
                row.barFill->endFill();
            }
        }
    }
}

} // namespace surv
