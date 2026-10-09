# Plan: Convert the web client (`../client`) to a Unity Android app

## 1. Overview

`../client` is the web version of Survev: a TypeScript + Vite single-page game
that renders with **PIXI.js-legacy**, builds its menus/HUD in **HTML + CSS +
jQuery + Bootstrap**, talks to the server over a **binary WebSocket protocol**
(`shared/lib/bitBuffer.ts` + `shared/net/*`), and keeps all simulation/game data
in `shared/` (defs, math, collision, terrain).

This project is the **Unity 2020.3 LTS (Android, C#)** conversion of that client.
The goal is a **1:1 port**: the same UI/UX, styles, look and feel, assets and
numbers as the web client — not an approximation or a redesign.

Target platform: **Android** (landscape). Unity 2020.3 LTS.

> **Scale warning.** This is a full rewrite of the client into C#/Unity. It is
> weeks-to-months of work for one developer. Treat it as a sequence of verifiable
> milestones that each end in something runnable — never one big-bang port.

## 2. Source of truth (what we port)

```
client/
  index.html            # menu + in-game #ui-game overlay markup (2175 lines)
  css/app.css           # menu + global styles (colors, menu-block, buttons)
  css/game.css          # in-game HUD styles
  src/main.ts           # bootstrap, menu glue, find_game, join, quit
  src/game.ts           # core Game: sim + update loop + barns
  src/renderer.ts       # PIXI renderer: z-sorted layers, masks, camera mapping
  src/camera.ts         # camera / view transform
  src/map.ts            # map render + structure/collision
  src/gas.ts            # zone/gas sim + draw
  src/input.ts          # mouse + touch input
  src/inputBinds.ts     # key/input binds
  src/audioManager.ts   # sound engine
  src/soundDefs.ts      # sound defs
  src/resources.ts      # asset loading
  src/api.ts, proxy.ts  # HTTP API, proxy/login detection
  src/account.ts        # account / auth
  src/config.ts         # ConfigManager (settings in localStorage)
  src/device.ts         # device/os detection
  src/ambiance.ts       # theme/map ambient audio + colors
  src/emote.ts, crosshair.ts, helpers.ts, errorLogs.ts, pingTest.ts, siteInfo.ts
  src/objects/          # pooled objects (player, bullet, projectile, loot, ...)
  src/ui/               # DOM UI: menu, loadout, team, profile, pass, HUD, touch
  src/sdk/              # SpellSync / ad SDK glue (out of scope, see D6)
  public/img|audio|fonts|l10n   # static assets + localization JSON
shared/
  lib/bitBuffer.ts      # bit-level reader/writer (net protocol)  ← port first
  net/*.ts              # WebSocket connection + all game messages
  utils/*.ts            # v2/math/collider/coldet/terrain/river/spline/loadout
  defs/**               # GameObjectDefs / MapDefs / map objects / puzzles
  gameConfig.ts         # constants (protocolVersion, max players, ...)
```

Traits that drive the port:

- **Bit-exact binary protocol.** `BitBuffer` is byte/bit compatible with the
  server; this is the critical path and must be unit-tested against captured
  frames.
- **PIXI scene model.** `Container` z-sorting (`__zOrd`/`__zIdx`), `Graphics`
  vector fills/lines, stencil masks for building vision, `Sprite` + atlases,
  tints and blend modes.
- **DOM UI.** Everything outside the canvas is HTML/CSS; its exact colors,
  sizes, 9-slice-ish borders and fonts must be reproduced (uGUI, D3).
- **`shared/defs` is the single source of truth.** Defs are TS modules with
  computed values consumed by the server, web client and tests.

## 3. Decisions (resolved)

| # | Decision | Choice |
|---|---|---|
| D1 | Language / scripting backend | **C#**, IL2CPP for Android release builds |
| D2 | UI strategy | **Rebuilt 1:1 in Unity uGUI** (+ TextMeshPro). No WebView. |
| D3 | Unity version | **2020.3 LTS** (`ProjectVersion.txt` pinned to a 2020.3.x patch) |
| D4 | Third-party code | **Vendored under `ThirdParty/`** for offline builds; fetched by `tools/fetch-thirdparty.ps1` (see §7) |
| D5 | Orientation | **Landscape only** (`ScreenOrientation.LandscapeLeft/Right`) |
| D6 | Scope | Android, gameplay + menu/loadout/team/HUD. Accounts/SDK/ads later (§T14). |
| D7 | Defs porting | **Codegen from `shared/defs`** into C# (§6.3) — never hand-port 757+ types |
| D8 | Networking | `websocket-sharp` (vendored) as the primary transport, `ClientWebSocket` as fallback |
| D9 | Android ABIs | **armeabi-v7a + arm64-v8a** (both 32-bit and 64-bit ARM devices) |

## 4. Target architecture (Unity)

```
Rendering:   PIXI.Container/Graphics/Sprite  ->  UnityEngine.SpriteRenderer + Quad mesh
                                                  batched through a custom GraphicsLayer
                                                  (LineRenderer / Mesh) held in a z-sorted
                                                  hierarchy — mirrors renderer.ts 1:1
Game loop:   PIXI.Ticker                     ->  Unity PlayerLoop / MonoBehaviour.Update
Networking:  browser WebSocket               ->  ClientWebSocket (raw binary frames)
Audio:       audioManager/Howler             ->  AudioSource pool + AudioClip from StreamingAssets
HTTP/API:    fetch                           ->  UnityWebRequest
Settings:    localStorage                    ->  PlayerPrefs (ConfigManager wrapper)
Assets:      public/**                       ->  StreamingAssets/** (img, audio, l10n)
UI:          HTML/CSS                        ->  uGUI Canvas + TMP, CSS-exact colors/sizes
Atlas:       atlas-builder (PIXI)            ->  SpriteAtlas/ or generated .png + frame data
```

**Adapter layer.** As with any port of this client, keep a thin adapter in front
of the renderer (`Assets/Scripts/Render/PixiLike/`) exposing the exact PIXI calls
the game uses (`moveTo`, `lineTo`, `drawRect`, `beginFill`, `endFill`, `Sprite`,
`setMask`, ...). Port `renderer.ts`/`map.ts`/`objects/*` against the adapter so
the C# draw code stays diffable against the original TS.

## 5. Repository layout

```
client-unity-mobile/
  plan.md                     this file
  AGENTS.md                   setup + workflow for agents/devs
  README.md                   short human intro
  ProjectSettings/            Unity project (ProjectVersion 2020.3.x)
  Packages/
    manifest.json             pinned registry packages (2020.3 versions)
    packages-lock.json        committed lock
  Assets/
    Scenes/                   Bootstrap, Menu, Game
    Scripts/
      Core/                   BitBuffer, Vec2, MathUtil, Collider, GameConfig
      Net/                    Connection, messages, Api, SiteInfo
      Game/                   Game, GameWorld, Map, Gas, pools, objects/*
      Render/                 PixiLike adapter, Camera, Renderer, Terrain
      UI/                     menu, loadout, team, profile, pass, HUD, touch
      Config/                 ConfigManager, Device, Localization, Files
    Editor/                   codegen importers, atlas/font importers, build hooks
    Fonts/                    Roboto Condensed (TMP SDF assets)
    Sprites/                  generated atlases + GUI icons (from Content/, §6)
    StreamingAssets/
      img|audio|l10n/         staged from ../client/public (gitignored)
  ThirdParty/                 vendored plugins/libraries (see §7)
  tools/                      PowerShell/mjs helpers (staging, codegen, fetch)
  ProjectSettings/            as above
```

## 6. Asset & data pipeline

### 6.1 Static assets
- `../client/public/img`, `audio`, `l10n`, `fonts` -> `Assets/StreamingAssets/`
  via `tools/sync-assets.ps1` (mirrors the web asset tree). `img/**` is mostly
  `.svg` (1616 files) plus a few `.png`/`.webp`.
- `Assets/StreamingAssets/` is **gitignored** (large); regenerate with the script.
- `Client/src/en.json` -> `Assets/StreamingAssets/l10n/en.json` (English is not
  under `public/l10n`).

### 6.2 Textures
- **Atlases are required.** The web client packs `public/img/**/*.svg` into
  virtual atlases via `client/atlas-builder`; every `*.img` name resolves to an
  atlas frame. Reuse the web atlas cache (`client/node_modules/.atlas-cache`)
  with `tools/build-atlas.mjs` to emit **PNG sheets + a frame index**
  (Unity-readable JSON), keeping the web `*.img` frame names verbatim.
- Runtime: a `SpriteAtlasRegistry` maps `"loot-shirt-01.img"` -> `(sheet,
  Rect, pivot)` and feeds `PixiLike.Sprite`. This is the Unity analogue of
  PIXI `Assets`/`Spritesheet`.
- Scale: the web mobile path uses the `low` (0.5) atlas; ship `low` and optionally
  `high` for tablets.

### 6.3 Game data (`shared/defs`)
- Write a **codegen** (`tools/codegen-defs.mjs`, Node with
  `--experimental-transform-types`, run from `client-unity-mobile/`) that imports
  the real `shared/defs` registry and emits C# (`Assets/Scripts/Generated/`).
  Mirrors the approach already proven for other ports: never hand-transcribe
  hundreds of computed defs.
- Also generate: `gameConfig` constants, the `type -> id` order for all game and
  map types, `protocolVersion`, bag/inventory order, biome colors, map-object
  images/colliders, structure layers.
- Generated files are **deterministic** — commit them, but only via the tool.

### 6.4 Sounds
- `public/audio/**` is **422 `.mp3` files** — already Android-friendly; copy as
  is into StreamingAssets and load with `UnityWebRequestMultimedia` (or
  `AudioClip` streaming). Port `soundDefs.ts` volume tiers / mute and
  `ambiance.ts` loops.

### 6.5 Fonts & UI styles
- Web uses **Roboto Condensed** (woff2). Unity/TMP needs a TTF: convert the
  woff2 to TTF and build **TMP SDF** assets (Normal + Bold), exact family.
- Port `css/app.css` + `css/game.css` values verbatim: the green `#80af49` page
  background, `--side-pad: 12px`, `.menu-block` translucent panels, `.btn-green`
  /`.btn-darken`/`.btn-hollow` colors + 2px shadows, gold news headers, the
  `.btn-*` icon sheet, etc. Keep a `UiTheme.cs` of named colors/sizes and a
  `UiStyles` 9-slice library so building `index.html` is mechanical.

## 7. Third-party / offline policy

Everything needed to build must be inside `client-unity-mobile/`; **no network at
build time**. Two categories:

1. **Unity packages** — pinned in `Packages/manifest.json` and cached locally.
   ![ ] Keep the registry versions fixed (2020.3-compatible): TextMeshPro,
   Unity UI, Addressables (optional), Burst/Collections (if used).
2. **Vendored libs** — under `ThirdParty/` as source or prebuilt `.dll`
   (IL2CPP-safe), each with a `ThirdParty/<name>/README.md` recording origin +
   license + exact version.

Planned third-party surface (see `ThirdParty/README.md`):

| Need | Library | Form |
|---|---|---|
| WebSocket (mobile-safe) | `websocket-sharp` (sta/websocket-sharp) | source, MIT |
| JSON (fast) | `Newtonsoft.Json` 13.0.4 | source, MIT |
| Async/threading helpers | `UniTask` 2.5.11 | local UPM package, MIT |
| Atlas/runtime utils | in-house (generated) | codegen, not a plugin |

`tools/fetch-thirdparty.ps1` downloads/vendors each pinned revision and verifies
a hash; `ThirdParty/README.md` documents the exact steps for a fresh machine.
The vendored trees are committed so the checkout is self-contained. Until Unity
is available locally, `manifest.json` + the fetch script are the deliverable (D4).

## 8. Module-by-module porting strategy

### 8.1 Shared core (port first, unit-testable)
| TS source | C# target | Notes |
|---|---|---|
| `shared/utils/v2.ts` | `Vec2` | 1:1 math, struct |
| `shared/utils/math.ts` | `MathUtil` | clamp/lerp/rand |
| `shared/lib/bitBuffer.ts` | `BitBuffer`/`BitView` | **bit-exact**; JS float64 big-endian scratch semantics |
| `shared/net/*.ts` | `Net/Messages` | one C# class per message, `Read/Write(BitBuffer)`, field order exact |
| `shared/net/connection.ts` | `Connection` + `WebSocketTransport` | queue frames, pump on main thread |
| `shared/utils/coldet.ts`, `collider.ts`, `collisionHelpers.ts` | `Collider`/`Coldet` | earcut-style triangulation vendored |
| `shared/utils/terrainGen.ts`, `river.ts`, `spline.ts`, `mapHelpers.ts` | `Terrain`/`MapHelpers` | client-side terrain draw |
| `shared/defs/**` | `Generated/*` | **codegen** (D7) |
| `shared/gameConfig.ts` | `GameConfig` | constants |

### 8.2 Game simulation
- `game.ts` -> `Game` (connection, object pools, message dispatch, update order).
- `objects/*` -> C# classes (player, bullet, projectile, loot, smoke, particles,
  explosion, structure, shot, deadBody, airdrop, plane, flare, decal,
  mapIndicator, mapSprite). Logic identical; only draw calls change.
