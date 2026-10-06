#pragma once
// axmol application entry point (global namespace, matching the platform
// jni/win32 main.cpp glue). Game code lives in namespace surv.
#include "axmol.h"

class AppDelegate : private ax::Application {
public:
    AppDelegate();
    ~AppDelegate() override;

    void initGfxContextAttrs() override;
    bool applicationDidFinishLaunching() override;
    void applicationDidEnterBackground() override;
    void applicationWillEnterForeground() override;
    void applicationWillQuit() override;
};
