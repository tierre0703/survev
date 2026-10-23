#include "PingWheel.h"

#include "EmoteWheel.h"
#include "../render/Defs.h"

#include <algorithm>
#include <cmath>

namespace surv {

namespace {

constexpr uint32_t kButtonFill = 0x000000;
constexpr uint32_t kButtonRing = 0xffffff;
constexpr uint32_t kBackdropFill = 0x101010;
constexpr uint32_t kItemFill = 0x222222;
constexpr uint32_t kHighlightRing = 0xffffff;

constexpr float kPi = 3.14159265358979f;

// emote.ts vectorToDegreeAngle / isAngleBetween sector bounds. `angleA` is the
// counter-clockwise bound, `angleC` the clockwise one (same field names as the
// TS wheel data); `centre` is the sector midpoint where the icon is placed.
struct Sector {
    float angleA = 0.0f;
    float angleC = 0.0f;
    float centre = 0.0f;
    bool major = true;
};

const Sector kSectors[PingWheel::kSelectorCount] = {
    { 0.0f, 0.0f, 0.0f, true },      // Middle (close)
    { 135.0f, 45.0f, 90.0f, true },  // Top: ping_danger
    { 45.0f, 315.0f, 0.0f, true },   // Right: ping_coming
    { 315.0f, 225.0f, 270.0f, true },// Bottom: ping_help
    { 225.0f, 180.0f, 202.5f, false }, // Bottom-left: emote_medical
    { 180.0f, 135.0f, 157.5f, false }, // Top-left: emote_ammo*
};

// emote.ts teamPingData ping types.
const char* const kPingOf[PingWheel::kSelectorCount] = {
    "", "ping_danger", "ping_coming", "ping_help", "", "",
};

// emote.ts teamPingData emote types (TopLeft is filled from the ammo table).
const char* const kEmoteOf[PingWheel::kSelectorCount] = {
    "", "", "", "", "emote_medical", "emote_ammo",
};

const char* const kLabels[PingWheel::kSelectorCount] = {
    "", "Danger", "Coming", "Help", "Medical", "Ammo",
};

const uint32_t kRingColors[PingWheel::kSelectorCount] = {
    0xffffff, 0xff6644, 0xffaa33, 0x44cc66, 0x88ccff, 0xffaa33,
};

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

float itemRadius(int selector) {
    return kSectors[selector].major ? PingWheel::kMajorRadius : PingWheel::kMinorRadius;
}

float itemDistance(int selector) {
    // Both ring radii sit at the same distance; the minor wedges are just
    // narrower, matching the eighth-slice selectors in the web wheel.
    return PingWheel::kMajorDistance;
}

} // namespace

std::string PingWheel::ammoEmoteForAmmo(const std::string& ammoType) {
    // emote.ts AmmoTypeToEmote.
    if (ammoType == "9mm") return "emote_ammo9mm";
    if (ammoType == "12gauge") return "emote_ammo12gauge";
    if (ammoType == "762mm") return "emote_ammo762mm";
    if (ammoType == "556mm") return "emote_ammo556mm";
    if (ammoType == "50AE") return "emote_ammo50ae";
    if (ammoType == "308sub") return "emote_ammo308sub";
    if (ammoType == "flare") return "emote_ammoflare";
    if (ammoType == "45acp") return "emote_ammo45acp";
    return "emote_ammo";
}

const char* PingWheel::pingType(int selector) {
    if (selector < 0 || selector >= kSelectorCount) {
        return "";
    }
    return kPingOf[selector];
}

void PingWheel::build(pix::Factory* factory, pix::Container* root, const Vec2& design) {
    _factory = factory;
    _root = root;
    _design = design;
    if (!_factory || !_root) {
        return;
    }

    // On-screen team-ping button (#ui-emote-button's neighbour).
    _button = _factory->createContainer();
    _root->addChild(_button);
    pix::Graphics* buttonBg = _factory->createGraphics();
    fillCircle(buttonBg, kButtonRadius, kButtonFill, 0.4f);
    _button->addChild(buttonBg);
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

    pix::Graphics* backdrop = _factory->createGraphics();
    fillCircle(backdrop, kOuterRadius, kBackdropFill, 0.28f);
    _wheel->addChild(backdrop);
    pix::Graphics* backdropRing = _factory->createGraphics();
    ringCircle(backdropRing, kOuterRadius, 2.0f, 0xffffff, 0.18f);
    _wheel->addChild(backdropRing);

    for (int i = 1; i < kSelectorCount; ++i) {
        Item& item = _items[i];
        const float dist = itemDistance(i);
        const float angle = kSectors[i].centre * kPi / 180.0f;
        const float rad = itemRadius(i);

        pix::Container* holder = _factory->createContainer();
        holder->setPosition(std::cos(angle) * dist, std::sin(angle) * dist);
        _wheel->addChild(holder);

        item.bg = _factory->createGraphics();
        fillCircle(item.bg, rad, kItemFill, 0.7f);
        holder->addChild(item.bg);

        // Coloured ring identifies the slot even when no icon resolved.
        pix::Graphics* ring = _factory->createGraphics();
        ringCircle(ring, rad, 2.0f, kRingColors[i], 0.75f);
        holder->addChild(ring);

        item.icon = _factory->createSprite();
        item.icon->setAnchor(0.5f, 0.5f);
        holder->addChild(item.icon);

        item.highlight = _factory->createGraphics();
        ringCircle(item.highlight, rad + 4.0f, 4.0f, kHighlightRing, 0.95f);
        item.highlight->setVisible(false);
        holder->addChild(item.highlight);

        item.label = _factory->createText();
        item.label->setFontSize(i < 4 ? 11.0f : 10.0f);
        item.label->setColor(0xffffff, 0x000000, 1.0f, false);
        item.label->setAnchor(0.5f, 0.5f);
        // Keep the label inside the backdrop, below/above the icon.
        const Vec2 labelDir(std::cos(angle), std::sin(angle));
        item.label->setPosition(labelDir.x * (dist + rad * 0.8f), labelDir.y * (dist + rad * 0.8f));
        item.label->setText(kLabels[i]);
        _wheel->addChild(item.label);
    }

    // Centre close selector (emote.ts middle: "close.svg").
    pix::Graphics* closeBg = _factory->createGraphics();
    fillCircle(closeBg, kCloseRadius, 0x333333, 0.85f);
    _wheel->addChild(closeBg);
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
    refreshIcons();
}

void PingWheel::layout() {
    if (!_built) {
        return;
    }
    const float halfW = _design.x * 0.5f;
    const float halfH = _design.y * 0.5f;
    // One gap left of the emote button (game/EmoteWheel.cpp).
    const float emoteButtonX = _design.x - EmoteWheel::kButtonRight;
    _buttonCenter = Vec2(emoteButtonX - kButtonGap, _design.y * 0.5f);
    if (_button) {
        _button->setPosition(_buttonCenter.x - halfW, _buttonCenter.y - halfH);
    }
    if (_wheel) {
        _wheel->setPosition(_center.x - halfW, _center.y - halfH);
    }
}

std::string PingWheel::emoteType(int selector) const {
    if (selector == TopLeft) {
        return _ammoEmote;
    }
    if (selector < 0 || selector >= kSelectorCount) {
        return {};
    }
    return kEmoteOf[selector];
}

void PingWheel::refreshIcons() {
    if (!_built) {
        return;
    }
    const DefProvider* provider = getDefProvider();
    for (int i = 1; i < kSelectorCount; ++i) {
        Item& item = _items[i];
        const char* ping = kPingOf[i];
        std::string texture;
        item.teamOnly = false;
        if (ping && *ping) {
            const PingRenderDef* def = provider ? provider->ping(ping) : nullptr;
            if (def) {
                texture = def->texture;
            }
        } else {
            const std::string emote = emoteType(i);
            const EmoteRenderDef* def = (!emote.empty() && provider) ? provider->emote(emote) : nullptr;
            if (def) {
                texture = def->texture;
                item.teamOnly = def->teamOnly;
            }
        }
        const bool hasIcon = !texture.empty();
        item.icon->setVisible(hasIcon);
        if (hasIcon) {
            item.icon->setFrame(texture);
            const float source = std::max(item.icon->sourceWidth(), item.icon->sourceHeight());
            const float scale = source > 0.0f ? (itemRadius(i) * 1.5f) / source : 1.0f;
            item.icon->setScale(scale, scale);
        }
    }
    // Button art: the danger ping icon (the web's team-ping button).
    if (_buttonIcon) {
        const PingRenderDef* danger = provider ? provider->ping("ping_danger") : nullptr;
        const bool hasIcon = danger != nullptr && !danger->texture.empty();
        _buttonIcon->setVisible(hasIcon);
        if (hasIcon) {
            _buttonIcon->setFrame(danger->texture);
            const float source = std::max(_buttonIcon->sourceWidth(), _buttonIcon->sourceHeight());
            const float scale = source > 0.0f ? (kButtonRadius * 1.7f) / source : 1.0f;
            _buttonIcon->setScale(scale, scale);
        }
    }
    updateGrey();
}

void PingWheel::refreshAmmoIcon() {
    if (!_built) {
        return;
    }
    Item& item = _items[TopLeft];
    const DefProvider* provider = getDefProvider();
    const EmoteRenderDef* def = provider ? provider->emote(_ammoEmote) : nullptr;
    const std::string texture = def ? def->texture : std::string();
    const bool hasIcon = !texture.empty();
    item.icon->setVisible(hasIcon);
    if (hasIcon) {
        item.icon->setFrame(texture);
        const float source = std::max(item.icon->sourceWidth(), item.icon->sourceHeight());
        const float scale = source > 0.0f ? (itemRadius(TopLeft) * 1.5f) / source : 1.0f;
        item.icon->setScale(scale, scale);
    }
    item.teamOnly = def != nullptr && def->teamOnly;
    updateGrey();
}

void PingWheel::setAmmoType(const std::string& ammoType) {
    if (_ammoType == ammoType) {
        return;
    }
    _ammoType = ammoType;
    const std::string next = ammoEmoteForAmmo(ammoType);
    if (next == _ammoEmote) {
        return;
    }
    _ammoEmote = next;
    refreshAmmoIcon();
}

void PingWheel::setTeamMode(bool solo) {
    if (_solo == solo) {
        return;
    }
    _solo = solo;
    updateGrey();
}

void PingWheel::setGreyed(bool greyed) {
    if (_greyed == greyed) {
        return;
    }
    _greyed = greyed;
    updateGrey();
}

void PingWheel::updateGrey() {
    if (!_built) {
        return;
    }
    for (int i = 1; i < kSelectorCount; ++i) {
        Item& item = _items[i];
        const bool disabled = (item.teamOnly && _solo) || _greyed;
        item.icon->setAlpha(disabled ? 0.35f : 1.0f);
        item.label->setAlpha(disabled ? 0.4f : 1.0f);
    }
}

void PingWheel::setHudVisible(bool visible) {
    const bool becameVisible = visible && !_hudVisible;
    _hudVisible = visible;
    if (_button) {
        _button->setVisible(visible);
    }
    if (!visible && _active) {
        close();
    }
    // The HUD turns visible after the map (and its atlases) loaded, so this is
    // the first point where the ping icons can resolve. open() refreshes again.
    if (becameVisible) {
        refreshIcons();
    }
}

void PingWheel::open(const Vec2& center) {
    if (!_built) {
        return;
    }
    // Atlases are registered on MapMsg, which can arrive after build().
    refreshIcons();
    _center = center;
    _active = true;
    _openT = 0.0f;
    _idle = 0.0f;
    _highlighted = -1;
    applyHighlight();
    _wheel->setVisible(true);
    _wheel->setScale(0.85f, 0.85f);
    _wheel->setAlpha(1.0f);
    updateGrey();
    layout();
}

void PingWheel::close() {
    _active = false;
    _highlighted = -1;
    _openT = 0.0f;
    applyHighlight();
    if (_wheel) {
        _wheel->setVisible(false);
    }
}

bool PingWheel::isAngleBetween(float target, float angle1, float angle2) {
    // emote.ts isAngleBetween, verbatim.
    if (angle1 <= angle2) {
        if (angle2 - angle1 <= 180.0f) {
            return angle1 <= target && target <= angle2;
        }
        return angle2 <= target || target <= angle1;
    }
    if (angle1 - angle2 <= 180.0f) {
        return angle2 <= target && target <= angle1;
    }
    return angle1 <= target || target <= angle2;
}

int PingWheel::selectorAt(float dx, float dy) {
    const float dist = std::sqrt(dx * dx + dy * dy);
    if (dist <= kDeadZone) {
        return Middle;
    }
    float angle = std::atan2(dy, dx) * 180.0f / kPi;
    if (angle < 0.0f) {
        angle += 360.0f;
    }
    // emote.ts passes (angleB, angleC, angleA).
    for (int i = 1; i < kSelectorCount; ++i) {
        if (isAngleBetween(angle, kSectors[i].angleC, kSectors[i].angleA)) {
            return i;
        }
    }
    return -1;
}

bool PingWheel::itemUsable(int selector) const {
    if (selector <= Middle || selector >= kSelectorCount) {
        return false;
    }
    if (_greyed) {
        return false;
    }
    const char* ping = kPingOf[selector];
    if (ping && *ping) {
        return true;
    }
    const std::string emote = emoteType(selector);
    if (emote.empty()) {
        return false;
    }
    return !(_items[selector].teamOnly && _solo);
}

bool PingWheel::select(const Vec2& pos, Selection& out) {
    if (!_active) {
        return false;
    }
    _idle = 0.0f;
    const int idx = selectorAt(pos.x - _center.x, pos.y - _center.y);
    const int nextHighlight = _greyed ? -1 : idx;
    if (nextHighlight != _highlighted) {
        _highlighted = nextHighlight;
        applyHighlight();
    }
    if (!itemUsable(idx)) {
        return false;
    }
    const char* ping = kPingOf[idx];
    if (ping && *ping) {
        out.ping = ping;
        out.emote.clear();
        out.ammoEmote = false;
        return true;
    }
    const std::string emote = emoteType(idx);
    if (emote.empty()) {
        return false;
    }
    out.ping.clear();
    out.emote = emote;
    out.ammoEmote = idx == TopLeft;
    return true;
}

void PingWheel::applyHighlight() {
    for (int i = 1; i < kSelectorCount; ++i) {
        if (_items[i].highlight) {
            _items[i].highlight->setVisible(i == _highlighted);
        }
    }
}

void PingWheel::update(float dt) {
    if (!_button || !_wheel) {
        return;
    }
    _button->setVisible(_hudVisible);
    if (!_active) {
        return;
    }
    _idle += dt;
    if (_idle > 10.0f) {
        // emote.ts emoteTimeout: an untouched wheel closes after 10s.
        close();
        return;
    }
    _openT += dt;
    const float t = std::min(_openT / kOpenDuration, 1.0f);
    const float eased = 1.0f - (1.0f - t) * (1.0f - t);
    const float scale = 0.85f + 0.15f * eased;
    _wheel->setScale(scale, scale);
    _wheel->setAlpha(_greyed ? 0.5f : 1.0f);
}

bool PingWheel::buttonHit(float designX, float designY) const {
    if (!_built || !_hudVisible) {
        return false;
    }
    const float dx = designX - _buttonCenter.x;
    const float dy = designY - _buttonCenter.y;
    const float reach = kButtonRadius * 1.5f;
    return dx * dx + dy * dy <= reach * reach;
}

} // namespace surv
