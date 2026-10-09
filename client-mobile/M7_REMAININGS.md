# M7 UI (menu / loadout / team / HUD) — remaining work (handoff)

This is a self-contained work order for finishing **M7 (native UI port)**. It
records what already exists, the parts that must not be regressed, the exact
build/test workflow, and the remaining tasks with enough spec to implement them.
Read `plan.md` §"M7 / UI layer" and §10 decision **D1** for the original plan;
**D1 is resolved as Option B (native axmol UI) — do not introduce a WebView.**

> **Scope reminder.** The web client keeps *everything* outside the canvas in
> HTML/CSS: `client/index.html` (menu + `#ui-game` overlay markup),
> `client/src/ui/menu.ts`, `loadoutMenu.ts`, `teamMenu.ts`, `localization.ts`,
> `touch.ts`, plus the `main.ts` glue (config, find_game, join, quit). M7 is the
> port of all of that to axmol widgets.

---

## 1. Current status

The native menu, loadout, team screen and HUD are **implemented, host-tested and
build clean for Android** (x86_64 `.so` + APK). The app boots into the native
menu; it has not yet been driven end-to-end on a device in this session.

- `surv_tests`: **1658 assertions, 0 failures, 79 tests** (green).
- `src/ui` modules: JSON, config (UserDefault persistence), localization, device,
  widget toolkit, team client, loadout, news, `StartMenu`, and the in-game
  overlay.
- `src/net`: URL parser, site-info fetch, and the UI-facing WebSocket factory.
- `Game` carries the menu-supplied join identity; `GameScene` exposes a
  menu-driven enter/leave + find-game API with a multi-URL join fallback and the
  old `DevConfig` auto-connect **removed**.
- **Built in this session:** the `MenuScene` chrome (see §3.2), the loadout
  screen, the team screen, the full HUD (weapon/ammo/killfeed/alive), the
  `ax::UserDefault` config binding, `AppDelegate` wiring, high-quality PNG GUI
  icons, the Roboto Condensed TTF, and permanent-landscape locking.
- **Remaining:** on-device verification of the full loop (see T5) and a few
  polish items listed in §4.

### What was added this session (files)

| File | What it is |
|---|---|
| `tools/build-gui-icons.mjs` | Rasterizes `client/public/img/gui/*.svg` to high-res PNGs (`Content/gui/<name>.png`, 512px @4x). Wired into `sync-content.ps1`. |
| `tools/fetch-fonts.ps1` | Converts the web Roboto Condensed woff2 to `Content/fonts/RobotoCondensed-{Normal,Bold}.ttf` (needs Python + fontTools + brotli). Wired into `sync-content.ps1`. |
| `src/ui/News.{h,cpp}` | Bundled news entries (the web `#news-block` list has no API); newest first. |
| `src/ui/StartMenu.cpp` | The menu chrome helper: splash + logo, centred `#start-menu` block, news box, bottom-right cog/mute, settings/help/join modals. Declared in `UiOverlay.h`. |
| `src/ui/LoadoutMenu.{h,cpp}` | Reworked to the web layout (category tabs, item grid, paging, selected highlight). |
| `src/ui/Ui.{h,cpp}` | Button now draws the web `.btn-*` body + 2px shadow and supports PNG icons; `makeLabel`/`fontPath` picks the TTF; `Modal::setCloseLabel`. |
| `src/app/MenuScene.{h,cpp}` | Thin scene wrapper: owns config/localization/team/overlay, drives `StartMenu`, team screen, matchmaking. |
| `src/app/AppDelegate.cpp` | Registers the GUI font at startup; landscape note. |
| `proj.android/app/AndroidManifest.xml` | `screenOrientation="landscape"` (permanent landscape). |

---

## 2. Build & test workflow

