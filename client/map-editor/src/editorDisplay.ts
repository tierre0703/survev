import * as PIXI from "pixi.js-legacy";

import { MapDefs } from "../../../shared/defs/mapDefs.ts";
import { MapObjectDefs } from "../../../shared/defs/register.ts";
import { MapMsg } from "../../../shared/net/mapMsg.ts";
import { type ObjectData, ObjectType } from "../../../shared/net/objectSerializeFns.ts";
import type { LocalDataWithDirty } from "./../../../shared/net/updateMsg.ts";
import { collider } from "../../../shared/utils/collider.ts";
import { math } from "../../../shared/utils/math.ts";
import { util } from "../../../shared/utils/util.ts";
import { v2, type Vec2 } from "../../../shared/utils/v2.ts";
import type { Ambiance } from "../../src/ambiance.ts";
import type { AudioManager } from "../../src/audioManager.ts";
import Camera from "../../src/camera.ts";
import type { ConfigManager, DebugRendererOpts } from "../../src/config.ts";
import { debugLines } from "../../src/debug/debugLines.ts";
import { device } from "../../src/device.ts";
import type { Game } from "../../src/game.ts";
import type { InputBinds } from "../../src/inputBinds.ts";
import { Map } from "../../src/map.ts";
import type { Building } from "../../src/objects/building.ts";
import { DecalBarn } from "../../src/objects/decal.ts";
import { LootBarn } from "../../src/objects/loot.ts";
import { Creator } from "../../src/objects/objectPool.ts";
import { ParticleBarn } from "../../src/objects/particles.ts";
import { type Player, PlayerBarn } from "../../src/objects/player.ts";
import { SmokeBarn } from "../../src/objects/smoke.ts";
import type { Structure } from "../../src/objects/structure.ts";
import { Renderer } from "../../src/renderer.ts";
import type { ResourceManager } from "../../src/resources.ts";
import type { UiManager2 } from "../../src/ui/ui2.ts";
import { type MapDoc, type MapObjectEntry } from "./mapDocument.ts";

/**
 * Render plumbing + live pool access for the map editor.
 *
 * This is a fork of `building-editor/src/editorDisplay.ts`: the parts that
 * build a scratch `MapMsg` for a single preview object are replaced with
 * `loadDocument`/`renderDocument`, which replay a `MapDoc` into the pools.
 *
 * IMPORTANT: pool objects are compacted by `Pool.m_free` when the pool grows
 * past 128 entries, so never hold raw object references across a rebuild. The
 * document is keyed by index into `MapDoc.objects`; re-resolve through
 * `m_docIndexToObjId` after every rebuild.
 */
export class EditorDisplay {
    active = false;
    initialized = false;

    canvasMode!: boolean;
    camera!: Camera;
    renderer!: Renderer;
    particleBarn!: ParticleBarn;
    decalBarn!: DecalBarn;
    map!: Map;
    playerBarn!: PlayerBarn;
    smokeBarn!: SmokeBarn;
    lootBarn!: LootBarn;
    objectCreator!: Creator;
    debugDisplay!: PIXI.Graphics;

    /** Id of the (invisible) editor player. Camera is pinned to it. */
    activeId = 98;
    activePlayer!: Player;

    nextId = 100;

    /** The loaded document. Owned by `EditorContext`; treat as read-only here. */
    doc!: MapDoc;

    mapMsg = new MapMsg();

    /** Document entry index -> live object id, rebuilt on every loadDocument. */
    m_docIndexToObjId: number[] = [];
    /** Live object id of a top level entry -> document entry index. */
    m_objIdToDocIndex = new Map<number, number>();

    getNextId() {
        return this.nextId++;
    }

    constructor(
        public pixi: PIXI.Application,
        public audioManager: AudioManager,
        public config: ConfigManager,
        public inputBinds: InputBinds,
        public ambience: Ambiance,
        public resourceManager: ResourceManager,
    ) {}

