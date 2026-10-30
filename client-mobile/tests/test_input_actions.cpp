// P0.1 host tests: HUD action buttons -> InputActions -> InputMsg, plus the
// quick-use item/scope selection and the HUD hit-test geometry. Engine
// independent (NullPixi, FakeConnection), so it runs in `surv_tests`.
#include "TestFramework.h"
#include "FakeConnection.h"
#include "../src/game/Game.h"
#include "../src/game/Hud.h"
#include "../src/game/InputActions.h"
#include "../src/net/generatedDefs.inc"
#include "../src/render/GeneratedDefs.h"
#include "../src/render/NullPixi.h"
#include "../src/ui/Json.h"

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
    CHECK(hud.hitTest(782.5f, 96.0f).kind == Hud::TapKind::None);
    hud.setVisible(true);

    // The bullet reload button above the health bar (#ui-reload-button) and the
    // swap pill stay tappable while the HUD is up.
    CHECK(hud.hitTest(782.5f, 96.0f).kind == Hud::TapKind::Reload);
    CHECK(hud.hitTest(900.0f, 150.0f).kind == Hud::TapKind::SwapWeapons);
    // An empty bag leaves only inert ghost pills, and without a nearby prompt
    // the interact hit stays None (touch falls through to the pads).
    CHECK(hud.hitTest(400.0f, 300.0f).kind == Hud::TapKind::None);
    CHECK(hud.hitTest(640.0f, 692.0f).kind == Hud::TapKind::None);
    CHECK(hud.hitTest(40.0f, 466.0f).kind == Hud::TapKind::None);

    // Tap targets outside every control fall through to the pads.
    CHECK(hud.hitTest(640.0f, 600.0f).kind == Hud::TapKind::None);
    CHECK(hud.hitTest(250.0f, 200.0f).kind == Hud::TapKind::None);
    // The four weapon cards (right column) are tap-to-equip targets.
    const Hud::TapResult slot0 = hud.hitTest(1150.0f, 400.0f);
    CHECK(slot0.kind == Hud::TapKind::WeaponSlot);
    CHECK_EQ(slot0.slot, 0);
    const Hud::TapResult slot3 = hud.hitTest(1150.0f, 260.0f);
    CHECK(slot3.kind == Hud::TapKind::WeaponSlot);
    CHECK_EQ(slot3.slot, 3);

    // The settings gear sits inside the minimap's top-left corner.
    const float gearX = Hud::kMinimapLeft + Hud::kSettingsSize * 0.5f;
    const float gearY = Hud::kMinimapTop + Hud::kSettingsSize * 0.5f;
    CHECK(hud.hitTest(gearX, 720.0f - gearY).kind == Hud::TapKind::Settings);
}

