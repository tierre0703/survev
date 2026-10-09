// Host tests for the M7 UI foundation: the JSON reader, the config blob
// (defaults + persistence), localization lookup, device layout, URL parsing and
// the team-menu protocol (decode + client messages).
#include "TestFramework.h"

#include "net/WsUrl.h"
#include "ui/Config.h"
#include "ui/Device.h"
#include "ui/Json.h"
#include "ui/Localization.h"
#include "ui/TeamMenu.h"
#include "ui/Loadout.h"
#include "ui/Layout.h"
#include "render/GeneratedDefs.h"

#include <deque>
#include <memory>
#include <string>
#include <vector>

using namespace surv;
using namespace surv_test;

namespace {

// Small English table standing in for client/src/en.json.
const char* kEnglish = R"({
    "index-play-solo": "Play Solo",
    "index-play-duo": "Play Duo",
    "index-enter-name-here": "Enter your name here",
    "game-fists": "Fists",
    "loadout-title-outfit": "Outfit Skin",
    "index-host-closed": "Host closed",
    "spaced-key": "Spaced"
})";

// In-memory Connection used to drive the team menu (same shape as the game's
// FakeConnection: events queued and delivered by pump()).
class FakeTeamSocket : public Connection {
public:
    explicit FakeTeamSocket(std::vector<std::string>* sentList) : sent(sentList) {}

    ConnectionState state() const override { return _state; }
    size_t bufferedAmount() const override { return 0; }
    void send(const uint8_t* data, size_t len) override {
        if (sent) {
            sent->emplace_back(reinterpret_cast<const char*>(data), len);
        }
    }
    void close(const std::string& reason) override {
        (void)reason;
        if (_state < ConnectionState::Closing) {
            _state = ConnectionState::Closing;
        }
    }

    void pump() override {
        if (!_openDelivered && _state == ConnectionState::Open) {
            _openDelivered = true;
            if (Connection::onOpen) {
                Connection::onOpen();
            }
        }
        while (!_incoming.empty()) {
            std::vector<uint8_t> frame = std::move(_incoming.front());
            _incoming.pop_front();
            if (Connection::onMessage) {
                Connection::onMessage(std::move(frame));
            }
        }
    }

    // Test drivers.
    void openNow() { _state = ConnectionState::Open; }
    void deliver(const std::string& text) {
        _incoming.emplace_back(text.begin(), text.end());
    }

    std::vector<std::string>* sent = nullptr;
    std::string url;

private:
    ConnectionState _state = ConnectionState::Connecting;
    std::deque<std::vector<uint8_t>> _incoming;
    bool _openDelivered = false;
};

} // namespace

// ---------------------------------------------------------------------------
TEST(ui_name_and_team_invite_helpers) {
    CHECK_EQ(ui::sanitizePlayerName("  A\tB\n C  "), std::string("AB C"));
    CHECK_EQ(ui::sanitizePlayerName("1234567890123456789"), std::string("1234567890123456"));
    CHECK_EQ(ui::sanitizePlayerName("123456789012345\xc3\xa9"), std::string("123456789012345"));
    CHECK_EQ(ui::teamEndpoint("https://example.com/api/"), std::string("wss://example.com/team_v2"));
    CHECK_EQ(ui::teamEndpoint("http://[::1]:8000/"), std::string("ws://[::1]:8000/team_v2"));
    CHECK_EQ(ui::teamInviteCode("https://example.com/#ABCD"), std::string("ABCD"));
    CHECK_EQ(ui::teamInviteCode(" #ABCD "), std::string("ABCD"));
}

