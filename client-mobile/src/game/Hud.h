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
// Touch action controls (P0.1): reload + interaction buttons (#ui-reload-button
// and the touch #ui-interaction tap target), a quick-use heal/boost pair, a
// scope cycle pill, a weapon-slot swap pill, and the four weapon cards as
// tap-to-equip / long-press-to-drop targets. The web drives these through
// DOM buttons; the native HUD exposes hitTest() so GameScene can route a touch
// into the InputActions queue instead of the joystick pads.
//
// Engine-independent like the rest of `src/game`: it only uses pix::Factory
// nodes and reads the Game snapshot, so `surv_tests` exercises it with
// NullPixi.
#include "../core/Vec2.h"
#include "../net/Messages.h"
#include "../render/PixiLike.h"
#include "EmoteManager.h"
#include "MapView.h"

#include <array>
#include <string>
#include <vector>

namespace surv {

class Game;

// ui.ts team squad bars / FguiHud::buildSquadBars input.
struct SquadMember {
    std::string name;
    float health = 100.0f;
    bool dead = false;
    bool downed = false;
    bool disconnected = false;
    bool active = false;
};

// ui.ts updateGasDisplay: gas countdown shown under the minimap.
struct GasHudState {
    bool active = false;
    bool moving = false;
    float secondsLeft = 0.0f;
};

// ui.ts team arrows: a teammate outside the camera view.
struct TeammateArrow {
    Vec2 dir = Vec2(1.0f, 0.0f);
    uint32_t tint = 0xffffff;
    bool downed = false;
};

class Hud {
public:
    // Web sizes (client/css/game.css).
    static constexpr float kHealthBarWidth = 400.0f;
    static constexpr float kHealthBarHeight = 32.0f;
    static constexpr float kHealthBarPadding = 6.0f;
    static constexpr float kBoostBarHeight = 8.0f;
    static constexpr float kMinimapSize = 140.0f;
    // Gear (helmet/chest/backpack) + up to 4 perk icons.
    static constexpr int kGearIconCount = 7;
    static constexpr float kWeaponSlotWidth = 160.0f;
    static constexpr float kWeaponSlotHeight = 30.0f;

    // P0.1 action controls (design px, bottom-left origin).
    static constexpr float kReloadButtonWidth = 108.0f;
    static constexpr float kReloadButtonHeight = 44.0f;
    static constexpr float kInteractButtonWidth = 180.0f;
    static constexpr float kInteractButtonHeight = 56.0f;
    static constexpr float kPillWidth = 160.0f;
    static constexpr float kPillHeight = 42.0f;
    static constexpr float kSmallPillWidth = 110.0f;
    // Touch target around the #ui-interaction prompt (the web binds touchstart
    // to the prompt element itself, not just the action pill).
    static constexpr float kPromptHitWidth = 240.0f;
    static constexpr float kPromptHitHeight = 40.0f;
    // Long-press a pill/weapon card to drop instead of use/equip (the web's
    // touch UI only binds tap-to-use; drop is a native touch affordance for the
    // mouse right-click action in ui2.ts itemActions).
    static constexpr float kLongPressSeconds = 0.45f;

    // Result of a HUD action hit test (design coords).
    enum class TapKind {
        None,
        Reload,
        Interact,
        Heal,
        Boost,
        ScopeNext,
        SwapWeapons,
        WeaponSlot,
        GearSlot,
    };
    struct TapResult {
        TapKind kind = TapKind::None;
        int slot = -1;
    };

    // Builds every node under `root`; `design` is the design-space rect.
    void build(pix::Factory* factory, pix::Container* root, const Vec2& design);

