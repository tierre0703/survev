#pragma once
// Plain (non-FairyGUI) in-game HUD, drawn through the pix adapter on the game
// root, i.e. in the 1280x720 design space of the scene (the same space the menu
// uses via `ui::layoutUiRoot`). It mirrors the web HUD's layout
// (client/index.html + client/css/game.css):
//
//   #ui-bottom-center-0  health bar (400x32) + boost, bottom centre
//   #ui-map-wrapper      minimap + alive counter, top right
//   #ui-bottom-right     weapon cards / ammo, bottom right
//   #ui-bottom-center-2  pickup message, below the centre
//   #ui-killfeed         kill feed, right side
//
// Engine-independent like the rest of `src/game`: it only uses pix::Factory
// nodes and reads the Game snapshot, so `surv_tests` exercises it with
// NullPixi.
#include "../core/Vec2.h"
#include "../render/PixiLike.h"
#include "EmoteManager.h"
#include "MapView.h"

#include <string>
#include <vector>

namespace surv {

class Game;

// ui.ts updateGasDisplay: gas countdown shown under the minimap.
struct GasHudState {
    bool active = false;
    bool moving = false;
    float secondsLeft = 0.0f;
};

class Hud {
public:
    // Web sizes (client/css/game.css).
    static constexpr float kHealthBarWidth = 400.0f;
    static constexpr float kHealthBarHeight = 32.0f;
    static constexpr float kHealthBarPadding = 6.0f;
    static constexpr float kBoostBarHeight = 8.0f;
    static constexpr float kMinimapSize = 140.0f;
    static constexpr float kWeaponSlotWidth = 160.0f;
    static constexpr float kWeaponSlotHeight = 30.0f;

    // Builds every node under `root`; `design` is the design-space rect.
    void build(pix::Factory* factory, pix::Container* root, const Vec2& design);

    // Per-frame repaint (only the widgets whose value changed are touched).
    void update(float dt, const Game& game, bool visible);
    void setGasState(const GasHudState& gas);

    // ui.ts displayStats: result screen (victory/defeat, rank, team kills and
    // the local player's kills/damage/time). Called with GameOverMsg data.
    void showGameOver(const GameOverMsg& over, uint8_t localTeamId, uint16_t localPlayerId);
    bool gameOverShown() const { return _overShown; }
    static std::string formatTime(uint16_t seconds);

    // map.ts renderMap(): show the baked minimap texture under the frame.
    void setMinimap(pix::RenderTexture* texture, float mapWidth, float mapHeight);
    // ui.ts createPing / mapSprite.ts: draw the active minimap ping sprites.
    void updateMapPings(const std::vector<MapPing>& pings);
    // ui.ts updatePlayerMapSprites + m_render: player/team dots and gas.
    void updateMapOverlay(const std::vector<MapDot>& dots, const MapGas& gas,
                          const std::vector<MapAirstrike>& strikes);
    // map.ts renderMap place names, over both map views.
    void setMapLabels(const std::vector<MapPlace>& places);

    // Full-screen map (#big-map): toggled from the minimap's expand button.
    static constexpr float kBigMapSize = 640.0f;
    static constexpr float kMinimapExpandSize = 22.0f;
    bool bigMapVisible() const { return _bigMapVisible; }
    void setBigMapVisible(bool visible);
    void toggleBigMap() { setBigMapVisible(!_bigMapVisible); }
    bool minimapExpandHit(float designX, float designY) const;
    bool bigMapContains(float designX, float designY) const;
    bool bigMapCloseHit(float designX, float designY) const;

    // #ui-pickup-message: shows 	ext for 3s (ui2.ts pickup duration).
    void displayPickupMessage(const std::string& text);

    void setVisible(bool visible);
    bool isVisible() const { return _visible; }

    // Health fraction/colour as the web HUD interpolates them (ui2.ts
    // healthColourSteps); exposed for the host tests.
    static uint32_t healthColor(float health, bool downed);

    // Diagnostics for the host tests (map.ts renderMap / ui.ts createPing).
    const MapView& minimap() const { return _miniMap; }
    const MapView& bigMap() const { return _bigMap; }
    const pix::Text* gasText() const { return _gasText; }
    const pix::Text* gameOverTitleText() const { return _overTitle; }
    const pix::Text* gameOverRankText() const { return _overRank; }
    const pix::Text* gameOverStatsText() const { return _overStats; }

private:
    void layout(pix::Graphics* g, float w, float h) const;

    pix::Factory* _factory = nullptr;
    pix::Container* _root = nullptr;

    pix::Graphics* _healthBg = nullptr;
    pix::Graphics* _healthFill = nullptr;
    pix::Text* _healthText = nullptr;
    pix::Graphics* _boostSegments[4] = {nullptr, nullptr, nullptr, nullptr};
    pix::Text* _ammoText = nullptr;

    pix::Container* _mapBox = nullptr;
    pix::Graphics* _mapFrame = nullptr;
    MapView _miniMap;
    MapView _bigMap;
    pix::Graphics* _bigMapBackdrop = nullptr;
    pix::Graphics* _bigMapCloseBg = nullptr;
    pix::Text* _bigMapCloseText = nullptr;
    pix::Graphics* _mapExpandBg = nullptr;
    pix::Graphics* _mapExpandIcon = nullptr;
    Vec2 _bigMapPos;
    Vec2 _expandPos;
    bool _bigMapVisible = false;
    pix::Text* _aliveText = nullptr;
    pix::Text* _killsText = nullptr;
    pix::Text* _killLeaderText = nullptr;
    std::string _lastKillLeader;

    // Gas countdown (ui.ts #ui-gas-timer) and the result screen.
    pix::Text* _gasText = nullptr;
    GasHudState _gas;
    std::string _lastGasText;

    pix::Graphics* _overBackdrop = nullptr;
    pix::Graphics* _overPanel = nullptr;
    pix::Text* _overTitle = nullptr;
    pix::Text* _overRank = nullptr;
    pix::Text* _overStats = nullptr;
    bool _overShown = false;

    pix::Container* _weaponBox = nullptr;
    pix::Graphics* _weaponBoxBg = nullptr;
    pix::Text* _weaponSlots[4] = {nullptr, nullptr, nullptr, nullptr};

    pix::Text* _pickupText = nullptr;
    pix::Text* _killFeedText = nullptr;

    Vec2 _design = Vec2(1280.0f, 720.0f);
    // Bottom-left of the minimap frame in the centred HUD container.
    Vec2 _minimapPos;
    bool _visible = false;
    bool _built = false;

    // Change detection (only repaint what moved).
    float _lastHealth = -1.0f;
    float _lastBoost = -1.0f;
    int _lastAlive = -1;
    int _lastKills = -1;
    std::string _lastWeapon;
    std::string _lastPickup;
    std::string _lastFeed;
    float _pickupTicker = 0.0f;
};

} // namespace surv
