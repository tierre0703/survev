# Plan: Convert `client` to a native Android app with axmol

## 1. Overview

`client/` is the web version of Survev. It is a TypeScript + Vite single-page game
using **PIXI.js-legacy** for rendering, **jQuery/Bootstrap + EJS** for UI, **WebSocket**
for networking, and a large amount of shared logic in `shared/` (game defs, binary net
protocol, math/collision utilities).

This plan converts that client into a **native Android game built with axmol**
(C++ game engine, cocos2d-x successor). It covers toolchain setup, architecture mapping,
module-by-module porting, milestones, and risks. It is a **porting guide**, not a full
spec — exact API details should be verified against the installed axmol version.

> **Scale warning:** This is effectively a rewrite of the whole client into C++.
> Estimate is weeks-to-months of work for one developer. Treat it as a series of
> verifiable milestones rather than a single "big bang" port.

## 2. Current architecture (source of truth)

```
client/                     Web client (Vite + TypeScript)
  src/
    main.ts                 Application bootstrap, menu glue, game join/find
    game.ts                 Core Game class: sim + update loop + barns
    renderer.ts             PIXI renderer: z-sorted layers, masks, camera mapping
    camera.ts               Camera / view transform
    map.ts                  Map render + structure/collision rendering
    gas.ts                  Zone/gas simulation & drawing
    input.ts, inputBinds.ts Mouse + touch input, key/input binds
    audioManager.ts         Sound engine wrapper (createJS/Howler-style)
    soundDefs.ts            Sound definitions
    resources.ts            Texture/audio/resource loading
    api.ts, proxy.ts        HTTP API client, proxy/login detection
    account.ts              Player account / auth
    config.ts               ConfigManager (settings in localStorage)
    device.ts               Device/os detection
    ambiance.ts             Theme/map ambient audio + colors
    emote.ts, crosshair.ts, helpers.ts, errorLogs.ts, pingTest.ts, siteInfo.ts
    objects/                Barns + pooled objects (player, bullet, projectile,
                            loot, smoke, particles, explosion, structure, ...)
    ui/                     DOM UI: menu, loadout, team, profile, pass, HUD, touch
    sdk/                    SpellSync / ad SDK glue
  public/
    img/  audio/  fonts/  l10n/   Static assets, localization JSON
shared/                     Code shared with server (TypeScript, runs in both)
  lib/bitBuffer.ts          Binary bit-level reader/writer (net protocol)
  net/                      WebSocket connection + all game message classes
  utils/                    v2 math, math, collider/coldet, terrainGen, river,
                            spline, mapHelpers, loadout, ...
  defs/                     GameObjectDefs, MapDefs, map objects, puzzles
  gameConfig.ts             Game constants (protocolVersion, max players, ...)
  types/                    API / team / user / moderation types
```

Key traits that matter for porting:
- **Binary network protocol** built on `BitBuffer` (bit-level reads/writes). Must be
  bit-for-bit compatible with the server, so this is the single highest-priority port.
- **PIXI scene model**: `Graphics` (vector drawing), `Sprite`, `Container`, per-object
  `__zOrd`/`__zIdx` manual sorting, stencil masks for buildings, `tint`, blend modes.
- **DOM UI**: everything outside the canvas (menus, loadout, team, pass, HUD chrome) is
  HTML/jQuery/Bootstrap and heavily uses CSS. axmol UI widgets cannot replicate this 1:1.
- **Shared game data** (`shared/defs`) is written as TypeScript modules with computed
  values, and is consumed by server, client, and tests.

## 3. Target architecture (axmol)

- **Language:** C++17 (axmol requirement).
- **Rendering:** axmol `Scene`/`Node`/`Sprite`/`DrawNode` replacing PIXI. The existing
  z-sorting renderer maps naturally to axmol `Node::setLocalZOrder` / children ordering.
- **Game loop:** axmol `Director` tick replaces `PIXI.Ticker`; `Game::update(dt)` is called
  from a `Node`/`Layer` scheduled update (fixed or variable dt, same clamping logic).
- **Networking:** `ax::network::WebSocket` (raw binary frames) replacing browser `WebSocket`.
- **Audio:** `ax::AudioEngine` replacing `audioManager`/Howler.
- **HTTP/API:** `ax::network::HttpClient` (async) replacing `fetch`.
- **Settings:** `ax::UserDefault` (SharedPreferences-backed) replacing `localStorage`.
- **Assets:** bundled into APK `assets/`, loaded via `FileUtils`; existing `public/img`,
  `public/audio`, `public/fonts`, `public/l10n` reused as-is where formats match.
