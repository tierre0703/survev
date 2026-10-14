import { MapObjectDefs } from "../../../shared/defs/register.ts";
import { collider } from "../../../shared/utils/collider.ts";
import { mapHelpers } from "../../../shared/utils/mapHelpers.ts";
import type { Vec2 } from "../../../shared/utils/v2.ts";
import { debugLines } from "../../src/debug/debugLines.ts";
import { type InputHandler, Key, MouseButton } from "../../src/input.ts";
import type { EditorContext } from "./editorContext.ts";
import type { EditorDisplay } from "./editorDisplay.ts";
import { MAP_DOC_LIMITS } from "./mapDocument.ts";
import { type HitCandidate, pickAll } from "./pick.ts";

interface ColliderLike {
    type: string;
    pos: Vec2;
    rot?: number;
    ori?: number;
    scale?: number;
}

/**
 * Mouse/keyboard tool layer: place, select, move, rotate, delete.
 *
 * Gesture model (one undo step per gesture):
 *  - the first mutation of a gesture calls `ctx.beginGesture()`;
 *  - the document is mutated directly for live feedback;
 *  - `ctx.endGesture()` rebuilds the world once, on mouse-up.
 *
 * Hotkeys match the web client's own single-letter binds (`inputBinds.ts`) only
 * where they do not collide with an editor action. Actions that would otherwise
 * be letter keys are also exposed as buttons in the UI pane.
 */
export class EditorTools {
    /** Live object ids of the dragged selection, resolved through the pool. */
    private dragObjIds: number[] = [];
    private dragStartPos = { x: 0, y: 0 };
    private dragMoved = false;

    private mouseDownWorld = { x: 0, y: 0 };

    /** Status line, shown in the UI */
    status = "";

    constructor(
        public ctx: EditorContext,
        public display: EditorDisplay,
        public input: InputHandler,
    ) {
        // Expose the live hit test to the UI (status line) without the UI having
        // to know how to reach into the render pools.
        this.ctx.getHover = () => pickAll(this.display, this.ctx.cursor, this.activeLayer)[0];

        this.display.onUpdate = () => this.m_update();
        this.display.drawOverlays = () => this.drawOverlays();
    }

    get pointerWorld() {
        return this.display.camera.m_screenToPoint(this.input.mousePos);
    }

    /** Objects are authored on the layer the editor player is currently on. */
    get activeLayer() {
        return this.display.config.get("mapEditor")!.layer;
    }

    m_update() {
        this.ctx.cursor = this.pointerWorld;
        const overUi = this.isPointerOverUi();

        this.handleShortcuts(overUi);
        this.handlePlacement(overUi);
        this.handleDrag(overUi);
    }

    //
    // Keyboard
    //

    private handleShortcuts(overUi: boolean) {
        const input = this.input;
        // While the cursor is over a pane the user is typing in an input box, so
        // single letter shortcuts must not fire.
        if (overUi) return;

        const ctrl = input.keyDown(Key.Control);
        const shift = input.keyDown(Key.Shift);

        // Undo / redo. These live above the placement picker button so an
        // accidental Shift+R does not rotate.
        if (ctrl && !shift && input.keyPressed(Key.Z)) {
            this.ctx.undo();
            return;
        }
        // Ctrl+Shift+Z is the other common redo, next to Ctrl+Y.
        if (ctrl && shift && input.keyPressed(Key.Z)) {
            this.ctx.redo();
            return;
        }
        if (ctrl && input.keyPressed(Key.Y)) {
            this.ctx.redo();
            return;
        }

        // Ctrl/Shift hold repeats: picking a nudge/scale is a per-press action,
        // so everything below uses `keyPressed`.
        if (ctrl && input.keyPressed(Key.D)) {
            this.ctx.duplicateSelection();
            this.status = "Duplicated selection";
            return;
        }
        if (ctrl && input.keyPressed(Key.C)) {
            this.ctx.copySelection();
            return;
        }
        if (ctrl && input.keyPressed(Key.V)) {
            this.ctx.pasteClipboard();
            return;
        }

        // Delete
        if (input.keyPressed(Key.Delete)) {
            this.ctx.deleteSelection();
            this.status = "Deleted selection";
            return;
        }

        // Rotate / flip / scale
        if (!ctrl && input.keyPressed(Key.R)) {
            this.ctx.rotateSelection(1);
            this.status = "Rotated 90 degrees";
        }
        if (!ctrl && input.keyPressed(Key.F)) {
            this.ctx.flipSelection();
        }
        if (!ctrl && input.keyPressed(Key.Minus)) {
            this.ctx.scaleSelection(1 / 1.1);
        }
        if (!ctrl && input.keyPressed(Key.Equal)) {
            this.ctx.scaleSelection(1.1);
        }

        // Camera jump to selection
        if (input.keyPressed(Key.Backquote)) {
            this.ctx.focusSelection();
        }

        // Deselect
        if (input.keyPressed(Key.Escape)) {
            this.ctx.setSelection([]);
        }

        // Arrow keys nudge the selection. Ctrl/Shift hold repeats: nudging is
        // continuous while the key is down.
        const step = ctrl || shift ? 5 : 0.5;
        if (input.keyDown(Key.Left)) this.ctx.nudgeSelection(-step, 0);
        if (input.keyDown(Key.Right)) this.ctx.nudgeSelection(step, 0);
        if (input.keyDown(Key.Up)) this.ctx.nudgeSelection(0, step);
        if (input.keyDown(Key.Down)) this.ctx.nudgeSelection(0, -step);
    }

