// Host tests for the team-ping wheel (game/PingWheel) and the ping defs emitted
// by tools/codegen_render_defs.mjs (shared/defs/gameObjects/pingDefs.ts).
#include "TestFramework.h"

#include "game/PingWheel.h"
#include "game/GameWorld.h"
#include "game/EmoteManager.h"
#include "game/Hud.h"
#include "game/MapView.h"
#include "game/objects/MapIndicator.h"
#include "audio/AudioManager.h"
#include "audio/GeneratedSoundDefs.h"
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

    // refreshFrames() re-resolves the icons after the map atlases load.
    fx.wheel.setHudVisible(true);
    fx.wheel.refreshFrames();
    const auto* icon = static_cast<const pix::NullSprite*>(fx.wheel.buttonIcon());
    CHECK(icon != nullptr);
    CHECK(icon->visible);
    CHECK_EQ(icon->frame, std::string("ping-team-danger.img"));
}

TEST(ping_wheel_tap_selects_pings_and_team_emotes) {
    PingFixture fx;
    // Runtime opens the (1.5x) wheel at the design centre (GameScene).
    const Vec2 c(kDesign.x * 0.5f, kDesign.y * 0.5f);
    fx.wheel.setAmmoType("556mm");
    fx.wheel.open(c);
    CHECK(fx.wheel.visible());

    PingWheel::Selection sel;
    // The centre dead zone is the close selector: tap-to-close, never a
    // payload (GameScene closes on it).
    CHECK_EQ(fx.wheel.tap(Vec2(c.x + 10.0f, c.y + 10.0f), sel), PingWheel::TapResult::Close);
    CHECK(!sel.valid());

    // The scaled ring spans 198px: hits between the original 132px radius and
    // the scaled one must still select.
    sel = {};
    CHECK_EQ(fx.wheel.tap(Vec2(c.x, c.y + 150.0f), sel), PingWheel::TapResult::Send);
    CHECK_EQ(sel.ping, std::string("ping_danger"));
    CHECK(sel.emote.empty());

    sel = {};
    CHECK_EQ(fx.wheel.tap(Vec2(c.x + 150.0f, c.y), sel), PingWheel::TapResult::Send);
    CHECK_EQ(sel.ping, std::string("ping_coming"));

    sel = {};
    CHECK_EQ(fx.wheel.tap(Vec2(c.x, c.y - 100.0f), sel), PingWheel::TapResult::Send);
    CHECK_EQ(sel.ping, std::string("ping_help"));

    sel = {};
    CHECK_EQ(fx.wheel.tap(Vec2(c.x - 100.0f, c.y - 40.0f), sel), PingWheel::TapResult::Send);
    CHECK(sel.ping.empty());
    CHECK_EQ(sel.emote, std::string("emote_medical"));
    CHECK(!sel.ammoEmote);

    sel = {};
    CHECK_EQ(fx.wheel.tap(Vec2(c.x - 100.0f, c.y + 40.0f), sel), PingWheel::TapResult::Send);
    CHECK(sel.ping.empty());
    CHECK_EQ(sel.emote, std::string("emote_ammo556mm"));
    CHECK(sel.ammoEmote);

    // Outside the scaled ring: dismiss gesture, no selection.
    sel = {};
    CHECK_EQ(fx.wheel.tap(Vec2(c.x, c.y + 250.0f), sel), PingWheel::TapResult::None);
    CHECK(!sel.valid());

    fx.wheel.close();
    CHECK(!fx.wheel.visible());
    // Tapping only works while the wheel is open.
    sel = {};
    CHECK_EQ(fx.wheel.tap(Vec2(c.x, c.y + 100.0f), sel), PingWheel::TapResult::None);
    CHECK(!sel.valid());
}

TEST(ping_wheel_solo_gating_and_throttle_grey) {
    PingFixture fx;
    // Runtime opens the wheel at the design centre.
    const Vec2 c(kDesign.x * 0.5f, kDesign.y * 0.5f);
    fx.wheel.setTeamMode(false); // team: medical/ammo emotes allowed
    fx.wheel.open(c);
    PingWheel::Selection sel;
    CHECK_EQ(fx.wheel.tap(Vec2(c.x - 100.0f, c.y - 40.0f), sel), PingWheel::TapResult::Send);
    CHECK_EQ(sel.emote, std::string("emote_medical"));

    // Solo greys out the team-only emote selectors, but pings still work. An
    // unusable tap leaves the wheel open.
    fx.wheel.setTeamMode(true);
    fx.wheel.open(c);
    sel = {};
    CHECK_EQ(fx.wheel.tap(Vec2(c.x - 100.0f, c.y - 40.0f), sel), PingWheel::TapResult::None);
    CHECK(!sel.valid());
    CHECK(fx.wheel.visible());
    CHECK_EQ(fx.wheel.tap(Vec2(c.x, c.y + 100.0f), sel), PingWheel::TapResult::Send);
    CHECK_EQ(sel.ping, std::string("ping_danger"));

    // The shared emote hard cooldown greys the whole wheel.
    fx.wheel.setGreyed(true);
    fx.wheel.open(c);
    sel = {};
    CHECK_EQ(fx.wheel.tap(Vec2(c.x, c.y + 100.0f), sel), PingWheel::TapResult::None);
    CHECK(!sel.valid());
    fx.wheel.setGreyed(false);
    CHECK_EQ(fx.wheel.tap(Vec2(c.x, c.y + 100.0f), sel), PingWheel::TapResult::Send);
    CHECK_EQ(sel.ping, std::string("ping_danger"));
}