TEST(hud_action_bar_background_stays_above_the_messages) {
    pix::NullPixiFactory factory;
    pix::Container* root = factory.createContainer();
    Hud hud;
    hud.build(&factory, root, Vec2(1280.0f, 720.0f));
    hud.setVisible(true);
    hud.setActionProgress("RELOADING", 0.0f, 1.0f);

    auto* bg = static_cast<const pix::NullGraphics*>(hud.actionBarBg());
    CHECK(bg != nullptr);
    if (!bg) {
        return;
    }
    // The 240x14 bar is centred 88px below the top of the 1280x720 design
    // space, in the centred HUD container. paintBox() positions its node from
    // the rect it draws; passing the relative {0,0,...} rect parked the
    // background at the HUD origin (screen centre = the player) while the fill
    // and label stayed above the messages.
    CHECK_NEAR(bg->x, -120.0f, 0.001f);
    CHECK_NEAR(bg->y, 258.0f, 0.001f);
    bool sawRect = false;
    for (const auto& cmd : bg->commands) {
        if (cmd.kind == pix::DrawCommand::Rect) {
            sawRect = true;
            CHECK_NEAR(cmd.a, 0.0f, 0.001f);
            CHECK_NEAR(cmd.b, 0.0f, 0.001f);
            CHECK_NEAR(cmd.c, 240.0f, 0.001f);
            CHECK_NEAR(cmd.d, 14.0f, 0.001f);
        }
    }
    CHECK(sawRect);
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

    // Top bag counters: bandage=3, healthkit/soda/painkiller per inventory
    // (painkiller=1 behind the LOADOUT tap-to-use rows). Every pill stays on
    // screen: empty ones are light ghosts with a dimmed, centred "0".
    auto* bandage = static_cast<const pix::NullText*>(hud.topLootCountText(0));
    auto* pills = static_cast<const pix::NullText*>(hud.topLootCountText(3));
    auto* medkit = static_cast<const pix::NullText*>(hud.topLootCountText(1));
    CHECK(bandage && pills && medkit);
    CHECK_EQ(bandage->text, std::string("3"));
    CHECK(bandage->visible);
    CHECK_NEAR(bandage->alpha, 1.0f, 0.001f);
    CHECK_NEAR(bandage->fontSize, Hud::kTopLootFontSize, 0.001f);
    CHECK_EQ(hud.topLootType(0), std::string("bandage"));
    CHECK_EQ(pills->text, std::string("1"));
    CHECK(pills->visible);
    // healthkit/soda are 0 in the bag -> ghost pills, not hidden counters.
    CHECK(medkit->visible);
    CHECK_EQ(medkit->text, std::string("0"));
    CHECK_NEAR(medkit->alpha, Hud::kTopLootZeroTextAlpha, 0.001f);
    // The filled pill shows its loot icon (the host defs carry no atlas frame,
    // so only the empty pill's hidden icon is asserted here); the empty pill
    // has none.
    auto* bandageIcon = static_cast<const pix::NullSprite*>(hud.topLootIcon(0));
    auto* medkitIcon = static_cast<const pix::NullSprite*>(hud.topLootIcon(1));
    CHECK(bandageIcon != nullptr);
    CHECK(medkitIcon && !medkitIcon->visible);

    // Capsule backgrounds: a dark fill for the filled pill and a light ghost
    // fill for the empty one, both rounded (corner-arc polygon, no square).
    auto hasFill = [](const pix::Graphics* g, uint32_t color) {
        if (!g) {
            return false;
        }
        const auto* nullG = static_cast<const pix::NullGraphics*>(g);
        return std::any_of(nullG->commands.begin(), nullG->commands.end(),
            [color](const pix::DrawCommand& cmd) {
                return cmd.kind == pix::DrawCommand::BeginFill && cmd.color == color;
            });
    };
    CHECK(hasFill(hud.topLootBackground(0), 0x000000));
    CHECK(hasFill(hud.topLootBackground(1), 0xffffff));
    const auto* pillBg = static_cast<const pix::NullGraphics*>(hud.topLootBackground(0));
    CHECK(pillBg != nullptr);
    if (pillBg) {
        bool rounded = false;
        bool square = false;
        for (const auto& cmd : pillBg->commands) {
            if (cmd.kind == pix::DrawCommand::Polygon) {
                rounded = true;
            }
            if (cmd.kind == pix::DrawCommand::Rect) {
                square = true;
            }
        }
        CHECK(rounded);
        CHECK(!square);
    }

    // Only the filled bandage pill is tappable; the ghost pills are inert.
    // Slot 0 starts at the player's left edge (640 - 26) and is 80px wide.
    CHECK(hud.hitTest(654.0f, 692.0f).kind == Hud::TapKind::LootUse);
    CHECK_EQ(hud.hitTest(654.0f, 692.0f).slot, 0);
    CHECK(hud.hitTest(740.0f, 692.0f).kind == Hud::TapKind::None);

    // Scope strip: only owned scopes show; "4xscope" is owned and equipped.
    auto* scope4 = static_cast<const pix::NullText*>(hud.scopeSlotLabel(2));
    auto* scope1 = static_cast<const pix::NullText*>(hud.scopeSlotLabel(0));
    CHECK(scope4 && scope1);
    CHECK_EQ(scope4->text, std::string("4x"));
    CHECK(scope4->visible);
    CHECK_NEAR(scope4->fontSize, Hud::kScopeLabelFontSize, 0.001f);
    CHECK(!scope1->visible);
    CHECK_EQ(hud.scopeSlotType(2), std::string("4xscope"));
    const Hud::TapResult scopeTap = hud.hitTest(40.0f, 386.0f);
    CHECK(scopeTap.kind == Hud::TapKind::ScopeSlot);
    CHECK_EQ(scopeTap.slot, 2);

    // Weapon cards expose the live types for the long-press DropItemMsg.
    CHECK_EQ(hud.weaponType(0), std::string("ak47"));
    CHECK_EQ(hud.weaponType(1), std::string());
    CHECK_EQ(hud.weaponType(2), std::string("fists"));
    auto* slot0 = static_cast<const pix::NullText*>(hud.weaponSlotText(0));
    CHECK(slot0 && slot0->text == std::string("AK47"));
    // Card name/art are sized to the card height (18px in the 42px row).
    CHECK_NEAR(slot0->fontSize, Hud::kWeaponCardFontSize, 0.001f);
    CHECK(Hud::kWeaponCardIconSize <= Hud::kWeaponSlotHeight);

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
    // #ui-equipped-ammo is hidden with the result screen too.
    auto* clipText = static_cast<const pix::NullText*>(hud.ammoClipText());
    CHECK(clipText != nullptr && !clipText->visible);
}