    //
    // Placement
    //

    private handlePlacement(overUi: boolean) {
        const input = this.input;
        const config = this.display.config.get("mapEditor")!;

        const placeRequested = input.keyPressed(Key.P) && !input.keyDown(Key.Control);
        if (!placeRequested || overUi) return;

        if (!config.object || !MapObjectDefs.typeExists(config.object)) {
            this.ctx.onNotice(`Unknown object type "${config.object}"`);
            return;
        }
        if (this.ctx.doc.objects.length >= MAP_DOC_LIMITS.maxObjects) {
            this.ctx.onNotice("Document object limit reached");
            return;
        }

        const pos = this.pointerWorld;
        this.ctx.addEntry({
            type: config.object,
            pos: {
                x: Math.round(pos.x * 10) / 10,
                y: Math.round(pos.y * 10) / 10,
            },
            ori: config.ori,
            scale: 1,
            layer: this.activeLayer,
        });
        this.status = `Placed ${config.object}`;
    }

    //
    // Selection + drag
    //

    private handleDrag(overUi: boolean) {
        const input = this.input;

        if (input.mousePressed(MouseButton.Left) && !overUi) {
            const world = this.pointerWorld;
            this.mouseDownWorld = { x: world.x, y: world.y };

            const hits = pickAll(this.display, world, this.activeLayer);
            if (hits.length) {
                const candidate = this.chooseCandidate(hits);
                if (input.keyDown(Key.Shift)) {
                    // Shift-click only extends the selection; it must not start
                    // a drag, or the drag would move everything under the cursor
                    // that the user merely meant to add.
                    this.ctx.toggleSelection(candidate.docIndex);
                } else {
                    if (!this.ctx.selection.includes(candidate.docIndex)) {
                        this.ctx.setSelection([candidate.docIndex]);
                    }
                    this.startDrag(candidate);
                }
            } else if (!input.keyDown(Key.Shift)) {
                this.ctx.setSelection([]);
            }
        }

        if (input.mouseDown(MouseButton.Left) && this.dragObjIds.length) {
            const world = this.pointerWorld;
            const dx = Math.round((world.x - this.mouseDownWorld.x) * 10) / 10;
            const dy = Math.round((world.y - this.mouseDownWorld.y) * 10) / 10;
            if (dx !== 0 || dy !== 0) {
                this.dragMoved = true;
                this.ctx.beginGesture();
                this.moveDraggedTo({
                    x: this.dragStartPos.x + dx,
                    y: this.dragStartPos.y + dy,
                });
            }
        }

        if (input.mouseReleased(MouseButton.Left) && this.dragObjIds.length) {
            this.ctx.endGesture(true);
            this.status = this.dragMoved ? "Moved selection" : this.status;
            this.dragObjIds = [];
            this.dragMoved = false;
        }
    }

