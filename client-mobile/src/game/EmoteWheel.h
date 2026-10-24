#pragma once
// Emote wheel for the touch client (client/src/emote.ts emote-wheel path).
//
// The web client shows an on-screen emote button (#ui-emote-button); pressing
// it opens a four-wedge wheel with the player's emote loadout and sends the
// emote as soon as a wedge is highlighted while dragging. Emote sends are
// throttled by GameConfig.player.emoteSoftCooldown / emoteHardCooldown /
// emoteThreshold, and the wheel greys out during the hard cooldown.
//
// This port keeps the same behaviour but draws the wheel with pix adapter nodes
// (design space, 1280x720) so it lives beside the rest of the in-game HUD and
// can be driven headlessly by surv_tests (NullPixi).
//
// Coordinates: design-space points from the HUD bottom-left, y up (the HUD
// container is centred/scaled by GameScene, see game/Hud.h).
#include "../core/GameConfig.h"
#include "../core/Vec2.h"
#include "../render/PixiLike.h"

#include <array>
#include <string>
#include <vector>

namespace surv {

class EmoteWheel {
public:
    // Wheel geometry (design px). Mirrors the web 256px wheel scaled down so
    // the four wedges fit around the screen centre on a phone.
    static constexpr float kOuterRadius = 132.0f;
    static constexpr float kWedgeDistance = 78.0f;
    static constexpr float kWedgeRadius = 46.0f;
    static constexpr float kCloseRadius = 30.0f;
    // emote.ts `distMinLength`: inside this radius the wheel counts as "close".
    static constexpr float kDeadZone = 35.0f;
    static constexpr float kOpenDuration = 0.15f;

    // On-screen emote button (#ui-emote-button). Placed one wheel radius in
    // from the right edge so the wheel it opens stays fully on screen; the
    // vertical centre sits between the minimap (top right) and the weapon
    // cards (bottom right).
    static constexpr float kButtonRadius = 22.0f;
    static constexpr float kButtonRight = kOuterRadius + 16.0f; // from the right design edge

    void build(pix::Factory* factory, pix::Container* root, const Vec2& design);

    // Re-applies the icon sprite frames. The wheel is built in resetWorld()
    // before the map's atlases are registered, so the first setFrame() calls
    // miss; GameScene calls this after loadAtlasesForMap().
    void refreshFrames();

    // Emote loadout (EmoteSlot Top/Right/Bottom/Left, i.e. JoinedMsg.emotes).
    // Only refreshes the wedges when the loadout actually changed, so it is
    // safe to call every frame.
    void setLoadout(const std::vector<std::string>& emotes);
    // TeamMode::Solo (1) greys out teamOnly emotes (emote.ts teamEmotesGreyed).
    void setTeamMode(bool solo);

    // Show/hide the button with the rest of the HUD; hides the wheel too.
    void setHudVisible(bool visible);

    // Trigger (design coords): the wheel opens around `center` (the screen
    // centre, like emote.ts `emoteScreenPos`).
    void open(const Vec2& center);
    void close();
    bool visible() const { return _active; }

    // True once a wedge is highlighted; writes the emote type to `outType`.
    // emote.ts sends on highlight while the finger is still down.
    bool select(const Vec2& pos, std::string& outType);

    // Per-frame animation + throttle tickers.
    void update(float dt);

    // Emote throttle (emote.ts incrementEmote/m_update).
    void incrementEmote();
    bool canSend() const { return _hardTicker <= 0.0f; }
    float hardTicker() const { return _hardTicker; }
    int emoteCounter() const { return _counter; }
    void resetThrottle();

    // Emote button hit test (design coords).
    bool buttonHit(float designX, float designY) const;
    const Vec2& buttonCenter() const { return _buttonCenter; }

    // Wedge for a drag offset from the wheel centre: 0 top, 1 right, 2 bottom,
    // 3 left, 4 centre/close. `< 0` only when the wheel is closed.
    static int wedgeAt(float dx, float dy);

    const std::array<std::string, 4>& loadout() const { return _loadout; }
    int highlighted() const { return _highlighted; }
    // Test/debug access: the on-screen button icon sprite.
    pix::Sprite* buttonIcon() const { return _buttonIcon; }

private:
    struct Wedge {
        pix::Graphics* bg = nullptr;
        pix::Graphics* highlight = nullptr;
        pix::Sprite* icon = nullptr;
        pix::Text* label = nullptr;
        bool teamOnly = false;
    };

    void layout();
    void refreshWedges();
    void applyHighlight();
    void updateThrottle(float dt);

    pix::Factory* _factory = nullptr;
    pix::Container* _root = nullptr;
    Vec2 _design = Vec2(1280.0f, 720.0f);

    pix::Container* _button = nullptr;
    pix::Graphics* _buttonBg = nullptr;
    pix::Sprite* _buttonIcon = nullptr;
    Vec2 _buttonCenter;

    pix::Container* _wheel = nullptr;
    pix::Graphics* _backdrop = nullptr;
    std::array<Wedge, 4> _wedges{};
    pix::Graphics* _closeBg = nullptr;
    pix::Text* _closeText = nullptr;

    std::array<std::string, 4> _loadout{};
    bool _solo = false;
    bool _built = false;
    bool _hudVisible = false;
    bool _active = false;
    Vec2 _center;
    int _highlighted = -1;
    float _openT = 0.0f;

    float _softTicker = 0.0f;
    float _hardTicker = 0.0f;
    int _counter = 0;
};

} // namespace surv