    // Per-frame repaint (only the widgets whose value changed are touched).
    void update(float dt, const Game& game, bool visible);
    void setGasState(const GasHudState& gas);
    // Team modes: up to 4 members (name, health bar, Dead/Down/DC).
    void setSquadState(const std::vector<SquadMember>& members);
    // ui2.ts gear/perk slots: resolved atlas frames, gear then perks (max 7).
    void setGearState(const std::vector<std::string>& frames);
    // Full ui2.ts slot state: frame + item type (for tap-to-drop), gear level
    // (Lvl. N badge) and perk droppable flag.
    struct GearSlot {
        std::string frame;
        std::string type;
        int level = 0;
        bool droppable = true;
    };
    void setGearSlots(const std::vector<GearSlot>& slots);
    // ui.ts #ui-interaction: nearby item / action text ("" hides it).
    void setInteractionPrompt(const std::string& text);
    // ui.ts announcements (role assigned/dead, rare loot). Visible for seconds.
    void showAnnouncement(const std::string& text, float seconds = 3.5f);
    // game.ts faction mode: two alive counts (red/blue) instead of one total.
    void setFactionMode(bool enabled) { _factionMode = enabled; }
    // ui.ts off-screen teammate arrows around the screen edge.
    void setTeammateArrows(const std::vector<TeammateArrow>& arrows);
    // ui.ts #ui-waiting-text: shown after join until the match starts.
    void setWaitingForPlayers(bool waiting);

    // --- Perk-mode class picker (ui.ts setRoleMenuOptions / setRoleMenuActive) --
    // Shown on a perk map until the local player picks a class (or the 20 s
    // timer runs out). `preferred` is config `perkModeRole`; the web picks
    // roles[0] when it is not in the map's list.
    static constexpr int kRoleOptionMax = 8;
    static constexpr int kRolePerkMax = 4;
    static constexpr float kRoleMenuWidth = 1200.0f;
    static constexpr float kRoleMenuHeight = 770.0f; // 200 header + 450 body + 120 footer
    static constexpr float kRoleSelectDuration = 20.0f;
    void setRoleMenuOptions(const std::string& preferred, const std::vector<std::string>& roles);
    void setRoleMenuActive(bool active);
    bool roleMenuActive() const { return _roleMenuActive; }
    // Clears the picker state for a new match (the web rebuilds it on MapMsg).
    void resetRoleMenu();
    // ui.ts roleMenuTicker + footer countdown, split out for the host tests.
    void tickRoleMenu(float dt);
    enum class RoleTap {
        None,
        Option,
        Confirm,
    };
    // `option` receives the clicked class index for RoleTap::Option.
    RoleTap roleMenuActionAt(float designX, float designY, int& option) const;
    // ui.ts .ui-role-option click / #ui-role-footer-enter click.
    void selectRoleOption(int index) { selectRole(index); }
    void confirmRoleSelection();
    // The confirmed/timed-out class, consumed once by GameScene to send
    // PerkModeRoleSelectMsg (game.ts:697-705).
    bool consumeRoleSelection(std::string& role);
    const std::string& roleMenuDisplayed() const { return _roleDisplayed; }
    int roleMenuOptionCount() const { return static_cast<int>(_roleOptionsState.size()); }

    // ui.ts displayStats: result screen (victory/defeat, rank, team kills and
    // the local player's kills/damage/time). Called with GameOverMsg data.
    void showGameOver(const GameOverMsg& over, uint8_t localTeamId, uint16_t localPlayerId,
                      const Game* game = nullptr);
    bool gameOverShown() const { return _overShown; }
    static std::string formatTime(uint16_t seconds);

    // P0.4 death/spectate. PlayerStatsMsg arrives when the local player dies in
    // a team match and teammates are still alive (the web showTeamAd screen);
    // it shows the local stats with SPECTATE/LEAVE. Hud::update() swaps this for
    // the final result when GameOverMsg lands and shows the spectating overlay
    // once the server moves activePlayerId to a teammate.
    void showDeathPanel(const PlayerStatsMsg& stats);
    bool deathPanelShown() const { return _deathShown; }
    bool spectating() const { return _spectating; }
    bool overlayShown() const { return _overShown || _deathShown; }
    // True while the result/death panel is visible (touches are modal).
    enum class OverlayAction {
        None,
        SpectateBegin,
        SpectateNext,
        SpectatePrev,
        ToggleStats,
        Leave,
    };
    OverlayAction overlayActionAt(float designX, float designY) const;
    // Spectate stats panel toggle (ui.ts toggleLocalStats / View Match Stats).
    void toggleSpectateStats();
    bool spectateStatsShown() const { return _specStatsShown; }

