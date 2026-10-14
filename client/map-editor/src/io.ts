import { MapObjectDefs } from "../../../shared/defs/register.ts";
import { MapMsg } from "../../../shared/net/mapMsg.ts";
import { BitStream } from "../../../shared/net/net.ts";
import { ObjectType } from "../../../shared/net/objectSerializeFns.ts";
import { math } from "../../../shared/utils/math.ts";
import { v2 } from "../../../shared/utils/v2.ts";
import type { Building } from "../../src/objects/building.ts";
import type { Obstacle } from "../../src/objects/obstacle.ts";
import type { Structure } from "../../src/objects/structure.ts";
import type { EditorDisplay } from "./editorDisplay.ts";
import { type MapDoc, type MapObjectEntry, normalizeDoc } from "./mapDocument.ts";

/**
 * Documents live in their own localStorage key. `ConfigManager` owns
 * `surviv_config`, which is re-serialized on every `set()` — a whole map would
 * be written to disk on every edit and would be passed through every
 * `onConfigModified` listener.
 */
export const MAP_DOC_STORAGE_KEY = "surviv_map_editor_doc";

/** Refuse to autosave beyond this many bytes of JSON. */
export const MAP_DOC_STORAGE_MAX_BYTES = 4 * 1024 * 1024;

/**
 * Exports a document as a `MapMsg`.
 *
 * `objects` only includes top level entries that would be sent to the client as
 * minimap markers (`def.map?.display` at layer 0), matching the rule used by
 * the server in `server/src/game/map.ts#genObstacle` / `genBuilding`. Child
 * obstacles belonging to a building are skipped: the parent recreates them.
 */
export function toMapMsg(display: EditorDisplay, doc: MapDoc): MapMsg {
    const msg = new MapMsg();

    msg.mapName = doc.mapName;
    msg.seed = doc.seed;
    msg.width = doc.width;
    msg.height = doc.height;
    msg.shoreInset = doc.shoreInset;
    msg.grassInset = doc.grassInset;

    msg.rivers = doc.rivers.map((river) => {
        return {
            width: river.width,
            looped: river.looped,
            points: river.points.map((p) => v2.copy(p)),
        };
    });

    msg.places = doc.places.map((place) => {
        return {
            name: place.name,
            pos: v2.copy(place.pos),
        };
    });

    msg.objects = [];
    for (let i = 0; i < doc.objects.length; i++) {
        const objId = display.m_docIndexToObjId[i];
        const obj = objId === undefined ? undefined : display.objectCreator.m_getObjById(objId);
        if (!obj) continue;

        if (obj.__type === ObjectType.Building) {
            const building = obj as unknown as Building;
            if (building.layer !== 0) continue;
            msg.objects.push({
                type: building.type,
                pos: v2.copy(building.pos),
                ori: building.ori,
                scale: 1,
            });
        } else if (obj.__type === ObjectType.Structure) {
            const structure = obj as unknown as Structure;
            msg.objects.push({
                type: structure.type,
                pos: v2.copy(structure.pos),
                ori: math.radToOri(structure.rot),
                scale: 1,
            });
        } else if (obj.__type === ObjectType.Obstacle) {
            const obstacle = obj as unknown as Obstacle;
            if (obstacle.parentBuildingId !== undefined) continue;
            const def = MapObjectDefs.typeToDef(obstacle.type);
            if (!def.map?.display || obstacle.layer !== 0) continue;
            msg.objects.push({
                type: obstacle.type,
                pos: v2.copy(obstacle.pos),
                ori: math.radToOri(obstacle.rot),
                scale: obstacle.scale,
            });
        }
    }

    msg.groundPatches = display.mapMsg.groundPatches.map((patch) => {
        return {
            bound: patch.bound,
            color: patch.color,
            roughness: patch.roughness,
            offsetDist: patch.offsetDist,
            order: patch.order ?? 0,
            useAsMapShape: patch.useAsMapShape ?? true,
        };
    });

    return msg;
}

/** The authoring view of a document — the fields the editor is able to edit. */
export interface MapDocSummary {
    mapName: string;
    seed: number;
    width: number;
    height: number;
    shoreInset: number;
    grassInset: number;
    rivers: number;
    places: number;
    objects: number;
    objectCounts: Array<[string, number]>;
}