- `gas.ts` -> `Gas` (zone circles + damage ticks).

### 8.3 Rendering (PIXI -> Unity)
| PIXI | Unity |
|---|---|
| `PIXI.Application` | `Camera` + root `GraphicsLayer` in the Game scene |
| `Container` + `__zOrd`/`__zIdx` | sorted child list per layer (mirror `renderer.ts`) |
| `Graphics` (`beginFill`, paths) | `Mesh`/`DrawMesh` immediate geometry (adapter) |
| `beginHole` / stencil mask | stencil buffer material or per-layer clipping |
| `Sprite` + atlas | `SpriteRenderer` + `SpriteAtlasRegistry` frames |
| `tint` / alpha / blend | `SpriteRenderer.color`, `Material` blend |
| `PIXI.Text` | TMP (`TextMeshPro`) with Roboto Condensed |
| `resize`/`scaleToScreen` | camera orthographic size from a 1280x720 design space |

### 8.4 Input
- Mouse + touch mapped to the same `InputMsg` fields (touch move/aim, shoot
  start/hold, portrait flag). Port `input.ts`, `inputBinds.ts` semantics and the
  `ui/touch.ts` dual-joystick logic (dead-zone, locked/anywhere styles, throwable
  latch, turn-to-move aim cooldown).

### 8.5 Audio
- `audioManager.ts`/`soundDefs.ts` -> `AudioBackend` with master/sound/music
  tiers, mute, and `ambiance.ts` loop switching. `soundDefs` -> codegen.