// ui2.ts:896-910 / 1226-1246 (#ui-equipped-ammo): the equipped gun's clip and
// the bag count for its ammo type above the health bar, plus the bullet reload
// button. The web sizes come from game.css:2147-2195.
TEST(hud_equipped_ammo_matches_web_client) {
    installGeneratedDefs();
    Harness h;
    UpdateMsg u;
    u.activePlayerId = 42;
    u.activePlayerIdDirty = true;
    u.activePlayerData.inventoryDirty = true;
    u.activePlayerData.inventory.assign(defs::kBagSizeKeys.size(), 0);
    const int ammo762 = bagIndex("762mm");
    CHECK(ammo762 >= 0);
    if (ammo762 >= 0) {
        u.activePlayerData.inventory[static_cast<size_t>(ammo762)] = 90;
    }
    u.activePlayerData.weapsDirty = true;
    u.activePlayerData.weapons = {{"ak47", 15}, {"", 0}, {"fists", 0}, {"", 0}};
    u.activePlayerData.curWeapIdx = 0;
    u.ack = 1;
    h.push(MsgType_Update, u);

    pix::NullPixiFactory factory;
    pix::Container* root = factory.createContainer();
    Hud hud;
    hud.build(&factory, root, Vec2(1280.0f, 720.0f));
    hud.update(0.016f, h.game, true);

    auto* clip = static_cast<const pix::NullText*>(hud.ammoClipText());
    auto* reserve = static_cast<const pix::NullText*>(hud.ammoReserveText());
    auto* icon = static_cast<const pix::NullSprite*>(hud.ammoReloadIcon());
    auto* clipBox = static_cast<const pix::NullGraphics*>(hud.ammoClipBox());
    auto* reserveBox = static_cast<const pix::NullGraphics*>(hud.ammoReserveBox());
    CHECK(clip && reserve && icon && clipBox && reserveBox);
    if (!clip || !reserve || !icon || !clipBox || !reserveBox) {
        return;
    }
    CHECK_EQ(clip->text, std::string("15"));
    CHECK(clip->visible);
    CHECK_EQ(clip->fill, 0xffffffu);
    CHECK(clip->fontSize > 30.0f); // #ui-current-clip is the large bold box
    CHECK_EQ(reserve->text, std::string("90"));
    CHECK(reserve->visible);
    CHECK(icon->visible);
    CHECK_EQ(icon->frame, std::string("gui/bullets.png"));

    // ui2.ts:867-868: the equipped weapon card is highlighted. Slot 0 holds the
    // equipped ak47; the other cards keep the dim fill and no border.
    auto hasBorder = [](const pix::Graphics* g) {
        if (!g) {
            return false;
        }
        const auto* nullG = static_cast<const pix::NullGraphics*>(g);
        return std::any_of(nullG->commands.begin(), nullG->commands.end(),
            [](const pix::DrawCommand& cmd) { return cmd.kind == pix::DrawCommand::LineStyle; });
    };
    auto hasFill = [](const pix::Graphics* g, uint32_t color) {
        if (!g) {
            return false;
        }
        const auto* nullG = static_cast<const pix::NullGraphics*>(g);
        return std::any_of(nullG->commands.begin(), nullG->commands.end(),
            [color](const pix::DrawCommand& cmd) {
                return cmd.kind == pix::DrawCommand::BeginFill && cmd.color == color;
            });
    };
    CHECK(hasBorder(hud.weaponSlotBg(0)));
    CHECK(hasFill(hud.weaponSlotBg(0), 0xffffff));
    CHECK(!hasBorder(hud.weaponSlotBg(1)));
    CHECK(hasFill(hud.weaponSlotBg(2), 0x000000));

    // Swapping to the melee slot moves the highlight with curWeapIdx.
    u.activePlayerData.curWeapIdx = 2;
    h.push(MsgType_Update, u);
    hud.update(0.016f, h.game, true);
    CHECK(!hasBorder(hud.weaponSlotBg(0)));
    CHECK(hasBorder(hud.weaponSlotBg(2)));
    // Restore the gun for the ammo assertions below.
    u.activePlayerData.curWeapIdx = 0;

    // #ui-ammo-interactive pills (ui2.ts:1269-1277): 762mm has 90 rounds, the
    // 9mm row sits dimmed at 0, and the special 45acp row hides while empty.
    auto* row762 = static_cast<const pix::NullText*>(hud.ammoRowCountText(2));
    auto* row9mm = static_cast<const pix::NullText*>(hud.ammoRowCountText(1));
    auto* row45acp = static_cast<const pix::NullText*>(hud.ammoRowCountText(4));
    CHECK(row762 && row9mm && row45acp);
    if (row762 && row9mm && row45acp) {
        CHECK_EQ(row762->text, std::string("90"));
        CHECK(row762->visible);
        CHECK_NEAR(row762->alpha, 1.0f, 0.001f);
        CHECK_NEAR(row762->fontSize, Hud::kAmmoCountFontSize, 0.001f);
        CHECK_EQ(row9mm->text, std::string("0"));
        CHECK(row9mm->visible);
        CHECK_NEAR(row9mm->alpha, Hud::kAmmoZeroAlpha, 0.001f);
        CHECK(!row45acp->visible);
    }
    const auto* ammoBg = static_cast<const pix::NullGraphics*>(hud.ammoRowBackground(2));
    CHECK(ammoBg != nullptr);
    if (ammoBg) {
        bool rounded = false;
        bool square = false;
        for (const auto& cmd : ammoBg->commands) {
            if (cmd.kind == pix::DrawCommand::Polygon) {
                rounded = true;
            }
            if (cmd.kind == pix::DrawCommand::Rect) {
                square = true;
            }
        }
        CHECK(rounded);  // corner arcs
        CHECK(!square);  // no square panel
    }

    // game.css: #ui-current-clip is 100x50 centred on the screen; the reserve
    // box is 65x32 at left 82% / top 18px. Both sit 80px above the design
    // bottom (wrapper bottom 62 + 12 margin + 56 counter - 50 box).
    CHECK_NEAR(clipBox->x, -50.0f, 0.001f);
    CHECK_NEAR(clipBox->y, -280.0f, 0.001f);
    CHECK_NEAR(reserveBox->x, 56.0f, 0.001f);
    CHECK_NEAR(reserveBox->y, -280.0f, 0.001f);
    // #ui-reload-button-container (36x36 at left 212px / top 16px) is the
    // reload tap target: centred at x=782.5, y=96 from the bottom-left edge.
    CHECK(hud.hitTest(782.5f, 96.0f).kind == Hud::TapKind::Reload);

    // An empty clip still shows, in red (ui2.ts:1229).
    u.activePlayerData.weapons = {{"ak47", 0}, {"", 0}, {"fists", 0}, {"", 0}};
    u.activePlayerData.weapsDirty = true;
    h.push(MsgType_Update, u);
    hud.update(0.016f, h.game, true);
    CHECK_EQ(clip->text, std::string("0"));
    CHECK(clip->visible);
    CHECK_EQ(clip->fill, 0xff0000u);

    // Melee: #ui-current-clip is hidden and there is no reserve box/reload.
    u.activePlayerData.weapons = {{"ak47", 0}, {"", 0}, {"fists", 0}, {"", 0}};
    u.activePlayerData.curWeapIdx = 2;
    u.activePlayerData.weapsDirty = true;
    h.push(MsgType_Update, u);
    hud.update(0.016f, h.game, true);
    CHECK(!clip->visible);
    CHECK(!reserve->visible);
    CHECK(!icon->visible);

    // ammoInfinite (m9_cursed): the reserve is Number.MAX_VALUE in ui2.ts,
    // printed as ∞.
    u.activePlayerData.weapons = {{"m9_cursed", 10}, {"", 0}, {"fists", 0}, {"", 0}};
    u.activePlayerData.curWeapIdx = 0;
    u.activePlayerData.weapsDirty = true;
    h.push(MsgType_Update, u);
    hud.update(0.016f, h.game, true);
    CHECK(clip->visible);
    CHECK_EQ(clip->text, std::string("10"));
    CHECK(reserve->visible);
    CHECK_EQ(reserve->text, std::string("\xE2\x88\x9E"));
    CHECK(icon->visible);
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
    CHECK(specStats->text.find("YOUR RESULTS") != std::string::npos);
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
    // The final result screen is Java; the HUD routes no result taps.
    CHECK(hud.overlayActionAt(640.0f, 266.0f) == Hud::OverlayAction::None);
    CHECK(hud.overlayActionAt(520.0f, 266.0f) == Hud::OverlayAction::None);
}

