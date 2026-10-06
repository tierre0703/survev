#pragma once
// Port of client/src/map.ts: terrain generation from MapMsg, ground rendering,
// the minimap render, and the obstacle/building/structure pools. Object
// spawning comes from UpdateMsg (via GameWorld), not from loadMap.
#include "../net/Messages.h"
#include "../render/Defs.h"
#include "../render/PixiLike.h"
#include "../render/Terrain.h"
#include "objects/Barns.h"
#include "objects/Structure.h"
#include <string>
#include <vector>

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

    Pool<Obstacle> obstaclePool;
    Pool<Building> buildingPool;
    Pool<Structure> structurePool;

    explicit Map(pix::Factory* factory, bool canvasMode);

    void loadMap(const MapMsg& msg, Camera& camera);
    void renderTerrain(pix::Graphics* gfx, float gridThickness, bool canvasMode, bool mapRender);
    void m_render(const Camera& camera);
    void update(float dt, GameWorld& ctx);

    Building* getBuildingById(uint16_t id);
    bool insideStructureStairs(const Collider& c) const;
    bool insideStructureMask(const Collider& c) const;
    bool insideBuildingCeiling(const Collider& c, bool checkVisible) const;

private:
    bool _canvasMode = false;
};

} // namespace surv