    /** Repeated clicking at the same spot cycles through overlapping objects. */
    private chooseCandidate(hits: HitCandidate[]): HitCandidate {
        const selectedIdx = this.ctx.selection[0];
        if (selectedIdx === undefined || hits.length < 2) return hits[0];
        const at = hits.findIndex((h) => h.docIndex === selectedIdx);
        if (at < 0) return hits[0];
        return hits[(at + 1) % hits.length];
    }

    private startDrag(candidate: HitCandidate) {
        this.dragObjIds = [];
        for (const i of this.ctx.selection) {
            const objId = this.display.m_docIndexToObjId[i];
            if (objId !== undefined && objId >= 0) {
                this.dragObjIds.push(objId);
            }
        }
        if (!this.dragObjIds.includes(candidate.objId)) {
            this.dragObjIds.push(candidate.objId);
        }

        const entry = this.ctx.doc.objects[candidate.docIndex];
        this.dragStartPos = entry ? { ...entry.pos } : { ...candidate.pos };
        this.dragMoved = false;

        const world = this.pointerWorld;
        this.mouseDownWorld = { x: world.x, y: world.y };
    }

    /**
     * Moves the object the drag started on to `target` and applies the same
     * delta to the rest of the selection. Live objects are moved in place so the
     * preview is instant and no rebuild is needed mid-drag.
     */
    private moveDraggedTo(target: { x: number; y: number }) {
        // `startDrag` guarantees the dragged object is itself in the selection,
        // so moving the whole selection moves it too.
        const dragIndex = this.display.m_objIdToDocIndex.get(this.dragObjIds[0]);
        if (dragIndex === undefined) return;

        const deltaX = target.x - this.dragStartPos.x;
        const deltaY = target.y - this.dragStartPos.y;

        for (const i of this.ctx.selection) {
            const entry = this.ctx.doc.objects[i];
            if (!entry) continue;
            // The dragged entry keeps the cursor delta exactly; the others keep
            // their relative offset from it.
            const base = i === dragIndex ? this.dragStartPos : entry.pos;
            entry.pos = {
                x: Math.round((base.x + deltaX) * 10) / 10,
                y: Math.round((base.y + deltaY) * 10) / 10,
            };
        }

        this.ctx.previewSelectionPositions();
    }

    //
    // Overlays
    //

    drawOverlays() {
        const config = this.display.config.get("mapEditor")!;

        if (config.showBounds) {
            for (let i = 0; i < this.ctx.doc.objects.length; i++) {
                const obj = this.liveObj(i);
                if (!obj) continue;
                const col = boundingColliderOf(obj);
                if (col) debugLines.addCollider(col, 0x0088ff, 0.0);
            }
        }

        for (const i of this.ctx.selection) {
            const obj = this.liveObj(i);
            if (!obj) continue;
            const col = boundingColliderOf(obj);
            if (col) debugLines.addCollider(col, 0x00ff00, 0.0);
        }

        const hover = pickAll(this.display, this.ctx.cursor, this.activeLayer)[0];
        if (hover) {
            debugLines.addCollider(hover.collider, 0xffff00, 0.05);
        }

        // Placement ghost
        if (config.object && MapObjectDefs.typeExists(config.object)) {
            const def = MapObjectDefs.typeToDef(config.object);
            if (def.type === "obstacle" || def.type === "building" || def.type === "structure") {
                const col = collider.transform(
                    mapHelpers.getBoundingCollider(config.object),
                    this.ctx.cursor,
                    (config.ori * Math.PI) / 2,
                    1,
                );
                debugLines.addCollider(col, 0xff00ff, 0.0);
            }
        }
    }

    private liveObj(docIndex: number) {
        const objId = this.display.m_docIndexToObjId[docIndex];
        if (objId === undefined) return undefined;
        return this.display.objectCreator.m_getObjById(objId);
    }

    isPointerOverUi() {
        const pos = this.input.mousePos;
        const target = document.elementFromPoint(pos.x, pos.y);
        return !!target?.closest("#editor-ui, #editor-side-ui, .tp-dfwv");
    }
}

function boundingColliderOf(obj: ColliderLike) {
    try {
        const rot = obj.rot !== undefined ? obj.rot : ((obj.ori ?? 0) * Math.PI) / 2;
        return collider.transform(
            mapHelpers.getBoundingCollider(obj.type),
            obj.pos,
            rot,
            obj.scale ?? 1,
        );
    } catch (_e) {
        return null;
    }
}
