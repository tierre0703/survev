#pragma once
// Team-ping wheel for the touch client (client/src/emote.ts team-ping path).
//
// The web client has no dedicated touch button for the team ping wheel: the
// wheel opens over the crosshair when the team-ping bind is held, and the big
// map can move its world position. On the native touch client it gets its own
// on-screen button next to the emote button; the ping is sent at the crosshair
// (the camera centre, i.e. the active player), which is the web's behaviour for
// `TeamPingSingle` presses without a big-map position.
//
// The six selectors mirror emote.ts `teamPingData` exactly:
//   middle      -> close
//   top         -> ping_danger
//   right       -> ping_coming
//   bottom      -> ping_help
//   bottom-left -> emote_medical
//   top-left    -> emote_ammo* (ammo type of the equipped weapon)
//
// Drawn with pix adapter nodes in HUD design space (1280x720, y up) so it can
// be driven headlessly by surv_tests (NullPixi), like game/EmoteWheel.
#include "../core/Vec2.h"
#include "../render/PixiLike.h"

#include <array>
#include <string>

namespace surv {

class PingWheel {
public:
    // Wheel geometry (design px). The web wheel is a DOM element scaled by
    // device scale; these values keep the six selectors finger-sized.
    static constexpr float kOuterRadius = 132.0f;
    static constexpr float kMajorDistance = 82.0f;
    static constexpr float kMajorRadius = 42.0f;
    static constexpr float kMinorRadius = 32.0f;
    static constexpr float kCloseRadius = 30.0f;
    // emote.ts `distMinLength`: inside this radius the centre selector wins.
    static constexpr float kDeadZone = 35.0f;
    static constexpr float kOpenDuration = 0.15f;

    // On-screen team-ping button, placed one gap left of the emote button so
    // the wheel it opens stays fully on screen.
    static constexpr float kButtonRadius = 22.0f;
    static constexpr float kButtonGap = 56.0f;

    // emote.ts teamPingSelectors order.
    enum Selector {
        Middle = 0,
        Top,
        Right,
        Bottom,
        BottomLeft,
        TopLeft,
        kSelectorCount,
    };

    struct Selection {
        std::string ping;  // ping type (`ping_danger`, ...) when a ping wedge
        std::string emote; // emote type (`emote_medical`, ...) when an emote wedge
        bool ammoEmote = false;
        bool valid() const { return !ping.empty() || !emote.empty(); }
    };

    void build(pix::Factory* factory, pix::Container* root, const Vec2& design);

    // Re-applies the selector/button sprite frames after the map atlases load
    // (the wheel is built in resetWorld(), before the first MapMsg).
    void refreshFrames();

    // TeamMode::Solo (1) greys out the team-only emote selectors (medical and
    // ammo), like emote.ts's `disableInSolo`.
    void setTeamMode(bool solo);
    // Show/hide the button with the rest of the HUD; hides the wheel too.
    void setHudVisible(bool visible);
    // Shared emote throttle: the web greys both wheels during the hard
    // cooldown (EmoteBarn.emoteWheelsGreyed).
    void setGreyed(bool greyed);

    // Equipped weapon ammo type -> ammo-specific emote (emote.ts
    // `AmmoTypeToEmote`). Safe to call every frame; only refreshes on change.
    void setAmmoType(const std::string& ammoType);
    const std::string& ammoEmote() const { return _ammoEmote; }
    static std::string ammoEmoteForAmmo(const std::string& ammoType);

    // The wheel opens around `center` and animates open.
    void open(const Vec2& center);
    void close();
    bool visible() const { return _active; }

    // Highlights the selector under `pos` (design coords) and writes the send
    // payload when it is usable. emote.ts sends as soon as a wedge highlights.
    bool select(const Vec2& pos, Selection& out);

    void update(float dt);

    bool buttonHit(float designX, float designY) const;
    const Vec2& buttonCenter() const { return _buttonCenter; }
    int highlighted() const { return _highlighted; }

    // Selector under a drag offset from the wheel centre (0 = middle/close,
    // 1..5 = teamPingSelectors). Returns -1 only if no sector matches.
    static int selectorAt(float dx, float dy);
    // Port of emote.ts isAngleBetween (degrees, 0..360).
    static bool isAngleBetween(float target, float angle1, float angle2);

    // Fixed ping type of a selector (empty for the emote/close selectors).
    static const char* pingType(int selector);
    // Test/debug access: the on-screen button icon sprite.
    pix::Sprite* buttonIcon() const { return _buttonIcon; }

private:
    struct Item {
        pix::Graphics* bg = nullptr;
        pix::Graphics* highlight = nullptr;
        pix::Sprite* icon = nullptr;
        pix::Text* label = nullptr;
        bool teamOnly = false;
    };

    void layout();
    void refreshIcons();
    void refreshAmmoIcon();
    void applyHighlight();
    void updateGrey();
    std::string emoteType(int selector) const;
    bool itemUsable(int selector) const;

    pix::Factory* _factory = nullptr;
    pix::Container* _root = nullptr;
    Vec2 _design = Vec2(1280.0f, 720.0f);

    pix::Container* _button = nullptr;
    pix::Sprite* _buttonIcon = nullptr;
    Vec2 _buttonCenter;

    pix::Container* _wheel = nullptr;
    std::array<Item, kSelectorCount> _items{};
    pix::Text* _closeText = nullptr;

    bool _solo = false;
    bool _greyed = false;
    bool _hudVisible = false;
    bool _active = false;
    bool _built = false;
    Vec2 _center;
    int _highlighted = -1;
    float _openT = 0.0f;
    float _idle = 0.0f;
    std::string _ammoType;
    std::string _ammoEmote = "emote_ammo";
};

} // namespace surv
