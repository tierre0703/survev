# client-mobile — what is left to implement (1:1 web → axmol port)

Consolidated status of the native Android port of `client/` (`client-mobile/`).
This file is a **summary/index**; the detailed work orders live in:

- `plan.md` — module map, milestones M0–M9, decisions D1–D5.
- `M4_REMAINING.md` — rendering (sessions 1–6), verified object-render parity.
- `M6_AUDIO.md` — audio port notes.
- `M7_REMAININGS.md` — UI (menu/loadout/team/HUD) handoff, **superseded by the
  UI split below** (kept as the record of what exists + the build/test workflow).
- **`ARCHITECTURE.md`** — **the current UI architecture: gameplay = axmol +
  FairyGUI, every other screen = Android Java, one JNI bridge (`SurvBridge`).**
  Read §5 (move matrix) and §7 (migration steps S1–S7) before touching UI code.

Latest recorded state: **88 host tests / 1746 assertions green** (after §6),
x86_64 `libSurvevMobile.so` builds, APK packages and installs, `logcat -b crash`
empty across the recorded flows. **No UI migration code has landed yet: S1 of
`ARCHITECTURE.md` §7 is the next work item.**

---

## 1. Milestone scorecard

| # | Milestone | Status | Notes |
|---|---|---|---|
| M0 | Toolchain + axmol builds | **done** | axmol vendored, Gradle-free APK pipeline (`tools/build-apk.ps1`). |
| M1 | Asset pipeline | **done** | atlases, GUI PNGs (4x), Roboto Condensed TTF, l10n. |
| M2 | Shared core ports (bit-exact) | **done** | `BitBuffer`, `Vec2/MathUtil/Collider`, message classes, TS-generated fixtures. |
| M3 | Net + headless sim | **done (host)** | join/update/kill/gameover/pickup/map dispatch, `snapshotText()`; multi-URL join fallback. |
| M4 | Rendering | **done, latest verified** | terrain/island hole, obstacles, buildings + ceilings, particles, decals, gas, skeletal player. |
| M5 | Input + touch | **mostly done** | move/aim/fire → `InputMsg`; joystick gfx is a placeholder shape; aim-line dots missing. |
| M6 | Audio | **partial** | engine + mixer + ambience done; only a handful of events are wired (see §3.4). |
| M7 | UI layer | **architecture changed 2026-10-14** | native axmol menu/loadout/team/HUD/pause exist but are being **replaced** per `ARCHITECTURE.md`: gameplay HUD → axmol + **FairyGUI**, all other screens → **Android Java** over the `SurvBridge` JNI bridge. Migration steps S1–S7; nothing deleted until its Java/FairyGUI replacement lands. |
| M8 | Accounts / API | **not started** | login/profile/team-link/quests/pass. |
| M9 | Polish & release | **not started** | lifecycle, perf, soak, store-ready AAB. |

> **§6 (latest session)** ports the missing `airdrop`/`plane`/`flare`/`shot`
> barns — see the session log at the bottom of this file.

---

## 2. Module-by-module: ported vs left

