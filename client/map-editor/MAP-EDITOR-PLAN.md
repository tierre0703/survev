# Map Editor — implementation plan

Status: **implemented (phases 1–6).** Target directory: `client/map-editor/`.
Derived from a read of `client/building-editor/`, `client/src/{map,config,main,input,objects/*}.ts`,
`shared/net/mapMsg.ts`, `shared/defs/*`, and `server/src/game/map.ts`.

> Implementation notes live in §10. Phase 7 (playing an authored map) is still
> deferred.
>
> **Local verification note:** the page boots and renders (terrain grid, tweakpane
> UI, editor player) when driven headlessly:
> `chrome --headless=new --screenshot=shot.png http://127.0.0.1:3000/map-editor/`.
> In the interactive browser on this machine it instead paints blank, because the
> Astrill `ASProxy64.dll` Winsock LSP resets keep-alive HTTP/1.1 responses partway
> through, so `editorUi.ts` (the largest editor module) never finishes loading.
> Node/Python/`curl` against the same URL get the full body, and
> `client/building-editor/` fails identically, so it is a host-wide proxy
> condition rather than an editor bug.
>
> Because that hides page errors from the interactive console, the headless
> screenshot is the smoke test for `EditorUi` — `dprint check`, `oxlint` and
> `tsc --noEmit` cannot see runtime API misuse such as the two bugs found this
> way: `editorDisplay.ts` rebuilding `m_objIdToDocIndex`, and `editorUi.ts` using
> tweakpane 3's `addMonitor` (removed in v4).
>
> Independently verified with `dprint check`, `oxlint`, `tsc --noEmit` in
> `client/` and `server/`, and `pnpm --filter @survev/tests test` (11 files,
> 11841 tests green).

## 1. Goal and scope

Build a **map editor** that can author a whole map and hand the result to the game, as opposed to
`client/building-editor/` which is an object/placement sandbox.

| | `building-editor` (exists) | `map-editor` (this plan) |
|---|---|---|
| Scene | one object at the map centre | a list of placed objects + terrain config |
| Persistence | none (only camera/zoom in `localStorage`) | save/load/export authored map JSON |
| Map size / seed / rivers / places | hardcoded 720x720, fixed seed | editable |
| Object manipulation | spawn only, no selection/delete | select, move, rotate, delete, undo |
| Gameplay | cannot be played | exports a def the server can generate from |

**Non-goal (phase 1):** shipping authored maps into the live game server. That needs server-side
generation changes (§7) and is tracked as a separate phase.

## 2. Why not just extend `building-editor`

- `building-editor/src/editorDisplay.ts` resets the world on every config change (`resetObj()`),
  which destroys all placed objects. It has no model of "the user's map" separate from
  "the object currently being previewed".
- `mapMsg` in that class is a *scratch* `MapMsg` used to feed `Map.loadMap`; `mapMsg.objects` is
  only ever appended to as a side effect of `addBuilding`, and never read back.
- `BuildingEditorConfig` (`client/src/config.ts:86`) is persisted to `localStorage` via
  `ConfigManager`, so a full map document must **not** live there (size, and it would be sent
  through `runOnConfigModified` on every edit).

So: same visual stack, new document model, new UI. Reuse the render plumbing, do not fork it.

## 3. Reusable pieces (verified)

| Need | Where it already exists |
|---|---|
| Full client render pipeline without networking | `building-editor/src/editorDisplay.ts` `init()` / `update()` / `render()` |
| Object spawn for every def kind | `editorDisplay.addAuto/addBuilding/addStructure/addObstacle/addDecal/addLootspawner` |
| Deterministic object ids | `Creator.m_updateObjFull/m_updateObjPart/m_deleteObj` (`client/src/objects/objectPool.ts`) |
| Terrain + rivers from a descriptor | `Map.loadMap(mapMsg, camera, canvasMode, particleBarn)` (`client/src/map.ts:196`) |
| Terrain/mesh generation | `generateTerrain(w,h,shoreInset,grassInset,rivers,seed)` (`shared/utils/terrainGen.ts:75`) |
| Serialisable map document | `MapMsg` (`shared/net/mapMsg.ts:95`) — has `serialize`/`deserialize` |
| Wire-safe bounds | `mapMsg` field limits: `width`/`height` are `uint16`, `objects` array max 16, `groundPatches` max 8, `rivers` max 8, `places` max 8 |
| Object type enumeration for pickers | `MapObjectDefs.getAllTypes()` + `typeToDef(type, defType)` (`shared/defs/register.ts`) |
| Input polling | `InputHandler` (`client/src/input.ts`) — `keys`, `mousePos`, `mouseButtons`, `mouseWheelState`, `flush()` |
| Minimap rendering | `Map.renderMap(renderer, canvasMode)` (`client/src/map.ts:529`) |