### 8.6 UI (uGUI, 1:1 with `index.html` + CSS)
- Build each DOM block as a prefab under a 1280x720 design resolution Canvas,
  matching CSS pixel sizes/colors/anchors. Web y-down coordinates are mapped by
  the layout helpers, exactly like the CSS box model.
- Screens: main menu (`#start-menu-wrapper`, news, cog/mute), settings, how-to-play,
  join-team, loadout (`#modal-customize`), team room, in-game HUD (`#ui-game`:
  health/boost, weapon slots, ammo, killfeed, alive count, minimap, pause menu,
  role select).
- `ui2.ts` HUD updates, `menuModal.ts` modals, `profileUi`, `pass`.
- Localization: `l10n/*.json` loaded from StreamingAssets; port
  `localization.ts` (18 locales, `en.json` fallback).

### 8.7 API / account / proxy
- `api.ts`/`proxy.ts`/`account.ts` -> `UnityWebRequest` ports. Scope for v1:
  `site_info`, `find_game_v2`, ping/region (D6 keeps login/team-account later).
- `pingTest.ts`, `siteInfo.ts` -> region list + latency.
- `sdk/` (SpellSync/ads) — **out of scope** (browser glue).

### 8.8 Android specifics
- Landscape lock; handle safe areas / notches for the HUD and menu.
- `OnApplicationPause/Focus`: pause sim, close socket gracefully, rejoin on
  resume (server replay/snapshot) with no desync.