// ui.ts showStats: a GameOverMsg while the match is still running (solo death
// or a wiped team) shows SPECTATE beside LEAVE. beginSpectating just queues
// SpectateAction.Begin (game.ts sends SpectateMsg); once the server moves
// activePlayerId the stats panel gives way to the spectating overlay
// (setSpectating -> hideStats). A running match's other-team result does not
// reopen the panel.
TEST(hud_gameover_spectate_matches_web_client) {
    Harness h;
    JoinedMsg joined;
    joined.teamMode = 1; // solo
    joined.playerId = 42;
    joined.started = true;
    h.push(MsgType_Joined, joined);

    UpdateMsg u;
    u.activePlayerId = 42;
    u.activePlayerIdDirty = true;
    PlayerInfo me;
    me.playerId = 42;
    me.teamId = 1;
    me.name = "Me";
    u.playerInfos.push_back(me);
    u.ack = 1;
    h.push(MsgType_Update, u);

    AliveCountsMsg alive;
    alive.teamAliveCounts = {4}; // other players still in the match
    h.push(MsgType_AliveCounts, alive);

    pix::NullPixiFactory factory;
    pix::Container* root = factory.createContainer();
    Hud hud;
    hud.build(&factory, root, Vec2(1280.0f, 720.0f));
    hud.update(0.016f, h.game, true);

    PlayerStatsMsg stats;
    stats.playerId = 42;
    stats.dead = true;
    stats.kills = 2;
    stats.damageDealt = 150;
    stats.damageTaken = 100;
    stats.timeAlive = 60;

    // Solo death before the match ends: the result panel keeps SPECTATE.
    GameOverMsg over;
    over.teamId = 0;
    over.teamRank = 5;
    over.gameOver = false;
    over.winningTeamId = 0;
    over.playerStats = {stats};
    h.push(MsgType_GameOver, over);
    hud.update(0.016f, h.game, true);
    CHECK(hud.gameOverShown());
    CHECK(hud.gameOverCanSpectate());
    // The Java GameOverDialog owns the result buttons; nothing routes here.
    CHECK(hud.overlayActionAt(520.0f, 266.0f) == Hud::OverlayAction::None);
    CHECK(hud.overlayActionAt(760.0f, 266.0f) == Hud::OverlayAction::None);

    // Game::gameOverJson is the payload the Java dialog renders (ui.ts
    // showStats): victory/defeat inputs, rank, team kills and the local card.
    std::string result = h.game.gameOverJson();
    // The Java side parses this with Gson; keep it strictly well-formed.
    ui::JsonValue parsed;
    CHECK(ui::JsonParser::parse(result, parsed));
    CHECK(parsed.isObject());
    CHECK(result.find("\"gameOver\":false") != std::string::npos);
    CHECK(result.find("\"victory\":false") != std::string::npos);
    CHECK(result.find("\"teamMode\":1") != std::string::npos);
    CHECK(result.find("\"teamRank\":5") != std::string::npos);
    CHECK(result.find("\"canSpectate\":true") != std::string::npos);
    CHECK(result.find("\"name\":\"Me\"") != std::string::npos);
    CHECK(result.find("\"kills\":2") != std::string::npos);
    CHECK(result.find("\"damageDealt\":150") != std::string::npos);
    CHECK(result.find("\"damageTaken\":100") != std::string::npos);
    CHECK(result.find("\"timeAlive\":60") != std::string::npos);

    // The server answers SpectateMsg Begin with a new active player: the
    // result panel closes and the spectating overlay takes over.
    UpdateMsg spec;
    spec.activePlayerId = 77;
    spec.activePlayerIdDirty = true;
    spec.activePlayerData.spectatorCountDirty = true;
    spec.activePlayerData.spectatorCount = 1;
    PlayerInfo other;
    other.playerId = 77;
    other.teamId = 2;
    other.name = "Killer";
    spec.playerInfos.push_back(other);
    spec.ack = 2;
    h.push(MsgType_Update, spec);
    hud.update(0.016f, h.game, true);
    CHECK(!hud.gameOverShown());
    CHECK(hud.spectating());
    auto* specLabel = static_cast<const pix::NullText*>(hud.spectateLabelText());
    CHECK(specLabel != nullptr);
    CHECK_EQ(specLabel->text, std::string("SPECTATING Killer"));

    // The settings gear stays tappable while spectating (hitTest is disabled).
    // Design space is y-up: the gear sits at CSS top 3 (design y 720 - 25).
    CHECK(hud.settingsTapHit(25.0f, 695.0f));
    CHECK(hud.hitTest(25.0f, 695.0f).kind == Hud::TapKind::None);

    // MATCH STATS shows the local result even though the panel is opened after
    // a GameOverMsg (the solo death path never enters the PlayerStats branch).
    hud.toggleSpectateStats();
    auto* specStats = static_cast<const pix::NullText*>(hud.spectateStatsText());
    CHECK(specStats != nullptr);
    CHECK_NEAR(specStats->fontSize, Hud::kSpecStatsFontSize, 0.001f);
    CHECK(specStats->text.find("YOUR RESULTS") != std::string::npos);
    CHECK(specStats->text.find("KILLS  2") != std::string::npos);
    CHECK(specStats->text.find("DAMAGE  150") != std::string::npos);
    CHECK(specStats->text.find("TAKEN  100") != std::string::npos);
    hud.toggleSpectateStats();

    // Spectated player's team dies while we keep watching: ui.ts showStats
    // suppresses another team's result in a running match.
    GameOverMsg otherOver;
    otherOver.teamId = 2;
    otherOver.teamRank = 3;
    otherOver.gameOver = false;
    otherOver.playerStats = {stats};
    h.push(MsgType_GameOver, otherOver);
    hud.update(0.016f, h.game, true);
    CHECK(!hud.gameOverShown());
    CHECK(hud.spectating());

    // Final result: SPECTATE is gone and LEAVE is centred again.
    otherOver.gameOver = true;
    otherOver.winningTeamId = 2;
    otherOver.teamRank = 3;
    h.push(MsgType_GameOver, otherOver);
    hud.update(0.016f, h.game, true);
    CHECK(hud.gameOverShown());
    CHECK(!hud.gameOverCanSpectate());
    CHECK(!hud.spectating());
    CHECK(hud.overlayActionAt(520.0f, 266.0f) == Hud::OverlayAction::None);
    CHECK(hud.overlayActionAt(640.0f, 266.0f) == Hud::OverlayAction::None);

    // Another team's final result while spectating: the title is "<name> won
    // the game." and SPECTATE is gone.
    result = h.game.gameOverJson();
    CHECK(result.find("\"victory\":true") != std::string::npos);
    CHECK(result.find("\"spectatingAnotherTeam\":true") != std::string::npos);
    CHECK(result.find("\"spectatedName\":\"Killer\"") != std::string::npos);
    CHECK(result.find("\"canSpectate\":false") != std::string::npos);
}

