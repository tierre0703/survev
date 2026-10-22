#include "Hud.h"

#include "Game.h"

#include <algorithm>
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

    // #ui-bottom-right: four weapon cards.
    _weaponBoxBg = _factory->createGraphics();
    _root->addChild(_weaponBoxBg);
    for (auto*& slot : _weaponSlots) {
        slot = _factory->createText();
        slot->setFontSize(13.0f);
        slot->setColor(0xdcdcdc, 0x000000, 1.0f, false);
        slot->setAnchor(0.0f, 0.0f);
        _root->addChild(slot);
    }

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
    const float vw = _design.x;
    const float vh = _design.y;
    const float hudW = kHealthBarWidth + kHealthBarPadding * 2.0f;
    const float hudH = kHealthBarHeight + kHealthBarPadding * 2.0f;
    // The HUD container's origin is the design rect's centre (uiRoot-style),
    // so everything below is placed relative to it.
    if (_root) {
        _root->setPosition(vw * 0.5f, vh * 0.5f);
    }
    const float hudX = -hudW * 0.5f;
    const float hudY = vh * 0.5f - 12.0f - hudH;

    _healthBg->clear();
    _healthBg->beginFill(0x000000, 0.4f);
    _healthBg->drawRect(hudX, hudY, hudW, hudH);
    _healthBg->endFill();

    _healthFill->setPosition(hudX + kHealthBarPadding, hudY + kHealthBarPadding);
    _healthText->setPosition(hudX + hudW * 0.5f, hudY + hudH * 0.5f);
    _ammoText->setPosition(hudX + hudW + 16.0f, hudY + hudH);

    const float mm = kMinimapSize;
    const float mmX = vw * 0.5f - mm - 12.0f;
    const float mmY = -vh * 0.5f + 12.0f;
    _mapFrame->clear();
    _mapFrame->lineStyle(2.0f, 0xffffff, 0.6f);
    _mapFrame->moveTo(mmX, mmY);
    _mapFrame->lineTo(mmX + mm, mmY);
    _mapFrame->lineTo(mmX + mm, mmY + mm);
    _mapFrame->lineTo(mmX, mmY + mm);
    _mapFrame->lineTo(mmX, mmY);
    _aliveText->setPosition(mmX - 12.0f, 10.0f);
    _killsText->setPosition(mmX - 12.0f, 32.0f);

    const float boxW = kWeaponSlotWidth + 16.0f;
    const float boxH = 4.0f * (kWeaponSlotHeight + 2.0f) + 12.0f;
    const float boxX = vw * 0.5f - boxW - 8.0f;
    const float boxY = vh * 0.5f - boxH - 8.0f;
    _weaponBoxBg->clear();
    _weaponBoxBg->beginFill(0x000000, 0.25f);
    _weaponBoxBg->drawRect(boxX, boxY, boxW, boxH);
    _weaponBoxBg->endFill();
    for (int i = 0; i < 4; ++i) {
        const float y = boxY + 6.0f + static_cast<float>(i) * (kWeaponSlotHeight + 2.0f);
        _weaponSlots[i]->setPosition(boxX + 8.0f, y);
    }

    _pickupText->setPosition(0.0f, vh * 0.12f);
    _killFeedText->setPosition(vw * 0.5f - 16.0f, -vh * 0.5f + 12.0f);

    // The boost segments are placed relative to the health block here; update()
    // only repaints their width.
    {
        float x = hudX + kHealthBarPadding;
        for (int i = 0; i < 4; ++i) {
            const float span = kBoostBreakpoints[i] / kBoostBreakpointsTotal;
            _boostSegments[i]->setPosition(x, hudY - 4.0f);
            x += kHealthBarWidth * span;
        }
    }

    _built = true;
    setVisible(false);
}

void Hud::setVisible(bool visible) {
    if (!_built) {
        _visible = visible;
        return;
    }
    _visible = visible;
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
    show(_weaponBoxBg);
    show(_killFeedText);
    for (auto* seg : _boostSegments) {
        show(seg);
    }
    for (auto* slot : _weaponSlots) {
        show(slot);
    }
    if (!visible) {
        show(_pickupText);
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

void Hud::update(float dt, const Game& game, bool visible) {
    if (!_built) {
        return;
    }
    setVisible(visible);

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