TEST(ui_loadout_available_validation_and_persistence) {
    installGeneratedDefs();
    ui::MemoryConfigStorage storage;
    ui::Config config(&storage);
    config.load();
    ui::Loadout loadout(&config);
    CHECK_EQ(loadout.items("outfit").size(), size_t(1));
    CHECK_EQ(loadout.items("outfit")[0], std::string("outfitBase"));
    CHECK(loadout.items("emote").size() > 50);
    CHECK(!loadout.select("melee", "ak47"));
    CHECK(!loadout.select("melee", "katana_rusted"));
    loadout.setAccountItems({"katana_rusted", "outfitWoodland", "ak47", "katana_rusted"});
    CHECK_EQ(loadout.items("melee").size(), size_t(2));
    CHECK(loadout.select("melee", "katana_rusted"));
    CHECK(loadout.select("outfit", "outfitWoodland"));
    CHECK(loadout.select("emote", "emote_gg", 0));
    CHECK(!loadout.select("emote", "emote_gg", 100));
    CHECK(loadout.select("player_icon", "emote_gg"));
    config.setString("playerName", "  Native  ");
    const auto join = loadout.joinInfo();
    CHECK_EQ(join.name, std::string("Native"));
    CHECK_EQ(join.melee, std::string("katana_rusted"));
    CHECK_EQ(join.outfit, std::string("outfitWoodland"));
    CHECK_EQ(join.heal, std::string("heal_basic"));
    CHECK_EQ(join.boost, std::string("boost_basic"));
    CHECK_EQ(join.emotes[0], std::string("emote_gg"));
    ui::Config reloaded(&storage);
    reloaded.load();
    ui::Loadout restored(&reloaded);
    restored.setAccountItems({"katana_rusted", "outfitWoodland"});
    CHECK_EQ(restored.joinInfo().melee, join.melee);
    restored.setAccountItems({});
    CHECK_EQ(restored.joinInfo().melee, std::string("fists"));
    CHECK_EQ(restored.joinInfo().outfit, std::string("outfitBase"));
}

TEST(ui_json_parses_values) {
    const std::string text = R"({
        "s": "hello\nworld",
        "n": -12.5,
        "i": 7,
        "b": true,
        "z": null,
        "arr": [1, "two", false],
        "obj": { "nested": { "deep": "value" } },
        "unicode": "\u00e9\u65e5"
    })";
    ui::JsonValue root;
    CHECK(ui::JsonParser::parse(text, root));
    CHECK(root.isObject());

    CHECK_EQ(root.getString("s"), std::string("hello\nworld"));
    CHECK_NEAR(root.getFloat("n"), -12.5f, 1e-6f);
    CHECK_EQ(root.getInt("i"), 7);
    CHECK(root.getBool("b"));
    CHECK(root.get("z") != nullptr && root.get("z")->isNull());
    CHECK_EQ(root.getString("missing", "fallback"), std::string("fallback"));

    const ui::JsonValue* arr = root.get("arr");
    CHECK(arr != nullptr);
    CHECK_EQ(arr->size(), 3u);
    CHECK_EQ(arr->at(1)->asString(), std::string("two"));
    CHECK_EQ(root.getString("unicode").size() >= 4u, true);

    const ui::JsonValue* nested = root.get("obj")->get("nested");
    CHECK(nested != nullptr);
    CHECK_EQ(nested->getString("deep"), std::string("value"));
}

TEST(ui_json_rejects_malformed) {
    ui::JsonValue out;
    CHECK(!ui::JsonParser::parse("{", out));
    CHECK(!ui::JsonParser::parse("{\"a\":}", out));
    CHECK(!ui::JsonParser::parse("[\"unterminated]", out));
}