    init() {
        this.canvasMode = this.pixi.renderer.type == PIXI.RENDERER_TYPE.CANVAS;
        this.camera = new Camera();
        // There is no network updates here, so the server-derived interp
        // interval stays 0. Player positions are lerped against it
        // (player.ts#m_update) and a 0 divisor yields NaN, which makes every
        // sprite vanish the first time the editor player moves.
        this.camera.m_interpInterval = 1;
        this.renderer = new Renderer(this as unknown as Game, this.canvasMode);
        this.particleBarn = new ParticleBarn(this.renderer);
        this.decalBarn = new DecalBarn();
        this.map = new Map(this.decalBarn);
        this.playerBarn = new PlayerBarn();
        this.smokeBarn = new SmokeBarn();
        this.lootBarn = new LootBarn();

        // Register types
        const TypeToPool = {
            [ObjectType.Player]: this.playerBarn.playerPool,
            [ObjectType.Obstacle]: this.map.m_obstaclePool,
            [ObjectType.Loot]: this.lootBarn.lootPool,
            [ObjectType.Building]: this.map.m_buildingPool,
            [ObjectType.Structure]: this.map.m_structurePool,
            [ObjectType.Decal]: this.decalBarn.decalPool,
            [ObjectType.Smoke]: this.smokeBarn.m_smokePool,
        };

        this.objectCreator = new Creator();
        for (const type in TypeToPool) {
            if (TypeToPool.hasOwnProperty(type)) {
                this.objectCreator.m_registerType(
                    type,
                    TypeToPool[type as unknown as keyof typeof TypeToPool],
                );
            }
        }

        // Render ordering
        this.debugDisplay = new PIXI.Graphics();

        const pixiContainers = [
            this.map.display.ground,
            this.renderer.layers[0],
            this.renderer.ground,
            this.renderer.layers[1],
            this.renderer.layers[2],
            this.renderer.layers[3],
            this.debugDisplay,
        ];
        for (let i = 0; i < pixiContainers.length; i++) {
            const container = pixiContainers[i];
            if (container) {
                container.interactiveChildren = false;
                this.pixi.stage.addChild(container);
            }
        }

        this.updatePlayer();

        this.activePlayer = this.playerBarn.getPlayerById(this.activeId)!;
        this.activePlayer.m_setLocalData(
            {
                boost: 100,
                boostDirty: true,
                health: 100,
                inventoryDirty: false,
                spectatorCountDirty: false,
                weapsDirty: true,
                curWeapIdx: 2,
                weapons: [
                    {
                        type: "",
                        ammo: 0,
                    },
                    {
                        type: "",
                        ammo: 0,
                    },
                    {
                        type: "fists",
                        ammo: 0,
                    },
                    {
                        type: "",
                        ammo: 0,
                    },
                ],
            } satisfies Partial<LocalDataWithDirty> as LocalDataWithDirty,
        );

        this.activePlayer.layer = this.activePlayer.m_netData.m_layer;
        this.renderer.setActiveLayer(this.activePlayer.layer);
        this.audioManager.activeLayer = this.activePlayer.layer;

        this.initialized = true;

        this.resize();
    }

    free() {
        if (this.initialized) {
            this.map.m_free();
            this.particleBarn.m_free();
            this.renderer.m_free();
            while (this.pixi.stage.children.length > 0) {
                const e = this.pixi.stage.children[0];
                this.pixi.stage.removeChild(e);
                e.destroy({
                    children: true,
                });
            }
        }
        this.initialized = false;
    }

    getCtx() {
        return {
            audioManager: this.audioManager,
            renderer: this.renderer,
            particleBarn: this.particleBarn,
            map: this.map,
            smokeBarn: this.smokeBarn,
            decalBarn: this.decalBarn,
        };
    }

    toWorldPos(pos: Vec2) {
        const center = v2.create(this.map.width / 2, this.map.height / 2);

        return v2.add(center, pos);
    }

    toBuildingPos(pos: Vec2) {
        const center = v2.create(this.map.width / 2, this.map.height / 2);
        return v2.sub(pos, center);
    }

