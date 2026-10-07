#pragma once
// Port of client/src/map.ts: terrain generation from MapMsg, ground rendering,
// the minimap render, and the obstacle/building/structure pools. Object
// spawning comes from UpdateMsg (via GameWorld), not from loadMap.
#include "../net/Messages.h"
#include "../render/Defs.h"
#include "../render/PixiLike.h"
#include "../render/Terrain.h"
#include <string>
#include <vector>

namespace pix {
class Sprite;
class Graphics;
class Container;
class Text;
class Factory;
} // namespace pix

#include "objects/Barns.h"
#include "objects/Structure.h"

namespace surv {

class Camera;
class GameWorld;

class Map {
public:
    pix::Graphics* groundGfx = nullptr;

    std::string mapName;
    MapRenderDef mapDef;
    bool factionMode = false;
    bool potatoMode = false;
    bool perkMode = false;
    bool turkeyMode = false;
    uint32_t seed = 0;
    float width = 0.0f;
    float height = 0.0f;

    TerrainData terrain;
    std::vector<MapPlace> places;
    std::vector<MapObj> objects;
    std::vector<GroundPatch> groundPatches;

    bool mapLoaded = false;

    // map.ts bookkeeping: suppress one-shot destroy/drop effects for objects
    // that were already destroyed before this client joined.
    std::vector<uint16_t> deadObstacleIds;
    std::vector<uint16_t> deadCeilingIds;
    std::vector<uint16_t> solvedPuzzleIds;
    std::vector<uint16_t> lootDropSfxIds;

    // map.ts constructor injection (decalBarn): getGroundSurface checks decals
    // before buildings/rivers/terrain.
    DecalBarn* decalBarn = nullptr;
    void setDecalBarn(DecalBarn* barn) { decalBarn = barn; }

    // biome camera particle emitter (map.ts cameraEmitter), driven per frame.
    Emitter* cameraEmitter = nullptr;

    Pool<Obstacle> obstaclePool;
    Pool<Building> buildingPool;
    Pool<Structure> structurePool;

    explicit Map(pix::Factory* factory, bool canvasMode);

    void loadMap(const MapMsg& msg, Camera& camera);
    void renderTerrain(pix::Graphics* gfx, float gridThickness, bool canvasMode, bool mapRender);
    void m_render(const Camera& camera);
    void update(float dt, GameWorld& ctx);

    // Ground surface classes (player submerge/tint + game logic). Port of
    // map.ts getGroundSurface.
    enum class SurfaceType { Water, Sand, Grass };
    struct GroundSurface {
        SurfaceType type = SurfaceType::Water;
        uint32_t waterColor = 0;
        uint32_t rippleColor = 0;
    };

    // map.ts getGroundSurface(pos, layer).
    GroundSurface getGroundSurface(const Vec2& pos, int layer) const;
    // map.ts isInOcean / distanceToShore.
    bool isInOcean(const Vec2& pos) const;
    float distanceToShore(const Vec2& pos) const;

    Building* getBuildingById(uint16_t id);
    bool insideStructureStairs(const Collider& c) const;
    bool insideStructureMask(const Collider& c) const;
    bool insideBuildingCeiling(const Collider& c, bool checkVisible) const;
    // M6: true when a layer-1 point is inside an underground structure layer.
    bool isUnderground(const Vec2& pos, int layer) const;

private:
    bool _canvasMode = false;
    float _lastValueAdjust = 1.0f;
};

} // namespace surv
