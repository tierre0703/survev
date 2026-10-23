// Host tests for the team-ping wheel (game/PingWheel) and the ping defs emitted
// by tools/codegen_render_defs.mjs (shared/defs/gameObjects/pingDefs.ts).
#include "TestFramework.h"

#include "game/PingWheel.h"
#include "game/GameWorld.h"
#include "game/EmoteManager.h"
#include "net/Messages.h"
#include "render/Defs.h"
#include "render/GeneratedDefs.h"
#include "render/NullPixi.h"

#include <string>

using namespace surv;
using namespace surv_test;

namespace {

const Vec2 kDesign(1280.0f, 720.0f);

struct PingFixture {
    pix::NullPixiFactory factory;
    pix::Container* root = nullptr;
    PingWheel wheel;

    PingFixture() {
        installGeneratedDefs();
        root = factory.createContainer();
        wheel.build(&factory, root, kDesign);
        wheel.setHudVisible(true);
    }
};

} // namespace

TEST(ping_defs_provider) {
    installGeneratedDefs();
    const DefProvider* provider = getDefProvider();
    CHECK(provider != nullptr);

    const PingRenderDef* danger = provider->ping("ping_danger");
    CHECK(danger != nullptr);
    CHECK_EQ(danger->texture, std::string("ping-team-danger.img"));
    CHECK_EQ(danger->mapTexture, std::string("ping-map-danger.img"));
    CHECK_EQ(danger->sound, std::string("ping_danger_01"));
    CHECK_EQ(danger->soundLeader, std::string("ping_leader_01"));
    CHECK(danger->pingMap);
    CHECK(!danger->mapEvent);
    CHECK(danger->worldDisplay);
    CHECK_NEAR(danger->pingLife, 4.0f, 0.001f);

    // Map events carry a tint and have no world sprite.
    const PingRenderDef* airdrop = provider->ping("ping_airdrop");
    CHECK(airdrop != nullptr);
    CHECK(airdrop->mapEvent);
    CHECK(!airdrop->worldDisplay);
    CHECK_EQ(airdrop->tint, 0xff6600u);
    CHECK_NEAR(airdrop->mapLife, 10.0f, 0.001f);
    CHECK(airdrop->soundLeader.empty());

    CHECK(provider->ping("ping_does_not_exist") == nullptr);
}

TEST(ping_wheel_selector_sectors) {
    // Inside the dead zone the centre/close selector wins.
    CHECK_EQ(PingWheel::selectorAt(0.0f, 0.0f), PingWheel::Middle);
    CHECK_EQ(PingWheel::selectorAt(20.0f, -20.0f), PingWheel::Middle);

    // Major quarters: top, right, bottom.
    CHECK_EQ(PingWheel::selectorAt(0.0f, 80.0f), PingWheel::Top);
    CHECK_EQ(PingWheel::selectorAt(80.0f, 0.0f), PingWheel::Right);
    CHECK_EQ(PingWheel::selectorAt(0.0f, -80.0f), PingWheel::Bottom);

    // Eighth slices on the left half: bottom-left 180..225, top-left 135..180.
    CHECK_EQ(PingWheel::selectorAt(-80.0f, -25.0f), PingWheel::BottomLeft);
    CHECK_EQ(PingWheel::selectorAt(-80.0f, 25.0f), PingWheel::TopLeft);

    // isAngleBetween is the emote.ts implementation (wrapping sectors).
    CHECK(PingWheel::isAngleBetween(90.0f, 45.0f, 135.0f));
    CHECK(!PingWheel::isAngleBetween(200.0f, 45.0f, 135.0f));
    CHECK(PingWheel::isAngleBetween(0.0f, 315.0f, 45.0f));
    CHECK(PingWheel::isAngleBetween(350.0f, 315.0f, 45.0f));
    CHECK(PingWheel::isAngleBetween(210.0f, 225.0f, 180.0f));
}

TEST(ping_wheel_button_and_ammo_mapping) {
    PingFixture fx;
    // The ping button is one gap left of the emote button.
    CHECK_NEAR(fx.wheel.buttonCenter().x, kDesign.x - (132.0f + 16.0f) - PingWheel::kButtonGap, 0.01f);
    CHECK_NEAR(fx.wheel.buttonCenter().y, kDesign.y * 0.5f, 0.01f);
    CHECK(fx.wheel.buttonHit(fx.wheel.buttonCenter().x, fx.wheel.buttonCenter().y));
    CHECK(!fx.wheel.buttonHit(fx.wheel.buttonCenter().x + 200.0f, fx.wheel.buttonCenter().y));

    // emote.ts AmmoTypeToEmote.
    CHECK_EQ(PingWheel::ammoEmoteForAmmo("9mm"), std::string("emote_ammo9mm"));
    CHECK_EQ(PingWheel::ammoEmoteForAmmo("12gauge"), std::string("emote_ammo12gauge"));
    CHECK_EQ(PingWheel::ammoEmoteForAmmo("762mm"), std::string("emote_ammo762mm"));
    CHECK_EQ(PingWheel::ammoEmoteForAmmo("556mm"), std::string("emote_ammo556mm"));
    CHECK_EQ(PingWheel::ammoEmoteForAmmo("50AE"), std::string("emote_ammo50ae"));
    CHECK_EQ(PingWheel::ammoEmoteForAmmo("308sub"), std::string("emote_ammo308sub"));
    CHECK_EQ(PingWheel::ammoEmoteForAmmo("flare"), std::string("emote_ammoflare"));
    CHECK_EQ(PingWheel::ammoEmoteForAmmo("45acp"), std::string("emote_ammo45acp"));
    // Unknown/melee -> generic ammo emote.
    CHECK_EQ(PingWheel::ammoEmoteForAmmo(""), std::string("emote_ammo"));
    CHECK_EQ(PingWheel::ammoEmoteForAmmo("fists"), std::string("emote_ammo"));

    fx.wheel.setAmmoType("9mm");
    CHECK_EQ(fx.wheel.ammoEmote(), std::string("emote_ammo9mm"));

    // Hidden HUD never fires the button.
    fx.wheel.setHudVisible(false);
    CHECK(!fx.wheel.buttonHit(fx.wheel.buttonCenter().x, fx.wheel.buttonCenter().y));
}