    /** Screen pixel -> world position */
    screenToWorld(screen: Vec2) {
        return this.camera.m_screenToPoint(screen);
    }

    updatePlayer() {
        const config = this.config.get("mapEditor")!;
        const pos = this.toWorldPos(config.pos);

        const obj: ObjectData<ObjectType.Player> = {
            outfit: "outfitDev",
            backpack: "backpack02",
            helmet: "helmet01",
            chest: "chest03",
            activeWeapon: "fists",
            layer: config.layer,
            // not "dead" — a dead editor player renders as a corpse
            dead: false,
            downed: false,
            animType: 0,
            animSeq: 0,
            actionSeq: 0,
            actionType: 0,
            actionItem: "",
            wearingPan: false,
            healEffect: false,
            lastStandEffect: false,
            frozen: false,
            frozenOri: 0,
            frozenType: "",
            hasteType: 0,
            hasteSeq: 0,
            scale: 1,
            role: "",
            perks: [],
            pos: pos,
            dir: v2.create(0, -1),
        };

        this.activePlayer = this.objectCreator.m_updateObjFull(
            ObjectType.Player,
            this.activeId,
            obj,
            this.getCtx(),
        ) as Player;

        this.playerBarn.setPlayerInfo({
            playerId: 98,
            teamId: 1,
            groupId: 0,
            name: "",
            loadout: {
                heal: "heal_basic",
                boost: "boost_basic",
            },
        });

        this.activePlayer.layer = config.layer;
        this.renderer.setActiveLayer(config.layer);
        const underground = this.activePlayer.isUnderground(this.map);
        this.renderer.setUnderground(underground);
    }

    setPlayerPos(pos: Vec2) {
        this.objectCreator.m_updateObjPart(
            this.activeId,
            {
                pos: pos,
                dir: v2.create(0, -1),
            },
            this.getCtx(),
        );
        const underground = this.activePlayer.isUnderground(this.map);
        this.renderer.setUnderground(underground);
    }

    setLayer(layer: number) {
        this.activePlayer.layer = layer;
        this.renderer.setActiveLayer(layer);
        this.audioManager.activeLayer = layer;
        this.objectCreator.m_updateObjPart(
            this.activeId,
            {
                layer,
            },
            this.getCtx(),
        );
        const underground = this.activePlayer.isUnderground(this.map);
        this.renderer.setUnderground(underground);
    }

    //
    // Object spawning
    //

    addStructure(type: string, pos: Vec2, ori: number) {
        const def = MapObjectDefs.typeToDef(type, "structure");

        const data: ObjectData<ObjectType.Structure> = {
            type,
            pos,
            ori,
            layerObjIds: [],
            interiorSoundAlt: false,
            interiorSoundEnabled: true,
        };
        const obj = this.objectCreator.m_updateObjFull(
            ObjectType.Structure,
            this.getNextId(),
            data,
            this.getCtx(),
        );

        for (let i = 0; i < def.layers.length; i++) {
            const layerDef = def.layers[i];
            this.genBuilding(
                layerDef.type,
                math.addAdjust(pos, layerDef.pos, ori),
                (layerDef.ori + ori) % 4,
                i,
            );
        }

        return obj;
    }

