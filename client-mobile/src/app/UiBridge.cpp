// The Android half of the UI seam (ARCHITECTURE.md §4): the JNI entry points
// `dev.axmol.app.SurvBridge` calls into C++, and the C++ -> Java notifications
// the app raises through `ax::JniHelper`.
//
// Build rules (ARCHITECTURE.md §4.1/§6.1):
//   * Android-only: every symbol is behind
//     `AX_TARGET_PLATFORM == AX_PLATFORM_ANDROID`, so the host tests never
//     compile it. `tests/CMakeLists.txt` links `src/app/UiHost.cpp`, which
//     defines the same `installUiBridge` as a no-op.
//   * Java never blocks the GL thread and C++ never touches Views off-thread:
//     the Java side posts every callback to the UI thread itself (§4.2).

#include "axmol.h"

#if (AX_TARGET_PLATFORM == AX_PLATFORM_ANDROID)

// axmol only pulls JniHelper in for platform code; the bridge needs it for the
// C++ -> Java notifications (ARCHITECTURE.md §4.1).
#include "platform/android/jni/JniHelper.h"

#include "GameScene.h"
#include "MenuScene.h"
#include "UiHost.h"
#include "../audio/AudioManager.h"
#include "../ui/Config.h"
#include "../ui/L10nJson.h"
#include "../ui/Loadout.h"
#include "../ui/NewsJson.h"
#include "../ui/Localization.h"

#include <jni.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace surv {

// Test/diagnostic hook: the JNI entry points go through the process-wide
// bridge. Android-only like the rest of this file.
UiBridge* bridgeForTest();

namespace {

// The Java bridge class (proj.android/app/src/dev/axmol/app/SurvBridge.java).
constexpr const char* kBridgeClass = "dev/axmol/app/SurvBridge";

// JNI calls arrive on the Java UI thread; the scene graph, GameWorld and audio
// mixer are owned by the GL thread. World teardown (resetWorld()) must never
// race a frame's update, so scene-mutating bridge calls are queued onto the
// axmol scheduler (ARCHITECTURE.md A4.2).
void runOnGameThread(std::function<void()> action) {
    if (auto* director = ax::Director::getInstance()) {
        if (auto* scheduler = director->getScheduler()) {
            scheduler->runOnAxmolThread(std::move(action));
            return;
        }
    }
    AXLOGW("bridge: no scheduler; running action inline");
    action();
}

std::string fromJstring(JNIEnv* env, jstring value) {
    if (!value) return {};
    const char* chars = env->GetStringUTFChars(value, nullptr);
    std::string out = chars ? chars : "";
    if (chars) env->ReleaseStringUTFChars(value, chars);
    return out;
}

std::vector<std::string> fromJstringArray(JNIEnv* env, jobjectArray array) {
    std::vector<std::string> out;
    if (!array) return out;
    const jsize count = env->GetArrayLength(array);
    out.reserve(static_cast<size_t>(count));
    for (jsize i = 0; i < count; ++i) {
        auto* item = static_cast<jstring>(env->GetObjectArrayElement(array, i));
        out.push_back(fromJstring(env, item));
        env->DeleteLocalRef(item);
    }
    return out;
}

jstring toJstring(JNIEnv* env, const std::string& text) {
    return env->NewStringUTF(text.c_str());
}

} // namespace

// The `ui::` toolkit namespace collides with `ax::ui` in this unit (axmol.h
// brings it in); alias it once, like GameScene/MenuScene do.
namespace uikit = ::ui;

// The concrete bridge. It owns nothing: it reads the scenes the AppDelegate
// handed it and mutates the same `ui::Config` the C++ menu uses, so both UI
// halves keep one config blob (ARCHITECTURE.md §5.3).
class AndroidUiBridge : public UiBridge {
public:
    void setScenes(MenuScene* menu, GameScene* game) {
        _menu = menu;
        _game = game;
    }

    // Android has no C++ MenuScene (AppDelegate passes nullptr), but the Java
    // menu still needs the same persisted config (name/region/loadout/crosshair/
    // volumes). Keep a standalone Config over the UserDefault storage in that
    // case instead of silently dropping every bridge write.
    uikit::Config& config() {
        if (_menu) {
            return _menu->config();
        }
        if (!_standaloneStorage) {
            _standaloneStorage = std::make_unique<uikit::UserDefaultConfigStorage>();
            _standaloneConfig = std::make_unique<uikit::Config>(_standaloneStorage.get());
            _standaloneConfig->load();
        }
        return *_standaloneConfig;
    }

    std::string configJson() override {
        return uikit::jsonToString(config().root());
    }

    void setPlayerName(const std::string& name) override {
        config().setString("playerName", name);
    }

    void setRegion(const std::string& region) override {
        config().setString("region", region);
        // main.ts: the config `region` listener marks the choice as explicit, so
        // the latency test stops auto-selecting on later launches. Auto-select
        // also goes through here (pingTest.getRegion() -> config.set("region")).
        config().setBool("regionSelected", true);
    }

