#include "UiOverlay.h"

#include "../game/Game.h"
#include "../ui/Touch.h"

#include <cmath>
#include <string>

USING_NS_AX;

namespace ui {

namespace {

constexpr float kBarWidth = 200.0f;
constexpr float kBarHeight = 14.0f;

void drawBar(ax::DrawNode* node, float value, const ax::Color4F& color, const ax::Color4F& back) {
    node->clear();
    node->drawSolidRect(ax::Vec2(0.0f, 0.0f), ax::Vec2(kBarWidth, kBarHeight), back);
    const float w = kBarWidth * (value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value));
    if (w > 0.0f) {
        node->drawSolidRect(ax::Vec2(0.0f, 0.0f), ax::Vec2(w, kBarHeight), color);
    }
}

} // namespace

UiOverlay::~UiOverlay() {
    if (_hudRoot) {
        _hudRoot->removeFromParent();
        _hudRoot = nullptr;
    }
    if (_pauseRoot) {
        _pauseRoot->removeFromParent();
        _pauseRoot = nullptr;
    }
}

void UiOverlay::build(ax::Scene* scene, Config* config, Localization* localization,
                      surv::Touch* touch) {
    _config = config;
    _loc = localization;
    _touch = touch;
    buildHud(scene);
    buildPauseMenu(scene);
}

void UiOverlay::buildHud(ax::Scene* scene) {
    const float designW = kit::designWidth();
    const float designH = kit::designHeight();
    _hudRoot = ax::Node::create();
    _hudRoot->setContentSize(ax::Size(designW, designH));
    scene->addChild(_hudRoot, 100);
    layoutUiRoot(_hudRoot, ax::Director::getInstance()->getVisibleSize(),
                 ax::Vec2(designW, designH));

    // Health + boost bars (bottom-left, like the web HUD).
    _hudPanel = Panel::create(kBarWidth + 20.0f, 52.0f);
    _hudPanel->setBackgroundColor(ax::Color3B(0, 0, 0), 0.35f);
    _hudPanel->placeBottomLeft(14.0f, 14.0f);
    _hudRoot->addChild(_hudPanel);

    _healthBar = ax::DrawNode::create();
    _healthBar->setPosition(ax::Vec2(10.0f, 32.0f));
    _hudPanel->addChild(_healthBar);
    _boostBar = ax::DrawNode::create();
    _boostBar->setPosition(ax::Vec2(10.0f, 12.0f));
    _hudPanel->addChild(_boostBar);

    _healthText = ax::Label::createWithSystemFont("", "sans-serif", 14);
    _healthText->setAnchorPoint(ax::Vec2(1.0f, 0.5f));
    _healthText->setPosition(ax::Vec2(kBarWidth + 10.0f, 39.0f));
    _healthText->setTextColor(ax::Color4B::WHITE);
    _hudPanel->addChild(_healthText);

    // Weapon + ammo (bottom-right).
    _weaponText = ax::Label::createWithSystemFont("", "sans-serif", 16);
    _weaponText->setAnchorPoint(ax::Vec2(1.0f, 0.0f));
    _weaponText->setPosition(ax::Vec2(designW - 16.0f, 40.0f));
    _weaponText->setTextColor(ax::Color4B::WHITE);
    _hudRoot->addChild(_weaponText);
    _ammoText = ax::Label::createWithSystemFont("", "sans-serif", 22);
    _ammoText->setAnchorPoint(ax::Vec2(1.0f, 0.0f));
    _ammoText->setPosition(ax::Vec2(designW - 16.0f, 12.0f));
    _ammoText->setTextColor(ax::Color4B::WHITE);
    _hudRoot->addChild(_ammoText);

    // Players alive (top-right).
    _aliveText = ax::Label::createWithSystemFont("", "sans-serif", 16);
    _aliveText->setAnchorPoint(ax::Vec2(1.0f, 1.0f));
    _aliveText->setPosition(ax::Vec2(designW - 16.0f, kit::fromCssY(14.0f, designH)));
    _aliveText->setTextColor(ax::Color4B::WHITE);
    _hudRoot->addChild(_aliveText);
}