    addBuilding(type: string, pos: Vec2, ori: number, layer: number) {
        const data: ObjectData<ObjectType.Building> = {
            type,
            pos,
            ori,
            layer,
            occupied: false,
            ceilingDamaged: false,
            ceilingDead: false,
            hasPuzzle: false,
        };
        const obj = this.objectCreator.m_updateObjFull(
            ObjectType.Building,
            this.getNextId(),
            data,
            this.getCtx(),
        );
        const def = MapObjectDefs.typeToDef(type, "building");

        for (const child of def.mapObjects) {
            let partType = child.type;

            if (typeof partType === "object") {
                partType = util.weightedRandomObject(partType);
            }
            if (!partType) continue;

            let partOri: number;
            if (child.inheritOri === false) partOri = child.ori;
            else partOri = (child.ori + ori) % 4;

            const partPos = math.addAdjust(pos, child.pos, ori);

            this.addAuto(
                partType,
                partPos,
                child.layer ?? layer,
                partOri,
                child.scale,
                obj.__id,
                !!child.puzzlePiece,
            );
        }

        if (def.mapGroundPatches) {
            for (const patch of def.mapGroundPatches) {
                if (patch.bound.type === collider.Type.Circle) {
                    const worldCenter = math.addAdjust(pos, patch.bound.pos, ori);
                    this.mapMsg.groundPatches.push({
                        bound: collider.createCircle(worldCenter, patch.bound.rad),
                        color: patch.color,
                        roughness: patch.roughness ?? 0,
                        offsetDist: patch.offsetDist ?? 0,
                        order: patch.order ?? 0,
                        useAsMapShape: patch.useAsMapShape ?? true,
                    });
                } else {
                    this.mapMsg.groundPatches.push({
                        bound: collider.createAabb(
                            math.addAdjust(pos, patch.bound.min, ori),
                            math.addAdjust(pos, patch.bound.max, ori),
                        ),
                        color: patch.color,
                        roughness: patch.roughness ?? 0,
                        offsetDist: patch.offsetDist ?? 0,
                        order: patch.order ?? 0,
                        useAsMapShape: patch.useAsMapShape ?? true,
                    });
                }
            }
        }

        return obj;
    }

    addObstacle(
        type: string,
        pos: Vec2,
        ori: number,
        scale: number,
        layer: number,
        parentId?: number,
        puzzlePiece?: boolean,
    ) {
        MapObjectDefs.typeToDef(type, "obstacle");

        const data: ObjectData<ObjectType.Obstacle> = {
            type,
            pos,
            ori,
            scale,
            layer,
            healthT: 1,
            dead: false,
            isDoor: false,
            door: {
                open: false,
                canUse: false,
                locked: false,
                seq: 0,
            },
            isButton: false,
            button: {
                onOff: false,
                canUse: false,
                seq: 0,
            },
            isPuzzlePiece: !!puzzlePiece,
            parentBuildingId: parentId,
            isSkin: false,
            skinPlayerId: 0,
        };
        const obj = this.objectCreator.m_updateObjFull(
            ObjectType.Obstacle,
            this.getNextId(),
            data,
            this.getCtx(),
        );
        return obj;
    }

    addDecal(type: string, pos: Vec2, ori: number, scale: number, layer: number) {
        MapObjectDefs.typeToDef(type, "decal");

        const data: ObjectData<ObjectType.Decal> = {
            type,
            pos,
            ori,
            scale,
            layer,
            goreKills: 0,
        };
        const obj = this.objectCreator.m_updateObjFull(
            ObjectType.Decal,
            this.getNextId(),
            data,
            this.getCtx(),
        );
        return obj;
    }

    addLootspawner(type: string, pos: Vec2, layer: number) {
        const def = MapObjectDefs.typeToDef(type, "loot_spawner");

        const addLoot = (type: string, pos: Vec2, preloaded?: boolean) => {
            const data: ObjectData<ObjectType.Loot> = {
                type,
                pos,
                layer,
                count: 0,
                isOld: true,
                isPreloadedGun: preloaded ?? false,
                hasOwner: false,
                ownerId: 0,
            };
            return this.objectCreator.m_updateObjFull(
                ObjectType.Loot,
                this.getNextId(),
                data,
                this.getCtx(),
            );
        };

        const lootTables = this.map.mapDef.lootTable;
        const getLootTable = (type: string) => {
            const item = util.weightedRandom(lootTables[type]);
            if (item.name.startsWith("tier_")) {
                return getLootTable(item.name);
            }
            return item;
        };

        for (const lootSpawner of def.loot) {
            const item = getLootTable(lootSpawner.tier!);
            if (item.name) {
                addLoot(item.name, pos, !!item.preload);
            }
        }
    }

