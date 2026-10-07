#include "AppDelegate.h"
#include "GameScene.h"
#include "MenuScene.h"
#include "../net/Api.h"
#include "../net/Net.h"
#include "../ui/UiOverlay.h"

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

    // M7: the menu and the game are sibling scenes; the menu drives matchmaking
    // and switches which one is running.
    auto* menuScene = surv::MenuScene::createScene();
    auto* gameScene = surv::GameScene::createScene();

    if (menuScene && gameScene) {
        auto* overlay = menuScene->overlay();
        if (overlay) {
            overlay->build(gameScene, const_cast<surv::ui::Config*>(&menuScene->config()),
                           nullptr, gameScene->getTouch());
            overlay->onQuit = [menuScene, gameScene] {
                gameScene->leaveGame();
                menuScene->onMatchEnded();
            };
        }
        gameScene->setOverlay(overlay);

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

        surv::MenuScene::GameHooks hooks;
        hooks.scene = [gameScene] { return gameScene; };
        hooks.joinInfo = [menuScene] { return menuScene->joinInfo(); };
        hooks.onMatchStarted = [director, gameScene] { director->replaceScene(gameScene); };
        hooks.onReturnToMenu = [director, menuScene] { director->replaceScene(menuScene); };
        menuScene->setGameHooks(hooks);
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