    // map.ts renderMap(): show the baked minimap texture under the frame.
    void setMinimap(pix::RenderTexture* texture, float mapWidth, float mapHeight);
    // ui.ts createPing / mapSprite.ts: draw the active minimap ping sprites.
    void updateMapPings(const std::vector<MapPing>& pings);
    // ui.ts updatePlayerMapSprites + m_render: player/team dots and gas.
    void updateMapOverlay(const std::vector<MapDot>& dots, const MapGas& gas,
                          const std::vector<MapAirstrike>& strikes);
    // mapIndicator.ts: role/item markers (def sprite + pulse ring) on both views.
    void updateMapIndicators(const std::vector<MapIndicatorMarker>& indicators);
    // map.ts renderMap place names, over both map views.
    void setMapLabels(const std::vector<MapPlace>& places);
    // ui.ts: keep the player-centred minimap scrolled to the active player.
    void setMapCenter(const Vec2& worldPos);

    // Full-screen map (#big-map): toggled from the minimap's expand button.
    static constexpr float kBigMapSize = 640.0f;
    static constexpr float kMinimapExpandSize = 22.0f;
    bool bigMapVisible() const { return _bigMapVisible; }
    void setBigMapVisible(bool visible);
    void toggleBigMap() { setBigMapVisible(!_bigMapVisible); }
    // ui.ts cycleVisibilityMode (V): hide/show the minimap without opening the
    // big map. Touch users have no button for this, matching the web.
    bool minimapHidden() const { return _minimapHidden; }
    void setMinimapHidden(bool hidden);
    void toggleMinimapHidden() { setMinimapHidden(!_minimapHidden); }
    bool minimapExpandHit(float designX, float designY) const;
    bool bigMapContains(float designX, float designY) const;
    bool bigMapCloseHit(float designX, float designY) const;

    // P0.1: action controls. `designX/Y` are HUD design-space coords (0..1280,
    // 0..720 bottom-left); only live while the HUD is visible and no result
    // screen/map is open. Touch order is buttons first, then weapon cards.
    TapResult hitTest(float designX, float designY) const;

    // ui2.ts loot/scope strip helpers, reduced to the single quick-use pills the
    // touch HUD shows. `inventory` is ActivePlayerData.inventory (kBagSizeKeys
    // order). Returns "" when the item is not in the bag.
    static int bagCount(const std::vector<uint16_t>& inventory, const char* type);
    static std::string healItemFor(const std::vector<uint16_t>& inventory, float health);
    static std::string boostItemFor(const std::vector<uint16_t>& inventory, float boost);
    // "SCOPE 4X" for 4xscope, "SCOPE" when no scope is equipped.
    static std::string scopeLabel(const std::string& scope);

    // Currently selected quick-use items ("" = empty bag/hidden pill). The pills
    // feed these to InputActions::use; long-press drops them.
    const std::string& healItem() const { return _healItem; }
    const std::string& boostItem() const { return _boostItem; }
    const std::string& scopeType() const { return _scopeType; }
    // Weapon type in a slot ("" = empty), used for Long-press DropItemMsg.
    const std::string& weaponType(int slot) const;

    // #ui-pickup-message: shows 	ext for 3s (ui2.ts pickup duration).
    void displayPickupMessage(const std::string& text);

    void setVisible(bool visible);
    bool isVisible() const { return _visible; }

    // Health fraction/colour as the web HUD interpolates them (ui2.ts
    // healthColourSteps); exposed for the host tests.
    static uint32_t healthColor(float health, bool downed);
    // CSS `ui-bar-danger` pulse (0 = black, 1 = red) and the ui.ts squad-bar
    // palette; both are pure so the host tests can pin the colours.
    static uint32_t healthDangerColor(float mix);
    static uint32_t squadHealthColor(float health, bool dead, bool downed);