- **UI:** two options, see Decision D1. Either a WebView overlay for menus (fastest,
  reuses existing DOM UI) or a full axmol `ui::` widget port (largest effort).

## 4. Toolchain & project scaffolding

1. Clone axmol (`https://github.com/axmolengine/axmol`), run `setup.py` for Android
   (NDK, CMake, SDK paths). Confirm `axmol version` works.
2. Create the Android project from the axmol template (`axmol/templates/`) with
   package name derived from the web client (e.g. `com.survev.mobile`), language C++.
3. Build the template against Android Studio (SDK + NDK 27.x, CMake) and run on an
   emulator/device. **This is Milestone M0 — do not port code before the toolchain works.**
4. Structure to create under `client-mobile/`:

```
client-mobile/
  plan.md               this file
  CMakeLists.txt        top-level cmake (game lib + android/app)
  proj.android/         Android Studio project (Gradle + CMake)
  src/                  C++ game code
    app/                AppDelegate, Application (menu/lobby glue)
    game/               Game, GameObjects, barns, renderer, camera, map, gas
    net/                BitBuffer, Connection, message classes
    ui/                 axmol UI layer (or WebView host)
    assets/audio/       audio engine wrapper
    config/             ConfigManager, device, helpers
  assets/               copy/symlink of client/public assets + l10n
  shared/               C++ ports of shared/ (or generated)
  third_party/          (none expected; axmol bundled)
```

> Symlinking `client/public` into `assets/` avoids duplicating images/audio/l10n.
> `FileUtils::setSearchPaths` must account for Android asset packaging.

## 5. Module-by-module porting strategy

### 5.1 Shared core (highest priority, port first)
| TS source | C++ target | Notes |
|---|---|---|
| `shared/utils/v2.ts` | `Vec2` (or `ax::Vec2` + helpers) | Straightforward 1:1 |
| `shared/utils/math.ts` | `MathUtil` | clamp/lerp/rand — trivial |
| `shared/lib/bitBuffer.ts` | `BitBuffer` / `BitView` | **Bit-exact critical**. Copy semantics incl. little-endian bit ordering, UTF-8 string handling, float64 bit ops. Add unit tests against captured packets. |
| `shared/net/*.ts` | `net::*Msg` classes | One C++ class per message with `ReadFrom/WriteTo(BitBuffer)`. Keep field order exact. |
| `shared/net/connection.ts` | `WebsocketConnection` | axmol WebSocket, `Binary` data type, arraybuffer semantics |
| `shared/utils/coldet.ts`, `collider.ts`, `collisionHelpers.ts`, `earcut.js` | C++ equivalents | earcut is already a known C++ lib — vendor it |
| `shared/utils/terrainGen.ts`, `river.ts`, `spline.ts`, `mapHelpers.ts` | C++ | Needed for client-side terrain draw + map metadata |
| `shared/defs/*` | Static data | **Decision D2**: hand-port to C++ structs vs. generate C++ from TS via a build-time codegen script. |
| `shared/gameConfig.ts` | `GameConfig` constants | Mostly enums/ints/arrays — direct port |

### 5.2 Game simulation (port after net protocol)
- `game.ts` → `Game` (C++): connection handling, object pools (`objectPool.ts` →
  `Pool<T>` template), message dispatch, update order. The `Ctx` struct maps to a
  `GameContext` holding barn pointers.
- `objects/*` (player, bullet, projectile, loot, smoke, particles, explosion,
  structure, shot, deadBody, airdrop, plane, flare, decal, mapIndicator, mapSprite)
  → C++ classes. These are mostly pure logic + drawing; keep logic identical, replace
  only the draw calls.
- `gas.ts` → `Gas` (zone circles + damage ticks).

