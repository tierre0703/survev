#pragma once
// axmol application entry point (global namespace, matching the platform
// jni/win32 main.cpp glue). Game code lives in namespace surv.
#include "axmol.h"

namespace surv {
class GameScene;
} // namespace surv

class AppDelegate : private ax::Application {
public:
    AppDelegate();
    ~AppDelegate() override;

    void initGfxContextAttrs() override;
    bool applicationDidFinishLaunching() override;
    void applicationDidEnterBackground() override;
    void applicationWillEnterForeground() override;
    void applicationWillQuit() override;

private:
    // The game scene renders the game canvas; the Java UI handles all menus.
    surv::GameScene* _gameScene = nullptr;
};