    // Diagnostics for the host tests (map.ts renderMap / ui.ts createPing).
    const MapView& minimap() const { return _miniMap; }
    const MapView& bigMap() const { return _bigMap; }
    const pix::Text* gasText() const { return _gasText; }
    const pix::Sprite* gasIcon() const { return _gasIcon; }
    const pix::Text* gameOverTitleText() const { return _overTitle; }
    const pix::Text* gameOverRankText() const { return _overRank; }
    const pix::Text* gameOverStatsText() const { return _overStats; }
    const pix::Text* gameOverStatRow(int idx) const {
        return (idx >= 0 && idx < kOverStatRows) ? _overStatRow[static_cast<size_t>(idx)] : nullptr;
    }
    const pix::Text* squadNameText(int idx) const {
        return (idx >= 0 && idx < 4) ? _squad[static_cast<size_t>(idx)].name : nullptr;
    }
    const pix::Text* squadStatusText(int idx) const {
        return (idx >= 0 && idx < 4) ? _squad[static_cast<size_t>(idx)].status : nullptr;
    }
    const pix::Graphics* squadColorDot(int idx) const {
        return (idx >= 0 && idx < 4) ? _squad[static_cast<size_t>(idx)].colorDot : nullptr;
    }
    const pix::Graphics* squadHealthFill(int idx) const {
        return (idx >= 0 && idx < 4) ? _squad[static_cast<size_t>(idx)].barFill : nullptr;
    }
    const pix::Sprite* gearIcon(int idx) const {
        return (idx >= 0 && idx < kGearIconCount) ? _gearIcons[static_cast<size_t>(idx)] : nullptr;
    }
    // ui2.ts gear slot metadata (tap-to-drop + level badge) for GameScene/tests.
    const std::string& gearType(int idx) const;
    bool gearDroppable(int idx) const;
    const pix::Text* gearLevelText(int idx) const {
        return (idx >= 0 && idx < kGearIconCount) ? _gearLevelText[static_cast<size_t>(idx)] : nullptr;
    }
    const pix::Text* reloadButtonText() const { return _reloadText; }
    const pix::Text* interactButtonText() const { return _interactText; }
    const pix::Text* healButtonText() const { return _healText; }
    const pix::Text* boostButtonText() const { return _boostText; }
    const pix::Text* scopeButtonText() const { return _scopeText; }
    const pix::Text* swapButtonText() const { return _swapText; }
    const pix::Text* weaponSlotText(int idx) const {
        return (idx >= 0 && idx < 4) ? _weaponSlots[idx] : nullptr;
    }
    const pix::Text* spectateLabelText() const { return _specLabel; }
    const pix::Text* spectateCountText() const { return _specCountText; }
    const pix::Text* spectateStatsText() const { return _specStatsText; }
    const pix::Text* deathTitleText() const { return _overTitle; }
    const pix::Text* interactionPromptText() const { return _promptText; }
    const pix::Text* announcementText() const { return _announcementText; }
    const pix::Text* aliveText() const { return _aliveText; }
    const pix::Text* waitingText() const { return _waitingText; }
    const pix::Text* pickupMessageText() const { return _pickupText; }
    const pix::Text* roleNameText() const { return _roleName; }
    const pix::Text* roleFooterText() const { return _roleFooter; }
    const pix::Text* roleConfirmText() const { return _roleFooter; }
    const pix::Text* rolePerkNameText(int idx) const {
        return (idx >= 0 && idx < kRolePerkMax) ? _rolePerkName[static_cast<size_t>(idx)] : nullptr;
    }
    const pix::Text* killFeedRowText(int idx) const {
        return (idx >= 0 && idx < 6) ? _killFeedRows[static_cast<size_t>(idx)] : nullptr;
    }

private:
    void layout(pix::Graphics* g, float w, float h) const;

