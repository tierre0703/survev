import { MapDefs } from "../../../shared/defs/mapDefs.ts";
import { MapObjectDefs } from "../../../shared/defs/register.ts";
import type { MapMsg } from "../../../shared/net/mapMsg.ts";
import { math } from "../../../shared/utils/math.ts";
import { util } from "../../../shared/utils/util.ts";
import { v2, type Vec2 } from "../../../shared/utils/v2.ts";
import type { ConfigManager } from "../../src/config.ts";
import type { EditorDisplay } from "./editorDisplay.ts";
import { exportJson, loadFromLocalStorage, mapDataToDocument, saveToLocalStorage, toMapMsg } from "./io.ts";
import { createDefaultDoc, type MapDoc, normalizeDoc } from "./mapDocument.ts";

export interface EditorContextOptions {
    display: EditorDisplay;
    config: ConfigManager;
    onNotice?: (message: string) => void;
}

export interface HistoryLike {
    canUndo: boolean;
    canRedo: boolean;
    push(doc: MapDoc): void;
    undo(current: MapDoc): MapDoc | null;
    redo(current: MapDoc): MapDoc | null;
    clear(): void;
}

/**
 * Owns the authored document, its replay into the render pools, and all
 * document mutations. Every mutation the UI performs goes through here so undo
 * and autosave stay consistent.
 */
export class EditorContext {
    display: EditorDisplay;

    doc: MapDoc;

    /** Replaced with `History` from `main.ts`. */
    history: HistoryLike = {
        canUndo: false,
        canRedo: false,
        push: (_doc: MapDoc) => {},
        undo: (_doc: MapDoc): MapDoc | null => null,
        redo: (_doc: MapDoc): MapDoc | null => null,
        clear: () => {},
    };

    /** Indices into `doc.objects`. Stable while the document is unchanged. */
    selection: number[] = [];

    /** Index of the entry being dragged, or -1 */
    draggingIndex = -1;
    private dragStartPos: Vec2 | null = null;
    private dragStartEntryPos: Vec2 | null = null;
    private dragMoved = false;

    /** Cursor position in world space, updated every frame */
    cursor: Vec2 = v2.create(0, 0);

    /** Set while a drag/paint gesture is in progress so it is one undo step. */
    private gesturePushed = false;

    /** Clipboard for copy/paste */
    clipboard: MapDoc["objects"] = [];

    /** Top-most object under the cursor. Wired up by `EditorTools`. */
    getHover: () => { type: string } | undefined = () => undefined;

    onNotice: (message: string) => void;

    constructor(opts: EditorContextOptions) {
        this.display = opts.display;
        this.onNotice = opts.onNotice ?? (() => {});
        this.doc = createDefaultDoc("main");
    }

    setHistory(history: HistoryLike) {
        this.history = history;
    }

    get selectedEntries() {
        const entries: MapDoc["objects"] = [];
        for (const i of this.selection) {
            const entry = this.doc.objects[i];
            if (entry) entries.push(entry);
        }
        return entries;
    }

    //
    // Document lifecycle
    //

    /** Replays `doc` into the render pools. Does not touch history. */
    rebuild() {
        this.display.loadDocument(this.doc);
        this.selection = this.selection.filter((i) => !!this.doc.objects[i]);
    }

    /**
     * Runs `fn` as a single, undoable document mutation.
     */
    edit(fn: (doc: MapDoc) => void, opts: { rebuild?: boolean } = {}) {
        const rebuild = opts.rebuild ?? true;
        this.history.push(this.doc);
        fn(this.doc);
        if (rebuild) {
            this.rebuild();
        }
        this.autosave();
    }

    /** Push the pre-gesture state once, then mutate freely for the gesture. */
    beginGesture() {
        if (this.gesturePushed) return;
        this.history.push(this.doc);
        this.gesturePushed = true;
    }

    endGesture(rebuild = true) {
        if (!this.gesturePushed) return;
        this.gesturePushed = false;
        if (rebuild) this.rebuild();
        this.autosave();
    }

    get gestureActive() {
        return this.gesturePushed;
    }

    undo() {
        const prev = this.history.undo(this.doc);
        if (!prev) return;
        this.doc = prev;
        this.selection = [];
        this.rebuild();
        this.autosave();
    }

    redo() {
        const next = this.history.redo(this.doc);
        if (!next) return;
        this.doc = next;
        this.selection = [];
        this.rebuild();
        this.autosave();
    }

    /**
     * Autosaves the document, but never inside a gesture: a drag mutates the
     * document every frame, and serializing the whole map per frame is what the
     * `MAP_DOC_STORAGE_MAX_BYTES` guard exists to survive. The write happens once
     * when the gesture ends.
     */
    autosave() {
        if (this.gesturePushed) return;
        saveToLocalStorage(this.doc);
    }

