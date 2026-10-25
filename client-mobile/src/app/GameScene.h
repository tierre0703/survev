#pragma once
#include "axmol.h"
#include "../game/Game.h"
#include "../game/TouchInput.h"
#include "../net/ErrorLog.h"
#include "../net/Net.h"
#include "../net/PingTest.h"
#include "../render/PixiLike.h"
#include "../ui/Touch.h"
#include <atomic>
#include <memory>
#include <string>

namespace ax {
class Node;
}
namespace pix {
class Factory;
}
namespace ui {
class UiOverlay; // M7 in-game UI (HUD + pause menu)
}
namespace fairygui {
class GRoot;
}

namespace surv {

// The global UI toolkit (see ui/Ui.h). Aliased inside `surv` so `uikit::` reads
// like the web `ui.ts` while staying distinct from the vestigial `surv::ui`
// forward declarations above.
namespace uikit = ::ui;

class GameWorld;

fairygui::GRoot* fguiRoot();

class FguiHud;
class Hud;
class PieTimer;
class EmoteWheel;
class PingWheel;

class PadGraphicsAxmol;

namespace audio {
class AxmolAudioBackend;
class AudioManager;
class Ambiance;
} // namespace audio

// axmol PadGraphics implementation (touch.ts pad.img sprites).
// Legacy atlas-based pad; the live pads are PadGraphicsAxmol (gui/pad.png +
// DrawNode dot), so this class is kept only as a fallback and is not used.
class TouchPadGfx : public PadGraphics {
public:
    explicit TouchPadGfx(ax::Node* parent);
    void setVisible(bool visible) override;
    void setCenterPos(float x, float y) override;
    void setTouchPos(float x, float y) override;
    void setCenterScale(float s) override;
    void setTouchScale(float s) override;
    void update() override;
    // A map load registers the atlas holding `pad.img` after the pads were
    // created; call this then (see Game::setAtlasCallback).
    void refreshPadFrame();
    // True when the pad texture resolved (pad.img frame or gui/pad.png).
    bool hasTexture() const;
    // True once the pads are using the atlas `pad.img` (not the GUI fallback).
    bool isAtlasPad() const;
    const std::string& padFrame() const;

private:
    // Applies `_padScale` to a raw Touch scale (see GameScene.cpp).
    void sizeSprite(ax::Sprite* sprite, float scale);

    ax::Sprite* _center = nullptr;
    ax::Sprite* _touch = nullptr;
    std::string _padFrame;
    // Maps touch.ts's pad.img scale onto `_padFrame`'s pixel size.
    float _padScale = 0.25f;
    bool _textureReady = false;
    bool _atlasPad = false;
};

// Main scene: owns the game (simulation + net), the renderer adapter,
// and the touch joysticks. This is the axmol counterpart of
// client/src/game.ts + main.ts glue for the in-game surface.
class GameScene : public ax::Scene {
public:
    static GameScene* createScene();

    ~GameScene() override;

    bool init() override;
    void update(float delta) override;

    // Input callbacks -> Touch
    void onTouchBegan(float x, float y, int id);
    void onTouchMoved(float x, float y, int id);
    void onTouchEnded(int id);

    // Android hardware back button / desktop Escape: navigate back.
    bool handleBackPressed();

    ax::Node* getGameRoot() const { return _gameRoot; }
    Game* getGame() const { return _game.get(); }

    // M7: the menu owns the join flow; `enterWithJoin`/`enterWithFindGame`
    // start a game (replacing the old DevConfig auto-connect).
    struct FindGameResultInfo {
        bool ok = false;
        std::vector<std::string> urls;
        std::string joinToken;
        std::string error;
    };
    using FindGameDone = std::function<void(const FindGameResultInfo&)>;
    using FindGameRequest =
        std::function<void(const std::string& region, int gameModeIdx, FindGameDone done)>;
    void setFindGameRequest(FindGameRequest request) { _findGameRequest = std::move(request); }

    // Direct join (dev/URL param or team `joinGame`).
    void enterWithJoin(const std::vector<std::string>& urls, const std::string& joinToken);
    // Quick-start a mode: discover a match with the locale's backoff, then join.
    void enterWithFindGame(const std::string& region, int gameModeIdx);
    void leaveGame();

    bool isInGame() const { return _inGame; }
    bool isStarted() const { return _game && _game->hasJoined(); }
    bool isConnected() const { return _game && _game->isConnected(); }
    bool isPlaying() const { return _game && _game->isPlaying(); }