- Back button = in-game menu / confirm, not app exit.
- Perf: cap resolution by device class (`device.ts`), disable MSAA, power-of-two
  atlas textures, target 60fps on mid-range.

## 9. Milestones (each ends runnable)

| # | Milestone | Gate |
|---|---|---|
| M0 | Unity 2020.3 Android project + toolchain | Empty scene builds and runs on emulator/device (**armeabi-v7a + arm64-v8a**) |
| M1 | Asset pipeline | Atlas + sprite + audio + TMP font + `l10n` all load in a test scene |
| M2 | Shared core | BitBuffer + v2/math + all message classes pass NUnit tests from captured frames |
| M3 | Net + headless sim | Connects to a dev server, receives ticks, deterministic state snapshot matches web |
| M4 | Rendering | Map/terrain/buildings/players/particles render as the web client |
| M5 | Input + touch | Dual joystick + aim/fire + camera follow; player moves/shoots correctly |
| M6 | Audio + ambience | SFX/gunshots/music/gas correct |
| M7 | UI | Menu -> loadout -> find game -> game -> death flow, 1:1 with web |
| M8 | Account/API | Login/profile/team if kept in scope |
| M9 | Polish/release | Lifecycle, orientation/safe-area, perf, soak test, signed APK/AAB |

## 10. Verification

