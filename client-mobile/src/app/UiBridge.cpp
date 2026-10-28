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
#include "NativeCrash.h"
#include "UiHost.h"
#include "../audio/AudioManager.h"
#include "../net/Api.h"
#include "../net/Net.h"
#include "../ui/Config.h"
#include "../ui/Files.h"
#include "../ui/L10nJson.h"
#include "../ui/Loadout.h"
#include "../ui/NewsJson.h"
#include "../ui/Localization.h"

#include <jni.h>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace surv {

// Test/diagnostic hook: the JNI entry points go through the process-wide
// bridge. Android-only like the rest of this file.
UiBridge* bridgeForTest();

namespace {

// The Java bridge class (proj.android/app/src/dev/axmol/app/SurvBridge.java).
constexpr const char* kBridgeClass = "dev/axmol/app/SurvBridge";
// Java-side render-resolution helper (applies the persisted "Low GFX" tier).
constexpr const char* kRenderScaleClass = "dev/axmol/app/RenderScale";

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
        // Diagnostics owned by the engine, not a scene: `showFps` (pause dialog)
        // lives in the config JSON blob behind this bridge, so it cannot be read
        // with UserDefault::getBoolForKey in AppDelegate.
        if (auto* director = ax::Director::getInstance()) {
            const bool show = config().getBool("showFps", false) || isDevLogEnabled();
            director->setStatsDisplay(show);
            if (show) {
                AXLOGI("[perf] stats overlay enabled at startup");
            }
        }

        // Render-resolution (battery) tier: Full/Medium/Low/Potato. The surface
        // scale is applied on the Java side; the config lives here, so hand it
        // over once. Apply even at Full so a stale fixed surface size (e.g. a
        // Low tier from the previous launch) is cleared.
        const float renderScale = config().getFloat("renderScale", 1.0f);
        ax::JniHelper::callStaticVoidMethod(kRenderScaleClass, "apply",
                                            static_cast<float>(renderScale));
        AXLOGI("[perf] render scale {:.2f} at startup", renderScale);