```powershell
# Environment (must dot-source first; sets ANDROID_HOME/NDK/CMake 3.22.1/ninja)
cd E:\work\survev\client-mobile
. .\tools\android-env.ps1

# Host tests (engine-independent; no axmol)
cmake -S tests -B build-tests -G "Visual Studio 17 2022" -A x64
cmake --build build-tests --config Release
.\build-tests\Release\surv_tests.exe        # currently 79 tests / 1658 assertions, 0 failures

# Native lib (one build dir per ABI; first build compiles the engine)
.\tools\build-native.ps1 -Abis "x86_64"     # native emulator (best for crash stacks)
.\tools\build-native.ps1 -Abis "arm64-v8a"  # device / Houdini emulator

# APK (Gradle-free; packages Content/** + axslc, stores resources.arsc uncompressed)
.\tools\build-apk.ps1 -Abis "x86_64"
# -> SurvevMobile-debug.apk

# Install / run / logs
$adb = Join-Path $env:ANDROID_HOME 'platform-tools\adb.exe'
& $adb install -r .\SurvevMobile-debug.apk
& $adb shell am force-stop com.survev.mobile
& $adb logcat -c
& $adb shell am start -n com.survev.mobile/dev.axmol.app.AppActivity
& $adb logcat -d -v brief
& $adb logcat -b crash -d -v brief                  # native crash backtraces
```

> **Adding source files:** `CMakeLists.txt` uses `file(GLOB_RECURSE ...)` for
> `src/*.cpp`, so a new file needs a re-configure:
> `.\tools\build-native.ps1 -Abis "x86_64"` does that for you (it re-runs CMake).
> The same applies to `tests/CMakeLists.txt`, which lists files explicitly.

### Content staging (required before packaging)

`Content/` is gitignored, so run the staging script after a fresh checkout:

```powershell
.\tools\sync-content.ps1
# - robocopy ../client/public -> Content
# - copies en.json -> Content/l10n/en.json
# - node tools/build-gui-icons.mjs --scale 4   -> Content/gui/<name>.png
# - pwsh tools/fetch-fonts.ps1                 -> Content/fonts/*.ttf
```

### Dev server + emulator networking

The dev server runs the API on `:8000` and a game process on `:9000+`
(`pnpm dev:server`; needs Postgres `:5432` + Redis `:6379`). Point the emulator
at the host without touching server config:

```powershell
& $adb reverse tcp:8000 tcp:8000
& $adb reverse tcp:9000 tcp:9000
```

`DevConfig.h` keeps `kApiBaseUrl = "http://127.0.0.1:8000"` as the default for
the menu's `site_info`/`find_game` calls; the returned join URL
(`ws://127.0.0.1:9000/play`) is mapped back by `adb reverse`. Join tokens are
single-use and expire in 10s, so **always use the discovery path** (never a
cached token).

### Debugging crashes

The arm64 emulator runs under **Houdini ARM translation** → backtraces are
useless. Build/install the **x86_64** APK (the emulator is x86_64) for real
stacks. Symbolize with the NDK:

```powershell
$sym = Join-Path $env:ANDROID_NDK_HOME 'toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-symbolizer.exe'
& $sym --obj=build-android-x86_64\libSurvevMobile.so --functions=linkage --inlines <addr>
```

---

## 3. What already exists (so nothing is redone)

### 3.1 `src/ui/` — engine-independent foundation + widget toolkit