void UiOverlay::buildPauseMenu(ax::Scene* scene) {
    const float designW = kit::designWidth();
    const float designH = kit::designHeight();
    _pauseRoot = ax::Node::create();
    _pauseRoot->setContentSize(ax::Size(designW, designH));
    _pauseRoot->setVisible(false);
    scene->addChild(_pauseRoot, 200);
    layoutUiRoot(_pauseRoot, ax::Director::getInstance()->getVisibleSize(),
                 ax::Vec2(designW, designH));

    auto* dim = ax::LayerColor::create(ax::Color4B(0, 0, 0, 160), designW, designH);
    dim->setAnchorPoint(ax::Vec2(0.0f, 0.0f));
    dim->setPosition(ax::Vec2::ZERO);
    dim->setIgnoreAnchorPointForPosition(false);
    _pauseRoot->addChild(dim);
    swallowTouches(dim);

    const float panelW = 420.0f;
    const float panelH = 520.0f;
    auto* panel = Panel::create(panelW, panelH);
    panel->setBackgroundColor(ax::Color3B(52, 52, 52), 0.97f);
    panel->placeCenter();
    panel->setInteractive(true);
    _pauseRoot->addChild(panel);

    auto addLabel = [&](const std::string& text, float x, float y, float size) {
        auto* label = ax::Label::createWithSystemFont(text, "sans-serif", size);
        label->setAnchorPoint(ax::Vec2(0.0f, 1.0f));
        label->setPosition(ax::Vec2(x, kit::fromCssY(y, panelH)));
        label->setTextColor(ax::Color4B::WHITE);
        panel->addChild(label);
        return label;
    };

    // Touch move/aim styles + aim line (web: #btn-game-move-style etc.).
    _moveStyleBtn = Button::create("Move: Anywhere", 190.0f, 44.0f);
    _moveStyleBtn->setCssPosition(20.0f, 20.0f);
    panel->addChild(_moveStyleBtn);
    _aimStyleBtn = Button::create("Aim: Anywhere", 190.0f, 44.0f);
    _aimStyleBtn->setCssPosition(panelW - 20.0f - 190.0f, 20.0f);
    panel->addChild(_aimStyleBtn);
    _aimLineBtn = Button::create("Aim Line", 400.0f, 44.0f);
    _aimLineBtn->setCssPosition(20.0f, 72.0f);
    panel->addChild(_aimLineBtn);
    _soundBtn = Button::create("Sound", 400.0f, 44.0f);
    _soundBtn->setCssPosition(20.0f, 124.0f);
    panel->addChild(_soundBtn);

    // Volume sliders (web: .sl-master-volume / .sl-sound-volume / .sl-music-volume).
    addLabel(_loc ? _loc->translate("index-master-volume") : "Master Volume", 20.0f, 182.0f, 15);
    auto* master = Slider::create(panelW - 40.0f, _config ? _config->floatOrDefault("masterVolume") : 1.0f);
    master->setCssPosition(20.0f, 196.0f);
    panel->addChild(master);

    addLabel(_loc ? _loc->translate("index-sfx-volume") : "SFX Volume", 20.0f, 244.0f, 15);
    auto* sound = Slider::create(panelW - 40.0f, _config ? _config->floatOrDefault("soundVolume") : 1.0f);
    sound->setCssPosition(20.0f, 258.0f);
    panel->addChild(sound);

    addLabel(_loc ? _loc->translate("index-music-volume") : "Music Volume", 20.0f, 306.0f, 15);
    auto* music = Slider::create(panelW - 40.0f, _config ? _config->floatOrDefault("musicVolume") : 1.0f);
    music->setCssPosition(20.0f, 320.0f);
    panel->addChild(music);

    // Error/status line (also used for matchmaking feedback).
    _errorText = ax::Label::createWithSystemFont("", "sans-serif", 15);
    _errorText->setAnchorPoint(ax::Vec2(0.5f, 0.5f));
    _errorText->setPosition(ax::Vec2(panelW * 0.5f, kit::fromCssY(388.0f, panelH) - 8.0f));
    _errorText->setTextColor(ax::Color4B(255, 120, 120, 255));
    panel->addChild(_errorText);

    // Return to game / Quit.
    auto* resume = Button::create(_loc ? _loc->translate("game-return-to-game") : "Return to Game",
                                  260.0f, 52.0f);
    resume->setCssPosition((panelW - 260.0f) * 0.5f, 404.0f);
    panel->addChild(resume);
    auto* quit = Button::create(_loc ? _loc->translate("game-quit-game") : "Quit Game",
                                260.0f, 52.0f);
    quit->setCssPosition((panelW - 260.0f) * 0.5f, 464.0f);
    panel->addChild(quit);

    // Wire the handlers.
    resume->onClick = [this] { hide(); };
    quit->onClick = [this] {
        hide();
        if (onQuit) {
            onQuit();
        }
    };
    _moveStyleBtn->onClick = [this] {
        if (!_touch || !_config) {
            return;
        }
        _touch->toggleMoveStyle();
        _config->setString("touchMoveStyle",
                           _touch->moveStyle == surv::TouchStyle::Locked ? "locked" : "anywhere");
        _moveStyleBtn->setLabel(_touch->moveStyle == surv::TouchStyle::Locked ? "Move: Locked"
                                                                            : "Move: Anywhere");
        if (onMoveStyleChanged) {
            onMoveStyleChanged(_touch->moveStyle == surv::TouchStyle::Locked);
        }
    };
    _aimStyleBtn->onClick = [this] {
        if (!_touch || !_config) {
            return;
        }
        _touch->toggleAimStyle();
        _config->setString("touchAimStyle",
                           _touch->aimStyle == surv::TouchStyle::Locked ? "locked" : "anywhere");
        _aimStyleBtn->setLabel(_touch->aimStyle == surv::TouchStyle::Locked ? "Aim: Locked"
                                                                          : "Aim: Anywhere");
        if (onAimStyleChanged) {
            onAimStyleChanged(_touch->aimStyle == surv::TouchStyle::Locked);
        }
    };
    _aimLineBtn->onClick = [this] {
        if (!_touch || !_config) {
            return;
        }
        _touch->toggleAimLine();
        _config->setBool("touchAimLine", _touch->touchAimLine);
        if (onAimLineChanged) {
            onAimLineChanged(_touch->touchAimLine);
        }
    };
    _soundBtn->onClick = [this] {
        if (!_config) {
            return;
        }
        const bool muted = !_config->boolOrDefault("muteAudio");
        _config->setBool("muteAudio", muted);
        if (onMuteChanged) {
            onMuteChanged(muted);
        }
    };
    master->onChanged = [this](float v) {
        if (_config) {
            _config->setFloat("masterVolume", v);
        }
        if (onMasterVolume) {
            onMasterVolume(v);
        }
    };
    sound->onChanged = [this](float v) {
        if (_config) {
            _config->setFloat("soundVolume", v);
        }
        if (onSoundVolume) {
            onSoundVolume(v);
        }
    };
    music->onChanged = [this](float v) {
        if (_config) {
            _config->setFloat("musicVolume", v);
        }
        if (onMusicVolume) {
            onMusicVolume(v);
        }
    };

    // Sync the toggle labels with the current touch state.
    if (_touch) {
        _moveStyleBtn->setLabel(_touch->moveStyle == surv::TouchStyle::Locked ? "Move: Locked"
                                                                            : "Move: Anywhere");
        _aimStyleBtn->setLabel(_touch->aimStyle == surv::TouchStyle::Locked ? "Aim: Locked"
                                                                          : "Aim: Anywhere");
    }
}

