#pragma once
#include "axmol.h"

namespace surv {

// axmol app entry point. Mirrors the axmol template AppDelegate.
class AppDelegate : public ax::Application {
public:
    bool applicationDidFinishLaunching() override;
    void applicationDidEnterBackground() override;
    void applicationWillEnterForeground() override;
    void applicationScreenSizeChanged(int newWidth, int newHeight) override;
    void initGLContextAttrs() override;
};

} // namespace surv