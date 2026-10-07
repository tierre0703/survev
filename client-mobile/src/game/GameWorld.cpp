#include "GameWorld.h"

#include "Gas.h"
#include "Map.h"
#include "objects/Barns.h"
#include "objects/GameObject.h"
#include "render/Renderer.h"
#include "../audio/Ambiance.h"
#include "../audio/AudioManager.h"

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
    _map->setDecalBarn(_decalBarn.get());
}

void GameWorld::attachTo(pix::Node* root) {
    root->addChild(_map->groundGfx);
    root->addChild(_renderer->layers[0]);
    root->addChild(_renderer->ground);
    root->addChild(_renderer->layers[1]);
    root->addChild(_renderer->layers[2]);
    root->addChild(_renderer->layers[3]);
    // The gas overlay draws above the world (gas.ts m_render runs after the
    // map/objects, before the UI).
    if (_gas.gasRenderer.display) {
        root->addChild(_gas.gasRenderer.display);
    }
}

void GameWorld::setScreenSize(float width, float height) {
    _camera.m_screenWidth = width;
    _camera.m_screenHeight = height;
    if (mapLoaded()) {
        _renderer->resize(*_map, _camera);
    }
}

bool GameWorld::mapLoaded() const {
    return _map && _map->mapLoaded;
}

void GameWorld::loadMap(const MapMsg& msg) {
    _map->loadMap(msg, _camera);
    _particleBarn->valueAdjust = _map->mapDef.valueAdjust;
    _renderer->resize(*_map, _camera);
    _gas.m_init(_factory);

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
        audio::AmbienceMap amb;
        amb.music = _map->mapDef.ambienceMusic;
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
        _playerBarn->names[info.playerId] = info.name;
        _playerBarn->teams[info.playerId] = info.teamId;
    }

    for (const auto& bullet : msg.bullets) {
        _bulletBarn->spawn(_factory, bullet);
        if (bullet.shotFx) {
            auto* p = _playerBarn->getPlayerById(bullet.playerId);
            const auto* defs = getDefProvider();
            const auto* weapon = defs ? defs->gameObject(bullet.shotSourceType) : nullptr;
            if (p && weapon && weapon->category == "gun") {
                if (!weapon->isDual || bullet.shotOffhand) p->gunRecoil[0] += weapon->recoil;
                if (!weapon->isDual || !bullet.shotOffhand) p->gunRecoil[1] += weapon->recoil;
            }
        }
    }

    for (const auto& ex : msg.explosions) {
        auto* e = _explosionBarn->explosionPool.m_alloc();
        e->pos = ex.pos;
        e->type = ex.type;
        e->layer = ex.layer;
        e->t = 0.0f;

        // M6: explosion SFX (explosion.ts picks a per-def sound; a generic
        // explosion works until the explosion defs are ported).
        if (_audio) {
            audio::PlaySoundOptions opts;
            opts.channel = "sfx";
            opts.hasSoundPos = true;
            opts.soundPos = ex.pos;
            opts.hasLayer = true;
            opts.layer = ex.layer;
            _audio->playSound("explosion_01", opts);
        }
    }

    if (msg.gasDirty) {
        _gas.setFullState(msg.gasT, msg.gasData);
    }
    if (msg.gasTDirty) {
        _gas.setProgress(msg.gasT);
    }

    _activePlayer = _playerBarn->getPlayerById(_activePlayerId);
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
    _particleBarn->update(dt, *this);
    _decalBarn->update(dt, *this);

    _renderer->m_update(dt, _camera, *_map, false);
    _map->m_render(_camera);
    _gas.m_render(_factory, dt, _camera);

    // M6: drive the audio manager + ambience (game.ts update order).
    if (_activePlayer) {
        const bool underground = _map->isUnderground(_activePlayer->pos, _activePlayer->layer);
        _renderer->setUnderground(underground);
        if (_audio) {
            _audio->activeLayer = _activePlayer->layer;
            _audio->underground = underground;
        }
    }
    if (_audio) {
        _audio->cameraPos = _camera.m_pos;
        _audio->update(dt);
    }
    if (_ambiance && _audio) {
        _ambiance->update(dt, *_audio, true);
    }
}

const std::string& GameWorld::getPlayerName(uint16_t playerId) const {
    auto it = _playerNames.find(playerId);
    return it == _playerNames.end() ? _emptyName : it->second;
}

} // namespace surv