| Web source (`client/src`, `shared/`) | Native target | Status |
|---|---|---|
| `main.ts` (glue) | `app/MenuScene.*`, `app/GameScene.*` | done (join/matchmaking/quit). |
| `game.ts` | `game/Game.*`, `game/GameWorld.*` | done for net + object deltas; **missing barns**: airdrop, plane, flare, shot, emote, mapIndicator (see §3.1). |
| `renderer.ts`, `camera.ts` | `render/Renderer.*`, `render/Camera.h` | done. |
| `map.ts` | `game/Map.*` | done (terrain, surfaces, pools, camera emitter); **minimap/big map not ported**. |
| `gas.ts` | `game/Gas.*` | done. |
| `objects/player.ts` + `animData.ts` | `objects/PlayerRender.cpp` | done (bones/poses/animations/aura/submerge/frozen/team patches/anim effects). |
| `objects/obstacle.ts`, `building.ts`, `loot.ts`, `decal.ts`, `deadBody.ts`, `projectile.ts`, `smoke.ts`, `bullet.ts`, `explosion.ts`, `particles.ts`, `structure.ts` | `objects/Barns.*`, `objects/Structure.*` | done. |
| `objects/airdrop.ts`, `plane.ts`, `flare.ts`, `shot.ts` | `objects/Barns.*` | **ported** (§6): airdrop wired to `ObjectType_Airdrop`, planes from `UpdateMsg.planes[]`, flare-gun bullets, per-shot recoil. |
| `emote.ts` (pings, emote wheel, indicators) | — | **not ported**. |
| `objects/mapIndicator.ts`, `mapSprite.ts` | — | **not ported**. |
| `ui/touch.ts` | `ui/Touch.h` | data model done; **pad sprites + `LineSprites` aim line missing** (§3.2). |
| `input.ts`, `inputBinds.ts` | `ui/Touch.h`, `game/TouchInput.h` | touch done; **mouse/keyboard/binds not ported** (BT keyboard/controller). |
| `audioManager.ts`, `soundDefs.ts`, `ambiance.ts` | `audio/*` | engine done; **per-event wiring mostly missing** (§3.4). |
| `config.ts`, `device.ts`, `ui/localization.ts` | `ui/Config.*`, `ui/Device.*`, `ui/Localization.*` | done. |
| `ui/menu.ts`, `loadoutMenu.ts`, `teamMenu.ts`, `ui2.ts`, `pieTimer.ts`, `profileUi.ts`, `pass.ts`, `opponentDisplay.ts`, `menuModal.ts` | **→ Java** (`MenuFragment`, `LoadoutFragment`, `TeamFragment`, `GamePauseDialog`) + gameplay HUD → **FairyGUI** `HUD_pkg` | current C++ screens exist (`ui/StartMenu.cpp`, `ui/LoadoutMenu.*`, `ui/TeamMenu.*`, `ui/UiOverlay.*`) but are being replaced per `ARCHITECTURE.md`; profile/pass/opponents/pie-timer **not ported** (§3.5). |
| `api.ts`, `proxy.ts`, `siteInfo.ts`, `pingTest.ts` | **→ Java** (`net/SurvApi.java`) | `net/Api.*`/`net/SiteInfo.*` currently do site-info + find_game (kept for host tests); **proxy/login/region-ping-test not ported**. |
| `account.ts` | — | **not ported** (M8). |
| `resources.ts` | `tools/build-atlas.mjs` + `SpriteFrameCache` | done. |
| `errorLogs.ts` | — | **not ported**. |
| `crosshair.ts` | — (config defaults exist) | **not ported** (no crosshair rendering). |
| `emote.ts` indicators, `sdk/*` | — | out of scope per plan D3. |
| `debug/*` (graph, editor, debugLines, debugHUD, debugHelpers) | — | **not ported** (dev tooling). |

---

## 3. Remaining work, by area (priority order)

### 3.1 In-game barns / objects still missing (highest gameplay impact)

**Implemented (this pass):** `airdrop.ts`, `plane.ts`, `flare.ts` and `shot.ts`
barns (see §6). Still missing:

- **`emote.ts` → `EmoteBarn`** — emote/ping wheel (touch + mouse), the
  ping/emote indicators (world indicator, minimap indicator, big-map ping),
  `PingDefs`/`EmoteDefs` icons, throttling (`emoteSoftTicker`/`emoteHardTicker`),
  emote timeout/`ui-emote-button` state. Also the ping/emote sounds.
- **`mapIndicator.ts` → `MapIndicatorBarn`** and **`mapSprite.ts`** —
  `UpdateMsg.mapIndicators[]` is parsed but unused; drives the minimap/big-map
  markers (team pings, airdrop/airstrike markers from `emote.ts`).
