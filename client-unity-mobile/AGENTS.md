# AGENTS.md — client-unity-mobile

Setup and workflow guide for AI agents and developers working on the **Unity
Android port** of the Survev web client (`../client`). Read `plan.md` first: it
maps every web module to its Unity/C# target, records the resolved decisions
(D1–D8), and lists milestones M0–M9.

This is a **1:1 conversion** of the web client: same UI/UX, styles, look and
feel, assets and numbers. Do not redesign or approximate.

## Repository overview

| Path | What it is |
|---|---|
| `Assets/Scripts/` | All C# game + UI code (see subfolders below). |
| `Assets/Scenes/` | `Bootstrap`, `Menu`, `Game` scenes. |
| `Assets/Editor/` | Codegen importers, atlas/font importers, build hooks. |
| `Assets/StreamingAssets/` | Staged web assets (`img/`, `audio/`, `l10n/`). **Gitignored** — regenerate. |
| `Packages/manifest.json` | Pinned Unity 2020.3 package set (offline-friendly). |
| `ProjectSettings/` | Unity project settings (`ProjectVersion.txt` = 2020.3.x). |
| `ThirdParty/` | Vendored plugins/libraries; all offline deps live here. |
| `tools/` | PowerShell/mjs helpers: asset staging, atlas build, codegen, fetch. |

### C# script folders (`Assets/Scripts/`)

| Folder | Ports |
|---|---|
| `Core/` | `shared/{lib/bitBuffer, utils/v2, utils/math, utils/coldet, utils/collider}` + `gameConfig` |
| `Net/` | `shared/net/*` (connection + messages), `client/src/{api,proxy,siteInfo,pingTest}.ts` |
| `Game/` | `client/src/{game,map,gas}.ts` and `client/src/objects/*` |
| `Render/` | `client/src/{renderer,camera,map(render part),resources,ambiance}.ts` |
| `UI/` | `client/src/ui/*`, `client/index.html`, `css/app.css`, `css/game.css` |
| `Config/` | `client/src/{config,device,helpers}.ts` |
| `Generated/` | **Machine-generated** defs/constants — never hand-edit (§Codegen). |

`shared/defs` is the **single source of truth** for game data. C# defs in
`Assets/Scripts/Generated/` are generated from it; never transcribe by hand.

## Toolchain & versions

- **Unity 2020.3 LTS** (Android module: SDK/NDK/OpenJDK via Unity Hub).
  Pin the exact patch in `ProjectSettings/ProjectVersion.txt`.
- **Scripting backend:** IL2CPP for release, Mono for editor/dev.
- **Orientation:** landscape only.
- Node `>=22.18.0` for the staging/codegen tools (matches the repo root).
- PowerShell 7 (`pwsh`) for the `.ps1` tools.

> The mono-repo root `AGENTS.md` still applies for commit conventions and shared
> code. This file adds Unity-specific setup only.

## First-time setup (offline)

```powershell
cd client-unity-mobile

# 1. Stage the web client's static assets into Assets/StreamingAssets (gitignored)
.\tools\sync-assets.ps1
#    - copies ../client/public/{img,audio,l10n,fonts} -> Assets/StreamingAssets
#    - copies ../client/src/en.json -> Assets/StreamingAssets/l10n/en.json

# 2. Build the Unity sprite atlas (PNG sheets + frame JSON, web *.img names kept)
node tools\build-atlas.mjs --res low

# 3. Generate the C# defs/constants from the real shared/ TS registry
node --experimental-transform-types tools\codegen-defs.mjs

# 4. Vendor the offline third-party libraries (UniTask, websocket-sharp,
#    Newtonsoft.Json). Already committed in the repo; the script re-fetches /
#    verifies them for a fresh machine.
.\tools\fetch-thirdparty.ps1
```

Then open the folder with Unity Hub (Editor **2020.3.x**) and let it import.

## Common commands

```powershell
# Editor build for Android (IL2CPP) — replace with your Editor path
& "C:\Program Files\Unity\Hub\Editor\2020.3.x\Editor\Unity.exe" `
    -batchmode -quit -projectPath . `
    -executeMethod BuildScript.BuildAndroid -logFile build.log
# -> Builds/SurvevMobile.apk

# Regenerate C# defs after changing ../shared/defs
node --experimental-transform-types tools\codegen-defs.mjs

# Rebuild the atlas after changing ../client/public/img
node tools\build-atlas.mjs --res low

# Run the edit-mode/play-mode tests (headless)
& "C:\Program Files\Unity\Hub\Editor\2020.3.x\Editor\Unity.exe" `
    -batchmode -runTests -projectPath . -testPlatform EditMode `
    -testResults TestResults.xml -logFile test.log
```