TEST(ping_manager_resets_indicator_alpha_on_reuse) {    // A reused indicator slot must not inherit the previous ping's fade-out
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

namespace {

MapMsg minimapMap() {
    MapMsg msg;
    msg.mapName = "main";
    msg.seed = 123456;
    msg.width = 512.0f;
    msg.height = 512.0f;
    msg.shoreInset = 4.0f;
    msg.grassInset = 2.0f;
    return msg;
}

} // namespace

TEST(role_defs_provider) {
    installGeneratedDefs();
    const DefProvider* provider = getDefProvider();
    CHECK(provider != nullptr);

    const RoleRenderDef* leader = provider->role("leader");
    CHECK(leader != nullptr);
    CHECK_EQ(leader->mapIconAlive, std::string("player-star.img"));
    CHECK_EQ(leader->mapIconDead, std::string("skull-leader.img"));
    CHECK_EQ(leader->soundAssign, std::string("leader_assigned_01"));
    CHECK_EQ(leader->soundDead, std::string("leader_dead_01"));

    const RoleRenderDef* captain = provider->role("captain");
    CHECK(captain != nullptr);
    CHECK_EQ(captain->mapIconAlive, std::string("player-captain.img"));
    CHECK(captain->mapIconDead.empty());
    CHECK_EQ(captain->soundAssign, std::string("captain_assigned_01"));
    CHECK(captain->soundDead.empty());

    // Roles without custom icons still resolve (empty alive means "no icon").
    const RoleRenderDef* marksman = provider->role("marksman");
    CHECK(marksman != nullptr);
    CHECK(marksman->mapIconAlive.empty());

    CHECK(provider->role("not_a_role") == nullptr);
}

TEST(map_dot_style_matches_web) {
    // ui.ts updatePlayerMapSprites texture/scale/outer/z matrix.
    MapDotStyle style = mapDotStyle(true, false, false, "", "");
    CHECK_EQ(style.texture, std::string("player-map-inner.img"));
    CHECK_NEAR(style.scale, 0.2f, 0.001f);
    CHECK(style.outer);
    CHECK_EQ(style.zBump, 0);

    style = mapDotStyle(true, true, false, "", "");
    CHECK_EQ(style.texture, std::string("skull-outlined.img"));
    CHECK_NEAR(style.scale, 0.3f, 0.001f); // dotScale * 1.5
    CHECK(style.outer);

    style = mapDotStyle(true, false, true, "", "");
    CHECK_EQ(style.texture, std::string("player-group-downed.img"));
    CHECK_NEAR(style.scale, 0.2f, 0.001f);

    style = mapDotStyle(true, false, false, "player-star.img", "skull-leader.img");
    CHECK(style.custom);
    CHECK_EQ(style.texture, std::string("player-star.img"));
    CHECK_NEAR(style.scale, 0.25f, 0.001f); // dotScale * 1.25
    CHECK(!style.outer);
    CHECK_EQ(style.zBump, 65535);

    style = mapDotStyle(true, true, false, "player-star.img", "skull-leader.img");
    CHECK_EQ(style.texture, std::string("skull-leader.img"));
    CHECK_NEAR(style.scale, 0.3f, 0.001f);

    style = mapDotStyle(true, true, false, "player-captain.img", "");
    CHECK_EQ(style.texture, std::string("skull-outlined.img"));

    style = mapDotStyle(false, false, false, "", "");
    CHECK_NEAR(style.scale, 0.15f, 0.001f); // dotScale * 0.75
    CHECK(!style.outer);

    style = mapDotStyle(false, false, true, "", "");
    CHECK_EQ(style.texture, std::string("player-map-downed.img"));
    CHECK_NEAR(style.scale, 0.25f, 0.001f);

    style = mapDotStyle(false, false, false, "player-captain.img", "");
    CHECK_EQ(style.texture, std::string("player-captain.img"));
    CHECK_NEAR(style.scale, 0.25f, 0.001f);
    CHECK(!style.outer);
}

TEST(map_minimap_texture_is_baked) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    world.loadMap(minimapMap());

    pix::RenderTexture* tex = world.map().getMinimapTexture();
    CHECK(tex != nullptr);
    CHECK_NEAR(tex->width(), 1024.0f, 0.001f);
    CHECK_NEAR(tex->height(), 1024.0f, 0.001f);
    CHECK_NEAR(world.map().getMinimapScale(), 2.0f, 0.001f);
    // The bake must actually reach the adapter renderer.
    CHECK(factory.renderer() != nullptr);
    CHECK(factory.renderer()->renderCount >= 1);
}

