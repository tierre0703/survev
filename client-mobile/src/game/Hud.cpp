#include "Hud.h"

#include "Game.h"
#include "EmoteManager.h"

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

    _built = true;
    setVisible(false);
}

void Hud::setVisible(bool visible) {
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
    const bool over = visible && _overShown;
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
        _overRank->setVisible(over);
    }
    if (_overStats) {
        _overStats->setVisible(over);
    }
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
    _bigMapVisible = false;
    setVisible(_visible);
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

    // Result screen (ui.ts displayStats): GameOverMsg latches until the next
    // match resets the HUD.
    if (const GameOverMsg* over = game.gameOver()) {
        if (!_overShown) {
            showGameOver(*over, game.teamIdForPlayer(game.getLocalPlayerId()), game.getLocalPlayerId());
        }
    } else if (_overShown) {
        _overShown = false;
        setVisible(_visible);
    }

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