    struct Rect {
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;
        bool contains(float px, float py) const {
            return px >= x && px <= x + w && py >= y && py <= y + h;
        }
    };

    // Repaints the P0.1 action controls from the state update() sampled.
    void updateActionButtons();
    // ui.ts setRoleMenuInfo: repaints the class picker for one option.
    void selectRole(int index);

    pix::Factory* _factory = nullptr;
    pix::Container* _root = nullptr;

    pix::Graphics* _healthBg = nullptr;
    pix::Graphics* _healthFill = nullptr;
    pix::Text* _healthText = nullptr;
    pix::Graphics* _boostSegments[4] = {nullptr, nullptr, nullptr, nullptr};
    pix::Text* _ammoText = nullptr;
    pix::Text* _lootCountsText = nullptr;

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
    bool _minimapHidden = false;
    pix::Text* _aliveText = nullptr;
    pix::Text* _killsText = nullptr;
    pix::Text* _killLeaderText = nullptr;
    std::string _lastKillLeader;

    // Gas countdown (ui.ts #ui-gas-timer) and the result screen.
    pix::Text* _gasText = nullptr;
    // #ui-gas-icon: gas.svg normally, danger.svg while the gas is moving.
    pix::Sprite* _gasIcon = nullptr;
    GasHudState _gas;
    std::string _lastGasText;
    float _gasPulseTicker = 0.0f;

    pix::Graphics* _overBackdrop = nullptr;
    pix::Graphics* _overPanel = nullptr;
    pix::Text* _overTitle = nullptr;
    pix::Text* _overRank = nullptr;
    pix::Text* _overStats = nullptr;
    // ui.ts showStats player cards, reduced to one text row per teammate.
    static constexpr int kOverStatRows = 4;
    std::array<pix::Text*, kOverStatRows> _overStatRow{};
    std::array<bool, kOverStatRows> _overStatRowHas{};
    pix::Graphics* _overSpectateBg = nullptr;
    pix::Text* _overSpectateText = nullptr;
    pix::Graphics* _overLeaveBg = nullptr;
    pix::Text* _overLeaveText = nullptr;
    Rect _overSpectateRect;
    Rect _overLeaveRect;
    bool _overShown = false;
    bool _deathShown = false;
    bool _spectating = false;
    PlayerStatsMsg _localStats;
    bool _hasLocalStats = false;

    // Spectating overlay (ui.ts setSpectating / #ui-spectate-options).
    pix::Text* _specLabel = nullptr;
    pix::Text* _specCountText = nullptr;
    pix::Graphics* _specPrevBg = nullptr;
    pix::Text* _specPrevText = nullptr;
    pix::Graphics* _specNextBg = nullptr;
    pix::Text* _specNextText = nullptr;
    pix::Graphics* _specStatsBg = nullptr;
    pix::Text* _specStatsTextBtn = nullptr;
    pix::Graphics* _specLeaveBg = nullptr;
    pix::Text* _specLeaveText = nullptr;
    Rect _specPrevRect;
    Rect _specNextRect;
    Rect _specStatsRect;
    Rect _specLeaveRect;
    pix::Graphics* _specStatsPanel = nullptr;
    pix::Text* _specStatsText = nullptr;
    bool _specStatsShown = false;
    bool _specCanCycle = false;
    uint8_t _specCount = 0;
    std::string _specName;

    // Squad bars (top-left, team modes).
    struct SquadRow {
        pix::Text* name = nullptr;
        pix::Graphics* barBg = nullptr;
        pix::Graphics* barFill = nullptr;
        pix::Text* status = nullptr;
        pix::Graphics* colorDot = nullptr;
        std::string key;
    };
    std::array<SquadRow, 4> _squad{};
    std::vector<SquadMember> _squadState;
    float _squadPulseTicker = 0.0f;

