import { MapObjectDefs } from "../../../shared/defs/register.ts";
import { ObjectType } from "../../../shared/net/objectSerializeFns.ts";
import { type Collider, collider } from "../../../shared/utils/collider.ts";
import { mapHelpers } from "../../../shared/utils/mapHelpers.ts";
import { math } from "../../../shared/utils/math.ts";
import { v2, type Vec2 } from "../../../shared/utils/v2.ts";
import type { AbstractObject } from "../../src/objects/objectPool.ts";
import type { EditorDisplay } from "./editorDisplay.ts";
import type { MapObjectEntry } from "./mapDocument.ts";

/**
 * Object picking / hit testing.
 *
 * The server does the same thing through a spatial grid
 * (`server/src/game/objects/player.ts#moveObjUpdate`); the editor client has no
 * grid, so we iterate the pools.
 *
 * The test matches the *drawn sprite*, not just the bounds: a single
 * `toAabb` test against `getBoundingCollider` (what the building editor does) is
 * too generous, so aiming at the ground next to a wall selected the wall, and
 * aiming at a door's bottom half selected the building it belongs to.
 *
 * `score` orders the candidates under the cursor: on the sprite first, then
 * inside the collider of circle-collision obstacles (whose sprite overhangs
 * their collider), then near a sprite or inside bounds.
 */
export interface HitCandidate {
    /** Live object id */
    objId: number;
    /** Index into `MapDoc.objects`, -1 for non-authored objects (e.g. loot) */
    docIndex: number;
    /** Bounding collider, already transformed into world space */
    collider: Collider;
    pos: Vec2;
    layer: number;
    type: string;
    objectType: ObjectType;
    /** Depth used to sort overlapping candidates */
    zIdx: number;
    /** Top level authored objects (0) win over structure floors (1) */
    depth: number;
    /** Lower is a better hit; see `pickAll`. Infinity never reaches here. */
    score: number;
}

function worldBounds(type: string, pos: Vec2, rot: number, scale: number) {
    return collider.transform(mapHelpers.getBoundingCollider(type), pos, rot, scale);
}

function pointInCollider(col: Collider, pos: Vec2) {
    const aabb = collider.toAabb(col);
    return (
        pos.x >= aabb.min.x && pos.x <= aabb.max.x && pos.y >= aabb.min.y && pos.y <= aabb.max.y
    );
}

/** Radius of a circle collider the cursor is inside, or -1. */
function insideCircle(col: Collider, pos: Vec2) {
    if (col.type !== collider.Type.Circle) return -1;
    return v2.lengthSqr(v2.sub(pos, col.pos)) <= col.rad * col.rad ? col.rad : -1;
}

/**
 * Cursor distance to the sprite's own bounding box, in world units.
 *
 * The renderers draw `def.img.scale` throttle units per 32 texture pixels
 * (`obstacle.render` passes `camera.m_pixels(scale * imgScale)` to
 * `sprite.scale`, so a sprite of `px` pixels spans `px / 32 * img.scale` world
 * units), and the obfuscated `getImgWidth`/`getImgHeight` report `px / 32`.
 * The drawn box is therefore `img{Width,Height} / img.scale` world units before
 * the object's own scale. `img.scale` is orthogonal to the collision collider,
 * which is why clicking used to require much more precision than the object
 * looked like it needed.
 */
function imageOverflow(box: { w: number; h: number }, local: Vec2): number {
    const dx = Math.max(-local.x, local.x - box.w, 0);
    const dy = Math.max(-local.y, local.y - box.h, 0);
    return Math.sqrt(dx * dx + dy * dy);
}

/** How far outside the drawn sprite a click still counts, in world units. */
const IMAGE_PICK_SLOP = 1;

function sameLayer(a: number, b: number) {
    return (a & 1) === (b & 1);
}