- **Minimap / big map** (`ui2.ts` `#ui-map-wrapper`, `#big-map-*`) — the native
  HUD has **no minimap at all**; `renderer.ts`'s `RenderTexture` minimap path
  exists in the adapter but nothing drives it.

### 3.2 Touch / input gaps

- **Pad sprites**: the web client draws `pad.img` for the center/touch dots
  (`alpha 0.2` / `1.0`, `padScaleDown`/`padScalePos`). Native
  `TouchPadGfx` (`app/GameScene.cpp`) draws placeholder `DrawNode` dots instead.
  Port the real sprite + scaling.
- **Aim line (`LineSprites`)**: not ported. Spec from `touch.ts`:
  - Visible when `device.touch && touch.touchingAim && touch.touchAimLine`.
  - `maxRange = barrelLength + BulletDefs[bulletType].distance` (guns; 30 default),
    clamped to `sqrt(zoom*1.414*zoom)`; ray-cast against `map.obstaclePool`
    (`active && !dead && height >= GameConfig.bullet.height && collidable &&
    !isWindow && sameLayer`, throwables also `height > projectile.maxHeight`)
    via `collider.intersectSegment`, shrinking `maxRange` at the first hit.
  - Dots every `1.5` from `3.5`, `dot.img`, scale `(1/32)*0.375`, container
    `addPIXIObj(container, activePlayer.layer, 19, 0)`, alpha 0.3.
- **Mouse/keyboard path** (`input.ts`, `inputBinds.ts`): omitted; matters only for
  emulator/BT-keyboard/controller play. Also the web `input.ts` mouse-aim maps to
  the desktop HUD (`ui-right-center-desktop`).
- **`ui2.ts` mobile breakpoint (`UiLayout.Sm`)**: the native port hard-assumes the
  mobile layout; the desktop variant (leaderboard, ability keys, tab strip) is
  not ported.

### 3.3 Simulation/render parity items (small, known)

- **`Map::renderTerrain` order-2 patches / `useAsMapShape`** — order 0/1 plus
  rivers are drawn; confirm no order-2/`mapRender`-only shapes are missing for
  every shipped map.