TEST(map_pings_spawn_on_minimap_and_expire) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    world.loadMap(minimapMap());
    EmoteManager& manager = world.emoteManager();

    Emote playerPing;
    playerPing.isPing = true;
    playerPing.type = "ping_danger";
    playerPing.hasPos = true;
    playerPing.playerId = 7;
    playerPing.pos = Vec2(100.0f, 100.0f);
    manager.addPing(playerPing, false, world);

    CHECK_EQ(manager.mapPings().size(), size_t(2)); // icon + pulse
    const MapPing& icon = manager.mapPings()[0];
    const MapPing& pulse = manager.mapPings()[1];
    CHECK(!icon.pulse);
    CHECK_EQ(icon.texture, std::string("ping-map-danger.img"));
    CHECK_NEAR(icon.life, 4.0f, 0.001f);
    // Player pings: the web's raw 0.15 Sm createPing scale.
    CHECK_NEAR(icon.scale, 0.15f, 0.001f);
    CHECK_EQ(icon.tint, 0xffffffu);
    CHECK(pulse.pulse);
    CHECK_EQ(pulse.texture, std::string("ping-map-pulse.img"));
    CHECK_NEAR(pulse.life, 4.0f, 0.001f);

    // A newer ping from the same player replaces the older map sprites.
    playerPing.type = "ping_help";
    manager.addPing(playerPing, false, world);
    int active = 0;
    for (const auto& mp : manager.mapPings()) {
        active += mp.active ? 1 : 0;
    }
    CHECK_EQ(active, 2);
    manager.update(0.1f, world);
    CHECK_EQ(manager.mapPings().size(), size_t(2));
    CHECK_EQ(manager.mapPings()[0].texture, std::string("ping-map-help.img"));

    // Map events (airdrop) keep the def tint and stack with player pings.
    Emote airdrop;
    airdrop.isPing = true;
    airdrop.type = "ping_airdrop";
    airdrop.hasPos = true;
    airdrop.playerId = 7;
    airdrop.pos = Vec2(200.0f, 200.0f);
    manager.addPing(airdrop, false, world);
    bool sawAirdrop = false;
    for (const auto& mp : manager.mapPings()) {
        if (!mp.pulse && mp.texture == "ping-map-airdrop.img") {
            sawAirdrop = true;
            CHECK_EQ(mp.tint, 0xff6600u);
            CHECK_NEAR(mp.scale, 0.3f, 0.001f);
            CHECK_NEAR(mp.life, 10.0f, 0.001f);
        }
    }
    CHECK(sawAirdrop);

    // Everything expires (mapLife <= 10s).
    manager.update(11.0f, world);
    CHECK(manager.mapPings().empty());
}

TEST(hud_minimap_positions_ping_sprites) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    pix::Container* root = factory.createContainer();
    Hud hud;
    hud.build(&factory, root, Vec2(1280.0f, 720.0f));
    auto* rt = static_cast<pix::NullRenderTexture*>(factory.createRenderTexture());
    rt->resize(256.0f, 256.0f);
    hud.setMinimap(rt, 512.0f, 512.0f);
    hud.setVisible(true);

    const MapView& mini = hud.minimap();
    const auto* map = static_cast<const pix::NullSprite*>(mini.mapSprite());
    CHECK(map != nullptr);
    CHECK(map->texture == rt);
    CHECK(mini.zoomed());
    // ui.ts: map sprite = window / 0.256 (~3.9x), centred on the player.
    const float display = Hud::kMinimapSize / MapView::kMinimapViewFraction;
    CHECK_NEAR(map->sx, display / 256.0f, 0.001f);
    hud.setMapCenter(Vec2(256.0f, 256.0f));
    CHECK_NEAR(map->x, Hud::kMinimapSize * 0.5f - display * 0.5f, 0.01f);
    CHECK_NEAR(map->y, Hud::kMinimapSize * 0.5f - display * 0.5f, 0.01f);
    CHECK_NEAR(map->getAlpha(), 0.8f, 0.001f);
    const auto* clip = static_cast<const pix::NullContainer*>(mini.root());
    CHECK(clip != nullptr && clip->maskNode != nullptr);
    // Bottom-left of the top-left 140px frame under the settings gear
    // (design 1280x720, centred): left 12, top 50.
    CHECK_NEAR(mini.pos().x, 12.0f - 640.0f, 0.01f);
    CHECK_NEAR(mini.pos().y, (720.0f - 50.0f - Hud::kMinimapSize) - 360.0f, 0.01f);

    std::vector<MapPing> pings(1);
    pings[0].texture = "ping-map-danger.img";
    pings[0].pos = Vec2(256.0f, 256.0f); // map centre
    pings[0].tint = 0xffff00;
    pings[0].scale = 0.2f;
    pings[0].life = 4.0f;
    pings[0].maxLife = 4.0f;
    pings[0].ticker = 1.0f;
    hud.updateMapPings(pings);

    const auto* sprite = static_cast<const pix::NullSprite*>(mini.pingSprite(0));
    CHECK(sprite != nullptr);
    CHECK(sprite->visible);
    CHECK_EQ(sprite->frame, std::string("ping-map-danger.img"));
    CHECK_NEAR(sprite->x, Hud::kMinimapSize * 0.5f, 0.01f);
    CHECK_NEAR(sprite->y, Hud::kMinimapSize * 0.5f, 0.01f);
    // ui.ts createPing: the sprite is fitted to ping.scale * 36 design px
    // (relative to the sprite's own source size).
    const float source = sprite->texture ? sprite->texture->width() : 1.0f;
    CHECK_NEAR(sprite->sx, 0.2f * MapView::kPingDisplayPx / source, 0.001f);
    CHECK_EQ(sprite->getTint(), 0xffff00u);

    // Hidden HUD hides the minimap view (and its pings).
    hud.setVisible(false);
    hud.updateMapPings(pings);
    CHECK(!mini.visible());
}