    std::array<pix::Sprite*, kGearIconCount> _gearIcons{};
    std::array<std::string, kGearIconCount> _gearFrames{};
    std::array<pix::Text*, kGearIconCount> _gearLevelText{};
    std::array<GearSlot, kGearIconCount> _gearSlots{};
    std::array<Rect, kGearIconCount> _gearRects{};

    pix::Container* _weaponBox = nullptr;
    pix::Graphics* _weaponBoxBg = nullptr;
    pix::Text* _weaponSlots[4] = {nullptr, nullptr, nullptr, nullptr};

    // P0.1 action controls.
    pix::Graphics* _reloadBg = nullptr;
    pix::Text* _reloadText = nullptr;
    pix::Graphics* _interactBg = nullptr;
    pix::Text* _interactText = nullptr;
    pix::Graphics* _healBg = nullptr;
    pix::Text* _healText = nullptr;
    pix::Graphics* _boostBg = nullptr;
    pix::Text* _boostText = nullptr;
    pix::Graphics* _scopeBg = nullptr;
    pix::Text* _scopeText = nullptr;
    pix::Graphics* _swapBg = nullptr;
    pix::Text* _swapText = nullptr;
    pix::Graphics* _weaponSlotBg[4] = {nullptr, nullptr, nullptr, nullptr};
    Rect _reloadRect;
    Rect _interactRect;
    Rect _promptRect;
    Rect _healRect;
    Rect _boostRect;
    Rect _scopeRect;
    Rect _swapRect;
    Rect _weaponRect[4];
    std::array<std::string, 4> _weaponTypes{};
    std::string _healItem;
    std::string _boostItem;
    std::string _scopeType;
    int _healCount = 0;
    int _boostCount = 0;
    int _equippedSlot = -1;
    std::string _actionKey;

    pix::Text* _pickupText = nullptr;
    pix::Text* _killFeedText = nullptr;
    std::array<pix::Text*, 6> _killFeedRows{};
    pix::Text* _promptText = nullptr;
    pix::Text* _announcementText = nullptr;
    pix::Text* _waitingText = nullptr;
    bool _waiting = false;

    // Perk-mode class picker (ui.ts #ui-role-menu-wrapper / #ui-role-menu).
    pix::Container* _rolePanel = nullptr;
    pix::Graphics* _roleBackdrop = nullptr;
    pix::Graphics* _rolePanelBg = nullptr;
    pix::Graphics* _roleHeaderBg = nullptr;
    pix::Graphics* _roleFooterBg = nullptr;
    pix::Graphics* _roleConfirmBg = nullptr;
    pix::Text* _roleName = nullptr;
    pix::Text* _roleDesc = nullptr;
    pix::Text* _roleFooter = nullptr;
    pix::Sprite* _roleImage = nullptr;
    pix::Sprite* _roleOptionSprites[kRoleOptionMax] = {};
    std::array<pix::Sprite*, kRolePerkMax> _rolePerkRing{};
    std::array<pix::Sprite*, kRolePerkMax> _rolePerkIcon{};
    std::array<pix::Text*, kRolePerkMax> _rolePerkName{};
    std::vector<std::string> _roleOptionsState;
    std::string _roleDisplayed;
    std::string _roleSelectedPending;
    bool _roleMenuActive = false;
    float _roleTicker = 0.0f;
    float _roleScale = 1.0f;
    // Hit rects in design space (the panel itself is scaled by _roleScale).
    std::array<Rect, kRoleOptionMax> _roleOptionRects{};
    Rect _roleConfirmRect;
    std::string _lastRoleFooter;
    float _announcementTicker = 0.0f;
    bool _factionMode = false;
    std::array<pix::Graphics*, 3> _teamArrows{};
    std::vector<TeammateArrow> _teamArrowState;

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
    std::string _lastAliveText;
    std::string _lastPrompt;
    std::string _lastAnnouncement;
    std::vector<std::string> _lastPerkTypes;
    bool _perksInitialized = false;
    int _lastLocalKills = -1;
    float _pickupTicker = 0.0f;
};

} // namespace surv
