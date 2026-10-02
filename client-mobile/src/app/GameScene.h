#pragma once
#include "axmol.h"
#include "../game/Game.h"
#include "../net/Net.h"
#include "../render/PixiLike.h"
#include "../ui/Touch.h"

namespace surv {

// Main scene: owns the game (simulation + net), the renderer adapter,
// and the touch joysticks. This is the axmol counterpart of
// client/src/game.ts + main.ts glue for the in-game surface.
class GameScene : public ax::Scene {
public:
    static ax::Scene* createScene();

    bool init() override;
    void update(float delta) override;

    // Input callbacks -> Touch
    void onTouchBegan(float x, float y, int id);
    void onTouchMoved(float x, float y, int id);
    void onTouchEnded(int id);

    ax::Node* getGameRoot() const { return _gameRoot; }

private:
    ax::Node* _gameRoot = nullptr;
    std::unique_ptr<Game> _game;
    std::unique_ptr<Touch> _touch;

    // axmol implementations of PadGraphics
    std::unique_ptr<TouchPadGfx> _movePad;
    std::unique_ptr<TouchPadGfx> _aimPad;
};

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
    ax::Sprite* _center = nullptr;
    ax::Sprite* _touch = nullptr;
};

} // namespace surv