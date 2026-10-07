# M7 UI (menu / loadout / team) — remaining work (handoff)

This is a self-contained work order for finishing **M7 (native UI port)** in a
new session. It records what already exists, the parts that must not be
regressed, the exact build/test workflow, and the remaining tasks with enough
spec to implement them. Read `plan.md` §"M7 / UI layer" and §10 decision **D1**
for the original plan; **D1 is resolved as Option B (native axmol UI) — do not
introduce a WebView.**

> **Scope reminder.** The web client keeps *everything* outside the canvas in
> HTML/CSS: `client/index.html` (menu + `#ui-game` overlay markup),
> `client/src/ui/menu.ts`, `loadoutMenu.ts`, `teamMenu.ts`, `localization.ts`,
> `touch.ts`, plus the `main.ts` glue (config, find_game, join, quit). M7 is the
> port of all of that to axmol widgets.

---

## 1. Current status

The **engine-independent UI foundation and the scene-level join flow are
implemented and host-tested**; the menu *screen* and the loadout/team *screens*
are not built yet, and none of it has been run on device.

- `surv_tests`: **1612 assertions, 0 failures, 75 tests** (green).
- `src/ui` modules exist: JSON, config, localization, device, widget toolkit,
  team client, and the in-game overlay.
- `src/net` gained the URL parser, site-info fetch, and the UI-facing WebSocket
  factory alias.
- `Game` carries the menu-supplied join identity; `GameScene` exposes a
  menu-driven enter/leave + find-game API with a multi-URL join fallback and the
  old `DevConfig` auto-connect **removed**.
- **Not built:** `MenuScene` (the actual menu screen), the config persistence
  binding to `ax::UserDefault`, `AppDelegate` wiring, the loadout screen, the
  team screen, and the full HUD (ammo/weapon/killfeed).
- **Never run on device** since the auto-connect was removed: with no
  `MenuScene` the app currently boots into a hidden game scene and does nothing.

---

## 2. Build & test workflow

```powershell
# Environment (must dot-source first; sets ANDROID_HOME/NDK/CMake 3.22.1/ninja)
cd E:\work\survev\client-mobile
. .\tools\android-env.ps1

# Host tests (engine-independent; no axmol)
cmake -S tests -B build-tests -G "Visual Studio 17 2022" -A x64
cmake --build build-tests --config Release
.\build-tests\Release\surv_tests.exe        # currently 75 tests / 1612 assertions, 0 failures

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
& $adb shell screencap -p /sdcard/s.png; & $adb pull /sdcard/s.png .\s.png
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

### `src/ui/` — engine-independent foundation + widget toolkit

| File | What it is |
|---|---|
| `Json.h` | Minimal JSON reader (`JsonValue` tree + `JsonParser::parse`), `\u`/surrogate → UTF-8. No axmol. |
| `Files.{h,cpp}` | axmol `FileUtils` wrapper: `readText/readBytes/readJson/exists/resolveRelative/fullPath`. Resolves Content-root-relative paths and `./`/`../` refs. |
| `Config.{h,cpp}` | Port of `client/src/config.ts`. `ConfigStorage` (`read`/`write` string) + `MemoryConfigStorage`; `Config` with typed getters/setters, `save()`, listeners; web defaults; `defaultLoadout()`. `StorageConfigStorage` (UserDefault) **still to add**. |
| `Localization.{h,cpp}` | Port of `client/src/ui/localization.ts`. 18 locales, `registerEnglish(json)`, `setLoader(loader)`, `setLocale`, `translate` (key → space-as-dash → English), `localeName`, `detectLocale`. |
| `Device.{h,cpp}` | Port of the parts of `client/src/device.ts` the UI needs (`mobile`/`tablet`/`touch`/`isLandscape`/`uiLayout`). |
| `Ui.{h,cpp}` | Widget toolkit (see §5): `kit::fromCssY/fromCss`, `Panel`, `Button`, `TextField`, `Slider`, `Modal`, `uiRoot`, `layoutUiRoot`, `swallowTouches`. |
| `TeamMenu.{h,cpp}` | `/team_v2` JSON control channel: `connect(create, roomUrl)`, `leave`, `update`, `setRoomRegion/AutoFill/GameMode`, `tryStartGame`, `onGameComplete`, `handleMessage`; callbacks `onRoomChanged/onError/onPlay/onLostConnection`; `teamErrorL10n`. |
| `UiOverlay.{h,cpp}` | In-game HUD + pause menu (see §5). |

### `src/net/`

| Change | Notes |
|---|---|
| `WsUrl.{h,cpp}` | `WebSocketEndpoint` + `parseWebSocketUrl` (scheme/host/port/path/secure, IPv6, default ports). Host-tested. |
| `SiteInfo.{h,cpp}` | `GameModeInfo`, `PopInfo`, `SiteInfo{modeIndexForTeamMode,hasTeamMode}`, `fetchSiteInfo(apiBaseUrl, cb)` over `ax::network::HttpClient` + rapidjson. |
| `WebSocketConnection.{h,cpp}` | Added `createWebSocketConnectionTo(url)`. It is a thin alias of `createWebSocketConnection` — axmol's `WebSocket::open` already accepts a full `ws://host:port/path` URL. Use it for both `/play` and `/team_v2`. |