// ---------------------------------------------------------------------------
TEST(ui_config_defaults_and_persistence) {
    ui::MemoryConfigStorage storage;
    ui::Config config(&storage);
    config.load();

    // Web defaults.    CHECK_EQ(config.getString("touchMoveStyle"), std::string("anywhere"));
    CHECK_EQ(config.getString("region"), std::string("na"));
    CHECK_EQ(config.getString("playerName"), std::string(""));
    CHECK_NEAR(config.getFloat("masterVolume"), 1.0f, 1e-6f);
    CHECK(config.getBool("touchAimLine"));
    CHECK_EQ(config.getInt("gameModeIdx"), 2);
    CHECK(config.getBool("teamAutoFill"));

// The default loadout matches GameConfig.defaultEmoteLoadout (5 slots: the
// EmoteSlot enum only has 5 entries).
    // The default loadout matches GameConfig.defaultEmoteLoadout.
    const ui::JsonValue* loadout = config.getJson("loadout");
    CHECK(loadout != nullptr && loadout->isObject());
    CHECK_EQ(loadout->getString("outfit"), std::string("outfitBase"));
    CHECK_EQ(loadout->getString("melee"), std::string("fists"));
    CHECK_EQ(loadout->getString("heal"), std::string("heal_basic"));
    CHECK_EQ(loadout->getString("boost"), std::string("boost_basic"));
    CHECK(loadout->get("emotes") != nullptr);
    CHECK_EQ(loadout->get("emotes")->size(), 5u);
    CHECK_EQ(loadout->get("emotes")->at(0)->asString(), std::string("emote_happyface"));
    CHECK_EQ(loadout->get("emotes")->at(4)->asString(), std::string(""));
    CHECK_EQ(loadout->get("crosshair")->getString("type"), std::string("crosshair_default"));

    config.setString("playerName", "Tester");
    config.setBool("muteAudio", true);
    CHECK_EQ(storage.value.find("\"playerName\":\"Tester\"") != std::string::npos, true);

    // Reload from the same storage.
    ui::Config reloaded(&storage);
    reloaded.load();
    CHECK_EQ(reloaded.getString("playerName"), std::string("Tester"));
    CHECK(reloaded.getBool("muteAudio"));
    // Untouched keys keep their defaults.
    CHECK_EQ(reloaded.getString("touchAimStyle"), std::string("anywhere"));
}

// ---------------------------------------------------------------------------
TEST(ui_localization_lookup) {
    ui::Localization loc;
    CHECK(loc.registerEnglish(kEnglish));

    CHECK_EQ(loc.translate("index-play-solo"), std::string("Play Solo"));
    CHECK_EQ(loc.translate("game-fists"), std::string("Fists"));
    CHECK_EQ(loc.translate("does-not-exist"), std::string(""));

    // Unknown locales fall back to English.
    loc.setLocale("xx-not-a-locale");
    CHECK_EQ(loc.getLocale(), std::string("en"));
    CHECK_EQ(loc.translate("index-play-solo"), std::string("Play Solo"));

    // A registered locale overrides English.
    loc.setLoader([](const std::string&) {
        return std::string(R"({"index-play-solo": "Jouer Solo"})");
    });
    loc.setLocale("fr");
    CHECK_EQ(loc.getLocale(), std::string("fr"));
    CHECK_EQ(loc.translate("index-play-solo"), std::string("Jouer Solo"));
    // Missing keys fall back to English.
    CHECK_EQ(loc.translate("game-fists"), std::string("Fists"));

    CHECK_EQ(loc.localeCount(), 18);
    CHECK_EQ(loc.localeName("de"), std::string("Deutsch"));
    CHECK_EQ(loc.detectLocale("zh-cn"), std::string("zh-cn"));
    CHECK_EQ(loc.detectLocale("pt-BR"), std::string("pt"));
    CHECK_EQ(loc.detectLocale("fr-CA"), std::string("fr"));
    CHECK_EQ(loc.detectLocale("ru"), std::string("ru"));
    CHECK_EQ(loc.detectLocale("xx"), std::string("en"));
}

// ---------------------------------------------------------------------------
TEST(ui_device_layout) {
    ui::Device& device = ui::Device::get();
    device.resize(1280.0f, 720.0f);
    CHECK(device.isLandscape);
    CHECK(device.isSmallLayout()); // phones always use the small layout
    device.resize(1280.0f, 800.0f);
    CHECK(device.isLandscape);
}

// ---------------------------------------------------------------------------
TEST(ui_ws_url_parsing) {
    WebSocketEndpoint ep;
    CHECK(parseWebSocketUrl("ws://127.0.0.1:9000/play", ep));
    CHECK_EQ(ep.scheme, std::string("ws"));
    CHECK_EQ(ep.host, std::string("127.0.0.1"));
    CHECK_EQ(ep.port, static_cast<uint16_t>(9000));
    CHECK_EQ(ep.requestPath(), std::string("/play"));
    CHECK_EQ(ep.socketUrl(), std::string("ws://127.0.0.1:9000"));

    CHECK(parseWebSocketUrl("wss://survev.io/team_v2", ep));
    CHECK_EQ(ep.secure, true);
    CHECK_EQ(ep.port, static_cast<uint16_t>(443));
    CHECK_EQ(ep.requestPath(), std::string("/team_v2"));

    CHECK(parseWebSocketUrl("ws://example.com", ep));
    CHECK_EQ(ep.port, static_cast<uint16_t>(80));
    CHECK_EQ(ep.requestPath(), std::string("/"));

    CHECK(!parseWebSocketUrl("", ep));
    CHECK(!parseWebSocketUrl("ws://", ep));
}

