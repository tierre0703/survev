import { type MapDefKey } from "../../../shared/defs/mapDefs.ts";
import { MapObjectDefs } from "../../../shared/defs/register.ts";
import { TeamMode } from "../../../shared/gameConfig.ts";
import { util } from "../../../shared/utils/util.ts";
import { v2, type Vec2 } from "../../../shared/utils/v2.ts";
import { Game } from "../game/game.ts";

/**
 * Captures a live generated `GameMap` as a map editor document.
 *
 * This is the deferred phase 7 of `client/map-editor/MAP-EDITOR-PLAN.md` in
 * reverse: instead of the server reading an authored object list, the server
 * writes the object list it generated. The result uses the same document shape
 * as `client/map-editor/src/mapDocument.ts`, so the editor can open it and, in
 * the future, a `MapDef` could be created from it.
 *
 * The capture is lossy by design:
 *
 * - Only top level `mapGen` output is captured (the same rule
 *   `client/map-editor/src/io.ts#toMapMsg` uses). Child objects are recreated by
 *   their parent building/structure on rebuild.
 * - Density spawns (`tree_01`, `bush_*`, ...) are not authored: a `MapDef` grows
 *   them again from `mapGen.densitySpawns`, which is what the capture skips.
 * - Decals whose def fades (`lifetime`) are skipped, because a rebuild re-rolls
 *   them. Ground patches are captured instead of the decals that painted them,
 *   matching the editor's derived `mapMsg.groundPatches`.
 * - Objects use `MapObjectDefs.typeToDef` (the same call the client editor's
 *   `addAuto` makes), so a `spawnReplacements` entry cannot be misread at
 *   rebuild time.
 * - Orientation is stored as the document's 0..3 `ori`, taken from the object's
 *   `ori` field (the form `MapObjectEntry` uses).
 * - `layer` is a plain number from the document, but `MapObjectEntry` treats it
 *   as 0 or 1. Objects on stair side layers (2/3) are captured as 0/1, and the
 *   rebuild walks them back onto the stairs.
 */
export interface MapDocDocument {
    version: 1;
    generator?: string;
    mapName: string;
    seed: number;
    width: number;
    height: number;
    shoreInset: number;
    grassInset: number;
    rivers: Array<{
        width: number;
        looped: boolean;
        points: Vec2[];
    }>;
    places: Array<{
        name: string;
        pos: Vec2;
    }>;
    groundPatches: Array<{
        bound: {
            /** `collider.Type.Aabb`; structures/buildings only emit AABB patches. */
            type: number;
            min: Vec2;
            max: Vec2;
        };
        color: number;
        roughness: number;
        offsetDist: number;
        order: number;
        useAsMapShape: boolean;
    }>;
    objects: Array<{
        type: string;
        pos: Vec2;
        ori: number;
        scale: number;
        layer: number;
    }>;
}

const CAPTURE_GENERATOR = "survev-capture-map";

/** `Structure` orients itself from `ori` too; this keeps the read in one place. */
function oriOf(obj: { ori?: number; rot?: number }) {
    return obj.ori ?? 0;
}

function mapKeyOf(game: Game): string {
    return game.config.mapName;
}

/**
 * Builds the document for `game`'s map.
 *
 * `game.map` must have finished `init()`.
 */