TEST(ping_wheel_selects_pings_and_team_emotes) {
    PingFixture fx;
    const Vec2 c = fx.wheel.buttonCenter();
    fx.wheel.setAmmoType("556mm");
    fx.wheel.open(c);
    CHECK(fx.wheel.visible());

    PingWheel::Selection sel;
    // Centre is the close selector: no payload.
    CHECK(!fx.wheel.select(Vec2(c.x + 10.0f, c.y + 10.0f), sel));
    CHECK(!sel.valid());

    CHECK(fx.wheel.select(Vec2(c.x, c.y + 100.0f), sel));
    CHECK_EQ(sel.ping, std::string("ping_danger"));
    CHECK(sel.emote.empty());

    sel = {};
    CHECK(fx.wheel.select(Vec2(c.x + 100.0f, c.y), sel));
    CHECK_EQ(sel.ping, std::string("ping_coming"));

    sel = {};
    CHECK(fx.wheel.select(Vec2(c.x, c.y - 100.0f), sel));
    CHECK_EQ(sel.ping, std::string("ping_help"));

    sel = {};
    CHECK(fx.wheel.select(Vec2(c.x - 100.0f, c.y - 40.0f), sel));
    CHECK(sel.ping.empty());
    CHECK_EQ(sel.emote, std::string("emote_medical"));
    CHECK(!sel.ammoEmote);

    sel = {};
    CHECK(fx.wheel.select(Vec2(c.x - 100.0f, c.y + 40.0f), sel));
    CHECK(sel.ping.empty());
    CHECK_EQ(sel.emote, std::string("emote_ammo556mm"));
    CHECK(sel.ammoEmote);

    fx.wheel.close();
    CHECK(!fx.wheel.visible());
}

TEST(ping_wheel_solo_gating_and_throttle_grey) {
    PingFixture fx;
    const Vec2 c = fx.wheel.buttonCenter();
    fx.wheel.setTeamMode(false); // team: medical/ammo emotes allowed
    fx.wheel.open(c);
    PingWheel::Selection sel;
    CHECK(fx.wheel.select(Vec2(c.x - 100.0f, c.y - 40.0f), sel));
    CHECK_EQ(sel.emote, std::string("emote_medical"));

    // Solo greys out the team-only emote selectors, but pings still work.
    fx.wheel.setTeamMode(true);
    fx.wheel.open(c);
    sel = {};
    CHECK(!fx.wheel.select(Vec2(c.x - 100.0f, c.y - 40.0f), sel));
    CHECK(!sel.valid());
    CHECK(fx.wheel.select(Vec2(c.x, c.y + 100.0f), sel));
    CHECK_EQ(sel.ping, std::string("ping_danger"));

    // The shared emote hard cooldown greys the whole wheel.
    fx.wheel.setGreyed(true);
    fx.wheel.open(c);
    sel = {};
    CHECK(!fx.wheel.select(Vec2(c.x, c.y + 100.0f), sel));
    CHECK(!sel.valid());
    fx.wheel.setGreyed(false);
    CHECK(fx.wheel.select(Vec2(c.x, c.y + 100.0f), sel));
    CHECK_EQ(sel.ping, std::string("ping_danger"));
}

TEST(ping_manager_resets_indicator_alpha_on_reuse) {
    // A reused indicator slot must not inherit the previous ping's fade-out
    // alpha (device bug: the second ping on a slot was nearly invisible).
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    world.setScreenSize(960.0f, 540.0f);

    Emote ping;
    ping.isPing = true;
    ping.type = "ping_airdrop";
    ping.hasPos = true;
    ping.pos = Vec2(100.0f, 100.0f);

    EmoteManager& manager = world.emoteManager();
    manager.addPing(ping, false, world);
    const PingIndicator* ind = manager.indicator(4); // airdrop slot
    CHECK(ind != nullptr);
    CHECK(ind->pingContainer != nullptr);
    CHECK_NEAR(ind->pingContainer->getAlpha(), 1.0f, 0.001f);
    CHECK_EQ(ind->playerId, ping.playerId);

    // Let it fade all the way out (fade in 0.5 + life 4.25 + fade out 0.1).
    for (int i = 0; i < 600; ++i) {
        manager.update(1.0f / 60.0f, world);
    }
    CHECK(!ind->displayed);
    CHECK(ind->pingContainer->getAlpha() < 0.05f);

    // The next ping on the same slot starts fully opaque again.
    manager.addPing(ping, false, world);
    CHECK_NEAR(ind->pingContainer->getAlpha(), 1.0f, 0.001f);
    CHECK_NEAR(ind->indContainer->getAlpha(), 1.0f, 0.001f);
    CHECK_NEAR(ind->pingSprite->getAlpha(), 1.0f, 0.001f);
}
