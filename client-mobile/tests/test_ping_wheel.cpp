// Host tests for the team-ping wheel (game/PingWheel) and the ping defs emitted
// by tools/codegen_render_defs.mjs (shared/defs/gameObjects/pingDefs.ts).
#include "TestFramework.h"

#include "game/PingWheel.h"
#include "game/GameWorld.h"
#include "game/EmoteManager.h"
#include "game/Hud.h"
#include "game/MapView.h"
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

    const RoleRenderDef* captain = provider->role("captain");
    CHECK(captain != nullptr);
    CHECK_EQ(captain->mapIconAlive, std::string("player-captain.img"));
    CHECK(captain->mapIconDead.empty());

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
    CHECK_NEAR(tex->width(), 256.0f, 0.001f);
    CHECK_NEAR(tex->height(), 256.0f, 0.001f);
    CHECK_NEAR(world.map().getMinimapScale(), 0.5f, 0.001f);
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
    CHECK_NEAR(icon.scale, 0.2f, 0.001f);
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
    CHECK_NEAR(map->sx, Hud::kMinimapSize / 256.0f, 0.001f);
    // Bottom-left of the top-right 140px frame (design 1280x720, centred).
    CHECK_NEAR(mini.pos().x, (1280.0f - Hud::kMinimapSize - 12.0f) - 640.0f, 0.01f);
    CHECK_NEAR(mini.pos().y, (720.0f - Hud::kMinimapSize - 12.0f) - 360.0f, 0.01f);

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
    CHECK_NEAR(sprite->sx, 0.2f * Hud::kMinimapSize / 256.0f, 0.001f);
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

    // Expand affordance sits inside the minimap's bottom-right corner. Hit
    // tests take design-space coordinates; the view positions are centred.
    const Vec2 designOrigin(640.0f, 360.0f);
    const Vec2 expand = v2Add(v2Add(hud.minimap().pos(), designOrigin),
                              Vec2(Hud::kMinimapSize - Hud::kMinimapExpandSize - 4.0f + 11.0f, 15.0f));
    CHECK(hud.minimapExpandHit(expand.x, expand.y));
    CHECK(!hud.minimapExpandHit(expand.x - 100.0f, expand.y));

    hud.setBigMapVisible(true);
    CHECK(hud.bigMapVisible());
    CHECK(!hud.minimap().visible());
    CHECK(!hud.minimapExpandHit(expand.x, expand.y));

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

    // Hiding the HUD also closes the big map.
    hud.setBigMapVisible(true);
    hud.setVisible(false);
    CHECK(!hud.bigMapVisible());
}