TEST(map_view_dots_gas_and_pings) {
    pix::NullPixiFactory factory;
    pix::Container* root = factory.createContainer();
    MapView view;
    view.build(&factory, root, Vec2(10.0f, 20.0f), 256.0f);
    auto* rt = static_cast<pix::NullRenderTexture*>(factory.createRenderTexture());
    rt->resize(256.0f, 256.0f);
    view.setTexture(rt);
    view.setMapSize(512.0f, 512.0f);

    std::vector<MapDot> dots(2);
    dots[0].pos = Vec2(256.0f, 256.0f);
    dots[0].scale = 0.3f;
    dots[0].texture = "player-map-outer.img";
    dots[0].tint = 0xffffff;
    dots[0].zOrder = 10;
    dots[1].pos = Vec2(512.0f, 512.0f);
    dots[1].scale = 0.2f;
    dots[1].texture = "player-map-inner.img";
    dots[1].tint = 0xffff00;
    dots[1].zOrder = 11;
    view.setDots(dots);

    const auto* outer = static_cast<const pix::NullSprite*>(view.dotSprite(0));
    const auto* inner = static_cast<const pix::NullSprite*>(view.dotSprite(1));
    CHECK(outer != nullptr && inner != nullptr);
    CHECK_NEAR(outer->x, 128.0f, 0.01f);
    CHECK_NEAR(outer->y, 128.0f, 0.01f);
    CHECK_NEAR(inner->x, 256.0f, 0.01f);
    CHECK_NEAR(inner->y, 256.0f, 0.01f);
    CHECK_NEAR(inner->sx, 0.2f, 0.001f); // 256px view / 256px texture
    CHECK_EQ(inner->getTint(), 0xffff00u);
    CHECK(view.dotSprite(2) == nullptr); // pool grows lazily

    MapGas gas;
    gas.active = true;
    gas.pos = Vec2(256.0f, 256.0f);
    gas.rad = 128.0f;
    gas.hasSafe = true;
    gas.safePos = Vec2(256.0f, 256.0f);
    gas.safeRad = 64.0f;
    gas.playerPos = Vec2(0.0f, 0.0f);
    view.setGas(gas);
    CHECK(view.gasGraphics() != nullptr);
    CHECK(!static_cast<pix::NullGraphics*>(view.gasGraphics())->commands.empty());

    // Inverse projection (big-map taps): local centre -> world centre.
    const Vec2 world = view.unproject(Vec2(128.0f, 128.0f));
    CHECK_NEAR(world.x, 256.0f, 0.01f);
    CHECK_NEAR(world.y, 256.0f, 0.01f);

    // Airstrike warning zone (yellow fill/outline) is drawn as well.
    std::vector<MapAirstrike> strikes(1);
    strikes[0].pos = Vec2(128.0f, 128.0f);
    strikes[0].rad = 64.0f;
    strikes[0].alpha = 1.0f;
    view.setAirstrikes(strikes);
    CHECK(view.airstrikeGraphics() != nullptr);
    CHECK(!static_cast<pix::NullGraphics*>(view.airstrikeGraphics())->commands.empty());

    // Place names (map.ts nameContainer) are laid over the map in HUD text.
    // MapMsg places are normalized (0..1) into the view box.
    std::vector<MapPlace> places(1);
    places[0].name = "Bank";
    places[0].pos = Vec2(0.5f, 0.5f);
    view.setLabels(places);
    auto* label = static_cast<pix::NullText*>(view.labelText(0));
    CHECK(label != nullptr);
    CHECK_EQ(label->text, std::string("Bank"));
    CHECK_NEAR(label->x, 128.0f, 0.01f);
    CHECK_NEAR(label->y, 128.0f, 0.01f);
}

TEST(map_indicator_barn_data_and_pulse) {
    // mapIndicator.ts: server data adds/updates/frees indicators; the pulse
    // ticker eases between 0.5 and 1 (speed 0.3) and flips at both ends.
    MapIndicatorBarn barn;
    barn.addOrUpdate(7, false, true, "the_hunted", Vec2(100.0f, 50.0f));
    CHECK_EQ(barn.indicators.size(), 1u);
    const Indicator* ind = barn.find(7);
    CHECK(ind != nullptr);
    if (ind) {
        CHECK(ind->active);
        CHECK(ind->equipped);
        CHECK_EQ(ind->type, std::string("the_hunted"));
        CHECK_NEAR(ind->pos.x, 100.0f, 1e-4f);
        CHECK_NEAR(ind->pulseTicker, 0.5f, 1e-4f);
        CHECK_NEAR(ind->pulseDir, 1.0f, 1e-4f);
    }

    barn.updatePulses(1.0f); // 0.5 + 0.3 = 0.8, rising
    ind = barn.find(7);
    CHECK(ind != nullptr);
    if (ind) {
        CHECK_NEAR(ind->pulseTicker, 0.8f, 1e-4f);
        CHECK_NEAR(ind->pulseScale, 0.8f, 1e-4f);
        CHECK_NEAR(ind->pulseDir, 1.0f, 1e-4f);
    }
    barn.updatePulses(1.0f); // clamped at 1.0 -> flips down
    ind = barn.find(7);
    if (ind) {
        CHECK_NEAR(ind->pulseTicker, 1.0f, 1e-4f);
        CHECK_NEAR(ind->pulseDir, -1.0f, 1e-4f);
    }
    barn.updatePulses(1.0f); // 0.7, still falling
    ind = barn.find(7);
    if (ind) {
        CHECK_NEAR(ind->pulseTicker, 0.7f, 1e-4f);
        CHECK_NEAR(ind->pulseDir, -1.0f, 1e-4f);
    }
    barn.updatePulses(1.0f); // clamped at the 0.5 floor -> flips up
    ind = barn.find(7);
    if (ind) {
        CHECK_NEAR(ind->pulseTicker, MapIndicatorBarn::kPulseScaleMin, 1e-4f);
        CHECK_NEAR(ind->pulseDir, 1.0f, 1e-4f);
    }

    // An update moves the existing entry instead of adding a second one.
    barn.addOrUpdate(7, false, false, "helmet03_forest", Vec2(10.0f, 20.0f));
    CHECK_EQ(barn.indicators.size(), 1u);
    ind = barn.find(7);
    if (ind) {
        CHECK(!ind->equipped);
        CHECK_EQ(ind->type, std::string("helmet03_forest"));
        CHECK_NEAR(ind->pos.y, 20.0f, 1e-4f);
    }

    // `dead` frees the slot; a later add reuses it (no unbounded growth).
    barn.addOrUpdate(8, false, false, "the_hunted", Vec2(0.0f, 0.0f));
    CHECK_EQ(barn.indicators.size(), 2u);
    barn.addOrUpdate(7, true, false, "", Vec2(0.0f, 0.0f));
    CHECK(barn.find(7) == nullptr);
    barn.addOrUpdate(9, false, false, "the_hunted", Vec2(1.0f, 1.0f));
    CHECK_EQ(barn.indicators.size(), 2u);
    CHECK(barn.find(9) != nullptr);
    barn.clear();
    CHECK(barn.indicators.empty());
}