// loot.ts m_update touch branch: the pickup prompt reach is
// player.rad + loot.rad * touchLootRadMult, the same radius the server's
// Player.getClosestLoot uses for mobile players. The old prompt check
// (loot.rad + 2.5) showed the prompt out of pickup range.
TEST(touch_loot_reach_matches_web_and_server) {
    CHECK_NEAR(PlayerConfig::touchLootRadMult, 1.4f, 1e-6f);
    CHECK_NEAR(touchLootReach(1.0f, 1.0f), 2.4f, 1e-5f);
    CHECK_NEAR(touchLootReach(1.5f, 2.0f), 4.3f, 1e-5f);
    // 2.45 units away: out of reach now, but inside the old loot.rad + 2.5.
    CHECK(2.45f >= touchLootReach(1.0f, 1.0f));
    CHECK(2.45f < 1.0f + 2.5f);
}

TEST(hud_prompt_announcement_and_faction_counts) {
    Harness h;
    pix::NullPixiFactory factory;
    pix::Container* root = factory.createContainer();
    Hud hud;
    hud.build(&factory, root, Vec2(1280.0f, 720.0f));
    hud.update(0.016f, h.game, true);

    hud.setInteractionPrompt("PICK UP FRAG", Vec2(640.0f, 640.0f));
    auto* prompt = static_cast<const pix::NullText*>(hud.interactionPromptText());
    CHECK(prompt != nullptr);
    CHECK(prompt->visible);
    CHECK_EQ(prompt->text, std::string("PICK UP FRAG"));
    CHECK_EQ(prompt->fill, 0xffffffu); // white name text on the dark box
    // 24px fills the 52px prompt box (web #ui-interaction-description).
    CHECK_NEAR(prompt->fontSize, Hud::kPromptFontSize, 0.001f);
    // The item name gets its own box, right of the finger-press button and
    // vertically centred on it (ui2.ts #ui-interaction-outer).
    auto* nameBox = static_cast<const pix::NullGraphics*>(hud.interactionPromptTextBox());
    CHECK(nameBox != nullptr);
    if (nameBox) {
        CHECK(nameBox->visible);
        CHECK_NEAR(nameBox->x, 34.0f, 0.001f);  // 26 (half icon) + 8 gap
        CHECK_NEAR(nameBox->y, 254.0f, 0.001f); // cy 280 - 52/2 (same as icon)
        bool sawRect = false;
        for (const auto& cmd : nameBox->commands) {
            if (cmd.kind == pix::DrawCommand::Rect) {
                sawRect = true;
                // Same height as the finger-press box (Hud::kPromptSize).
                CHECK_NEAR(cmd.d, Hud::kPromptSize, 0.001f);
                CHECK(cmd.c > 100.0f);
            }
        }
        CHECK(sawRect);
    }
    // The finger-press button and the item name are one tap target beside the
    // loot (design position fed by GameScene).
    CHECK(hud.hitTest(640.0f, 640.0f).kind == Hud::TapKind::Interact);
    hud.setInteractionPrompt(std::string(), Vec2(640.0f, 640.0f));
    CHECK(!prompt->visible);
    if (nameBox) {
        CHECK(!nameBox->visible);
    }
    CHECK(hud.hitTest(640.0f, 640.0f).kind == Hud::TapKind::None);

    hud.showAnnouncement("YOU'VE BEEN PROMOTED TO COMMANDER!");
    auto* announcement = static_cast<const pix::NullText*>(hud.announcementText());
    CHECK(announcement != nullptr);
    CHECK(announcement->visible);
    CHECK_EQ(announcement->text, std::string("YOU'VE BEEN PROMOTED TO COMMANDER!"));
    hud.update(4.0f, h.game, true);
    CHECK(!announcement->visible);

    // Faction mode: two alive counts instead of one total.
    AliveCountsMsg alive;
    alive.teamAliveCounts = {7, 3};
    h.push(MsgType_AliveCounts, alive);
    hud.setFactionMode(true);
    hud.update(0.016f, h.game, true);
    auto* aliveText = static_cast<const pix::NullText*>(hud.aliveText());
    CHECK(aliveText != nullptr);
    CHECK_EQ(aliveText->text, std::string("7:3"));

    // JoinedMsg.started=false: waiting text stays visible even though the
    // normal HUD is gated off until the match is playable.
    hud.setWaitingForPlayers(true);
    hud.update(0.016f, h.game, false);
    auto* waiting = static_cast<const pix::NullText*>(hud.waitingText());
    CHECK(waiting != nullptr);
    CHECK(waiting->visible);
    CHECK_EQ(waiting->text, std::string("Waiting for players..."));
    hud.setWaitingForPlayers(false);
    CHECK(!waiting->visible);
}