### Pool accessors needed for an object list

The render pools are `Map.m_obstaclePool`, `Map.m_buildingPool`, `Map.m_structurePool`,
`DecalBarn.decalPool`, `LootBarn.lootPool`. Each `Pool` exposes `m_getPool()` and each object
carries `active`, `__id`, `__type`, `type`, `pos`, `ori`/`rot`, `layer`, `scale`:

- `Obstacle` (`client/src/objects/obstacle.ts:30`): `type`, `pos`, `rot`, `scale`, `layer`, `isWall`
- `Building` (`client/src/objects/building.ts:49`): `type`, `pos`, `ori`, `rot`, `scale`, `layer`, `zIdx`
- `Structure` (`client/src/objects/structure.ts:34`): `type`, `pos`, `rot`, `scale`
- `Decal` (`client/src/objects/decal.ts:23`) — **not exported**, only reachable through the pool
- `Loot` (`client/src/objects/loot.ts:24`): `type`, `pos`, `layer`, `count`

These are sufficient to round-trip an authored scene back out to a `MapMsg`.

## 4. Document model

New file `client/map-editor/src/mapDocument.ts`. Keep it **shared-safe** (no PIXI, no DOM) so the
server can consume it later.

```ts
export interface MapObjectEntry {
    type: string;          // MapObjectDefs key
    pos: Vec2;
    ori: number;
    scale: number;
    layer: number;
}

export interface MapDoc {
    version: 1;
    mapName: MapDefKey;     // picks biome/assets/lootTable
    seed: number;
    width: number;          // uint16 on the wire
    height: number;         // uint16 on the wire
    shoreInset: number;
    grassInset: number;
    rivers: MapRiverData[]; // shared/utils/terrainGen.ts
    places: Array<{ name: string; pos: Vec2 }>;
    objects: MapObjectEntry[];
}
```

Design decisions:

- **One flat object list**, not the recursive structure/building hierarchy the renderer builds.
  The renderer expands `Structure.layers` and `Building.mapObjects` itself; the document only needs
  the top-level entry. `editorDisplay.addAuto` already dispatches by `MapObjectDefs.typeToDef` kind,
  so loading a doc is `for (entry of doc.objects) addAuto(entry.type, ...)`.
- **`scale` default 1**, `ori` 0..3, `layer` 0 or 1 (buildings are layer-agnostic).
- **Ground patches are derived, not authored.** `addBuilding` already emits `mapGroundPatches` into
  `mapMsg.groundPatches`; on save, collect that array rather than storing it in the doc. This keeps
  the doc small and avoids the `groundPatches` wire cap of 8.

## 5. File layout

```
client/map-editor/
├── index.html                    # copy of building-editor/index.html, title "Map Editor"
├── src/
│   ├── main.ts                   # Application bootstrap (copy building-editor/src/main.ts)
│   ├── mapDocument.ts            # MapDoc type + validate/upgrade
│   ├── editorContext.ts          # EditorContext: display + document + history (no DOM)
│   ├── editorDisplay.ts          # editorContext-aware fork of building-editor's display
│   ├── tools.ts                  # placement / select / move / delete tools
│   ├── history.ts                # undo/redo stack of MapDoc snapshots
│   ├── io.ts                     # toMapDoc / fromMapDoc / export / import
│   ├── editorUi.ts               # tweakpane UI (extends building-editor's)
│   └── editor.css                # copy of building-editor/src/editor.css
```

### `vite.config.mts` change

The building editor is registered only in dev (`vite.config.mts:85-92`). Add `map-editor` in the
same `isDev ? {...} : {}` block. Dev URL: `http://127.0.0.1:3000/map-editor/`.

## 6. Implementation phases

### Phase 1 — scaffold that renders (smallest useful step)