        // game.ts: camera/player/touch/audio config listeners. The scene keeps
        // them for the next match world (none exists yet).
        applyGameSettings();
        // inputBinds.ts: the persisted key/mouse table (defaults on first run).
        loadInputBinds();
    }

    // Applies the persisted runtime settings (game.ts config listeners) to the
    // live scene/audio manager. Called once at bridge install and after a Java
    // settings screen writes one of the keys handled here.
    void applyGameSettings() {
        if (!_game) {
            return;
        }
        uikit::Config& cfg = config();
        _game->setInterpolationEnabled(cfg.getBool("interpolation", true));
        _game->setScreenShakeEnabled(cfg.getBool("screenShake", true));
        _game->setLocalRotationEnabled(cfg.getBool("localRotation", false));
        _game->setAnonPlayerNames(cfg.getBool("anonPlayerNames", false));
        _game->setTouchSettings(cfg.getString("touchMoveStyle", "anywhere"),
                                cfg.getString("touchAimStyle", "anywhere"),
                                cfg.getBool("touchAimLine", true));
        if (audio::AudioManager* audio = _game->audioManager()) {
            audio->setMasterVolume(cfg.getFloat("masterVolume", 1.0f));
            audio->setSoundVolume(cfg.getFloat("soundVolume", 1.0f));
            audio->setMusicVolume(cfg.getFloat("musicVolume", 1.0f));
            audio->setMute(cfg.getBool("muteAudio", false));
        }
    }

    // Restricts the persisted language to the shipped list (en/fr/jp). A
    // language from an older build is corrected to English and saved, so the
    // Settings dropdown and the l10n table always agree.
    std::string normalizeLanguage() {
        const std::string saved = config().getString("language", "en");
        for (int i = 0; i < uikit::kLocaleCount; ++i) {
            if (saved == uikit::kLocales[i].code) {
                return saved;
            }
        }
        config().setString("language", "en");
        config().save();
        return "en";
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
            // Persist with the value applied (Java also calls saveConfig when
            // the slider is released; this keeps the GL-side write authoritative).
            config().save();
        });
    }

    void setLanguage(const std::string& locale) override {
        // The Java UI reads `getL10nJson()`; keep the standalone table (used
        // when there is no C++ MenuScene on Android) in sync before the config
        // listener re-renders the Java screens.
        if (!_standaloneLocalization) {
            ensureStandaloneLocalization();
        }
        if (_standaloneLocalization) {
            _standaloneLocalization->setLocale(locale);
        }
        config().setString("language", locale);
        config().save();
    }

    void setShowFps(bool show) override {
        // Visual diagnostics only; safe to apply from the game thread.
        runOnGameThread([this, show] {
            if (auto* director = ax::Director::getInstance()) {
                director->setStatsDisplay(show);
                AXLOGI("[perf] stats display {}", director->isStatsDisplay() ? "on" : "off");
            }
            config().setBool("showFps", show);
            config().save();
        });
    }

    int protocolVersion() override { return surv::defs::kProtocolVersion; }

    void playSound(const std::string& name, const std::string& channel) override {
        // One-shot UI/avatar cue from the Java screens. The mixer is owned by
        // the GL thread, and playSound() already drops unknown keys/channels and
        // honours mute/volumes, so this is a plain fire-and-forget queue.
        if (name.empty()) return;
        runOnGameThread([this, name, channel] {
            audio::AudioManager* audio = _game ? _game->audioManager() : nullptr;
            if (!audio) return;
            audio::PlaySoundOptions options;
            options.channel = channel.empty() ? "ui" : channel;
            audio->playSound(name, options);
        });
    }

    // --- key/mouse binds (client/src/inputBinds.ts) -------------------------

    std::string bindsJson() override { return input::bindsJson(_inputBinds); }

    std::string bindsCode() override { return _inputBinds.toBase64(); }

    bool setBind(int action, int type, int code) override {
        if (action < 0 || action >= input::kActionCount) {
            return false;
        }
        input::InputValue value;
        if (type != static_cast<int>(input::InputType::None)) {
            if (type < static_cast<int>(input::InputType::Key) ||
                type > static_cast<int>(input::InputType::MouseWheel)) {
                return false;
            }
            value.type = static_cast<input::InputType>(type);
            if (value.type == input::InputType::Key) {
                // Java captures an Android KeyEvent.KEYCODE_* value; the bind
                // table stores the input.ts Key code (web-compatible codes).
                const int keyCode = input::webKeyFromAndroidKeyCode(code);
                if (keyCode < 0) {
                    return false;
                }
                value.code = static_cast<uint8_t>(keyCode);
            } else {
                value.code = static_cast<uint8_t>(code & 0xff);
            }
        }
        _inputBinds.setBind(action, value);
        persistInputBinds();
        return true;
    }

    void resetBinds() override {
        _inputBinds.loadDefaultBinds();
        persistInputBinds();
    }

    bool setBindsCode(const std::string& code) override {
        input::InputBinds next;
        if (!next.fromBase64(code)) {
            return false;
        }
        _inputBinds = next;
        persistInputBinds();
        return true;
    }

    void gamepadMotion(float moveX, float moveY, float aimX, float aimY, float fire) override {
        // Android gamepad axes (MotionEvents arrive on the UI thread). The input
        // state lives on the scene/game thread, so queue it like hardwareKey.
        runOnGameThread([this, moveX, moveY, aimX, aimY, fire] {
            if (_game) _game->onGamepadMotion(moveX, moveY, aimX, aimY, fire);
        });
    }

    void setConfigValue(const std::string& key, const std::string& value) override {
        // Java sends every value as a string; keep the JSON type so the C++ side
        // can read it with getFloat/getBool (e.g. renderScale, sensitivities).
        if (value == "true" || value == "false") {
            config().setBool(key, value == "true");
        } else {
            std::size_t consumed = 0;
            bool numeric = false;
            try {
                const float parsed = std::stof(value, &consumed);
                numeric = consumed == value.size();
                if (numeric) {
                    config().setFloat(key, parsed);
                }
            } catch (const std::exception&) {
                numeric = false;
            }
            if (!numeric) {
                config().setString(key, value);
            }
        }
        config().save();
        // Settings that the live render/input path reads immediately (game.ts
        // config listeners). Apply on the game thread, like every other scene
        // mutation from the Java UI.
        if (key == "interpolation" || key == "screenShake" || key == "localRotation" ||
            key == "anonPlayerNames" || key == "touchMoveStyle" || key == "touchAimStyle" ||
            key == "touchAimLine") {
            runOnGameThread([this] { applyGameSettings(); });
        }
    }

    std::string l10nJson() override {
        if (_menu) {
            return uikit::l10nTableToJson(_menu->localization().flattened());
        }
        // Android has no C++ MenuScene (`installUiBridge(nullptr, gameScene)`),
        // so the Java UI needs its own Localization instance. Load it lazily
        // from the bundled l10n assets using the config language.
        ensureStandaloneLocalization();
        // The config may have been loaded after the first l10n request; keep the
        // standalone table on the persisted language instead of caching English.
        _standaloneLocalization->setLocale(normalizeLanguage());
        return uikit::l10nTableToJson(_standaloneLocalization->flattened());
    }

    std::string language() override { return normalizeLanguage(); }

    std::string loadoutJson() override {
        uikit::Loadout loadout(&config());
        return uikit::jsonToString(loadout.validated());
    }
    // Language dropdown source: ui::kLocales (localization.ts Locales order).
    std::string localesJson() override {
        std::string out = "[";
        for (int i = 0; i < uikit::kLocaleCount; ++i) {
            if (i) {
                out += ",";
            }
            out += "{\"code\":\"";
            out += uikit::kLocales[i].code;
            out += "\",\"name\":\"";
            out += uikit::kLocales[i].name;
            out += "\"}";
        }
        out += "]";
        return out;
    }
    // Owned loadout items + shared-def metadata for the Java customize screen.
    std::string loadoutItemsJson() override {
        const auto* provider = surv::getDefProvider();
        if (!provider || !provider->loadoutItems()) {
            return "[]";
        }
        uikit::Loadout loadout(&config());
        std::map<std::string, std::set<std::string>> owned;
        for (const char* category : { "outfit", "melee", "emote", "heal", "boost", "crosshair" }) {
            for (const auto& type : loadout.items(category)) {
                owned[category].insert(type);
            }
        }
        const auto categoryFor = [](const std::string& defCategory) {
            if (defCategory == "heal_effect") return std::string("heal");
            if (defCategory == "boost_effect") return std::string("boost");
            return defCategory;
        };
        // shared/utils/loadout.ts getUserAvailableItems gives every
        // unlock_default item source "unlock_default" (the web then shows the
        // def name "standard-issue" -> "Standard Issue").
        std::set<std::string> defaultUnlocks;
        if (const auto* unlockDef = provider->gameObject("unlock_default")) {
            defaultUnlocks.insert(unlockDef->unlocks.begin(), unlockDef->unlocks.end());
        }
        uikit::JsonValue out;
        out.kind = uikit::JsonValue::Kind::Array;
        for (const auto& def : *provider->loadoutItems()) {
            const std::string category = categoryFor(def.category);
            const auto it = owned.find(category);
            if (it == owned.end() || it->second.count(def.type) == 0) {
                continue;
            }
            uikit::JsonValue item;
            item.kind = uikit::JsonValue::Kind::Object;
            auto addString = [&item](const char* key, const std::string& value) {
                uikit::JsonValue v;
                v.kind = uikit::JsonValue::Kind::String;
                v.string = value;
                item.object[key] = std::move(v);
            };
            addString("type", def.type);
            addString("category", category);
            addString("name", def.name);
            addString("lore", def.lore);
            addString("sprite", def.sprite);
            addString("subcat", def.subcat);
            addString("source", defaultUnlocks.count(def.type) ? "unlock_default" : "");
            uikit::JsonValue rarity;
            rarity.kind = uikit::JsonValue::Kind::Number;
            rarity.number = def.rarity;
            item.object["rarity"] = std::move(rarity);
            uikit::JsonValue tint;
            tint.kind = uikit::JsonValue::Kind::Number;
            tint.number = def.tint;
            item.object["tint"] = std::move(tint);
            out.array.push_back(std::move(item));
        }
        return uikit::jsonToString(out);
    }
    bool setAccountItemsJson(const std::string& json) override {
        uikit::JsonValue parsed;
        if (!uikit::JsonParser::parse(json, parsed) || !parsed.isArray()) {
            return false;
        }
        uikit::JsonValue types;
        types.kind = uikit::JsonValue::Kind::Array;
        for (const auto& value : parsed.array) {
            if (value.isString() && !value.string.empty()) {
                uikit::JsonValue type;
                type.kind = uikit::JsonValue::Kind::String;
                type.string = value.string;
                types.array.push_back(std::move(type));
            }
        }
        config().setJson("accountItems", types);
        config().save();
        return true;
    }
    // Display names for the account item `source` types (loadoutMenu.ts
    // getItemSourceName): [{"type":"unlock_new_account","name":"new-account"}].
    std::string loadoutSourcesJson() override {
        const auto* provider = surv::getDefProvider();
        if (!provider || !provider->loadoutSources()) {
            return "[]";
        }
        uikit::JsonValue out;
        out.kind = uikit::JsonValue::Kind::Array;
        for (const auto& source : *provider->loadoutSources()) {
            uikit::JsonValue item;
            item.kind = uikit::JsonValue::Kind::Object;
            uikit::JsonValue type;
            type.kind = uikit::JsonValue::Kind::String;
            type.string = source.type;
            item.object["type"] = std::move(type);
            uikit::JsonValue name;
            name.kind = uikit::JsonValue::Kind::String;
            name.string = source.name;
            item.object["name"] = std::move(name);
            out.array.push_back(std::move(item));
        }
        return uikit::jsonToString(out);
    }

    std::string siteInfoJson() override { return _menu ? _menu->siteInfoJson() : std::string(); }

    std::string newsJson() override { return _menu ? _menu->newsJson() : std::string("[]"); }

    void reportError(const std::string& loc, const std::string& dataJson) override {
        runOnGameThread([this, loc, dataJson] {
            if (_game) {
                _game->reportError(loc, dataJson);
            }
        });
    }

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

    void spectate(int action) override {
        runOnGameThread([this, action] {
            if (!_game) return;
            if (Game* game = _game->getGame()) {
                game->sendSpectate(static_cast<uint8_t>(action));
            }
        });
    }

    void requestLeaderboard() override {
        runOnGameThread([this] {
            if (!_game) return;
            Game* game = _game->getGame();
            if (!game) return;
            ax::JniHelper::callStaticVoidMethod(kBridgeClass, "onNativeLeaderboard",
                                                game->leaderboardJson());
        });
    }

    void hardwareKey(int androidKeyCode, bool down) override {
        runOnGameThread([this, androidKeyCode, down] {
            if (_game) {
                _game->onHardwareKey(androidKeyCode, down);
            }
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
    // inputBinds.ts loadBinds(): the persisted `binds` code, or the defaults
    // (written back so the next launch and the Java screen agree).
    void loadInputBinds() {
        _inputBinds.loadDefaultBinds();
        const std::string saved = config().getString("binds", "");
        if (!_inputBinds.loadFromConfigString(saved)) {
            config().setString("binds", _inputBinds.toBase64());
            config().save();
            AXLOGI("[input] default key binds written");
        }
        if (_game) {
            _game->setInputBinds(_inputBinds);
        }
    }

    // inputBinds.ts saveBinds(): persist and apply to the live hardware input.
    void persistInputBinds() {
        config().setString("binds", _inputBinds.toBase64());
        config().save();
        const input::InputBinds copy = _inputBinds;
        runOnGameThread([this, copy] {
            if (_game) {
                _game->setInputBinds(copy);
            }
        });
        AXLOGI("[input] key binds updated");
    }

    // Java-only builds have no C++ MenuScene to own the l10n tables; build the
    // same tables here from the bundled assets (ARCHITECTURE.md A5.3).
    void ensureStandaloneLocalization() {
        if (_standaloneLocalization) {
            return;
        }
        _standaloneLocalization = std::make_unique<uikit::Localization>();
        const std::string english = uikit::Files::readText("l10n/en.json");
        if (!english.empty()) {
            _standaloneLocalization->registerEnglish(english);
        } else {
            AXLOGW("Missing l10n/en.json; Java UI strings fall back to keys");
        }
        _standaloneLocalization->setLoader([](const std::string& locale) {
            return uikit::Files::readText("l10n/" + locale + ".json");
        });
        _standaloneLocalization->setLocale(normalizeLanguage());
    }

    MenuScene* _menu = nullptr;
    GameScene* _game = nullptr;
    // inputBinds.ts table (UI-thread owned; copies are pushed to the scene).
    input::InputBinds _inputBinds;
    std::unique_ptr<uikit::UserDefaultConfigStorage> _standaloneStorage;
    std::unique_ptr<uikit::Config> _standaloneConfig;
    std::unique_ptr<uikit::Localization> _standaloneLocalization;
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
    bool togglePause() override {
        notify("onNativeTogglePause");
        return true; // the Java UI owns the pause dialog
    }
    void leaderboard(const std::string& json) override {
        ax::JniHelper::callStaticVoidMethod(kBridgeClass, "onNativeLeaderboard", json);
    }
    std::string configString(const std::string& key) override {
        return androidBridge().config().getString(key, "");
    }
    void setConfigValue(const std::string& key, const std::string& value) override {
        androidBridge().config().setString(key, value);
        androidBridge().config().save();
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

JNIEXPORT void JNICALL Java_dev_axmol_app_SurvBridge_onRequestLeaderboard(JNIEnv*, jclass) {
    surv::uiBridge()->requestLeaderboard();
}

JNIEXPORT void JNICALL Java_dev_axmol_app_SurvBridge_spectate(JNIEnv*, jclass, jint action) {
    surv::uiBridge()->spectate(static_cast<int>(action));
}

JNIEXPORT void JNICALL Java_dev_axmol_app_SurvBridge_hardwareKey(JNIEnv*, jclass, jint keyCode,
                                                                jboolean down) {
    surv::uiBridge()->hardwareKey(static_cast<int>(keyCode), down == JNI_TRUE);
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

// One-shot cue from the Java UI (menu avatar, buttons): Queued onto the GL
// thread inside the bridge; unknown keys/channels and mute are handled there.
JNIEXPORT void JNICALL Java_dev_axmol_app_SurvBridge_playSound(JNIEnv* env, jclass, jstring name,
                                                              jstring channel) {
    surv::uiBridge()->playSound(surv::fromJstring(env, name), surv::fromJstring(env, channel));
}

// Android gamepad axes (AppActivity.onGenericMotionEvent): queued onto the GL
// thread and applied to the scene's HardwareInput.
JNIEXPORT void JNICALL Java_dev_axmol_app_SurvBridge_gamepadAxes(JNIEnv*, jclass, jfloat moveX,
                                                                jfloat moveY, jfloat aimX,
                                                                jfloat aimY, jfloat fire) {
    surv::uiBridge()->gamepadMotion(static_cast<float>(moveX), static_cast<float>(moveY),
                                    static_cast<float>(aimX), static_cast<float>(aimY),
                                    static_cast<float>(fire));
}

JNIEXPORT void JNICALL Java_dev_axmol_app_SurvBridge_setLanguage(JNIEnv* env, jclass, jstring locale) {
    surv::uiBridge()->setLanguage(surv::fromJstring(env, locale));
}

JNIEXPORT void JNICALL Java_dev_axmol_app_SurvBridge_setShowFps(JNIEnv*, jclass, jboolean show) {
    surv::uiBridge()->setShowFps(show == JNI_TRUE);
}

JNIEXPORT jint JNICALL Java_dev_axmol_app_SurvBridge_getProtocolVersion(JNIEnv*, jclass) {
    return surv::uiBridge()->protocolVersion();
}

JNIEXPORT void JNICALL Java_dev_axmol_app_SurvBridge_setConfigValue(JNIEnv* env, jclass, jstring key,
                                                                   jstring value) {
    surv::uiBridge()->setConfigValue(surv::fromJstring(env, key), surv::fromJstring(env, value));
}

JNIEXPORT jstring JNICALL Java_dev_axmol_app_SurvBridge_getConfigJson(JNIEnv* env, jclass) {
    return surv::toJstring(env, surv::uiBridge()->configJson());
}

JNIEXPORT jstring JNICALL Java_dev_axmol_app_SurvBridge_getL10nJson(JNIEnv* env, jclass) {
    return surv::toJstring(env, surv::uiBridge()->l10nJson());
}

JNIEXPORT jstring JNICALL Java_dev_axmol_app_SurvBridge_getLanguage(JNIEnv* env, jclass) {
    return surv::toJstring(env, surv::uiBridge()->language());
}

JNIEXPORT jstring JNICALL Java_dev_axmol_app_SurvBridge_getLocalesJson(JNIEnv* env, jclass) {
    return surv::toJstring(env, surv::uiBridge()->localesJson());
}

JNIEXPORT jstring JNICALL Java_dev_axmol_app_SurvBridge_getLoadoutJson(JNIEnv* env, jclass) {
    return surv::toJstring(env, surv::uiBridge()->loadoutJson());
}

// Key/mouse binds (client/src/inputBinds.ts): the Java Key Binds screen lists
// the table, captures a key and imports/exports the share code.
JNIEXPORT jstring JNICALL Java_dev_axmol_app_SurvBridge_getBindsJson(JNIEnv* env, jclass) {
    return surv::toJstring(env, surv::uiBridge()->bindsJson());
}

JNIEXPORT jstring JNICALL Java_dev_axmol_app_SurvBridge_getBindsCode(JNIEnv* env, jclass) {
    return surv::toJstring(env, surv::uiBridge()->bindsCode());
}

JNIEXPORT jboolean JNICALL Java_dev_axmol_app_SurvBridge_setBind(JNIEnv*, jclass, jint action,
                                                                jint type, jint code) {
    return surv::uiBridge()->setBind(static_cast<int>(action), static_cast<int>(type),
                                     static_cast<int>(code))
        ? JNI_TRUE
        : JNI_FALSE;
}

JNIEXPORT void JNICALL Java_dev_axmol_app_SurvBridge_resetBinds(JNIEnv*, jclass) {
    surv::uiBridge()->resetBinds();
}

JNIEXPORT jboolean JNICALL Java_dev_axmol_app_SurvBridge_setBindsCode(JNIEnv* env, jclass,
                                                                     jstring code) {
    return surv::uiBridge()->setBindsCode(surv::fromJstring(env, code)) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jstring JNICALL Java_dev_axmol_app_SurvBridge_getLoadoutItemsJson(JNIEnv* env, jclass) {
    return surv::toJstring(env, surv::uiBridge()->loadoutItemsJson());
}

JNIEXPORT jstring JNICALL Java_dev_axmol_app_SurvBridge_getLoadoutSourcesJson(JNIEnv* env, jclass) {
    return surv::toJstring(env, surv::uiBridge()->loadoutSourcesJson());
}

JNIEXPORT jboolean JNICALL Java_dev_axmol_app_SurvBridge_setAccountItems(JNIEnv* env, jclass,
                                                                        jstring json) {
    return surv::uiBridge()->setAccountItemsJson(surv::fromJstring(env, json)) ? JNI_TRUE
                                                                              : JNI_FALSE;
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

JNIEXPORT void JNICALL Java_dev_axmol_app_SurvBridge_reportError(JNIEnv* env, jclass, jstring loc,
                                                                jstring data) {
    surv::uiBridge()->reportError(surv::fromJstring(env, loc), surv::fromJstring(env, data));
}

JNIEXPORT void JNICALL Java_dev_axmol_app_SurvBridge_installNativeCrashHandler(JNIEnv* env, jclass,
                                                                              jstring path) {
    surv::installNativeCrashHandler(surv::fromJstring(env, path));
}

} // extern "C"

#endif // AX_TARGET_PLATFORM == AX_PLATFORM_ANDROID
