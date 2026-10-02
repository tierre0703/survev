#include "AppDelegate.h"
#include "../game/Game.h"
#include "../net/Net.h"
#include "GameScene.h"

USING_NS_AX;

namespace surv {

void AppDelegate::initGLContextAttrs() {
    // OpenGL context attributes: red, green, blue, alpha, depth, stencil, msaa
    GLContextAttrs glContextAttrs = {8, 8, 8, 8, 24, 8, 0};
    glContextAttrs.vsync = true;
    GLView::setGLContextAttrs(glContextAttrs);
}

bool AppDelegate::applicationDidFinishLaunching() {
    // Surface data errors at startup rather than mid-game.
    initDefinitionRegistries();

    auto director = Director::getInstance();
    auto glview = director->getGLView();
    if (!glview) {
        glview = GLViewImpl::create("Survev Mobile");
        director->setGLView(glview);
    }

    // Lock to landscape like the web client's mobile layout.
    glview->setDesignResolutionSize(1280, 720, ResolutionPolicy::NO_BORDER);

    director->setDisplayStats(false);
    director->setAnimationInterval(1.0f / 60.0f);

    auto scene = GameScene::createScene();
    director->runWithScene(scene);
    return true;
}

void AppDelegate::applicationDidEnterBackground() {
    Director::getInstance()->stopAnimation();
    // TODO(M6): pause game ticker + graceful WebSocket close (plan.md 5.8).
    // Game::getInstance()->onAppBackground();
}

void AppDelegate::applicationWillEnterForeground() {
    Director::getInstance()->startAnimation();
    // TODO(M6): resume ticker; re-sync via server snapshot.
    // Game::getInstance()->onAppForeground();
}

void AppDelegate::applicationScreenSizeChanged(int newWidth, int newHeight) {
    auto director = Director::getInstance();
    auto glview = director->getGLView();
    if (glview && !glview->getFrameSize().equals(Size(newWidth, newHeight))) {
        glview->setFrameSize(newWidth, newHeight);
        // TODO: propagate to device.onResize() equivalent (plan.md 5.8).
    }
}

} // namespace surv