1. Copy `building-editor/{index.html,editor.css}` and `src/main.ts`, retitling to Map Editor.
2. Add `MapEditorConfig` to `client/src/config.ts` next to `BuildingEditorConfig` (zoom, pos, layer,
   map, hideCeilings, showGrid) — **document data stays out of `ConfigManager`**.
3. Add the dev-only `map-editor` entry to `client/vite.config.mts`.
4. Verify it boots and renders an empty `main` map.

> **Known trap:** in `building-editor/src/main.ts` the map is only created because
> `EditorDisplay.init()` calls `resetObj()`. In the new editor, terrain must be loaded by the
> document loader instead, or the first paint is a blank canvas. Confirm visually before building
> the UI.

### Phase 2 — document + object list

1. `MapDoc` type and a default document derived from `Main.mapGen` (`shared/defs/maps/baseDefs.ts:813`).
2. `history.ts`: snapshot-based undo/redo, cap depth (e.g. 100) and store `structuredClone(doc)`.
3. `editorContext.ts`: owns `doc`, `history`, and a `rebuild()` that clears pools and replays the doc.
4. `editorDisplay.ts`: keep `init/update/render/resize`, but replace `resetObj()` with
   `loadDocument(doc)` — build a `MapMsg` from the doc, `map.loadMap(...)`, `clearAllObjs()`, then
   `addAuto` each entry, then re-render terrain and `renderer.resize(...)` (mirror
   `editorDisplay.resetObj()` lines 573-581).
5. Wire UI actions (spawn / delete / clear) through the context so undo sees them.

### Phase 3 — manipulation tools

1. Hit-testing: the pattern already exists in `server/src/game/objects/player.ts:2350`
   (`moveObjUpdate`) — grid intersect via `mapHelpers.getBoundingCollider(type)` → `collider.toAabb`
   → `coldet.testPointAabb`. Port that read-only for the client; the client has no `grid`, so
   iterate the pools and test each object's collider.
2. Select tool: click selects the top-most object at the cursor; shift-click extends.
3. Move tool: drag updates `entry.pos`; call `Creator.m_updateObjPart` for a live preview, commit to
   the doc on mouse-up (single undo step).
4. Rotate (R) / delete (Del) / duplicate (Ctrl+D) operate on the selection.
5. Highlight overlay: reuse `debugLines`/`debugDisplay` from `editorDisplay.render()`.

### Phase 4 — map-level parameters

Expose the fields the `MapMsg` wire format actually supports:
`width`, `height`, `seed`, `shoreInset`, `grassInset`, `mapName`, and `places`.
Width/height edits require a full `rebuild()`. Seed/river edits likewise. Note `Map.loadMap` calls
`generateTerrain(width, height, shoreInset, grassInset, rivers, seed)`, so the terrain is
reproducible from the doc alone — do not try to persist the generated polygons.

### Phase 5 — save / load / export

1. `io.ts`:
   - `fromDocument(display, doc): MapMsg` — walk pools, emit `MapObjectEntry[]`, attach rivers/places.
   - `toDocument(mapMsg): MapDoc`.
   - `exportJson(doc)` → `Blob` download `map-<name>-<seed>.json`.
   - `importJson(file)` → validate `version`, fill missing fields, `rebuild()`.
2. Autosave the last doc to `localStorage` under a **separate key** (e.g. `surviv_map_editor_doc`)
   with a size guard and a manual "clear" action — `ConfigManager` already owns `surviv_config`.
3. Optional: also emit a `MapMsg` binary via `mapMsg.serialize(new BitStream(...))` for inspection,
   but the JSON doc is the source of truth.

### Phase 6 — polish

Minimap preview (`Map.renderMap`), object search/filter in the picker (there are hundreds of
`MapObjectDefs` types; `building-editor/src/editorUi.ts` already filters out `loot_spawner` and
`isWall` obstacles), stats (object counts per type), and a "jump camera to selection" action.

## 7. Deferred: making an authored map playable

This is **not** part of `client/map-editor/` and should be a separate change. Key findings:

- `server/src/game/map.ts` generates everything from `MapDef.mapGen` (`customSpawnRules`,
  `densitySpawns`, `fixedSpawns`, `randomSpawns`, `places`, `rivers`). There is **no** code path that
  reads a pre-authored object list.