TEST(map_indicator_defs_have_sprite_and_pulse) {
    installGeneratedDefs();
    const DefProvider* provider = getDefProvider();
    CHECK(provider != nullptr);
    if (!provider) {
        return;
    }
    // roleDefs.ts `the_hunted`: orange hunted marker with a pulse ring.
    const GameObjRenderDef* hunted = provider->gameObject("the_hunted");
    CHECK(hunted != nullptr);
    if (hunted) {
        CHECK_EQ(hunted->mapIndicatorSprite, std::string("player-the-hunted.img"));
        CHECK_EQ(hunted->mapIndicatorTint, 0xff8400u);
        CHECK(hunted->mapIndicatorPulse);
        CHECK_EQ(hunted->mapIndicatorPulseTint, 0xff8400u);
    }
    // gearDefs.ts `helmet03_forest` (Shishigami no Kabuto): green woods-king.
    const GameObjRenderDef* king = provider->gameObject("helmet03_forest");
    CHECK(king != nullptr);
    if (king) {
        CHECK_EQ(king->mapIndicatorSprite, std::string("player-king-woods.img"));
        CHECK_EQ(king->mapIndicatorTint, 0x00ff00u);
        CHECK(king->mapIndicatorPulse);
    }
    // Plain items carry no map indicator.
    const GameObjRenderDef* bandage = provider->gameObject("bandage");
    CHECK(bandage == nullptr || bandage->mapIndicatorSprite.empty());
}

TEST(map_view_indicator_markers_and_pulse_ring) {
    pix::NullPixiFactory factory;
    pix::Container* root = factory.createContainer();
    MapView view;
    view.build(&factory, root, Vec2(0.0f, 0.0f), 256.0f);
    auto* rt = static_cast<pix::NullRenderTexture*>(factory.createRenderTexture());
    rt->resize(256.0f, 256.0f);
    view.setTexture(rt);
    view.setMapSize(512.0f, 512.0f);

    std::vector<MapIndicatorMarker> markers(2);
    markers[0].pos = Vec2(256.0f, 256.0f);
    markers[0].texture = "player-the-hunted.img";
    markers[0].tint = 0xff8400;
    markers[0].equipped = true;
    markers[0].pulse = true;
    markers[0].pulseTint = 0xff8400;
    markers[0].pulseScale = 0.75f;
    markers[1].pos = Vec2(512.0f, 512.0f);
    markers[1].texture = "player-king-woods.img";
    markers[1].tint = 0x00ff00;
    view.setIndicators(markers);

    const auto* hunted = static_cast<const pix::NullSprite*>(view.indicatorSprite(0));
    CHECK(hunted != nullptr && hunted->visible);
    if (hunted) {
        CHECK_EQ(hunted->frame, std::string("player-the-hunted.img"));
        CHECK_EQ(hunted->getTint(), 0xff8400u);
        CHECK_NEAR(hunted->x, 128.0f, 0.01f);
        CHECK_NEAR(hunted->y, 128.0f, 0.01f);
        // mapIndicator.ts: (Sm ? 0.15 : 0.2) * 1.25, 256px view / 256px texture.
        CHECK_NEAR(hunted->sx, 0.15f * 1.25f, 1e-4f);
        // Equipped markers sort above the player dots; plain ones below them.
        CHECK_EQ(MapView::indicatorZOrder(true), 655350);
        CHECK_EQ(MapView::indicatorZOrder(false), 1);
        CHECK_EQ(MapView::indicatorPulseZOrder(true), 655349);
    }
    const auto* pulse = static_cast<const pix::NullSprite*>(view.indicatorPulseSprite(0));
    CHECK(pulse != nullptr && pulse->visible);
    if (pulse) {
        CHECK_EQ(pulse->frame, std::string("part-pulse-01.img"));
        CHECK_EQ(pulse->getTint(), 0xff8400u);
        CHECK_NEAR(pulse->sx, 0.75f, 1e-4f);
    }
    const auto* king = static_cast<const pix::NullSprite*>(view.indicatorSprite(1));
    CHECK(king != nullptr && king->visible);
    if (king) {
        CHECK_NEAR(king->x, 256.0f, 0.01f);
    }
    CHECK(view.indicatorPulseSprite(1) == nullptr); // def has no pulse ring

    // Clearing markers hides the pooled sprites (new map / mode change).
    view.setIndicators({});
    CHECK(!hunted->visible);
    CHECK(!pulse->visible);
    CHECK(!king->visible);
}