void UiOverlay::setMenuError(const std::string& l10nKey, const std::string& fallback) {
    if (!_errorText) {
        return;
    }
    if (l10nKey.empty()) {
        _errorText->setString("");
        return;
    }
    std::string text = _loc ? _loc->translate(l10nKey) : "";
    if (text.empty()) {
        text = fallback;
    }
    _errorText->setString(text);
}

void UiOverlay::show() {
    if (!_pauseRoot) {
        return;
    }
    _visible = true;
    _pauseRoot->setVisible(true);
    if (_touch) {
        _touch->display = false;
    }
    if (_soundBtn && _config) {
        _soundBtn->setLabel(_config->boolOrDefault("muteAudio")
                                ? (_loc ? _loc->translate("game-sound-off") : "Sound: Off")
                                : (_loc ? _loc->translate("game-sound") : "Sound"));
    }
}

void UiOverlay::hide() {
    if (!_pauseRoot) {
        return;
    }
    _visible = false;
    _pauseRoot->setVisible(false);
    if (_touch) {
        _touch->display = true;
    }
}

void UiOverlay::toggle() {
    if (_visible) {
        hide();
    } else {
        show();
    }
}

void UiOverlay::refreshHud(const surv::Game* game, bool started, bool playing) {
    if (!game) {
        return;
    }
    (void)started;
    (void)playing;

    // Health/boost live on the active player data inside Game's last update;
    // the scene passes the snapshot it already tracks via Game::snapshot().
    // (Avoids exposing the whole UpdateMsg here.)
    const surv::GameStateSnapshot snap = game->snapshot();
    _health = snap.health;
    drawBar(_healthBar, _health / 100.0f, ax::Color4F(0.85f, 0.2f, 0.2f, 1.0f),
            ax::Color4F(0.15f, 0.15f, 0.15f, 0.9f));
    _healthText->setString(std::to_string(static_cast<int>(std::lround(_health))));

    // Boost is not part of the snapshot; keep the bar hidden unless Game
    // exposes it (the fill uses the same widget).
    drawBar(_boostBar, 0.0f, ax::Color4F(0.95f, 0.75f, 0.1f, 1.0f),
            ax::Color4F(0.15f, 0.15f, 0.15f, 0.9f));

    // Ammo/weapon from the active player's inventory (tracked by the world's HUD
    // pass once the loadout/HUD port lands; empty until then).
    _ammo = -1;
    _weapon.clear();
    _weaponText->setString(_weapon);
    _ammoText->setString(_ammo >= 0 ? std::to_string(_ammo) : "");

    _aliveCount = static_cast<int>(snap.players.size());
    _aliveText->setString(std::to_string(_aliveCount));
}

void UiOverlay::update(float dt, bool started, bool playing, bool connected,
                       const surv::Game* game) {
    (void)dt;
    (void)connected;
    if (_hudRoot) {
        _hudRoot->setVisible(!_visible);
    }
    refreshHud(game, started, playing);
}

} // namespace ui