- `MapMsg` is a **network** message and `MapMsg.serialize` is wrapped in
  `STRIP_FROM_PROD_CLIENT:START/END` (`shared/net/mapMsg.ts:108`), so it is stripped from production
  client builds. `deserialize` remains. Do not build server → client map transfer on `serialize`
  without checking the strip blocks.
- `client/src/map.ts:202` resolves `mapMsg.mapName` → `MapDefs[mapName]` and only uses
  `mapMsg.{width,height,seed,shoreInset,grassInset,rivers,places,objects,groundPatches}`; places and
  objects from the message go into `map.mapData`, **not** back into the def.

A playable pipeline would need: a new `MapDef` field (or a separate authored-map registry) holding
the object list, a `Map.init()` branch that spawns from it instead of `mapGen`, and grid/bounds
registration identical to `genObstacle`/`genBuilding`. Fixtures in `tests/` assert map generation
(11831 passing tests as of this writing), so that change must keep `surv_tests`/vitest green.

## 8. Risks / gotchas

- **`Decal` is not exported** (`client/src/objects/decal.ts:23`). To serialise decals you must either
  export the class or read decals through a new accessor on `DecalBarn`.
- **`ObjectType` ids are stable** (`shared/net/objectSerializeFns.ts:5`) but the editor should key off
  `MapObjectDefs` types, not `ObjectType`.
- **`addAuto` applies `spawnReplacements`** from `mapDef.mapGen.spawnReplacements[0]`
  (`editorDisplay.addAuto:494`). For faithful authoring, pass `ignoreMapSpawnReplacement = true` when
  replaying a saved document, or the editor silently substitutes types.
- **`ConfigManager.store()` runs on every `set`**; never route per-frame edit state through it.
- **Dev-only registration.** `vite.config.mts` only includes the extra entry when `isDev`; a
  production build will not contain `map-editor`. Keep it that way unless asked otherwise.
- **Asset loading.** `ResourceManager` needs `loadMapAssets(mapName)` in addition to `loadMap`, and
  `building-editor` passes `basePath = "../"` because its HTML lives one directory deep.
- **Pool compacting.** `Pool.m_free` compacts when `m_pool.length > 128 && m_activeCount < length/2`
  (`client/src/objects/objectPool.ts:61`), so never hold raw pool object references across a
  rebuild — hold `__id`s and re-resolve through `Creator.m_getObjById`.

## 9. First commit checklist

- [x] `client/map-editor/{index.html,src/main.ts,src/editor.css}` scaffold boots at
      `http://127.0.0.1:3000/map-editor/` and renders terrain
- [x] `MapEditorConfig` added to `client/src/config.ts`
- [x] dev-only `map-editor` entry in `client/vite.config.mts`
- [x] `mapDocument.ts` with `MapDoc` + default doc from `Main.mapGen`
- [x] `pnpm lint` / `pnpm lint:ci` clean, `tsc --noEmit` clean in `client/` and `server/`
- [x] no changes outside `client/` and the two shared files listed above

## 10. Implementation notes

Files, in dependency order:

| File | Role |
|---|---|
| `src/mapDocument.ts` | `MapDoc`/`MapObjectEntry` types, `createDefaultDoc` (from `Main.mapGen`), `normalizeDoc` (never throws, clamps + drops junk), `MAP_DOC_LIMITS` |
| `src/history.ts` | snapshot undo/redo, capped at 100 `structuredClone`d docs |
| `src/editorDisplay.ts` | fork of the building editor's display; `loadDocument(doc)` replaces `resetObj()`; `m_docIndexToObjId` / `m_objIdToDocIndex`; `moveObj()` for live drags |
| `src/editorContext.ts` | owns the document, the selection and every mutation; `edit()` wraps a mutation in one undo step |
| `src/pick.ts` | pool iteration hit testing (no client grid); `objectToEntry()` converts a live object back to a document entry |
| `src/tools.ts` | place / select / drag / rotate / delete + overlay drawing |
| `src/io.ts` | `toMapMsg`, `toDocument`, JSON import/export, localStorage autosave, `summarize` |
| `src/editorUi.ts` | tweakpane UI (tools, map params, renderer, document, minimap) |
Verified with `pnpm lint:ci`, `tsc --noEmit` in `client/` and `server/`, and
`pnpm --filter @survev/tests test` (6 new tests in `tests/src/mapEditor.test.ts`).
`surv_tests` is unaffected: nothing under `shared/` changed.

