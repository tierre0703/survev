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

#include <string>
#include <vector>

namespace surv {

class Game;

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
    // `minimapDisplay`, when given, is adopted as the minimap contents.
    void build(pix::Factory* factory, pix::Container* root, const Vec2& design,
               const pix::Node* minimapDisplay = nullptr);

    // Per-frame repaint (only the widgets whose value changed are touched).
    void update(float dt, const Game& game, bool visible);

    // map.ts renderMap(): show the baked minimap texture under the frame.
    void setMinimap(pix::RenderTexture* texture);
    // ui.ts createPing / mapSprite.ts: draw the active minimap ping sprites.
    void updateMapPings(const std::vector<MapPing>& pings, float mapWidth, float mapHeight);

    // #ui-pickup-message: shows 	ext for 3s (ui2.ts pickup duration).
    void displayPickupMessage(const std::string& text);

    void setVisible(bool visible);
    bool isVisible() const { return _visible; }

    // Health fraction/colour as the web HUD interpolates them (ui2.ts
    // healthColourSteps); exposed for the host tests.
    static uint32_t healthColor(float health, bool downed);

    // Diagnostics for the host tests (map.ts renderMap / ui.ts createPing).
    const pix::Sprite* minimapSprite() const { return _minimap; }
    const pix::Sprite* mapPingSprite(int idx) const {
        return (idx >= 0 && idx < kMapPingPool) ? _mapPingSprites[idx] : nullptr;
    }
    const Vec2& minimapPos() const { return _minimapPos; }

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
    pix::Sprite* _minimap = nullptr;
    // Map ping sprites live in a container clipped to the minimap rect (the
    // web's mapSpriteBarn sits inside the minimap container).
    pix::Container* _mapPingLayer = nullptr;
    pix::Graphics* _mapPingMask = nullptr;
    static constexpr int kMapPingPool = 32;
    pix::Sprite* _mapPingSprites[kMapPingPool] = {};
    pix::Text* _aliveText = nullptr;
    pix::Text* _killsText = nullptr;

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