## Asset pipeline rules

- **Atlases are required.** The web client resolves every `*.img` sprite name to
  a virtual atlas frame built by `client/atlas-builder`. `tools/build-atlas.mjs`
  converts the web atlas cache (`../client/node_modules/.atlas-cache`) into PNG
  sheets + frame JSON under `Assets/` and **keeps the web `*.img` frame keys**, so
  `SpriteAtlasRegistry.Get("loot-shirt-01.img")` matches the web client exactly.
  Run the web client build once to populate the cache.
- **`Assets/StreamingAssets/` is gitignored** (large). Always run
  `tools/sync-assets.ps1` on a fresh checkout before building.
- **Audio** is 422 `.mp3` files — load as `AudioClip` and port the
  `soundDefs.ts` volume tiers. Do not transcode.
- **Fonts**: the web ships Roboto Condensed **woff2** (Unity cannot load it).
  Convert to TTF and build **TMP SDF** assets (Normal + Bold) with the same
  family name.
- **UI styles** are ported verbatim from `css/app.css` + `css/game.css` into
  `UiTheme.cs` / `UiStyles` (page green `#80af49`, `--side-pad: 12px`,
  `.menu-block`, `.btn-green`/`.btn-darken`/`.btn-hollow`, gold news headers).
- **Generated files are deterministic** — commit them, but only via the tools.

## Codegen

`tools/codegen-defs.mjs` imports the real TypeScript `shared/defs` registry (run
from `client-unity-mobile/` with `node --experimental-transform-types`) and emits
C# into `Assets/Scripts/Generated/`:

- the `type -> id` order for all game and map types,
- `protocolVersion`, bag/inventory order, structure layers,
- biome colors, map-object images/colliders, loot images, `gameConfig` constants.

Never hand-edit `Assets/Scripts/Generated/**`. If a def must change, change
`shared/defs` (the source of truth) and regenerate.

## Networking rules

- The protocol is **bit-exact** against the server. `Core/BitBuffer` and every
  `Net/*Msg` class must match the TS byte layout; add a captured-frame test for
  any change.
- **Do not add fields to any message** without a server change and regenerated
  fixtures. The HUD/UI must derive new values from existing fields.
- WebSocket frames arrive on a background thread: queue them and **pump on the
  main thread** so join/pump/dispatch stays deterministic (mirrors
  `shared/net/connection.ts`).
- Join tokens are **single-use and expire (~10s)** — always use the
  `/api/find_game_v2` discovery path, never a cached token.

## Android / dev-server notes

- Dev server: API on `:8000`, game on `:9000`. Point an emulator at the host
  without touching server config:
  ```powershell
  adb reverse tcp:8000 tcp:8000
  adb reverse tcp:9000 tcp:9000
  ```
  A physical device on the LAN needs the machine's IP (the server listens on
  `0.0.0.0`).
- Emulator host loopback is `10.0.2.2`; `adb reverse` maps back to `127.0.0.1`.
- Use a **development build with the Mono backend** for readable stack traces;
  verify the **IL2CPP release** build early to catch stripping issues
  (`link.xml` for net/serialization types).

## Third-party / offline policy

Everything required to build must live in `client-unity-mobile/`; no network at
build time.

- Unity packages are pinned in `Packages/manifest.json` (2020.3-compatible) and
  cached locally; UniTask is referenced as a local package from
  `ThirdParty/UniTask` (`"com.cysharp.unitask": "file:../ThirdParty/UniTask"`).
- Vendored libraries live under `ThirdParty/<name>/` (source or IL2CPP-safe
  `.dll`) with a `README.md` recording origin, license and exact version. The
  vendored trees are **committed** so a checkout builds offline.
- `tools/fetch-thirdparty.ps1` vendors the pinned revisions, verifies hashes and
  prefers a locally-provided `ThirdParty/<name>.zip` over downloading.
- See `ThirdParty/README.md` for the current list (UniTask 2.5.11,
  websocket-sharp `f7904e6…`, Newtonsoft.Json 13.0.4) and how to refresh it.

## Working in this repo

- Match the web client exactly: same colors, sizes, assets, numbers, flows.
  Keep ported logic diffable against its TS source and reference the TS file in
  a comment.
- Keep changes scoped; do not mass-reformat unrelated files.
- Update `plan.md` when a milestone advances or a decision changes.
- Follow the repo-root `COMMIT_FORMAT.md`:
  `<type> (<scope>): (<file>) <subject>`.
- Treat unfamiliar/modified files as potential user work; investigate before
  overwriting. Regenerate machine-produced files, never hand-edit them.