### `src/game/Game`

```cpp
struct JoinInfo {
    std::string name = "Player";
    std::string outfit = "outfitBase";
    std::string melee = "fists";
    std::string heal = "heal_basic";
    std::string boost = "boost_basic";
    std::vector<std::string> emotes;
};
void setJoinInfo(const JoinInfo&);
const JoinInfo& getJoinInfo() const;
bool hasJoined() const;     // JoinedMsg or first UpdateMsg seen
bool isConnecting() const;
```

`sendJoinMessage()` builds the `JoinMsg` from `_joinInfo` (name/outfit/melee/
heal/boost/emotes). Defaults match `loadout.validate({})`.

### `src/app/GameScene` (menu-driven join flow; `maybeAutoConnect` removed)

```cpp
struct FindGameResultInfo { bool ok; std::vector<std::string> urls; std::string joinToken; std::string error; };
using FindGameRequest = std::function<void(const std::string& region, int gameModeIdx, FindGameDone done)>;

void setFindGameRequest(FindGameRequest);           // app wires this to net/Api findGame()
void setJoinInfo(const Game::JoinInfo&);            // identity for the next join
void enterWithJoin(urls, joinToken);                // direct join (find_game or team)
void enterWithFindGame(region, gameModeIdx);        // quick-start w/ backoff
void leaveGame();
bool isInGame()/isStarted()/isConnected()/isPlaying() const;
void setOverlay(ui::UiOverlay*);                    // HUD + pause menu
```

- `init()` no longer connects and hides the world root + joystick pads.
- `update()` services the quick-start delay, retries the **next URL with the same
  token** when the socket closes before the server accepted us, maps close codes
  → `index-*` l10n keys and otherwise `index-host-closed`, then drives the
  overlay.
- Close code map: 4001 invalid-token, 4002 invalid-protocol, 4003 invalid-packet,
  4004 behind-proxy, 4005 player-not-found, 4006 ip-banned, 4007 rate-limited,
  4008 server-crashed, 4009 server-restart, 4010 invalid-captcha,
  4011 failed-finding-game.

### Tests

`tests/test_ui.cpp` — 8 tests: JSON round-trip/reject, config defaults +
persistence, localization lookup/fallback, device layout, url parsing, team
protocol (create/state/setRoomProps/playGame/joinGame/error), keep-alive.
`tests/test_game.cpp::game_join_sends_joinmsg` now verifies a custom
`Game::JoinInfo` round-trips through the `JoinMsg`.

---

## 4. Remaining tasks (priority order)

### T1. `MenuScene` + config persistence + AppDelegate wiring — HIGHEST

This is what makes the app usable again after the auto-connect removal.

**`src/ui/Config` — add the UserDefault binding** (the only piece of the
foundation left):

```cpp
class UserDefaultConfigStorage : public ConfigStorage {
public:
    // ax::UserDefault keys: "surviv_config" (the JSON blob). SharedPreferences
    // on Android, so it survives restarts like localStorage does on the web.
    std::string read() override;          // ud->getStringForKey("surviv_config", "")
    void write(const std::string&) override;  // ud->setStringForKey("surviv_config", ...); flush
};
```