### 5.3 Rendering (PIXI → axmol)
| PIXI concept | axmol equivalent |
|---|---|
| `PIXI.Application` | `ax::Scene` created in `AppDelegate` |
| `PIXI.Container` / z-sorting (`__zOrd`,`__zIdx`) | `Node` + `setLocalZOrder` (or keep a custom `RenderGroup` that sorts children) |
| `PIXI.Graphics` (vector paths, `beginHole`) | `ax::DrawNode` (no true holes; use stencil `ClippingNode` for building masks) |
| `PIXI.Sprite` / texture atlas | `ax::Sprite` + `SpriteFrameCache` (atlas from `atlas-builder`) |
| Stencil mask for bunkers (`layerMask`) | `ClippingNode` stencil — verify performance; canvas fallback becomes `DrawNode` fill |
| `tint` / alpha / blend modes | `Sprite::setColor`/`setOpacity`, `BlendFunc` |
| `PIXI.Text` | `ax::Label` (BMFont or TTF, see fonts) |
| Renderer `resize`/`scaleToScreen` | `Director::getVisibleSize` + camera transform on a root node |

Implementation note: build a small **adapter layer** (`PixiLike.hpp`) exposing the
subset of PIXI calls the game actually uses (`moveTo/lineTo/drawRect/beginFill/...`),
then port renderer/map/objects against the adapter. This keeps ported draw code close to
the original TS and makes bugs easier to diff against the browser client.

### 5.4 Input
- `input.ts` → touch/mouse listeners via `EventListenerTouchOneByOne` +
  `EventListenerMouse`; replicate the same virtual joystick / aim dead-zone logic from
  `ui/touch.ts`.
- `inputBinds.ts` → config-driven key map (mostly irrelevant on touch, but keep for
  BT-keyboard/controller support).

### 5.5 Audio
- `audioManager.ts` / `soundDefs.ts` → `ax::AudioEngine` wrapper. Map preloaded sound
  defs to files; implement the same volume tiers (master/sound/music), mute toggle,
  and ambient loop switching (`ambiance.ts`).

### 5.6 UI (Decision D1)
- **Option A (recommended first): WebView overlay.** Keep the existing jQuery/EJS menus
  in a fullscreen `android.webkit.WebView` bridge, switch to the native axmol surface
  only for the in-game canvas. Pros: near-zero UI porting, reuses localization/DOM logic.
  Cons: two surfaces to keep in sync, WebView memory footprint.
- **Option B: Native axmol UI.** Port `ui/menu.ts`, `loadoutMenu`, `teamMenu`,
  `profileUi`, `pass`, HUD to `ax::ui::*`. Pros: single surface, best perf/consistency.
  Cons: very large effort; every modal/panel must be rebuilt and re-layouted for phones.

Either way, `localization.ts` (JSON `l10n/*.json`) must be loaded (via `FileUtils` +
a tiny JSON lib — axmol ships `rapidjson`).

### 5.7 API / account / proxy
- `api.ts` + `proxy.ts` + `account.ts` → `HttpClient`-based port. Decide whether login
  is in scope for the first release (accounts are optional server-side).
- `siteInfo.ts`, `pingTest.ts` → small ports (region list, latency checks).
- `sdk/` (SpellSync/ads) — **out of scope** unless explicitly required; it is browser SDK
  glue.

### 5.8 App lifecycle / Android specifics
- Handle `onPause/onResume` (axmol `AppDelegate::applicationDidEnterBackground` etc.):
  pause ticker, drop WebSocket gracefully, resume without desync (server replay/snapshot).
- Handle orientation + notch/insets; the game is portrait-hostile — lock to landscape
  (recommend `sensorLandscape`).
- Back button = in-game menu / confirmation, not app exit.
- Battery/perf: cap resolution by device class (`device.ts` logic), disable msaa,
  prefer power-of-two textures for the atlas.

## 6. Asset pipeline
- Textures: reuse `atlas-builder` output; convert `.png` atlases to axmol-compatible
  plist/`.png` (or use `SpriteFrameCache` with generated frames). `sharp`/`svgo`
  steps already exist.
- Fonts: Roboto Condensed `.woff2` cannot be used directly — embed TTF and register
  with `Label`, or pre-generate `.fnt` BMFont.
- Audio: verify formats supported by `ax::AudioEngine` (ogg/mp3/wav); transcode any
  `m4a`/`webm`.
- Localization: keep `l10n/*.json` as-is; rapidjson read.

## 7. Milestones (each ends with a runnable APK)