// ---------------------------------------------------------------------------
TEST(ui_team_menu_protocol) {
    std::vector<std::string> sent;
    FakeTeamSocket* socketPtr = nullptr;
    ui::TeamMenu menu;
    menu.setUrl("ws://127.0.0.1:9000/team_v2");
    menu.setFactory([&](const std::string& url) -> std::unique_ptr<Connection> {
        auto socket = std::make_unique<FakeTeamSocket>(&sent);
        socket->url = url;
        socketPtr = socket.get();
        return socket;
    });

    int roomChanges = 0;
    menu.onRoomChanged = [&] { roomChanges++; };
    std::vector<ui::TeamMenu::MatchData> matches;
    menu.onPlay = [&](const ui::TeamMenu::MatchData& m) { matches.push_back(m); };
    ui::TeamErrorType lastError = ui::TeamErrorType::Unknown;
    menu.onError = [&](ui::TeamErrorType e, const std::string&) { lastError = e; };

    menu.setPlayerName("Tester");
    CHECK_EQ(menu.roomData().region, std::string("na"));
    menu.connect(true, "");
    CHECK(menu.isActive());
    CHECK(socketPtr != nullptr);
    CHECK_EQ(socketPtr->url, std::string("ws://127.0.0.1:9000/team_v2"));

    // Opening sends the `create` message with the room + player data.
    socketPtr->openNow();
    menu.update(0.0f);
    CHECK_EQ(sent.size(), 1u);
    CHECK(sent[0].find("\"type\":\"create\"") != std::string::npos);
    CHECK(sent[0].find("\"name\":\"Tester\"") != std::string::npos);

    // A state message updates the roster and marks us leader.
    menu.handleMessage(R"({
        "type": "state",
        "data": {
            "localPlayerId": 7,
            "room": { "roomUrl": "abc123", "region": "eu", "gameModeIdx": 1, "autoFill": false,
                      "findingGame": false, "maxPlayers": 4, "captchaEnabled": false,
                      "enabledGameModeIdxs": [0, 1] },
            "players": [
                { "playerId": 7, "name": "Tester", "isLeader": true, "inGame": false },
                { "playerId": 8, "name": "Friend", "isLeader": false, "inGame": true }
            ]
        }
    })");
    CHECK(menu.isJoined());
    CHECK(menu.isLeader());
    CHECK_EQ(menu.localPlayerId(), static_cast<uint16_t>(7));
    CHECK_EQ(menu.players().size(), 2u);
    CHECK_EQ(menu.players()[1].name, std::string("Friend"));
    CHECK_EQ(menu.roomUrl(), std::string("abc123"));
    // The leader's local region/autoFill win over the server state.
    CHECK_EQ(menu.roomData().region, std::string("na"));
    CHECK(roomChanges >= 1);

    // Room property changes are broadcast to the room.
    sent.clear();
    menu.setRoomRegion("eu");
    CHECK_EQ(sent.size(), 1u);
    CHECK(sent[0].find("\"type\":\"setRoomProps\"") != std::string::npos);
    CHECK(sent[0].find("\"region\":\"eu\"") != std::string::npos);

    // Starting a game sends playGame and marks the room as finding.
    sent.clear();
    menu.tryStartGame();
    CHECK(menu.isFindingGame());
    CHECK_EQ(sent.size(), 1u);
    CHECK(sent[0].find("\"type\":\"playGame\"") != std::string::npos);

    // joinGame hands the match data to the app.
    menu.handleMessage(
        R"({"type":"joinGame","data":{"urls":["ws://127.0.0.1:9000/play"],"joinToken":"tok"}})");
    CHECK_EQ(matches.size(), 1u);
    CHECK_EQ(matches[0].urls.size(), 1u);
    CHECK_EQ(matches[0].joinToken, std::string("tok"));

    // An error message leaves the room with the mapped reason.
    menu.handleMessage(R"({"type":"error","data":{"type":"join_full"}})");
    CHECK(!menu.isActive());
    CHECK(lastError == ui::TeamErrorType::JoinFull);
    CHECK_EQ(std::string(ui::teamErrorL10n(lastError)), std::string("index-team-is-full"));
}