| File | What it is |
|---|---|
| `Json.h` | Minimal JSON reader (`JsonValue` tree + `JsonParser::parse`), `\u`/surrogate → UTF-8. No axmol. |
| `Files.{h,cpp}` | axmol `FileUtils` wrapper: `readText/readBytes/readJson/exists/resolveRelative/fullPath`, plus `registerFonts`/`fontFace` for the bundled TTF faces. |
| `Config.{h,cpp}` | Port of `client/src/config.ts`. `ConfigStorage` + `MemoryConfigStorage` + `UserDefaultConfigStorage` (SharedPreferences); typed getters/setters, `save()`, listeners; web defaults; `defaultLoadout()`. |
| `Localization.{h,cpp}` | Port of `client/src/ui/localization.ts`. 18 locales, `registerEnglish`, `setLoader`, `setLocale`, `translate`, `localeName`, `detectLocale`. |
| `Device.{h,cpp}` | Port of the parts of `client/src/device.ts` the UI needs. |
| `Ui.{h,cpp}` | Widget toolkit (`§5`): `kit::fromCssY`, `Panel`, `Button` (web colours + PNG icons), `TextField`, `Slider`, `Modal`, `makeLabel`/`fontPath`, `uiRoot`, `layoutUiRoot`, `swallowTouches`. |
| `News.{h,cpp}` | Bundled news entries for the menu's news box. |
| `TeamMenu.{h,cpp}` | `/team_v2` JSON control channel (create/join/roster/props/playGame/keep-alive/errors). |
| `UiOverlay.{h,cpp}` | In-game HUD + pause menu (`§5`) and the `StartMenu` declaration. |
| `StartMenu.cpp` | Main-menu chrome builder (web `#start-menu-wrapper`). |
| `LoadoutMenu.{h,cpp}` | Native `#modal-customize` (category strip + item grid + paging). |
| `Loadout.{h,cpp}` | Port of `shared/utils/loadout.ts` (selection/validation/`joinInfo`). |

### 3.2 `src/app/`

| File | What it is |
|---|---|
| `MenuScene.{h,cpp}` | Starts the menu, owns config/localization/team/overlay, drives `StartMenu`, the team screen and matchmaking. |
| `GameScene.{h,cpp}` | Gameplay scene; `enterWithJoin`/`enterWithFindGame`/`leaveGame`, overlay, close-code errors. |
| `AppDelegate.cpp` | Creates `MenuScene` + `GameScene` (siblings), registers fonts, fixed 1280×720 landscape design resolution. |

### 3.3 Assets

| Path | How it is produced |
|---|---|
| `Content/gui/*.png` | `tools/build-gui-icons.mjs` (from `client/public/img/gui/*.svg`, 4x = 512px). |
| `Content/fonts/RobotoCondensed-*.ttf` | `tools/fetch-fonts.ps1` (from the web woff2). |
| `Content/img/**`, `Content/audio/**`, `Content/l10n/*.json` | `tools/sync-content.ps1`. |
| `Content/atlas/**` | `tools/build-atlas.mjs --res low`. |

---

## 4. Remaining tasks (priority order)

### T1. On-device verification (after every UI change) — HIGHEST

Full loop, then record screenshots + `logcat -b crash` (must be empty):

```powershell
. .\tools\android-env.ps1
.\tools\sync-content.ps1
.\tools\build-native.ps1 -Abis "x86_64"
.\tools\build-apk.ps1 -Abis "x86_64"
& $adb reverse tcp:8000 tcp:8000; & $adb reverse tcp:9000 tcp:9000
& $adb install -r .\SurvevMobile-debug.apk
& $adb shell am force-stop com.survev.mobile
& $adb shell am start -n com.survev.mobile/dev.axmol.app.AppActivity
```

Verify: menu renders (centred menu, news box to its right, cog/mute bottom-right)
→ Play Solo finds + joins a game (logcat `Connected to game server` /
`Receiving game updates`) → HUD shows weapon/ammo/killfeed → pause menu → Quit
returns to the menu. Keep screenshots few (each `screencap` is ~1 MB).

### T2. Loadout polish (`client/src/ui/loadoutMenu.ts`)

- The modal uses a text grid + the shared atlas frames. The web client renders
  each item with its SVG/atlas image and an outfit 3-variant tint row — the
  tint row works, but **emote/crosshair icons still have no frames** (the atlases
  have no `emote_*`/`crosshair_*` entries). Either extend `build-atlas.mjs` or
  keep text placeholders.
- The web crosshair pane (colour/size/stroke) and the emote drag-and-drop wheel
  are not ported; the native emote flow uses the "Emote slot N" button.