| # | Milestone | Deliverable / gate |
|---|---|---|
| M0 | Toolchain + axmol template builds | Hello-world axmol game on emulator + device |
| M1 | Asset pipeline | In-game placeholder scene draws atlas sprites, plays a sound, shows localized label |
| M2 | Shared core ports | BitBuffer + v2/math + message classes pass **unit tests** (port fixtures captured from web client) |
| M3 | Net + sim (headless) | Game connects to a dev server, receives update ticks, simulation state matches web client on same replay |
| M4 | Rendering port | Same scene as web client rendering through axmol (map, buildings, players, particles) |
| M5 | Input + touch controls | Virtual joystick + aim/fire, camera follow, player moves/shoots correctly |
| M6 | Audio + ambiance | SFX, gunshots, music, gas ambience correct |
| M7 | UI layer (D1 chosen) | Menu → loadout → find game → game → death flow works end-to-end |
| M8 | Account/API (optional) | Login/profile/team if kept in scope |
| M9 | Polish & release | Lifecycle, orientation, perf, crash-free soak test, store-ready APK/AAB |

## 8. Verification strategy
- **Protocol tests:** capture binary frames from the web client (or server fixtures)
  and assert the C++ BitBuffer reads/writes byte-identical results.
- **Simulation tests:** run both clients against the same server replay; compare
  player/object transforms per tick (dev tooling to dump state).
- **Visual diff:** screenshots of web vs native at matching camera/settings for M4+.
- **Perf gate:** M9 targets 60fps on a mid-range device; measure with `_CrtSetDbgFlag`/profiler.

## 9. Risks & mitigations
| Risk | Mitigation |
|---|---|
| Net protocol bit-incompatibility | Unit tests from M2 with captured frames; server is source of truth |
| PIXI `Graphics`/mask features without direct axmol equivalent | Adapter layer + `ClippingNode`; where impossible, fall back to `DrawNode` fill like the canvas path does |
| DOM UI is huge; axmol UI can't replicate it cheaply | D1 Option A (WebView menus) to ship; Option B as long-term |
| WebSocket reliability on mobile networks | Reconnect + backoff like `findGameImpl` retry; surface to UI |
| Asset format mismatches (woff2, m4a) | Convert in M1 pipeline step; document in `assets/README` |
| Shared defs TS→C++ drift | Codegen (D2) preferred to keep defs in sync with server |

## 10. Decisions to confirm before starting
- **D1 — UI strategy:** WebView overlay (fast) vs native axmol UI (thorough)?
- **D2 — Defs porting:** hand-port `shared/defs` vs. write a TS→C++ codegen build step?
- **D3 — Accounts/SDK:** is login, team link, pass, ads/SpellSync in scope for v1?
- **D4 — Android scope only?** axmol is cross-platform; iOS build is near-free once
  C++ exists. Is iOS also desired?
- **D5 — Orientation:** lock landscape (recommended) or support portrait?

## 11. Immediate next steps
1. Confirm decisions D1–D5.
2. Set up axmol + Android toolchain (M0).
3. Start the **BitBuffer + net message** port (M2) — it is the critical path and
   testable in isolation.
4. Build the `PixiLike.hpp` adapter and port `v2`/`math` while M0 toolchain matures.

## 12. Implementation status (this repo)

The **platform-independent core has been implemented and verified on the host**
(built with MSVC/clang, run via `surv_tests`). The engine/UI layers are
scaffolded and require the axmol SDK (M0) to compile.

### Done and verified (`src/core`, `src/net`, `tests`)
- **Codegen** (`tools/codegen_defs.mjs`): generates `src/net/generatedDefs.inc`
  from the real TS defs registry — exact `type→id` order for all 757 game types
  and 1072 map types, `protocolVersion=1028`, `structureLayerCount`, bag-size
  inventory order. Run with:
  `node --experimental-transform-types tools/codegen_defs.mjs src/net/generatedDefs.inc`
- **BitBuffer** (`src/core/BitBuffer.h`): bit-exact port of `shared/lib/bitBuffer.ts`
  (BitView + BitStream), incl. JS float64 big-endian scratch semantics.
- **Vec2 / MathUtil / Collider** (`src/core/`): ports of `shared/utils/v2.ts`,
  `math.ts`, `coldet.ts` collider types.
- **Net core** (`src/net/Net.h/.cpp`): Constants, BitSizes, MsgType, the
  definition registry, and the `BitStream` extensions (`writeFloat/readFloat`,
  `writeVec`, `writeMapPos`, `writeUnitVec`, game/map types, arrays, colliders,
  `MsgStream`).
