// S1 host tests for the UI seam (ARCHITECTURE.md §4/§5.3/§7).
//
// `src/app/UiBridge.cpp` is Android-only (it owns the JNI symbols), so what is
// testable on the host is the contract Java depends on:
//   * the engine-free `UiBridge`/`UiHost` slots (`src/app/UiHost.cpp`),
//   * the JSON documents the bridge hands Java (config, flattened l10n, news,
//     site info) and their exact encoding,
//   * the C++ -> Java notifications the app raises through the host.
#include "TestFramework.h"

#include "app/UiHost.h"
#include "net/SiteInfo.h"
#include "ui/Config.h"
#include "ui/Json.h"
#include "ui/L10nJson.h"
#include "ui/Loadout.h"
#include "ui/Localization.h"
#include "ui/News.h"
#include "ui/NewsJson.h"
#include "ui/SiteInfoJson.h"

#include <map>
#include <string>
#include <vector>

using namespace surv;
using namespace surv_test;

namespace {

const char* kEnglishTable = R"({
    "index-play-solo": "Play Solo",
    "index-join-team": "Join Team",
    "index-play-solo-2": "Play Solo Too"
})";

// Records the C++ -> Java notifications (UiHost implementation).
class RecordingHost : public UiHost {
public:
    void showMenu() override { ++menuCalls; }
    void enterGame(int activePlayerId) override {
        lastActivePlayerId = activePlayerId;
        ++enterGameCalls;
    }
    void returnToMenu(const std::string& errorL10nKey) override {
        lastError = errorL10nKey;
        ++returnCalls;
    }
    void showResult() override { ++resultCalls; }

    int menuCalls = 0, enterGameCalls = 0, returnCalls = 0, resultCalls = 0;
    int lastActivePlayerId = -1;
    std::string lastError;
};

} // namespace

// ---------------------------------------------------------------------------
// The bridge/host slots never hand out a null pointer: the app may call them
// before any platform install, which is what keeps GameScene/MenuScene safe on
// a desktop build and in these tests.
TEST(bridge_slots_have_safe_defaults) {    setUiBridge(nullptr);
    setUiHost(nullptr);
    CHECK(uiBridge() != nullptr);
    CHECK(uiHost() != nullptr);
    CHECK_EQ(uiBridge()->configJson(), std::string("{}"));
    CHECK_EQ(uiBridge()->l10nJson(), std::string("{}"));
    CHECK(!uiBridge()->isInGame());
    // Commands on the null bridge are silent no-ops.
    uiBridge()->quickStart(1);
    uiBridge()->quitGame();
    uiHost()->enterGame(7);

    RecordingHost host;
    setUiHost(&host);
    CHECK(uiHost() == &host);
    uiHost()->enterGame(41);
    uiHost()->returnToMenu("index-invalid-token");
    uiHost()->showResult();
    uiHost()->showMenu();
    CHECK_EQ(host.enterGameCalls, 1);
    CHECK_EQ(host.lastActivePlayerId, 41);
    CHECK_EQ(host.returnCalls, 1);
    CHECK_EQ(host.lastError, std::string("index-invalid-token"));
    CHECK_EQ(host.resultCalls, 1);
    CHECK_EQ(host.menuCalls, 1);
    setUiHost(nullptr);

    MemoryUiBridge bridge;
    setUiBridge(&bridge);
    bridge.quickStart(4);
    bridge.joinUrls({"ws://127.0.0.1:9000/play", "ws://127.0.0.1:9001/play"}, "tok_abc");
    bridge.setAudio("music", 0.25f);
    CHECK_EQ(bridge.quickStartCalls, 1);
    CHECK_EQ(bridge.lastTeamMode, 4);
    CHECK_EQ(bridge.joinCalls, 1);
    CHECK_EQ(bridge.joinedUrls.size(), size_t(2));
    CHECK_EQ(bridge.joinedUrls[1], std::string("ws://127.0.0.1:9001/play"));
    CHECK_EQ(bridge.joinToken, std::string("tok_abc"));
    CHECK_EQ(bridge.setAudioCalls, 1);
    CHECK_EQ(bridge.audioKeys[0], std::string("music"));
    CHECK_NEAR(bridge.audioValue, 0.25f, 1e-6f);
    // Java UI cues (menu avatar swing/equip): name + channel reach the bridge
    // unchanged; the Android bridge queues them onto the GL thread.
    bridge.playSound("punch_swing_01", "sfx");
    bridge.playSound("clothes_pickup_01", "ui");
    CHECK_EQ(bridge.playSoundCalls, 2);
    CHECK_EQ(bridge.soundNames[0], std::string("punch_swing_01"));
    CHECK_EQ(bridge.soundChannels[0], std::string("sfx"));
    CHECK_EQ(bridge.soundNames[1], std::string("clothes_pickup_01"));
    CHECK_EQ(bridge.soundChannels[1], std::string("ui"));
    // Android gamepad axes arrive from AppActivity's MotionEvents.
    bridge.gamepadMotion(0.5f, -0.25f, 0.0f, 1.0f, 1.0f);
    CHECK_EQ(bridge.gamepadMotionCalls, 1);
    CHECK_NEAR(bridge.padMoveX, 0.5f, 1e-6f);
    CHECK_NEAR(bridge.padMoveY, -0.25f, 1e-6f);
    CHECK_NEAR(bridge.padAimY, 1.0f, 1e-6f);
    CHECK_NEAR(bridge.padFire, 1.0f, 1e-6f);
    setUiBridge(nullptr);
}