### 10.1 Interaction fixes

Everything below was found by driving the module directly (a headless page plus a stub viewer that
mirrors `obstacle.render`/`decal.update`) rather than by reading the code. They are all in editor
plumbing or a latent decal bug the editor exposes, not in the document model. Item 3 is tracked in
§11 rather than fixed here; the rest are fixed, and the decal change is the only one outside
`client/map-editor/`.

1. **Decals vanished after the first rebuild.** `clearAllObjs` frees every decal in the same frame in
   which the replay allocates the replacements, and the pool hands objects back by allocation order,
   so a decal came back with its old `decalRender` still attached. `allocDecalRender` then handed that
   same render to another decal, and the freed decal drove *someone else's* sprite (or a detached one,
   which keeps painting at its last position). All decals disappeared.
   - `EditorDisplay.loadDocument` now drops `decalRender` on every pooled decal and clears
     `decalBarn.decalRenders` **before** clearing the pools.
   - `client/src/objects/decal.ts` moved the sprite update out of `DecalRender.update` (which the barn
     reached through the decal's stale `decalRender` reference on `Decal.update`, i.e. *not at all*
     for the freed decal) onto `Decal.updateRender`, driven once per decal by `DecalBarn.m_update`.
     A decal whose render has been reclaimed now deactivates itself instead of writing through it.
     The move is behaviour preserving (same transform, same order of `addPIXIObj` calls) and closes a
     latent bug that live play could hit if a decal were recreated before the next `m_update`.
2. **A moved player made the whole map vanish.** `camera.m_interpInterval` stays 0 without network
   updates, and `Player.m_update` divides by it (`posInterpTicker / 0` -> `NaN`), which poisons the
   renderer's z-sort. `EditorDisplay.init` sets it to a 1-second dummy interval, as the real client
   does from the update interval.
3. **Type substitution on rebuild.** `addAuto` rolls `spawnReplacements` (and defs may roll
   `mapObjects[].type` weights); the replay passes `ignoreMapSpawnReplacement`, but a def that picks
   its own children randomly still re-rolls. Documents are authored against one concrete object, so
   the editor should pin what it spawned; unverified until the deviation is handled, see §11.
4. **Clicking selected the wrong thing.** Picking was a `toAabb` test against
   `mapHelpers.getBoundingCollider`, i.e. the full building bounds: aiming at the ground next to a wall
   selected the wall, aiming under a door selected the building. `src/pick.ts` now scores candidates
   against the drawn sprite box (`getImgWidth`/`getImgHeight` / `img.scale` / object scale, which is
   what `renderer.m_pixels(scale * imgScale)` actually draws) and only falls back to the collider for
   structures/buildings (no sprite of their own) and for circle-collision obstacles (whose sprite
   overhangs the collider). A hovered type is shown in the status line.
5. **Shift+click started a drag** that moved the object the user was only adding to the selection.
6. **Shift+click could not extend onto an object the cursor covered.** `chooseCandidate` cycled through
   hits only when the previously selected object was under the cursor, so clicking the object that
   already covered it just re-selected it. Cycling is disabled for shift-clicks.
7. **Autosave ran once per drag frame.** `EditorContext.autosave` is now a no-op inside a gesture; the
   write happens on `endGesture`.
8. **The camera could end up off-map after a rebuild** because `ConfigManager`'s stored `pos` is
   relative to the map centre and is only clamped when the size changes. `loadDocument` re-derives the
   player/camera position from the document on every load (`updatePlayer`).
