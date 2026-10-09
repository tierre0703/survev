#include "AppDelegate.h"
#include "GameScene.h"
#include "MenuScene.h"
#include "DevConfig.h"
#include "../net/Api.h"
#include "../net/Net.h"
#include "../ui/Files.h"
#include "../ui/Ui.h"
#include "../ui/UiOverlay.h"

using namespace ax;

// Declared in Ui.h; the explicit `::surv::` scope is required because this
// translation unit also has axmol's `ax::ui` in scope via `using namespace ax`.
namespace surv { namespace ui { void registerGuiFonts(const char* dir); } }

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

    // The app is permanently landscape: the design resolution is a fixed
    // 1280x720 and NO_BORDER letterboxes/crops instead of rotating.
    renderView->setDesignResolutionSize(
        designResolutionSize.width,
        designResolutionSize.height,
        ResolutionPolicy::NO_BORDER);

    // The web client's UI font is Roboto Condensed (woff2). Stage the TTF copy
    // (tools/fetch-fonts.ps1) and register it before any Label is created.
    ::surv::ui::registerGuiFonts("fonts");
    // One persistent scene keeps the team socket and matchmaking ticking even
    // while their UI is hidden. Node ownership avoids dangling replaceScene pointers.
    auto* menuScene = surv::MenuScene::createScene();
    auto* gameScene = surv::GameScene::createScene();

    if (menuScene && gameScene) {
        menuScene->addChild(gameScene, 1);
        gameScene->setVisible(false);
        menuScene->bindGame(gameScene);

        // The menu owns the find_game HTTP call; the scene only schedules it.
        gameScene->setFindGameRequest(
            [](const std::string& region, int gameModeIdx,
               surv::GameScene::FindGameDone done) {
                surv::FindGameBody body;
                body.region = region;
                body.version = surv::defs::kProtocolVersion;
                body.playerCount = 1;
                body.autoFill = true;
                body.gameModeIdx = gameModeIdx;
                const std::string apiUrl(
                    ax::UserDefault::getInstance()->getStringForKey(
                        surv::dev::kKeyApiUrl, surv::dev::kApiBaseUrl));
                surv::findGame(apiUrl, body, [done](surv::FindGameResult result) {
                    surv::GameScene::FindGameResultInfo info;
                    info.ok = result.ok;
                    info.urls = result.data.urls;
                    info.joinToken = result.data.joinToken;
                    info.error = result.error;
                    if (done) {
                        done(info);
                    }
                });
            });

    }

    director->runWithScene(menuScene ? static_cast<ax::Scene*>(menuScene)
                                     : static_cast<ax::Scene*>(gameScene));
    _menuScene = menuScene;
    _gameScene = gameScene;
    return true;
}

void AppDelegate::applicationDidEnterBackground() {
    Director::getInstance()->stopAnimation();
    // Pause the game ticker + close the WebSocket gracefully (plan.md 5.8).
    if (_gameScene) {
        _gameScene->pauseGame();
    }
    if (_menuScene) {
        _menuScene->pauseMenu();
    }
}

void AppDelegate::applicationWillEnterForeground() {
    Director::getInstance()->startAnimation();
    // Resume the ticker; the game rejoins and re-syncs from the server snapshot.
    if (_gameScene) {
        _gameScene->resumeGame();
    }
    if (_menuScene) {
        _menuScene->resumeMenu();
    }
}

void AppDelegate::applicationWillQuit() {}