    // UI overlay (HUD + pause menu). Not owned by the scene.
    void setOverlay(uikit::UiOverlay* overlay) { _overlay = overlay; }
    // The menu's scaled design-space root; the overlay adopts it so HUD and
    // pause geometry (plus the toolkit tap mapping) match the menu.
    void setUiRoot(ax::Node* root) { _uiRoot = root; }
    ax::Node* getUiRoot() const { return _uiRoot; }
    surv::Touch* getTouch() { return _touch.get(); }
    // Player name + loadout used for the next join (mirrors main.ts
    // setConfigFromDOM + JoinMsg fields).
    void setJoinInfo(const Game::JoinInfo& info) { _joinInfo = info; }
    const Game::JoinInfo& joinInfo() const { return _joinInfo; }
    audio::AudioManager* audioManager() const { return _audio.get(); }
    std::function<void()> onMatchStarted;
    std::function<void(const std::string&)> onMatchEnded;
    std::function<void(const std::string&, const std::string&)> onError;

    // App lifecycle hooks (AppDelegate).
    void pauseGame();
    void resumeGame();

    // Called from Game::handlePickup to play the item pickup sound.
    void onPickupSound(const std::string& item);

    // M8: pre-game region latency test (client/src/pingTest.ts). The probe
    // targets come from the generated `Config.regions` table (net/Regions.h),
    // the native counterpart of the web's PING_TEST_URLS. Results are pushed
    // back through UiHost::pingResults() once every test finished.
    void startRegionPingTest();
    void stopPingTest();

    // M8: client error reporting (client/src/errorLogs.ts). `dataJson` is an
    // optional JSON value; the report goes through net/ErrorLog's sampled/
    // throttled /api/report_error writer. Used by the Java bridge for errors
    // that are not fatal (fatal ones are posted by the Java crash handler).
    void reportError(const std::string& loc, const std::string& dataJson);

    // API origin for `/api/*` calls (UserDefault override, else DevConfig).
    static std::string resolveApiBaseUrl();

    // Aborts any join in progress and notifies the menu that the match should
    // re-open. Used by the matchmaking overlay's "Back to Main Menu".
    void returnToMenu();

private:
    // M5: build an InputMsg from Touch and send it at the server's input rate.
    void updateInput(float dt);
    // Web screen point (y down, visible pixels) -> HUD design space (y up,
    // 1280x720 bottom-left origin): the inverse of the _hudRoot transform.
    Vec2 screenToDesign(float x, float y) const;
    // Retries the next url with the same token (main.ts joinGame()).
    void tryJoinUrls(const std::vector<std::string>& urls, const std::string& joinToken, size_t index = 0);
    // Pending quick-start: waits out the anti-spam delay, then calls the
    // injected find_game request.
    void runFindGameAttempt();
    void setError(const std::string& key, const std::string& fallback = "");
    void clearError();
    void resetWorld();
    // view.ts resizeGame callback registered on every new GameWorld.
    void onWorldResize(float width, float height);
    // Notch/system-bar safe rect (view points) and the HUD fit/centre inside it.
    ax::Rect safeAreaRect() const;
    void layoutHud(const ax::Rect& safe);
    // main.ts #game-area backdrop: keeps the full-screen black canvas rect
    // (created in init()) in front of the Java menu's translucent scrim.
    void ensureBackdropVisible();

    // Scene child behind the world: a full-screen black rect, so the Java
    // menu's scrim never shows the launcher/home screen through the canvas.
    ax::Node* _backdrop = nullptr;
    ax::Node* _gameRoot = nullptr;
    std::unique_ptr<Game> _game;
    std::unique_ptr<pix::Factory> _pixiFactory;
    // Owned by `_pixiFactory` (like every other adapter node); GameScene only
    // keeps the handle. A unique_ptr here double-deleted the wrapper: the
    // factory already destroys all of its nodes, so resetting this after the
    // factory was replaced freed an already-deleted object (the second-join
    // crash, M4_REMAINING 12.4).
    pix::Container* _worldRoot = nullptr;
    std::unique_ptr<GameWorld> _world;
    // Last viewport size handed to _world (view.ts setSize tracking).
    ax::Size _worldSize;
    bool _worldSized = false;
    // view.setSize() bumps `_worldSizeDirty` through the world's resize callback.
    bool _worldSizeDirty = false;
    // game.ts free() keeps the canvas hidden until the next match; the world
    // container mirrors that so a stale terrain never shows over the menu.
    bool _worldVisible = false;
    std::unique_ptr<audio::AxmolAudioBackend> _audioBackend;
    std::unique_ptr<audio::AudioManager> _audio;
    std::unique_ptr<audio::Ambiance> _ambiance;
    // The pad sprites outlive resetWorld(): they are created once against
    // _gameRoot and merely re-positioned by Touch each frame.
    std::unique_ptr<Touch> _touch;
    std::unique_ptr<PadGraphicsAxmol> _movePad;
    std::unique_ptr<PadGraphicsAxmol> _aimPad;
    std::shared_ptr<std::atomic<bool>> _alive;
    bool _wasConnected = false;
    bool _wasPlaying = false;
    bool _inGame = false;

