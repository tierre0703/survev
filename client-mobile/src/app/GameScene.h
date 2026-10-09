#pragma once
#include "axmol.h"
#include "../game/Game.h"
#include "../game/TouchInput.h"
#include "../net/Net.h"
#include "../render/PixiLike.h"
#include "../ui/Touch.h"
#include <atomic>
#include <memory>

namespace ax {
class Node;
}
namespace pix {
class Factory;
}
namespace ui {
class UiOverlay; // M7 in-game UI (HUD + pause menu)
}

namespace surv {

// The global UI toolkit (see ui/Ui.h). Aliased inside `surv` so `uikit::` reads
// like the web `ui.ts` while staying distinct from the vestigial `surv::ui`
// forward declarations above.
namespace uikit = ::ui;

class GameWorld;

namespace audio {
class AxmolAudioBackend;
class AudioManager;
class Ambiance;
} // namespace audio

// axmol PadGraphics implementation (draws the joystick circles).
class TouchPadGfx : public PadGraphics {
public:
    explicit TouchPadGfx(ax::Node* parent);
    void setVisible(bool visible) override;
    void setCenterPos(float x, float y) override;
    void setTouchPos(float x, float y) override;
    void setCenterScale(float s) override;
    void setTouchScale(float s) override;
    void update() override;

private:
    ax::DrawNode* _center = nullptr;
    ax::DrawNode* _touch = nullptr;
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

private:
    // M5: build an InputMsg from Touch and send it at the server's input rate.
    void updateInput(float dt);
    // Retries the next url with the same token (main.ts joinGame()).
    void tryJoinUrls(const std::vector<std::string>& urls, const std::string& joinToken, size_t index = 0);
    // Pending quick-start: waits out the anti-spam delay, then calls the
    // injected find_game request.
    void runFindGameAttempt();
    void setError(const std::string& key, const std::string& fallback = "");
    void clearError();
    void resetWorld();

    ax::Node* _gameRoot = nullptr;
    std::unique_ptr<Game> _game;
    std::unique_ptr<pix::Factory> _pixiFactory;
    std::unique_ptr<pix::Container> _worldRoot;
    std::unique_ptr<GameWorld> _world;
    std::unique_ptr<audio::AxmolAudioBackend> _audioBackend;
    std::unique_ptr<audio::AudioManager> _audio;
    std::unique_ptr<audio::Ambiance> _ambiance;
    std::unique_ptr<Touch> _touch;
    std::unique_ptr<TouchPadGfx> _movePad;
    std::unique_ptr<TouchPadGfx> _aimPad;
    std::shared_ptr<std::atomic<bool>> _alive;
    bool _wasConnected = false;
    bool _wasPlaying = false;
    bool _inGame = false;

    // M7: matchmaking state (findGameAttempts/timers mirrored from main.ts).
    FindGameRequest _findGameRequest;
    uikit::UiOverlay* _overlay = nullptr;
    ax::Node* _uiRoot = nullptr;
    Game::JoinInfo _joinInfo;
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

    TouchInput _touchInput;
    float _inputMsgTimeout = 0.0f;
    bool _shootStartPending = false;
};

} // namespace surv
