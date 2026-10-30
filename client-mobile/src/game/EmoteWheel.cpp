#include "EmoteWheel.h"

#include "../render/Defs.h"

#include <algorithm>
#include <cmath>

namespace surv {

namespace {

constexpr uint32_t kButtonFill = 0x000000;
constexpr uint32_t kButtonRing = 0xffffff;
constexpr uint32_t kBackdropFill = 0x101010;
constexpr uint32_t kWedgeFill = 0x222222;
constexpr uint32_t kHighlightRing = 0xffffff;

// Graphics can draw either a fill or a stroked path (AxGraphics switches to
// DrawNode line drawing when lineStyle is set), so fills and rings are separate
// nodes.
void fillCircle(pix::Graphics* g, float radius, uint32_t color, float alpha) {
    if (!g) {
        return;
    }
    g->clear();
    g->beginFill(color, alpha);
    g->drawCircle(0.0f, 0.0f, radius);
    g->endFill();
}

void ringCircle(pix::Graphics* g, float radius, float width, uint32_t color, float alpha) {
    if (!g) {
        return;
    }
    g->clear();
    g->lineStyle(width, color, alpha);
    g->drawCircle(0.0f, 0.0f, radius);
}

} // namespace

void EmoteWheel::build(pix::Factory* factory, pix::Container* root, const Vec2& design) {
    _factory = factory;
    _root = root;
    _design = design;
    if (!_factory || !_root) {
        return;
    }

    // #ui-emote-button: round button with the static gas icon. The circle/ring
    // stay visible even when no texture resolved yet, so the button is always
    // tappable.
    _button = _factory->createContainer();
    _root->addChild(_button);
    _buttonBg = _factory->createGraphics();
    fillCircle(_buttonBg, kButtonRadius, kButtonFill, 0.4f);
    _button->addChild(_buttonBg);
    pix::Graphics* buttonRing = _factory->createGraphics();
    ringCircle(buttonRing, kButtonRadius, 2.0f, kButtonRing, 0.55f);
    _button->addChild(buttonRing);
    _buttonIcon = _factory->createSprite();
    _buttonIcon->setAnchor(0.5f, 0.5f);
    _buttonIcon->setVisible(false);
    _button->addChild(_buttonIcon);
    _button->setVisible(false);

    // --- Wheel ---
    _wheel = _factory->createContainer();
    _root->addChild(_wheel);
    _wheel->setVisible(false);

    _backdrop = _factory->createGraphics();
    fillCircle(_backdrop, kOuterRadius, kBackdropFill, 0.28f);
    _wheel->addChild(_backdrop);
    pix::Graphics* backdropRing = _factory->createGraphics();
    ringCircle(backdropRing, kOuterRadius, 2.0f, 0xffffff, 0.18f);
    _wheel->addChild(backdropRing);

    // Wedge order matches gameConfig.ts EmoteSlot: top, right, bottom, left.
    static const float offsets[4][2] = {
        { 0.0f, +kWedgeDistance },
        { +kWedgeDistance, 0.0f },
        { 0.0f, -kWedgeDistance },
        { -kWedgeDistance, 0.0f },
    };
    static const uint32_t wedgeColors[4] = { 0x4488ff, 0xff6644, 0x44cc66, 0xffaa33 };

    for (int i = 0; i < 4; ++i) {
        Wedge& w = _wedges[i];
        pix::Container* holder = _factory->createContainer();
        holder->setPosition(offsets[i][0], offsets[i][1]);
        _wheel->addChild(holder);

        w.bg = _factory->createGraphics();
        fillCircle(w.bg, kWedgeRadius, kWedgeFill, 0.7f);
        holder->addChild(w.bg);

        // Coloured ring identifies the slot even when no emote is assigned.
        pix::Graphics* ring = _factory->createGraphics();
        ringCircle(ring, kWedgeRadius, 2.0f, wedgeColors[i], 0.75f);
        holder->addChild(ring);

        w.icon = _factory->createSprite();
        w.icon->setAnchor(0.5f, 0.5f);
        holder->addChild(w.icon);

        w.highlight = _factory->createGraphics();
        ringCircle(w.highlight, kWedgeRadius + 4.0f, 4.0f, kHighlightRing, 0.95f);
        w.highlight->setVisible(false);
        holder->addChild(w.highlight);
    }

    // Centre close icon (emote.ts middle selector: "close.svg").
    _closeBg = _factory->createGraphics();
    fillCircle(_closeBg, kCloseRadius, 0x333333, 0.85f);
    _wheel->addChild(_closeBg);
    pix::Graphics* closeRing = _factory->createGraphics();
    ringCircle(closeRing, kCloseRadius, 2.0f, 0xffffff, 0.5f);
    _wheel->addChild(closeRing);
    _closeText = _factory->createText();
    _closeText->setFontSize(20.0f);
    _closeText->setColor(0xffffff, 0x000000, 0.0f, true);
    _closeText->setAnchor(0.5f, 0.5f);
    _closeText->setText("X");
    _wheel->addChild(_closeText);

    _built = true;
    layout();
    refreshWedges();
}

void EmoteWheel::setButtonPosition(const Vec2& center) {
    _buttonPosOverride = center;
    _buttonPosCustom = true;
    layout();
}

void EmoteWheel::layout() {
    if (!_built) {
        return;
    }
    const float halfW = _design.x * 0.5f;
    const float halfH = _design.y * 0.5f;
    _buttonCenter = _buttonPosCustom
        ? Vec2(_buttonPosOverride.x, _buttonPosOverride.y)
        : Vec2(_design.x - kButtonRight, _design.y * 0.5f);
    if (_button) {
        _button->setPosition(_buttonCenter.x - halfW, _buttonCenter.y - halfH);
    }
    // The wheel container is centred on the trigger point; children are laid
    // out around its origin.
    if (_wheel) {
        _wheel->setPosition(_center.x - halfW, _center.y - halfH);
    }
}

void EmoteWheel::setLoadout(const std::vector<std::string>& emotes) {
    // Called every frame by GameScene; compare first so unchanged loads do not
    // rebuild the wedge icons every frame.
    bool same = true;
    for (size_t i = 0; i < _loadout.size(); ++i) {
        const std::string& next = i < emotes.size() ? emotes[i] : std::string();
        if (_loadout[i] != next) {
            same = false;
            break;
        }
    }
    if (same) {
        return;
    }
    for (size_t i = 0; i < _loadout.size(); ++i) {
        _loadout[i] = i < emotes.size() ? emotes[i] : std::string();
    }
    refreshWedges();
}

void EmoteWheel::setTeamMode(bool solo) {
    if (_solo == solo) {
        return;
    }
    _solo = solo;
    refreshWedges();
}

void EmoteWheel::refreshWedges() {
    if (!_built) {
        return;
    }
    const DefProvider* provider = getDefProvider();
    for (int i = 0; i < 4; ++i) {
        Wedge& w = _wedges[i];
        const std::string& type = _loadout[i];
        const EmoteRenderDef* def = (!type.empty() && provider) ? provider->emote(type) : nullptr;
        w.teamOnly = def != nullptr && def->teamOnly;

        const bool disabled = type.empty() || (w.teamOnly && _solo);

        const bool hasIcon = def != nullptr && !def->texture.empty();
        w.icon->setVisible(hasIcon);
        w.icon->setAlpha(disabled ? 0.35f : 1.0f);
        if (hasIcon) {
            w.icon->setFrame(def->texture);
            // Fit the atlas frame inside the wedge circle.
            const float source = std::max(w.icon->sourceWidth(), w.icon->sourceHeight());
            const float scale = source > 0.0f ? (kWedgeRadius * 1.5f) / source : 1.0f;
            w.icon->setScale(scale, scale);
        }
    }

    // Button icon: the static gas icon (EmoteWheel::kButtonIconFrame). The
    // loose "gui/*.png" asset resolves before the map atlases are registered,
    // so the art shows right away.
    if (_buttonIcon) {
        _buttonIcon->setFrame(kButtonIconFrame);
        _buttonIcon->setVisible(true);
        const float source = std::max(_buttonIcon->sourceWidth(), _buttonIcon->sourceHeight());
        const float scale = source > 0.0f ? (kButtonRadius * 1.7f) / source : 1.0f;
        _buttonIcon->setScale(scale, scale);
    }
}

void EmoteWheel::refreshFrames() {
    // Build-time setFrame() calls run before the map atlases are registered
    // (GameScene builds the wheels in resetWorld(), the atlas load happens on
    // the first MapMsg), so re-resolve them once the frames exist.
    refreshWedges();
}

void EmoteWheel::setHudVisible(bool visible) {
    _hudVisible = visible;
    if (_button) {
        _button->setVisible(visible);
    }
    if (!visible && _active) {
        close();
    }
}

void EmoteWheel::open(const Vec2& center) {
    if (!_built) {
        return;
    }
    // The atlas for the loadout emotes is registered on MapMsg, which can
    // arrive after build(); re-resolve the frames here.
    refreshWedges();
    // Clamp the wheel centre so the whole (1.5x) ring stays inside the design
    // space (GameScene opens at the design centre; the clamp is a safety net).
    const float outer = kOuterRadius * kGeometryScale;
    _center = Vec2(
        std::clamp(center.x, outer + 16.0f, _design.x - outer - 16.0f),
        std::clamp(center.y, outer + 16.0f, _design.y - outer - 16.0f));
    _active = true;
    _openT = 0.0f;
    _highlighted = -1;
    applyHighlight();
    _wheel->setVisible(true);
    _wheel->setScale(0.85f * kGeometryScale, 0.85f * kGeometryScale);
    _wheel->setAlpha(canSend() ? 1.0f : 0.5f);
    layout();
}

void EmoteWheel::close() {
    _active = false;
    _highlighted = -1;
    _openT = 0.0f;
    applyHighlight();
    if (_wheel) {
        _wheel->setVisible(false);
    }
}

int EmoteWheel::wedgeAt(float dx, float dy) {
    const float dist = std::sqrt(dx * dx + dy * dy);
    if (dist <= kDeadZone) {
        return 4; // centre / close
    }
    // Design space is y-up, so +dy is the top wedge.
    if (std::fabs(dx) > std::fabs(dy)) {
        return dx > 0.0f ? 1 : 3;
    }
    return dy > 0.0f ? 0 : 2;
}

EmoteWheel::TapResult EmoteWheel::tap(const Vec2& pos, std::string& outType) {
    outType.clear();
    if (!_active) {
        return TapResult::None;
    }
    const float dx = pos.x - _center.x;
    const float dy = pos.y - _center.y;
    const float outer = kOuterRadius * kGeometryScale;
    // Touches outside the ring are the dismiss gesture (GameScene handles
    // them), they never select.
    if (dx * dx + dy * dy > outer * outer) {
        return TapResult::None;
    }
    const int idx = wedgeAt(dx / kGeometryScale, dy / kGeometryScale);
    // Centre dead zone is the close selector (emote.ts middle: "close.svg");
    // tap-select closes on it without sending.
    if (idx < 0 || idx > 3) {
        _highlighted = -1;
        applyHighlight();
        return TapResult::Close;
    }
    _highlighted = idx;
    applyHighlight();
    if (!canSend()) {
        return TapResult::None;
    }
    const std::string& type = _loadout[idx];
    if (type.empty() || (_wedges[idx].teamOnly && _solo)) {
        return TapResult::None;
    }
    outType = type;
    return TapResult::Send;
}

void EmoteWheel::applyHighlight() {
    for (int i = 0; i < 4; ++i) {
        if (_wedges[i].highlight) {
            _wedges[i].highlight->setVisible(i == _highlighted);
        }
    }
}

void EmoteWheel::update(float dt) {
    updateThrottle(dt);
    if (!_button || !_wheel) {
        return;
    }
    _button->setVisible(_hudVisible);
    if (!_active) {
        return;
    }
    _openT += dt;
    const float t = std::min(_openT / kOpenDuration, 1.0f);
    const float eased = 1.0f - (1.0f - t) * (1.0f - t);
    const float scale = kGeometryScale * (0.85f + 0.15f * eased);
    _wheel->setScale(scale, scale);
    _wheel->setAlpha(canSend() ? 1.0f : 0.5f);
}

// emote.ts m_update: the hard cooldown only drains while the counter is at the
// threshold; otherwise the counter decays one emote per soft cooldown tick.
void EmoteWheel::updateThrottle(float dt) {
    _softTicker -= dt;
    if (_counter >= PlayerConfig::emoteThreshold && _hardTicker > 0.0f) {
        _hardTicker -= dt;
        if (_hardTicker < 0.0f) {
            _counter = 0;
        }
    } else if (_softTicker < 0.0f && _counter > 0) {
        --_counter;
        _softTicker = PlayerConfig::emoteSoftCooldown * 1.5f;
    }
}

void EmoteWheel::incrementEmote() {
    ++_counter;
    if (_counter >= PlayerConfig::emoteThreshold) {
        if (_hardTicker <= 0.0f) {
            _hardTicker = PlayerConfig::emoteHardCooldown * 1.5f;
        }
    }
}

void EmoteWheel::resetThrottle() {
    _softTicker = 0.0f;
    _hardTicker = 0.0f;
    _counter = 0;
}

bool EmoteWheel::buttonHit(float designX, float designY) const {
    if (!_built || !_hudVisible) {
        return false;
    }
    const float dx = designX - _buttonCenter.x;
    const float dy = designY - _buttonCenter.y;
    const float reach = kButtonRadius * 1.5f; // forgiving for a finger
    const bool hit = dx * dx + dy * dy <= reach * reach;
#ifdef SURV_WHEEL_DEBUG
    std::printf("[ddd] emote hit=%d design=(%g,%g) center=(%g,%g)\n", (int)hit, designX, designY,
                _buttonCenter.x, _buttonCenter.y);
#endif
    return hit;
}

bool EmoteWheel::wheelContains(float designX, float designY) const {
    if (!_built || !_active) {
        return false;
    }
    const float dx = designX - _center.x;
    const float dy = designY - _center.y;
    const float outer = kOuterRadius * kGeometryScale;
    return dx * dx + dy * dy <= outer * outer;
}

} // namespace surv
