#pragma once
// The engine-free interfaces behind the axmol / Android-Java UI seam
// (ARCHITECTURE.md §4.3 and §5.1). Everything the game scene needs from "the
// UI" is expressed here in plain C++ so
//
//   * `GameScene`/`MenuScene` can be host-tested against a fake host, and
//   * the Android-only bridge (`src/app/UiBridge.cpp`) is the single
//     translation unit that knows about JNI.
//
// Naming follows ARCHITECTURE.md: `UiHost` is the C++ side of the bridge (the
// game tells Java what happened), `UiBridge` is the Java-facing data facade
// (Java asks C++ for the config/l10n/loadout blobs and pushes edits back).
#ifndef SURV_UI_BRIDGE
#define SURV_UI_BRIDGE 1
#endif

#include <map>
#include <string>
#include <vector>

namespace surv {

class MenuScene;
class GameScene;

// Java -> C++ data facade, implemented by `UiBridge.cpp` on Android and by
// `NullUiBridge` everywhere else (host tests link a `MemoryUiBridge`). All
// methods are main-thread only: the Java side must marshal onto the GL thread
// before calling anything that touches engine state (ARCHITECTURE.md §4.2).
class UiBridge {
public:
    virtual ~UiBridge() = default;

    // --- config blob (SurvConfig.java reads the same SharedPreferences) -----
    virtual std::string configJson() = 0;
    virtual void setPlayerName(const std::string& name) = 0;
    virtual void setRegion(const std::string& region) = 0;
    // Re-validates `json` through ui::Loadout and persists the result. Returns
    // false when the document is malformed (Java then keeps its own copy).
    virtual bool setLoadoutJson(const std::string& json) = 0;
    // key is one of "master"/"sound"/"music"/"mute" (audio.ts volume tiers).
    virtual void setAudio(const std::string& key, float value) = 0;
    virtual void setLanguage(const std::string& locale) = 0;
    // Generic config write from the Java settings screens (sensitivity sliders,
    // renderScale, ...). Values arrive as strings; numbers/bools keep their JSON
    // type so the C++ config reads them back with getFloat/getBool.
    virtual void setConfigValue(const std::string& key, const std::string& value) {
        (void)key;
        (void)value;
    }
    // Engine stats overlay (fps/draw calls), persisted as `showFps`.
    virtual void setShowFps(bool show) { (void)show; }
    // GameConfig.protocolVersion (shared/gameConfig.ts) for find_game/playGame.
    virtual int protocolVersion() { return 0; }

    // --- read-only snapshots ------------------------------------------------
    // Flattened table for the current locale ({"key": "text"}).
    virtual std::string l10nJson() = 0;
    // Active language code (config `language`): Java uses it to invalidate its
    // cached l10n table when the settings screen changes it.
    virtual std::string language() { return "en"; }
    // Language list for the settings dropdown: [{"code","name"}, ...].
    virtual std::string localesJson() { return "[]"; }
    virtual std::string loadoutJson() = 0;
    // Owned loadout items with shared-def metadata (type/category/name/lore/
    // sprite/rarity) for the Java customize screen: a JSON array.
    virtual std::string loadoutItemsJson() { return "[]"; }
    // Item source def display names ({"type","name"}) for the detail panel's
    // "Acquired: <source>" line (loadoutMenu.ts getItemSourceName).
    virtual std::string loadoutSourcesJson() { return "[]"; }
    // Persists the account's owned item types (from /api/user profile `items`)
    // so ui::Loadout keeps non-default unlocks; JSON array of strings.
    virtual bool setAccountItemsJson(const std::string& json) {
        (void)json;
        return false;
    }
    // /api/site_info is fetched by the C++ menu today; Java reads the last
    // response through this (empty until the first fetch responds).
    virtual std::string siteInfoJson() = 0;
    virtual bool isInGame() = 0;

    // --- lifecycle ----------------------------------------------------------
    // "Play Solo/Duo/Squad" from the Java menu: discover a match and join.
    virtual void quickStart(int teamMode) = 0;
    // Direct join with the URLs/join token the Java HTTP layer discovered.
    virtual void joinUrls(const std::vector<std::string>& urls, const std::string& joinToken) = 0;
    virtual void cancelPending() = 0;
    virtual void quitGame() = 0;
    virtual void pauseGame() = 0;
    virtual void resumeGame() = 0;
    // SpectateMsg action from the Java pause dialog: 1=Begin, 2=Next, 3=Prev.
    virtual void spectate(int action) { (void)action; }
    // Java pause dialog: build the live leaderboard snapshot and show it.
    virtual void requestLeaderboard() {}
    // BT-keyboard key event forwarded by AppActivity with an Android
    // KeyEvent.KEYCODE_* value. Routed to the active GameScene's HardwareInput
    // on the game thread (P1.6); axmol's Android GLSurfaceView only forwards
    // DPAD/BACK, so letters/digits need this path.
    virtual void hardwareKey(int androidKeyCode, bool down) {
        (void)androidKeyCode;
        (void)down;
    }
    // Android gamepad axes forwarded by AppActivity's MotionEvents (native-only
    // mapping): left/right stick and fire trigger, -1..1 with +Y up.
    virtual void gamepadMotion(float moveX, float moveY, float aimX, float aimY, float fire) {
        (void)moveX;
        (void)moveY;
        (void)aimX;
        (void)aimY;
        (void)fire;
    }
    // Persist the config blob (Java calls this before backgrounding).
    virtual void saveConfig() {}
    // The bundled news entries as JSON (the Java menu's news column).
    virtual std::string newsJson() { return "[]"; }