**`src/app/MenuScene.{h,cpp}`** — a sibling of `GameScene`, built from the web
menu markup (`client/index.html` `#start-menu-wrapper`/`#start-menu` →
`#player-name-input-solo`, `#server-select-main`, `#btn-start-mode-0..2`,
`#btn-join-team`, `#btn-create-team`, `#btn-help`; plus the sound/mute + volume
controls from `main.ts tryLoad()`):

1. **Background**: full-screen sprite `img/splashes/main.webp` (the web client's
   default `cachedBgImg`), plus the `survev_logo_full.png` logo.
2. **Name field** (`TextField`, max length `net.Constants.PlayerNameMaxLen` = 16)
   → `config.setString("playerName", sanitized)`. Web sanitation lives in
   `helpers.sanitizeNameInput`; port a minimal version (trim, strip control
   chars, cap length).
3. **Region select** — from `SiteInfo.pops` keys (`config.get("region")`,
   default `"na"`); web uses `#server-select-main` fed by
   `siteInfo.pops`/`l10n`. Selecting sets `config.setString("region", ...)`.
4. **Play Solo / Duo / Squad** → the **game-mode index** is a `SiteInfo.modes`
   index, not a team mode: look up `modes` for `teamMode == 1|2|4` via
   `SiteInfo::modeIndexForTeamMode()` and call
   `gameScene->enterWithFindGame(region, modeIdx)`.
   - **Spinner + lockout + backoff**: `tryQuickStartGame` in `main.ts` — disable
     the buttons, show a spinner while pending, and skip the request unless
     `Date.now() - findGameTime > 30000` (the delay grows to
     `min(attempts*2.5s, 7500ms)` inside the scene). `GameScene` already applies
     the retry delay; the menu owns the button lockout/visibility.
   - **Errors**: `GameScene::setError` writes into `UiOverlay::setMenuError`, but
     the *menu* needs its own error line for pre-game failures — wire
     `onError`-style text from the find-game callback into a menu label.
5. **Join Team / Create Team** → `ui::TeamMenu` (`setUrl` = `wss?://<host>/team_v2`
   derived from the API URL; see §6), then a `TeamScene`/modal (T3).
6. **How to Play** modal (`Modal`) with the `index-controls` strings.
7. **Customize** stub button (opens the loadout screen in T2; a disabled stub is
   acceptable for the first pass).
8. **Sound toggle + Master/SFX/Music sliders** (same widgets/keys as
   `UiOverlay`: `muteAudio`, `masterVolume`, `soundVolume`, `musicVolume`) wired
   to `AudioManager::setMute/setMasterVolume/setSoundVolume/setMusicVolume`.
9. **Localization**: register English from `Content/l10n/en.json` via
   `Files::readText`, then `setLocale(config.get("language"))` with a loader over
   `Files::readText("l10n/" + locale + ".json")`.
10. **Wiring**: the menu creates/owns `ui::Config`, `ui::Localization`,
    `ui::TeamMenu`, `ui::UiOverlay`, and holds a pointer to the `GameScene`.
    `AppDelegate` runs `MenuScene` first; `MenuScene` switches to the game path
    when a match starts and switches back on `leaveGame()`/`onQuit`.

**Acceptance:** app launches to a native menu, Play Solo joins the dev server and
shows the HUD, Quit returns to the menu with no crash.

### T2. Loadout menu (`client/src/ui/loadoutMenu.ts`)

Server-authoritative, so no bundled item list is needed:

- **Items** = `unlock_default.unlocks` (from `shared/defs/gameObjects/unlockDefs.ts`)
  plus account items when accounts exist; every candidate is validated through
  the generated `DefProvider::gameObject(type)` (category `outfit`/`melee`/
  `heal_effect`/`boost_effect`).
- **Categories** (web order): outfit, melee, emote, heal, boost, player_icon
  (+ crosshair on desktop only — mobile skips it).
