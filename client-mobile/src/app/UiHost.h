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

    // --- read-only snapshots ------------------------------------------------
    // Flattened table for the current locale ({"key": "text"}).
    virtual std::string l10nJson() = 0;
    virtual std::string loadoutJson() = 0;
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
    std::string newsJson() override { return news; }

    std::string playerName, region, language, news = "[]";
    std::string loadoutJsonText, joinToken;
    std::vector<std::string> joinedUrls, audioKeys;
    float audioValue = 0.0f;
    int lastTeamMode = 0;
    int setPlayerNameCalls = 0, setRegionCalls = 0, setLoadoutCalls = 0, setAudioCalls = 0;
    int setLanguageCalls = 0, quickStartCalls = 0, joinCalls = 0, cancelCalls = 0;
    int quitCalls = 0, pauseCalls = 0, resumeCalls = 0, saveConfigCalls = 0;

private:
    std::string _config = "{}", _l10n = "{}";
    bool _loadoutOk = true;
};

} // namespace surv