    // M8 error reporting (client/src/errorLogs.ts): a non-fatal report from the
    // Java half. `loc` is "windowOnError" for worker-thread crashes, otherwise
    // the error name; `dataJson` is the payload. Budgeted by net/ErrorLog.
    virtual void reportError(const std::string& loc, const std::string& dataJson) {
        (void)loc;
        (void)dataJson;
    }

    // One-shot UI sound from the Java screens (menu avatar cues, button clicks).
    // `name` is a soundDefs.ts sound key, `channel` one of the soundDefs channel
    // names ("ui", "sfx", ...). Unknown keys/channels are silently dropped, and
    // the user's mute/volume settings still apply, so Java never needs to know
    // whether audio is available.
    virtual void playSound(const std::string& name, const std::string& channel) {
        (void)name;
        (void)channel;
    }

    // --- key/mouse binds (client/src/inputBinds.ts) -------------------------
    // The Java Key Binds screen lists every BindDef and edits the table:
    //   bindsJson()  [{"action","name","label","type","code"}, ...]
    //   bindsCode()  the share/import code (base64 of toArray(), CRC-checked)
    //   setBind()    capture result: action id + input.ts InputType + code. For
    //                InputType::Key the code is an Android KeyEvent.KEYCODE_*
    //                value (the C++ side maps it to an input.ts Key code).
    //   resetBinds() restore the inputBinds.ts defaults
    //   setBindsCode() import a shared code; false when the CRC/format is bad
    // Every edit is persisted to the `binds` config key and applied to the live
    // hardware input immediately.
    virtual std::string bindsJson() { return "[]"; }
    virtual std::string bindsCode() { return ""; }
    virtual bool setBind(int action, int type, int code) {
        (void)action;
        (void)type;
        (void)code;
        return false;
    }
    virtual void resetBinds() {}
    virtual bool setBindsCode(const std::string& code) {
        (void)code;
        return false;
    }
};

// The C++ side of the bridge: the app installs one host at startup and the
// bridge implementation forwards to it (ARCHITECTURE.md §4.3).
class UiHost {
public:
    virtual ~UiHost() = default;
    // Java: the menu fragment is built and visible.
    virtual void showMenu() = 0;
    // Java: a match is running (the Java UI hides the menu fragments).
    virtual void enterGame(int activePlayerId) = 0;
    // Java: the match ended or failed; `errorL10nKey` is empty on a clean exit.
    virtual void returnToMenu(const std::string& errorL10nKey) = 0;
    // Java: the active player died (result panel / spectate prompt).
    virtual void showResult() = 0;
    // Java: every region probe finished; `json` is PingTest::resultsJson()
    // ({"complete","bestRegion","tests":[{"region","zone","pingMs",...}]}).
    virtual void pingResults(const std::string& json) { (void)json; }
    // Java: open/close the in-game pause dialog. Returns false when the host has
    // no such UI (desktop), so the caller can fall back to leaving the match.
    virtual bool togglePause() { return false; }
    // Java: the hardware BACK key reached the engine (axmol's GL surface
    // consumes it and returns true, so AppActivity.onKeyDown never sees it)
    // while a Java menu modal may be open: close the top one
    // (JavaUiHost.handleBackPressed). No-op on hosts without Java screens.
    virtual void handleBack() {}
    // Java: live in-game leaderboard snapshot (Game::leaderboardJson).
    virtual void leaderboard(const std::string& json) { (void)json; }
    // Config access for in-game state that the web persists (game.ts
    // `perkModeRole`): the C++ menu/bridge owns the blob, so the scene reads
    // and writes through the host instead of keeping a second Config.
    virtual std::string configString(const std::string& key) {
        (void)key;
        return {};
    }
    virtual void setConfigValue(const std::string& key, const std::string& value) {
        (void)key;
        (void)value;
    }
};

// No-op host: used by the host tests and by any build without Java.
class NullUiHost : public UiHost {
public:
    void showMenu() override {}
    void enterGame(int) override {}
    void returnToMenu(const std::string&) override {}
    void showResult() override {}
};

// The process-wide bridge/host slots. `setUiBridge(nullptr)` installs the
// no-op bridge so call sites never have to null-check.
UiBridge* uiBridge();
void setUiBridge(UiBridge* bridge);
UiHost* uiHost();
void setUiHost(UiHost* host);

// NullUiBridge answers queries from empty data and drops every command; it is
// what the host tests and the desktop build get unless they install their own.
class NullUiBridge : public UiBridge {
public:
    std::string configJson() override { return "{}"; }
    void setPlayerName(const std::string&) override {}
    void setRegion(const std::string&) override {}
    bool setLoadoutJson(const std::string&) override { return false; }
    void setAudio(const std::string&, float) override {}
    void setLanguage(const std::string&) override {}
    std::string l10nJson() override { return "{}"; }
    std::string loadoutJson() override { return "{}"; }
    std::string siteInfoJson() override { return ""; }
    bool isInGame() override { return false; }
    void quickStart(int) override {}
    void joinUrls(const std::vector<std::string>&, const std::string&) override {}
    void cancelPending() override {}
    void quitGame() override {}
    void pauseGame() override {}
    void resumeGame() override {}
    void saveConfig() override {}
};

// Installs the platform bridge + host (Android: `UiBridge.cpp`; everywhere
// else: a no-op defined in `UiHost.cpp`). Returns nothing and must be called
// once from `AppDelegate::applicationDidFinishLaunching`, after the scenes
// exist (the Android bridge reads them).
void installUiBridge(MenuScene* menu, GameScene* game);

// A recording bridge for the host tests: counts every command and answers
// queries from the values it was handed.
class MemoryUiBridge : public NullUiBridge {
public:
    void setConfigJson(std::string json) { _config = std::move(json); }
    void setL10nJson(std::string json) { _l10n = std::move(json); }
    void setLoadoutResult(bool ok) { _loadoutOk = ok; }