    bool setLoadoutJson(const std::string& json) override {
        uikit::JsonValue parsed;
        if (!uikit::JsonParser::parse(json, parsed) || !parsed.isObject()) {
            return false;
        }
        // Store the selection, then let the C++ validator normalise it: Java
        // must not be able to bypass the checks `ui::Loadout` performs before
        // the loadout reaches the JoinMsg. Java may send a partial update (the
        // crosshair editor does), so merge into the current loadout instead of
        // replacing it wholesale.
        uikit::JsonValue merged = config().getJson("loadout") &&
                config().getJson("loadout")->isObject()
            ? *config().getJson("loadout")
            : uikit::Config::defaultLoadout();
        for (const auto& entry : parsed.object) {
            if (entry.first == "crosshair" && entry.second.isObject() &&
                merged.object["crosshair"].isObject()) {
                for (const auto& field : entry.second.object) {
                    merged.object["crosshair"].object[field.first] = field.second;
                }
            } else {
                merged.object[entry.first] = entry.second;
            }
        }
        config().setJson("loadout", merged);
        uikit::Loadout loadout(&config());
        config().setJson("loadout", loadout.validated());
        config().save();
        return true;
    }

    void setAudio(const std::string& key, float value) override {
        runOnGameThread([this, key, value] {
            audio::AudioManager* audio = _game ? _game->audioManager() : nullptr;
            if (key == "master") {
                config().setFloat("masterVolume", value);
                if (audio) audio->setMasterVolume(value);
            } else if (key == "sound") {
                config().setFloat("soundVolume", value);
                if (audio) audio->setSoundVolume(value);
            } else if (key == "music") {
                config().setFloat("musicVolume", value);
                if (audio) audio->setMusicVolume(value);
            } else if (key == "mute") {
                const bool mute = value != 0.0f;
                config().setBool("muteAudio", mute);
                if (audio) audio->setMute(mute);
            }
        });
    }

    void setLanguage(const std::string& locale) override {
        config().setString("language", locale);
    }

    std::string l10nJson() override {
        return _menu ? uikit::l10nTableToJson(_menu->localization().flattened()) : std::string("{}");
    }

    std::string loadoutJson() override {
        uikit::Loadout loadout(&config());
        return uikit::jsonToString(loadout.validated());
    }

    std::string siteInfoJson() override { return _menu ? _menu->siteInfoJson() : std::string(); }

    std::string newsJson() override { return _menu ? _menu->newsJson() : std::string("[]"); }

    bool isInGame() override { return _game && _game->isInGame(); }

    void quickStart(int teamMode) override {
        runOnGameThread([this, teamMode] {
            if (_menu) _menu->quickStartFromBridge(teamMode);
        });
    }

    void joinUrls(const std::vector<std::string>& urls, const std::string& joinToken) override {
        if (!_game || urls.empty()) return;
        runOnGameThread([this, urls, joinToken] {
            if (!_game) return;
            // Keep the loadout the Java UI settled on in the join parcel,
            // exactly like main.ts sets the config fields before
            // Game.enterWithJoin().
            const Game::JoinInfo info = uikit::Loadout(&config()).joinInfo();
            if (Game* game = _game->getGame()) {
                game->setJoinInfo(info);
            }
            _game->enterWithJoin(urls, joinToken);
        });
    }

    void cancelPending() override {
        runOnGameThread([this] {
            if (_menu) _menu->cancelPending();
        });
    }

    void quitGame() override {
        runOnGameThread([this] {
            if (_game) _game->returnToMenu();
        });
    }

    void pauseGame() override {
        runOnGameThread([this] {
            if (_game) _game->pauseGame();
        });
    }

    void resumeGame() override {
        runOnGameThread([this] {
            if (_game) _game->resumeGame();
        });
    }

    void saveConfig() override {
        if (_menu) {
            _menu->saveConfig();
        } else {
            config().save();
        }
    }

private:
    MenuScene* _menu = nullptr;
    GameScene* _game = nullptr;
    std::unique_ptr<uikit::UserDefaultConfigStorage> _standaloneStorage;
    std::unique_ptr<uikit::Config> _standaloneConfig;
};

namespace {

AndroidUiBridge& androidBridge() {
    static AndroidUiBridge bridge;
    return bridge;
}

// The C++ -> Java host: forwards engine events to `SurvBridge`'s static
// handlers, which post to the UI thread themselves (ARCHITECTURE.md §4.2).
class AndroidUiHost : public UiHost {
public:
    void showMenu() override { notify("onNativeShowMenu"); }
    void enterGame(int activePlayerId) override {
        ax::JniHelper::callStaticVoidMethod(kBridgeClass, "onNativeMatchStarted",
                                            static_cast<int>(activePlayerId));
    }
    void returnToMenu(const std::string& errorL10nKey) override {
        ax::JniHelper::callStaticVoidMethod(kBridgeClass, "onNativeMatchEnded", errorL10nKey);
    }
    void showResult() override { notify("onNativePlayerDead"); }
    void pingResults(const std::string& json) override {
        ax::JniHelper::callStaticVoidMethod(kBridgeClass, "onNativePingResults", json);
    }

private:
    void notify(const char* method) {
        ax::JniHelper::callStaticVoidMethod(kBridgeClass, method);
    }
};

AndroidUiHost& androidHost() {
    static AndroidUiHost host;
    return host;
}

} // namespace