- **Sprites**: outfits render with `skin.baseSprite` + `skin.baseTint` (and the
  `baseTintRed`/`baseTintBlue` variants); melee/guns use `lootImg`. The atlas
  cache has the outfit base/hands/feet frames but **no `emote_*`/`crosshair_*`
  frames** — so the emote wheel shows text placeholders until a GUI atlas exists
  (see §6).
- **Selection** feeds `Game::JoinInfo` (`outfit`/`melee`/`heal`/`boost`/`emotes`).
- **Acceptance:** changing outfit/melee/heal/boost changes what the player looks
  like/uses on the next join.

### T3. Team screen (`client/src/ui/teamMenu.ts`)

- Wire `MenuScene` → `ui::TeamMenu::connect(create, roomUrl)`; render the roster
  (`TeamMenu::players()`), room code (`roomUrl()`), region/autoFill/gameMode
  properties (leader-only edits via `setRoomRegion/AutoFill/GameMode`), and a
  **Start Game** button (`tryStartGame()`).
- `onPlay` → `gameScene->enterWithJoin(match.urls, match.joinToken)`.
- `onError` → localized via `teamErrorL10n` + `Localization::translate`
  (`behind_proxy`/`banned` are surfaced to the main menu as in the web client).
- `GameScene` leaving a team game should call `TeamMenu::onGameComplete()`.
- **Acceptance:** create a room on device, join from the web client, see the
  roster, start a match.

### T4. HUD completion (`client/src/ui/ui2.ts`)

`UiOverlay` currently draws health/boost bars, health, alive count, and empty
weapon/ammo. Still needed:

- **Ammo/weapon**: active player's `weapons[curWeapIdx]` (type + ammo) from the
  active-player data; melee/throwable variants. Needs `Game` to expose the
  active-player inventory (add an accessor rather than reaching into
  `_lastUpdate`), then format with `game-hud-<type>` localization.
- **Kill feed** (`ui-killfeed-contents`, `maxKillFeedLines = 6`): consume
  `KillMsg` (currently deserialized and discarded in `Game::handleKill`).
- **Alive/team counts**: factions show per-team; derive from
  `UpdateMsg.playerInfos`/`playerStatus` rather than adding a wire field.
- **Pause menu polish**: touch style/aim line/sound labels re-sync on show (the
  values are already persisted by `UiOverlay`).
- **Acceptance:** HUD mirrors the web HUD for ammo/weapon/killfeed.

### T5. On-device verification (after T1)

Full loop, then record screenshots + `logcat -b crash` (must be empty):

```powershell
. .\tools\android-env.ps1
.\tools\build-native.ps1 -Abis "x86_64"
.\tools\build-apk.ps1 -Abis "x86_64"
& $adb reverse tcp:8000 tcp:8000; & $adb reverse tcp:9000 tcp:9000
& $adb install -r .\SurvevMobile-debug.apk
& $adb shell am force-stop com.survev.mobile
& $adb shell am start -n com.survev.mobile/dev.axmol.app.AppActivity
```

Drive with `adb shell input tap <x> <y>` / `swipe`; capture with
`screencap`/`pull`. Verify: menu renders → Play Solo finds + joins a game
(logcat `Connected to game server` / `Receiving game updates`) → pause menu →
Quit returns to the menu.

---

## 5. Widget toolkit notes (`src/ui/Ui.h`)

```
web                          -> native
<div class="menu-block">     -> ui::Panel      (LayerColor bg, top-left anchor)
<a class="btn-darken">       -> ui::Button     (Scale9Sprite + Label + badge)
<input type="text">          -> ui::TextField
<input type="range">         -> ui::Slider
#modal-... + overlay         -> ui::Modal      (dim LayerColor + centred Panel)
```

- **Coordinates**: geometry is authored in CSS-like units (1280×720) with y
  growing **down**; every helper flips via `kit::fromCssY(y) = 720 - y`. The
  scene root is scaled to the real display by `layoutUiRoot`.
- `Panel` anchor is `(0,1)`; `setCssPosition(x, y)` takes the **top-left** in CSS
  coords.
- `Button` installs its own touch listener (`setSwallowTouches(true)`, fires
  `onClick` when released inside bounds).
- `Slider` maps touch x → value in `onTouchBegan/Moved` and fires `onChanged`.
- `Modal` builds a dim overlay + centred panel + close button; `onShow`/`onHide`.