TEST(pickup_error_callback_and_success_sound) {
    Harness h;
    int errorType = -1;
    std::string soundItem;
    h.game.setPickupErrorCallback([&](int type) { errorType = type; });
    h.game.setPickupSoundCallback([&](const std::string& item) { soundItem = item; });

    PickupMsg error;
    error.type = 0; // PickupMsgType.Full
    error.item = "frag";
    h.push(MsgType_Pickup, error);
    CHECK_EQ(errorType, 0);

    PickupMsg success;
    success.type = 4; // PickupMsgType.Success
    success.item = "mp5";
    h.push(MsgType_Pickup, success);
    CHECK_EQ(soundItem, std::string("mp5"));
    CHECK_EQ(errorType, 0); // success does not fire the error toast
}

TEST(hud_local_kill_feed_and_toast) {
    Harness h;
    JoinedMsg joined;
    joined.teamMode = 1;
    joined.playerId = 42;
    joined.started = true;
    h.push(MsgType_Joined, joined);

    UpdateMsg u;
    u.activePlayerId = 42;
    u.activePlayerIdDirty = true;
    u.activePlayerData.healthDirty = true;
    u.activePlayerData.health = 100.0f;
    u.ack = 1;
    h.push(MsgType_Update, u);

    pix::NullPixiFactory factory;
    pix::Container* root = factory.createContainer();
    Hud hud;
    hud.build(&factory, root, Vec2(1280.0f, 720.0f));
    hud.update(0.016f, h.game, true); // seed the kill counter at 0

    KillMsg kill;
    kill.damageType = DamageType_Player;
    kill.killerId = 42;
    kill.killCreditId = 42;
    kill.targetId = 99;
    kill.killed = true;
    h.push(MsgType_Kill, kill);
    hud.update(0.016f, h.game, true);

    CHECK_EQ(h.game.localKills(), 1);
    auto* row = static_cast<const pix::NullText*>(hud.killFeedRowText(0));
    CHECK(row != nullptr);
    CHECK(row->text.find("Player") != std::string::npos);
    auto* toast = static_cast<const pix::NullText*>(hud.pickupMessageText());
    CHECK(toast != nullptr);
    CHECK(toast->text.find("YOU KILLED") != std::string::npos);
}

