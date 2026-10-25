// P0.1 host tests: HUD action buttons -> InputActions -> InputMsg, plus the
// quick-use item/scope selection and the HUD hit-test geometry. Engine
// independent (NullPixi, FakeConnection), so it runs in `surv_tests`.
#include "TestFramework.h"
#include "FakeConnection.h"
#include "../src/game/Game.h"
#include "../src/game/Hud.h"
#include "../src/game/InputActions.h"
#include "../src/net/generatedDefs.inc"
#include "../src/render/NullPixi.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

using namespace surv;
using namespace surv_test;

namespace {

int bagIndex(const char* type) {
    for (size_t i = 0; i < defs::kBagSizeKeys.size(); ++i) {
        if (defs::kBagSizeKeys[i] == type) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

std::vector<uint8_t> makeFrame(MsgType type, Msg& msg) {
    MsgStream stream(std::vector<uint8_t>(1024, 0));
    stream.serializeMsg(type, [&msg](NetBitStream& s) { msg.serialize(s); });
    return stream.getBuffer();
}

struct Harness {
    Game game;
    FakeConnection* conn = nullptr;

    Harness() {
        game.init(nullptr, [this](const std::string& url) -> std::unique_ptr<Connection> {
            auto* c = new FakeConnection(url);
            conn = c;
            return std::unique_ptr<Connection>(c);
        });
        game.tryJoinGame("ws://dev.example:9000/play", "tok");
        conn->open();
        game.update(0.0f);
    }

    void push(MsgType type, Msg& msg) {
        conn->pushFrame(makeFrame(type, msg));
        game.update(0.0f);
    }
};

// Builds a live UpdateMsg with a bandage + scope inventory and two guns.
UpdateMsg makeActionUpdate() {
    UpdateMsg u;
    u.activePlayerId = 42;
    u.activePlayerIdDirty = true;
    u.activePlayerData.healthDirty = true;
    u.activePlayerData.health = 80.0f;
    u.activePlayerData.boostDirty = true;
    u.activePlayerData.boost = 10.0f;
    u.activePlayerData.inventoryDirty = true;
    u.activePlayerData.inventory.assign(defs::kBagSizeKeys.size(), 0);
    const int bandage = bagIndex("bandage");
    const int soda = bagIndex("soda");
    const int pills = bagIndex("painkiller");
    const int scope = bagIndex("4xscope");
    if (bandage >= 0) u.activePlayerData.inventory[static_cast<size_t>(bandage)] = 3;
    if (soda >= 0) u.activePlayerData.inventory[static_cast<size_t>(soda)] = 2;
    if (pills >= 0) u.activePlayerData.inventory[static_cast<size_t>(pills)] = 1;
    if (scope >= 0) u.activePlayerData.inventory[static_cast<size_t>(scope)] = 1;
    u.activePlayerData.weapsDirty = true;
    u.activePlayerData.weapons = {{"ak47", 30}, {"", 0}, {"fists", 0}, {"", 0}};
    u.activePlayerData.curWeapIdx = 0;
    u.activePlayerData.scope = "4xscope";
    u.ack = 1;
    return u;
}

} // namespace

TEST(input_actions_serialize_inputs_and_use_item) {
    InputActions actions;
    actions.press(Input_Reload);
    actions.press(Input_Interact);
    actions.press(Input_Cancel);
    actions.use("bandage");
    CHECK(!actions.empty());

    InputMsg msg;
    actions.applyTo(msg);
    CHECK_EQ(msg.inputs.size(), static_cast<size_t>(3));
    CHECK_EQ(msg.inputs[0], static_cast<uint8_t>(Input_Reload));
    CHECK_EQ(msg.inputs[1], static_cast<uint8_t>(Input_Interact));
    CHECK_EQ(msg.inputs[2], static_cast<uint8_t>(Input_Cancel));
    CHECK_EQ(msg.useItem, std::string("bandage"));

    // The wire form carries the same actions (server-side handleInput reads
    // msg.inputs and server handleInputMsg reads msg.useItem).
    MsgStream stream(std::vector<uint8_t>(256, 0));
    stream.serializeMsg(MsgType_Input, [&msg](NetBitStream& s) { msg.serialize(s); });
    MsgStream in(stream.getBuffer());
    CHECK_EQ(static_cast<int>(in.deserializeMsgType()), static_cast<int>(MsgType_Input));
    InputMsg out;
    out.deserialize(in.getStream());
    CHECK_EQ(out.inputs.size(), static_cast<size_t>(3));
    CHECK_EQ(out.inputs[0], static_cast<uint8_t>(Input_Reload));
    CHECK_EQ(out.useItem, std::string("bandage"));
}

TEST(input_actions_dedupe_and_cap_like_add_input) {
    InputActions actions;
    // game.ts interaction touch sends Interact + Cancel.
    actions.press(Input_Interact);
    actions.press(Input_Cancel);
    actions.press(Input_Interact); // duplicate dropped
    CHECK_EQ(actions.inputs().size(), static_cast<size_t>(2));

    InputMsg msg;
    actions.applyTo(msg);
    actions.clear();
    CHECK(actions.empty());
    CHECK_EQ(msg.inputs.size(), static_cast<size_t>(2));

    // InputMsg::addInput caps at 7 (shared/net/inputMsg.ts).
    InputActions many;
    for (int i = 0; i < 12; ++i) {
        many.press(static_cast<uint8_t>(i));
    }
    CHECK_EQ(many.inputs().size(), InputActions::kMaxInputs);
    InputMsg capped;
    many.applyTo(capped);
    CHECK_EQ(capped.inputs.size(), InputActions::kMaxInputs);
}

TEST(input_actions_weapon_slot_mapping) {
    CHECK_EQ(inputForWeaponSlot(WeaponSlot_Primary), static_cast<uint8_t>(Input_EquipPrimary));
    CHECK_EQ(inputForWeaponSlot(WeaponSlot_Secondary), static_cast<uint8_t>(Input_EquipSecondary));
    CHECK_EQ(inputForWeaponSlot(WeaponSlot_Melee), static_cast<uint8_t>(Input_EquipMelee));
    CHECK_EQ(inputForWeaponSlot(WeaponSlot_Throwable), static_cast<uint8_t>(Input_EquipThrowable));
    CHECK_EQ(inputForWeaponSlot(99), static_cast<uint8_t>(Input_Count));
}

TEST(hud_quick_use_item_selection) {
    std::vector<uint16_t> inv(defs::kBagSizeKeys.size(), 0);
    const int bandage = bagIndex("bandage");
    const int healthkit = bagIndex("healthkit");
    const int soda = bagIndex("soda");
    const int pills = bagIndex("painkiller");

    CHECK_EQ(Hud::healItemFor(inv, 50.0f), std::string());
    inv[static_cast<size_t>(bandage)] = 4;
    CHECK_EQ(Hud::healItemFor(inv, 80.0f), std::string("bandage"));
    CHECK_EQ(Hud::healItemFor(inv, 20.0f), std::string("bandage"));

    // A med kit takes over when badly hurt (or when the bandages run out).
    inv[static_cast<size_t>(healthkit)] = 1;
    CHECK_EQ(Hud::healItemFor(inv, 20.0f), std::string("healthkit"));
    CHECK_EQ(Hud::healItemFor(inv, 80.0f), std::string("bandage"));
    inv[static_cast<size_t>(bandage)] = 0;
    CHECK_EQ(Hud::healItemFor(inv, 80.0f), std::string("healthkit"));

    CHECK_EQ(Hud::boostItemFor(inv, 10.0f), std::string());
    inv[static_cast<size_t>(soda)] = 2;
    CHECK_EQ(Hud::boostItemFor(inv, 90.0f), std::string("soda"));
    inv[static_cast<size_t>(pills)] = 1;
    CHECK_EQ(Hud::boostItemFor(inv, 10.0f), std::string("painkiller"));
    CHECK_EQ(Hud::boostItemFor(inv, 90.0f), std::string("soda"));
    inv[static_cast<size_t>(soda)] = 0;
    CHECK_EQ(Hud::boostItemFor(inv, 90.0f), std::string("painkiller"));

    CHECK_EQ(Hud::bagCount(inv, "painkiller"), 1);
    CHECK_EQ(Hud::bagCount(inv, "not_an_item"), 0);
    CHECK_EQ(Hud::bagCount(inv, ""), 0);
}

TEST(hud_scope_label) {
    CHECK_EQ(Hud::scopeLabel(""), std::string("SCOPE"));
    CHECK_EQ(Hud::scopeLabel("1xscope"), std::string("SCOPE 1X"));
    CHECK_EQ(Hud::scopeLabel("4xscope"), std::string("SCOPE 4X"));
    CHECK_EQ(Hud::scopeLabel("8xscope"), std::string("SCOPE 8X"));
}

TEST(hud_action_bar_hit_test) {
    pix::NullPixiFactory factory;
    pix::Container* root = factory.createContainer();
    Hud hud;
    hud.build(&factory, root, Vec2(1280.0f, 720.0f));
    hud.setVisible(true);

    // Hidden HUD never claims a touch.
    hud.setVisible(false);
    CHECK(hud.hitTest(900.0f, 90.0f).kind == Hud::TapKind::None);
    hud.setVisible(true);

    CHECK(hud.hitTest(900.0f, 90.0f).kind == Hud::TapKind::Reload);
    CHECK(hud.hitTest(400.0f, 300.0f).kind == Hud::TapKind::Interact);
    // Empty quick-use pills are hidden and must not swallow aiming touches.
    CHECK(hud.hitTest(610.0f, 326.0f).kind == Hud::TapKind::None);
    CHECK(hud.hitTest(610.0f, 276.0f).kind == Hud::TapKind::None);
    CHECK(hud.hitTest(770.0f, 326.0f).kind == Hud::TapKind::None);
    CHECK(hud.hitTest(770.0f, 276.0f).kind == Hud::TapKind::SwapWeapons);

    // Tap targets outside every control fall through to the pads.
    CHECK(hud.hitTest(640.0f, 600.0f).kind == Hud::TapKind::None);
    CHECK(hud.hitTest(250.0f, 200.0f).kind == Hud::TapKind::None);

    // The four weapon cards (bottom-right) are tap-to-equip targets.
    const Hud::TapResult slot0 = hud.hitTest(1150.0f, 142.0f);
    CHECK(slot0.kind == Hud::TapKind::WeaponSlot);
    CHECK_EQ(slot0.slot, 0);
    const Hud::TapResult slot3 = hud.hitTest(1150.0f, 46.0f);
    CHECK(slot3.kind == Hud::TapKind::WeaponSlot);
    CHECK_EQ(slot3.slot, 3);
}

TEST(hud_action_bar_shows_inventory_state) {
    Harness h;
    UpdateMsg update = makeActionUpdate();
    h.push(MsgType_Update, update);

    pix::NullPixiFactory factory;
    pix::Container* root = factory.createContainer();
    Hud hud;
    hud.build(&factory, root, Vec2(1280.0f, 720.0f));
    hud.update(0.016f, h.game, true);

    // Quick-use pills: best heal/boost from the bag, counts from inventory.
    CHECK_EQ(hud.healItem(), std::string("bandage"));
    CHECK_EQ(Hud::bagCount(h.game.activePlayerData().inventory, "painkiller"), 1);
    // 10 boost is quantized to 8 bits on the wire (10.196...).
    CHECK(h.game.activePlayerData().boost > 9.0f);
    CHECK_EQ(hud.boostItem(), std::string("painkiller"));
    auto* heal = static_cast<const pix::NullText*>(hud.healButtonText());
    auto* boost = static_cast<const pix::NullText*>(hud.boostButtonText());
    auto* scope = static_cast<const pix::NullText*>(hud.scopeButtonText());
    CHECK(heal && boost && scope);
    CHECK(heal->visible);
    CHECK(boost->visible);
    CHECK(scope->visible);
    CHECK_EQ(heal->text, std::string("BANDAGE x3"));
    // boost=10: the 50-boost pills beat the 25-boost soda.
    CHECK_EQ(boost->text, std::string("PILLS x1"));
    CHECK_EQ(scope->text, std::string("SCOPE 4X"));

    // The pills are live tap targets now that the bag has the items.
    CHECK(hud.hitTest(610.0f, 326.0f).kind == Hud::TapKind::Heal);
    CHECK(hud.hitTest(610.0f, 276.0f).kind == Hud::TapKind::Boost);
    CHECK(hud.hitTest(770.0f, 326.0f).kind == Hud::TapKind::ScopeNext);

    // Weapon cards expose the live types for the long-press DropItemMsg.
    CHECK_EQ(hud.weaponType(0), std::string("ak47"));
    CHECK_EQ(hud.weaponType(1), std::string());
    CHECK_EQ(hud.weaponType(2), std::string("fists"));
    auto* slot0 = static_cast<const pix::NullText*>(hud.weaponSlotText(0));
    CHECK(slot0 && slot0->text.find("ak47") != std::string::npos);

    // The action bar disappears with the result screen.
    GameOverMsg over;
    over.teamId = 1;
    over.teamRank = 1;
    over.winningTeamId = 1;
    over.gameOver = true;
    h.push(MsgType_GameOver, over);
    hud.update(0.016f, h.game, true);
    CHECK(hud.gameOverShown());
    CHECK(hud.hitTest(900.0f, 90.0f).kind == Hud::TapKind::None);
    auto* reloadText = static_cast<const pix::NullText*>(hud.reloadButtonText());
    CHECK(reloadText != nullptr && !reloadText->visible);
}

TEST(hud_death_panel_and_spectate_overlay) {
    Harness h;
    JoinedMsg joined;
    joined.teamMode = 2;
    joined.playerId = 42;
    joined.started = true;
    h.push(MsgType_Joined, joined);

    UpdateMsg u;
    u.activePlayerId = 42;
    u.activePlayerIdDirty = true;
    u.activePlayerData.healthDirty = true;
    u.activePlayerData.health = 100.0f;
    PlayerInfo me;
    me.playerId = 42;
    me.teamId = 2;
    me.name = "Me";
    u.playerInfos.push_back(me);
    u.ack = 1;
    h.push(MsgType_Update, u);

    pix::NullPixiFactory factory;
    pix::Container* root = factory.createContainer();
    Hud hud;
    hud.build(&factory, root, Vec2(1280.0f, 720.0f));
    hud.update(0.016f, h.game, true);
    CHECK(!hud.deathPanelShown());
    CHECK(!hud.spectating());

    // Team death while a teammate is still alive: PlayerStatsMsg -> YOU DIED
    // panel with SPECTATE + LEAVE.
    PlayerStatsMsg stats;
    stats.playerId = 42;
    stats.dead = true;
    stats.kills = 4;
    stats.damageDealt = 321;
    stats.damageTaken = 111;
    stats.timeAlive = 95;
    h.push(MsgType_PlayerStats, stats);
    hud.update(0.016f, h.game, true);
    CHECK(hud.deathPanelShown());
    CHECK(hud.hitTest(520.0f, 266.0f).kind == Hud::TapKind::None);
    CHECK(hud.overlayActionAt(520.0f, 266.0f) == Hud::OverlayAction::SpectateBegin);
    CHECK(hud.overlayActionAt(760.0f, 266.0f) == Hud::OverlayAction::Leave);
    auto* deathTitle = static_cast<const pix::NullText*>(hud.deathTitleText());
    CHECK(deathTitle != nullptr);
    CHECK_EQ(deathTitle->text, std::string("YOU DIED"));

    // The server moves activePlayerId to a teammate: spectating overlay with
    // the target name/count and the touch options.
    UpdateMsg spec;
    spec.activePlayerId = 77;
    spec.activePlayerIdDirty = true;
    spec.activePlayerData.spectatorCountDirty = true;
    spec.activePlayerData.spectatorCount = 2;
    PlayerInfo mate;
    mate.playerId = 77;
    mate.teamId = 2;
    mate.name = "Mate";
    spec.playerInfos.push_back(mate);
    spec.ack = 2;
    h.push(MsgType_Update, spec);
    hud.update(0.016f, h.game, true);
    CHECK(hud.spectating());
    CHECK(!hud.deathPanelShown());
    auto* label = static_cast<const pix::NullText*>(hud.spectateLabelText());
    auto* count = static_cast<const pix::NullText*>(hud.spectateCountText());
    CHECK(label != nullptr && count != nullptr);
    CHECK_EQ(label->text, std::string("SPECTATING Mate"));
    CHECK_EQ(count->text, std::string("2 SPECTATING"));
    CHECK(hud.overlayActionAt(95.0f, 250.0f) == Hud::OverlayAction::SpectatePrev);
    CHECK(hud.overlayActionAt(95.0f, 200.0f) == Hud::OverlayAction::SpectateNext);
    CHECK(hud.overlayActionAt(95.0f, 150.0f) == Hud::OverlayAction::ToggleStats);
    CHECK(hud.overlayActionAt(95.0f, 100.0f) == Hud::OverlayAction::Leave);

    hud.toggleSpectateStats();
    CHECK(hud.spectateStatsShown());
    auto* specStats = static_cast<const pix::NullText*>(hud.spectateStatsText());
    CHECK(specStats != nullptr);
    CHECK(specStats->text.find("KILLS  4") != std::string::npos);
    CHECK(specStats->text.find("DAMAGE  321") != std::string::npos);

    // Final result replaces the spectate overlay; only LEAVE remains.
    GameOverMsg over;
    over.teamId = 2;
    over.teamRank = 1;
    over.gameOver = true;
    over.winningTeamId = 2;
    over.playerStats = {stats};
    h.push(MsgType_GameOver, over);
    hud.update(0.016f, h.game, true);
    CHECK(hud.gameOverShown());
    CHECK(!hud.spectating());
    CHECK(!hud.deathPanelShown());
    CHECK(hud.overlayActionAt(640.0f, 266.0f) == Hud::OverlayAction::Leave);
    CHECK(hud.overlayActionAt(520.0f, 266.0f) == Hud::OverlayAction::None);
}