- Selection currently writes straight to config; the web client confirms new
  items (`#modal-item-confirm`) — not needed until accounts exist.

### T3. Team screen polish (`client/src/ui/teamMenu.ts`)

- Roster/region/mode/auto-fill/Play are wired; remaining: the invite link
  copy button, per-player kick, and the in-game rename flow.
- `onPlay` → `gameScene->enterWithJoin(match.urls, match.joinToken)` works;
  `GameScene` leaving a team game calls `TeamMenu::onGameComplete()`.
- The web client shows `#msg-wait-reason` for non-leaders; the native screen
  hides the Play button for non-leaders instead.

### T4. HUD completion (`client/src/ui/ui2.ts`)

`UiOverlay` draws health/boost bars, health, alive count, weapon/ammo and the
killfeed. Still needed:

- **Team/faction colours** on the alive counter (web `game-red-team`/`game-blue-team`).
- **Kill leader** + **spectate options** (web `#ui-kill-leader-*`,
  `#ui-spectate-options`).
- **Pause menu polish**: keybind tab and the tab strip (web `#btn-game-tabs`).
- Weapon slot strip (`#ui-weapon-container`, 4 slots) — the native HUD shows the
  active weapon only.

### T5. Menu polish

- **Language select** is a cycle button; the web client uses a `<select>` with
  18 locales. Fine for now, a list modal is nicer.
- **Social/ad columns**: the web `#left-column` (Discord/Wiki/Ko-fi + ads) and
  the featured-streamer block are intentionally omitted (no sites backend).
- **Persist the paging/category** of the loadout modal between opens (web keeps
  the sort option in `#modal-customize-sort`).

---

## 5. Widget toolkit notes (`src/ui/Ui.h`)

```
web                          -> native
<div class="menu-block">     -> ui::Panel      (LayerColor bg, top-left anchor)
<a class="btn-darken">       -> ui::Button     (web body colour + 2px shadow + PNG icon)
<a class="btn-green">        -> ui::Button     (setColors(#83af50, #5b7a38))
<a class="btn-hollow">       -> ui::Button     (setHollow(true[, selected]))
<input type="text">          -> ui::TextField  (setWhiteBackground for the name field)
<input type="range">         -> ui::Slider
#modal-... + overlay         -> ui::Modal      (dim LayerColor + centred Panel)
<span class="highlight">     -> ui::makeLabel(text, size, bold) + gold colour
```

- **Coordinates**: geometry is authored in CSS-like units (1280×720) with y
  growing **down**; every helper flips via `kit::fromCssY(y) = 720 - y`. The
  scene root is scaled to the real display by `layoutUiRoot`.
- `Panel` anchor is `(0,1)`; `setCssPosition(x, y)` takes the **top-left** in CSS
  coords. `placeCenter()` centres in the 1280×720 design rect.
- `Button` installs its own touch listener (swallows touches, fires `onClick` on
  release inside bounds); `setIcon("cog")` loads `Content/gui/cog.png`.
- `makeLabel(text, size, bold)` uses the bundled Roboto Condensed TTF when it is
  staged and falls back to the platform sans-serif otherwise.
- `Modal` builds a dim overlay + centred panel + a corner close button;
  `setCloseLabel` overrides its text.

### Menu layout (web parity)

The web client lays the main menu in a horizontally centred column (`#start-menu`
inside `#start-row-top`) with the news column (`#news-block`, 300 px) to its
right and `#start-bottom-right` (cog + mute) pinned to the bottom-right. The
native `StartMenu` reproduces exactly that at the 1280×720 design resolution.
The app is locked to landscape (`AndroidManifest.xml` `screenOrientation="landscape"`).

---

## 6. Key files & gotchas