TEST(map_view_minimap_is_zoom_centred_and_clipped) {
    pix::NullPixiFactory factory;
    pix::Container* root = factory.createContainer();
    MapView view;
    view.build(&factory, root, Vec2(10.0f, 20.0f), 140.0f);
    auto* rt = static_cast<pix::NullRenderTexture*>(factory.createRenderTexture());
    rt->resize(1024.0f, 1024.0f);
    view.setZoomed(true);
    view.setTexture(rt);
    view.setMapSize(512.0f, 512.0f);
    view.setCenter(Vec2(128.0f, 384.0f));

    const float display = 140.0f / MapView::kMinimapViewFraction;
    const auto* map = static_cast<const pix::NullSprite*>(view.mapSprite());
    CHECK(map != nullptr);
    CHECK_NEAR(map->sx, display / 1024.0f, 1e-4f);
    CHECK_NEAR(map->x, 70.0f - (128.0f / 512.0f) * display, 0.01f);
    CHECK_NEAR(map->y, 70.0f - (384.0f / 512.0f) * display, 0.01f);
    CHECK_NEAR(map->getAlpha(), 0.8f, 0.001f);

    // The active player is at the centre; points outside the 140px window
    // project outside (the ClippingNode hides them).
    const Vec2 centre = view.project(Vec2(128.0f, 384.0f));
    CHECK_NEAR(centre.x, 70.0f, 0.01f);
    CHECK_NEAR(centre.y, 70.0f, 0.01f);
    const Vec2 corner = view.project(Vec2(0.0f, 0.0f));
    CHECK(corner.x < 0.0f);
    CHECK(corner.y < 0.0f);
    const Vec2 back = view.unproject(centre);
    CHECK_NEAR(back.x, 128.0f, 0.01f);
    CHECK_NEAR(back.y, 384.0f, 0.01f);

    const auto* clip = static_cast<const pix::NullContainer*>(view.root());
    CHECK(clip != nullptr && clip->maskNode != nullptr);
}

TEST(ping_wheel_idle_timeout_closes) {
    PingFixture fx;
    fx.wheel.open(fx.wheel.buttonCenter());
    CHECK(fx.wheel.visible());
    fx.wheel.update(9.0f);
    CHECK(fx.wheel.visible());
    // emote.ts emoteTimeout = 10s.
    fx.wheel.update(1.5f);
    CHECK(!fx.wheel.visible());
}

TEST(hud_gas_timer_and_result_overlay) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    pix::Container* root = factory.createContainer();
    Hud hud;
    hud.build(&factory, root, Vec2(1280.0f, 720.0f));
    hud.setVisible(true);

    // Gas countdown: m:ss, hidden when gas is inactive.
    CHECK_EQ(Hud::formatTime(5), std::string("0:05"));
    CHECK_EQ(Hud::formatTime(65), std::string("1:05"));
    CHECK_EQ(Hud::formatTime(600), std::string("10:00"));
    GasHudState gas;
    gas.active = true;
    gas.moving = false;
    gas.secondsLeft = 83.9f;
    hud.setGasState(gas);
    auto* gasText = static_cast<const pix::NullText*>(hud.gasText());
    CHECK(gasText != nullptr);
    CHECK_EQ(gasText->text, std::string("1:23"));
    CHECK(gasText->visible);
    gas.active = false;
    hud.setGasState(gas);
    CHECK(!gasText->visible);

    // Result screen: defeat + rank + team kills + local stats.
    GameOverMsg over;
    over.teamId = 1;
    over.teamRank = 3;
    over.winningTeamId = 2;
    over.gameOver = true;
    PlayerStatsMsg me;
    me.playerId = 42;
    me.kills = 4;
    me.damageDealt = 321;
    me.timeAlive = 95;
    PlayerStatsMsg mate;
    mate.playerId = 43;
    mate.kills = 2;
    over.playerStats = { me, mate };
    hud.showGameOver(over, 1, 42);
    CHECK(hud.gameOverShown());
    auto* title = static_cast<const pix::NullText*>(hud.gameOverTitleText());
    auto* rank = static_cast<const pix::NullText*>(hud.gameOverRankText());
    auto* stats = static_cast<const pix::NullText*>(hud.gameOverStatsText());
    CHECK(title != nullptr && rank != nullptr && stats != nullptr);
    CHECK_EQ(title->text, std::string("DEFEAT"));
    CHECK_EQ(rank->text, std::string("Rank #3"));
    CHECK(stats->text.find("Team kills: 6") != std::string::npos);
    CHECK(stats->text.find("Kills: 4") != std::string::npos);
    CHECK(stats->text.find("Damage: 321") != std::string::npos);
    CHECK(stats->text.find("Survived: 1:35") != std::string::npos);
    auto* statRow0 = static_cast<const pix::NullText*>(hud.gameOverStatRow(0));
    auto* statRow1 = static_cast<const pix::NullText*>(hud.gameOverStatRow(1));
    CHECK(statRow0 != nullptr && statRow1 != nullptr);
    CHECK(statRow0->text.find("Player 42") != std::string::npos);
    CHECK(statRow0->text.find("4 K") != std::string::npos);
    CHECK(statRow0->text.find("321 DMG") != std::string::npos);
    CHECK(statRow0->visible);
    CHECK(statRow1->text.find("2 K") != std::string::npos);
    CHECK(!static_cast<const pix::NullText*>(hud.gameOverStatRow(2))->visible);

    over.winningTeamId = 1;
    hud.showGameOver(over, 1, 42);
    CHECK_EQ(title->text, std::string("VICTORY"));
}