    loadDoc(doc: MapDoc, opts: { clearHistory?: boolean } = {}) {
        this.doc = normalizeDoc(doc, this.doc.mapName);
        if (opts.clearHistory !== false) {
            this.history.clear();
        }
        this.selection = [];
        this.rebuild();
        this.autosave();
    }

    loadFromStorage() {
        const stored = loadFromLocalStorage(this.doc.mapName);
        if (stored) {
            this.doc = stored;
            this.selection = [];
            this.rebuild();
            return true;
        }
        return false;
    }

    /**
     * Loads a map captured by `server/src/mapdoc/captureMap.ts` (or by a
     * `MapMsg` in the same JSON shape). Used when the page is opened with
     * `?mapData=<url>`.
     *
     * The map key from the document is trusted only if it is a real `MapDef`;
     * `loadDoc` -> `display.loadDocument` still validates it before loading
     * assets.
     */
    async loadMapDataUrl(url: string) {
        const res = await fetch(url);
        if (!res.ok) {
            throw new Error(`HTTP ${res.status} for ${url}`);
        }
        const json: unknown = await res.json();
        const raw = json as { mapData?: unknown } | null;
        const mapData = (raw?.mapData && typeof raw.mapData === "object"
            ? raw.mapData
            : json) as Partial<MapMsg>;

        const fallback = this.doc.mapName;
        const candidate = mapData.mapName;
        const mapName = typeof candidate === "string" && candidate in MapDefs
            ? (candidate as MapDoc["mapName"])
            : fallback;

        this.loadDoc(mapDataToDocument(mapData, mapName));
        return this.doc;
    }

    resetDoc() {
        this.loadDoc(createDefaultDoc(this.doc.mapName));
        this.onNotice("Document reset");
    }

    //
    // Editing operations
    //

    addEntry(entry: MapDoc["objects"][number], opts: { select?: boolean } = {}) {
        const select = opts.select ?? true;
        this.edit((doc) => {
            doc.objects.push(entry);
            if (select) this.selection = [doc.objects.length - 1];
        });
        return this.doc.objects.length - 1;
    }

    deleteSelection() {
        if (!this.selection.length) return;
        this.edit((doc) => {
            doc.objects = doc.objects.filter((_e, i) => !this.selection.includes(i));
            this.selection = [];
        });
    }

    clearObjects() {
        this.edit((doc) => {
            doc.objects.length = 0;
            this.selection = [];
        });
    }

    rotateSelection(delta: number) {
        if (!this.selection.length) return;
        this.edit((doc) => {
            for (const i of this.selection) {
                const entry = doc.objects[i];
                if (!entry) continue;
                // Buildings and structures only support 4 orientations
                entry.ori = (entry.ori + delta + 4) % 4;
            }
        });
    }

    /** 180 degree flip, the only mirroring the 4-orientation model supports. */
    flipSelection() {
        if (!this.selection.length) return;
        this.edit((doc) => {
            for (const i of this.selection) {
                const entry = doc.objects[i];
                if (!entry) continue;
                entry.ori = (entry.ori + 2) % 4;
            }
        });
    }

    /** Scales obstacles/decals; buildings and structures are always 1. */
    scaleSelection(mult: number) {
        if (!this.selection.length) return;
        this.edit((doc) => {
            for (const i of this.selection) {
                const entry = doc.objects[i];
                if (!entry) continue;
                const def = MapObjectDefs.typeToDefSafe(entry.type);
                if (!def) continue;
                if (def.type === "building" || def.type === "structure") continue;
                entry.scale = math.clamp(entry.scale * mult, 0.1, 10);
            }
        });
    }

    /** Nudges the selection by `dx`/`dy`, clamped to the current map bounds. */
    nudgeSelection(dx: number, dy: number) {
        if (!this.selection.length) return;
        const maxX = this.display.map.width;
        const maxY = this.display.map.height;
        this.edit((doc) => {
            for (const i of this.selection) {
                const entry = doc.objects[i];
                if (!entry) continue;
                entry.pos = {
                    x: math.clamp(Math.round((entry.pos.x + dx) * 10) / 10, 0, maxX),
                    y: math.clamp(Math.round((entry.pos.y + dy) * 10) / 10, 0, maxY),
                };
            }
        });
    }

    duplicateSelection() {
        if (!this.selection.length) return;
        const offset = 8;
        this.edit((doc) => {
            const copies: MapDoc["objects"] = [];
            for (const i of this.selection) {
                const entry = doc.objects[i];
                if (!entry) continue;
                copies.push({
                    ...entry,
                    pos: v2.create(entry.pos.x + offset, entry.pos.y + offset),
                });
            }
            const base = doc.objects.length;
            doc.objects.push(...copies);
            this.selection = copies.map((_c, i) => base + i);
        });
    }