- **Fonts**: the web client only ships woff2 (`Content/fonts/*.woff2`), which
  axmol cannot load. `tools/fetch-fonts.ps1` writes `RobotoCondensed-Normal.ttf`
  and `RobotoCondensed-Bold.ttf`; `Files::fontFace` resolves the family+weight
  and `ui::makeLabel` falls back to the system font when they are missing.
- **GUI art**: `Content/gui/*.png` is generated from `client/public/img/gui/*.svg`
  by `tools/build-gui-icons.mjs` (high-res, transparent). Do not hand-edit.
- **`ui` namespace collision**: axmol itself declares `ax::ui`, and several
  `src/app` files have `using namespace ax`. `GameScene.*` and `MenuScene.*`
  therefore alias the toolkit (`namespace uikit = ::ui;` / `svui` / `menuui`)
  instead of using `ui::` inside `namespace surv`. Keep that convention.
- **`surv::ui` is not the toolkit.** `UiOverlay.h` keeps legacy
  `namespace ui { class UiOverlay; }` forward declarations for the in-game
  overlay; the toolkit is the global `::ui`.
- **Protocol is bit-exact** (`tests/ReferenceData.h` fixtures are generated from
  the TS server, `node tools/gen_reference.mjs`). **Do not add fields to
  `UpdateMsg` or any message** without regenerating the fixtures *and* changing
  the server — the HUD must derive new values from existing fields.
- **`y_down` → `y_up`**: the world/render path is CSS-like y-down
  (`Camera::m_pointToScreen`), axmol is y-up; all UI geometry must go through
  `kit::fromCssY`.
- **Generated files** (`src/net/generatedDefs.inc`, `src/render/GeneratedDefs.cpp`,
  `src/audio/GeneratedSoundDefs.cpp`, `tests/ReferenceData.h`,
  `Content/atlas/**`, `Content/gui/**`, `Content/fonts/*.ttf`) must be
  regenerated with the `tools/*.mjs` / `tools/*.ps1` scripts, never hand-edited.
- **`Content/` is gitignored**; run `sync-content.ps1` + `build-atlas.mjs --res low`
  before packaging.
- **New `.cpp` files need a CMake re-configure** (the glob is evaluated at
  configure time) — `build-native.ps1` re-runs configure for you.

### Do NOT regress (carried from `M4_REMAINING.md` §1)

1. Reparenting semantics in both adapters (`addChild` removes from the old
   parent first).