TEST(ui_team_menu_keepalive) {
    std::vector<std::string> sent;
    FakeTeamSocket* socketPtr = nullptr;
    ui::TeamMenu menu;
    menu.setUrl("/team_v2");
    menu.setFactory([&](const std::string&) -> std::unique_ptr<Connection> {
        auto socket = std::make_unique<FakeTeamSocket>(&sent);
        socketPtr = socket.get();
        return socket;
    });
    menu.connect(false, "joinme");
    socketPtr->openNow();
    menu.update(0.0f);
    menu.handleMessage(R"({"type":"state","data":{"localPlayerId":1,
        "room":{"roomUrl":"joinme","region":"na","gameModeIdx":0,"autoFill":true},
        "players":[{"playerId":1,"name":"n","isLeader":false,"inGame":false}]}})");
    CHECK(menu.isJoined());

    // The keep-alive fires every 10s.
    for (int i = 0; i < 20; i++) {
        menu.update(0.5f);
    }
    bool sawKeepAlive = false;
    for (const auto& s : sent) {
        if (s.find("\"keepAlive\"") != std::string::npos) {
            sawKeepAlive = true;
        }
    }
    CHECK(sawKeepAlive);
}

// ---------------------------------------------------------------------------
// M7: shared design-space root + overlay tag.
TEST(ui_design_root_tag) {
    // The menu tags its scaled 1280x720 root with `kUiRootTag`; the in-game
    // overlay looks the tag up to adopt the same coordinate system.
    CHECK_EQ(ui::kUiRootTag, 0x5A17);
}

// The web layout constants the native chrome is authored against; guards against
// accidental drift from the CSS (`client/css/app.css`).
TEST(ui_layout_constants) {
    CHECK_EQ(ui::kit::kDesignWidth, 1280.0f);
    CHECK_EQ(ui::kit::kDesignHeight, 720.0f);
    // kit::fromCssY flips the web's y-down space into axmol's y-up space.
    CHECK_EQ(ui::kit::fromCssY(0.0f), 720.0f);
    CHECK_EQ(ui::kit::fromCssY(720.0f), 0.0f);
    CHECK_EQ(ui::kit::fromCssY(100.0f, 400.0f), 300.0f);
    const auto p = ui::kit::fromCss(40.0f, 60.0f);
    CHECK_EQ(p.x, 40.0f);
    CHECK_EQ(p.y, 660.0f);
}

// CSS colour parsing used for the web `.btn-*` palettes. Compiled without the
// engine (the parser only needs `parseCss`/`parseHexColor`, both engine-free).
TEST(ui_css_colors) {
    const ui::Rgb green = ui::parseHexColorRgb("#83af50");
    CHECK_EQ(int(green.r), 0x83);
    CHECK_EQ(int(green.g), 0xaf);
    CHECK_EQ(int(green.b), 0x50);

    // Short form expands per-channel.
    const ui::Rgb white = ui::parseHexColorRgb("#fff");
    CHECK_EQ(int(white.r), 255);
    CHECK_EQ(int(white.g), 255);
    CHECK_EQ(int(white.b), 255);

    const ui::Rgba half = ui::parseHexColorRgba("rgba(0,0,0,0.5)");
    CHECK_EQ(int(half.r), 0);
    CHECK_EQ(int(half.a), 127);

    // An unparsable string yields the caller's fallback.
    const ui::Rgb fallback{1, 2, 3};
    const ui::Rgb bad = ui::parseHexColorRgb("not-a-color", fallback);
    CHECK_EQ(int(bad.r), 1);
    CHECK_EQ(int(bad.g), 2);
    CHECK_EQ(int(bad.b), 3);
}