- **Protocol:** capture binary frames from the web client / server fixtures and
  assert C# BitBuffer reads/writes are byte-identical (NUnit).
- **Simulation:** run both clients against the same replay; diff per-tick state.
- **Visual:** side-by-side screenshots web vs Android at matching camera/HUD.
- **Perf:** 60fps on a mid-range device; measure CPU/GPU + allocs.
- **UI parity:** compare each screen against `index.html` at 1280x720.

## 11. Risks & mitigations

| Risk | Mitigation |
|---|---|
| Bit protocol incompatibility | Unit tests from M2 against captured frames; server is truth |
| SVG-heavy asset pipeline | Reuse `client` atlas cache via `build-atlas.mjs`; keep `*.img` names |
| CSS 1:1 fidelity | `UiTheme`/`UiStyles` from `app.css`/`game.css`; design-space canvas |
| Unity package availability offline | Pinned manifest + `ThirdParty/` vendoring (D4) |
| IL2CPP stripping of net/serialization code | `link.xml` preservation + smoke test release build early |
| Mobile WebSocket reliability | reconnection/backoff + surface errors in UI |
| `shared/defs` drift | Codegen (D7) from the live TS registry |

## 12. Immediate next steps

1. Create the Unity 2020.3 project skeleton (this scaffold): `ProjectSettings`,
   `Packages/manifest.json`, `Assets/`, `ThirdParty/`, `tools/`.
2. Stage assets (`tools/sync-assets.ps1`) and build the Unity atlas
   (`tools/build-atlas.mjs`) + TMP fonts.
3. Start **M2** (BitBuffer + messages) — critical path, testable without Unity.
4. Stand up the **M0** Android build once Unity 2020.3 is installed locally.
5. Port `v2`/`math` and build the `PixiLike` adapter while M0/M1 mature.