9. **`MapMsg.mapName` was untyped** in `EditorDisplay`; it is a `MapDefKey` now, no behaviour change.
10. **`Key.Equal`/`Key.Backquote`** were added as explicit aliases of `Key.Plus`/`Key.Tilde` (same
    keyCodes 187/192) so `-`/`=`, `` ` `` and Shift-click work without new duplicate-enum warnings.

Deviations from the original plan, and why:

1. **Document data stays out of `ConfigManager`** (as planned), but the editor also does not
   round-trip through `ConfigManager.set("mapEditor", ...)` for the document. Only camera/zoom/
   picker state lives there.
2. **`Decal` is still not exported.** Hit testing reads decals through
   `DecalBarn.decalPool.m_getPool()` and `objectToEntry` handles `ObjectType.Decal` without naming the
   class. `client/src/objects/decal.ts` did have to change — see §10.1 — but only inside `Decal` /
   `DecalRender` / `DecalBarn`.
3. **Changes outside `client/map-editor/`:** `client/src/input.ts` gained the missing `Key` enum
   members used by the tools (`Delete`, `Z`, `Y`, `F`, `E`, `Q`, `Equal`, `Backquote`, ...). No
   behaviour change. `client/src/objects/decal.ts` is the only other client file touched: §10.1 moved
   the decal's sprite update so a reclaimed `DecalRender` cannot hijack another decal's sprite.
4. **`ObjectType.Smoke` is intentionally not cleared on rebuild** (`clearAllObjs` mirrors the
   building editor's pool list). Smokes are transient effects.
5. **`Map.deadObstacleIds` / `deadCeilingIds` / `solvedPuzzleIds` are reset** at the start and after
   `loadMap` in `loadDocument`, otherwise a deleted obstacle would be re-spawned in the "dead" state
   on the next rebuild.
6. **`debugLines` overlays render unconditionally** in the editor (`EditorDisplay.render` calls
   `debugLines.m_render` regardless of `debugRenderer.enabled`), so selection/bounds outlines are
   always visible. `debugLines` itself is a no-op in production, which is fine: the editor is a
   dev-only entry.
7. **`addAuto(..., ignoreMapSpawnReplacement = true)`** is passed when replaying a document, so the
   editor never silently substitutes a different type.
8. **Ground patches are derived**, as planned: `addBuilding` fills `display.mapMsg.groundPatches`
   and `toMapMsg` copies that array out.
9. **`fromDocument(display, doc): MapMsg`** ended up as `io.toMapMsg`; it emits only top level
   entries that would be minimap markers (`def.map?.display` at layer 0), matching
   `server/src/game/map.ts#genObstacle`/`genBuilding`.

### Hotkeys

| Key | Action |
|---|---|
| Left click | select (Shift+click extends); drag moves the selection |
| Right drag | pan the camera |
| `P` | place the picker's object at the cursor |
| `R` / `F` | rotate 90 degrees / flip 180 degrees |
| `-` / `=` | scale down / up (obstacles and decals only) |
| `Del` | delete the selection |
| `Ctrl+D` / `Ctrl+C` / `Ctrl+V` | duplicate / copy / paste |
| `Ctrl+Z` / `Ctrl+Y` / `Ctrl+Shift+Z` | undo / redo |
| Arrows | nudge 0.5 (Ctrl or Shift: 5) |
| Wheel | zoom |
| `` ` `` | jump the camera to the selection (also a UI button) |
| `Esc` | deselect |

`building-editor`'s `Space`-to-switch-layers became a `Layer` dropdown (and `Space` now jumps to the
selection), because the browser scrolls on space.

## 11. Known deviation (not yet handled)

`addBuilding` (`editorDisplay.ts`) assigns building children randomly, exactly like
`server/src/game/map.ts#genBuilding` and `building-editor`. Several `MapObjectDefs` entries pick their
type from a weighted table (`mapObjects[].type` may be an object of weights) and
`MapObjectDefs.typeToDef(...).mapObjects` differs between sibling defs (`console_cache_05` spawns
`console_cache_05_collider`, so every rebuild can re-roll the sprite). A rebuild therefore does not
necessarily reproduce the layout the author saw.

`toMapMsg`/`MapDoc` only carry the top level entry, so the fix is to make the document pin the rolled
children once, e.g. store the derived child `MapObjectEntry[]` on the entry (a `children` field), or
resolve the roll with a seed derived from the entry's document index. Neither is implemented; the
editor currently reloads the same document into a potentially different arrangement.

### Verification

`tests/src/mapEditor.test.ts` covers `createDefaultDoc`/`normalizeDoc` (garbage input, clamping,
wire-format array caps, JSON round trip, unknown object types). The UI itself needs a running
`pnpm dev:client`; the module-level def access in `client/src/resources.ts` makes the editor page
unusable from a production build, which is why it stays a dev-only entry.

