#include "GameScene.h"
#include "UiHost.h"
#include "DevConfig.h"
#include "../net/Api.h"
#include "../net/ErrorLog.h"
#include "../net/Messages.h"
#include "../net/Regions.h"
#include "../net/WebSocketConnection.h"
#include "../game/GameWorld.h"
#include "../game/DisconnectReason.h"
#include "../game/Map.h"
#include "../game/objects/Barns.h"
#include "../render/AxmolPixi.h"
#include "../render/PixiLike.h"
#include "../render/Defs.h"
#include "../render/GeneratedDefs.h"
#include "../audio/AudioManager.h"
#include "../audio/Ambiance.h"
#include "../audio/AxmolAudioBackend.h"
#include "../audio/GeneratedSoundDefs.h"
#include "../ui/FguiHud.h"
#include "../game/Hud.h"
#include "../game/RoleFeed.h"
#include "../game/EmoteWheel.h"
#include "../game/PingWheel.h"
#include "../ui/FguiPieTimer.h"
#include "../ui/PadGraphicsAxmol.h"
#include "../ui/Layout.h"
#include "../../../shared/config/ui2Constants.inc"

#include "fairygui/GRoot.h"
#include "fairygui/FairyGUI.h"

#if AX_TARGET_PLATFORM == AX_PLATFORM_ANDROID
#include "platform/android/jni/JniHelper.h"
#include <android/keycodes.h>
#endif

#include "renderer/TextureCache.h"

#include <set>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

USING_NS_AX;