    copySelection() {
        this.clipboard = this.selectedEntries.map((entry) => {
            return {
                ...entry,
                pos: v2.copy(entry.pos),
            };
        });
        if (this.clipboard.length) {
            this.onNotice(`Copied ${this.clipboard.length} object(s)`);
        }
    }

    pasteClipboard() {
        if (!this.clipboard.length) return;
        const offset = 8;
        this.edit((doc) => {
            const base = doc.objects.length;
            doc.objects.push(
                ...this.clipboard.map((entry) => {
                    return {
                        ...entry,
                        pos: v2.create(entry.pos.x + offset, entry.pos.y + offset),
                    };
                }),
            );
            this.selection = this.clipboard.map((_e, i) => base + i);
        });
    }

    setSelection(indices: number[]) {
        this.selection = indices;
    }

    toggleSelection(index: number) {
        const i = this.selection.indexOf(index);
        if (i >= 0) {
            this.selection.splice(i, 1);
        } else {
            this.selection.push(index);
        }
    }

    /** Moves the camera so the selection is centred. */
    focusSelection() {
        const entries = this.selectedEntries;
        if (!entries.length) return;

        let minX = Infinity;
        let minY = Infinity;
        let maxX = -Infinity;
        let maxY = -Infinity;
        for (const entry of entries) {
            minX = Math.min(minX, entry.pos.x);
            minY = Math.min(minY, entry.pos.y);
            maxX = Math.max(maxX, entry.pos.x);
            maxY = Math.max(maxY, entry.pos.y);
        }
        this.moveCameraTo(v2.create((minX + maxX) / 2, (minY + maxY) / 2));
    }

    /**
     * Moves the live objects of the current selection to match `doc.objects`.
     * Used for instant drag feedback without rebuilding the world.
     */
    previewSelectionPositions() {
        for (const i of this.selection) {
            const entry = this.doc.objects[i];
            if (!entry) continue;
            const objId = this.display.m_docIndexToObjId[i];
            if (objId === undefined || objId < 0) continue;
            this.display.moveObj(objId, entry.pos.x, entry.pos.y);
        }
    }

    /** World position -> camera (stored relative to the map centre). */
    moveCameraTo(worldPos: Vec2) {
        const clamped = v2.create(
            math.clamp(worldPos.x, 0, this.display.map.width),
            math.clamp(worldPos.y, 0, this.display.map.height),
        );
        const rel = this.display.toBuildingPos(clamped);
        const config = this.display.config.get("mapEditor")!;
        config.pos = rel;
        this.display.config.set("mapEditor", config);
        this.display.setPlayerPos(clamped);
    }

    //
    // Map level parameters. These all require a full rebuild.
    //

    setMapParam<K extends keyof MapDoc>(key: K, value: MapDoc[K], rebuild = true) {
        this.edit((doc) => {
            doc[key] = value;
        }, { rebuild });
    }

    setMapName(mapName: string) {
        if (!(mapName in MapDefs)) {
            this.onNotice(`Unknown map "${mapName}"`);
            return;
        }
        this.edit((doc) => {
            doc.mapName = mapName as MapDoc["mapName"];
        });
    }

    setSeedFromInput(text: string) {
        const seed = Number.parseInt(text, 10);
        if (!Number.isFinite(seed)) {
            this.onNotice("Invalid seed");
            return;
        }
        this.edit((doc) => {
            doc.seed = seed >>> 0;
        });
    }

    randomSeed() {
        this.edit((doc) => {
            doc.seed = util.randomInt(0, 2 ** 32 - 1) >>> 0;
        });
    }

    addRiver() {
        const w = this.display.map.width;
        const h = this.display.map.height;
        const baseY = h * util.random(0.3, 0.7);
        this.edit((doc) => {
            if (doc.rivers.length >= 8) {
                this.onNotice("At most 8 rivers");
                return;
            }
            const width = util.randomItem([4, 8, 16]);
            doc.rivers.push({
                width,
                looped: false,
                points: [
                    v2.create(w * 0.15, baseY),
                    v2.create(w * 0.5, baseY + h * util.random(-0.1, 0.1)),
                    v2.create(w * 0.85, baseY),
                ],
            });
        });
    }

    removeRiver(index: number) {
        this.edit((doc) => {
            doc.rivers.splice(index, 1);
        });
    }

    addPlace(name: string, pos: Vec2) {
        this.edit((doc) => {
            if (doc.places.length >= 8) {
                this.onNotice("At most 8 places");
                return;
            }
            doc.places.push({
                name: name || `Place ${doc.places.length + 1}`,
                pos: v2.copy(pos),
            });
        });
    }

    removePlace(index: number) {
        this.edit((doc) => {
            doc.places.splice(index, 1);
        });
    }

    //
    // Export
    //

    exportDoc() {
        exportJson(this.doc);
    }

    getMapMsg() {
        return toMapMsg(this.display, this.doc);
    }
}