    std::string configJson() override { return _config; }
    std::string l10nJson() override { return _l10n; }
    bool setLoadoutJson(const std::string& json) override {
        loadoutJsonText = json;
        ++setLoadoutCalls;
        return _loadoutOk;
    }
    void setPlayerName(const std::string& name) override {
        playerName = name;
        ++setPlayerNameCalls;
    }
    void setRegion(const std::string& regionName) override {
        region = regionName;
        ++setRegionCalls;
    }
    void setAudio(const std::string& key, float value) override {
        audioKeys.push_back(key);
        audioValue = value;
        ++setAudioCalls;
    }
    void setLanguage(const std::string& locale) override {
        language = locale;
        ++setLanguageCalls;
    }
    void quickStart(int teamMode) override {
        lastTeamMode = teamMode;
        ++quickStartCalls;
    }
    void joinUrls(const std::vector<std::string>& urls, const std::string& token) override {
        joinedUrls = urls;
        joinToken = token;
        ++joinCalls;
    }
    void cancelPending() override { ++cancelCalls; }
    void quitGame() override { ++quitCalls; }
    void pauseGame() override { ++pauseCalls; }
    void resumeGame() override { ++resumeCalls; }
    void saveConfig() override { ++saveConfigCalls; }
    void playSound(const std::string& soundName, const std::string& soundChannel) override {
        soundNames.push_back(soundName);
        soundChannels.push_back(soundChannel);
        ++playSoundCalls;
    }
    void gamepadMotion(float moveX, float moveY, float aimX, float aimY, float fire) override {
        padMoveX = moveX;
        padMoveY = moveY;
        padAimX = aimX;
        padAimY = aimY;
        padFire = fire;
        ++gamepadMotionCalls;
    }
    std::string newsJson() override { return news; }

    std::string playerName, region, language, news = "[]";
    std::string loadoutJsonText, joinToken;
    std::vector<std::string> joinedUrls, audioKeys;
    std::vector<std::string> soundNames, soundChannels;
    float audioValue = 0.0f;
    float padMoveX = 0.0f, padMoveY = 0.0f, padAimX = 0.0f, padAimY = 0.0f, padFire = 0.0f;
    int lastTeamMode = 0;
    int setPlayerNameCalls = 0, setRegionCalls = 0, setLoadoutCalls = 0, setAudioCalls = 0;
    int setLanguageCalls = 0, quickStartCalls = 0, joinCalls = 0, cancelCalls = 0;
    int quitCalls = 0, pauseCalls = 0, resumeCalls = 0, saveConfigCalls = 0;
    int playSoundCalls = 0, gamepadMotionCalls = 0;

private:
    std::string _config = "{}", _l10n = "{}";
    bool _loadoutOk = true;
};

} // namespace surv