#include "MapView.h"

#include <algorithm>
#include <cmath>

namespace surv {

MapDotStyle mapDotStyle(bool sameGroup, bool dead, bool downed, const std::string& aliveIcon,
                        const std::string& deadIcon, float dotScale) {
    MapDotStyle style;
    style.custom = !aliveIcon.empty();
    if (dead) {
        style.texture = deadIcon.empty() ? "skull-outlined.img" : deadIcon;
    } else if (downed) {
        style.texture = sameGroup ? "player-group-downed.img" : "player-map-downed.img";
    } else if (style.custom) {
        style.texture = aliveIcon;
    }
    const float factor = sameGroup
        ? (dead ? 1.5f : style.custom ? 1.25f : 1.0f)
        : (dead || downed || style.custom ? 1.25f : 0.75f);
    style.scale = dotScale * factor;
    style.outer = sameGroup && !style.custom;
    style.zBump = style.custom ? 65535 : 0;
    return style;
}

void MapView::build(pix::Factory* factory, pix::Container* parent, const Vec2& pos, float size) {
    _factory = factory;
    _pos = pos;
    _parent = parent;
    _size = size > 1.0f ? size : 1.0f;
    if (!_factory || !parent) {
        return;
    }

    _root = _factory->createContainer();
    _root->setPosition(_pos.x, _pos.y);
    parent->addChild(_root);

    _map = _factory->createSprite();
    _map->setAnchor(0.0f, 0.0f);
    _map->setLocalZOrder(0);
    _root->addChild(_map);

    _gasGfx = _factory->createGraphics();
    // Initialize the DrawNode's buffers even when there is no gas yet: a
    // brand-new contentless DrawNode submits an empty command and crashes
    // axmol's GL index upload on the emulator.
    _gasGfx->clear();
    _gasGfx->setLocalZOrder(1);
    _root->addChild(_gasGfx);

    _strikeGfx = _factory->createGraphics();
    _strikeGfx->clear();
    _strikeGfx->setLocalZOrder(1);
    _root->addChild(_strikeGfx);

    _labelLayer = _factory->createContainer();
    _labelLayer->setLocalZOrder(2);
    _root->addChild(_labelLayer);

    _dotLayer = _factory->createContainer();
    _dotLayer->setLocalZOrder(3);
    _root->addChild(_dotLayer);

    // The ping layer is created lazily by setPings() (see ensurePingLayer):
    // creating a ClippingNode during HUD build and rendering it later crashed
    // axmol's GL path, while the previous lazily-masked ping layer worked.
    _root->setVisible(_visible);
}

void MapView::setTexture(pix::RenderTexture* texture) {
    if (!_map || !texture) {
        return;
    }
    _texture = texture;
    _map->setRenderTexture(texture);
    updateViewTransform();
}

void MapView::setZoomed(bool zoomed) {
    _zoomed = zoomed;
    updateViewTransform();
}

void MapView::setCenter(const Vec2& worldPos) {
    _center = worldPos;
    updateViewTransform();
    // The minimap needs to clip the oversized map texture / overlays. Do this
    // lazily (during gameplay) rather than in build(): creating a ClippingNode
    // while the HUD is being built then rendering it later crashed axmol.
    if (_zoomed) {
        ensureClip();
    }
}

void MapView::updateViewTransform() {
    // ui.ts: the minimap mapSprite is `screenScaleFactor * 1600 / 1.2` pixels
    // (~1000px desktop) inside a 256px window, i.e. it shows 25.6% of the map.
    // Use the same fraction for the 140px native window; the big map fits.
    _mapScale = _zoomed ? _size / kMinimapViewFraction : _size;
    if (_zoomed && _mapW > 0.0f && _mapH > 0.0f) {
        _mapOffset = Vec2(_size * 0.5f - (_center.x / _mapW) * _mapScale,
                          _size * 0.5f - (_center.y / _mapH) * _mapScale);
    } else {
        _mapOffset = Vec2(0.0f, 0.0f);
    }

    if (_map && _texture) {
        const float texW = _texture->width();
        const float texH = _texture->height();
        _map->setScale(texW > 0.0f ? _mapScale / texW : 1.0f,
                       texH > 0.0f ? _mapScale / texH : 1.0f);
        _map->setPosition(_mapOffset.x, _mapOffset.y);
        // ui.ts displayMiniMap: mapSprite.alpha = 0.8 on the minimap.
        _map->setAlpha(_zoomed ? 0.8f : 1.0f);
    }
}

void MapView::ensureClip() {
    if (_clipMask || !_factory || !_root) {
        return;
    }
    _clipMask = _factory->createGraphics();
    _clipMask->beginFill(0xffffff, 1.0f);
    _clipMask->drawRect(0.0f, 0.0f, _size, _size);
    _clipMask->endFill();
    // The stencil lives in the clipper's parent space; the view root is at
    // _pos, so the clip rect must be offset to match it.
    _clipMask->setPosition(_pos.x, _pos.y);
    _root->setMask(_clipMask);
}

void MapView::setVisible(bool visible) {
    _visible = visible;
    if (_root) {
        _root->setVisible(visible);
    }
    if (_pingLayer) {
        _pingLayer->setVisible(visible);
    }
}

Vec2 MapView::project(const Vec2& worldPos) const {
    if (_mapW <= 0.0f || _mapH <= 0.0f || _mapScale <= 0.0f) {
        return Vec2(0.0f, 0.0f);
    }
    return Vec2(_mapOffset.x + worldPos.x / _mapW * _mapScale,
                _mapOffset.y + worldPos.y / _mapH * _mapScale);
}

Vec2 MapView::unproject(const Vec2& localPos) const {
    if (_mapW <= 0.0f || _mapH <= 0.0f || _mapScale <= 0.0f) {
        return Vec2(0.0f, 0.0f);
    }
    return Vec2((localPos.x - _mapOffset.x) / _mapScale * _mapW,
                (localPos.y - _mapOffset.y) / _mapScale * _mapH);
}

bool MapView::contains(const Vec2& parentPos) const {
    return parentPos.x >= _pos.x && parentPos.x <= _pos.x + _size && parentPos.y >= _pos.y &&
           parentPos.y <= _pos.y + _size;
}

void MapView::setDots(const std::vector<MapDot>& dots) {
    if (!_dotLayer) {
        return;
    }
    int used = 0;
    const float artScale = _size / kTextureSize;
    for (const auto& dot : dots) {
        if (dot.texture.empty() || used >= kDotPool) {
            continue;
        }
        pix::Sprite* sprite = _dotSprites[static_cast<size_t>(used)];
        if (!sprite) {
            sprite = _factory->createSprite();
            sprite->setAnchor(0.5f, 0.5f);
            _dotLayer->addChild(sprite);
            _dotSprites[static_cast<size_t>(used)] = sprite;
        }
        const Vec2 p = project(dot.pos);
        sprite->setPosition(p.x, p.y);
        sprite->setFrame(dot.texture);
        const float s = dot.scale * artScale;
        sprite->setScale(s, s);
        sprite->setTint(dot.tint);
        sprite->setAlpha(dot.alpha);
        sprite->setLocalZOrder(dot.zOrder);
        sprite->setVisible(true);
        ++used;
    }
    for (int i = used; i < kDotPool; ++i) {
        if (_dotSprites[static_cast<size_t>(i)]) {
            _dotSprites[static_cast<size_t>(i)]->setVisible(false);
        }
    }
}

void MapView::ensurePingLayer() {
    if (_pingLayer || !_factory || !_parent) {
        return;
    }
    _pingLayer = _factory->createContainer();
    _pingLayer->setPosition(_pos.x, _pos.y);
    _parent->addChild(_pingLayer);
    pix::Graphics* mask = _factory->createGraphics();
    mask->beginFill(0xffffff, 1.0f);
    mask->drawRect(0.0f, 0.0f, _size, _size);
    mask->endFill();
    // Stencil coordinates are in the clipper's parent space, not the layer's.
    mask->setPosition(_pos.x, _pos.y);
    _pingLayer->setMask(mask);
    _pingLayer->setVisible(_visible);
}

void MapView::setPings(const std::vector<MapPing>& pings) {
    ensurePingLayer();
    if (!_pingLayer) {
        return;
    }
    int used = 0;
    for (const auto& ping : pings) {
        if (!ping.active || ping.texture.empty() || used >= kPingPool) {
            continue;
        }
        pix::Sprite* sprite = _pingSprites[static_cast<size_t>(used)];
        if (!sprite) {
            sprite = _factory->createSprite();
            sprite->setAnchor(0.5f, 0.5f);
            _pingLayer->addChild(sprite);
            _pingSprites[static_cast<size_t>(used)] = sprite;
        }
        const Vec2 p = project(ping.pos);
        sprite->setPosition(p.x, p.y);
        sprite->setFrame(ping.texture);
        // ui.ts createPing renders the map ping textures at a raw canvas
        // scale (map events 0.3 / player pings 0.15) on the web's ~750px map
        // sprite, i.e. ping.scale * 36 CSS px (64px atlas art * 0.5626 DOM
        // zoom) on both the minimap and the big map. Scale the sprite from
        // its own frame size to that target so airdrop markers stay small.
        const float source = std::max(sprite->sourceWidth(), sprite->sourceHeight());
        const float s = source > 0.0f ? ping.scale * kPingDisplayPx / source : 0.0f;
        sprite->setScale(s, s);
        sprite->setTint(ping.tint);

        // mapSprite.ts: fade in over 0.1s, then fade out over the last 0.5s.
        float alpha = 1.0f;
        if (ping.ticker < 0.1f) {
            alpha = ping.ticker / 0.1f;
        }
        if (ping.life < 0.5f) {
            alpha = std::min(alpha, std::max(ping.life / 0.5f, 0.0f));
        }
        sprite->setAlpha(alpha);
        // Pulses under their icon (mapSprite.zOrder 99 vs 100).
        sprite->setLocalZOrder(ping.pulse ? 0 : 1);
        sprite->setVisible(true);
        ++used;
    }
    for (int i = used; i < kPingPool; ++i) {
        if (_pingSprites[static_cast<size_t>(i)]) {
            _pingSprites[static_cast<size_t>(i)]->setVisible(false);
        }
    }
}

void MapView::setIndicators(const std::vector<MapIndicatorMarker>& indicators) {
    if (!_dotLayer) {
        return;
    }
    int used = 0;
    const float artScale = _size / kTextureSize;
    for (const auto& marker : indicators) {
        if (marker.texture.empty() || used >= kIndicatorPool) {
            continue;
        }
        pix::Sprite* sprite = _indicatorSprites[static_cast<size_t>(used)];
        if (!sprite) {
            sprite = _factory->createSprite();
            sprite->setAnchor(0.5f, 0.5f);
            _dotLayer->addChild(sprite);
            _indicatorSprites[static_cast<size_t>(used)] = sprite;
        }
        const Vec2 p = project(marker.pos);
        sprite->setPosition(p.x, p.y);
        sprite->setFrame(marker.texture);
        const float s = marker.scale * artScale;
        sprite->setScale(s, s);
        sprite->setTint(marker.tint);
        sprite->setAlpha(1.0f);
        // mapIndicator.ts zOrder: an equipped indicator sorts above every player
        // dot, a plain one below them.
        sprite->setLocalZOrder(indicatorZOrder(marker.equipped));
        sprite->setVisible(true);

        // The pulse ring only exists for defs with `mapIndicator.pulse` and is
        // drawn for the equipped holder (mapIndicator.ts updateIndicatorPulses).
        pix::Sprite* pulse = _indicatorPulseSprites[static_cast<size_t>(used)];
        if (marker.pulse) {
            if (!pulse) {
                pulse = _factory->createSprite();
                pulse->setAnchor(0.5f, 0.5f);
                _dotLayer->addChild(pulse);
                _indicatorPulseSprites[static_cast<size_t>(used)] = pulse;
            }
            pulse->setPosition(p.x, p.y);
            pulse->setFrame("part-pulse-01.img");
            const float ps = marker.pulseScale * artScale;
            pulse->setScale(ps, ps);
            pulse->setTint(marker.pulseTint);
            pulse->setAlpha(1.0f);
            pulse->setLocalZOrder(indicatorPulseZOrder(marker.equipped));
            pulse->setVisible(true);
        } else if (pulse) {
            pulse->setVisible(false);
        }
        ++used;
    }
    for (int i = used; i < kIndicatorPool; ++i) {
        if (_indicatorSprites[static_cast<size_t>(i)]) {
            _indicatorSprites[static_cast<size_t>(i)]->setVisible(false);
        }
        if (_indicatorPulseSprites[static_cast<size_t>(i)]) {
            _indicatorPulseSprites[static_cast<size_t>(i)]->setVisible(false);
        }
    }
}

void MapView::setLabels(const std::vector<MapPlace>& places) {
    if (!_labelLayer) {
        return;
    }
    const float fontSize = std::max(7.0f, _size / kTextureSize * 14.0f);
    int used = 0;
    for (const auto& place : places) {
        if (place.name.empty() || used >= kLabelPool) {
            continue;
        }
        pix::Text* text = _labels[static_cast<size_t>(used)];
        if (!text) {
            text = _factory->createText();
            text->setAnchor(0.5f, 0.5f);
            text->setColor(0xffffff, 0x000000, 1.0f, true);
            _labelLayer->addChild(text);
            _labels[static_cast<size_t>(used)] = text;
        }
        // MapMsg places are normalized (0..1), like map.ts renderMap.
        text->setFontSize(fontSize);
        text->setPosition(_mapOffset.x + place.pos.x * _mapScale,
                          _mapOffset.y + place.pos.y * _mapScale);
        text->setText(place.name);
        text->setVisible(true);
        ++used;
    }
    for (int i = used; i < kLabelPool; ++i) {
        if (_labels[static_cast<size_t>(i)]) {
            _labels[static_cast<size_t>(i)]->setVisible(false);
        }
    }
}

void MapView::setAirstrikes(const std::vector<MapAirstrike>& strikes) {
    if (!_strikeGfx) {
        return;
    }
    _strikeGfx->clear();
    if (_mapW <= 0.0f || _mapH <= 0.0f) {
        return;
    }
    const float radScale = _mapScale / std::max(_mapW, _mapH);
    for (const auto& strike : strikes) {
        if (strike.alpha <= 0.001f || strike.rad <= 0.0f) {
            continue;
        }
        const Vec2 c = project(strike.pos);
        const float r = strike.rad * radScale;
        // plane.ts AirstrikeZone: 1.5px yellow outline + 20% fill, fading in
        // over 0.5s and out over the last 0.5s.
        _strikeGfx->beginFill(0xeaff00, 0.2f * strike.alpha);
        _strikeGfx->drawCircle(c.x, c.y, r);
        _strikeGfx->endFill();
        _strikeGfx->lineStyle(1.5f, 0xeaff00, strike.alpha);
        _strikeGfx->drawCircle(c.x, c.y, r);
        _strikeGfx->lineStyle(0.0f, 0, 0.0f);
    }
}

void MapView::setGas(const MapGas& gas) {    if (!_gasGfx) {
        return;
    }
    _gasGfx->clear();
    if (!gas.active || _mapW <= 0.0f || _mapH <= 0.0f) {
        return;
    }
    const float mapSize = std::max(_mapW, _mapH);
    const float radScale = _mapScale / mapSize;
    const Vec2 c = project(gas.pos);
    const float r = gas.rad * radScale;

    // ui.ts GasRenderer: red overlay covering the map with a hole at the gas
    // circle (everything outside the safe circle is red).
    _gasGfx->beginFill(0xff0000, 0.28f);
    _gasGfx->drawRect(0.0f, 0.0f, _size, _size);
    if (r > 0.5f) {
        _gasGfx->beginHole();
        _gasGfx->drawCircle(c.x, c.y, r);
        _gasGfx->endHole();
    }
    _gasGfx->endFill();

    if (gas.hasSafe && gas.safeRad > 0.0f) {
        const Vec2 sc = project(gas.safePos);
        const float sr = gas.safeRad * radScale;
        // GasSafeZoneRenderer: the next circle plus a line to it when the big
        // map is closed.
        _gasGfx->lineStyle(1.5f, 0xffffff, 0.8f);
        _gasGfx->drawCircle(sc.x, sc.y, sr);
        const Vec2 pc = project(gas.playerPos);
        _gasGfx->lineStyle(1.0f, 0xffffff, 0.5f);
        _gasGfx->moveTo(pc.x, pc.y);
        _gasGfx->lineTo(sc.x, sc.y);
        _gasGfx->lineStyle(0.0f, 0, 0.0f);
    }
}

} // namespace surv