- **Messages** (`src/net/Messages.h`): JoinMsg, JoinedMsg, InputMsg, EditMsg,
  SpectateMsg, DropItemMsg, EmoteMsg, PickupMsg, PlayerStatsMsg, KillMsg,
  RoleAnnouncementMsg, PerkModeRoleSelectMsg, AliveCountsMsg, GameOverMsg,
  MapMsg, UpdateMsg (incl. per-object serialize fns mirroring
  `server/src/game/objects/gameObject.ts` stream layout).
- **Reference fixtures** (`tools/gen_reference.mjs` → `tests/ReferenceData.h`):
  19 fixtures produced by running the **actual shared/ TS code**, so the C++ port
  is checked byte-for-byte against the JS implementation.
- **Tests** (`tests/`): 22 tests, all passing (`surv_tests`, exit 0), including
  the full `UpdateMsg` roundtrip (serialize → deserialize → re-serialize).

### M3 networking + headless sim (implemented, host-verified)
- `src/net/Connection.h` — engine-independent port of
  `shared/net/connection.ts` (`ConnectionState`, callbacks, `pump()`,
  `resetAndClose()`), plus the `ConnectionFactory` used to inject a transport.
- `src/net/WebSocketConnection.{h,cpp}` — `ax::network::WebSocket` adapter
  (binary frames). Delegate events are queued and delivered by `pump()` on the
  main thread, so the join/pump/dispatch flow is testable and thread-safe.
- `src/net/Api.{h,cpp}` — `find_game` over `ax::network::HttpClient` +
  rapidjson (port of `api.ts` + `main.ts findGame()`), posting to
  `/api/find_game_v2`.
- `src/game/Game.{h,cpp}` — `tryJoinGame()` creates the connection and sends
  the `JoinMsg` on open; `update()` pumps frames and dispatches
  `Joined`/`Update`/`Kill`/`GameOver`/`Pickup`/`Map`; `pause()`/`resume()`
  implement the background/foreground lifecycle; `snapshot()` /
  `snapshotText()` expose a deterministic headless state for replay diffing.
- `src/app/GameScene.{h,cpp}` — injects `createWebSocketConnection`, and
  auto-connects from a dev join target (`DevConfig.h` / UserDefault keys
  `surv_joinUrl`/`surv_joinToken`/`surv_apiUrl`/`surv_region`).
- `src/app/AppDelegate.cpp` — background pauses the ticker + closes the socket
  gracefully; foreground resumes and rejoins (server replay/snapshot).
- `tests/test_game.cpp` (+ `FakeConnection.h`) — drives the whole flow through
  an in-memory connection: JoinMsg bytes, multi-message frames, state snapshot,
  pause/close/resume, GameOver. All 29 `surv_tests` pass.

### M5 input → InputMsg (implemented, host-verified)
- `src/game/TouchInput.h` — engine-independent port of the touch branch of
  `client/src/game.ts update()`: turns `Touch::getMovement`/`getAim` into an
  `InputMsg` (`touchMoveDir`/`touchMoveLen`, `toMouseDir`/`toMouseLen`,
  `shootStart`/`shootHold`, `portrait`), including the turn-to-move aim
  cooldown and the throwable priming latch.
- `src/app/GameScene.cpp` — `updateInput()` builds the message every frame and
  sends it via `Game::sendInput()` at the server net-sync/input rate
  (`kNetSyncTps` = 33), latching a quick `shootStart` tap so it isn't dropped
  between sends. `Game` now assigns input seq/ack (`UpdateMsg.ack`), mirroring
  `game.ts` `seq`/`seqInFlight`.
- `src/ui/Touch.h` — added `turnDirCooldown`/`turnDirTicker`/`setAimDir` used by
  the turn-to-move aim logic.
- `tests/test_touch_input.cpp` — 4 tests covering move/aim scaling, zero-move,
  and the throwable latch (all 33 `surv_tests` pass). On-device move/aim/fire
  verification against a dev server remains (needs M4 rendering).