2. Wrappers own their native node (`ownNode()` + `release()` in the destructor).
3. `drawSolidPoly` / `endFill` use the convex-fan path (never axmol's poly2tri).
4. `build-apk.ps1` keeps packaging `assets/axslc/**` and storing
   `resources.arsc` uncompressed + aligned.
5. `Renderer::addPIXIObj` clamps the layer index to `0..3`.
6. `build-native.ps1` quotes `-DANDROID_ABI=$abi`.

---

## 7. Acceptance for M7

- `surv_tests` stays green (currently 1658 assertions / 79 tests, 0 failures).
- The Android x86_64 build links and the APK packages `assets/gui/**`,
  `assets/fonts/*.ttf`, `assets/img/**`, `assets/atlas/**`, `assets/axslc/**`.
- On device (x86_64 emulator, dev server via `adb reverse`): the app launches to
  the **native menu** (centred menu block, news box to its right, cog + mute
  bottom-right, permanent landscape); Play Solo finds and joins a match; the HUD
  shows weapon/ammo/killfeed/alive count; the pause menu opens and returns to the
  menu; Quit Game leaves the match; a team room can be created/joined and can
  start a match; the loadout selection is sent in the `JoinMsg`.
- `adb logcat -b crash` is empty across the flows.
- No WebView, no DOM: all UI is axmol nodes.

---

## 8. Session log

### Session 1 — foundation + join flow

Implemented and host-verified:

- `src/ui/{Json.h, Files.{h,cpp}, Config.{h,cpp}, Localization.{h,cpp},
  Device.{h,cpp}, Ui.{h,cpp}, TeamMenu.{h,cpp}, UiOverlay.{h,cpp}}` and
  `src/net/{WsUrl.{h,cpp}, SiteInfo.{h,cpp}}`; `tests/test_ui.cpp` (8 tests).
- `createWebSocketConnectionTo(url)` in `src/net/WebSocketConnection.{h,cpp}`.
- `Game::JoinInfo` (+ `setJoinInfo`/`hasJoined`/`isConnecting`);
  `sendJoinMessage` now takes name/loadout/emotes from the menu.
- `GameScene`: removed `maybeAutoConnect`; added `enterWithJoin`,
  `enterWithFindGame`, `leaveGame`, `setFindGameRequest`, `setOverlay`,
  `setJoinInfo`, plus the multi-URL join fallback and localized close-code
  errors.

### Session 2 — menu / loadout / team / HUD (this session)

Implemented and host-verified (+ Android build):

- **`StartMenu`** (`src/ui/StartMenu.cpp`): splash + `#start-overlay` wash, white
  logo (`survev_logo_full.png`, 220 px), the **centred** `#start-menu` block
  (name field, region cycle, Play Solo/Duo/Squad, Join/Create Team, Loadout,
  How to Play, error line), the **news box to its right** (bundled `News.cpp`
  entries with gold headers / grey dates / highlight styling), and the
  **bottom-right cog + mute** icons (`Content/gui/cog.png`, `audio-on/off.png`).
  Settings modal (language cycle + sound toggle + 3 volume sliders), How to Play
  modal (touch control rows), Join Team modal, and the loadout modal.
- **Loadout screen** reworked to the web layout: 6 category tabs with an active
  highlight, a 12-item paged grid with badges + outfit tint previews, an emote
  slot selector.
- **Team screen** polish: `#<code>` invite code, invite-code status line, roster
  with leader marker `*` and "(in game)" state, region/mode/auto-fill props
  (leader-only), green Play button, Leave Team.
- **HUD**: weapon/ammo (+reserve) from `activePlayerData`, killfeed from
  `Game::killFeed()`, alive/faction counts from `Game::aliveCounts()`, and the
  result panel. All HUD + menu labels now use the Roboto Condensed TTF when
  staged.
- **Web-exact buttons**: `.btn-green` (`#83af50`/`#5b7a38`), `.btn-darken`
  (`#7a7a7a`/`#3e3e3e`), 2 px bottom shadow, translucent `menu-block` panels
  (`rgba(0,0,0,.5)`), the white bold name field, gold news headers.
- **High-quality PNG icons**: `tools/build-gui-icons.mjs` rasterizes all
  `client/public/img/gui/*.svg` to `Content/gui/*.png` at 4x (512 px, alpha).
- **Roboto Condensed TTF**: `tools/fetch-fonts.ps1` (Python fontTools) converts
  the web woff2 to `Content/fonts/*.ttf`; `AppDelegate` registers it at startup
  and `ui::makeLabel` uses it everywhere.
- **Permanent landscape**: `AndroidManifest.xml` `screenOrientation="landscape"`
  plus the fixed 1280×720 `NO_BORDER` design resolution.
- **Build fixes for the axmol `ui` namespace collision**: `GameScene.*` /
  `MenuScene.*` alias `::ui` (`svui` / `menuui`); `Ui.cpp` includes `Files.h`
  for the TTF; `StartMenu` lives in `UiOverlay.h` so `MenuScene.h` never names
  the widgets.

**Verified:** `surv_tests` **1658 assertions, 0 failures, 79 tests**;
`libSurvevMobile.so` links for x86_64; `SurvevMobile-debug.apk` (45.0 MB)
packages `assets/gui/*.png`, `assets/fonts/RobotoCondensed-*.ttf`, the web
assets and `assets/axslc/**`.

**Next:** T1 — on-device verification of the full loop (menu → Play Solo → HUD →
pause → Quit), then the §4 polish items.