    addAuto(
        type: string,
        pos: Vec2,
        layer: number,
        ori: number,
        scale: number,
        parentId?: number,
        puzzlePiece?: boolean,
        ignoreMapSpawnReplacement?: boolean,
    ) {
        let def = MapObjectDefs.typeToDef(type);

        const spawnReplacements = this.map.getMapDef().mapGen.spawnReplacements[0];
        if (spawnReplacements[type] && !ignoreMapSpawnReplacement) {
            type = spawnReplacements[type];
            def = MapObjectDefs.typeToDef(type);
        }

        switch (def.type) {
            case "obstacle":
                return this.addObstacle(
                    type,
                    pos,
                    ori,
                    scale,
                    layer,
                    parentId,
                    puzzlePiece,
                );
            case "building":
                return this.addBuilding(type, pos, ori, layer);
            case "structure":
                return this.addStructure(type, pos, ori);
            case "decal": {
                return this.addDecal(type, pos, ori, scale, layer);
            }
            case "loot_spawner":
                return this.addLootspawner(type, pos, layer);
        }
    }

    /**
     * Adds a document entry and returns the live object id of the *top level*
     * object (the building/obstacle/etc the entry describes).
     */
    addEntry(entry: MapObjectEntry) {
        const obj = this.addAuto(
            entry.type,
            v2.copy(entry.pos),
            entry.layer,
            entry.ori,
            entry.scale,
            undefined,
            undefined,
            // never substitute types: the document is the source of truth
            true,
        );
        return obj ? obj.__id : undefined;
    }

    clearAllObjs() {
        const pools = [
            this.map.m_structurePool.m_getPool(),
            this.map.m_buildingPool.m_getPool(),
            this.map.m_obstaclePool.m_getPool(),
            this.decalBarn.decalPool.m_getPool(),
            this.lootBarn.lootPool.m_getPool(),
        ];

        for (const pool of pools) {
            for (const obj of pool) {
                if (obj.active) {
                    this.objectCreator.m_deleteObj(obj.__id);
                }
            }
        }
    }

    /**
     * Clears the world and rebuilds it from `doc`.
     *
     * Mirrors `building-editor/src/editorDisplay.ts#resetObj` but takes the
     * whole document instead of a single preview object.
     */
    loadDocument(doc: MapDoc) {
        this.doc = doc;

        const mapName = doc.mapName in MapDefs ? doc.mapName : "main";
        this.resourceManager.loadMapAssets(mapName);

        // Drop the decal renders *before* clearing the pools.
        //
        // Pool objects are reused by allocation order, not by id, so the replay
        // below reuses the very decal objects `clearAllObjs` just freed, with
        // their stale `decalRender` still attached. `allocDecalRender` then hands
        // out those same renders, and every decal ends up driving someone else's
        // sprite (or a detached one, which paints at its last position).
        const oldDecals = this.decalBarn.decalPool.m_getPool();
        for (let i = 0; i < oldDecals.length; i++) {
            oldDecals[i].decalRender = null;
        }
        this.decalBarn.decalRenders.length = 0;

        // Reset the per-map state; otherwise destroyed obstacle/building ids
        // leak across rebuilds and freshly spawned objects render as exploded.
        this.map.deadObstacleIds.length = 0;
        this.map.deadCeilingIds.length = 0;
        this.map.solvedPuzzleIds.length = 0;

        this.mapMsg = new MapMsg();
        this.mapMsg.grassInset = doc.grassInset;
        this.mapMsg.groundPatches = [];
        this.mapMsg.width = doc.width;
        this.mapMsg.height = doc.height;
        this.mapMsg.mapName = mapName;
        this.mapMsg.objects = [];
        this.mapMsg.places = doc.places.map((place) => {
            return {
                name: place.name,
                pos: v2.copy(place.pos),
            };
        });
        this.mapMsg.rivers = doc.rivers.map((river) => {
            return {
                width: river.width,
                looped: river.looped,
                points: river.points.map((p) => v2.copy(p)),
            };
        });
        this.mapMsg.seed = doc.seed;
        this.mapMsg.shoreInset = doc.shoreInset;

        this.map.loadMap(this.mapMsg, this.camera, this.canvasMode, this.particleBarn);

        // loadMap resets mapData and dead ids; drop them again after the load
        this.map.deadObstacleIds.length = 0;
        this.map.deadCeilingIds.length = 0;
        this.map.solvedPuzzleIds.length = 0;

        this.clearAllObjs();

        this.m_docIndexToObjId = [];
        this.m_objIdToDocIndex = new Map<number, number>();
        for (let i = 0; i < doc.objects.length; i++) {
            const objId = this.addEntry(doc.objects[i]) ?? -1;
            this.m_docIndexToObjId.push(objId);
            if (objId >= 0) {
                this.m_objIdToDocIndex.set(objId, i);
            }
        }

        // The camera/player position is relative to the map centre, keep it in
        // sync so changing size does not strand the camera off map.
        this.updatePlayer();

        this.renderDocument();
    }

