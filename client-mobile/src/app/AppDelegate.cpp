#include "AppDelegate.h"
#include "GameScene.h"
#include "../net/Net.h"

using namespace ax;

static ax::Size designResolutionSize = ax::Size(1280, 720);

AppDelegate::AppDelegate() {}

AppDelegate::~AppDelegate() {}

void AppDelegate::initGfxContextAttrs() {
    // red, green, blue, alpha, depth, stencil, multisamplesCount
    GfxContextAttrs gfxContextAttrs = {8, 8, 8, 8, 24, 8, 0};
    RenderView::setGfxContextAttrs(gfxContextAttrs);
}

bool AppDelegate::applicationDidFinishLaunching() {
    // Surface definition-registry data errors at startup rather than mid-game.
    surv::initDefinitionRegistries();

    auto director = Director::getInstance();
    auto renderView = director->getRenderView();
    if (!renderView) {
#if (AX_TARGET_PLATFORM != AX_PLATFORM_ANDROID) && (AX_TARGET_PLATFORM != AX_PLATFORM_IOS)
        renderView = RenderViewImpl::createWithRect(
            "SurvevMobile",
            ax::Rect(0, 0, designResolutionSize.width, designResolutionSize.height));
#else
        renderView = RenderViewImpl::create("SurvevMobile");
#endif
        director->setRenderView(renderView);
    }

    director->setStatsDisplay(false);
    director->setAnimationInterval(1.0f / 60);

    // Lock the mobile layout to landscape (like the web client's mobile CSS).
    renderView->setDesignResolutionSize(
        designResolutionSize.width,
        designResolutionSize.height,
        ResolutionPolicy::NO_BORDER);

    auto scene = surv::GameScene::createScene();
    director->runWithScene(scene);
    return true;
}

void AppDelegate::applicationDidEnterBackground() {
    Director::getInstance()->stopAnimation();
    // Pause the game ticker + close the WebSocket gracefully (plan.md 5.8).
    if (auto* scene = dynamic_cast<surv::GameScene*>(Director::getInstance()->getRunningScene())) {
        scene->pauseGame();
    }
}

void AppDelegate::applicationWillEnterForeground() {
    Director::getInstance()->startAnimation();
    // Resume the ticker; the game rejoins and re-syncs from the server snapshot.
    if (auto* scene = dynamic_cast<surv::GameScene*>(Director::getInstance()->getRunningScene())) {
        scene->resumeGame();
    }
}

void AppDelegate::applicationWillQuit() {}
