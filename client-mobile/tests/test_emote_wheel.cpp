// Host tests for the emote wheel (game/EmoteWheel) and the emote defs emitted
// by tools/codegen_render_defs.mjs (shared/defs/gameObjects/emoteDefs.ts).
#include "TestFramework.h"

#include "core/GameConfig.h"
#include "game/EmoteWheel.h"
#include "render/Defs.h"
#include "render/GeneratedDefs.h"
#include "render/NullPixi.h"

#include <string>
#include <vector>

using namespace surv;
using namespace surv_test;

namespace {

const Vec2 kDesign(1280.0f, 720.0f);

struct WheelFixture {
    pix::NullPixiFactory factory;
    pix::Container* root = nullptr;
    EmoteWheel wheel;

    WheelFixture() {
        installGeneratedDefs();
        root = factory.createContainer();
        wheel.build(&factory, root, kDesign);
        wheel.setLoadout({ "emote_happyface", "emote_thumbsup", "emote_surviv", "emote_sadface" });
        wheel.setHudVisible(true);
    }
};

} // namespace

TEST(emote_defs_provider) {
    installGeneratedDefs();
    const DefProvider* provider = getDefProvider();
    CHECK(provider != nullptr);

    const EmoteRenderDef* happy = provider->emote("emote_happyface");
    CHECK(happy != nullptr);
    CHECK_EQ(happy->texture, std::string("face-happy.img"));
    CHECK_EQ(happy->sound, std::string("emote_01"));
    CHECK_EQ(happy->channel, std::string("ui"));
    CHECK_EQ(happy->name, std::string("Happy Face"));
    CHECK(!happy->teamOnly);

    // emote_medical is team-only in emoteDefs.ts.
    const EmoteRenderDef* medical = provider->emote("emote_medical");
    CHECK(medical != nullptr);
    CHECK(medical->teamOnly);

    CHECK(provider->emote("emote_does_not_exist") == nullptr);
}

TEST(emote_wheel_wedge_geometry) {
    // Inside the dead zone is the centre/close selector.
    CHECK_EQ(EmoteWheel::wedgeAt(0.0f, 0.0f), 4);
    CHECK_EQ(EmoteWheel::wedgeAt(10.0f, -10.0f), 4);
    CHECK_EQ(EmoteWheel::wedgeAt(30.0f, 0.0f), 4);

    // Outside it, the dominant axis picks the EmoteSlot wedge (y up).
    CHECK_EQ(EmoteWheel::wedgeAt(0.0f, 80.0f), 0);   // top
    CHECK_EQ(EmoteWheel::wedgeAt(80.0f, 0.0f), 1);   // right
    CHECK_EQ(EmoteWheel::wedgeAt(0.0f, -80.0f), 2);  // bottom
    CHECK_EQ(EmoteWheel::wedgeAt(-80.0f, 0.0f), 3);  // left
    CHECK_EQ(EmoteWheel::wedgeAt(20.0f, 100.0f), 0);
    CHECK_EQ(EmoteWheel::wedgeAt(-100.0f, 20.0f), 3);
}

TEST(emote_wheel_button_and_loadout) {
    WheelFixture fx;
    // The button sits one wheel radius in from the right edge, vertically
    // centred, so the open wheel stays on screen.
    CHECK_NEAR(fx.wheel.buttonCenter().x, kDesign.x - EmoteWheel::kButtonRight, 0.01f);
    CHECK_NEAR(fx.wheel.buttonCenter().y, kDesign.y * 0.5f, 0.01f);
    CHECK(fx.wheel.buttonHit(fx.wheel.buttonCenter().x, fx.wheel.buttonCenter().y));
    CHECK(!fx.wheel.buttonHit(fx.wheel.buttonCenter().x - 200.0f, fx.wheel.buttonCenter().y));

    CHECK_EQ(fx.wheel.loadout()[EmoteSlot_Top], std::string("emote_happyface"));
    CHECK_EQ(fx.wheel.loadout()[EmoteSlot_Right], std::string("emote_thumbsup"));

    // Hidden HUD never fires the button.
    fx.wheel.setHudVisible(false);
    CHECK(!fx.wheel.buttonHit(fx.wheel.buttonCenter().x, fx.wheel.buttonCenter().y));
}