    /** Re-renders terrain + ground patches. Call after objects are created. */
    renderDocument() {
        // re render the map so ground patches are rendered after the buildings are created
        this.map.display.ground.clear();
        this.map.renderTerrain(
            this.map.display.ground,
            2 / this.camera.m_ppu,
            this.canvasMode,
            false,
        );
        this.renderer.resize(this.map, this.camera);
    }

    /**
     * Moves a top level authored object (and, for buildings/structures, all of
     * the child objects it generated) to a new position without a full rebuild.
     * Used for live drag feedback.
     */
    moveObj(objId: number, x: number, y: number) {
        const obj = this.objectCreator.m_getObjById(objId);
        if (!obj) return;

        const dx = x - obj.pos.x;
        const dy = y - obj.pos.y;
        if (dx === 0 && dy === 0) return;

        const visited = new Set<number>();
        const move = (o: { __id: number; pos: Vec2; rot: number; __type: ObjectType }) => {
            if (visited.has(o.__id)) return;
            visited.add(o.__id);
            o.pos = v2.create(o.pos.x + dx, o.pos.y + dy);

            switch (o.__type) {
                case ObjectType.Building: {
                    const building = o as unknown as Building;
                    move(building.ceiling as unknown as typeof o);
                    for (const childId of building.childObjects ?? []) {
                        const child = this.objectCreator.m_getObjById(childId);
                        if (child) move(child as unknown as typeof o);
                    }
                    break;
                }
                case ObjectType.Structure: {
                    const structure = o as unknown as Structure;
                    for (const layerId of structure.layers ?? []) {
                        const building = this.objectCreator.m_getObjById(layerId.objId);
                        if (building) move(building as unknown as typeof o);
                    }
                    break;
                }
                case ObjectType.Decal: {
                    const decal = o as unknown as { decalRender?: { pos: Vec2 } | null };
                    if (decal.decalRender) {
                        decal.decalRender.pos = v2.create(
                            decal.decalRender.pos.x + dx,
                            decal.decalRender.pos.y + dy,
                        );
                    }
                    break;
                }
            }
        };

        move(obj as unknown as { __id: number; pos: Vec2; rot: number; __type: ObjectType });
    }

