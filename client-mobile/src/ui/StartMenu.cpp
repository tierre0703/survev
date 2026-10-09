#include "UiOverlay.h"
// The main-menu chrome (StartMenu) lives here; this is the only translation
// unit that pulls the extra scene headers the menu needs.
#include "../app/MenuScene.h"
#include "Files.h"
#include "News.h"
#include "../audio/AudioManager.h"

#include <algorithm>
#include <cctype>
#include <cmath>

USING_NS_AX;

namespace ui {

namespace {

// Web palette (client/css/app.css) reused so the native chrome matches exactly.
const ax::Color3B kGreen(131, 175, 80);        // .btn-green  #83af50
const ax::Color3B kGreenShadow(91, 122, 56);   // .btn-green border #5b7a38
const ax::Color3B kDark(122, 122, 122);        // .btn-darken #7a7a7a
const ax::Color3B kDarkShadow(62, 62, 62);     // .btn-darken border #3e3e3e
const ax::Color3B kPanel(0, 0, 0);             // .menu-block rgba(0,0,0,.5)

std::string tr(Localization* loc, const std::string& key, const std::string& fallback) {
    const std::string text = loc ? loc->translate(key) : "";
    return text.empty() ? fallback : text;
}

std::string regionDisplayName(Localization* loc, const std::string& region) {
    if (loc) {
        const std::string translated = loc->translate("index-" + region);
        if (!translated.empty()) {
            return translated;
        }
    }
    std::string upper = region;
    std::transform(upper.begin(), upper.end(), upper.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return upper;
}

Button* makeButton(const std::string& label, float w, float h, bool green, float fontSize) {
    auto* button = Button::create(label, w, h);
    button->setColors(green ? kGreen : kDark, green ? kGreenShadow : kDarkShadow);
    button->setFontSize(fontSize);
    return button;
}

} // namespace

void StartMenu::build(ax::Node* root, Config* config, Localization* loc) {
    _root = root;
    _config = config;
    _loc = loc;

    const float w = kit::designWidth();
    const float h = kit::designHeight();

    // #background: full-screen splash (web client's `cachedBgImg`).
    std::string bgPath = config->getString("cachedBgImg", "img/splashes/main.webp");
    if (bgPath.empty()) {
        bgPath = "img/splashes/main.webp";
    }
    _background = ax::Sprite::create(bgPath);
    if (_background) {
        _background->setAnchorPoint(ax::Vec2(0.0f, 0.0f));
        _background->setPosition(ax::Vec2::ZERO);
        const ax::Size size = _background->getContentSize();
        if (size.width > 0.0f && size.height > 0.0f) {
            _background->setScale(std::fmax(w / size.width, h / size.height));
        }
        root->addChild(_background, 0);
    }

    // #start-overlay: 20% black wash.
    _overlayDim = ax::LayerColor::create(ax::Color4B(0, 0, 0, 51), w, h);
    _overlayDim->setAnchorPoint(ax::Vec2(0.0f, 1.0f));
    _overlayDim->setIgnoreAnchorPointForPosition(false);
    _overlayDim->setPosition(ax::Vec2(0.0f, h));
    root->addChild(_overlayDim, 1);

    // #start-row-header: the logo above the menu columns (touch size: 220px).
    _logo = ax::Sprite::create("img/survev_logo_full.png");
    if (_logo) {
        _logo->setAnchorPoint(ax::Vec2(0.5f, 1.0f));
        _logo->setPosition(ax::Vec2(w * 0.5f, kit::fromCssY(10.0f, h)));
        const ax::Size size = _logo->getContentSize();
        if (size.width > 0.0f) {
            _logo->setScale(220.0f / size.width);
        }
        root->addChild(_logo, 2);
    }

    buildPanel();
    buildNewsBox();
    buildBottomIcons();
    buildSettingsModal();
    buildHelpModal();
    buildJoinModal();
}

void StartMenu::buildPanel() {
    // Web `#start-menu`: a translucent block centred in `#start-row-top`.
    const float panelW = 300.0f;
    const float panelH = 392.0f;
    _panel = Panel::create(panelW, panelH);
    _panel->setBackgroundColor(kPanel, 0.5f);
    _panel->placeCenter(0.0f, 8.0f);
    _root->addChild(_panel, 3);

    const float pad = 20.0f;
    const float fieldW = panelW - pad * 2.0f;
    constexpr float kRow = 40.0f;
    constexpr float kGap = 8.0f;
    float y = 20.0f;

    // Player name (`#player-name-input-solo`): white bg, black bold 16px.
    std::string nameHint = tr(_loc, "index-enter-name-here", "Enter your name here");
    _nameField = TextField::create(nameHint, fieldW, kRow);
    _nameField->setCssPosition(pad, y);
    _nameField->setMaxLength(16);
    _nameField->setWhiteBackground(true);
    _nameField->setTextColor(ax::Color3B(0, 0, 0));
    _nameField->setFontSize(16);
    _nameField->setBold(true);
    _panel->addChild(_nameField);
    y += kRow + kGap;

    // Region selector (web `#server-select-main`, a `.btn-hollow
    // .btn-hollow-selected` box with a caret; taps cycle the region).
    _regionBtn = makeButton("Region", fieldW, kRow, false, 16);
    _regionBtn->setHollowBorder(false);
    _regionBtn->setIcon("down");
    _regionBtn->setIconScale(14.0f);
    _regionBtn->setIconLayout(false);
    _regionBtn->setCssPosition(pad, y);
    _panel->addChild(_regionBtn);
    y += kRow + kGap;

    const char* keys[3] = { "index-play-solo", "index-play-duo", "index-play-squad" };
    const char* fallbacks[3] = { "Play Solo", "Play Duo", "Play Squad" };
    for (int i = 0; i < 3; i++) {
        _playBtns[i] = makeButton(tr(_loc, keys[i], fallbacks[i]), fieldW, kRow, true, 16);
        _playBtns[i]->setCssPosition(pad, y);
        _panel->addChild(_playBtns[i]);
        y += kRow + kGap;
    }

    // Join Team / Create Team. Web `.btn-team-option` is the purple mobile
    // variant (`.btn-green` at the mobile breakpoint: `#9850af`/`#683679`).
    {
        const float half = (fieldW - 4.0f) * 0.5f;
        _joinTeamBtn = makeButton(tr(_loc, "index-join-team", "Join Team"), half, kRow, false, 15);
        _joinTeamBtn->setColors(ax::Color3B(152, 80, 175), ax::Color3B(104, 54, 121));
        _joinTeamBtn->setCssPosition(pad, y);
        _panel->addChild(_joinTeamBtn);

        _createTeamBtn = makeButton(tr(_loc, "index-create-team", "Create Team"), half, kRow, false, 15);
        _createTeamBtn->setColors(ax::Color3B(152, 80, 175), ax::Color3B(104, 54, 121));
        _createTeamBtn->setCssPosition(pad + half + 4.0f, y);
        _panel->addChild(_createTeamBtn);
        y += kRow + kGap;
    }

    // Loadout (`#btn-customize`) + How to Play (`#btn-help`).
    {
        const float half = (fieldW - 4.0f) * 0.5f;
        _customizeBtn = makeButton(tr(_loc, "index-loadout", "Loadout"), half, kRow, false, 15);
        _customizeBtn->setIcon("loadout-outfit");
        _customizeBtn->setIconScale(20.0f);
        _customizeBtn->setIconLayout(true);
        _customizeBtn->setCssPosition(pad, y);
        _panel->addChild(_customizeBtn);

        _helpBtn = makeButton(tr(_loc, "index-how-to-play", "How to Play"), half, kRow, false, 15);
        _helpBtn->setCssPosition(pad + half + 4.0f, y);
        _panel->addChild(_helpBtn);
    }

    // Localized error/status line under the buttons.
    _errorLabel = makeLabel("", 15, true);
    _errorLabel->setAnchorPoint(ax::Vec2(0.5f, 0.0f));
    _errorLabel->setPosition(ax::Vec2(panelW * 0.5f, 8.0f));
    _errorLabel->setTextColor(ax::Color4B(255, 120, 120, 255));
    _errorLabel->setDimensions(panelW - 20.0f, 0);
    _panel->addChild(_errorLabel);
}

void StartMenu::buildNewsBox() {
    // Web `#news-block`: 300px column to the right of the centred menu block.
    const float panelW = 300.0f;
    const float panelH = 340.0f;
    _newsPanel = Panel::create(panelW, panelH);
    _newsPanel->setBackgroundColor(kPanel, 0.5f);
    _newsPanel->setCssPosition((kit::designWidth() + 300.0f) * 0.5f + 30.0f, 52.0f);
    _root->addChild(_newsPanel, 3);

    const float pad = 20.0f;
    float y = 12.0f;
    for (const auto& entry : newsEntries()) {
        // Titles may be an l10n key in some locales; the literal English is the
        // fallback (the web client hard-codes the same strings).
        auto* title = makeLabel(tr(_loc, entry.title, entry.title), 18, true);
        title->setAnchorPoint(ax::Vec2(0.0f, 1.0f));
        title->setTextColor(ax::Color4B(255, 215, 0, 255)); // .news-header gold
        title->setPosition(ax::Vec2(pad, kit::fromCssY(y, panelH)));
        _newsPanel->addChild(title);
        y += 22.0f;

        auto* date = makeLabel(entry.date, 12, false);
        date->setAnchorPoint(ax::Vec2(0.0f, 1.0f));
        date->setTextColor(ax::Color4B(128, 128, 128, 255)); // .news-date grey
        date->setPosition(ax::Vec2(pad, kit::fromCssY(y, panelH)));
        _newsPanel->addChild(date);
        y += 18.0f;

        for (const auto& paragraph : entry.paragraphs) {
            const std::string text = tr(_loc, paragraph.text, paragraph.text);
            ax::Color4B color(255, 255, 255, 255);
            bool bold = false;
            if (paragraph.style == NewsParagraph::Style::Highlight) {
                color = ax::Color4B(255, 215, 0, 255);
                bold = true;
            } else if (paragraph.style == NewsParagraph::Style::Redacted) {
                color = ax::Color4B(255, 0, 0, 255);
                bold = true;
            }
            auto* line = makeLabel(text, 13, bold);
            line->setAnchorPoint(ax::Vec2(0.0f, 1.0f));
            line->setTextColor(color);
            line->setDimensions(panelW - pad * 2.0f, 0);
            line->setPosition(ax::Vec2(pad, kit::fromCssY(y, panelH)));
            _newsPanel->addChild(line);
            y += line->getContentSize().height + 4.0f;
        }
        y += 8.0f;
        if (y > panelH - 24.0f) {
            break;
        }
    }
}

void StartMenu::buildBottomIcons() {
    // Web `#start-bottom-right`: cog + mute, 48x42 with a 4px gap.
    const float w = 48.0f;
    const float h = 42.0f;
    _settingsBtn = Button::create("", w, h);
    _settingsBtn->setColors(kDark, kDarkShadow);
    _settingsBtn->setIcon("cog");
    _settingsBtn->setIconScale(30.0f);
    _settingsBtn->setCssPosition(kit::designWidth() - 12.0f - w * 2.0f - 4.0f,
                                 kit::designHeight() - 12.0f - h);
    _settingsBtn->onClick = [this] { _settingsModal.show(); };
    _root->addChild(_settingsBtn, 5);

    _muteBtn = Button::create("", w, h);
    _muteBtn->setColors(kDark, kDarkShadow);
    _muteBtn->setCssPosition(kit::designWidth() - 12.0f - w, kit::designHeight() - 12.0f - h);
    _muteBtn->onClick = [this] {
        _config->setBool("muteAudio", !_config->boolOrDefault("muteAudio"));
        refresh();
    };
    _root->addChild(_muteBtn, 5);
}

void StartMenu::buildSettingsModal() {
    // Web `#modal-settings`: language, sound toggle and the volume sliders.
    auto* panel = _settingsModal.build(460, 380, tr(_loc, "index-settings", "Settings"));
    const float pad = 24.0f;
    const float innerW = 460.0f - pad * 2.0f;

    auto addLabel = [panel](const std::string& text, float x, float y) {
        auto* label = makeLabel(text, 15, false);
        label->setAnchorPoint(ax::Vec2(0.0f, 1.0f));
        label->setTextColor(ax::Color4B::WHITE);
        label->setPosition(ax::Vec2(x, kit::fromCssY(y, 380.0f)));
        panel->addChild(label);
    };

    addLabel(tr(_loc, "index-settings", "Language"), pad, 72.0f);
    _languageBtn = makeButton(_loc->localeName(_loc->getLocale()), innerW, 38, false, 16);
    _languageBtn->setCssPosition(pad, 90.0f);
    _languageBtn->onClick = [this] {
        const int count = _loc->localeCount();
        const std::string current = _loc->getLocale();
        int index = 0;
        for (int i = 0; i < count; ++i) {
            if (current == _loc->locales()[i].code) {
                index = i;
                break;
            }
        }
        _config->setString("language", _loc->locales()[(index + 1) % count].code);
        _loc->setLocale(_config->stringOrDefault("language"));
        refresh();
    };
    panel->addChild(_languageBtn);

    _modalMuteBtn = makeButton(tr(_loc, "game-sound", "Sound"), innerW, 38, false, 16);
    _modalMuteBtn->setCssPosition(pad, 138.0f);
    _modalMuteBtn->onClick = [this] {
        _config->setBool("muteAudio", !_config->boolOrDefault("muteAudio"));
        refresh();
    };
    panel->addChild(_modalMuteBtn);

    const char* volKeys[3] = { "masterVolume", "soundVolume", "musicVolume" };
    const char* volL10n[3] = { "index-master-volume", "index-sfx-volume", "index-music-volume" };
    const char* volFallback[3] = { "Master Volume", "SFX Volume", "Music Volume" };
    float y = 190.0f;
    _sliders.clear();
    for (int i = 0; i < 3; i++) {
        addLabel(tr(_loc, volL10n[i], volFallback[i]), pad, y);
        auto* slider = Slider::create(innerW, _config->floatOrDefault(volKeys[i]));
        slider->setCssPosition(pad, y + 18.0f);
        const std::string key = volKeys[i];
        slider->onChanged = [this, key](float v) { _config->setFloat(key, v); };
        panel->addChild(slider);
        _sliders.push_back(slider);
        y += 58.0f;
    }

    _root->addChild(_settingsModal.overlay(), 100);
}

void StartMenu::buildHelpModal() {
    // Web `#start-help`: control rows, with the `-touch` variants on mobile.
    auto* panel = _helpModal.build(700.0f, 460.0f, tr(_loc, "index-how-to-play", "How to Play"));
    auto* heading = makeLabel(tr(_loc, "index-controls", "Controls"), 20, true);
    heading->setAnchorPoint(ax::Vec2(0.5f, 1.0f));
    heading->setTextColor(ax::Color4B::WHITE);
    heading->setPosition(ax::Vec2(350.0f, kit::fromCssY(60.0f, 460.0f)));
    panel->addChild(heading);

    const char* lines[] = {
        "index-movement", "index-aim", "index-shoot", "index-change-weapons",
        "index-reload", "index-scope-zoom", "index-pickup", "index-use-medical",
        "index-use-emote", "index-use-ping",
    };
    float y = 104.0f;
    for (const char* key : lines) {
        const std::string action = tr(_loc, key, "");
        if (action.empty()) {
            continue;
        }
        auto* actionLabel = makeLabel(action, 15, false);
        actionLabel->setAnchorPoint(ax::Vec2(0.0f, 1.0f));
        actionLabel->setTextColor(ax::Color4B::WHITE);
        actionLabel->setPosition(ax::Vec2(32.0f, kit::fromCssY(y, 460.0f)));
        panel->addChild(actionLabel);

        const std::string control = tr(_loc, std::string(key) + "-ctrl-touch", "");
        if (!control.empty()) {
            auto* controlLabel = makeLabel(control, 15, true);
            controlLabel->setAnchorPoint(ax::Vec2(1.0f, 1.0f));
            controlLabel->setTextColor(ax::Color4B::WHITE);
            controlLabel->setPosition(ax::Vec2(668.0f, kit::fromCssY(y, 460.0f)));
            panel->addChild(controlLabel);
        }
        y += 30.0f;
    }
    _root->addChild(_helpModal.overlay(), 100);
}

void StartMenu::buildJoinModal() {
    // Web `#team-mobile-link`: paste an invite link/code.
    auto* panel = _joinModal.build(520, 240, tr(_loc, "index-join-team", "Join Team"));
    auto* desc = makeLabel(tr(_loc, "index-join-team-help", "Got a team link or code? Paste it here:"),
                           15, false);
    desc->setAnchorPoint(ax::Vec2(0.5f, 1.0f));
    desc->setTextColor(ax::Color4B::WHITE);
    desc->setDimensions(460.0f, 0);
    desc->setPosition(ax::Vec2(260.0f, kit::fromCssY(66.0f, 240.0f)));
    panel->addChild(desc);

    _roomField = TextField::create("", 460, 46);
    _roomField->setMaxLength(256);
    _roomField->setFontSize(16);
    panel->addChild(_roomField);
    _roomField->setCssPosition(30, 110);

    auto* join = makeButton(tr(_loc, "index-join-team", "Join Team"), 220, 46, true, 16);
    panel->addChild(join);
    join->setCssPosition(30, 168);
    join->onClick = [this] { requestTeam(false); };

    auto* back = makeButton(tr(_loc, "index-back-to-main", "Back to Main Menu"), 220, 46, false, 16);
    panel->addChild(back);
    back->setCssPosition(270, 168);
    back->onClick = [this] { _joinModal.hide(); };

    _root->addChild(_joinModal.overlay(), 100);
}

void StartMenu::requestTeam(bool create) {
    if (create) {
        if (onTeamRequested) {
            onTeamRequested(true);
        }
        return;
    }
    const std::string code = teamInviteCode(_roomField ? _roomField->getText() : std::string());
    if (code.empty()) {
        return;
    }
    _joinModal.hide();
    _pendingRoomCode = code;
    if (onTeamRequested) {
        onTeamRequested(false);
    }
}

const std::string& StartMenu::pendingRoomCode() const { return _pendingRoomCode; }

void StartMenu::setVisible(bool visible) {
    for (ax::Node* node : {static_cast<ax::Node*>(_background), static_cast<ax::Node*>(_overlayDim),
                           static_cast<ax::Node*>(_logo), static_cast<ax::Node*>(_panel),
                           static_cast<ax::Node*>(_newsPanel), static_cast<ax::Node*>(_settingsBtn),
                           static_cast<ax::Node*>(_muteBtn)}) {
        if (node) {
            node->setVisible(visible);
        }
    }
    _visible = visible;
    if (!visible) {
        _settingsModal.hide();
        _helpModal.hide();
        _joinModal.hide();
    }
}

void StartMenu::setTeamVisible(bool team) { (void)team; }

void StartMenu::setPlayEnabled(int index, bool enabled) {
    if (index >= 0 && index < 3 && _playBtns[index]) {
        _playBtns[index]->setEnabled(enabled);
    }
}

void StartMenu::setRegionLabel(const std::string& label) {
    if (_regionBtn) {
        _regionBtn->setLabel(label);
    }
}

void StartMenu::setMuteState(bool muted) {
    if (_muteBtn) {
        _muteBtn->setIcon(muted ? "audio-off" : "audio-on");
        _muteBtn->setIconScale(30.0f);
    }
    if (_modalMuteBtn) {
        _modalMuteBtn->setLabel(muted ? tr(_loc, "game-sound-off", "Sound: Off")
                                      : tr(_loc, "game-sound", "Sound"));
    }
}

void StartMenu::setLanguageLabel(const std::string& label) {
    if (_languageBtn) {
        _languageBtn->setLabel(label);
    }
}

void StartMenu::refresh() {
    if (_nameField) {
        _nameField->setText(_config->getString("playerName", ""));
    }
    setRegionLabel(tr(_loc, "index-region", "Region") + ": "
                   + regionDisplayName(_loc, _config->getString("region", "na")));
    setMuteState(_config->boolOrDefault("muteAudio"));
    setLanguageLabel(_loc->localeName(_loc->getLocale()));
}

void StartMenu::setError(const std::string& l10nKey, const std::string& fallback) {
    if (!_errorLabel) {
        return;
    }
    std::string text = l10nKey.empty() ? std::string() : tr(_loc, l10nKey, fallback);
    _errorLabel->setString(text);
}

void StartMenu::clearError() { setError("", ""); }

} // namespace ui
