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

namespace surv {

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

    // M3: join directly with a ws:// url + join token (from find_game or a
    // dev/URL param), or discover a match over HTTP first.
    void connectDirect(const std::string& url, const std::string& joinToken);
    void connectViaFindGame(const std::string& apiBaseUrl, const std::string& region, int gameModeIdx);

    // App lifecycle hooks (AppDelegate).
    void pauseGame();
    void resumeGame();

private:
    void maybeAutoConnect();
    // M5: build an InputMsg from Touch and send it at the server's input rate.
    void updateInput(float dt);

    ax::Node* _gameRoot = nullptr;
    std::unique_ptr<Game> _game;
    std::unique_ptr<pix::Factory> _pixiFactory;
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

    TouchInput _touchInput;
    float _inputMsgTimeout = 0.0f;
    bool _shootStartPending = false;
};

} // namespace surv