TEST(hud_perk_role_menu_selects_and_confirms) {
    installGeneratedDefs();
    const DefProvider* defs = getDefProvider();
    const MapRenderDef* cobalt = defs->mapRender("cobalt");
    CHECK(cobalt != nullptr);
    CHECK(cobalt->perkMode);
    CHECK_EQ(cobalt->perkModeRoles.size(), static_cast<size_t>(6));
    CHECK_EQ(cobalt->perkModeRoles[0], std::string("scout"));
    const RoleRenderDef* healer = defs->role("healer");
    CHECK(healer != nullptr);
    CHECK_EQ(healer->guiImg, std::string("gui/role-healer.png"));
    CHECK_EQ(healer->perks.size(), static_cast<size_t>(2));
    CHECK_EQ(healer->perks[0], std::string("field_medic"));
    CHECK_EQ(healer->perkNames[0], std::string("Combat Medic"));
    CHECK(healer->hasColor);

    pix::NullPixiFactory factory;
    pix::Container* root = factory.createContainer();
    Hud hud;
    hud.build(&factory, root, Vec2(1280.0f, 720.0f));
    hud.setVisible(true);

    // ui.ts setRoleMenuOptions: the saved config role wins when it is in the
    // map's list, otherwise roles[0] ("scout") is displayed.
    hud.setRoleMenuOptions("healer", cobalt->perkModeRoles);
    CHECK_EQ(hud.roleMenuOptionCount(), 6);
    CHECK_EQ(hud.roleMenuDisplayed(), std::string("healer"));
    hud.setRoleMenuActive(true);
    CHECK(hud.roleMenuActive());

    auto* name = static_cast<const pix::NullText*>(hud.roleNameText());
    CHECK(name != nullptr);
    CHECK_EQ(name->text, std::string("Medic"));
    auto* perk0 = static_cast<const pix::NullText*>(hud.rolePerkNameText(0));
    CHECK(perk0 != nullptr);
    CHECK_EQ(perk0->text, std::string("Combat Medic"));
    auto* footer = static_cast<const pix::NullText*>(hud.roleFooterText());
    CHECK(footer != nullptr);
    CHECK_EQ(footer->text, std::string("ENTER GAME (20)"));

    // Class options are 160x160 at 20 px margins, centred in the header; the
    // panel is 0.8x at 1280x720 (web min(1, w/1200, h/900)).
    int option = -1;
    CHECK(hud.roleMenuActionAt(280.0f, 588.0f, option) == Hud::RoleTap::Option);
    CHECK_EQ(option, 0);
    hud.selectRoleOption(option);
    CHECK_EQ(hud.roleMenuDisplayed(), std::string("scout"));
    CHECK_EQ(static_cast<const pix::NullText*>(hud.roleNameText())->text, std::string("Scout"));

    // The 20 s timer (ui.ts roleMenuTicker) auto-confirms the displayed class.
    hud.tickRoleMenu(19.2f);
    CHECK_EQ(static_cast<const pix::NullText*>(hud.roleFooterText())->text,
             std::string("ENTER GAME (1)"));
    hud.tickRoleMenu(1.0f);
    CHECK(!hud.roleMenuActive());
    std::string selected;
    CHECK(hud.consumeRoleSelection(selected));
    CHECK_EQ(selected, std::string("scout"));
    CHECK(!hud.consumeRoleSelection(selected));

    // Confirm button (rw*0.12..rw*0.48, footer y) sends the displayed class.
    hud.setRoleMenuOptions("tank", cobalt->perkModeRoles);
    hud.setRoleMenuActive(true);
    CHECK_EQ(hud.roleMenuDisplayed(), std::string("tank"));
    CHECK(hud.roleMenuActionAt(928.0f, 100.0f, option) == Hud::RoleTap::Confirm);
    hud.confirmRoleSelection();
    CHECK(!hud.roleMenuActive());
    CHECK(hud.consumeRoleSelection(selected));
    CHECK_EQ(selected, std::string("tank"));

    // While the picker is up the gameplay action bar is modal.
    hud.setRoleMenuActive(true);
    CHECK(hud.hitTest(900.0f, 90.0f).kind == Hud::TapKind::None);
}