// ---------------------------------------------------------------------------
// The l10n document Java receives: one flat object, deterministic key order,
// and every key resolves like `Localization::translate` does.
TEST(bridge_l10n_json_matches_the_localization_table) {
    ui::Localization loc;
    CHECK(loc.registerEnglish(kEnglishTable));

    const std::string english = ui::l10nTableToJson(loc.flattened());
    // Keys are emitted in map (sorted) order, so the text is stable.
    CHECK_EQ(english, std::string("{\"index-join-team\":\"Join Team\","
                                  "\"index-play-solo\":\"Play Solo\","
                                  "\"index-play-solo-2\":\"Play Solo Too\"}"));

    ui::JsonValue parsed;
    CHECK(ui::JsonParser::parse(english, parsed));
    CHECK_EQ(parsed.getString("index-play-solo"), std::string("Play Solo"));
    CHECK_EQ(parsed.getString("index-play-solo"), loc.translate("index-play-solo"));
    // An empty table is still a valid object (Java must not choke on it).
    ui::Localization empty;
    CHECK_EQ(ui::l10nTableToJson(empty.flattened()), std::string("{}"));
}

// ---------------------------------------------------------------------------
// Config blob round-trip: what `SurvBridge.getConfigJson()` returns parses as
// an object and carries the values the Java screens read.
TEST(bridge_config_json_round_trips) {
    ui::MemoryConfigStorage storage;
    ui::Config config(&storage);
    config.load();
    config.setString("playerName", "Java Player");
    config.setFloat("musicVolume", 0.5f);

    const std::string json = ui::jsonToString(config.root());
    ui::JsonValue parsed;
    CHECK(ui::JsonParser::parse(json, parsed));
    CHECK(parsed.isObject());
    CHECK_EQ(parsed.getString("playerName"), std::string("Java Player"));
    CHECK_NEAR(parsed.getFloat("musicVolume"), 0.5f, 1e-6f);
    // The loadout travels with the config so Java can render the current
    // selection without a second call.
    const ui::JsonValue* loadout = parsed.get("loadout");
    CHECK(loadout != nullptr && loadout->isObject());
    CHECK_EQ(loadout->getString("outfit"), std::string("outfitBase"));
    CHECK(loadout->get("emotes") != nullptr);
    CHECK_EQ(loadout->get("emotes")->size(), size_t(6));

    // ... and the C++ side derives the join parcel from that same blob (this is
    // what AndroidUiBridge::joinUrls hands to GameScene).
    ui::Loadout selection(&config);
    CHECK_EQ(selection.joinInfo().name, std::string("Java Player"));
}

// ---------------------------------------------------------------------------
// News document: the Java menu renders this array, so its shape is part of the
// bridge contract.
TEST(bridge_news_json_shape) {
    const std::string json = ui::newsToJson(ui::newsEntries());
    ui::JsonValue parsed;
    CHECK(ui::JsonParser::parse(json, parsed));
    CHECK(parsed.isArray());
    CHECK(parsed.size() > 0);
    const ui::JsonValue* first = parsed.at(0);
    CHECK(first != nullptr && first->isObject());
    // Every entry carries the four fields the Java list needs.
    CHECK(first->get("date") != nullptr);
    CHECK(first->get("title") != nullptr);
    CHECK(first->get("href") != nullptr);
    const ui::JsonValue* paragraphs = first->get("paragraphs");
    CHECK(paragraphs != nullptr && paragraphs->isArray());
    // Styles are one of the three literals the Java side styles on.
    for (size_t i = 0; i < paragraphs->size(); ++i) {
        const std::string style = paragraphs->at(i)->getString("style");
        CHECK(style == "normal" || style == "highlight" || style == "redacted");
        CHECK(paragraphs->at(i)->get("text") != nullptr);
    }
}

// ---------------------------------------------------------------------------
// Site info snapshot: the exact document the Java region spinner reads.
TEST(bridge_site_info_json) {
    surv::SiteInfo info;
    // Stable key order is part of the contract (Java's Json.peekString scans the
    // flat object); assert the encoder produces keys in map order.
    const std::string emptyJson = ui::siteInfoToJson(info);
    CHECK_EQ(emptyJson, std::string("{\"captchaEnabled\":false,\"clientTheme\":\"main\","
                                    "\"country\":\"\",\"gitRevision\":\"\",\"modes\":[],"
                                    "\"ok\":false,\"pops\":[]}"));

    // Empty snapshot: every field present, both lists non-null.
    ui::JsonValue empty;
    CHECK(ui::JsonParser::parse(ui::siteInfoToJson(info), empty));
    CHECK(!empty.getBool("ok"));
    CHECK_EQ(empty.getString("clientTheme"), std::string("main"));
    CHECK_EQ(empty.get("modes")->size(), size_t(0));
    CHECK_EQ(empty.get("pops")->size(), size_t(0));

    info.ok = true;
    info.country = "SE";
    surv::GameModeInfo mode;
    mode.mapName = "main";
    mode.teamMode = 1;
    info.modes.push_back(mode);
    mode.teamMode = 2;
    info.modes.push_back(mode);
    surv::PopInfo pop;
    pop.region = "local";
    pop.playerCount = 3;
    pop.l10n = "index-local";
    info.pops.push_back(pop);

    ui::JsonValue parsed;
    CHECK(ui::JsonParser::parse(ui::siteInfoToJson(info), parsed));
    CHECK(parsed.getBool("ok"));
    CHECK_EQ(parsed.getString("country"), std::string("SE"));
    CHECK_EQ(parsed.get("modes")->size(), size_t(2));
    CHECK_EQ(parsed.get("modes")->at(1)->getInt("teamMode"), 2);
    CHECK_EQ(parsed.get("pops")->at(0)->getString("region"), std::string("local"));
    CHECK_EQ(parsed.get("pops")->at(0)->getInt("playerCount"), 3);
}