---

## 6. Key files & gotchas

- **Fonts are woff2-only** (`Content/fonts/roboto-condensed-latin-400/700-normal.woff2`);
  axmol cannot load woff2. The toolkit therefore uses `Label::createWithSystemFont`
  ("sans-serif"). Shipping a TTF (Roboto Condensed) and switching to
  `Label::createWithTTF` is the intended upgrade for exact web typography.
- **GUI art is not in an atlas.** `Content/img/gui/*.svg` (buttons, icons,
  `hamburger`, `cog`, `loadout-*`) is not covered by `tools/build-atlas.mjs`,
  which only converts the web *virtual* atlases (game/loot/map sprites) from
  `client/node_modules/.atlas-cache`. Either extend the atlas builder to also
  pack `public/img/gui/**` into a `gui` sheet, or keep drawing buttons with the
  translucent 9-slice/`DrawNode` fallbacks. `Button::init(label,w,h,bgFrame)`
  already accepts an optional atlas frame; pass `""` until the GUI sheet exists.
- **Atlas contents**: the cached atlases contain the `*.img` game frames
  (incl. `player-base-*`/`player-hands-*`/`player-feet-*`) but **no**
  `emote_*`/`crosshair_*` frames, so the loadout emote/crosshair categories
  cannot show real icons yet.
- **Protocol is bit-exact** (`tests/ReferenceData.h` fixtures are generated from
  the TS server, `node tools/gen_reference.mjs`). **Do not add fields to
  `UpdateMsg` or any message** without regenerating the fixtures *and* changing
  the server — the HUD must derive new values from existing fields.
- **`y_down` → `y_up`**: the world/render path is CSS-like y-down
  (`Camera::m_pointToScreen`), axmol is y-up; all UI geometry must go through
  `kit::fromCssY`.
- **Placement over the world**: the overlay/HUD nodes are added to the scene
  directly and are scaled by `layoutUiRoot`; keep them above the game layers
  (`UiOverlay` uses z 100 for the HUD, 200 for the pause menu).
- **Generated files** (`src/net/generatedDefs.inc`, `src/render/GeneratedDefs.cpp`,
  `src/audio/GeneratedSoundDefs.cpp`, `tests/ReferenceData.h`,
  `Content/atlas/**`) must be regenerated with the `tools/codegen_*.mjs` /
  `gen_reference.mjs` / `build-atlas.mjs` scripts, never hand-edited.
- **`Content/` is gitignored**; run `build-atlas.mjs --res low` before packaging.
- **Adapters**: any new engine API must be added to `src/render/PixiLike.h`
  **and both** implementations (`AxmolPixi.h`, `NullPixi.h`). The UI widgets are
  discrete `ax::Node` subclasses, not part of that adapter.

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

- `surv_tests` stays green.
- On device (x86_64 emulator, dev server via `adb reverse`): the app launches to
  the **native menu**; Play Solo finds and joins a match; the pause menu opens
  and returns to the menu; Quit Game leaves the match; a team room can be
  created/joined and can start a match; the loadout selection is sent in the
  `JoinMsg`.
- `adb logcat -b crash` is empty across the flows.
- No WebView, no DOM: all UI is axmol nodes.

---

## 8. Session log

### Session 1 — foundation + join flow (this session)

Implemented and host-verified (no device run yet):

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
- `test_game.cpp::game_join_sends_joinmsg` now sets a custom `JoinInfo` so the
  menu-supplied identity is verified.
- Reverted an experimental `UpdateMsg` wire field (it broke the bit-exact
  `update-full` fixture); the HUD derives the alive count from
  `Game::snapshot()` instead.

`surv_tests`: **1612 assertions, 0 failures, 75 tests**.

**Verified UI logic:** JSON parse/reject, config defaults + persistence,
localization fallback, device layout, URL parsing, team protocol
(create/state/setRoomProps/playGame/joinGame/error) and keep-alive,
custom join identity, and the existing net/render/audio suites.

**Next:** T1 — `UserDefaultConfigStorage`, `src/app/MenuScene.{h,cpp}`, and the
`AppDelegate` wiring.
