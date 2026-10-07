#pragma once
// The in-game render context (the web client's Game `Ctx`). Owns the camera,
// renderer, map and object barns, applies UpdateMsg object deltas, and drives
// the per-frame object updates that position scene nodes.
#include "Gas.h"
#include "../net/Messages.h"
#include "../render/Camera.h"
#include "../render/PixiLike.h"
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

namespace surv {

class ObjectCreator;
class Renderer;
class Map;
class Player;
class LootBarn;
class DeadBodyBarn;
class ProjectileBarn;
class SmokeBarn;
class BulletBarn;
class ExplosionBarn;
class PlayerBarn;
class ParticleBarn;
class DecalBarn;
class Gas;

namespace audio {
class AudioManager;
class Ambiance;
} // namespace audio

class GameWorld {
public:
    GameWorld(pix::Factory* factory, bool canvasMode);
    ~GameWorld();

    // Attach the ground + z-sorted layers to a scene root, matching the
    // pixiContainers order in game.ts.
    void attachTo(pix::Node* root);

    void setScreenSize(float width, float height);
    void loadMap(const MapMsg& msg);
    void applyUpdate(const UpdateMsg& msg);
    void update(float dt);

    pix::Factory* factory() { return _factory; }
    Camera& camera() { return _camera; }
    Renderer& renderer() { return *_renderer; }
    Map& map() { return *_map; }
    LootBarn& lootBarn() { return *_lootBarn; }
    DeadBodyBarn& deadBodyBarn() { return *_deadBodyBarn; }
    ProjectileBarn& projectileBarn() { return *_projectileBarn; }
    SmokeBarn& smokeBarn() { return *_smokeBarn; }
    BulletBarn& bulletBarn() { return *_bulletBarn; }
    ExplosionBarn& explosionBarn() { return *_explosionBarn; }
    PlayerBarn& playerBarn() { return *_playerBarn; }
    ParticleBarn& particleBarn() { return *_particleBarn; }
    DecalBarn& decalBarn() { return *_decalBarn; }
    Gas& gas() { return _gas; }

    Player* activePlayer() { return _activePlayer; }
    void setActivePlayer(Player* p) { _activePlayer = p; }
    uint16_t activePlayerId() const { return _activePlayerId; }
    void setActivePlayerId(uint16_t id) { _activePlayerId = id; }

    // M6: audio is owned by the app (GameScene) and driven here.
    void setAudio(audio::AudioManager* audio, audio::Ambiance* ambiance) {
        _audio = audio;
        _ambiance = ambiance;
    }
    audio::AudioManager* audio() { return _audio; }

    void setPlayerName(uint16_t playerId, const std::string& name) {
        _playerNames[playerId] = name;
    }
    const std::string& getPlayerName(uint16_t playerId) const;

    bool mapLoaded() const;

    // Frame delta for the current update (obstacle.ts skin position interp).
    float dt() const { return _dt; }

private:
    void registerPools();

    pix::Factory* _factory = nullptr;
    Camera _camera;
    double _lastUpdateTime = 0.0;
    bool _lastUpdateTimeValid = false;
    std::unique_ptr<Renderer> _renderer;
    std::unique_ptr<Map> _map;
    std::unique_ptr<LootBarn> _lootBarn;
    std::unique_ptr<DeadBodyBarn> _deadBodyBarn;
    std::unique_ptr<ProjectileBarn> _projectileBarn;
    std::unique_ptr<SmokeBarn> _smokeBarn;
    std::unique_ptr<BulletBarn> _bulletBarn;
    std::unique_ptr<ExplosionBarn> _explosionBarn;
    std::unique_ptr<PlayerBarn> _playerBarn;
    std::unique_ptr<ParticleBarn> _particleBarn;
    std::unique_ptr<DecalBarn> _decalBarn;
    Gas _gas;
    std::unique_ptr<ObjectCreator> _creator;

    Player* _activePlayer = nullptr;
    uint16_t _activePlayerId = 0;
    std::unordered_map<uint16_t, std::string> _playerNames;
    std::string _emptyName;
    audio::AudioManager* _audio = nullptr;
    audio::Ambiance* _ambiance = nullptr;
    float _dt = 0.0f;
};

} // namespace surv