    update(dt: number) {
        const debug = this.config.get("debugRenderer")!;
        const editorCfg = this.config.get("mapEditor")!;
        // Camera
        this.camera.m_pos = v2.copy(this.activePlayer.m_pos);
        this.camera.m_targetZoom = editorCfg.zoom;

        this.camera.m_zoom = math.lerp(
            dt * 5,
            this.camera.m_zoom,
            this.camera.m_targetZoom,
        );

        this.audioManager.cameraPos = v2.copy(this.camera.m_pos);

        if (editorCfg.showGrid) {
            const width = this.camera.m_screenWidth / this.camera.m_z();
            const height = this.camera.m_screenHeight / this.camera.m_z();

            const roundV2 = (v: Vec2) => {
                return {
                    x: Math.round(v.x / 10) * 10,
                    y: Math.round(v.y / 10) * 10,
                };
            };
            const size = v2.create(width, height);
            const start = roundV2(v2.sub(this.camera.m_pos, size));
            const end = roundV2(v2.add(this.camera.m_pos, size));

            function addGrid(steps: number, color: number) {
                for (let y = start.y; y < end.y; y += steps) {
                    debugLines.addLine(v2.create(start.x, y), v2.create(end.x, y), color);
                }
                for (let x = start.x; x < end.x; x += steps) {
                    debugLines.addLine(v2.create(x, start.y), v2.create(x, end.y), color);
                }
            }
            if (this.camera.m_zoom >= 1.5) {
                addGrid(1, 0x424242);
            }
            if (this.camera.m_zoom >= 4.5) {
                addGrid(0.25, 0x424242);
            }
            addGrid(this.camera.m_zoom > 0.5 ? 5 : 10, 0x7f7f7f);

            debugLines.addRay(this.camera.m_pos, v2.create(0, 1), 1, 0x00ff00);
            debugLines.addRay(this.camera.m_pos, v2.create(1, 0), 1, 0x00ff00);
        }

        this.playerBarn.m_update(
            dt,
            this.activeId,
            this.renderer,
            this.particleBarn,
            this.camera,
            this.map,
            this.inputBinds,
            this.audioManager,
            // ui2 manager is only used for updating perks, as long as we dont add any perks
            // it should be fine :)
            undefined as unknown as UiManager2,
            false,
            false,
        );

        if (editorCfg.hideCeilings) {
            const buildings = this.map.m_buildingPool.m_getPool();
            for (let i = 0; i < buildings.length; i++) {
                buildings[i].ceiling.visionTicker = 1;
            }
        }

        this.map.m_update(
            dt,
            this.activePlayer,
            this.playerBarn,
            this.particleBarn,
            this.audioManager,
            this.ambience,
            this.renderer,
            this.camera,
            [],
            debug,
        );
        this.lootBarn.m_update(
            dt,
            this.activePlayer,
            this.map,
            this.audioManager,
            this.camera,
            debug,
        );
        this.smokeBarn.m_update(
            dt,
            this.camera,
            this.activePlayer,
            this.map,
            this.renderer,
        );
        this.particleBarn.m_update(dt, this.camera);
        this.decalBarn.m_update(dt, this.camera, this.renderer);
        this.renderer.m_update(dt, this.camera, this.map, debug.structures.layerMasks);
        this.activePlayer.playActionStartSfx = false;
        this.onUpdate?.(dt);

        this.render(debug);
    }

    render(debug: DebugRendererOpts) {
        const grassColor = this.map.mapLoaded
            ? this.map.getMapDef().biome.colors.grass
            : 0x80af49;

        this.pixi.renderer.background.color = grassColor;

        // Module rendering
        this.playerBarn.m_render(this.camera, debug);
        this.map.m_render(this.camera);

        this.debugDisplay.clear();
        // Overlays (selection, bounds, grid) are recorded into the shared
        // `debugLines` buffer by the tools and replayed here. They must run
        // unconditionally: `debugLines.m_render` is a no-op in production.
        this.drawOverlays();
        debugLines.m_render(this.camera, this.debugDisplay);
        debugLines.flush();
    }

    /** Hook for the tool layer to record selection/bounds overlays. */
    drawOverlays() {}

    /** Hook for the tool layer, called after the world has been updated. */
    onUpdate?: (dt: number) => void;

    resize() {
        if (this.initialized) {
            this.camera.m_screenWidth = device.screenWidth;
            this.camera.m_screenHeight = device.screenHeight;
            this.map.resize(this.pixi.renderer, this.canvasMode);
            this.renderer.resize(this.map, this.camera);
        }
    }
}