TEST(hud_squad_health_bars) {
    pix::NullPixiFactory factory;
    pix::Container* root = factory.createContainer();
    Hud hud;
    hud.build(&factory, root, Vec2(1280.0f, 720.0f));
    hud.setVisible(true);

    std::vector<SquadMember> squad(4);
    squad[0].name = "Alice";
    squad[0].health = 100.0f;
    squad[1].name = "Bob";
    squad[1].health = 0.0f;
    squad[1].dead = true;
    squad[2].name = "Cara";
    squad[2].health = 0.0f;
    squad[2].downed = true;
    squad[3].name = "Dan";
    squad[3].health = 50.0f;
    squad[3].disconnected = true;
    hud.setSquadState(squad);

    const auto* n0 = static_cast<const pix::NullText*>(hud.squadNameText(0));
    const auto* n1 = static_cast<const pix::NullText*>(hud.squadNameText(1));
    const auto* n2 = static_cast<const pix::NullText*>(hud.squadNameText(2));
    const auto* n3 = static_cast<const pix::NullText*>(hud.squadNameText(3));
    CHECK(n0 && n1 && n2 && n3);
    CHECK_EQ(n0->text, std::string("Alice"));
    CHECK_EQ(n1->text, std::string("Bob"));
    CHECK_EQ(n2->text, std::string("Cara"));
    CHECK_EQ(n3->text, std::string("Dan"));
    CHECK(n0->visible && n3->visible);

    CHECK_EQ(static_cast<const pix::NullText*>(hud.squadStatusText(0))->text, std::string(""));
    CHECK_EQ(static_cast<const pix::NullText*>(hud.squadStatusText(1))->text, std::string("Dead"));
    CHECK_EQ(static_cast<const pix::NullText*>(hud.squadStatusText(2))->text, std::string("Down"));
    CHECK_EQ(static_cast<const pix::NullText*>(hud.squadStatusText(3))->text, std::string("DC"));

    // ui.ts updateHealthBar colours: 100 grey, 50 pink, dead no fill, downed
    // (0 health) the 1px danger fill.
    const auto fillColor = [&hud](int idx, uint32_t& color, float& alpha) {
        const auto* g = static_cast<const pix::NullGraphics*>(hud.squadHealthFill(idx));
        if (!g) return false;
        for (const auto& cmd : g->commands) {
            if (cmd.kind == pix::DrawCommand::BeginFill) {
                color = cmd.color;
                alpha = cmd.alpha;
                return true;
            }
        }
        return false;
    };
    uint32_t color = 0;
    float alpha = 0.0f;
    CHECK(fillColor(0, color, alpha));
    CHECK_EQ(color, 0xb3b3b3u); // 100 health -> grey
    CHECK_NEAR(alpha, 1.0f, 1e-4f);
    CHECK(!fillColor(1, color, alpha)); // dead -> no fill
    CHECK(fillColor(2, color, alpha));
    CHECK_EQ(color, 0x000000u); // downed at 0 health -> danger pulse (black at t=0)
    CHECK_NEAR(alpha, 0.8f, 1e-4f);
    CHECK(fillColor(3, color, alpha));
    CHECK_EQ(color, 0xff7777u); // 50 health -> darkpink/lightpink interpolation
    CHECK_NEAR(alpha, 1.0f, 1e-4f);

    // Solo / hidden HUD: rows disappear.
    hud.setSquadState({});
    CHECK(!n0->visible);
    hud.setSquadState(squad);
    hud.setVisible(false);
    CHECK(!n0->visible);
}

TEST(hud_gear_and_perk_icons) {
    pix::NullPixiFactory factory;
    pix::Container* root = factory.createContainer();
    Hud hud;
    hud.build(&factory, root, Vec2(1280.0f, 720.0f));
    hud.setVisible(true);

    hud.setGearState({
        "loot-helmet-02.img",
        "loot-chest-02.img",
        "loot-pack-00.img",
        "loot-perk-takedown.img",
    });
    const auto* helmet = static_cast<const pix::NullSprite*>(hud.gearIcon(0));
    const auto* chest = static_cast<const pix::NullSprite*>(hud.gearIcon(1));
    const auto* perk = static_cast<const pix::NullSprite*>(hud.gearIcon(3));
    const auto* empty = static_cast<const pix::NullSprite*>(hud.gearIcon(4));
    CHECK(helmet && chest && perk && empty);
    CHECK(helmet->visible);
    CHECK_EQ(helmet->getFrame(), std::string("loot-helmet-02.img"));
    CHECK_EQ(chest->getFrame(), std::string("loot-chest-02.img"));
    CHECK_EQ(perk->getFrame(), std::string("loot-perk-takedown.img"));
    CHECK(helmet->sx > 0.0f);
    CHECK(!empty->visible); // unused slots stay hidden

    hud.setGearState({});
    CHECK(!helmet->visible);
    hud.setGearState({ "loot-helmet-02.img" });
    CHECK(helmet->visible);
    hud.setVisible(false);
    CHECK(!helmet->visible);
}