export function captureMapDoc(game: Game): MapDocDocument {
    const map = game.map;

    const objects: MapDocDocument["objects"] = [];
    const push = (type: string, pos: Vec2, ori: number, scale: number, layer: number) => {
        if (!MapObjectDefs.typeExists(type)) {
            // Defensive: every authored entry has to rebuild through `MapObjectDefs`.
            throw new Error(`capture: unknown map object type "${type}"`);
        }
        objects.push({
            type,
            pos: v2.copy(pos),
            // `MapObjectEntry.ori` is 0..3; wrap negative or unbounded values.
            ori: ((ori % 4) + 4) % 4,
            scale,
            // `MapObjectEntry.layer` is 0 or 1; stair side layers (2/3) fold down.
            layer: layer === 1 ? 1 : 0,
        });
    };

    // Top level obstacles only: children are recreated by their building, and
    // density spawns are regenerated from `mapGen.densitySpawns` by the map def.
    const densitySpawns = new Set(Object.keys(map.mapDef.mapGen.densitySpawns[0]));
    for (const obstacle of map.obstacles) {
        if (obstacle.parentBuildingId !== undefined) continue;
        if (densitySpawns.has(obstacle.type)) continue;
        const def = MapObjectDefs.typeToDefSafe(obstacle.type);
        if (def?.type !== "obstacle") continue;
        push(obstacle.type, obstacle.pos, oriOf(obstacle), obstacle.scale, obstacle.layer);
    }

    // Top level buildings. Structure floors and nested buildings are children.
    for (const building of map.buildings) {
        if (building.parentStructure !== undefined || building.parentBuilding !== undefined) {
            continue;
        }
        push(building.type, building.pos, building.ori, 1, building.layer);
    }

    for (const structure of map.structures) {
        if (structure.parentBuilding !== undefined) continue;
        push(structure.type, structure.pos, oriOf(structure), 1, 0);
    }

    for (const decal of game.decalBarn.decals) {
        const def = MapObjectDefs.typeToDefSafe(decal.type);
        // Fading decals re-roll on rebuild, so they are not authorable.
        if (def?.type !== "decal" || def.lifetime) continue;
        push(decal.type, decal.pos, oriOf(decal), decal.scale, decal.layer);
    }

    return {
        version: 1,
        generator: CAPTURE_GENERATOR,
        mapName: mapKeyOf(game),
        seed: map.seed,
        width: map.width,
        height: map.height,
        shoreInset: map.shoreInset,
        grassInset: map.grassInset,
        rivers: map.msg.rivers.map((river) => {
            return {
                width: river.width,
                looped: river.looped,
                points: river.points.map((p) => v2.copy(p)),
            };
        }),
        places: map.msg.places.map((place) => {
            return {
                name: place.name,
                pos: v2.copy(place.pos),
            };
        }),
        groundPatches: map.msg.groundPatches.map((patch) => {
            const aabb = patch.bound as { min: Vec2; max: Vec2 };
            return {
                bound: {
                    type: patch.bound.type,
                    min: v2.copy(aabb.min),
                    max: v2.copy(aabb.max),
                },
                color: patch.color,
                roughness: patch.roughness,
                offsetDist: patch.offsetDist,
                order: patch.order ?? 0,
                useAsMapShape: patch.useAsMapShape ?? true,
            };
        }),
        objects,
    };
}

export interface CaptureMapOptions {
    mapName: MapDefKey;
    /** Omit to pick a random seed that produces a river. */
    seed?: number;
    teamMode?: "solo" | "duo" | "squad";
}

const TEAM_MODES: Record<string, TeamMode> = {
    solo: TeamMode.Solo,
    duo: TeamMode.Duo,
    squad: TeamMode.Squad,
};

/**
 * Generates a map from scratch and captures it.
 *
 * Without an explicit seed the map is regenerated until it contains a river: a
 * riverless seed produces a technically valid but poor authoring starting point.
 * `solo` builds the small (512px) map, `duo`/`squad` the large (720px) one,
 * matching `GameMap`'s own scale choice.
 */
export function generateMapDoc(opts: CaptureMapOptions): MapDocDocument {
    const attempts = 20;
    const teamMode = TEAM_MODES[opts.teamMode ?? "solo"];
    for (let i = 0; i < attempts; i++) {
        const seed = opts.seed ?? util.randomInt(0, 2 ** 32 - 1);
        const game = new Game("capture", {
            mapName: opts.mapName,
            teamMode,
        });
        // `GameMap.init` picks a random seed on its own; force ours.
        game.map.init(seed);

        const doc = captureMapDoc(game);

        if (opts.seed !== undefined || doc.rivers.length > 0) {
            return doc;
        }
    }

    throw new Error(`Failed to generate a map with rivers for ${opts.mapName}`);
}