namespace surv {
namespace svui = ::ui;

// M4/T1: register the sprite atlases a map needs with the SpriteFrameCache.
// tools/build-atlas.mjs writes Content/atlas/<atlas>/{*.plist,index.txt}; the
// plist frame keys are the web client's `*.img` names, so AxSprite::setFrame()
// resolves them directly. Loading is deferred until the map arrives because the
// map def lists its atlases. Like resources.ts loadMapAssets, atlases that are
// not in the new map's list are dropped so the next match does not inherit the
// previous map's sheets.
static std::map<std::string, std::vector<std::string>> s_loadedAtlasSheets;

static void unloadAtlasesNotIn(const std::set<std::string>& keep) {
    if (s_loadedAtlasSheets.empty()) {
        return;
    }
    auto* cache = ax::SpriteFrameCache::getInstance();
    for (auto it = s_loadedAtlasSheets.begin(); it != s_loadedAtlasSheets.end();) {
        if (keep.count(it->first)) {
            ++it;
            continue;
        }
        for (const auto& sheet : it->second) {
            cache->removeSpriteFramesFromFile(sheet);
        }
        AXLOGI("Unloaded atlas {}", it->first);
        it = s_loadedAtlasSheets.erase(it);
    }
    // Frames are gone; drop any sheet texture no live sprite references any
    // more (resources.ts unloadAtlas destroys the base textures).
    if (auto* director = ax::Director::getInstance()) {
        if (auto* textures = director->getTextureCache()) {
            textures->removeUnusedTextures();
        }
    }
}

static void loadAtlasesForMap(const std::string& mapName) {
    const DefProvider* provider = getDefProvider();
    const MapRenderDef* mapDef = provider ? provider->mapRender(mapName) : nullptr;
    if (!mapDef) {
        static std::set<std::string> reported;
        if (reported.insert(mapName).second) {
            uiBridge()->reportError("mapDefMissing",
                                    "{\"map\":\"" + mapName + "\"}");
        }
        return;
    }
    std::set<std::string> needed;
    for (const auto& atlas : mapDef->atlases) {
        if (!atlas.empty()) {
            needed.insert(atlas);
        }
    }
    // Unload first (like the web) so shared/loadout sheets stay resident and
    // only the previous map's own sheets are released.
    unloadAtlasesNotIn(needed);

    auto* cache = ax::SpriteFrameCache::getInstance();
    auto* fileUtils = ax::FileUtils::getInstance();
    for (const auto& atlas : mapDef->atlases) {
        if (atlas.empty() || s_loadedAtlasSheets.count(atlas)) {
            continue;
        }
        const std::string index = "atlas/" + atlas + "/index.txt";
        const std::string list = fileUtils->getStringFromFile(index);
        if (list.empty()) {
            AXLOGW("Atlas index missing ({}); run tools/build-atlas.mjs", index);
            static std::set<std::string> reported;
            if (reported.insert(atlas).second) {
                uiBridge()->reportError(
                    "atlasMissing",
                    "{\"map\":\"" + mapName + "\",\"atlas\":\"" + atlas + "\"}");
            }
            continue;
        }
        std::vector<std::string> sheets;
        std::istringstream stream(list);
        std::string line;
        while (std::getline(stream, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (line.empty()) {
                continue;
            }
            cache->addSpriteFramesWithFile(line);
            sheets.push_back(line);
        }
        s_loadedAtlasSheets[atlas] = std::move(sheets);
        AXLOGI("Loaded atlas {}", atlas);
    }
}

// S2: FairyGUI GRoot for the UI layer. Created once in init(); the in-game HUD
// and pads now use plain pix/axmol nodes, so this is only needed if a
// FairyGUI-based widget (e.g. FguiHud) is enabled again.
static fairygui::GRoot* s_fguiRoot = nullptr;

fairygui::GRoot* fguiRoot() { return s_fguiRoot; }

// ui2.ts getPickupMessageText (PickupMsgType order).
static const char* pickupErrorText(int type) {
    switch (type) {
        case 0: return "NOT ENOUGH SPACE";
        case 1: return "ITEM ALREADY OWNED";
        case 2: return "ITEM ALREADY EQUIPPED";
        case 3: return "BETTER ITEM ALREADY EQUIPPED";
        case 5: return "GUN CANNOT FIRE";
        case 6: return "MAX PERKS EQUIPPED";
        default: return "CANNOT PICK UP";
    }
}

// inputBinds.ts default key codes -> input.ts `Key` codes. Both the shifted
// (KEY_CAPITAL_*) and plain letter codes are accepted, and the Android DPAD
// variants behave like the arrow keys.
static int webKeyFromEvent(ax::EventKeyboard::KeyCode code) {
    using KC = ax::EventKeyboard::KeyCode;
    switch (code) {
        case KC::KEY_A:
        case KC::KEY_CAPITAL_A: return input::key::A;
        case KC::KEY_B:
        case KC::KEY_CAPITAL_B: return input::key::B;
        case KC::KEY_C:
        case KC::KEY_CAPITAL_C: return input::key::C;
        case KC::KEY_D:
        case KC::KEY_CAPITAL_D: return input::key::D;
        case KC::KEY_E:
        case KC::KEY_CAPITAL_E: return input::key::E;
        case KC::KEY_F:
        case KC::KEY_CAPITAL_F: return input::key::F;
        case KC::KEY_G:
        case KC::KEY_CAPITAL_G: return input::key::G;
        case KC::KEY_H:
        case KC::KEY_CAPITAL_H: return input::key::H;
        case KC::KEY_I:
        case KC::KEY_CAPITAL_I: return input::key::I;
        case KC::KEY_J:
        case KC::KEY_CAPITAL_J: return input::key::J;
        case KC::KEY_K:
        case KC::KEY_CAPITAL_K: return input::key::K;
        case KC::KEY_L:
        case KC::KEY_CAPITAL_L: return input::key::L;
        case KC::KEY_M:
        case KC::KEY_CAPITAL_M: return input::key::M;
        case KC::KEY_N:
        case KC::KEY_CAPITAL_N: return input::key::N;
        case KC::KEY_O:
        case KC::KEY_CAPITAL_O: return input::key::O;
        case KC::KEY_P:
        case KC::KEY_CAPITAL_P: return input::key::P;
        case KC::KEY_Q:
        case KC::KEY_CAPITAL_Q: return input::key::Q;
        case KC::KEY_R:
        case KC::KEY_CAPITAL_R: return input::key::R;
        case KC::KEY_S:
        case KC::KEY_CAPITAL_S: return input::key::S;
        case KC::KEY_T:
        case KC::KEY_CAPITAL_T: return input::key::T;
        case KC::KEY_U:
        case KC::KEY_CAPITAL_U: return input::key::U;
        case KC::KEY_V:
        case KC::KEY_CAPITAL_V: return input::key::V;
        case KC::KEY_W:
        case KC::KEY_CAPITAL_W: return input::key::W;
        case KC::KEY_X:
        case KC::KEY_CAPITAL_X: return input::key::X;
        case KC::KEY_Y:
        case KC::KEY_CAPITAL_Y: return input::key::Y;
        case KC::KEY_Z:
        case KC::KEY_CAPITAL_Z: return input::key::Z;
        case KC::KEY_0: return input::key::Zero;
        case KC::KEY_1: return input::key::One;
        case KC::KEY_2: return input::key::Two;
        case KC::KEY_3: return input::key::Three;
        case KC::KEY_4: return input::key::Four;
        case KC::KEY_5: return input::key::Five;
        case KC::KEY_6: return input::key::Six;
        case KC::KEY_7: return input::key::Seven;
        case KC::KEY_8: return input::key::Eight;
        case KC::KEY_9: return input::key::Nine;
        case KC::KEY_LEFT_ARROW:
        case KC::KEY_DPAD_LEFT: return input::key::Left;
        case KC::KEY_RIGHT_ARROW:
        case KC::KEY_DPAD_RIGHT: return input::key::Right;
        case KC::KEY_UP_ARROW:
        case KC::KEY_DPAD_UP: return input::key::Up;
        case KC::KEY_DOWN_ARROW:
        case KC::KEY_DPAD_DOWN: return input::key::Down;
        case KC::KEY_SPACE: return input::key::Space;
        case KC::KEY_TAB: return input::key::Tab;
        case KC::KEY_RETURN: return input::key::Enter;
        case KC::KEY_BACKSPACE: return input::key::Backspace;
        case KC::KEY_DELETE: return input::key::Delete;
        case KC::KEY_F1: return input::key::F1;
        case KC::KEY_F2: return input::key::F2;
        case KC::KEY_F3: return input::key::F3;
        case KC::KEY_F4: return input::key::F4;
        case KC::KEY_F5: return input::key::F5;
        case KC::KEY_F6: return input::key::F6;
        case KC::KEY_F7: return input::key::F7;
        case KC::KEY_F8: return input::key::F8;
        case KC::KEY_F9: return input::key::F9;
        case KC::KEY_F10: return input::key::F10;
        case KC::KEY_F11: return input::key::F11;
        case KC::KEY_F12: return input::key::F12;
        default: return -1;
    }
}

// ---------------------------------------------------------------------------
// TouchPadGfx (touch.ts pad.img sprites) - legacy, no longer instantiated
// (the live pads are PadGraphicsAxmol). Kept as an atlas-based fallback.
// ---------------------------------------------------------------------------
// Port of touch.ts createPadSprite(): the joystick ring is the web client's
// `pad.img` (client/public/img/gui/pad.svg, a grey circle stroke). The web
// client tints the centre black at 20% alpha and the touch dot white, both
// anchored at 0.5; the scale comes from Touch (padScaleBase * 0.6 / 0.25).
// `pad.img` lives in the virtual atlas (Content/atlas/**) because that is how
// the web client resolves it; `gui/pad.png` (tools/build-gui-icons.mjs) is the
// loose fallback for a build whose atlas cache is unavailable.
TouchPadGfx::TouchPadGfx(ax::Node* parent) {
    // touch.ts's 0.6/0.25 sprite scales are relative to the *point* size of the
    // 208 px `pad.img` art drawn at 0.5 device/point scale, i.e. 125 px / 52 px
    // on screen. `gui/pad.png` (the loose fallback) is 4x that, so it needs 1/4
    // of the same scale to land on the same on-screen size.
    const bool frameReady =
        ax::SpriteFrameCache::getInstance()->getSpriteFrameByName("pad.img") != nullptr;
    _padFrame = frameReady ? "pad.img" : "gui/pad.png";
    _padScale = frameReady ? 0.25f : 0.0625f;

    auto createPadSprite = [this, parent](const ax::Color3B& tint, float alpha) -> ax::Sprite* {
        ax::Sprite* sprite = ax::Sprite::create(_padFrame);
        if (!sprite) {
            return nullptr;
        }
        sprite->setAnchorPoint(ax::Vec2(0.5f, 0.5f));
        sprite->setColor(tint);
        sprite->setOpacity(static_cast<uint8_t>(alpha * 255.0f + 0.5f));
        sprite->setVisible(false);
        parent->addChild(sprite, 1000);
        return sprite;
    };
    _center = createPadSprite(ax::Color3B(0, 0, 0), 0.2f);
    _touch = createPadSprite(ax::Color3B::WHITE, 1.0f);

    // The pad sprites are created from the menu, before any map (and therefore
    // before the atlas) is loaded; `_atlasPad` reports whether the atlas frame
    // is what is currently on the sprites, and `refreshPadFrame` re-resolves it
    // once a map load has registered the atlas.
    if (_center) {
        _textureReady = _center->getTexture() != nullptr;
        _atlasPad = _textureReady && _padFrame == "pad.img";
    }
    // Always start hidden; setVisible(false) hides either sprite.
    setVisible(false);
}

void TouchPadGfx::refreshPadFrame() {
    if (_atlasPad || !_center) {
        return;
    }
    if (!ax::SpriteFrameCache::getInstance()->getSpriteFrameByName("pad.img")) {
        return;
    }
    _padFrame = "pad.img";
    _padScale = 0.25f;
    _atlasPad = true;
    for (ax::Sprite* sprite : {_center, _touch}) {
        if (sprite) {
            sprite->setSpriteFrame("pad.img");
        }
    }
}

bool TouchPadGfx::hasTexture() const { return _textureReady; }
bool TouchPadGfx::isAtlasPad() const { return _atlasPad; }
const std::string& TouchPadGfx::padFrame() const { return _padFrame; }

void TouchPadGfx::setVisible(bool visible) {
    if (_center) _center->setVisible(visible);
    if (_touch) _touch->setVisible(visible);
}

// `scale` is the raw product Touch computes (touch.ts scales the pad.img
// sprites by padScaleBase * padScaleDown for the ring and * padScalePos for the
// dot); `_padScale` maps that onto the texture actually in use.
void TouchPadGfx::sizeSprite(ax::Sprite* sprite, float scale) {
    if (sprite) {
        sprite->setScale(scale * _padScale);
    }
}

void TouchPadGfx::setCenterPos(float x, float y) {
    if (_center) _center->setPosition(ax::Vec2(x, y));
}

void TouchPadGfx::setTouchPos(float x, float y) {
    if (_touch) _touch->setPosition(ax::Vec2(x, y));
}

void TouchPadGfx::setCenterScale(float s) {
    sizeSprite(_center, s);
}

void TouchPadGfx::setTouchScale(float s) {
    sizeSprite(_touch, s);
}

void TouchPadGfx::update() {}

// ---------------------------------------------------------------------------
// GameScene
// ---------------------------------------------------------------------------
GameScene* GameScene::createScene() {
    auto scene = new GameScene();
    if (scene && scene->init()) {
        scene->autorelease();
        return scene;
    }
    AX_SAFE_DELETE(scene);
    return nullptr;
}

bool GameScene::init() {
    if (!Scene::init()) {
        return false;
    }

    const ax::Size visible = ax::Director::getInstance()->getVisibleSize();
    const bool isLandscape = visible.width >= visible.height;

    // device.ts on a native app: there is no user agent, so `tablet` is
    // classified from the physical screen (GLView frame pixels + DPI); the web
    // reads it from the browser device database. One call is enough for a
    // landscape-locked Android app (there is no mid-session re-classify).
    {
        auto* glView = ax::Director::getInstance()->getGLView();
        const ax::Size frame = glView ? glView->getFrameSize() : visible;
        const int dpi = ax::Device::getDPI();
        svui::Device::get().resize(static_cast<float>(frame.width),
                                   static_cast<float>(frame.height),
                                   ax::Device::getPixelRatio(), dpi);
        const svui::Device& device = svui::Device::get();
        AXLOGI("[device] {} dpi={} ratio={} screen={}x{} landscape={}",
               device.tablet ? "tablet" : "phone", device.dpi, device.pixelRatio, frame.width,
               frame.height, device.isLandscape ? 1 : 0);
        // resources.ts selectTextureRes: log which spritesheet tier the web
        // client would choose (the native pipeline ships only low-res sheets;
        // `highResTex` is the EGL GFX tier here, REMAININGS §3.5).
        const auto tier = device.selectTextureRes(
            false, static_cast<unsigned int>(
                       ax::Configuration::getInstance()->getMaxTextureSize()),
            static_cast<float>(frame.width) / device.pixelRatio,
            static_cast<float>(frame.height) / device.pixelRatio, device.pixelRatio);
        AXLOGI("[device] textureRes={} maxTextureSize={}",
               tier == svui::Device::TextureRes::High ? "high" : "low",
               ax::Configuration::getInstance()->getMaxTextureSize());
        // touch.ts resize() reads device.tablet for the pad offsets; Touch is
        // created below.
    }

    // S2: create the FairyGUI root for gameplay HUD overlay.
    s_fguiRoot = fairygui::GRoot::create(this, 500);
    if (!s_fguiRoot) {
        AXLOGW("Failed to create FairyGUI root; HUD will be missing");
    } else {
        AXLOGI("FairyGUI GRoot created");
    }

    // The gameplay canvas backdrop. Before the world is built (and whenever no
    // match is running) the scene would otherwise clear to black, so the Java
    // menu's scrim showed the translucent launcher/home screen through it.
    // main.ts paints #game-area solid black; do the same here. It sits behind
    // the world layers.
    _backdrop = ax::LayerColor::create(ax::Color4B(0, 0, 0, 255),
                                       visible.width, visible.height);
    _backdrop->setLocalZOrder(-1);
    this->addChild(_backdrop);

    _gameRoot = ax::Node::create();
    this->addChild(_gameRoot);

    // Dual-joystick touch input (multi-touch).
    auto listener = ax::EventListenerTouchAllAtOnce::create();
    // Touch/GUI code (touch.ts) works in web screen space: origin top-left,
    // y increasing downwards. axmol hands us design-space points with y up, so
    // flip y here; the pad renderer flips it back for the axmol scene graph.
    auto webY = [](const ax::Vec2& p) {
        return ax::Director::getInstance()->getVisibleSize().height - p.y;
    };
    listener->onTouchesBegan = [this, webY](const std::vector<ax::Touch*>& touches, ax::Event*) {
        for (auto* t : touches) {
            const ax::Vec2 p = t->getLocation();
            onTouchBegan(p.x, webY(p), t->getID() & 3);
        }
    };
    listener->onTouchesMoved = [this, webY](const std::vector<ax::Touch*>& touches, ax::Event*) {
        for (auto* t : touches) {
            const ax::Vec2 p = t->getLocation();
            onTouchMoved(p.x, webY(p), t->getID() & 3);
        }
    };
    listener->onTouchesEnded = [this, webY](const std::vector<ax::Touch*>& touches, ax::Event*) {
        for (auto* t : touches) {
            const ax::Vec2 p = t->getLocation();
            onTouchEnded(p.x, webY(p), t->getID() & 3);
        }
    };
    listener->onTouchesCancelled = [this, webY](const std::vector<ax::Touch*>& touches, ax::Event*) {
        for (auto* t : touches) {
            const ax::Vec2 p = t->getLocation();
            onTouchEnded(p.x, webY(p), t->getID() & 3);
        }
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

    _touch = std::make_unique<Touch>(visible.width, visible.height, isLandscape);
    // Dual-joystick graphics: plain axmol sprites/draw nodes on the game root
    // (see PadGraphicsAxmol), updated from Touch every frame.
    _movePad = std::make_unique<PadGraphicsAxmol>(_gameRoot, true);
    _aimPad = std::make_unique<PadGraphicsAxmol>(_gameRoot, false);
    // Touch keeps feeding setCenterScale/setTouchScale with the raw touch.ts
    // ratios (asserted by the host tests); TouchPadGfx::sizeSprite maps those
    // onto the texture actually in use, so the on-screen ring size no longer
    // depends on the sprite's pixel size (the 208 px atlas frame vs the 4x
    // gui/pad.png fallback). Prime the sprite scale once here.
    _touch->m_update(*_movePad, *_aimPad);

    _alive = std::make_shared<std::atomic<bool>>(true);

    _game = std::make_unique<Game>();
    _game->init(this, createWebSocketConnection);

    // Priority 2: build the pie timer.
    _pieTimer = std::make_unique<PieTimer>();
    _pieTimer->build();

    // M4: render world (terrain + z-sorted object layers) driven by the map /
    // update messages the game dispatches.
    installGeneratedDefs();

    // M7: the menu starts a game via enterWithJoin()/enterWithFindGame().
    _gameRoot->setVisible(false);
    // The atlas converter emits the low-resolution sheets (the web client's
    // mobile path); 0.5 makes axmol render those frames at full point size.
    ax::Director::getInstance()->setContentScaleFactor(0.5f);

    // M6: audio backend + manager + ambience mixer.
    audio::installGeneratedSoundDefs();
    _audioBackend = std::make_unique<audio::AxmolAudioBackend>();
    _audio = std::make_unique<audio::AudioManager>(_audioBackend.get());
    _ambiance = std::make_unique<audio::Ambiance>();
    _audio->preloadSounds();

    _pixiFactory = std::make_unique<pix::AxPixiFactory>();
    // The pads use the bundled gui/pad.png (not a map atlas), so a map load
    // does not need to refresh them.
    _game->setAtlasCallback([this] {
        // No-op.
    });
    resetWorld();
    // ui2.ts reads the device scale (points per CSS pixel, i.e. the uiRoot
    // scale) for every HUD size; the camera carries it to the HUD.
    if (_world) {
        _world->setDeviceScale(::ui2::kDeviceScale);
    }
    // init() deliberately leaves the world null (resetWorld() only builds it on
    // a join); guard the sizing below.
    if (_world) {
        // Size the layers for the viewport now and again whenever the director
        // reports a real size (the world is built before the GL view exists, so
        // the size at init() can still be the default).
        const auto visible = ax::Director::getInstance()->getVisibleSize();
        _world->setScreenSize(visible.width, visible.height);
        _worldSized = true;
    }
    _game->setMapCallback([this](const MapMsg& msg) {
        loadAtlasesForMap(msg.mapName);
        // The emote/ping wheels are built in resetWorld(), before the map
        // atlases are registered; re-resolve their icon frames now.
        if (_emoteWheel) {
            _emoteWheel->refreshFrames();
        }
        if (_pingWheel) {
            _pingWheel->refreshFrames();
        }
        if (_world) {
            // The world (and its terrain node) was attached by resetWorld()
            // before this MapMsg arrived; loadMap renders the terrain in place.
            _world->loadMap(msg);
            // map.ts renderMap(): the minimap texture is baked during loadMap.
            if (_hud) {
                _hud->setMinimap(_world->map().getMinimapTexture(),
                                 _world->map().width, _world->map().height);
                _hud->setMapLabels(msg.places);
            }
        }
        // game.ts MapMsg: on a perk map, show the class picker until the local
        // player has a role; otherwise close it (ui.ts setRoleMenuOptions).
        if (_hud && _game) {
            const DefProvider* provider = getDefProvider();
            const MapRenderDef* mapDef = provider ? provider->mapRender(msg.mapName) : nullptr;
            if (mapDef && mapDef->perkMode && _game->activePlayerData().role.empty()) {
                _hud->setRoleMenuOptions(uiHost()->configString("perkModeRole"),
                                         mapDef->perkModeRoles);
                _hud->setRoleMenuActive(true);
                if (_audio && _roleMenuSound < 0) {
                    audio::PlaySoundOptions opts;
                    opts.channel = "ambient";
                    _roleMenuSound = _audio->playSound("ambient_lab_01", opts);
                }
            } else {
                _hud->setRoleMenuActive(false);
            }
        }
    });
    _game->setUpdateCallback([this](const UpdateMsg& msg) {
        // A Map/Update arriving while no world exists (the world is built in
        // enterWithJoin()) must not dereference a null world.
        if (_world) {
            _world->applyUpdate(msg);
            // shot.ts cycle/pull: the local gun's clip ammo from the active
            // player snapshot (the native client has no predicted clip).
            // player.ts dry fire also needs the equipped gun's reserve ammo
            // (inventory count for its ammo type), so publish both here.
            const ActivePlayerData& apd = _game->activePlayerData();
            int ammo = -1;
            int reserve = -1;
            if (apd.curWeapIdx >= 0 && apd.curWeapIdx < static_cast<int>(apd.weapons.size())) {
                const auto& weap = apd.weapons[static_cast<size_t>(apd.curWeapIdx)];
                ammo = weap.ammo;
                const DefProvider* provider = getDefProvider();
                const GameObjRenderDef* def = (!weap.type.empty() && provider)
                    ? provider->gameObject(weap.type)
                    : nullptr;
                if (def && !def->ammoType.empty()) {
                    reserve = Hud::bagCount(apd.inventory, def->ammoType.c_str());
                }
            }
            _world->setLocalAmmo(ammo, reserve);
            _world->setLocalPlayerId(_game->getLocalPlayerId());
        }
    });

    _game->setPickupSoundCallback(
        [this](const std::string& item) { onPickupSound(item); });
    _game->setPickupErrorCallback([this](int type) {
        if (_hud) {
            _hud->displayPickupMessage(pickupErrorText(type));
        }
    });

    // Android hardware back button / desktop Escape: pause/leave the game.
    // P1.6: every other key feeds HardwareInput through the bind table.
    auto* keyListener = ax::EventListenerKeyboard::create();
    keyListener->onKeyPressed = [this](ax::EventKeyboard::KeyCode code, ax::Event*) {
        if (code == ax::EventKeyboard::KeyCode::KEY_ESCAPE) {
            handleBackPressed();
            return;
        }
        const int key = webKeyFromEvent(code);
        if (key >= 0) {
            _hw.setKey(key, true);
        }
    };
    keyListener->onKeyReleased = [this](ax::EventKeyboard::KeyCode code, ax::Event*) {
        const int key = webKeyFromEvent(code);
        if (key >= 0) {
            _hw.setKey(key, false);
        }
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyListener, this);

    // Bluetooth/emulator mouse: cursor aim + left-click fire + wheel cycling.
    // `getLocation()` is GL space (y up), so flip it into web screen space like
    // the touch listener does.
    auto* mouseListener = ax::EventListenerMouse::create();
    auto mousePos = [](ax::EventMouse* e) {
        const ax::Vec2 p = e->getLocation();
        const float h = ax::Director::getInstance()->getVisibleSize().height;
        return Vec2(p.x, h - p.y);
    };
    mouseListener->onMouseMove = [this, mousePos](ax::EventMouse* e) {
        const Vec2 p = mousePos(e);
        _hw.setMousePos(p.x, p.y);
        return false;
    };
    mouseListener->onMouseDown = [this, mousePos](ax::EventMouse* e) {
        const Vec2 p = mousePos(e);
        _hw.setMousePos(p.x, p.y);
        _hw.setMouseButton(static_cast<int>(e->getMouseButton()), true);
        return false;
    };
    mouseListener->onMouseUp = [this](ax::EventMouse* e) {
        _hw.setMouseButton(static_cast<int>(e->getMouseButton()), false);
        return false;
    };
    mouseListener->onMouseScroll = [this](ax::EventMouse* e) {
        const float y = e->getScrollY();
        if (y > 0.0f) {
            _hw.addWheel(HardwareInput::kWheelUp);
        } else if (y < 0.0f) {
            _hw.addWheel(HardwareInput::kWheelDown);
        }
        return false;
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(mouseListener, this);

    this->scheduleUpdate();

    // M8: client error reporting (client/src/errorLogs.ts). Reports go to the
    // same API origin find_game uses; the poster is the axmol HTTP client.
    _errorLog = std::make_unique<ErrorLog>(
        [](const std::string& url, const std::string& body) { postJson(url, body); },
        resolveApiBaseUrl());
    AXLOGI("[net] api base {}", resolveApiBaseUrl());
    // Dev smoke test for the whole reporting path: with `debug.surv_debug=1`
    // (the EQ trace switch) send one tagged report at startup; the API log must
    // show `POST /api/report_error`. Nothing is sent in normal runs.
    if (isDevLogEnabled()) {
        _errorLog->logError("startup_probe", "{\"client\":\"android\"}");
    }

    // M8: start the pre-game region latency probes right away (pingTest.ts runs
    // them from main.ts init too). The web sorts `PING_TEST_URLS`; the native
    // list is generated from the same `Config.regions` (net/Regions.h).
    startRegionPingTest();
    return true;
}

std::string GameScene::resolveApiBaseUrl() {
#if AX_TARGET_PLATFORM == AX_PLATFORM_ANDROID
    // The manifest `surv_apiUrl` is resolved once by SurvApi.initialize() (it
    // also powers the Java HTTP/team clients); prefer it so release builds
    // report errors to the same store origin instead of DevConfig's loopback.
    const std::string javaUrl =
        ax::JniHelper::callStaticStringMethod("dev/axmol/app/SurvApi", "apiBaseUrl");
    if (javaUrl.rfind("http://", 0) == 0 || javaUrl.rfind("https://", 0) == 0) {
        return javaUrl;
    }
#endif
    // Same resolution as AppDelegate's find_game wiring.
    std::string apiUrl(ax::UserDefault::getInstance()->getStringForKey(dev::kKeyApiUrl, ""));
    if (apiUrl.rfind("http://", 0) != 0 && apiUrl.rfind("https://", 0) != 0) {
        apiUrl = dev::kApiBaseUrl;
    }
    return apiUrl;
}

void GameScene::reportError(const std::string& loc, const std::string& dataJson) {
    if (!_errorLog) {
        return;
    }
    // The bridge forwards non-fatal Java errors: a worker-thread crash is a
    // "windowOnError" report (dataJson carries message/stacktrace/thread),
    // everything else becomes a plain errorLog entry named after `loc`.
    if (loc == "windowOnError") {
        _errorLog->logWindowOnError(dataJson);
        return;
    }
    _errorLog->logError(loc, dataJson);
}

void GameScene::startRegionPingTest() {
    std::vector<PingTestEntry> regions = pingTestRegions();
    if (regions.empty()) {
        return;
    }
    if (!_pingTest) {
        // The `/ptc` echo socket uses the same binary axmol WebSocket transport
        // as the game connection (the team channel uses
        // createWebSocketConnectionTo, which sends text frames; `/ptc` mirrors
        // the web's ArrayBuffer probe).
        _pingTest = std::make_unique<PingTest>(createWebSocketConnection);
    }
    AXLOGI("[ping] start region probes: {} entries", regions.size());
    _pingTest->setEntries(std::move(regions));
    _pingTest->start(_pingTest->regionList());
    _pingReported = false;
}

void GameScene::setInterpolationEnabled(bool enabled) {
    _interpolationEnabled = enabled;
    if (_world) {
        _world->setInterpolationEnabled(enabled);
    }
}

void GameScene::setScreenShakeEnabled(bool enabled) {
    _screenShakeEnabled = enabled;
    if (_world) {
        _world->setScreenShakeEnabled(enabled);
    }
}

void GameScene::setLocalRotationEnabled(bool enabled) {
    _localRotationEnabled = enabled;
    if (_world) {
        _world->setLocalRotationEnabled(enabled);
    }
}

void GameScene::setAnonPlayerNames(bool enabled) {
    _anonPlayerNames = enabled;
    if (_world) {
        _world->setAnonPlayerNames(enabled);
    }
    if (_game) {
        _game->setAnonPlayerNames(enabled);
    }
}

void GameScene::setTouchSettings(const std::string& moveStyle, const std::string& aimStyle,
                                bool aimLine) {
    // touch.ts validateTouchStyle: anything other than "locked"/"anywhere"
    // falls back to "anywhere".
    _moveStyle = moveStyle == "locked" ? "locked" : "anywhere";
    _aimStyle = aimStyle == "locked" ? "locked" : "anywhere";
    _touchAimLine = aimLine;
    if (_touch) {
        _touch->setMoveStyle(_moveStyle == "locked" ? TouchStyle::Locked : TouchStyle::Anywhere);
        _touch->setAimStyle(_aimStyle == "locked" ? TouchStyle::Locked : TouchStyle::Anywhere);
        _touch->setTouchAimLine(_touchAimLine);
    }
}

void GameScene::resetWorld() {
    // game.ts builds a fresh world (renderer + map + barns) for every match.
    // The native world is only built on a join, never in init(), because
    // `_inGame` is set by enterWithJoin() before it calls this.
    if (!_inGame) {
        return;
    }
    // Tear the previous match down in dependency order: the world/HUD hold raw
    // pointers into the adapter nodes, and the factory owns every adapter node.
    // Dropping the world first (it never deletes nodes) and detaching the two
    // roots from the axmol scene leaves the factory as the only owner, so its
    // clearNodes() can delete the whole tree safely.
    if (_worldRoot) {
        static_cast<ax::Node*>(_worldRoot->native())->removeFromParent();
    }
    _crosshairSprite = nullptr; // owned (and freed) by the factory
    _crosshairFallback = nullptr;
    _crosshairFrame.clear();
    _emoteWheel.reset();
    _emoteTouchId = -1;
    _pingWheel.reset();
    _pingTouchId = -1;
    _bigMapTouchId = -1;
    _hudPress = HudPress{};
    _pendingInputs.clear();
    _pingTargetValid = false;
    if (_roleMenuSound >= 0) {
        if (_audio) {
            _audio->stopSound(_roleMenuSound);
        }
        _roleMenuSound = -1;
    }
    _hud.reset();
    _world.reset();
    _worldRoot = nullptr;
    _hudRoot = nullptr;
    // Replacing the factory destroys the previous match's adapter nodes.
    // clearNodes() deletes them while its own list is still alive (see
    // AxPixiFactory), which is the only device-safe teardown order.
    _pixiFactory = std::make_unique<pix::AxPixiFactory>();
    _world = std::make_unique<GameWorld>(_pixiFactory.get(), false);
    _world->setAudio(_audio.get(), _ambiance.get());
    _world->setInterpolationEnabled(_interpolationEnabled);
    _world->setScreenShakeEnabled(_screenShakeEnabled);
    _world->setLocalRotationEnabled(_localRotationEnabled);
    _world->setAnonPlayerNames(_anonPlayerNames);
    // emote.ts reads playerBarn.getPlayerStatus(playerId).role for the
    // leader/captain/last-man ping sound; Game owns the status table.
    _world->setPlayerRoleLookup([this](uint16_t playerId) {
        if (!_game) {
            return std::string();
        }
        const auto& statuses = _game->allPlayerStatus();
        auto it = statuses.find(playerId);
        return it == statuses.end() ? std::string() : it->second.role;
    });
    _world->setDeviceScale(::ui2::kDeviceScale);
    // view.ts resizeGame callbacks: the new layers must follow the camera.
    _world->setSizeCallbacks(
        [this](float width, float height) { onWorldResize(width, height); },
        [this] { _worldSizeDirty = true; });
    _worldRoot = _pixiFactory->createContainer();
    _world->attachTo(_worldRoot);
    _gameRoot->addChild(static_cast<ax::Node*>(_worldRoot->native()));
    _worldRoot->setVisible(_worldVisible);
    // view.ts setSize(): give the new layers the real viewport size right
    // away (they were just rebuilt).
    const auto visible = ax::Director::getInstance()->getVisibleSize();
    _world->setScreenSize(visible.width, visible.height);
    _worldSize = visible;
    _worldSizeDirty = false;
    _worldSized = true;
    // In-game HUD: plain pix nodes on _gameRoot (the 1280x720 design
    // space the menu uses); see game/Hud.h.
    _hudRoot = _pixiFactory->createContainer();
    // The HUD is laid out around the design rect's centre (like the menu's
    // uiRoot); ui::layoutUiRoot fits the 1280x720 rect into the view.
    {
        const ax::Rect safe = safeAreaRect();
        layoutHud(safe);
        if (isDevLogEnabled()) {
            const ax::Size visible = ax::Director::getInstance()->getVisibleSize();
            if (std::fabs(safe.origin.x) > 0.5f || std::fabs(safe.origin.y) > 0.5f
                || std::fabs(safe.size.width - visible.width) > 0.5f
                || std::fabs(safe.size.height - visible.height) > 0.5f) {
                AXLOGI("[perf] HUD fitted to safe area x={} y={} {}x{}", safe.origin.x,
                       safe.origin.y, safe.size.width, safe.size.height);
            }
        }
    }
    _gameRoot->addChild(static_cast<ax::Node*>(_hudRoot->native()));
    _hud = std::make_unique<Hud>();
    _hud->build(_pixiFactory.get(), _hudRoot, Vec2(svui::kit::designWidth(), svui::kit::designHeight()));
    // Emote wheel + button (Priority 2: emote wheel UI/input). The loadout comes
    // from the JoinMsg/JoinedMsg emotes; it is refreshed once the server accepts
    // the join in update().
    _emoteWheel = std::make_unique<EmoteWheel>();
    _emoteWheel->build(_pixiFactory.get(), _hudRoot,
                       Vec2(svui::kit::designWidth(), svui::kit::designHeight()));
    _emoteWheel->setLoadout(_game->emotes());
    _emoteWheel->setTeamMode(_game->teamMode() == 1);
    // Team-ping wheel (Priority 2: ping wheel input + ammo-specific pings).
    _pingWheel = std::make_unique<PingWheel>();
    _pingWheel->build(_pixiFactory.get(), _hudRoot,
                      Vec2(svui::kit::designWidth(), svui::kit::designHeight()));
    _pingWheel->setTeamMode(_game->teamMode() == 1);
}

// view.ts resizeGame(): every listener re-lays-out for the new size. The
// world (and its listeners) is rebuilt for every match, so work is routed
// through `_worldSizeDirty` and applied on the next update tick.
void GameScene::onWorldResize(float width, float height) {
    _worldSize = ax::Size(width, height);
    _worldSizeDirty = true;
    // ui2.ts sizes the HUD in CSS px / device scale; the native HUD is laid out
    // in the 1280x720 design space, which is already the uiRoot scale.
    const auto visible = ax::Director::getInstance()->getVisibleSize();
    const float scale = std::fmin(visible.width / ::ui::kit::designWidth(),
                                  visible.height / ::ui::kit::designHeight());
    if (_world && scale > 0.0f) {
        _world->setDeviceScale(scale);
    }
    // Full-screen layers follow the viewport (view.ts setSize()).
    if (_backdrop) {
        _backdrop->setContentSize(ax::Size(visible.width, visible.height));
    }
    // Re-fit the HUD to the real viewport once the GL view exists (the first
    // pass runs before it, so the HUD would keep the default size scale).
    if (_hudRoot) {
        const ax::Rect safe = safeAreaRect();
        layoutHud(safe);
        if (isDevLogEnabled() && (std::fabs(safe.origin.x) > 0.5f
                                  || std::fabs(safe.origin.y) > 0.5f
                                  || std::fabs(safe.size.width - visible.width) > 0.5f
                                  || std::fabs(safe.size.height - visible.height) > 0.5f)) {
            AXLOGI("[perf] safe area x={} y={} {}x{} (view {}x{})", safe.origin.x,
                   safe.origin.y, safe.size.width, safe.size.height, visible.width,
                   visible.height);
        }
    }
}

ax::Rect GameScene::safeAreaRect() const {
    const ax::Size visible = ax::Director::getInstance()->getVisibleSize();
    const ax::Rect fallback(0.0f, 0.0f, visible.width, visible.height);
    auto* renderView = ax::Director::getInstance()->getRenderView();
    if (!renderView) {
        return fallback;
    }
    // axmol folds display cutouts, rounded corners and system bars into this
    // rect (points, view space). It returns the visible rect when nothing is
    // known; guard degenerate/oversized values anyway.
    const ax::Rect safe = renderView->getSafeAreaRect();
    if (safe.size.width <= 0.0f || safe.size.height <= 0.0f
        || safe.size.width > visible.width + 0.5f
        || safe.size.height > visible.height + 0.5f) {
        return fallback;
    }
    return safe;
}

void GameScene::layoutHud(const ax::Rect& safe) {
    if (!_hudRoot) {
        return;
    }
    // Priority 5 polish: keep the HUD (minimap, kill feed, health/boost, squad
    // bars, emote/ping buttons, big map) out of the notch/system-bar area. The
    // world itself stays full-bleed; the HUD is fitted to and centred in the
    // safe rect instead of the whole viewport (same 1280x720 design space).
    const float fit = std::fmin(safe.size.width / ::ui::kit::designWidth(),
                                safe.size.height / ::ui::kit::designHeight());
    _hudRoot->setPosition(safe.origin.x + safe.size.width * 0.5f,
                          safe.origin.y + safe.size.height * 0.5f);
    _hudRoot->setScale(fit, fit);
}

void GameScene::ensureBackdropVisible() {
    if (_backdrop && !_backdrop->isVisible()) {
        _backdrop->setVisible(true);
    }
}

GameScene::~GameScene() {
    if (_alive) {
        *_alive = false;
    }
    if (_game) {
        _game->free();
    }
}

void GameScene::enterWithJoin(const std::vector<std::string>& urls, const std::string& joinToken) {
    if (!_game || urls.empty()) {
        return;
    }
    clearError();
    // No menu is visible any more: drop the region latency sockets, like the web
    // client stops updating the ping test while a match runs.
    stopPingTest();
    // Every join rebuilds the match world (and its adapter nodes) from
    // scratch, like game.ts free() + init(); resetWorld() below does the
    // teardown. _game itself survives across matches (free() resets it).
    _inGame = true;
    _notifiedStarted = false;
    _joinElapsed = 0;
    _deadNotified = false;
    _victoryMusicPlayed = false;
    _gameRoot->setVisible(true);
    _worldVisible = true;
    ensureBackdropVisible();
    // Build the match world *before* the socket opens. Otherwise the first
    // MapMsg (pumped from tryJoinUrls() below) reaches a null world and the
    // join crashes in GameWorld::loadMap().
    resetWorld();
    if (_worldRoot) {
        _worldRoot->setVisible(true);
    }
    // Frames can already be queued when re-joining, so adopt the cached MapMsg
    // when there is one (normally it lands in the map callback during
    // tryJoinUrls() below).
    if (_game && _game->hasMap()) {
        _world->replayMap(_game->map());
    }
    _touch->display = false;
    // Keep the menu/UiBridge JoinInfo (mirrors main.ts setConfigFromDOM before
    // JoinMsg). GameScene must not overwrite the name/loadout with its default.
    _awaitingJoin = true;
    _joinRetryUrls.clear();
    tryJoinUrls(urls, joinToken, 0);
}

void GameScene::enterWithFindGame(const std::string& region, int gameModeIdx) {
    _region = region;
    _gameModeIdx = gameModeIdx;
    _pendingFind = true;
    _findInFlight = false;
    ++_findGeneration;
    _findAttempts = 0;
    _lastAttemptClock = 0.0;
    _findDelay = 0.0f;
    _findTime = 0.0f;
    // First request fires on the next update tick (main.ts uses a 0ms timeout
    // on the first attempt so it can be skipped).
}

void GameScene::leaveGame() {
    if (_game) {
        _game->free();
    }
    _inGame = false;
    _pendingFind = false;
    _findInFlight = false;
    ++_findGeneration;
    if (_hud) {
        _hud->setWaitingForPlayers(false);
    }
    // An intentional leave cancels any pending reconnect.
    _reconnect = ReconnectState{};
    // game.ts free(): stop drawing the finished match's world (also hides a
    // world that a concurrent join is still rebuilding).
    _worldVisible = false;
    if (_worldRoot) {
        _worldRoot->setVisible(false);
    }
    // touch.ts hideAll(): the pads stay hidden until the next match starts.
    _touch->hideAll();
    for (auto& touch : _touch->touches) touch.active = false;
    // P1.6: keys/mouse held when the match ends must not carry into the menu or
    // the next match, and the diff cache starts fresh on rejoin.
    _hw.reset();
    _inputSync.reset();
    _shootStartPending = false;
    _awaitingJoin = false;
    _joinRetryUrls.clear();
    _gameRoot->setVisible(false);
    // main.ts un-hides the menu on quit; the HUD is hidden until the match is
    // playing again (FguiHud::update gates on isPlaying()).
    if (_hud) {
        _hud->resetRoleMenu();
        _hud->setVisible(false);
    }
    if (_roleMenuSound >= 0) {
        if (_audio) {
            _audio->stopSound(_roleMenuSound);
        }
        _roleMenuSound = -1;
    }
    if (_crosshairSprite) {
        _crosshairSprite->setVisible(false);
    }
    if (_crosshairFallback) {
        _crosshairFallback->setVisible(false);
    }
    if (_audio) {
        if (_ambiance) {
            _ambiance->onGameComplete(*_audio);
        }
        // game.ts free(): stopAll() keeps ambient/music loops running, so the
        // menu music and the ambience tracks survive the return to the menu.
        _audio->stopAll(true);
    }
}

void GameScene::setError(const std::string& key, const std::string& fallback) {
    if (onError) onError(key, fallback);
    // S6: the pause menu is now Java (GamePauseDialog); errors are routed
    // through the bridge. The C++ overlay no longer holds the pause menu.
}

void GameScene::clearError() {
}

void GameScene::tryJoinUrls(const std::vector<std::string>& urls, const std::string& joinToken,
                            size_t index) {
    if (!_game || index >= urls.size()) {
        setError("index-failed-joining-game", "Failed to join game");
        leaveGame();
        return;
    }
    // main.ts keeps a failure callback per URL so a dead host falls through to
    // the next advertised address; the socket close path marks the attempt.
    _joinRetryUrls = urls;
    _joinRetryToken = joinToken;
    _joinRetryIndex = index;
    _awaitingJoin = true;
    // `debug.surv_debug=1` keeps the join trace (same switch MenuScene reads).
    if (ax::UserDefault::getInstance()->getStringForKey(dev::kKeyDebug, "0") == "1" && index == 0) {
        AXLOGI("[dev] joining {} (advertised urls={})", urls[0], urls.size());
    }
    _game->tryJoinGame(urls[index], joinToken);
}

void GameScene::runFindGameAttempt() {
    if (!_findGameRequest) {
        setError("index-failed-finding-game", "Failed to find game");
        return;
    }
    _findAttempts++;
    _findInFlight = true;
    _findTime = 0.0f;
    std::weak_ptr<std::atomic<bool>> weak = _alive;
    const std::string region = _region;
    const int mode = _gameModeIdx;
    const unsigned generation = _findGeneration;
    _findGameRequest(region, mode, [this, weak, generation](const FindGameResultInfo& result) {
        const auto alive = weak.lock();
        if (!alive || !*alive || generation != _findGeneration) {
            return;
        }
        _findInFlight = false;
        if (result.ok && !result.urls.empty()) {
            _pendingFind = false;
            _findAttempts = 0;
            enterWithJoin(result.urls, result.joinToken);
            return;
        }
        const std::string key = result.error == "banned" ? "index-ip-banned"
            : result.error == "behind_proxy" ? "index-behind-proxy"
            : result.error == "invalid_protocol" ? "index-invalid-protocol"
            : result.error == "invalid_captcha" ? "index-invalid-captcha"
            : result.error == "rate_limited" ? "index-rate-limited" : "index-failed-finding-game";
        setError(key, "Failed to find game");
        // main.ts retries every 500ms until maxAttempts (2).
        if (_findAttempts < 2 && key == "index-failed-finding-game") {
            _findDelay = std::min(_findAttempts * 2.5f, 7.5f);
            _pendingFind = true;
        } else {
            _pendingFind = false;
            if (onMatchEnded) onMatchEnded(key);
        }
    });
}

void GameScene::pauseGame() {
    if (_game) {
        _game->pause();
    }
    // P1.6: drop held keys/mouse buttons and the send-diff cache so a hardware
    // key held across the pause boundary cannot stick or be replayed.
    _hw.reset();
    _inputSync.reset();
    // Audio keeps playing while paused/backgrounded: the socket and mixer stay
    // alive, and createJS.stopAll() would leave the Ambiance tracks unable to
    // restart (web client parity: pause only freezes input).
    // Backgrounded: the OS may freeze the process, so release the latency
    // sockets instead of leaving half-open probes behind (the Java menu asks
    // for a fresh test when it comes back).
    stopPingTest();
}

void GameScene::onPickupSound(const std::string& item) {
    if (!_audio) {
        return;
    }
    // player.ts playItemPickupSound: itemDef.sound.pickup on the ui channel.
    const DefProvider* provider = getDefProvider();
    const GameObjRenderDef* def = provider ? provider->gameObject(item) : nullptr;
    if (!def) {
        return;
    }
    // game.ts: XP drops show a rare-loot message in addition to the pickup cue.
    if (def->type == "xp" && _hud) {
        _hud->displayPickupMessage("RARE LOOT");
    }
    if (def->soundPickup.empty()) {
        return;
    }
    audio::PlaySoundOptions opts;
    opts.channel = "ui";
    opts.rangeMult = 48.0f;
    _audio->playSound(def->soundPickup, opts);
}

void GameScene::returnToMenu() {
    // Stop any in-flight matchmaking/join first so a late find_game response
    // cannot pull the player back out of the menu.
    if (_inGame || _pendingFind || _findInFlight) {
        leaveGame();
    }
    if (onMatchEnded) {
        onMatchEnded("");
    }
}

bool GameScene::handleBackPressed() {
    AXLOGI("[dev] GameScene::handleBackPressed _inGame={} _pendingFind={} _notifiedStarted={}",
           _inGame, _pendingFind, _findInFlight);
    // In a match, BACK opens (or closes) the Java pause dialog instead of
    // dropping the player out of it; the dialog's Quit calls onQuitGame.
    // Hosts without that UI (desktop) fall back to leaving.
    if (_inGame && uiHost()->togglePause()) {
        return true;
    }
    if (_inGame || _pendingFind || _findInFlight) {
        returnToMenu();
        return true;
    }
    return false;
}

void GameScene::resumeGame() {
    if (_game) {
        _game->resume();
    }
    // pauseGame() released the latency sockets; re-run the probes that never
    // finished (completed tests keep their measurements).
    if (_pingTest && !_pingTest->tests().empty() && !_pingTest->isComplete()) {
        _pingTest->start(_pingTest->regionList());
    }
}

void GameScene::stopPingTest() {
    if (_pingTest) {
        _pingTest->stop();
    }
    // Keep `_pingReported` as-is: a finished test stays finished, and re-reporting
    // it after every stop (match start, background) spammed the Java menu.
}

bool GameScene::tryReconnect() {
    // Only transport-level failures can heal on a new socket; a server rejection
    // (invalid token, ban, protocol) will not become valid by retrying.
    const uint16_t code = _game->getCloseCode();
    const std::string& reason = _game->getCloseReason();
    const bool rejected = reason == "invalid_token" || reason == "ip_banned"
        || reason == "rate_limited" || reason == "behind_proxy";
    const bool retryable = !rejected && (code == 0 || code == 1006 || code == 1011);
    if (!retryable) {
        _reconnect.active = false;
        return false;
    }

    if (!_reconnect.active) {
        // The first close of a match we actually joined: the server keeps a
        // rejoin ticket for the seat (server/src/game/game.ts), so retry with the
        // token this match was joined with.
        if (!_game->hasJoined() || _joinRetryToken.empty() || _joinRetryUrls.empty()) {
            return false;
        }
        _reconnect = ReconnectState{};
        _reconnect.active = true;
        _reconnect.url = _joinRetryUrls.front();
        _reconnect.token = _joinRetryToken;
        AXLOGW("[net] connection lost in a match; trying to rejoin the same game");
    }

    if (_reconnect.attempt >= kReconnectMaxAttempts) {
        _reconnect.active = false;
        return false;
    }
    if (_reconnect.elapsed < _reconnect.delay) {
        return true; // waiting for the next attempt
    }

    ++_reconnect.attempt;
    AXLOGI("[net] rejoin attempt {}/{}", _reconnect.attempt, kReconnectMaxAttempts);
    // Full re-entry: the server answers with Joined + Map + every visible object,
    // exactly like a fresh join, so the world is rebuilt for it.
    _game->free();
    enterWithJoin({_reconnect.url}, _reconnect.token);
    _reconnect.elapsed = 0.0f;
    _reconnect.delay = static_cast<float>(_reconnect.attempt); // 1 s, 2 s, 3 s
    return true;
}

void GameScene::update(float delta) {
    // view.ts setSize(): director callbacks run before the frame's update, so
    // pick up the first real window size here and re-layout the world layers
    // (the world is built before the GL view exists).
    if (_world) {
        const auto visible = ax::Director::getInstance()->getVisibleSize();
        if (!_worldSized || _worldSizeDirty || visible.width != _worldSize.width ||
            visible.height != _worldSize.height) {
            _world->setScreenSize(visible.width, visible.height);
            _worldSize = visible;
            _worldSizeDirty = false;
            _worldSized = true;
        }
    }
    _game->update(delta);
    // P0.4: the Java pause dialog's Spectate button is enabled once the local
    // player dies (the C++ HUD owns the in-game death/spectate overlay).
    if (_inGame && !_deadNotified && _game->isLocalDead()) {
        _deadNotified = true;
        if (UiHost* host = uiHost()) {
            host->showResult();
        }
    }
    // P0.1: a held weapon/item card turns into a drop after kLongPressSeconds.
    if (_hudPress.id >= 0) {
        _hudPress.elapsed += delta;
        if (!_game->isPlaying()) {
            _hudPress = HudPress{};
        }
    }
    // M8: region latency test (client/src/pingTest.ts). Echo every region's
    // `/ptc` socket while the Java menu is up, then hand the results back once.
    if (_pingTest) {
        _pingTest->update(delta);
        if (!_pingReported && _pingTest->isComplete()) {
            _pingReported = true;
            const std::string results = _pingTest->resultsJson();
            AXLOGI("[ping] results {}", results);
            if (UiHost* host = uiHost()) {
                host->pingResults(results);
            }
        }
    }
    // M8: errorLogs.ts runs its budget update from a 1 s interval.
    if (_errorLog) {
        _errorLogTick += delta;
        while (_errorLogTick >= 1.0f) {
            _errorLogTick -= 1.0f;
            _errorLog->tick();
        }
    }
    // Priority 5: pace the rejoin attempts while a reconnect is pending.
    if (_reconnect.active) {
        _reconnect.elapsed += delta;
    }
    // Perf/polish: with `debug.surv_debug=1` print the frame rate and the GL frame
    // size every 5 s (and on any frame-size change, e.g. a GFX tier change), plus
    // a 60 s soak summary for device perf runs.
    if (isDevLogEnabled()) {
        auto* director = ax::Director::getInstance();
        auto* renderView = director->getRenderView();
        const ax::Vec2 frame = renderView ? renderView->getFrameSize() : ax::Vec2(0.0f, 0.0f);
        if (frame.x != _perfFrameSize.x || frame.y != _perfFrameSize.y) {
            _perfFrameSize = frame;
            AXLOGI("[perf] frame size {}x{}", static_cast<int>(frame.x), static_cast<int>(frame.y));
        }
        _perfLogTick += delta;
        if (_perfLogTick >= 5.0f) {
            const float window = _perfLogTick;
            _perfLogTick = 0.0f;
            const float secondsPerFrame = director->getSecondsPerFrame();
            const float fps = secondsPerFrame > 0.0f ? 1.0f / secondsPerFrame : 0.0f;
            if (fps > 0.0f) {
                if (_perfSoakSamples == 0 || fps < _perfSoakMin) {
                    _perfSoakMin = fps;
                }
                if (fps > _perfSoakMax) {
                    _perfSoakMax = fps;
                }
                _perfSoakSum += fps;
                ++_perfSoakSamples;
            }
            AXLOGI("[perf] fps={:.1f} frame={}x{}", fps,
                   static_cast<int>(frame.x), static_cast<int>(frame.y));

            // Soak summary over 5 s samples (not per-frame spikes): 12 samples
            // per minute, which is what a "60 fps soak" report should show.
            _perfSoakTick += window;
            if (_perfSoakTick >= 60.0f) {
                if (_perfSoakSamples > 0) {
                    const float avg = _perfSoakSum / static_cast<float>(_perfSoakSamples);
                    AXLOGI("[perf] soak {:.0f}s fps min={:.1f} avg={:.1f} max={:.1f} frame={}x{}",
                           _perfSoakTick, _perfSoakMin, avg, _perfSoakMax,
                           static_cast<int>(frame.x), static_cast<int>(frame.y));
                    if (avg < 45.0f) {
                        AXLOGW("[perf] soak: average below 45 fps; choose a lower GFX tier in the "
                               "pause menu (Medium/Low/Potato)");
                    }
                }
                _perfSoakTick = 0.0f;
                _perfSoakMin = 0.0f;
                _perfSoakMax = 0.0f;
                _perfSoakSum = 0.0f;
                _perfSoakSamples = 0;
            }
        }
    }
    if (_findInFlight) {
        _findTime += delta;
        if (_findTime > 30) {
            leaveGame();
            if (onMatchEnded) onMatchEnded("index-failed-finding-game");
        }
    }
    if (_inGame && !_notifiedStarted) {
        _joinElapsed += delta;
        if (_game->hasJoined()) {
            _notifiedStarted = true;
            // A successful (re)join ends any pending reconnect.
            _reconnect = ReconnectState{};
            // The match is actually running; show the joystick pads (game.ts
            // only enables the touch display once the game starts).
            _touch->display = true;
            if (_hud) {
                _hud->setVisible(true);
            }
            // The Java layer hides its menu when a match starts (JavaUiHost::
            // enterGame); without this the menu fragment keeps painting over the
            // game canvas.
            if (UiHost* host = uiHost()) {
                host->enterGame(static_cast<int>(_game->getActivePlayerId()));
            }
            // Debug-only EQ self-test: with `debug.surv_debug=1` the `[audio]`
            // trace must show an EFX low-pass for muffled/club, so the filter
            // path is verifiable on-device without finding a club/underground.
            if (audio::audioTraceEnabled() && _audio) {
                AXLOGI("[audio] eq self-test: playing emote_01 muffled/club");
                audio::PlaySoundOptions eqOpts;
                eqOpts.channel = "ui";
                eqOpts.filter = "muffled";
                eqOpts.forceFilter = true;
                eqOpts.forceStart = true;
                eqOpts.volumeScale = 1.0f;
                _audio->playSound("emote_01", eqOpts);
                eqOpts.filter = "club";
                _audio->playSound("emote_01", eqOpts);
            }
            if (onMatchStarted) onMatchStarted();
        } else if (_joinElapsed > 30) {
            leaveGame();
            if (onMatchEnded) onMatchEnded("index-failed-joining-game");
        }
    }

    // M7 quick-start: wait out the anti-spam delay, then issue the request.
    if (_pendingFind) {
        if (_findDelay > 0.0f) {
            _findDelay -= delta;
        } else {
            _pendingFind = false;
            runFindGameAttempt();
        }
    }

    if (_world && _inGame) {
        _world->update(delta);
        for (const auto& emote : _world->emoteManager().consumeOutgoing()) {
            _game->sendEmote(emote.type, emote.pos, emote.isPing);
        }
    }

    // game.ts owns audioManager.update() + ambience.update() outside the world
    // update, so both keep running while the Java menu is up (menu music) and
    // the camera position follows the interpolated camera.
    if (_audio) {
        _audio->cameraPos = (_world && _inGame) ? _world->camera().m_pos : Vec2(0.0f, 0.0f);
        _audio->update(delta);
    }
    if (_ambiance && _audio) {
        _ambiance->update(delta, *_audio, _game && _game->isPlaying());
    }

    // game.ts GameOverMsg: on a win, play the map's music after 1.3 s (the
    // client's victory cue); defeat keeps the menu music already running.
    if (_audio && _game && !_victoryMusicPlayed) {
        if (const GameOverMsg* over = _game->gameOver()) {
            _victoryMusicPlayed = true;
            const uint16_t localTeam = _game->teamIdForPlayer(_game->getLocalPlayerId());
            if (over->winningTeamId != 0 && over->winningTeamId == localTeam) {
                const DefProvider* provider = getDefProvider();
                const MapRenderDef* mapDef =
                    provider ? provider->mapRender(_game->map().mapName) : nullptr;
                const std::string music = mapDef ? mapDef->ambienceMusic : std::string();
                if (!music.empty()) {
                    audio::PlaySoundOptions opts;
                    opts.channel = "music";
                    opts.delay = 1.3f;
                    opts.forceStart = true;
                    _audio->playSound(music, opts);
                }
            }
        }
    }

    // game.ts RoleAnnouncement: play the role's assigned/dead sound on the UI
    // channel (leader/captain/medic/...); the defs come from roleDefs.ts.
    if (_game) {
        for (const auto& ann : _game->consumeRoleAnnouncements()) {
            const DefProvider* provider = getDefProvider();
            const RoleRenderDef* roleDef = provider ? provider->role(ann.role) : nullptr;
            if (!roleDef) {
                continue;
            }
            const std::string& sound = ann.assigned ? roleDef->soundAssign : roleDef->soundDead;
            if (!sound.empty() && _audio) {
                audio::PlaySoundOptions opts;
                opts.channel = "ui";
                _audio->playSound(sound, opts);
            }

            // game.ts: role promoted/dead lines go into the kill feed with the
            // role's own colour (or the player's team colour).
            const uint8_t teamId = _game->playerTeamId(ann.playerId);
            const std::string playerName = _game->displayName(ann.playerId);
            const std::string killerName = ann.killerId != 0 && ann.killerId != ann.playerId
                ? _game->displayName(ann.killerId)
                : std::string();
            const RoleFeedLine line =
                roleFeedLine(*roleDef, ann.role, ann.assigned, ann.killed, teamId, playerName, killerName);
            if (line.show) {
                _game->addKillFeedText(line.text, line.color);
            }

            // game.ts: a perk-mode assignment for the local player closes the
            // class picker (the selection already went out).
            if (_hud && ann.assigned && ann.playerId == _game->getLocalPlayerId() &&
                _hud->roleMenuActive()) {
                _hud->setRoleMenuActive(false);
            }
            // kill_leader has its own top-centre HUD line; announcing it here
            // would duplicate/spam every leader change.
            if (_hud && ann.role != "kill_leader" &&
                ann.playerId == _game->getLocalPlayerId()) {
                const std::string roleName = roleDef->name.empty() ? ann.role : roleDef->name;
                _hud->showAnnouncement(ann.assigned ? "ROLE ASSIGNED: " + roleName
                                                    : "ROLE LOST: " + roleName);
            }
        }
    }

    // Low-noise lifecycle logging (once per transition) for the M3 net gate.
    const bool connected = _game->isConnected();
    if (connected != _wasConnected) {
        _wasConnected = connected;
        if (connected) {
            AXLOGI("Connected to game server");
        } else {
            AXLOGI("Disconnected: code={} reason={}", _game->getCloseCode(), _game->getCloseReason());
        }
    }
    if (_game->isPlaying() != _wasPlaying) {
        _wasPlaying = _game->isPlaying();
        if (_wasPlaying) {
            AXLOGI("Receiving game updates: activePlayerId={}", _game->getActivePlayerId());
            if (_ambiance) {
                _ambiance->onGameStart();
            }
        }
    }

    // A closed game socket (not an intentional leave) first tries the server's
    // rejoin ticket; if that fails, returns to the menu with a localized reason
    // (mirroring main.ts onQuit()).
    // Some transports report a close without a code (0), so a joined match that
    // stopped being connected also counts as a drop.
    const bool lostSocket = !_game->isConnected()
        && (_game->getCloseCode() != 0 || _game->hasJoined());
    if (_inGame && lostSocket) {
        // A normal match end sends GameOverMsg and then closes with 3000 (the
        // server stopTicker) or an empty reason. Keep the result screen/world
        // until the player taps Leave; do not reconnect or show a disconnect.
        if (_game->gameOver() != nullptr) {
            _awaitingJoin = false;
            _reconnect = ReconnectState{};
        } else {
            // Mid-match drop: rejoin the same game (the server keeps the seat).
            if (tryReconnect()) {
                return;
            }
            // If the server never accepted us, try the next advertised URL with
            // the same join token (main.ts joinGame()) before giving up.
            if (!_reconnect.active && _awaitingJoin && !_game->hasJoined()
                && _joinRetryIndex + 1 < _joinRetryUrls.size()) {
                _awaitingJoin = false;
                const size_t next = _joinRetryIndex + 1;
                const std::vector<std::string> urls = _joinRetryUrls;
                const std::string token = _joinRetryToken;
                AXLOGW("Join failed (code={}); trying next url", _game->getCloseCode());
                tryJoinUrls(urls, token, next);
                return;
            }
            _awaitingJoin = false;
            // server/src game close reasons (shared/types/api.ts
            // GameWsDisconnectReason). The server always closes with
            // 3000 + a reason string; the old 4001-4011 code table was invented.
            const std::string key = disconnectReasonKey(
                _game->getCloseReason(), _game->hasJoined(), _reconnect.attempt > 0);
            setError(key, "Connection lost");
            leaveGame();
            if (onMatchEnded) onMatchEnded(key);
        }
    } else if (_game->hasJoined()) {
        _awaitingJoin = false;
    }

    // In-game HUD (game/Hud.h): visible while the match is playable, like the
    // web client's stale flag.
    if (_hud) {
        // Gas countdown (ui.ts #ui-gas-timer).
        if (_world) {
            GasHudState gasHud;
            const Gas& gas = _world->gas();
            gasHud.active = gas.isActive();
            gasHud.moving = gas.mode == GasMode_Moving;
            gasHud.secondsLeft = std::max(0.0f, gas.duration * (1.0f - gas.circleT));
            _hud->setGasState(gasHud);
        }
        // Squad bars (ui.ts team health / FguiHud::buildSquadBars).
        {
            std::vector<SquadMember> squad;
            if (_game->teamMode() != 1) {
                std::vector<uint16_t> ids = _game->getTeamPlayerIds();
                std::sort(ids.begin(), ids.end());
                if (ids.size() > 1) {
                    const auto& infos = _game->allPlayerInfo();
                    const auto& statuses = _game->allPlayerStatus();
                    const auto& groupStatus = _game->groupStatus();
                    for (size_t i = 0; i < ids.size() && i < 4; ++i) {
                        SquadMember member;
                        const auto info = infos.find(ids[i]);
                        if (info != infos.end()) {
                            member.name = info->second.name;
                        }
                        if (i < groupStatus.size()) {
                            member.health = groupStatus[i].health;
                            member.disconnected = groupStatus[i].disconnected;
                        }
                        const auto status = statuses.find(ids[i]);
                        if (status != statuses.end()) {
                            member.dead = status->second.dead;
                            member.downed = status->second.downed;
                        }
                        member.active = ids[i] == _game->getActivePlayerId();
                        squad.push_back(member);
                    }
                }
            }
            _hud->setSquadState(squad);
        }
        // Gear/perk icons (ui2.ts): helmet, chest, backpack, then perks.
        {
            std::vector<Hud::GearSlot> slots;
            const ActivePlayerData& apd = _game->activePlayerData();
            const DefProvider* provider = getDefProvider();
            const auto pushSlot = [&](const std::string& type, bool droppable) {
                if (type.empty() || slots.size() >= static_cast<size_t>(Hud::kGearIconCount)) {
                    return;
                }
                const GameObjRenderDef* def = provider ? provider->gameObject(type) : nullptr;
                if (def && def->hasImg && !def->img.sprite.empty()) {
                    Hud::GearSlot slot;
                    slot.frame = def->img.sprite;
                    slot.type = type;
                    slot.level = def->level;
                    slot.droppable = droppable;
                    slots.push_back(std::move(slot));
                }
            };
            // Web excludes the backpack from drop actions.
            pushSlot(apd.helmet, true);
            pushSlot(apd.chest, true);
            pushSlot(apd.backpack, false);
            for (const auto& perk : apd.perks) {
                pushSlot(perk.type, perk.droppable);
            }
            _hud->setGearSlots(slots);
        }
        const bool deadOrSpectating = _game->isLocalDead() || _game->isSpectating();
        if (deadOrSpectating && _touch) {
            _touch->hideAll();
        }
        _hud->setWaitingForPlayers(_inGame && _game->hasJoined() && !_game->hasStarted() &&
                                   !_game->gameOver());
        // ui.ts interaction prompt: nearest loot on the active layer. The
        // server performs the actual pickup; this is only the touch hint.
        if (_world && !deadOrSpectating) {
            const Player* me = _world->activePlayer();
            std::string prompt;
            if (me && me->actionType == Action_None) {
                const Loot* best = nullptr;
                float bestDist = 0.0f;
                for (const Loot* loot : _world->lootBarn().lootPool.m_getPool()) {
                    if (!loot->active || !math::sameAudioLayer(loot->layer, me->layer)) {
                        continue;
                    }
                    const float dist = v2Length(v2Sub(loot->pos, me->pos));
                    if (dist <= loot->rad + 2.5f && (!best || dist < bestDist)) {
                        best = loot;
                        bestDist = dist;
                    }
                }
                if (best && !best->type.empty()) {
                    std::string name = best->type;
                    std::replace(name.begin(), name.end(), '_', ' ');
                    for (char& c : name) {
                        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                    }
                    prompt = "PICK UP " + name;
                }
            }
            _hud->setInteractionPrompt(prompt);
            _hud->setFactionMode(_world->map().mapDef.factionMode);
            // ui.ts faction flair: the health counter shows the active player's
            // red/blue patch in faction modes.
            _hud->setLocalTeamId(_game->playerTeamId(_game->getActivePlayerId()));

            // ui.ts off-screen teammate arrows.
            std::vector<TeammateArrow> arrows;
            const Vec2 screen = Vec2(_world->camera().m_screenWidth,
                                     _world->camera().m_screenHeight);
            const Vec2 center = Vec2(screen.x * 0.5f, screen.y * 0.5f);
            for (uint16_t id : _game->getTeamPlayerIds()) {
                if (id == _game->getActivePlayerId()) {
                    continue;
                }
                const Player* mate = _world->playerBarn().getPlayerById(id);
                if (!mate || !mate->active || mate->dead) {
                    continue;
                }
                const Vec2 p = _world->camera().m_pointToScreen(mate->pos);
                if (p.x >= 0.0f && p.x <= screen.x && p.y >= 0.0f && p.y <= screen.y) {
                    continue;
                }
                Vec2 dir(p.x - center.x, center.y - p.y);
                TeammateArrow arrow;
                arrow.dir = v2NormalizeSafe(dir, Vec2(1.0f, 0.0f));
                arrow.tint = mate->downed ? 0xffaa00 : 0xffffff;
                arrow.downed = mate->downed;
                arrows.push_back(arrow);
            }
            _hud->setTeammateArrows(arrows);
        } else if (_hud) {
            _hud->setInteractionPrompt(std::string());
            _hud->setTeammateArrows({});
        }
        _hud->update(delta, *_game,
                     _inGame && _notifiedStarted && (_game->isPlaying() || _game->gameOver() != nullptr));
        // game.ts:697-705: the picker's confirmed/timed-out class goes out on
        // the next frame and is remembered for the next perk match.
        if (_game->isPlaying()) {
            std::string role;
            if (_hud->consumeRoleSelection(role) && !role.empty()) {
                PerkModeRoleSelectMsg select;
                select.role = role;
                _game->sendMessage(MsgType_PerkModeRoleSelect, select, 128);
                uiHost()->setConfigValue("perkModeRole", role);
                AXLOGI("Perk role selected: {}", role);
            }
        }
        // ui.ts roleMenuInst: the ambient track plays only while the picker is
        // up (the web does not loop it, the 20 s countdown fits the clip).
        if (_roleMenuSound >= 0 && (!_hud->roleMenuActive() || _game->gameOver() != nullptr)) {
            if (_audio) {
                _audio->stopSound(_roleMenuSound);
            }
            _roleMenuSound = -1;
        }
        // ui.ts createPing / mapSprite.ts: minimap ping sprites over the baked
        // map texture (EmoteManager owns their lifetimes).
        if (_world) {
            _hud->updateMapPings(_world->emoteManager().mapPings());

            // ui.ts updatePlayerMapSprites + m_render: player/team dots and the
            // gas/safe-zone circles on both the minimap and the big map.
            std::vector<MapDot> dots;
            const auto& infos = _game->allPlayerInfo();
            const uint16_t activeId = _game->getActivePlayerId();
            const auto activeIt = infos.find(activeId);
            const uint8_t activeGroup = activeIt != infos.end() ? activeIt->second.groupId : 0;

            std::vector<uint16_t> groupMembers;
            for (const auto& entry : infos) {
                if (entry.second.groupId == activeGroup) {
                    groupMembers.push_back(entry.first);
                }
            }
            std::sort(groupMembers.begin(), groupMembers.end());
            const auto groupColor = [&](uint16_t id) -> uint32_t {
                for (size_t i = 0; i < groupMembers.size() && i < 4; ++i) {
                    if (groupMembers[i] == id) {
                        return kGroupColors[i];
                    }
                }
                return 0xffffff;
            };
            const auto teamColor = [&](uint8_t teamId) -> uint32_t {
                return teamId >= 1 && teamId <= 2 ? kTeamColors[teamId - 1] : 0xffffff;
            };
            const auto addDot = [&](const Vec2& pos, float scale, const std::string& texture,
                                    uint32_t tint, int zOrder) {
                MapDot dot;
                dot.pos = pos;
                dot.scale = scale;
                dot.texture = texture;
                dot.tint = tint;
                dot.zOrder = zOrder;
                dots.push_back(dot);
            };

            Player* me = _world->activePlayer();
            const DefProvider* provider = getDefProvider();
            const bool factionMode = _world->map().mapDef.factionMode;
            const auto addStyledDot = [&](const Vec2& pos, bool sameGroup, bool dead, bool downed,
                                          const std::string& roleType, uint32_t groupTint,
                                          uint32_t teamTint, int zBase) {
                std::string aliveIcon;
                std::string deadIcon;
                if (!roleType.empty() && provider) {
                    const RoleRenderDef* roleDef = provider->role(roleType);
                    if (roleDef) {
                        aliveIcon = roleDef->mapIconAlive;
                        deadIcon = roleDef->mapIconDead;
                    }
                }
                const MapDotStyle style = mapDotStyle(sameGroup, dead, downed, aliveIcon, deadIcon);
                uint32_t tint = sameGroup ? groupTint : teamTint;
                if (factionMode && style.custom) {
                    tint = teamTint;
                }
                const int z = zBase + style.zBump;
                if (style.outer) {
                    addDot(pos, 0.3f, "player-map-outer.img", 0xffffff, z - 1);
                }
                addDot(pos, style.scale, style.texture, tint, z);
            };

            if (me) {
                const auto meInfo = infos.find(activeId);
                const uint8_t meTeam = meInfo != infos.end() ? meInfo->second.teamId : 0;
                const bool downed = _game->activePlayerData().health < 0.0f;
                addStyledDot(me->pos, true, false, downed, _game->activePlayerData().role,
                             groupColor(activeId), teamColor(meTeam), 60000);
            }
            for (const auto& entry : _game->allPlayerStatus()) {
                const uint16_t id = entry.first;
                const PlayerStatus& status = entry.second;
                if (id == activeId || !status.hasData || !status.visible) {
                    continue;
                }
                const auto infoIt = infos.find(id);
                if (infoIt == infos.end()) {
                    continue;
                }
                const PlayerInfo& info = infoIt->second;
                const bool sameGroup = info.groupId == activeGroup;
                addStyledDot(status.pos, sameGroup, status.dead, status.downed, status.role,
                             groupColor(id), teamColor(info.teamId), 500 + static_cast<int>(id) * 2);
            }

            MapGas gas;
            const Gas& g = _world->gas();
            gas.active = g.isActive();
            const Gas::Circle current = g.getCircle(1.0f);
            gas.pos = current.pos;
            gas.rad = current.rad;
            if (g.circleNew.rad > 0.0f) {
                gas.hasSafe = true;
                gas.safePos = g.circleNew.pos;
                gas.safeRad = g.circleNew.rad;
            }
            gas.playerPos = me ? me->pos : Vec2(0.0f, 0.0f);

            std::vector<MapAirstrike> strikes;
            for (const AirstrikeZone* zone : _world->planeBarn().airstrikeZones) {
                if (!zone || !zone->active) {
                    continue;
                }
                MapAirstrike strike;
                strike.pos = zone->pos;
                strike.rad = zone->rad;
                // plane.ts AirstrikeZone.render: fade in/out over 0.5s.
                strike.alpha = math::smoothstep(zone->ticker, 0.0f, 0.5f) *
                               (1.0f - math::smoothstep(zone->ticker, zone->duration - 0.5f, zone->duration));
                strikes.push_back(strike);
            }
            // ui.ts m_render: the small minimap is centred on the active
            // player's world position (the big map stays fitted to the box).
            _hud->setMapCenter(me ? me->pos : gas.playerPos);
            _hud->updateMapOverlay(dots, gas, strikes);

            // mapIndicator.ts: role/item markers (UpdateMsg.mapIndicators)
            // drawn on both map views with the def's sprite/tints; an equipped
            // indicator pulses and sorts above the player dots.
            std::vector<MapIndicatorMarker> markers;
            for (const auto& ind : _world->indicatorBarn().indicators) {
                if (!ind.active || ind.type.empty() || !provider) {
                    continue;
                }
                const GameObjRenderDef* def = provider->gameObject(ind.type);
                if (!def || def->mapIndicatorSprite.empty()) {
                    continue;
                }
                MapIndicatorMarker marker;
                marker.pos = ind.pos;
                marker.texture = def->mapIndicatorSprite;
                marker.tint = def->mapIndicatorTint;
                marker.equipped = ind.equipped;
                marker.pulse = def->mapIndicatorPulse;
                marker.pulseTint = def->mapIndicatorPulseTint;
                marker.pulseScale = ind.pulseScale;
                // ui.ts: (uiLayout Sm ? 0.15 : 0.2) * 1.25; Android is the small
                // layout (ui/Device) so this is the 0.1875 branch.
                marker.scale = 0.15f * 1.25f;
                markers.push_back(marker);
            }
            _hud->updateMapIndicators(markers);
        }
    }
    // Emote wheel: same visibility rule as the HUD; refresh the loadout each
    // frame (cheap, it only rebuilds when the JoinedMsg emotes changed).
    if (_emoteWheel && _game) {
        const bool playing = _inGame && _notifiedStarted && _game->isPlaying();
        const bool deadOrSpectating = _game->isLocalDead() || _game->isSpectating();
        _emoteWheel->setLoadout(_game->emotes());
        _emoteWheel->setTeamMode(_game->teamMode() == 1);
        // The class picker's wrapper covers the web HUD (z-index 11), so the
        // emote/ping buttons hide behind it too.
        _emoteWheel->setHudVisible(playing && !deadOrSpectating && !_hud->roleMenuActive());
        _emoteWheel->update(delta);
    }
    // Team-ping wheel: same visibility, sharing the emote throttle (the web
    // EmoteBarn greys both wheels during the hard cooldown). The ammo emote
    // follows the equipped weapon's ammo type each frame.
    if (_pingWheel && _game) {
        const bool playing = _inGame && _notifiedStarted && _game->isPlaying();
        const bool deadOrSpectating = _game->isLocalDead() || _game->isSpectating();
        std::string ammoType;
        const ActivePlayerData& apd = _game->activePlayerData();
        if (apd.curWeapIdx >= 0 && apd.curWeapIdx < static_cast<int>(apd.weapons.size())) {
            const std::string& weaponType = apd.weapons[static_cast<size_t>(apd.curWeapIdx)].type;
            const DefProvider* provider = getDefProvider();
            const GameObjRenderDef* def = (!weaponType.empty() && provider)
                ? provider->gameObject(weaponType)
                : nullptr;
            if (def) {
                ammoType = def->ammoType;
            }
        }
        _pingWheel->setTeamMode(_game->teamMode() == 1);
        _pingWheel->setAmmoType(ammoType);
        _pingWheel->setHudVisible(playing && !deadOrSpectating && !_hud->roleMenuActive());
        _pingWheel->setGreyed(_emoteWheel && !_emoteWheel->canSend());
        _pingWheel->update(delta);
        if (!_pingWheel->visible()) {
            // Wheel dismissed (tap outside, timeout, or sent).
            _pingTargetValid = false;
            if (_pingTouchId >= 0 && !_pingWheel->visible()) {
                _pingTouchId = -1;
            }
        }
    }

    // Priority 2: pie timer — driven by active player action data each frame.
    if (_pieTimer && _game && isStarted()) {
        const auto& apd = _game->activePlayerData();
        if (apd.actionDirty && apd.actionDuration > 0.0f) {
            // pieTimer.ts action labels (Reloading/Using/Reviving).
            std::string label = "ACTION";
            const Player* me = _world ? _world->activePlayer() : nullptr;
            if (me) {
                switch (me->actionType) {
                    case Action_Reload:
                    case Action_ReloadAlt: label = "RELOADING"; break;
                    case Action_UseItem: label = "USING"; break;
                    case Action_Revive: label = "REVIVING"; break;
                    default: break;
                }
            }
            _pieTimer->start(label, apd.actionTime, apd.actionDuration);
        }
        if (_pieTimer->isActive()) {
            // ui.ts resize(): the small layout locks screenScaleFactor 0.5626
            // (Android is always the small layout — device.uiLayout == Sm);
            // the large layout follows the viewport.
            const Camera& camera = _world ? _world->camera() : Camera();
            const svui::Device& device = svui::Device::get();
            float screenScaleFactor = 0.5626f;
            if (device.uiLayout != svui::Device::Sm) {
                screenScaleFactor =
                    surv::math::min(1.0f, surv::math::clamp(camera.m_screenWidth / 1280.0f, 0.75f, 1.0f) *
                            surv::math::clamp(camera.m_screenHeight / 1024.0f, 0.75f, 1.0f));
            }
            _pieTimer->resize(device.uiLayout == svui::Device::Sm, device.tablet,
                              device.isLandscape, screenScaleFactor);
            _pieTimer->update(delta, camera);
            // Stop when action completes.
            if (apd.actionDuration > 0.0f && apd.actionTime >= apd.actionDuration) {
                _pieTimer->stop();
            }
        }
    }

    // M7: crosshair — a screen-space overlay drawn at the screen centre above
    // the game world. The default crosshair's art is an inline SVG in
    // crosshairDefs.ts, not an atlas frame, so it is drawn as two bars here
    // (64 px art: 32 px long, 4 px thick).
    if (_world && _pixiFactory && isStarted() && _game->isPlaying() && _game->hasJoined()) {
        const Game::JoinInfo& join = _game->getJoinInfo();
        // The web stroke slider is 0.0..1.5; Content/gui/crosshair bakes 0.25
        // steps (7 buckets), so the frame index is round(stroke * 4).
        const int strokeBucket =
            std::clamp(static_cast<int>(std::lround(join.crosshairStroke * 4.0f)), 0, 6);
        const std::string frame = "gui/crosshair/" + join.crosshairType + "_s" +
            std::to_string(strokeBucket) + ".png";
        const bool hasFrame = ax::FileUtils::getInstance()->isFileExist(frame);
        if (hasFrame && _crosshairFrame != frame) {
            if (audio::audioTraceEnabled()) {
                AXLOGI("[ui] crosshair type={} color={:06x} size={} stroke={} frame={} found={}",
                       join.crosshairType, join.crosshairColor, join.crosshairSize,
                       join.crosshairStroke, frame, hasFrame ? 1 : 0);
            }
            if (_crosshairSprite) {
                _crosshairSprite->setVisible(false);
            }
            _crosshairSprite = _pixiFactory->createSprite(frame);
            _crosshairSprite->setAnchor(0.5f, 0.5f);
            _crosshairSprite->setAlpha(0.85f);
            _crosshairSprite->setTint(join.crosshairColor);
            _gameRoot->addChild(static_cast<ax::Node*>(_crosshairSprite->native()));
            _crosshairFrame = frame;
        }
        const auto screen = ax::Director::getInstance()->getVisibleSize();
        if (hasFrame && _crosshairSprite) {
            const float designScale = screen.height > 0.0f ? screen.height / 720.0f : 1.0f;
            const float pixelSize = 64.0f * join.crosshairSize * designScale;
            const float source = _crosshairSprite->sourceWidth();
            const float scale = source > 0.0f ? pixelSize / source : 1.0f;
            _crosshairSprite->setPosition(screen.width * 0.5f, screen.height * 0.5f);
            _crosshairSprite->setScale(scale, scale);
            _crosshairSprite->setTint(join.crosshairColor);
            _crosshairSprite->setVisible(true);
            if (_crosshairFallback) {
                _crosshairFallback->setVisible(false);
            }
        } else {
            if (!_crosshairFallback) {
                _crosshairFallback = _pixiFactory->createGraphics();
                _crosshairFallback->beginFill(0xffffff, 0.85f);
                _crosshairFallback->drawRect(-2.0f, -16.0f, 4.0f, 32.0f);
                _crosshairFallback->drawRect(-16.0f, -2.0f, 32.0f, 4.0f);
                _crosshairFallback->endFill();
                _gameRoot->addChild(static_cast<ax::Node*>(_crosshairFallback->native()));
            }
            _crosshairFallback->setPosition(screen.width * 0.5f, screen.height * 0.5f);
            const float designScale = screen.height > 0.0f ? screen.height / 720.0f : 1.0f;
            const float fallbackScale = designScale * join.crosshairSize;
            _crosshairFallback->setScale(fallbackScale, fallbackScale);
            _crosshairFallback->setVisible(true);
            if (_crosshairSprite) {
                _crosshairSprite->setVisible(false);
            }
        }
    } else {
        if (_crosshairSprite) {
            _crosshairSprite->setVisible(false);
        }
        if (_crosshairFallback) {
            _crosshairFallback->setVisible(false);
        }
    }

    // S6: the pause menu is now Java (GamePauseDialog).
    // The C++ _overlay is never created; skip the isOpen check.
    if (_game->isPlaying() && _game->getActivePlayerId() != 0
        && !_game->isSpectating() && !_game->isLocalDead()) {
        updateInput(delta);
    } else if (_world) {
        // No local gameplay input while dead/spectating: a held fire state must
        // not keep the dry-fire latch alive into the next life.
        _world->setLocalInput(false, false, true);
    }
    // P1.6 hardware keys that change local UI (map, spectate cycling). Kept
    // outside updateInput() so they also work while dead/spectating, and the
    // per-frame key edges are cleared for every frame that runs.
    updateHardwareUi();
    // Dual joysticks: Touch::m_update reads the pad centres that getMovement/
    // getAim publish and forwards them to the axmol pad graphics. It is safe to
    // call every frame; `display` gates visibility (false until a match starts).
    if (_touch && _movePad && _aimPad) {
        _touch->m_update(*_movePad, *_aimPad);
    }

    // M5: pass touchingAim + touchAimLine state from Touch to GameWorld
    // for the LineSprites aim-line rendering in the world update.
    if (_world) {
        _world->setAimLineState(_touch->touchingAim, _touch->touchAimLine);
    }

    // P1.6: move this frame's key/mouse state into the edge snapshot so
    // keyPressed()/wheel() fire once (input.ts flush()).
    _hw.flush();
}

void GameScene::updateInput(float dt) {
    // The pause dialog / background pause keeps the socket, but the player must
    // not keep moving or firing with stale pad state.
    if (_game->isPaused()) {
        _hw.reset();
        _inputSync.reset();
        if (_world) {
            _world->setLocalInput(false, false, true);
        }
        return;
    }
    const ax::Size size = getContentSize();

    // Feed the dual-joystick state into an InputMsg. isHoldingThrowable drives
    // the throwable priming latch in Touch::getAim; the aim line direction is
    // taken from Touch's aim movement (see TouchInput::build).
    InputMsg msg = _touchInput.build(*_touch,
                                     size.width,
                                     _game->isHoldingThrowable(),
                                     size.width < size.height,
                                     dt);

    // P1.6 keyboard movement. The server ignores the move flags while
    // `touchMoveActive` is set (player.ts:1925), so keyboard movement clears it;
    // otherwise the joystick vectors keep driving (game.ts non-touch branch).
    if (_hw.anyMoveDown()) {
        msg.moveLeft = _hw.moveLeft();
        msg.moveRight = _hw.moveRight();
        msg.moveUp = _hw.moveUp();
        msg.moveDown = _hw.moveDown();
        msg.touchMoveActive = false;
    }

    // Mouse aim/fire: the camera follows the active player, so the cursor offset
    // from the screen centre is the world aim vector (game.ts:471-483).
    if (_hw.mouseUsed()) {
        const Camera* cam = _world ? &_world->camera() : nullptr;
        if (cam) {
            const float dx = _hw.mousePos().x - cam->m_screenWidth * 0.5f;
            const float dy = cam->m_screenHeight * 0.5f - _hw.mousePos().y;
            const float z = cam->m_z();
            msg.toMouseDir = v2NormalizeSafe(Vec2(dx, dy), Vec2(1.0f, 0.0f));
            msg.toMouseLen = z > 0.0f ? v2Length(Vec2(dx, dy)) / z : 0.0f;
            msg.toMouseLen = math::clamp(msg.toMouseLen, 0.0f, Constants::MouseMaxDist);
        }
    }
    // Fire is a normal bind (mouse left by default, remappable to any key).
    msg.shootStart = msg.shootStart || _hw.firePressed();
    msg.shootHold = msg.shootHold || _hw.fireDown();

    // Gamepad (native-only, see HardwareInput.h): the left stick drives the same
    // analog move fields as the touch pads, the right stick aims like the mouse
    // (direction latched, pull -> toMouseLen) and RT/R2 is the trigger.
    if (_hw.gamepadMoving()) {
        msg.touchMoveDir = v2NormalizeSafe(_hw.gamepadMove(), Vec2(1.0f, 0.0f));
        msg.touchMoveLen = static_cast<uint8_t>(
            std::lround(math::clamp(v2Length(_hw.gamepadMove()), 0.0f, 1.0f) * 255.0f));
        msg.touchMoveActive = true;
    }
    if (_hw.gamepadAiming() || _hw.gamepadFire()) {
        msg.toMouseDir = _hw.gamepadAim();
        msg.toMouseLen = math::clamp(_hw.gamepadAimPull(), 0.0f, 1.0f) * Constants::MouseMaxDist;
        msg.shootStart = msg.shootStart || _hw.gamepadFirePressed();
        msg.shootHold = msg.shootHold || _hw.gamepadFire();
    }

    // Keyboard/mouse/gamepad one-shot actions join the HUD's queued ones.
    _hw.collectActions(_pendingInputs);

    // A quick tap can begin and end between two sends; latch shootStart so the
    // server still observes the press.
    _shootStartPending = _shootStartPending || msg.shootStart;
    msg.shootStart = _shootStartPending;

    // Merge the queued one-shot actions (reload/interact/equip/use-item).
    _pendingInputs.applyTo(msg);

    // player.ts dry fire: publish the post-bind fire intent (shootStart is the
    // latched tap so a press that lands between world updates is not lost) and
    // the emote/ping wheel input blocker. The world consumes this on its next
    // update, exactly like inputBinds being polled by player.ts.
    if (_world) {
        const bool preventInput = (_emoteWheel && _emoteWheel->visible())
            || (_pingWheel && _pingWheel->visible());
        _world->setLocalInput(msg.shootHold, msg.shootStart, preventInput);
    }

    // Send when the data changed or after the 1 s keepalive (game.ts:731-771),
    // not unconditionally at the net-sync rate, so idle upstream traffic matches
    // the web client.
    if (!_inputSync.shouldSend(msg, dt)) {
        return;
    }
    _shootStartPending = false;
    if (!_pendingInputs.empty() && isDevLogEnabled()) {
        std::string actions;
        for (uint8_t input : _pendingInputs.inputs()) {
            actions += std::to_string(static_cast<int>(input)) + ",";
        }
        AXLOGI("[input] actions={} useItem={}", actions, _pendingInputs.useItem());
    }
    _game->sendInput(msg);
    _inputSync.markSent(msg);
    _pendingInputs.clear();
}

void GameScene::onHardwareKey(int androidKeyCode, bool down) {
#if AX_TARGET_PLATFORM == AX_PLATFORM_ANDROID
    // Keyboard letters/digits/arrows/F-keys/space/tab/enter/backspace: the same
    // input.ts Key codes the bind table stores (input/InputBinds.h).
    const int key = input::webKeyFromAndroidKeyCode(androidKeyCode);
    if (key >= 0) {
        _hw.setKey(key, down);
        return;
    }
    // Gamepad buttons map onto fixed bind actions (native-only mapping, see
    // HardwareInput.h). Triggers arrive as axes through onGamepadMotion, but
    // some pads report R2/L2 as buttons; R2 sets the fire trigger and L2
    // quick-uses a bandage.
    switch (androidKeyCode) {
        case AKEYCODE_BUTTON_A: _hw.setPadAction(Input_Interact, down); return;
        case AKEYCODE_BUTTON_B: _hw.setPadAction(Input_Cancel, down); return;
        case AKEYCODE_BUTTON_X: _hw.setPadAction(Input_Reload, down); return;
        case AKEYCODE_BUTTON_Y: _hw.setPadAction(Input_SwapWeapSlots, down); return;
        case AKEYCODE_BUTTON_L2: _hw.setPadAction(Input_UseBandage, down); return;
        case AKEYCODE_BUTTON_THUMBL: _hw.setPadAction(Input_StowWeapons, down); return;
        case AKEYCODE_BUTTON_THUMBR: _hw.setPadAction(Input_ToggleMap, down); return;
        case AKEYCODE_BUTTON_L1:
            if (down) _hw.addWheel(HardwareInput::kWheelUp); // previous weapon
            return;
        case AKEYCODE_BUTTON_R1:
            if (down) _hw.addWheel(HardwareInput::kWheelDown); // next weapon
            return;
        case AKEYCODE_BUTTON_R2:
            _hw.setGamepadTrigger(down ? 1.0f : 0.0f);
            return;
        default: return;
    }
#else
    (void)androidKeyCode;
    (void)down;
#endif
}

void GameScene::onGamepadMotion(float moveX, float moveY, float aimX, float aimY, float fire) {
    _hw.setGamepadAxes(moveX, moveY, aimX, aimY);
    _hw.setGamepadTrigger(fire);
}

void GameScene::updateHardwareUi() {
    if (!_game) {
        return;
    }
    // Spectating: the raw arrow keys cycle the observed teammate
    // (game.ts:709-715 uses Key.Left/Key.Right, not the movement binds).
    if (_game->isSpectating()) {
        if (_hw.keyPressed(input::key::Right)) {
            _game->sendSpectate(2);
        } else if (_hw.keyPressed(input::key::Left)) {
            _game->sendSpectate(3);
        }
        return;
    }
    if (!_game->isPlaying() || _game->isPaused() || !_hud || _game->isLocalDead() ||
        _hud->overlayShown()) {
        return;
    }
    // Large map: the ToggleMap bind (M) or the unbound Key.G fallback
    // (game.ts:452-458). The touch expand button and big-map tap targets already
    // share this state.
    if (_hw.bindPressed(Input_ToggleMap) ||
        (_hw.keyPressed(input::key::G) && !_hw.binds().isKeyBound(input::key::G))) {
        _hud->toggleBigMap();
    }
    // CycleUIMode (V): hide/show the minimap without opening the big map.
    if (_hw.bindPressed(Input_CycleUIMode)) {
        _hud->toggleMinimapHidden();
    }
}

void GameScene::onTouchBegan(float x, float y, int id) {
    // S6: _overlay is never created; the Java GamePauseDialog is the
    // actual pause. Don't block touches because of a null overlay.
    if (!_inGame || !_notifiedStarted) return;
    // Hybrid device: a touch takes the aim back from the mouse (the pads own
    // the aim direction again until the next mouse move).
    _hw.noteTouchInput();
    // P0.4 result/death/spectate overlays are modal; route their buttons before
    // any world/map interaction.
    if (_hud && _game && (_hud->overlayShown() || _hud->spectating())) {
        const Vec2 design = screenToDesign(x, y);
        switch (_hud->overlayActionAt(design.x, design.y)) {
            case Hud::OverlayAction::SpectateBegin:
                _game->sendSpectate(1);
                return;
            case Hud::OverlayAction::SpectateNext:
                _game->sendSpectate(2);
                return;
            case Hud::OverlayAction::SpectatePrev:
                _game->sendSpectate(3);
                return;
            case Hud::OverlayAction::ToggleStats:
                _hud->toggleSpectateStats();
                return;
            case Hud::OverlayAction::Leave:
                returnToMenu();
                return;
            case Hud::OverlayAction::None:
                break;
        }
        // The death/result panel dims the whole screen: swallow every touch so
        // it cannot move/fire behind it. The spectating overlay is not modal
        // (the minimap/big map stay usable).
        if (_hud->overlayShown()) {
            return;
        }
    }
    // Perk-mode class picker: #ui-role-menu-wrapper covers the viewport, so
    // every touch is modal; options select, the footer confirms.
    if (_hud && _hud->roleMenuActive()) {
        const Vec2 design = screenToDesign(x, y);
        int option = -1;
        switch (_hud->roleMenuActionAt(design.x, design.y, option)) {
            case Hud::RoleTap::Option:
                if (option >= 0) {
                    _hud->selectRoleOption(option);
                }
                break;
            case Hud::RoleTap::Confirm:
                _hud->confirmRoleSelection();
                break;
            case Hud::RoleTap::None:
                break;
        }
        return;
    }
    // Big map: swallow the touch; tapping outside (or the close button) closes.
    if (_hud && _hud->bigMapVisible()) {
        const Vec2 design = screenToDesign(x, y);
        if (_hud->bigMapCloseHit(design.x, design.y) || !_hud->bigMapContains(design.x, design.y)) {
            _hud->setBigMapVisible(false);
        } else {
            // ui.ts bigmapCollision: remember the tapped world position, then
            // open the team-ping wheel (it is sent at that position).
            const float halfW = ::ui::kit::designWidth() * 0.5f;
            const float halfH = ::ui::kit::designHeight() * 0.5f;
            const Vec2 centered(design.x - halfW, design.y - halfH);
            const Vec2 local(centered.x - _hud->bigMap().pos().x, centered.y - _hud->bigMap().pos().y);
            Vec2 world = _hud->bigMap().unproject(local);
            if (_world) {
                world.x = std::clamp(world.x, 0.0f, _world->map().width);
                world.y = std::clamp(world.y, 0.0f, _world->map().height);
            }
            _pingTarget = world;
            _pingTargetValid = true;
            _hud->setBigMapVisible(false);
            if (_pingWheel) {
                _pingWheel->open(Vec2(halfW, halfH));
                _pingTouchId = -1; // the next touch drives the open wheel
            }
        }
        _bigMapTouchId = id;
        return;
    }
    // A programmatically opened ping wheel (big-map tap) claims the next touch
    // so it does not move or fire.
    if (_pingWheel && _pingWheel->visible() && _pingTouchId < 0) {
        _pingTouchId = id;
        return;
    }
    // Minimap expand button (#ui-map-expand-mobile) opens the big map.
    if (_hud && _game->isPlaying()) {
        const Vec2 design = screenToDesign(x, y);
        if (_hud->minimapExpandHit(design.x, design.y)) {
            _hud->setBigMapVisible(true);
            _bigMapTouchId = id;
            return;
        }
    }
    // Emote wheel trigger (#ui-emote-button): the touch drives the wheel and is
    // kept away from the pads so opening it never moves or fires.
    if (_emoteWheel && _game->isPlaying()) {
        const Vec2 design = screenToDesign(x, y);
        if (_emoteWheel->buttonHit(design.x, design.y)) {
            _emoteTouchId = id;
            // Open the wheel around the button so dragging outward picks a
            // wedge (the button sits one wheel radius in from the edge, so the
            // whole wheel stays on screen).
            _emoteWheel->open(_emoteWheel->buttonCenter());
            return;
        }
    }
    // Team-ping wheel trigger (the button sits left of the emote button).
    if (_pingWheel && _game->isPlaying()) {
        const Vec2 design = screenToDesign(x, y);
        if (_pingWheel->buttonHit(design.x, design.y)) {
            _pingTouchId = id;
            _pingWheel->open(_pingWheel->buttonCenter());
            return;
        }
    }
    // Dead or spectating: the gameplay pads/action bar are hidden and the
    // overlay buttons were handled above. The minimap/big map stay usable.
    if (_game->isSpectating() || _game->isLocalDead()) {
        return;
    }
    // P0.1 gameplay action buttons (reload/interact/heal/boost/scope/swap and
    // the weapon cards). Hit-tested before the pads, like the web's DOM buttons
    // above the canvas. A weapon-card touch that turns into a drag is handed
    // back to Touch (see onTouchMoved), so the pads still work under the cards.
    if (_hud && _game->isPlaying()) {
        const Vec2 design = screenToDesign(x, y);
        const Hud::TapResult tap = _hud->hitTest(design.x, design.y);
        switch (tap.kind) {
            case Hud::TapKind::None:
                break;
            // Immediate actions (ui.ts touch listeners fire on touchstart).
            case Hud::TapKind::Reload:
                _pendingInputs.press(Input_Reload);
                return;
            case Hud::TapKind::Interact:
                // game.ts: interactionTouched adds both Interact and Cancel.
                _pendingInputs.press(Input_Interact);
                _pendingInputs.press(Input_Cancel);
                return;
            case Hud::TapKind::SwapWeapons:
                _pendingInputs.press(Input_SwapWeapSlots);
                return;
            // Item/weapon controls: tap on release, long-press drops (the web's
            // touch cards only bind tap-to-use; drop is the mouse right-click
            // action, so the native touch HUD uses a hold).
            case Hud::TapKind::Heal:
            case Hud::TapKind::Boost:
            case Hud::TapKind::ScopeNext:
            case Hud::TapKind::WeaponSlot:
                _hudPress = HudPress{id, static_cast<int>(tap.kind), tap.slot, 0.0f,
                                     Vec2(x, y), design};
                return;
        }
    }
    _touch->touches[id].active = true;
    _touch->touches[id].posDown = Vec2(x, y);
    _touch->touches[id].pos = Vec2(x, y);
}

void GameScene::sendHudDrop(const std::string& item, uint8_t weapIdx, bool playSound) {
    if (!_game || item.empty() || item == "fists") {
        return;
    }
    DropItemMsg drop;
    drop.item = item;
    drop.weapIdx = weapIdx;
    _game->sendMessage(MsgType_DropItem, drop, 128);
    if (playSound && _audio) {
        audio::PlaySoundOptions opts;
        opts.channel = "ui";
        _audio->playSound("loot_drop_01", opts);
    }
}

void GameScene::finishHudPress(bool longPress) {
    const Hud::TapKind kind = static_cast<Hud::TapKind>(_hudPress.kind);
    const int slot = _hudPress.slot;
    const Vec2 release = _hudPress.pos;
    _hudPress = HudPress{};
    if (!_hud || !_game || !_game->isPlaying()) {
        return;
    }
    switch (kind) {
        case Hud::TapKind::Heal:
            if (longPress) {
                sendHudDrop(_hud->healItem(), 0, true);
            } else {
                _pendingInputs.use(_hud->healItem());
            }
            break;
        case Hud::TapKind::Boost:
            if (longPress) {
                sendHudDrop(_hud->boostItem(), 0, true);
            } else {
                _pendingInputs.use(_hud->boostItem());
            }
            break;
        case Hud::TapKind::ScopeNext:
            if (longPress) {
                sendHudDrop(_hud->scopeType(), 0, true);
            } else if (!_hud->scopeType().empty()) {
                _pendingInputs.press(Input_EquipNextScope);
            }
            break;
        case Hud::TapKind::WeaponSlot: {
            if (!longPress) {
                const uint8_t input = inputForWeaponSlot(slot);
                if (input != Input_Count) {
                    _pendingInputs.press(input);
                }
                break;
            }
            // ui.ts weapon drag: dropping one gun card onto the other swaps the
            // primary/secondary slots; anywhere else drops the gun.
            int target = -1;
            const Hud::TapResult under = _hud->hitTest(release.x, release.y);
            if (under.kind == Hud::TapKind::WeaponSlot) {
                target = under.slot;
            }
            if ((slot == WeaponSlot_Primary && target == WeaponSlot_Secondary) ||
                (slot == WeaponSlot_Secondary && target == WeaponSlot_Primary)) {
                _pendingInputs.press(Input_SwapWeapSlots);
            } else if (!_hud->weaponType(slot).empty()) {
                sendHudDrop(_hud->weaponType(slot), static_cast<uint8_t>(slot), true);
            }
            break;
        }
        case Hud::TapKind::GearSlot:
            // ui2.ts gear/perk touch action: tapping a droppable helmet/chest/
            // perk drops it (backpack is not droppable).
            if (_hud->gearDroppable(slot)) {
                sendHudDrop(_hud->gearType(slot), 0, true);
            }
            break;
        default:
            break;
    }
}

void GameScene::onTouchMoved(float x, float y, int id) {
    if (id == _hudPress.id) {
        _hudPress.pos = screenToDesign(x, y);
        // Moving off a weapon card means the player is aiming (the cards sit
        // over the bottom-right joystick): hand the touch to Touch so a drag
        // fires/aims instead of equipping. A stationary tap still equips.
        if (static_cast<Hud::TapKind>(_hudPress.kind) == Hud::TapKind::WeaponSlot) {
            const float dx = x - _hudPress.downScreen.x;
            const float dy = y - _hudPress.downScreen.y;
            if (dx * dx + dy * dy > kHudDragHandoffPx * kHudDragHandoffPx) {
                _touch->touches[id].active = true;
                _touch->touches[id].posDown = _hudPress.downScreen;
                _touch->touches[id].pos = Vec2(x, y);
                _hudPress = HudPress{};
            }
        }
        return;
    }
    if (id == _bigMapTouchId) {
        return;
    }
    if (id == _emoteTouchId && _emoteWheel) {
        std::string type;
        const Vec2 design = screenToDesign(x, y);
        if (_emoteWheel->select(design, type)) {
            // Queue for Game::sendEmote (game.ts newEmotes) and apply the
            // emote.ts throttle.
            if (_world && _world->activePlayer()) {
                _world->emoteManager().queueEmote(type, _world->activePlayer()->pos);
            }
            _emoteWheel->incrementEmote();
            _emoteWheel->close();
            _emoteTouchId = -1;
        }
        return;
    }
    if (id == _pingTouchId && _pingWheel) {
        PingWheel::Selection selection;
        const Vec2 design = screenToDesign(x, y);
        if (_pingWheel->select(design, selection)) {
            if (_world && _world->activePlayer() && _emoteWheel && _emoteWheel->canSend()) {
                // Pings target the big-map tap when there was one, otherwise
                // the crosshair (the active player; web clamps to map bounds).
                Vec2 pos = (_pingTargetValid && !selection.ping.empty())
                    ? _pingTarget
                    : _world->activePlayer()->pos;
                pos.x = std::clamp(pos.x, 0.0f, _world->map().width);
                pos.y = std::clamp(pos.y, 0.0f, _world->map().height);
                if (!selection.ping.empty()) {
                    _world->emoteManager().queuePing(selection.ping, pos);
                } else if (!selection.emote.empty()) {
                    _world->emoteManager().queueEmote(selection.emote, _world->activePlayer()->pos);
                }
                // Pings and emotes share the emote.ts counter.
                _emoteWheel->incrementEmote();
            }
            _pingTargetValid = false;
            _pingWheel->close();
            _pingTouchId = -1;
        }
        return;
    }
    if (_touch->touches[id].active) {
        _touch->touches[id].pos = Vec2(x, y);
    }
}

void GameScene::onTouchEnded(float x, float y, int id) {
    if (id == _hudPress.id) {
        _hudPress.pos = screenToDesign(x, y);
        finishHudPress(_hudPress.elapsed >= Hud::kLongPressSeconds);
        return;
    }
    if (id == _bigMapTouchId) {
        _bigMapTouchId = -1;
        return;
    }
    if (id == _pingTouchId) {
        if (_pingWheel) {
            _pingWheel->close();
        }
        _pingTargetValid = false;
        _pingTouchId = -1;
        return;
    }
    if (id == _emoteTouchId) {
        if (_emoteWheel) {
            _emoteWheel->close();
        }
        _emoteTouchId = -1;
        return;
    }
    _touch->touches[id].active = false;
}

Vec2 GameScene::screenToDesign(float x, float y) const {
    const auto visible = ax::Director::getInstance()->getVisibleSize();
    const float designW = ::ui::kit::designWidth();
    const float designH = ::ui::kit::designHeight();
    const float fit = std::fmin(visible.width / designW, visible.height / designH);
    if (fit <= 0.0f) {
        return Vec2(designW * 0.5f, designH * 0.5f);
    }
    // Web screen space is y-down; the HUD design space is y-up.
    return Vec2((x - visible.width * 0.5f) / fit + designW * 0.5f,
                (visible.height * 0.5f - y) / fit + designH * 0.5f);
}

} // namespace surv