export function summarize(doc: MapDoc): MapDocSummary {
    const counts = new Map<string, number>();
    for (const entry of doc.objects) {
        counts.set(entry.type, (counts.get(entry.type) ?? 0) + 1);
    }
    const objectCounts = [...counts.entries()];
    objectCounts.sort((a, b) => b[1] - a[1]);

    return {
        mapName: doc.mapName,
        seed: doc.seed,
        width: doc.width,
        height: doc.height,
        shoreInset: doc.shoreInset,
        grassInset: doc.grassInset,
        rivers: doc.rivers.length,
        places: doc.places.length,
        objects: doc.objects.length,
        objectCounts,
    };
}

/**
 * Rebuilds a document from a `MapMsg`. `MapMsg` has no layer field (everything
 * it carries is on the ground layer) so layer is filled with 0.
 */
export function toDocument(mapMsg: MapMsg, fallbackMapName: MapDoc["mapName"]): MapDoc {
    return normalizeDoc(
        {
            version: 1,
            mapName: mapMsg.mapName || fallbackMapName,
            seed: mapMsg.seed,
            width: mapMsg.width,
            height: mapMsg.height,
            shoreInset: mapMsg.shoreInset,
            grassInset: mapMsg.grassInset,
            rivers: mapMsg.rivers,
            places: mapMsg.places,
            objects: mapMsg.objects.map((obj): MapObjectEntry => {
                return {
                    type: obj.type,
                    pos: v2.copy(obj.pos),
                    ori: obj.ori,
                    scale: obj.scale,
                    layer: 0,
                };
            }),
        },
        fallbackMapName,
    );
}

export function docToJson(doc: MapDoc) {
    return JSON.stringify(doc, null, 4);
}

export function parseDocJson(text: string, fallbackMapName: MapDoc["mapName"]): MapDoc {
    return normalizeDoc(JSON.parse(text), fallbackMapName);
}

export function exportFilename(doc: MapDoc) {
    return `map-${doc.mapName}-${doc.seed}.json`;
}

export function exportJson(doc: MapDoc, filename?: string) {
    const blob = new Blob([docToJson(doc)], { type: "application/json" });
    const url = URL.createObjectURL(blob);
    const link = document.createElement("a");
    link.href = url;
    link.download = filename ?? exportFilename(doc);
    document.body.appendChild(link);
    link.click();
    link.remove();
    URL.revokeObjectURL(url);
}

/**
 * Serializes the derived `MapMsg` into a `BitStream`, useful for inspecting the
 * wire representation. The JSON document remains the source of truth.
 *
 * NOTE: `MapMsg.serialize` is wrapped in `STRIP_FROM_PROD_CLIENT` blocks in
 * `shared/net/mapMsg.ts`, so this only does something in a dev build.
 */
export function serializeMapMsg(msg: MapMsg) {
    const stream = new BitStream(new ArrayBuffer(1 << 22));
    msg.serialize(stream);
    return stream;
}

export function saveToLocalStorage(doc: MapDoc) {
    const text = docToJson(doc);
    if (text.length > MAP_DOC_STORAGE_MAX_BYTES) {
        console.warn("Map document too large to autosave", text.length);
        return false;
    }
    try {
        localStorage.setItem(MAP_DOC_STORAGE_KEY, text);
        return true;
    } catch (_e) {
        console.warn("Failed to autosave map document");
        return false;
    }
}
export function loadFromLocalStorage(fallbackMapName: MapDoc["mapName"]): MapDoc | null {
    let text: string | null = null;
    try {
        text = localStorage.getItem(MAP_DOC_STORAGE_KEY);
    } catch (_e) {
        return null;
    }
    if (!text) return null;
    try {
        return parseDocJson(text, fallbackMapName);
    } catch (e) {
        console.warn("Failed to load autosaved map document", e);
        return null;
    }
}

export function clearLocalStorage() {
    try {
        localStorage.removeItem(MAP_DOC_STORAGE_KEY);
    } catch (_e) {
        // ignore
    }
}