// Test/diagnostic hook: the JNI entry points below go through the process-wide
// bridge, which the app installs with installUiBridge(). Only the Android build
// compiles this file, so it is not part of `surv_tests`.
UiBridge* bridgeForTest() { return uiBridge(); }

void installUiBridge(MenuScene* menu, GameScene* game) {
    androidBridge().setScenes(menu, game);
    setUiBridge(&androidBridge());
    setUiHost(&androidHost());
}

} // namespace surv

// ---------------------------------------------------------------------------
// JNI entry points (Java -> C++). The symbol names encode the Java package and
// class: dev.axmol.app.SurvBridge. axmol's own JNI_OnLoad does not know them,
// but the dynamic linker resolves them as long as they are extern "C" and
// exported (ARCHITECTURE.md §4.1).
// ---------------------------------------------------------------------------
extern "C" {

JNIEXPORT void JNICALL Java_dev_axmol_app_SurvBridge_onMenuReady(JNIEnv*, jclass) {}

JNIEXPORT void JNICALL Java_dev_axmol_app_SurvBridge_onQuickStart(JNIEnv*, jclass, jint teamMode) {
    surv::uiBridge()->quickStart(static_cast<int>(teamMode));
}

JNIEXPORT void JNICALL Java_dev_axmol_app_SurvBridge_onJoinUrls(JNIEnv* env, jclass,
                                                               jobjectArray urls, jstring token) {
    surv::uiBridge()->joinUrls(surv::fromJstringArray(env, urls), surv::fromJstring(env, token));
}

JNIEXPORT void JNICALL Java_dev_axmol_app_SurvBridge_onCancelPending(JNIEnv*, jclass) {
    surv::uiBridge()->cancelPending();
}

JNIEXPORT void JNICALL Java_dev_axmol_app_SurvBridge_onQuitGame(JNIEnv*, jclass) {
    surv::uiBridge()->quitGame();
}

JNIEXPORT void JNICALL Java_dev_axmol_app_SurvBridge_onPauseGame(JNIEnv*, jclass) {
    surv::uiBridge()->pauseGame();
}

JNIEXPORT void JNICALL Java_dev_axmol_app_SurvBridge_onResumeGame(JNIEnv*, jclass) {
    surv::uiBridge()->resumeGame();
}

JNIEXPORT void JNICALL Java_dev_axmol_app_SurvBridge_setPlayerName(JNIEnv* env, jclass, jstring name) {
    surv::uiBridge()->setPlayerName(surv::fromJstring(env, name));
}

JNIEXPORT void JNICALL Java_dev_axmol_app_SurvBridge_setRegion(JNIEnv* env, jclass, jstring region) {
    surv::uiBridge()->setRegion(surv::fromJstring(env, region));
}

JNIEXPORT jboolean JNICALL Java_dev_axmol_app_SurvBridge_setLoadout(JNIEnv* env, jclass, jstring json) {
    return surv::uiBridge()->setLoadoutJson(surv::fromJstring(env, json)) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL Java_dev_axmol_app_SurvBridge_setAudio(JNIEnv* env, jclass, jstring key,
                                                             jfloat value) {
    surv::uiBridge()->setAudio(surv::fromJstring(env, key), static_cast<float>(value));
}

JNIEXPORT void JNICALL Java_dev_axmol_app_SurvBridge_setLanguage(JNIEnv* env, jclass, jstring locale) {
    surv::uiBridge()->setLanguage(surv::fromJstring(env, locale));
}

JNIEXPORT jstring JNICALL Java_dev_axmol_app_SurvBridge_getConfigJson(JNIEnv* env, jclass) {
    return surv::toJstring(env, surv::uiBridge()->configJson());
}

JNIEXPORT jstring JNICALL Java_dev_axmol_app_SurvBridge_getL10nJson(JNIEnv* env, jclass) {
    return surv::toJstring(env, surv::uiBridge()->l10nJson());
}

JNIEXPORT jstring JNICALL Java_dev_axmol_app_SurvBridge_getLoadoutJson(JNIEnv* env, jclass) {
    return surv::toJstring(env, surv::uiBridge()->loadoutJson());
}

JNIEXPORT jstring JNICALL Java_dev_axmol_app_SurvBridge_getSiteInfoJson(JNIEnv* env, jclass) {
    return surv::toJstring(env, surv::uiBridge()->siteInfoJson());
}

JNIEXPORT jstring JNICALL Java_dev_axmol_app_SurvBridge_getNewsJson(JNIEnv* env, jclass) {
    return surv::toJstring(env, surv::uiBridge()->newsJson());
}

JNIEXPORT jboolean JNICALL Java_dev_axmol_app_SurvBridge_isInGame(JNIEnv*, jclass) {
    return surv::uiBridge()->isInGame() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL Java_dev_axmol_app_SurvBridge_saveConfig(JNIEnv*, jclass) {
    surv::uiBridge()->saveConfig();
}

} // extern "C"

#endif // AX_TARGET_PLATFORM == AX_PLATFORM_ANDROID