- **Structures**: `Map::update` deliberately does no structure rendering (matches
  the web client's debug-only path), but the `Structure` barn's mask/stairs are
  used for sorting — verify all structure collision queries used by
  `insideStructureMask`/`isUnderground` cover every map.
- **`touchPadDetectMult`/safari offsets, tablet offsets** — the web client has
  iOS/tablet-specific pad offsets; the native port keeps the plain landscape
  offsets (fine for Android, note it if iOS is ever built — plan D4).
- **`device.ts` parity**: resolution tiers/`pixelRatio`-driven font sizes are
  simplified in `ui/Device.*`.

### 3.4 Audio gaps (`M6_AUDIO.md` §"Remaining")

- **Per-def event sounds not wired**: bullet/weapon fire, footsteps, reload,
  pickup, gas ticks, UI clicks, loot drop, plane engine. The mixer supports
  `playSound`/`playGroup`; only these callers exist today:
  explosion (`explosion_01`, in `GameWorld::applyUpdate`),
  ceiling break (`ceiling_break_01`), player melee hit/swing
  (`PlayerRender.cpp` `animPlaySound`/`animMeleeCollision`).
- **`hits` channel** (obstacle punch `punchSound` groups / `playerHit`) — ported
  for melee only; bullet hits are not.
- **Reverb/filters**: `AudioManager` computes the filter but `AxmolAudioBackend`
  doesn't apply cathedral/cave reverbs yet.
- **Music tracks** (`ambiance.ts` music list) — mixer exists, track set is
  partial.

### 3.5 UI gaps (`M7_REMAININGS.md` §4)

> **Superseded by the UI split** (`ARCHITECTURE.md`): most of this list stops
> being C++ work. The gameplay HUD items move to the FairyGUI `HUD_pkg`; the
> menu/loadout/team items become ordinary Android screens. Keep the list only as
> a checklist of *features* the new screens must reach.

- **On-device verification of the full loop** was still outstanding: menu →
  Play Solo → join → HUD → pause → Quit → menu, plus the team-room flow, with an
  empty `logcat -b crash`. Do this first.
- **Loadout**: emote/crosshair icons have no atlas frames (text placeholders);
  the web crosshair pane (colour/size/stroke) and the drag-and-drop emote wheel
  are not ported; no item-confirm modal. *(→ `LoadoutFragment` + `HUD_pkg`
  crosshair icon.)*
- **Team screen**: invite-link copy button, per-player kick, in-game rename,
  `#msg-wait-reason` for non-leaders (Play is just hidden). *(→ `TeamFragment`.)*
- **HUD**: team/faction colours on the alive counter, kill leader panel,
  spectate options, weapon-slot strip (4 slots), pause-menu keybind tab/tab strip.
  *(→ `HUD_pkg`; the keybind tab is Java.)*
- **Menu**: language list modal (currently a cycle button), persist loadout
  paging/category, left-column social/ads columns intentionally omitted.
  *(→ `MenuFragment`/`LoadoutFragment`.)*
- **`pieTimer.ts`, `opponentDisplay.ts`, `profileUi.ts`, `pass.ts`,
  `menuModal.ts`** — not ported (profile/pass/opponents are M8 territory, and
  they are Java screens under the new split).

### 3.6 Platform / release (M8 + M9)

- **M8 accounts/API**: `account.ts` login/profile, team links, quests/pass,
  `proxy.ts` detection, `pingTest.ts` region latency, `errorLogs.ts` reporting.
- **Lifecycle hardening**: `AppDelegate` pause/resume closes the socket and
  rejoins; verify rejoin-on-resume under real backgrounding, plus
  reconnect/backoff on flaky mobile networks.
- **Perf/polish**: 60 fps on mid-range devices, notch/insets, back-button
  handling (menu instead of exit), battery/resolution tiers, crash-free soak.
- **Store-ready APK/AAB**, and decide D4 (iOS — near-free once the C++ port is
  cross-platform; currently `proj.android` only).

---

## 4. Verification workflow (unchanged)

```powershell
cd E:\work\survev\client-mobile
. .\tools\android-env.ps1
# host tests
cmake -S tests -B build-tests -G "Visual Studio 17 2022" -A x64
cmake --build build-tests --config Release; .\build-tests\Release\surv_tests.exe
# device (x86_64 emulator + dev server via adb reverse)
.\tools\sync-content.ps1
.\tools\build-native.ps1 -Abis "x86_64"
.\tools\build-apk.ps1 -Abis "x86_64"
& $adb reverse tcp:8000 tcp:8000; & $adb reverse tcp:9000 tcp:9000
& $adb install -r .\SurvevMobile-debug.apk
```

Regenerate (never hand-edit) after touching `shared/defs` or web sources:
`codegen_defs.mjs`, `codegen_render_defs.mjs`, `codegen_sound_defs.mjs`,
`gen_reference.mjs`, `build-atlas.mjs --res low`, `build-gui-icons.mjs`.

**Do NOT regress** (carried through every status doc):
adapter reparenting, wrapper node ownership, convex-fan fills, `axslc`
packaging, `resources.arsc` uncompressed+aligned, layer-index clamping,
quoted `-DANDROID_ABI`. During the UI migration also keep: the Java build
pipeline Gradle-free (vendor OkHttp jars), `assets/fgui/**` packaged alongside
`assets/axslc/**`, and `surv_tests` green with `src/app/UiBridge.cpp` excluded
from the host build (see `ARCHITECTURE.md` §4.1, §6.2).

---

## 5. Suggested next queue

0. **S1 of the UI migration (`ARCHITECTURE.md` §7)** — the `UiHost` interface,
   `src/app/UiBridge.cpp`, `SurvBridge.java`, the Java UI container in
   `AppActivity`, and the config round-trip. Everything UI-shaped now depends
   on this seam existing.
1. **Touch aim-line dots + real pad sprites** — folds into S2 (FairyGUI
   `Touch_pkg`), the most visible M5 parity gap.
2. **On-device end-to-end pass** — menu → match → Quit, once Java drives the menu
   (S3); until then the current C++ menu still works.
3. **Minimap/big map + `mapIndicator`** — needed for team pings and airdrop markers.
4. **Audio event wiring** (bullets, footsteps, pickup, UI) — biggest feel gap.
5. **Emote/ping wheel + `emote` barn** — touch UX feature with existing defs.

---

## 6. Session log

### Session 1 — airdrop / plane / flare / shot barns

The first item of the §5 queue (in the previous revision) is done: the four
missing in-game barns were ported and wired, and the render def provider now
carries the data they need.

| Port | Where | Notes |
|---|---|---|
| `airdrop.ts` | `Airdrop` + `AirdropBarn` (`Barns.{h,cpp}`) | `ObjectType_Airdrop` was previously **dropped silently**; now registered. Chute sprite (per-biome `airdropImg`), `fallTicker`, the `rad = lerp((1-fallT)**1.1, 5, 12)` scale, `zOrd 1500`, the hide-on-landed rule, the land FX (10 `airdropSmoke` particles + 12 water ripples + `airdrop_crash_01/02`), and the chute/fall sounds (only for a live fall, not a join snapshot). |
| `plane.ts` | `PlaneObj` + `PlaneBarn` | Reconciled from `UpdateMsg.planes[]` each tick (8-unit server snap), the `planeVel` advance, the `actionComplete` elevation (rad ×1.25, alpha ×0.75, sound-range collapse), per-biome sprite (`map-plane-01.img` / snow `01x`) and sound (`plane_01`/`plane_02`, airstrike `fighter_01` + `map-plane-02.img`), the indoor/layer-1 alpha rule, and the airstrike-zone lifetime. |
| `flare.ts` | `FlareVisual` + `FlareBarn` | Flare-gun bullets split out of the bullet barn; swelling `part-flare-01.img` (easeOutExpo to `maxFlareScale`), tinted trail (`player-bullet-trail-02.img`) with the tracer alpha decay, distance-decay from `GameConfig.bullet.reflectDistDecay`, collision + burn-out fade. |
| `shot.ts` | `ShotObj` + `ShotBarn` | Per-shot hand recoil (`addRecoil` equivalent — the recoil now lives in the shot barn, not `GameWorld::applyUpdate`) and the `pullDelay` release. Sound selection (team/last/alt/Splinter/AP/high-velocity variants) is still pending the audio pass. |

Supporting codegen (`tools/codegen_render_defs.mjs` →
`src/render/GeneratedDefs.cpp`): map renders now emit `biome.airdrop`
(`planeImg`/`planeSound`/`airdropImg`), and the provider gained `plane(type)`,
`tracerColor(name)` and `bullet(type)` backed by `GameConfig.airdrop`,
`GameConfig.airstrike`, `GameConfig.tracerColors` and `BulletDefs`
(`PlaneDef` / `TracerColorDef` / `BulletRenderDef`). `tools/codegen_defs.mjs`
also emits the `GameConfig` scalars the ports read (`kAirdropFallTime`,
`kBulletReflectDistDecay`, `kBulletHeight`, `kProjectileMaxHeightGame`).

**Verification:** `surv_tests` **1746 assertions, 0 failures, 88 tests** with a
new `tests/test_objects.cpp` (6 tests: the new def tables, airdrop fall/land,
plane reconcile/elevate/retire, flare growth + burnout, dual-wield shot recoil,
and an airdrop through `UpdateMsg`). `dprint check` clean. On-device verification
is manual (the user runs the emulator/APK pass).