    // In-game HUD drawn through the pix adapter on _gameRoot (design space).
    std::unique_ptr<Hud> _hud;
    // HUD container on _gameRoot (pix adapter nodes, design space). Owned by
    // `_pixiFactory`; see `_worldRoot` above.
    pix::Container* _hudRoot = nullptr;
    std::unique_ptr<PieTimer> _pieTimer;
    // Crosshair overlay (factor-owned; parented to _gameRoot, not the camera-
    // transformed world root).
    pix::Sprite* _crosshairSprite = nullptr;
    pix::Graphics* _crosshairFallback = nullptr;
    std::string _crosshairFrame;
    FindGameRequest _findGameRequest;
    uikit::UiOverlay* _overlay = nullptr;
    ax::Node* _uiRoot = nullptr;
    Game::JoinInfo _joinInfo;
    // S6: the pause menu is now Java (GamePauseDialog). The C++ _overlay
    // reference is kept as nullptr so the legacy UiOverlay code paths
    // become no-ops. The JavaUiHost owns the pause lifecycle via the bridge.
    bool _pendingFind = false;
    bool _findInFlight = false;
    unsigned _findGeneration = 0;
    bool _notifiedStarted = false;
    float _joinElapsed = 0.0f;
    float _findDelay = 0.0f;
    int _findAttempts = 0;
    float _findTime = 0.0f;
    double _lastAttemptClock = 0.0;
    std::string _region = "na";
    int _gameModeIdx = 0;
    // URL fallback (main.ts joinGame): retry the next advertised host when a
    // join fails before the server ever accepted us.
    bool _awaitingJoin = false;
    std::vector<std::string> _joinRetryUrls;
    std::string _joinRetryToken;
    size_t _joinRetryIndex = 0;
    // M8 region latency test (pingTest.ts). Created lazily by startRegionPingTest().
    std::unique_ptr<PingTest> _pingTest;
    bool _pingReported = false;
    // M8 client error reporting (client/src/errorLogs.ts), ticked at 1 Hz.
    std::unique_ptr<ErrorLog> _errorLog;
    float _errorLogTick = 0.0f;
    // Perf/polish instrumentation (`debug.surv_debug=1`): fps + GL frame size.
    float _perfLogTick = 0.0f;
    ax::Vec2 _perfFrameSize;
    // 60 s soak window summary (min/avg/max fps), for device perf runs.
    float _perfSoakTick = 0.0f;
    float _perfSoakMin = 0.0f;
    float _perfSoakMax = 0.0f;
    float _perfSoakSum = 0.0f;
    int _perfSoakSamples = 0;

    // Priority 5: a dropped mid-match socket reclaims the same match through the
    // server's rejoin ticket (server/src/game/game.ts) before falling back to the
    // menu: immediate, then 1 s, 2 s, 3 s and 4 s apart.
    static constexpr int kReconnectMaxAttempts = 5;
    struct ReconnectState {
        bool active = false;
        int attempt = 0;
        float elapsed = 0.0f;
        float delay = 0.0f;
        std::string url;
        std::string token;
    };
    ReconnectState _reconnect;
    bool tryReconnect();

    TouchInput _touchInput;
    float _inputMsgTimeout = 0.0f;
    bool _shootStartPending = false;

    // Emote wheel (game/EmoteWheel.h): the on-screen emote button + radial
    // loadout selector. Owned by the per-match world (raw pix nodes).
    std::unique_ptr<EmoteWheel> _emoteWheel;
    // Team-ping wheel (game/PingWheel.h): button + danger/coming/help pings and
    // the team-only medical/ammo emotes.
    std::unique_ptr<PingWheel> _pingWheel;
    // Touch id currently driving the wheel (-1 = none); the pad logic ignores
    // that touch so opening the wheel does not also move/aim.
    int _emoteTouchId = -1;
    int _pingTouchId = -1;
    int _bigMapTouchId = -1;
    // Big-map tap target for the next ping (web bigmapPingPos).
    bool _pingTargetValid = false;
    Vec2 _pingTarget;
};

} // namespace surv
