#include "GameWorld.h"

#include "EmoteManager.h"
#include "Gas.h"
#include "Map.h"
#include "objects/Barns.h"
#include "objects/GameObject.h"
#include "objects/MapIndicator.h"
#include "render/Renderer.h"
#include "render/PixiLike.h"
#include "../audio/Ambiance.h"
#include "../audio/AudioManager.h"

#include <algorithm>
#include <chrono>

namespace surv {

namespace {
double updateClockSeconds() {
    using clock = std::chrono::steady_clock;
    return std::chrono::duration<double>(clock::now().time_since_epoch()).count();
}
} // namespace

// --- ObjectCreator ---------------------------------------------------------
AbstractObject* ObjectCreator::updateObjFull(uint8_t type, uint16_t id, const ObjectData& data,
                                             Ctx& ctx) {
    AbstractObject* obj = getObjById(id);
    bool isNew = false;
    if (!obj) {
        auto it = _alloc.find(type);
        if (it == _alloc.end()) {
            return nullptr;
        }
        obj = it->second();
        obj->__id = id;
        obj->__type = type;
        _idToObj[id] = obj;
        _order.push_back(obj);
        isNew = true;
    }
    obj->m_updateData(data, true, isNew, ctx);
    return obj;
}

void ObjectCreator::updateObjPart(uint16_t id, const ObjectData& data, Ctx& ctx) {
    AbstractObject* obj = getObjById(id);
    if (obj) {
        obj->m_updateData(data, false, false, ctx);
    }
}

void ObjectCreator::deleteObj(uint16_t id) {
    auto it = _idToObj.find(id);
    if (it == _idToObj.end()) {
        return;
    }
    AbstractObject* obj = it->second;
    obj->m_free();
    obj->active = false;
    _idToObj.erase(it);
}

// --- GameWorld -------------------------------------------------------------
GameWorld::GameWorld(pix::Factory* factory, bool canvasMode) : _factory(factory) {
    _renderer = std::make_unique<Renderer>(factory, canvasMode);
    _map = std::make_unique<Map>(factory, canvasMode);
    _lootBarn = std::make_unique<LootBarn>();
    _deadBodyBarn = std::make_unique<DeadBodyBarn>();
    _projectileBarn = std::make_unique<ProjectileBarn>();
    _smokeBarn = std::make_unique<SmokeBarn>();
    _bulletBarn = std::make_unique<BulletBarn>();
    _explosionBarn = std::make_unique<ExplosionBarn>();
    _playerBarn = std::make_unique<PlayerBarn>();
    _particleBarn = std::make_unique<ParticleBarn>();
    _decalBarn = std::make_unique<DecalBarn>();
    _airdropBarn = std::make_unique<AirdropBarn>();
    _flareBarn = std::make_unique<FlareBarn>();
    _planeBarn = std::make_unique<PlaneBarn>();
    _shotBarn = std::make_unique<ShotBarn>();
    _creator = std::make_unique<ObjectCreator>();
    registerPools();
}

GameWorld::~GameWorld() = default;

void GameWorld::registerPools() {
    _creator->registerPool<Player>(ObjectType_Player, &_playerBarn->playerPool);
    _creator->registerPool<Obstacle>(ObjectType_Obstacle, &_map->obstaclePool);
    _creator->registerPool<Loot>(ObjectType_Loot, &_lootBarn->lootPool);
    _creator->registerPool<DeadBody>(ObjectType_DeadBody, &_deadBodyBarn->deadBodyPool);
    _creator->registerPool<Building>(ObjectType_Building, &_map->buildingPool);
    _creator->registerPool<Structure>(ObjectType_Structure, &_map->structurePool);
    _creator->registerPool<Projectile>(ObjectType_Projectile, &_projectileBarn->projectilePool);
    _creator->registerPool<Smoke>(ObjectType_Smoke, &_smokeBarn->m_smokePool);
    _creator->registerPool<Decal>(ObjectType_Decal, &_decalBarn->decalPool);
    _creator->registerPool<Airdrop>(ObjectType_Airdrop, &_airdropBarn->airdropPool);
    _map->setDecalBarn(_decalBarn.get());
}

void GameWorld::attachTo(pix::Node* root) {
    _root = root;
    // Terrain, the object layers and the gas overlay all live in world units in
    // one content container; the camera transform is applied to that container
    // (see update()), not to each node's vertices.
    if (!_contentRoot) {
        _contentRoot = _factory->createContainer();
    }
    root->addChild(_contentRoot);
    // Terrain first (map.ts display.ground), then the z-sorted object layers.
    _contentRoot->addChild(_map->groundGfx);
    _contentRoot->addChild(_renderer->layers[0]);
    _contentRoot->addChild(_renderer->ground);
    _contentRoot->addChild(_renderer->layers[1]);
    _contentRoot->addChild(_renderer->layers[2]);
    _contentRoot->addChild(_renderer->layers[3]);
    // The aim line renders with the player layer (renderer.addPIXIObj re-parents
    // it each frame anyway; creating it once keeps the node owned).
    if (_lineSprites.container) {
        _contentRoot->addChild(_lineSprites.container);
    }
    // The gas overlay is world-space geometry (authored around the safe-zone
    // circle in map units), so it belongs under the camera-transformed content
    // container, above the object layers. It is normally created later, by
    // loadMap() -> Gas::m_init(); attach it there when that happens.
    if (_gas.gasRenderer.display) {
        _contentRoot->addChild(_gas.gasRenderer.display);
    }
}

void GameWorld::setScreenSize(float width, float height) {
    _camera.m_screenWidth = width;
    _camera.m_screenHeight = height;
    // Mirror view.ts setSize(): resize the live layers immediately, and notify
    // the app so size() callbacks (renderer groups, HUD) re-layout. This must
    // not depend on mapLoaded(): GameScene builds the world before the first
    // MapMsg arrives, so the layers still need the real viewport size.
    _renderer->resize(*_map, _camera);
    if (_onResize) {
        _onResize(width, height);
    }
}

void GameWorld::onFactoryReset() {
    _smokeBarn->onFactoryReset();
    _map->onFactoryReset();
    // view.ts clearResizeGame(): the callbacks belong to the old layers.
    if (_clearSizeCallbacks) {
        _clearSizeCallbacks();
    }
}

bool GameWorld::mapLoaded() const {
    return _map && _map->mapLoaded;
}

void GameWorld::loadMap(const MapMsg& msg) {
    // game.ts loads the map once per match. A reused GameWorld must therefore
    // start from a clean object table: map.ts `clear()` empties the barns and
    // resets the creator, otherwise ids from the previous match would resolve
    // to pooled objects that still belong to it.
    for (auto* obj : _creator->objects()) {
        if (obj) {
            obj->active = false;
        }
    }
    _creator->reset();
    _playerBarn->playerPool.clear();
    _lootBarn->lootPool.clear();
    _deadBodyBarn->deadBodyPool.clear();
    _projectileBarn->projectilePool.clear();
    _bulletBarn->clear();
    _smokeBarn->m_smokePool.clear();
    _particleBarn->clear();
    _decalBarn->decalPool.clear();
    _airdropBarn->airdropPool.clear();
    _explosionBarn->explosionPool.clear();
    _explosionBarn->physicsParticles.clear();
    for (auto* plane : _planeBarn->planes) {
        if (plane->soundInstance != audio::kInvalidSound && _audio) {
            _audio->stopSound(plane->soundInstance);
        }
        plane->soundInstance = audio::kInvalidSound;
        plane->active = false;
        if (plane->sprite) {
            plane->sprite->setVisible(false);
        }
    }
    _map->resetObjectPools();
    _gas.m_initFree();
    _lineSprites.m_free();

    // Emote/ping cleanup on new map
    _emoteManager.reset();
    _playerGroups.clear();

    // GameScene builds the world through resetWorld() on join and hands it the
    // session's cached MapMsg (replayMap), so this is the world's first load.
    _renderer->reset();
    _activePlayer = nullptr;
    _activePlayerId = 0;
    _map->loadMap(msg, _camera);
    _particleBarn->valueAdjust = _map->mapDef.valueAdjust;
    _renderer->resize(*_map, _camera);
    // Hook a render lambda into the Map for minimap texture baking.
    // The lambda captures `renderer()` (pix::Renderer), which is only
    // available when the axmol adapter is linked (app build, not host tests).
    // When unset, the minimap texture is created but not baked — the FGUI
    // minimap layer still works with its own team-dot overlays.
    _gas.m_init(_factory);
    // The overlay's geometry is rebuilt per map and lives in world space:
    // parent it under the camera-transformed content container, above the
    // object layers.
    if (_contentRoot && _gas.gasRenderer.display) {
        _contentRoot->addChild(_gas.gasRenderer.display);
    }

    // Init the minimap indicator barn (data only; sprite barn set by FguiHud).
    _indicatorBarn.init(nullptr);

    // Generate the minimap texture (one-shot, after terrain is loaded).
    _map->generateMinimapTexture(_factory, _camera);

    // map.ts: spawn the biome camera particle emitter (falling leaves/snow/...).
    if (_map->cameraEmitter) {
        _map->cameraEmitter->stop();
        _map->cameraEmitter = nullptr;
    }
    if (!_map->mapDef.cameraEmitter.empty()) {
        EmitterOptions opts;
        opts.pos = Vec2(0.0f, 0.0f);
        opts.dir = Vec2(0.70710678f, -0.70710678f);
        opts.layer = 99999;
        _map->cameraEmitter = _particleBarn->addEmitter(_factory, _map->mapDef.cameraEmitter, opts);
    }

    // M6: point the ambience mixer at this map's tracks.
    if (_ambiance && _audio) {
        audio::AmbienceMap amb;        amb.music = _map->mapDef.ambienceMusic;
        amb.wind = _map->mapDef.ambienceWind;
        amb.river = _map->mapDef.ambienceRiver;
        amb.waves = _map->mapDef.ambienceWaves;
        _ambiance->setMap(amb, *_audio);
    }
}

void GameWorld::applyUpdate(const UpdateMsg& msg) {
    if (msg.activePlayerIdDirty) {
        _activePlayerId = msg.activePlayerId;
    }

    // camera.m_interpInterval tracks the wall-clock spacing of update messages
    // (game.ts m_processGameUpdate); the object interpolation uses it.
    if (_lastUpdateTimeValid) {
        const float interval = static_cast<float>(updateClockSeconds() - _lastUpdateTime);
        if (interval > 0.0f) {
            _camera.m_interpInterval = interval;
        }
    }
    _lastUpdateTime = updateClockSeconds();
    _lastUpdateTimeValid = true;

    for (uint16_t id : msg.delObjIds) {
        _creator->deleteObj(id);
    }
    for (const auto& obj : msg.fullObjects) {
        _creator->updateObjFull(obj.data.__type, obj.data.__id, obj.data, *this);
    }
    for (const auto& obj : msg.partObjects) {
        _creator->updateObjPart(obj.data.__id, obj.data, *this);
    }

    for (const auto& info : msg.playerInfos) {
        _playerNames[info.playerId] = info.name;
        _playerGroups[info.playerId] = info.groupId;
        _playerBarn->names[info.playerId] = info.name;
        _playerBarn->teams[info.playerId] = info.teamId;
    }

    // Process emotes and pings from ServerUpdateMsg (port of game.ts line 1197-1204)
    for (const auto& emote : msg.emotes) {
        // Determine factionMode from map render def
        const auto* provider = getDefProvider();
        const auto* mapDef = provider ? provider->mapRender(_map->mapName) : nullptr;
        const bool factionMode = mapDef && mapDef->factionMode;
        if (emote.isPing) {
            _emoteManager.addPing(emote, factionMode, *this);
        } else {
            _emoteManager.addEmote(emote, *this);
        }
    }

    for (const auto& bullet : msg.bullets) {
        // flare.ts splits a flare-gun shot out of the normal bullet barn.
        const DefProvider* provider = getDefProvider();
        const BulletRenderDef* bulletDef =
            provider ? provider->bullet(bullet.bulletType) : nullptr;
        if (bulletDef && bulletDef->flareColor != 0 && bulletDef->tracerLength > 0.0f &&
            bullet.bulletType == "bullet_flare") {
            _flareBarn->addFlare(*this, bullet);
        }
        _bulletBarn->spawn(_factory, bullet);
        if (bullet.shotFx) {
            // shot.ts: per-shot weapon handling (recoil + fireDelay + sound).
            _shotBarn->addShot(bullet);
        }
    }

    for (const auto& ex : msg.explosions) {
        auto* e = _explosionBarn->explosionPool.m_alloc();
        e->pos = ex.pos;
        e->type = ex.type;
        e->layer = ex.layer;
        e->t = 0.0f;

        // M6: per-def explosion sound selection (port of explosion.ts).
        // Map explosion effect type → (grassSound, waterSound, detune, volume).
        struct ExplosionSoundDef {
            const char* grass;
            const char* water;
            float detune;
            float volume;
        };
        static const std::unordered_map<std::string, ExplosionSoundDef> soundMap = {
            {"frag",              {"explosion_01",      "explosion_02",      0.0f, 1.0f}},
            {"smoke",             {"explosion_smoke_01", "explosion_smoke_01",0.0f, 1.0f}},
            {"strobe",            {"explosion_04",      "explosion_02",      0.0f, 1.0f}},
            {"barrel",            {"explosion_01",      "explosion_02",      0.0f, 1.0f}},
            {"usas",              {"explosion_03",      "explosion_02",      0.0f, 1.0f}},
            {"rounds",            {"explosion_04",      "explosion_04",    500.0f, 0.5f}},
            {"rounds_sg",         {"explosion_04",      "explosion_04",    500.0f, 0.2f}},
            {"mirv",              {"explosion_01",      "explosion_02",      0.0f, 1.0f}},
            {"mirv_mini",         {"explosion_03",      "explosion_02",      0.0f, 1.0f}},
            {"martyr_nade",       {"explosion_03",      "explosion_02",      0.0f, 1.0f}},
            {"snowball",          {"snowball_01",       "frag_water_01",     0.0f, 1.0f}},
            {"snowball_heavy",    {"snowball_02",       "frag_water_01",     0.0f, 1.0f}},
            {"potato",            {"potato_01",         "frag_water_01",     0.0f, 1.0f}},
            {"potato_heavy",      {"potato_02",         "frag_water_01",     0.0f, 1.0f}},
            {"potato_cannonball", {"explosion_05",      "explosion_02",      0.0f, 1.0f}},
            {"potato_smgshot",    {"potato_01",         "potato_02",       250.0f, 0.5f}},
            {"potato_lmgshot",    {"potato_01",         "potato_02",       400.0f, 0.5f}},
            {"bomb_iron",         {"explosion_01",      "explosion_02",      0.0f, 1.0f}},
            {"coconut",           {"coconut_01",        "frag_water_01",     0.0f, 1.0f}},
            {"tomato",            {"tomato_01",         "frag_water_01",     0.0f, 1.0f}},
        };

        if (_audio && _map) {
            const auto it = soundMap.find(ex.type);
            const ExplosionSoundDef def = it != soundMap.end()
                ? it->second
                : ExplosionSoundDef{"explosion_01", "explosion_02", 0.0f, 1.0f};

            const Map::GroundSurface surface =
                _map->getGroundSurface(ex.pos, ex.layer);
            const std::string& soundName = surface.type == Map::SurfaceType::Water
                ? def.water : def.grass;

            audio::PlaySoundOptions opts;
            opts.channel = "sfx";
            opts.hasSoundPos = true;
            opts.soundPos = ex.pos;
            opts.hasLayer = true;
            opts.layer = ex.layer;
            opts.filter = "muffled";
            opts.rangeMult = 2.0f;
            opts.ignoreMinAllowable = true;
            opts.detune = def.detune;
            opts.volumeScale = def.volume;
            _audio->playSound(soundName, opts);
        }
    }

    if (msg.gasDirty) {
        _gas.setFullState(msg.gasT, msg.gasData);
    }
    if (msg.gasTDirty) {
        _gas.setProgress(msg.gasT);
    }

    // plane.ts: reconcile the plane list from each update.
    _planeBarn->updatePlanes(*this, msg.planes);
    for (const auto& zone : msg.airstrikeZones) {
        _planeBarn->createAirstrikeZone(zone);
    }

    _activePlayer = _playerBarn->getPlayerById(_activePlayerId);
    // game.ts `m_activePlayer.m_setLocalData(msg.activePlayerData)`: the zoom
    // step lives on the active-player payload, not on the object delta, so
    // without this the camera never learns the player's view radius and zooms
    // to the 1.0 fallback (dozens of times too far in).
    if (_activePlayer) {
        if (msg.activePlayerData.zoomDirty) {
            _activePlayer->zoom = static_cast<float>(msg.activePlayerData.zoom);
            _activePlayer->zoomFast = false;
        }
    }

    // Process map indicators from the update message.
    for (const auto& mi : msg.mapIndicators) {
        _indicatorBarn.addOrUpdate(mi.id, mi.dead, mi.equipped, mi.type, mi.pos, _factory);
    }
}

void GameWorld::updatePlanes(const std::vector<PlaneData>& planes) {
    _planeBarn->updatePlanes(*this, planes);
}

// game.ts update() zoom block: the scoped view radius drives the camera zoom.
void GameWorld::updateCameraZoom(float dt) {
    if (!_activePlayer) {
        return;
    }
    const float minDim = math::min(_camera.m_screenWidth, _camera.m_screenHeight);
    const float maxDim = math::max(_camera.m_screenWidth, _camera.m_screenHeight);
    const float maxScreenDim = math::max(minDim * (16.0f / 9.0f), maxDim);
    const float zoom = math::max(_activePlayer->getZoom(true), 1.0f);
    _camera.m_targetZoom = (maxScreenDim * 0.5f) / (zoom * _camera.m_ppu);
    const float zoomLerpIn = _activePlayer->zoomFast ? 3.0f : 2.0f;
    const float zoomLerpOut = _activePlayer->zoomFast ? 3.0f : 1.4f;
    const float zoomLerp = _camera.m_targetZoom > _camera.m_zoom ? zoomLerpIn : zoomLerpOut;
    _camera.m_zoom = math::lerp(dt * zoomLerp, _camera.m_zoom, _camera.m_targetZoom);
}

void GameWorld::update(float dt) {
    if (!mapLoaded()) {
        return;
    }
    _dt = dt;
    _map->update(dt, *this);
    _playerBarn->update(dt, *this);
    _lootBarn->update(dt, *this);
    _deadBodyBarn->update(dt, *this);
    _projectileBarn->update(dt, *this);
    _smokeBarn->update(dt, *this);
    _bulletBarn->update(dt, *this);
    _explosionBarn->update(dt, *this);
    _planeBarn->update(dt, *this);
    _shotBarn->update(dt, *this);
    _flareBarn->update(dt, *this);
    _airdropBarn->update(dt, *this);
    _particleBarn->update(dt, *this);
    _decalBarn->update(dt, *this);

    // Map indicator pulse animations.
    if (_map) {
        _indicatorBarn.updatePulses(dt);
    }

    // Emote/ping manager update (positioned after all other objects)
    _emoteManager.update(dt, *this);

    // game.ts m_processGameUpdate(): the active player's layer and underground
    // state are applied before renderer.m_update(), so the bunker layer fade,
    // the ground fade and the tree/building mask all track the same frame.
    if (_activePlayer) {
        _renderer->setActiveLayer(_activePlayer->layer);
        const bool underground = _map->isUnderground(_activePlayer->pos, _activePlayer->layer, _activePlayer->rad);
        _renderer->setUnderground(underground);
        if (_audio) {
            _audio->activeLayer = _activePlayer->layer;
            _audio->underground = underground;
        }
    }

    _renderer->m_update(dt, _camera, *_map, false);
    // game.ts restoreCamera(): the camera follows the interpolated visual
    // position, not the raw net position (player.ts m_visualPos), so following
    // a 33 Hz player is smooth.
    if (_activePlayer) {
        _camera.m_pos = _activePlayer->visualPos;
    } else {
        const float w = _camera.m_screenWidth;
        const float h = _camera.m_screenHeight;
        if (w > 0.0f && h > 0.0f) {
            _camera.m_pos = _camera.m_screenToPoint(Vec2(w * 0.5f, h * 0.5f));
        }
    }
    updateCameraZoom(dt);
    // game.ts: apply the accumulated explosion/shake offset after restoring the
    // camera position; the `screenShake` setting gates it in Camera.
    _camera.m_applyShake();
    if (_root) {
        // renderer.ts: the camera live transform goes on the containers, right
        // after the zoom update so this frame already uses it. Only the world
        // content container carries it (see attachTo); the root itself stays
        // untransformed so the gas overlay can mimic the camera transform.
        _renderer->applyCamera(_contentRoot, _camera);
        // The ground rect and the layer mask are sized to the camera zoom /
        // viewport, so they are rebuilt whenever the transform changes.
        _renderer->resize(*_map, _camera);
    }
    _map->m_render(_camera);
    _gas.m_render(_factory, dt, _camera);

    // touch.ts LineSprites: driven each frame while the aim pad is touched.
    if (_activePlayer) {
        _lineSprites.update(_aimLineVisible && _touchAiming && _touchAimLine, *_activePlayer, *_map,
                            _camera, *_renderer, _factory, _mobile);
    }

    // M6: audio is ticked by GameScene (it must keep running in the Java menu,
    // where there is no world); this only publishes the active layer/underground
    // state above, before renderer.m_update.
}

const std::string& GameWorld::getPlayerName(uint16_t playerId) const {
    auto it = _playerNames.find(playerId);
    return it == _playerNames.end() ? _emptyName : it->second;
}

uint8_t GameWorld::playerGroupId(uint16_t playerId) const {
    auto it = _playerGroups.find(playerId);
    return it == _playerGroups.end() ? 0 : it->second;
}

bool GameWorld::hasPlayerGroup(uint16_t playerId) const {
    return _playerGroups.find(playerId) != _playerGroups.end();
}

uint8_t GameWorld::activeGroupId() const {
    return playerGroupId(_activePlayerId);
}

std::vector<uint16_t> GameWorld::groupMembers(uint8_t groupId) const {
    std::vector<uint16_t> ids;
    for (const auto& entry : _playerGroups) {
        if (entry.second == groupId) {
            ids.push_back(entry.first);
        }
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

} // namespace surv
