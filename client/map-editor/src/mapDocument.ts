import type { MapDefKey } from "../../../shared/defs/mapDefs.ts";
import { Main } from "../../../shared/defs/maps/baseDefs.ts";
import type { MapRiverData } from "../../../shared/utils/terrainGen.ts";
import { v2, type Vec2 } from "../../../shared/utils/v2.ts";

/**
 * A single authored map object.
 *
 * This is intentionally a *flat* list: the renderer (and the server) expand
 * `Structure.layers` / `Building.mapObjects` themselves, so the document only
 * needs to describe the top-level entry the author placed.
 */
export interface MapObjectEntry {
    /** `MapObjectDefs` type key */
    type: string;
    pos: Vec2;
    /** 0..3 */
    ori: number;
    scale: number;
    /** 0 or 1 (buildings are layer agnostic) */
    layer: number;
}

export interface MapPlace {
    name: string;
    pos: Vec2;
}

/**
 * The authored map document. This is the source of truth for the editor and is
 * deliberately free of PIXI/DOM so it can be consumed by other targets later.
 */
export interface MapDoc {
    version: 1;
    mapName: MapDefKey;
    seed: number;
    width: number;
    height: number;
    shoreInset: number;
    grassInset: number;
    rivers: MapRiverData[];
    places: MapPlace[];
    objects: MapObjectEntry[];
}

export const MAP_DOC_VERSION = 1;

/** `main` is 512 base * 1.28125 (large) + 112 extension, matching a live map */
export const DEFAULT_MAP_WIDTH = 720;
export const DEFAULT_MAP_HEIGHT = 720;
export const DEFAULT_MAP_SEED = 218051654;

/**
 * Width/height are serialized as uint16 in `MapMsg`, and the object/place/river
 * arrays are capped by the wire format. These are the hard limits the editor
 * clamps to so an export always round-trips.
 */
export const MAP_DOC_LIMITS = {
    minSize: 64,
    maxSize: 0xffff,
    maxRivers: 8,
    maxPlaces: 8,
    /** `MapMsg.objects` is capped at 16 on the wire, but the editor document is
     * not the wire message; ground patches and the minimap object list are
     * derived. Keep a generous cap so the UI can warn on absurd documents. */
    maxObjects: 100000,
} as const;

export function createDefaultDoc(mapName: MapDefKey = "main"): MapDoc {
    const mapGen = (Main.mapGen ?? {}) as Partial<typeof Main.mapGen>;

    return {
        version: MAP_DOC_VERSION,
        mapName,
        seed: DEFAULT_MAP_SEED,
        width: DEFAULT_MAP_WIDTH,
        height: DEFAULT_MAP_HEIGHT,
        shoreInset: mapGen.map?.shoreInset ?? 48,
        grassInset: mapGen.map?.grassInset ?? 18,
        rivers: [],
        places: (mapGen.places ?? []).map((place) => {
            return {
                name: place.name,
                pos: v2.copy(place.pos),
            };
        }),
        objects: [],
    };
}

function clampNumber(value: unknown, min: number, max: number, fallback: number) {
    if (typeof value !== "number" || !Number.isFinite(value)) return fallback;
    return Math.min(Math.max(value, min), max);
}

function readVec(value: unknown, fallback: Vec2): Vec2 {
    const v = value as Partial<Vec2> | undefined;
    if (!v || typeof v.x !== "number" || typeof v.y !== "number") {
        return v2.copy(fallback);
    }
    if (!Number.isFinite(v.x) || !Number.isFinite(v.y)) {
        return v2.copy(fallback);
    }
    return v2.create(v.x, v.y);
}

/**
 * Validates and normalises an unknown value (e.g. parsed JSON) into a `MapDoc`.
 *
 * Never throws: unknown/extra fields are dropped and missing fields are filled
 * from the defaults. Invalid object entries are skipped.
 */
export function normalizeDoc(data: unknown, fallbackMapName: MapDefKey = "main"): MapDoc {
    const doc = createDefaultDoc(fallbackMapName);
    if (!data || typeof data !== "object") return doc;

    const raw = data as Partial<MapDoc>;

    if (typeof raw.mapName === "string" && raw.mapName) {
        doc.mapName = raw.mapName as MapDefKey;
    }

    doc.seed = clampNumber(raw.seed, 0, 0xffffffff, doc.seed) >>> 0;

    doc.width = Math.round(
        clampNumber(raw.width, MAP_DOC_LIMITS.minSize, MAP_DOC_LIMITS.maxSize, doc.width),
    );
    doc.height = Math.round(
        clampNumber(raw.height, MAP_DOC_LIMITS.minSize, MAP_DOC_LIMITS.maxSize, doc.height),
    );

    doc.shoreInset = Math.round(clampNumber(raw.shoreInset, 0, 1024, doc.shoreInset));
    doc.grassInset = Math.round(clampNumber(raw.grassInset, 0, 1024, doc.grassInset));

    if (Array.isArray(raw.rivers)) {
        doc.rivers = raw.rivers
            .slice(0, MAP_DOC_LIMITS.maxRivers)
            .map((river) => {
                const r = (river ?? {}) as Partial<MapRiverData>;
                return {
                    width: Math.round(clampNumber(r.width, 0, 255, 4)),
                    looped: !!r.looped,
                    points: Array.isArray(r.points)
                        ? r.points.map((p) => readVec(p, v2.create(0, 0)))
                        : [],
                };
            });
    }

    if (Array.isArray(raw.places)) {
        doc.places = raw.places
            .slice(0, MAP_DOC_LIMITS.maxPlaces)
            .map((place) => {
                const p = (place ?? {}) as Partial<MapPlace>;
                return {
                    name: typeof p.name === "string" ? p.name : "",
                    pos: readVec(p.pos, v2.create(0.5, 0.5)),
                };
            });
    }

    if (Array.isArray(raw.objects)) {
        doc.objects = raw.objects
            .slice(0, MAP_DOC_LIMITS.maxObjects)
            .filter((entry) => {
                const e = (entry ?? {}) as Partial<MapObjectEntry>;
                return typeof e.type === "string" && e.type.length > 0;
            })
            .map((entry) => {
                const e = entry as MapObjectEntry;
                return {
                    type: e.type,
                    pos: readVec(e.pos, v2.create(0, 0)),
                    ori: Math.round(clampNumber(e.ori, 0, 3, 0)),
                    scale: clampNumber(e.scale, 0.01, 100, 1),
                    layer: clampNumber(e.layer, 0, 1, 0),
                };
            });
    }

    return doc;
}
