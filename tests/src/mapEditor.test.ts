import { describe, expect, test } from "vitest";
import {
    createDefaultDoc,
    MAP_DOC_LIMITS,
    MAP_DOC_VERSION,
    normalizeDoc,
} from "../../client/map-editor/src/mapDocument.ts";

describe("mapDocument", () => {
    test("default document is derived from the main map def", () => {
        const doc = createDefaultDoc("main");

        expect(doc.version).toBe(MAP_DOC_VERSION);
        expect(doc.mapName).toBe("main");
        expect(doc.width).toBeGreaterThan(0);
        expect(doc.height).toBeGreaterThan(0);
        expect(doc.shoreInset).toBeGreaterThan(0);
        expect(doc.grassInset).toBeGreaterThan(0);
        // main authors the standard 8 places
        expect(doc.places.length).toBe(8);
        expect(doc.places[0].name).toBe("The Killpit");
        expect(doc.objects).toStrictEqual([]);
        expect(doc.rivers).toStrictEqual([]);
    });

    test("normalizeDoc fills in missing fields from a partial document", () => {
        const doc = normalizeDoc({ version: 1, mapName: "main", width: 512, height: 512 });

        expect(doc.width).toBe(512);
        expect(doc.height).toBe(512);
        expect(doc.seed).toBeGreaterThan(0);
        expect(doc.objects).toStrictEqual([]);
        expect(doc.rivers).toStrictEqual([]);
    });

    test("normalizeDoc never throws on garbage", () => {
        expect(() => normalizeDoc(null)).not.toThrow();
        expect(() => normalizeDoc(42)).not.toThrow();
        expect(() => normalizeDoc("nope")).not.toThrow();
        expect(normalizeDoc(null).objects).toStrictEqual([]);
    });

    test("normalizeDoc drops invalid object entries and clamps values", () => {
        const doc = normalizeDoc({
            mapName: "main",
            width: -5,
            height: 1e9,
            shoreInset: -10,
            grassInset: 1e9,
            seed: -1,
            objects: [
                { type: "crate_01", pos: { x: 5, y: 5 }, ori: 99, scale: 1e9, layer: 7 },
                { type: "", pos: { x: 5, y: 5 } },
                { pos: { x: 5, y: 5 } },
                null,
                "nope",
                { type: "tree_01", pos: { x: "a", y: "b" } },
            ],
        });

        expect(doc.objects).toHaveLength(2);

        const crate = doc.objects[0];
        expect(crate.type).toBe("crate_01");
        expect(crate.ori).toBe(3);
        expect(crate.scale).toBe(100);
        expect(crate.layer).toBe(1);

        // invalid pos falls back to origin
        const tree = doc.objects[1];
        expect(tree.pos).toStrictEqual({ x: 0, y: 0 });

        expect(doc.width).toBe(MAP_DOC_LIMITS.minSize);
        expect(doc.height).toBe(MAP_DOC_LIMITS.maxSize);
        expect(doc.shoreInset).toBe(0);
        expect(doc.grassInset).toBe(1024);
        expect(doc.seed).toBe(0);
    });

    test("normalizeDoc caps the wire-limited arrays", () => {
        const doc = normalizeDoc({
            mapName: "main",
            rivers: Array.from({ length: 32 }, () => {
                return {
                    width: 4,
                    looped: false,
                    points: [{ x: 0, y: 0 }],
                };
            }),
            places: Array.from({ length: 32 }, (_v, i) => {
                return { name: `p${i}`, pos: { x: 0.5, y: 0.5 } };
            }),
        });

        expect(doc.rivers).toHaveLength(MAP_DOC_LIMITS.maxRivers);
        expect(doc.places).toHaveLength(MAP_DOC_LIMITS.maxPlaces);
    });

    test("normalizeDoc round-trips a serialized document", () => {
        const original = createDefaultDoc("main");
        original.objects.push({
            type: "crate_01",
            pos: { x: 100, y: 200 },
            ori: 2,
            scale: 1.5,
            layer: 0,
        });
        original.seed = 42;

        const roundTripped = normalizeDoc(JSON.parse(JSON.stringify(original)));
        expect(roundTripped).toStrictEqual(original);
    });

    test("default document can be authored for any biome", () => {
        const doc = createDefaultDoc("savannah");
        expect(doc.mapName).toBe("savannah");
        // Insets still come from the biome's own mapGen
        expect(doc.shoreInset).toBeGreaterThan(0);
        expect(doc.grassInset).toBeGreaterThan(0);
    });

    test("normalizeDoc keeps the fallback biome when mapName is missing", () => {
        const doc = normalizeDoc({ width: 128 }, "desert");
        expect(doc.mapName).toBe("desert");
    });

    test("normalizeDoc preserves authored objects including a full river", () => {
        const doc = normalizeDoc({
            version: 1,
            mapName: "main",
            rivers: [
                {
                    width: 8,
                    looped: true,
                    points: [
                        { x: 10, y: 20 },
                        { x: 30, y: 40 },
                    ],
                },
            ],
            places: [{ name: "Alpha", pos: { x: 0.25, y: 0.75 } }],
            objects: [
                { type: "house_red_01", pos: { x: 5, y: 6 }, ori: 1, scale: 1, layer: 0 },
                { type: "tree_01", pos: { x: 7, y: 8 }, ori: 0, scale: 0.5, layer: 1 },
            ],
        });

        expect(doc.rivers).toStrictEqual([
            {
                width: 8,
                looped: true,
                points: [
                    { x: 10, y: 20 },
                    { x: 30, y: 40 },
                ],
            },
        ]);
        expect(doc.places).toStrictEqual([{ name: "Alpha", pos: { x: 0.25, y: 0.75 } }]);
        expect(doc.objects).toHaveLength(2);
        expect(doc.objects[0].type).toBe("house_red_01");
        expect(doc.objects[0].ori).toBe(1);
        expect(doc.objects[1].layer).toBe(1);
    });

    test("every default object entry is a real MapObjectDefs type", async () => {
        const { MapObjectDefs } = await import("../../shared/defs/register.ts");

        const doc = createDefaultDoc("main");
        doc.objects.push({
            type: "crate_01",
            pos: { x: 1, y: 1 },
            ori: 0,
            scale: 1,
            layer: 0,
        });

        for (const entry of doc.objects) {
            expect(
                MapObjectDefs.typeExists(entry.type),
                `unknown map object type ${entry.type}`,
            ).toBe(true);
        }
    });
});