/** The drawn sprite box of `type` in world units, or null when it has none. */
function imageBox(type: string, scale: number): { w: number; h: number } | null {
    const def = MapObjectDefs.typeToDefSafe(type);
    if (!def?.img?.scale || def.img.scale <= 0) return null;
    const imgScale = def.img.scale;
    // The def stores the sprite filename (e.g. "map-crate-01-res-01.img")
    // but the texture dimensions are only known at runtime from the sprite
    // resource in the PIXI asset manager. There is no precomputed width/height
    // on the def. Fall back to collider-based picking for now.
    return null;
}

interface CandidateArgs {
    obj: AbstractObject;
    /** Object position, used for the collider and the result. */
    pos: Vec2;
    layer: number;
    rot: number;
    scale: number;
    type: string;
    depth: number;
    /** Sprite-less objects (structures, buildings) are hit on their bounds. */
    colliderOnly: boolean;
}

function baseCandidate(
    display: EditorDisplay,
    cursorPos: Vec2,
    args: CandidateArgs,
): HitCandidate | null {
    const { obj, layer, rot, scale, type, depth, colliderOnly } = args;
    const objPos = args.pos;

    // For sprites, `rot` is negated to match `obstacle.rot`: both sprite and rot
    // are then in the same frame the renderer draws (about the object position).
    const local = v2.rotate(cursorPos, -rot);
    const box = colliderOnly ? null : imageBox(type, scale);
    let score: number;

    if (!box) {
        // Nothing is drawn for the object itself (structures, buildings), so its
        // bounds are the only thing the author can aim at.
        score = pointInCollider(worldBounds(type, objPos, rot, scale), cursorPos) ? 0 : Infinity;
    } else {
        const overflow = imageOverflow(box, local);
        if (overflow <= 0) {
            score = 0;
        } else if (insideCircle(worldBounds(type, objPos, rot, scale), cursorPos) >= 0) {
            // Circle-collision obstacles (trees, bushes, stones, barrels) are hit
            // anywhere in their collider: the sprite overhangs it, so a click on
            // the visible foliage that is outside the box is still a hit.
            score = 1;
        } else {
            score = overflow <= IMAGE_PICK_SLOP ? 2 + overflow : Infinity;
        }
    }
    if (score === Infinity) return null;

    return {
        objId: obj.__id,
        docIndex: display.m_objIdToDocIndex.get(obj.__id) ?? -1,
        collider: worldBounds(type, objPos, rot, scale),
        pos: objPos,
        layer,
        type,
        objectType: obj.__type,
        zIdx: (obj as unknown as { zIdx?: number }).zIdx ?? 0,
        depth,
        score,
    };
}

function pushCandidate(candidates: HitCandidate[], candidate: HitCandidate | null) {
    if (candidate) candidates.push(candidate);
}

/**
 * Returns every pickable candidate under `pos`, best (top-most) first.
 *
 * Children generated from a building's `mapObjects` (and a structure's
 * `layers`) are *not* independently selectable: selecting one would author a
 * duplicate of what the parent already spawns. Select the parent instead.
 */