TEST(emote_wheel_selects_and_closes) {
    WheelFixture fx;
    fx.wheel.open(fx.wheel.buttonCenter());
    CHECK(fx.wheel.visible());

    std::string type;
    // A small drag stays in the dead zone: no emote, no send.
    CHECK(!fx.wheel.select(Vec2(fx.wheel.buttonCenter().x + 10.0f, fx.wheel.buttonCenter().y), type));
    CHECK(type.empty());

    // Drag up selects the top EmoteSlot.
    CHECK(fx.wheel.select(Vec2(fx.wheel.buttonCenter().x, fx.wheel.buttonCenter().y + 100.0f), type));
    CHECK_EQ(type, std::string("emote_happyface"));

    fx.wheel.close();
    CHECK(!fx.wheel.visible());
    // Selection only works while the wheel is open.
    CHECK(!fx.wheel.select(Vec2(fx.wheel.buttonCenter().x, fx.wheel.buttonCenter().y + 100.0f), type));
}

TEST(emote_wheel_team_only_wedges_are_disabled_in_solo) {
    WheelFixture fx;
    fx.wheel.setLoadout({ "emote_medical", "emote_thumbsup", "emote_surviv", "emote_sadface" });
    fx.wheel.setTeamMode(false); // team mode: allowed
    fx.wheel.open(fx.wheel.buttonCenter());
    std::string type;
    CHECK(fx.wheel.select(Vec2(fx.wheel.buttonCenter().x, fx.wheel.buttonCenter().y + 120.0f), type));
    CHECK_EQ(type, std::string("emote_medical"));

    // Solo (TeamMode.Solo == 1) greys out teamOnly emotes.
    fx.wheel.setTeamMode(true);
    fx.wheel.open(fx.wheel.buttonCenter());
    type.clear();
    CHECK(!fx.wheel.select(Vec2(fx.wheel.buttonCenter().x, fx.wheel.buttonCenter().y + 120.0f), type));
    CHECK(type.empty());
    // A non-team wedge still works in solo.
    CHECK(fx.wheel.select(Vec2(fx.wheel.buttonCenter().x + 120.0f, fx.wheel.buttonCenter().y), type));
    CHECK_EQ(type, std::string("emote_thumbsup"));
}

TEST(emote_wheel_throttle_matches_web_cooldowns) {
    EmoteWheel wheel;
    CHECK(wheel.canSend());
    CHECK_EQ(wheel.emoteCounter(), 0);

    // The 6th emote starts the hard cooldown (threshold = 6, hard * 1.5 = 9s).
    for (int i = 0; i < 6; ++i) {
        wheel.incrementEmote();
    }
    CHECK_EQ(wheel.emoteCounter(), 6);
    CHECK(!wheel.canSend());
    CHECK_NEAR(wheel.hardTicker(), PlayerConfig::emoteHardCooldown * 1.5f, 0.001f);

    // The hard cooldown drains, then the counter resets to zero.
    wheel.update(PlayerConfig::emoteHardCooldown * 1.5f + 0.01f);
    CHECK(wheel.canSend());
    CHECK_EQ(wheel.emoteCounter(), 0);

    // Under the threshold the counter decays one emote per soft tick (2s * 1.5).
    wheel.incrementEmote();
    wheel.incrementEmote();
    CHECK_EQ(wheel.emoteCounter(), 2);
    wheel.update(PlayerConfig::emoteSoftCooldown * 1.5f + 0.01f);
    CHECK_EQ(wheel.emoteCounter(), 1);
}

TEST(emote_wheel_opens_and_updates) {
    WheelFixture fx;
    CHECK(!fx.wheel.visible());
    fx.wheel.open(fx.wheel.buttonCenter());
    CHECK(fx.wheel.visible());
    CHECK(fx.wheel.canSend());
    // Opening animates the wheel and keeps it visible.
    fx.wheel.update(1.0f / 60.0f);
    CHECK(fx.wheel.visible());
    fx.wheel.close();
    CHECK(!fx.wheel.visible());
}