TEST(hud_big_map_toggle_and_hits) {
    installGeneratedDefs();
    pix::NullPixiFactory factory;
    pix::Container* root = factory.createContainer();
    Hud hud;
    hud.build(&factory, root, Vec2(1280.0f, 720.0f));
    auto* rt = static_cast<pix::NullRenderTexture*>(factory.createRenderTexture());
    rt->resize(256.0f, 256.0f);
    hud.setMinimap(rt, 512.0f, 512.0f);
    hud.setVisible(true);

    CHECK(!hud.bigMapVisible());
    CHECK(hud.minimap().visible());

    // The minimap itself is the big-map tap target: any point in its 140px
    // box (design-space coords) opens the big map.
    const Vec2 designOrigin(640.0f, 360.0f);
    const Vec2 mapPoints[2] = {
        v2Add(v2Add(hud.minimap().pos(), designOrigin), Vec2(5.0f, 5.0f)),
        v2Add(v2Add(hud.minimap().pos(), designOrigin),
              Vec2(Hud::kMinimapSize - 5.0f, Hud::kMinimapSize - 5.0f)),
    };
    for (const Vec2& point : mapPoints) {
        CHECK(hud.minimapTapHit(point.x, point.y));
    }
    CHECK(!hud.minimapTapHit(mapPoints[0].x - 100.0f, mapPoints[0].y));

    hud.setBigMapVisible(true);
    CHECK(hud.bigMapVisible());
    CHECK(!hud.minimap().visible());
    CHECK(!hud.minimapTapHit(mapPoints[0].x, mapPoints[0].y));

    const Vec2 center = v2Add(v2Add(hud.bigMap().pos(), designOrigin),
                              Vec2(Hud::kBigMapSize * 0.5f, Hud::kBigMapSize * 0.5f));
    CHECK(hud.bigMapContains(center.x, center.y));
    CHECK(!hud.bigMapContains(center.x - Hud::kBigMapSize, center.y));
    const Vec2 close = v2Add(v2Add(hud.bigMap().pos(), designOrigin),
                             Vec2(Hud::kBigMapSize + 10.0f, Hud::kBigMapSize - 10.0f));
    CHECK(hud.bigMapCloseHit(close.x, close.y));
    CHECK(!hud.bigMapCloseHit(center.x, center.y));

    hud.setBigMapVisible(false);
    CHECK(!hud.bigMapVisible());
    CHECK(hud.minimap().visible());

    // CycleUIMode (V): hide/show the minimap without opening the big map.
    hud.setMinimapHidden(true);
    CHECK(hud.minimapHidden());
    CHECK(!hud.minimap().visible());
    CHECK(!hud.minimapTapHit(mapPoints[0].x, mapPoints[0].y));
    hud.setMinimapHidden(false);
    CHECK(!hud.minimapHidden());
    CHECK(hud.minimap().visible());
    CHECK(hud.minimapTapHit(mapPoints[0].x, mapPoints[0].y));

    // Hiding the HUD also closes the big map.
    hud.setBigMapVisible(true);
    hud.setVisible(false);
    CHECK(!hud.bigMapVisible());
}

// ---------------------------------------------------------------------------
TEST(ping_sound_leader_and_airstrike_falloff) {
    installGeneratedDefs();
    audio::installGeneratedSoundDefs();

    class PingBackend : public audio::AudioBackend {
    public:
        struct Played {
            std::string id;
            float volume = 0.0f;
            float pan = 0.0f;
        };
        std::vector<Played> played;
        audio::SoundHandle next = 1;

        bool load(const std::string&, const std::string&) override { return true; }
        bool isLoaded(const std::string&) const override { return true; }
        audio::SoundHandle play(const std::string& id, const audio::PlayOptions& opts) override {
            played.push_back({id, opts.volume, opts.pan});
            return next++;
        }
        void stop(audio::SoundHandle) override {}
        void stopAll() override {}
        void setVolume(audio::SoundHandle, float) override {}
        float getVolume(audio::SoundHandle) const override { return 0.0f; }
        void setPan(audio::SoundHandle, float) override {}
        void setMute(bool) override {}
        bool isPlaying(audio::SoundHandle) const override { return true; }
    };

    PingBackend backend;
    audio::AudioManager audioManager(&backend);
    audioManager.cameraPos = Vec2(0.0f, 0.0f);
    audioManager.activeLayer = 0;

    pix::NullPixiFactory factory;
    GameWorld world(&factory, false);
    world.setAudio(&audioManager, nullptr);
    world.setPlayerRoleLookup([](uint16_t playerId) {
        return playerId == 2 ? std::string("leader")
            : playerId == 3 ? std::string("captain")
            : playerId == 4 ? std::string("last_man") : std::string();
    });

    const DefProvider* defs = getDefProvider();
    const PingRenderDef* danger = defs ? defs->ping("ping_danger") : nullptr;
    const PingRenderDef* airstrike = defs ? defs->ping("ping_airstrike") : nullptr;
    CHECK(danger != nullptr);
    CHECK(airstrike != nullptr);
    if (!danger || !airstrike) {
        return;
    }
    CHECK(!danger->soundLeader.empty());

    EmoteManager& manager = world.emoteManager();

    // Leader/captain/last-man pings use soundLeader.
    Emote leaderPing;
    leaderPing.isPing = true;
    leaderPing.playerId = 2;
    leaderPing.type = "ping_danger";
    leaderPing.hasPos = true;
    leaderPing.pos = Vec2(0.0f, 0.0f);
    manager.addPing(leaderPing, false, world);
    CHECK_EQ(backend.played.back().id, danger->soundLeader + "ui");

    for (uint16_t id : { 3u, 4u }) {
        leaderPing.playerId = id;
        manager.addPing(leaderPing, false, world);
        CHECK_EQ(backend.played.back().id, danger->soundLeader + "ui");
    }

    // A normal player uses the plain ping sound.
    leaderPing.playerId = 5;
    manager.addPing(leaderPing, false, world);
    CHECK_EQ(backend.played.back().id, danger->sound + "ui");
    const float dangerVolume = backend.played.back().volume;
    CHECK(dangerVolume > 0.0f);
    CHECK_NEAR(backend.played.back().pan, 0.0f, 1e-6f);

    // Non-faction airstrike pings attenuate with distance (rangeMult 20,
    // fallOff 1) and pan toward the ping.
    Emote strikePing;
    strikePing.isPing = true;
    strikePing.type = "ping_airstrike";
    strikePing.hasPos = true;
    strikePing.pos = Vec2(600.0f, 0.0f);
    manager.addPing(strikePing, false, world);
    CHECK_EQ(backend.played.back().id, airstrike->sound + "ui");
    CHECK(backend.played.back().volume < dangerVolume * 0.5f);
    CHECK(backend.played.back().pan > 0.1f);

    // Faction mode keeps the airstrike ping at full volume.
    manager.addPing(strikePing, true, world);
    CHECK_NEAR(backend.played.back().volume, dangerVolume, 1e-4f);
    CHECK_NEAR(backend.played.back().pan, 0.0f, 1e-6f);
}