export function pickAll(display: EditorDisplay, pos: Vec2, layer: number): HitCandidate[] {
    const candidates: HitCandidate[] = [];

    const structures = display.map.m_structurePool.m_getPool();
    for (const structure of structures) {
        if (!structure.active) continue;
        if (!display.m_objIdToDocIndex.has(structure.__id)) continue;
        pushCandidate(
            candidates,
            baseCandidate(display, pos, {
                obj: structure,
                pos: structure.pos,
                layer: 0,
                rot: structure.rot,
                scale: structure.scale,
                type: structure.type,
                depth: 1,
                colliderOnly: true,
            }),
        );
    }

    const buildings = display.map.m_buildingPool.m_getPool();
    for (const building of buildings) {
        if (!building.active) continue;
        // Only top level buildings are authored; structure floors are children.
        if (!display.m_objIdToDocIndex.has(building.__id)) continue;
        pushCandidate(
            candidates,
            baseCandidate(display, pos, {
                obj: building,
                pos: building.pos,
                layer: building.layer,
                rot: building.rot,
                scale: building.scale,
                type: building.type,
                depth: 0,
                colliderOnly: true,
            }),
        );
    }

    const obstacles = display.map.m_obstaclePool.m_getPool();
    for (const obstacle of obstacles) {
        if (!obstacle.active) continue;
        if (obstacle.dead || obstacle.isSkin) continue;
        // child obstacles are recreated by their parent building
        if (obstacle.parentBuildingId !== undefined) continue;
        if (!display.m_objIdToDocIndex.has(obstacle.__id)) continue;
        pushCandidate(
            candidates,
            baseCandidate(display, pos, {
                obj: obstacle,
                pos: obstacle.pos,
                layer: obstacle.layer,
                rot: obstacle.rot,
                scale: obstacle.scale,
                type: obstacle.type,
                depth: 0,
                colliderOnly: false,
            }),
        );
    }

    const decals = display.decalBarn.decalPool.m_getPool();
    for (const decal of decals) {
        if (!decal.active) continue;
        if (!display.m_objIdToDocIndex.has(decal.__id)) continue;
        pushCandidate(
            candidates,
            baseCandidate(display, pos, {
                obj: decal,
                pos: decal.pos,
                layer: decal.layer,
                rot: decal.rot,
                scale: decal.scale,
                type: decal.type,
                depth: 0,
                colliderOnly: false,
            }),
        );
    }

    const hits = candidates.filter((candidate) => sameLayer(candidate.layer, layer));

    // Prefer the tightest hit (sprite over collider), then authored top level
    // objects, then a higher zIdx, so clicking a roof selects the building under
    // the cursor rather than an overlapping decal.
    hits.sort((a, b) => {
        if (a.score !== b.score) return a.score - b.score;
        if (a.depth !== b.depth) return a.depth - b.depth;
        if (a.zIdx !== b.zIdx) return b.zIdx - a.zIdx;
        return a.objId - b.objId;
    });

    return hits;
}

export function pickTop(display: EditorDisplay, pos: Vec2, layer: number) {
    return pickAll(display, pos, layer)[0];
}

/** Converts a live top level object back into a document entry. */
export function objectToEntry(display: EditorDisplay, objId: number): MapObjectEntry | null {
    const obj = display.objectCreator.m_getObjById(objId);
    if (!obj) return null;

    switch (obj.__type) {
        case ObjectType.Obstacle: {
            const obstacle = obj as unknown as {
                type: string;
                pos: Vec2;
                rot: number;
                scale: number;
                layer: number;
            };
            return {
                type: obstacle.type,
                pos: v2.copy(obstacle.pos),
                ori: math.radToOri(obstacle.rot),
                scale: obstacle.scale,
                layer: obstacle.layer,
            };
        }
        case ObjectType.Building: {
            const building = obj as unknown as {
                type: string;
                pos: Vec2;
                ori: number;
                layer: number;
            };
            return {
                type: building.type,
                pos: v2.copy(building.pos),
                ori: building.ori,
                scale: 1,
                layer: building.layer,
            };
        }
        case ObjectType.Structure: {
            const structure = obj as unknown as {
                type: string;
                pos: Vec2;
                rot: number;
            };
            return {
                type: structure.type,
                pos: v2.copy(structure.pos),
                ori: math.radToOri(structure.rot),
                scale: 1,
                layer: 0,
            };
        }
        case ObjectType.Decal: {
            const decal = obj as unknown as {
                type: string;
                pos: Vec2;
                rot: number;
                scale: number;
                layer: number;
            };
            return {
                type: decal.type,
                pos: v2.copy(decal.pos),
                ori: math.radToOri(decal.rot),
                scale: decal.scale,
                layer: decal.layer,
            };
        }
        default:
            return null;
    }
}
