import type { MapDoc } from "./mapDocument.ts";

/**
 * Snapshot based undo/redo.
 *
 * Documents are plain JSON-safe objects, so a snapshot is a `structuredClone`
 * of the whole doc. That is cheap enough for authoring-sized maps (< 10k
 * objects) and keeps every tool trivially correct — no inverse operations.
 */
export class History {
    past: MapDoc[] = [];
    future: MapDoc[] = [];

    constructor(public maxDepth = 100) {}

    get canUndo() {
        return this.past.length > 0;
    }

    get canRedo() {
        return this.future.length > 0;
    }

    /** Push the doc state *before* a mutation. */
    push(doc: MapDoc) {
        this.past.push(structuredClone(doc));
        if (this.past.length > this.maxDepth) {
            this.past.shift();
        }
        // Any new edit invalidates the redo branch
        this.future.length = 0;
    }

    /** Returns the state to restore, or null when there is nothing to undo. */
    undo(current: MapDoc): MapDoc | null {
        const prev = this.past.pop();
        if (!prev) return null;
        this.future.push(structuredClone(current));
        return prev;
    }

    /** Returns the state to restore, or null when there is nothing to redo. */
    redo(current: MapDoc): MapDoc | null {
        const next = this.future.pop();
        if (!next) return null;
        this.past.push(structuredClone(current));
        return next;
    }

    clear() {
        this.past.length = 0;
        this.future.length = 0;
    }
}
