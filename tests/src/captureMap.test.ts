import { describe, expect, test } from "vitest";
import { GameMap } from "../../server/src/game/map.ts";
import { captureMapDoc, generateMapDoc, type MapDocDocument } from "../../server/src/mapdoc/captureMap.ts";
import { MapObjectDefs } from "../../shared/defs/register.ts";
import { v2 } from "../../shared/utils/v2.ts";

/** The smallest `Game`-shaped object `captureMapDoc` reads. */
function fakeGame(map: GameMap, decals: unknown[] = []) {
    return { map, decalBarn: { decals }, config: { mapName: "main" } } as never;
}

function fakeMap(overrides: Partial<GameMap> = {}) {
    const map = Object.create(GameMap.prototype) as GameMap;
    map.width = 128;
    map.height = 128;
    map.shoreInset = 8;
    map.grassInset = 4;
    map.seed = 4242;
    map.mapDef = { mapId: 3, mapGen: { densitySpawns: [{}] } } as never;
    map.obstacles = [];
    map.buildings = [];
    map.structures = [];
    map.msg = { rivers: [], places: [], groundPatches: [] } as never;
    Object.assign(map, overrides);
    return map;
}

function captured(map: GameMap, decals: unknown[] = []) {
    return captureMapDoc(fakeGame(map, decals));
}

describe("captureMap", () => {
    test("captures a real generated map as a valid document", () => {
        const doc = generateMapDoc({ mapName: "main", seed: 218051654 });

        expect(doc.version).toBe(1);
        // `generator` is informational; the editor's `normalizeDoc` ignores it.
        const generator = (doc as { generator?: string }).generator;
        expect(generator).toBe("survev-capture-map");
        expect(doc.mapName).toBe("main");
        expect(doc.seed).toBe(218051654);
        expect(doc.width).toBe(720);
        expect(doc.height).toBe(720);
        expect(doc.shoreInset).toBe(48);
        expect(doc.grassInset).toBe(18);
        expect(doc.rivers.length).toBeGreaterThan(0);
        expect(doc.places.length).toBe(8);
        expect(doc.groundPatches.length).toBeGreaterThan(0);
        expect(doc.objects.length).toBeGreaterThan(0);
    });

    test("object entries round-trip through MapObjectDefs", () => {
        const doc = generateMapDoc({ mapName: "main", seed: 1234567 });

        for (const entry of doc.objects) {
            expect(MapObjectDefs.typeExists(entry.type), entry.type).toBe(true);
            expect(entry.ori).toBeGreaterThanOrEqual(0);
            expect(entry.ori).toBeLessThanOrEqual(3);
            expect(entry.layer === 0 || entry.layer === 1).toBe(true);
            expect(Number.isFinite(entry.pos.x)).toBe(true);
            expect(Number.isFinite(entry.pos.y)).toBe(true);
            expect(entry.scale).toBeGreaterThan(0);
        }
    });

    test("only authors top level objects", () => {
        const doc = captured(
            fakeMap({
                obstacles: [
                    { type: "stone_01", pos: v2.create(2, 2), ori: 0, rot: 0, scale: 1, layer: 0 },
                    {
                        // child obstacle: recreated by its parent building
                        type: "stone_01",
                        pos: v2.create(3, 3),
                        ori: 0,
                        rot: 0,
                        scale: 1,
                        layer: 0,
                        parentBuildingId: 7,
                    },
                ] as never,
                buildings: [
                    { type: "house_red_01", pos: v2.create(10, 10), ori: 2, layer: 0 } as never,
                    // structure floor: recreated by the structure
                    {
                        type: "house_red_01",
                        pos: v2.create(11, 11),
                        ori: 0,
                        layer: 0,
                        parentStructure: {},
                    } as never,
                ],
                structures: [
                    { type: "bunker_structure_01", pos: v2.create(20, 20), ori: 1 } as never,
                ] as never,
            }),
        );

        expect(doc.objects.map((entry) => entry.type)).toStrictEqual([
            "stone_01",
            "house_red_01",
            "bunker_structure_01",
        ]);
        // Child obstacles/buildings/structures are skipped, so exactly one
        // entry per authoured object survives.
        expect(doc.objects).toHaveLength(3);
        // ori/scale are carried across.
        expect(doc.objects[0].scale).toBe(1);
        expect(doc.objects[1].ori).toBe(2);
        expect(doc.objects[2].ori).toBe(1);
    });

    test("skips density spawns and stair side layers", () => {
        const map = fakeMap({
            obstacles: [
                // authorable
                { type: "stone_01", pos: v2.create(2, 2), ori: 0, scale: 1, layer: 0 },
                // density spawns are regenerated from the map def
                { type: "stone_03", pos: v2.create(4, 4), ori: 0, scale: 1, layer: 0 },
                // door on the up-stairs layer folds down to the ground layer
                { type: "house_door_01", pos: v2.create(5, 5), ori: 0, scale: 1, layer: 2 },
            ] as never,
        });
        map.mapDef = { mapId: 3, mapGen: { densitySpawns: [{ stone_03: 100 }] } } as never;

        const doc = captured(map);

        expect(doc.objects.map((entry) => entry.type)).toStrictEqual([
            "stone_01",
            "house_door_01",
        ]);
        expect(doc.objects[1].layer).toBe(0);
    });

    test("skips fading decals", () => {
        const doc = captured(fakeMap(), [
            // persistent vent decal: authorable
            { type: "decal_vent_01", pos: v2.create(1, 1), ori: 0, scale: 1, layer: 0 },
            // lights fade out, so they cannot be authored
            { type: "decal_light_01", pos: v2.create(2, 2), ori: 1, scale: 2, layer: 0 },
        ]);

        const types = doc.objects.map((entry) => entry.type);
        expect(types).toContain("decal_vent_01");
        expect(types).not.toContain("decal_light_01");
    });

    test("copies rivers, places and ground patches", () => {
        const message = {
            rivers: [{ width: 8, looped: false, points: [v2.create(1, 2), v2.create(3, 4)] }],
            places: [{ name: "Alpha", pos: v2.create(0.25, 0.75) }],
            groundPatches: [
                {
                    bound: { type: 1, min: v2.create(1, 1), max: v2.create(2, 2) },
                    color: 0x123456,
                    roughness: 0.5,
                    offsetDist: 0.25,
                },
            ],
        };
        const doc = captured(fakeMap({ msg: message as never }));

        expect(doc.rivers).toStrictEqual([
            { width: 8, looped: false, points: [v2.create(1, 2), v2.create(3, 4)] },
        ]);
        expect(doc.places).toStrictEqual([{ name: "Alpha", pos: v2.create(0.25, 0.75) }]);
        expect(doc.groundPatches).toHaveLength(1);
        expect(doc.groundPatches[0]).toStrictEqual({
            bound: { type: 1, min: v2.create(1, 1), max: v2.create(2, 2) },
            color: 0x123456,
            roughness: 0.5,
            offsetDist: 0.25,
            order: 0,
            useAsMapShape: true,
        });
    });

    test("uses the configured map key, not mapId", () => {
        const map = fakeMap();
        const doc = captureMapDoc({
            map,
            decalBarn: { decals: [] },
            config: { mapName: "desert" },
        } as never);

        expect(doc.mapName).toBe("desert");
    });

    test("document is JSON safe", () => {
        const doc: MapDocDocument = generateMapDoc({ mapName: "main", seed: 999 });
        expect(() => JSON.parse(JSON.stringify(doc))).not.toThrow();
    });
});