**Verifying the M3 gate against a dev server** (`pnpm dev:server` starts the API
on `:8000` and a game process on `:9000`):
1. Build+install the APK (see `BUILDING.md`).
2. Point the client at the server. The server advertises its region address
   (`127.0.0.1:9000/play` in dev), so the simplest emulator path is
   `adb reverse` (no server-config change):
   ```sh
   adb reverse tcp:8000 tcp:8000
   adb reverse tcp:9000 tcp:9000
   ```
   then set `DevConfig.h` `kApiBaseUrl = "http://127.0.0.1:8000"` (or the
   `surv_apiUrl` UserDefault) and rebuild. The client then calls
   `/api/find_game_v2` and joins the returned URL, which `adb reverse` maps back
   to the host. For a physical device on the same LAN, set the server region
   `address` to the machine's LAN IP (or use `surv_joinUrl` with a direct token)
   instead. Join tokens are single-use and expire after 10s, so prefer the
   `find_game` discovery path.
3. `adb logcat` shows the join + update stream (`Connected to game server`,
   `Receiving game updates: activePlayerId=...`); `Game::snapshotText()` can be
   logged per tick and diffed against the web client's state on the same replay.
   Verified on an emulator (API `127.0.0.1:8000` → game `127.0.0.1:9000`, stable
   update stream).

### M4 rendering port (implemented, host-verified; axmol adapter needs M0)
- `src/render/PixiLike.h` — expanded PIXI→axmol adapter surface (`Node`,
  `Graphics`, `Sprite`, `Text`, `Container`, `RenderTexture`, `Renderer`,
  `Factory`), including `setMask`/ClippingNode semantics.
- `src/render/AxmolPixi.h` — the axmol implementation: `DrawNode` (immediate
  vector fills/lines/circles), `Sprite` + `SpriteFrameCache`, `Label`,
  `Node` z-order (RenderGroup sort key → `localZOrder`), `ClippingNode` for
  masks, `RenderTexture` for the minimap.
- `src/render/NullPixi.h` — headless recording adapter used by the host tests
  (draw-command capture + z-sort bookkeeping).
- `src/render/Camera.h` — exact port of `camera.ts` (point/screen transforms,
  zoom/ppu, shake).
- `src/render/Terrain.{h,cpp}` — exact ports of `spline.ts`, `river.ts`,
  `terrainGen.ts` (Catmull-Rom spline, river polygons, jagged shore/grass),
  verified against TS fixtures (`terrain`, `spline`).
- `src/render/Renderer.{h,cpp}` — port of `renderer.ts`: 4 z-sorted layers,
  the `layer & 2` stairs remap, layer/ground alpha fades, structure layer mask,
  and `addPIXIObj` `__zOrd`/`__zIdx` early-out.
- `src/game/Map.{h,cpp}` — port of `map.ts`: terrain from `MapMsg`
  (seed + rivers + objects + ground patches), ground render (water/beach/grass,
  riverbank/water, grid, order-0/1 patches; hole-less canvas fallback), minimap
  transform, and the obstacle/building/structure pools + queries.
- `src/game/objects/GameObject.h` — port of `objectPool.ts` (`AbstractObject`,
  pooled `Pool<T>`, `ObjectCreator` full/part/delete routing).
- `src/game/objects/Structure.{h,cpp}` — structure layers/stairs/mask
  transforms (uses `mapHelpers.getBoundingCollider` output).
- `src/game/objects/Barns.{h,cpp}` — ported barns/visuals for obstacle,
  building (collision/ceiling subset), loot, dead body, projectile, smoke
  (+ `SmokeParticle`), bullet (trail visual), explosion, player (render
  subset) and a particle-emitter stub.
- `src/game/GameWorld.{h,cpp}` — the in-game render context (`Ctx`): owns the
  camera/renderer/map/barns, registers the type→pool map, applies `UpdateMsg`
  object deltas, spawns bullets/explosions, and drives per-frame updates.
- `src/render/Defs.{h,cpp}` + `tools/codegen_render_defs.mjs` →
  `src/render/GeneratedDefs.cpp` — codegen'd `DefProvider` (biome colors,
  map-object images/colliders, structure layers/stairs/mask, loot images).
- `src/app/GameScene.cpp` — builds the `AxPixiFactory`/`GameWorld`, attaches
  the ground + layers, installs the generated defs, wires the `Game` map/update
  callbacks, and follows the active player with the camera.
- Tests (`tests/test_render.cpp`): camera transform, terrain + spline vs TS
  fixtures, pool reuse, structure mask transform, renderer z-sort/stairs
  remap/layer fade, and the generated defs provider. All 40 `surv_tests` pass.

Remaining for M4 polish: faithful `Graphics` polygon holes (currently the
canvas fallback), full building floor/ceiling rendering + vision fade, full
`particles.ts` emitters, and the complete `player.ts` pose/outfit rendering.

### Scaffolded, needs the axmol SDK (M0 → M5)
- `src/app/AppDelegate.{h,cpp}` — axmol entry point (scene, landscape lock,
  lifecycle hooks).
- `src/app/GameScene.{h,cpp}` — scene wiring game + touch pads.
- `src/ui/Touch.h` — engine-independent dual-joystick port of `client/src/ui/touch.ts`
  (move/aim pads, dead-zone, locked/anywhere styles, throwable latch, aim-line).
- `src/render/PixiLike.h` — PIXI→axmol adapter surface for the M4 rendering port.
- `proj.android/` — axmol Android project (Gradle + JNI), `cmake/modules/`.

### Android build status
- **Offline / self-contained**: the axmol engine is vendored at `client-mobile/axmol/`
  (via `tools/vendor-axmol.ps1`, ~590 MB, excludes the desktop `build/`), and game
  assets are staged into `Content/` (`tools/sync-content.ps1`). The top-level
  `CMakeLists.txt` auto-detects the embedded `axmol/` engine, so no `AX_ROOT` env
  and no network fetches are needed at configure/build time.
- **CMake**: the build uses the **Android SDK's bundled CMake 3.22.1 + ninja**
  (`$ANDROID_HOME\cmake\3.22.1\bin`), not the system CMake 4.4. axmol 2.11.5's
  `1k/fetch.cmake` requires 3.23, so `tools/vendor-axmol.ps1` lowers it to 3.22 in
  the vendored copy.
- **Native libraries**: `libSurvevMobile.so` builds for **armeabi-v7a** and
  **arm64-v8a** (x86_64 also supported for the emulator) via
  `tools/build-native.ps1` (one build dir per ABI: `build-android-<abi>/`).
- **Signed APK**: `tools/build-apk.ps1` packages `SurvevMobile-debug.apk` with the
  Gradle-free pipeline (aapt2 → javac → d8 → zip → zipalign → apksigner), verified
  with APK Signature Scheme v2/v3. See `BUILDING.md` for the full guide.

### Known environment blocker: Gradle + Astrill VPN
On this machine, Astrill installs a Winsock LSP (`C:\Windows\System32\ASProxy64.dll`,
"ASProxy over MSAFD Tcpip"). It breaks Java NIO:
- default selector (`WEPoll`) → JVM `EXCEPTION_ILLEGAL_INSTRUCTION` in
  `sun.nio.ch.WEPoll.ctl`;
- legacy `WindowsSelectorProvider` → `select()` fails with "operation attempted
  on something that is not a socket".

Gradle's daemon communicates over TCP loopback via NIO, so **`gradlew` cannot run
while Astrill is active**. `tools/build-apk.ps1` bypasses Gradle using the Android
build tools directly (those run fine). To use the standard Gradle path instead,
stop/disable the Astrill `ASProxy` service (admin) or whitelist `java.exe`, then:
```sh
cd proj.android && ./gradlew assembleDebug
```
Note: `proj.android/gradle.properties` was given a `SelectorProvider` override to
avoid the JVM crash in the client JVM; the daemon still needs Astrill disabled.

### How to run the tests (host, no Android SDK needed)
```sh
cmake -S client-mobile/tests -B client-mobile/build-tests -G "Visual Studio 17 2022" -A x64
cmake --build client-mobile/build-tests --config Release
client-mobile/build-tests/Release/surv_tests.exe
```

### Next work items (in dependency order)
1. **M3**: net flow is implemented and host-verified (see above); remaining is
   on-device replay verification against a dev server, plus URL fallback when a
   join URL in `find_game`'s list fails.
2. **M4**: rendering port is implemented and host-verified against the adapter
   (map/terrain, renderer z-sort, barns, codegen'd defs); remaining is on-device
   visual verification (and the M4 polish list above).
3. **M5**: input → `InputMsg` is implemented and host-tested (`TouchInput.h` +
   `GameScene::updateInput`); remaining is on-device verification of
   move/aim/fire once M4 rendering lands.
4. **M7**: UI (menu/loadout/team) — the DOM UI needs porting or a WebView overlay.