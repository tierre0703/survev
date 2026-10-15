# Architecture: axmol + FairyGUI gameplay, Android Java UI

This document replaces the UI sections of `plan.md` §3/§5.6 and supersedes
**decision D1** there (`M7_REMAININGS.md` §4 already listed native axmol UI as
the answer; this document changes it again).

> **Decision (2026-10-14).** `client-mobile` is split into two UI worlds:
>
> * **axmol (C++17) renders gameplay only** — the world, the camera, the actors,
>   the touch joysticks and the in-game HUD (designed in **FairyGUI**).
> * **Android Java (View system) owns every out-of-game screen** — main menu,
>   settings, loadout, team room, profile/pass, modals, error dialogs.
>
> The two halves talk over a **single JNI bridge**; there is exactly one
> `Activity` (the axmol `AppActivity`) with the `GLSurfaceView` and the Java UI
> in the same `FrameLayout`.

The four supporting decisions taken with it:

| # | Decision | Value |
|---|---|---|
| A1 | Java scope | Full UI split: everything outside the game canvas is Java. |
| A2 | FairyGUI role | Gameplay overlay toolkit (HUD, pause, killfeed, pads, crosshair). Replaces the hand-rolled `ui::` widget toolkit in-game. |
| A3 | Networking | Java owns HTTP + plain WebSockets (site info, `find_game`, `team_v2`). C++ keeps the `/play` binary game socket. |
| A4 | Surface model | One `Activity`; Java `View`s overlay the axmol surface (and vice-versa). |

---

## 1. Why this split

| Concern | Today (`src/ui/*` in C++) | With the split |
|---|---|---|
| Menu/loadout/team | ~2,400 lines of hand-rolled widget code re-implementing DOM+CSS (`Ui.cpp`, `StartMenu.cpp`, `LoadoutMenu.cpp`, `TeamMenu` screen, `UiOverlay.cpp`), plus a homegrown 1280×720 "CSS canvas" re-scaled into the real display. | Standard Android XML/`View`s. Text is real text (IME, fonts, accessibility, `robolectric`-style UI tests), lists scroll, dialogs are platform dialogs, and layout adapts to real screen sizes/DPIs. |
| Gameplay HUD | Mixed into `UiOverlay` (`DrawNode` rectangles + `Label`s). | **FairyGUI** package (`HUD_pkg`) authored in the FairyGUI Editor and rendered by the axmol runtime — resolution-independent, animated, artist-editable, and the same `.fui` can be exported for the Java screens if we ever want pixel parity. |
| Two coordinate systems | CSS y-down vs axmol y-up, plus a global scale factor, plus a custom hit-test walker (`localPointFromDesign`). | Gameplay UI lives in one coordinate system (`Camera`/design space). Java screens live in DP. No cross-system hit-testing. |
| Text input | `ax::ui::TextField` + a native IME bridge. | `EditText`, free. |
| Localization | `ui::Localization` (JSON) + per-label plumbing. | Java `Resources`/`values-*/strings.xml` generated from the same `l10n/*.json` by a tool (`tools/gen-l10n-json.mjs`), or `Localization` kept in C++ behind the bridge and Java asks for keys. See §6.3 — we keep **one** source (`client/public/l10n/*.json` + `client/src/en.json`) and generate both. |
| Effort left in M7 | Large (`M7_REMAININGS.md` §4: T2/T3/T4/T5 polish lists). | Most of it disappears: it is Android's job now. |

What we **keep** in C++ (it is gameplay, not UI):

* `src/game/*` — `Game`, `GameWorld`, `Map`, `Gas`, `TouchInput`, all barns.
* `src/render/*` — `Renderer`, `Camera`, `Terrain`, `PixiLike`/`AxmolPixi`, defs.
* `src/net/*` — the bit-exact protocol, `Connection`, `WebSocketConnection`,
  `Messages`, `Net`, and `Game`'s join/update flow.
* `src/audio/*` — `AudioManager`, `Ambiance` (volume/mute state now driven from Java).
* `src/core/*` — `BitBuffer`, `Vec2`, `Collider`, `GameConfig`.
* `src/ui/Touch.h` — the joystick *logic* (engine-independent). Its rendering
  moves to FairyGUI.
* `src/ui/Config.{h,cpp}`, `Loadout.{h,cpp}`, `Localization.{h,cpp}`,
  `Json.h`, `Files.*`, `Device.*` — engine-independent ports of web modules.
  These do **not** move into Java; Java calls them through the bridge (§5.3), so
  the config blob, loadout validation and l10n lookup keep exactly one
  implementation and stay covered by `surv_tests`.

What we **delete** from C++ (see §9): `Ui.{h,cpp}`, `StartMenu.cpp`,
`LoadoutMenu.{h,cpp}`, `UiOverlay.{h,cpp}`, `News.{h,cpp}`, `Layout.{h,cpp}`,
`menu`-side `StartMenu`/`AppDelegate` scene wiring, and the axmol-only
`net/SiteInfo.cpp` / `net/Api.cpp` HTTP paths once Java owns them.

---

## 2. Target architecture

```
┌──────────────────────────────────────────────────────────────────────────────┐
│ AppActivity : AxmolActivity                    (one Activity, landscape)      │
│                                                                              │
│  FrameLayout (AxmolActivity.mFrameLayout)                                    │
│   ├── GLSurfaceView (axmol)          ── gameplay only                         │
│   │     ├── Renderer/Map/objects     ── world                                 │
│   │     ├── Touch pads (FairyGUI)    ── joysticks, aim line, crosshair        │
│   │     └── HUD_pkg (FairyGUI)       ── health/boost/ammo/killfeed/leaderboard│
│   │                                                                          │
│   └── ui:FrameLayout (id=surv_java_ui)   ── Java UI, above the GL surface     │
│         ├── MenuFragment      (start menu, settings, news, how-to-play)       │
│         ├── LoadoutFragment   (categories, item grid, emotes, crosshair)      │
│         ├── TeamFragment      (roster, invite code, region/mode, play)        │
│         ├── AccountFragment   (login/profile/pass — M8)                       │
│         └── Shared: ErrorDialog, LoadingOverlay, RegionSpinner, ...           │
│                                                                              │
│  SurvBridge (JNI, one class)  ⇄  src/app/UiBridge.cpp (C++ facade)            │
└──────────────────────────────────────────────────────────────────────────────┘
```

### 2.1 Who owns which screen

| Screen (web) | Java / FairyGUI | Notes |
|---|---|---|
| `#start-menu-wrapper`, splash, news, cog/mute, settings/help modals | **Java** `MenuFragment` | the whole current `StartMenu.cpp` |
| `#modal-customize` (loadout) | **Java** `LoadoutFragment` | uses the C++ `Loadout` port via the bridge for validation/`joinInfo` |
| `#team-menu` (team room) | **Java** `TeamFragment` + OkHttp `TeamSocket` | the JSON control channel moves to Java (A3) |
| `#ui-game` HUD (`ui2.ts`) | **FairyGUI** `HUD_pkg` | health/boost, ammo, weapon strip, alive count, killfeed, kill leader, result panel |
| `#ui-game` pause menu | **Java** `GamePauseDialog` over the surface (decision A4) | sliders/language/quit are all standard widgets; needs `onPauseGame`/`onResumeGame` on the bridge |
| touch joystick / aim line / crosshair / emote wheel | **FairyGUI** (`HUD_pkg` + `Touch_pkg`) | `ui/Touch.h` stays the source of truth for the math |
| account/profile/pass (M8) | **Java** | out of the axmol build entirely |

Rationale for the pause menu living in Java while the HUD is FairyGUI: the pause
menu is a settings surface (sliders, list of languages, links, quit) and it is
opened *rarely*; paying for Java views only while it is up is cheaper than
maintaining a second widget toolkit in C++, and it is the same surface the web
client shows outside the canvas.

---

## 3. The two UI halves in detail

### 3.1 FairyGUI gameplay UI (axmol side)

FairyGUI is a UI editor + runtime: you author components in the FairyGUI Editor,
export a **package** (`HUD.bin` + atlas), and the runtime (`fairygui` C++ port)
loads it from the asset root. Integration plan:

```
client-mobile/
  third_party/fairygui/          vendored C++ runtime (axmol backend)
  fgui/                          FairyGUI Editor project (.fairy)
    HUD_pkg/                     health, boost, ammo, weapon strip, killfeed,
                                 alive counter, kill leader, result panel
    Touch_pkg/                   move pad, aim pad, aim line, crosshair, pings
    Common_pkg/                  shared 9-slices, fonts, icons
  Content/fgui/                  exported packages (generated, gitignored like Content/)
tools/build-fgui.ps1             runs the editor's CLI/headless export into Content/fgui
```

Rules:

1. **`Content/fgui/**` is generated** — like `Content/gui/**` and `Content/atlas/**`,
   it is produced by `tools/build-fgui.ps1` and must never be hand-edited.
   `tools/sync-content.ps1` gains a call to it (or a warning when the editor is
   absent, so the APK can still be built without it).
2. **FairyGUI owns only gameplay overlay.** No menu logic, no HTTP, no net.
   `GameWorld`/`GameScene` push plain data into the HUD (`UiBridge`-style
   setters) and the HUD reads nothing else.
3. **Textures are shared with the web atlas.** FairyGUI Editor can reference
   files from `client/public/img/**`; the export packs them into
   `Content/fgui/**/*.png`. Where an image is already in `Content/atlas/**`
   (the web client's `*.img` sprites) reference the atlas frame instead of
   duplicating the bitmap.
4. **The adapter rule still applies.** Nothing outside `src/render/AxmolPixi.h`
   may touch axmol draw APIs directly; the FairyGUI backend binds to the
   engine, and the game code keeps talking to *game data*, not widgets.
5. **`PixiLike` is untouched by this change.** The world rendering path
   (`Renderer`, `Barns`, `Map`) is not affected; FairyGUI renders onto its own
   `GRoot` stage ordered above the world layers and below the Java UI.

The `TouchPadGfx` class in `GameScene.cpp` (today: `pad.img` sprites) becomes a
thin adapter that drives `Touch_pkg` components instead of two `ax::Sprite`s.
`ui/Touch.h` is unchanged: it keeps producing
`setCenterPos/setTouchPos/setCenterScale/setTouchScale`, backed by a `PadGraphics`
implementation that sets FairyGUI object properties.

### 3.2 Java UI (Android side)

```
proj.android/app/
  res/layout/
    activity_surv.xml          FrameLayout: <GLSurfaceView holder> + <FrameLayout id="surv_java_ui">
    fragment_menu.xml          start menu (name, region, Play Solo/Duo/Squad, Join/Create Team,
                               Loadout, How to Play, news list, cog/mute)
    fragment_loadout.xml       TabLayout + RecyclerView + pager
    fragment_team.xml          roster RecyclerView + props + Play/Leave
    dialog_settings.xml, dialog_help.xml, dialog_join_team.xml, dialog_pause.xml
  res/values/strings.xml       generated from l10n/en.json (see §6.3)
  res/values-*/strings.xml     generated from client/public/l10n/*.json
  src/dev/axmol/app/
    AppActivity.java           (exists) + JavaUiHost: creates the Java overlay FrameLayout
    ui/JavaUi.java             the only class that touches the bridge: screen switching
    ui/MenuFragment.java
    ui/LoadoutFragment.java
    ui/TeamFragment.java
    ui/GamePauseDialog.java
    ui/ErrorDialog.java
    net/SurvApi.java           OkHttp: /api/site_info, /api/find_game_v2 (port of net/Api.*, SiteInfo.*)
    net/TeamSocket.java        OkHttp WebSocket: ws(s)://<host>/team_v2 JSON channel (port of ui/TeamMenu.cpp)
    net/SessionStore.java      session cookie + config blob mirror (SharedPreferences)
    cfg/SurvConfig.java        typed wrapper over the same SharedPreferences key ("surviv_config")
    SurvBridge.java            `native` methods (see §4.1)
```

Rules:

1. **One Activity.** `AppActivity` keeps extending `AxmolActivity` (so the
   `AxmolActivity.init()` `FrameLayout` + `GLSurfaceView` + `AxmolEditBox`
   plumbing keeps working) and `JavaUiHost` adds the `surv_java_ui` container
   **after** `super.onCreate()`, i.e. after `mFrameLayout` exists.
2. **Java never blocks the GL thread.** Every bridge call from Java is either
   fire-and-forget on the GL thread (`Director::getScheduler()->runOnAxmolThread`,
   the same mechanism `AxmolEngine.runOnGLThread` already uses) or a cheap
   query answered from a snapshot the C++ side publishes each frame
   (config/l10n/loadout are main-thread-safe anyway).
3. **The Java UI never talks to native gameplay objects directly.** It talks to
   `SurvBridge` (find/join/leave, config, l10n, loadout, audio, HUD numbers as
   needed by the pause dialog), never to `Game`/`GameWorld` pointers.
4. **Java UI is the "camera" for everything outside the canvas** (A1), so the
   `GLSurfaceView` stays visible only while a match runs. When the menu is up
   we *may* keep the GL surface alive (it shows the last game frame/black) but
   the Java overlay is opaque over it. Prefer: keep the surface alive and
   `setVisibility(INVISIBLE)` when the menu is fully covering it, to save power
   while still allowing instant resume.

---

## 4. The JNI bridge

### 4.1 Shape

One Java class, one C++ facade, one direction each.

```java
// dev.axmol.app.SurvBridge
public final class SurvBridge {
    // --- C++ entry points (implemented in src/app/UiBridge.cpp) ------------
    public static native void onMenuReady();                 // Java UI is built
    public static native void onQuickStart(int teamMode);     // 1/2/4
    public static native void onJoinUrls(String[] urls, String joinToken);
    public static native void onCancelPending();              // "Back to main menu"
    public static native void onQuitGame();                   // leave match
    public static native void onResumeGame();                 // pause dialog closed
    public static native void onPauseGame();                  // pause dialog opened
    public static native void setPlayerName(String name);
    public static native void setRegion(String region);
    public static native void setLoadout(String loadoutJson);
    public static native void setAudio(String key, float value); // master/sound/music/mute
    public static native void setLanguage(String locale);

    // --- read-only queries answered synchronously --------------------------
    public static native String getConfigJson();      // the whole config blob
    public static native String getL10nJson();        // current locale table (menus)
    public static native String getLoadoutJson();     // validated loadout
    public static native String getSiteInfoJson();    // cached /api/site_info
    public static native boolean isInGame();
    public static native void setEffect(String effect, int value); // haptics/notch etc. later

    // --- called from C++ (JNI, on the GL thread) ---------------------------
    static void onNativeMatchStarted(int activePlayerId) { JavaUi.get().enterGame(); }
    static void onNativeMatchEnded(String errorL10nKey)  { JavaUi.get().returnToMenu(errorL10nKey); }
    static void onNativePlayerDead()                     { JavaUi.get().showResult(); }
    // ... plus onNativeSiteInfo(...), onNativeJoinFailed(...) when Java drives net
}
```

`src/app/UiBridge.cpp` implements both directions:

* **Java → C++** are `JNIEXPORT ... Java_dev_axmol_app_SurvBridge_<name>` symbols
  in our own `.so` (nothing to patch in axmol; `JNI_OnLoad` in axmol's
  `javaactivity-android.cpp` simply won't find them there but the dynamic linker
  resolves them normally as long as they are `extern "C"` and exported).
* **C++ → Java** uses `ax::JniHelper::callStaticVoidMethod("dev/axmol/app/SurvBridge", "onNativeMatchStarted", id)`
  (axmol already ships `JniHelper` with a typed `TypeSignature` layer). **The
  bridge must therefore be built only in the Android app build**, exactly like
  `AxmolAudioBackend.h` is today: `src/app/UiBridge.cpp` is behind
  `#if (AX_TARGET_PLATFORM == AX_PLATFORM_ANDROID)` and the host tests never see
  it. Keep the *interfaces* engine-free in `src/app/UiHost.h` so `GameScene` can
  be host-tested with a fake host (same trick as `FakeConnection.h`).

### 4.2 Threading rules (must not be broken)

| Direction | Thread | Rule |
|---|---|---|
| Java UI → C++ (`onQuickStart`, `set*`) | UI thread | Post to the GL thread via `runOnAxmolThread` before touching engine state, *except* the pure-data setters (config/l10n/loadout/audio) which are already main-thread-safe plain structs. |
| C++ → Java (`onNative*`) | GL thread | `JniHelper` acquires the JVM env; the Java handler must `Activity.runOnUiThread` before touching Views. |
| Queries (`getConfigJson`, ...) | UI thread | Answer from a C++ snapshot; never block on the network. |

**Never** call into `Game`/`GameWorld` from the UI thread without marshalling,
and **never** touch Views from the GL thread.

### 4.3 Lifecycle

`AppDelegate` stays the single owner of `MenuScene`/`GameScene` transitions, and
gains a `UiHost*` member:

```cpp
class UiHost {                       // src/app/UiHost.h (engine-free interface)
public:
    virtual ~UiHost() = default;
    virtual void showMenu() = 0;     // Java: back to the menu fragment
    virtual void enterGame(int activePlayerId) = 0;
    virtual void returnToMenu(const std::string& errorL10nKey) = 0;
    virtual void showResult() = 0;
};
void setUiHost(UiHost* host);        // installed by UiBridge.cpp at startup
```

`AndroidUiHost` (Android-only) forwards to `SurvBridge`'s static handlers.

---

## 5. Move matrix

### 5.1 C++ files → status

| File | Action | Replacement |
|---|---|---|
| `src/ui/Ui.{h,cpp}` | **delete** | Android Views + `ui::` rewrite of gameplay widgets in FairyGUI bindings |
| `src/ui/StartMenu.cpp` (+ `StartMenu` in `UiOverlay.h`) | **delete** | `MenuFragment.java` |
| `src/ui/LoadoutMenu.{h,cpp}` | **delete** | `LoadoutFragment.java` |
| `src/ui/UiOverlay.{h,cpp}` | **split**: HUD → FairyGUI, pause → Java | `HUD_pkg` + `GamePauseDialog.java` |
| `src/ui/Layout.{h,cpp}` | **delete** | DP/XML; the CSS colour parser moves to a small `src/ui/CssColor.*` used by nothing but l10n tooling |
| `src/ui/News.{h,cpp}` | **keep as data**, rendered by Java | `NewsFragment` reads `getNewsJson()` |
| `src/ui/TeamMenu.{h,cpp}` | **move to Java** (`TeamSocket.java`); the C++ class stays for the host tests until the Java one is verified | one of the two must be deleted afterwards |
| `src/ui/Config.{h,cpp}`, `Loadout.{h,cpp}`, `Localization.{h,cpp}`, `Json.h`, `Device.{h,cpp}`, `Files.{h,cpp}` | **keep in C++**, exposed through the bridge | Java uses `SurvBridge.getConfigJson()/getL10nJson()/getLoadoutJson()` |
| `src/ui/Touch.h` | **keep** (logic) | `PadGraphics` implemented over FairyGUI |
| `src/net/Api.{h,cpp}`, `src/net/SiteInfo.{h,cpp}` | **keep for host tests**, app build routes through Java | `SurvApi.java` |
| `src/game/TouchInput.h`, `src/game/*`, `src/render/*`, `src/net/{Net,Messages,Connection,WebSocketConnection}`, `src/audio/*`, `src/core/*` | **unchanged** | — |
| `src/app/MenuScene.{h,cpp}` | **shrinks to a thin `GameScene` host**; the menu/matchmaking/team logic moves to Java | Java `MenuFragment` + `SurvApi` + `TeamSocket`; C++ keeps `find_game` retry/backoff only if Java does not take it |
| `src/app/GameScene.{h,cpp}` | **keep**, minus the overlay/touch-pad sprites → FairyGUI | `HUD_pkg`, `Touch_pkg` |
| `src/app/AppDelegate.cpp` | **keep**, gains the `UiHost` install + `Content/fgui` init | `UiBridge.cpp` |
| `src/app/DevConfig.h` | **keep**; the Java side gets the same values (generated or via `SurvBridge`) | — |

### 5.2 Matchmaking / team flow after the move

```
MenuFragment "Play Solo"
   └─ SurvApi.findGame(region, zones, gameModeIdx, version)        [OkHttp, Java]
        ├─ success → SurvBridge.onJoinUrls(urls, joinToken)
        │              └─ C++ GameScene.enterWithJoin()  → /play socket (C++)
        └─ error   → Java shows the localized error, retries with the same
                     backoff table as main.ts (findGameAttempts / findGameTime)
```
```
Create/Join Team
   └─ TeamSocket.connect(create, roomUrl)                          [OkHttp WS, Java]
        ├─ state  → TeamFragment roster/props
        ├─ joinGame → SurvBridge.onJoinUrls(urls, token) → C++
        └─ gameComplete on match end (C++ → Java callback → TeamSocket)
```
The C++ `GameScene` keeps only: join URL fallback, the `/play` socket, input
messages, and the map/update dispatch. Its `find_game` request injection
(`setFindGameRequest`) becomes unused in the Android build and is kept for the
host tests and the dev `DevConfig` path.

### 5.3 Config / loadout / l10n contract

* The blob key stays `surviv_config` in the same `SharedPreferences` file
  (`ax::UserDefault` writes there today). `SurvConfig.java` reads/writes the
  **same keys** so both halves see one config; the C++ `Config` remains the
  validator (defaults, `Loadout::validated`, `joinInfo`).
* Java edits a value → `SurvBridge.setX(...)` → C++ `Config::setX()` → persist.
  C++ edits a value (e.g. a HUD toggle) → `SurvBridge.onNativeConfigChanged(key)`
  → Java re-reads. Keep the listener plumbing in one place (`SurvConfig`).
* `getL10nJson()` returns the currently selected locale's flattened table, so
  Java can show any string the C++ side already knows. `values-*/strings.xml`
  (generated) cover the *static* Java screens.

---

## 6. Build, assets, tooling

### 6.1 Build

* **Native**: unchanged (`tools/build-native.ps1`); the new
  `src/app/UiBridge.cpp` + vendored FairyGUI sources are picked up by the
  existing `file(GLOB_RECURSE src/*.cpp)`; `third_party/fairygui` needs an
  explicit include dir + backend sources added to `CMakeLists.txt`.
* **Java**: `proj.android/app/src/**/*.java` already compiles in
  `tools/build-apk.ps1` (`Get-ChildItem proj.android\app\src -Recurse *.java`).
  Adding **OkHttp** introduces a Maven dependency the Gradle-free pipeline does
  not fetch — see §6.2.
* **Host tests**: `tests/CMakeLists.txt` must stay green and axmol-free. New
  engine-free pieces (the FairyGUI-agnostic HUD data model, the bridge
  interfaces) get host tests; `UiBridge.cpp` and the FairyGUI backend are
  excluded by the `SURV_UI_NO_AXMOL`/platform guards (mirror how
  `AxmolAudioBackend.h` and `WebSocketConnection.cpp` are handled).

### 6.2 OkHttp without Gradle (important)

The APK pipeline is Gradle-free (Astrill breaks the Gradle daemon, see
`BUILDING.md`). Options, in order of preference:

1. **Vendor the jars** into `proj.android/app/libs/` (OkHttp + Okio + Kotlin
   stdlib if the version needs it) and let `aapt2/javac/d8` link them; the
   pipeline already has an `app/libs` `fileTree` in `build.gradle` and
   `build-apk.ps1` can add `-classpath app/libs/*`. Pin versions in
   `proj.android/app/libs/README.md` so the set is reproducible.
2. **Drop to `java.net.HttpURLConnection` + `okhttp3`-free WebSocket**: Android
   has no built-in WebSocket client until API 34 (`android.net.http`), so a
   hand-rolled WS client is the fallback if vendoring is rejected.
3. Use Gradle when Astrill is off (the canonical path, documented in
   `BUILDING.md`).

> Whatever is chosen, `build-apk.ps1` must keep packaging `assets/axslc/**`,
> `assets/fgui/**`, storing `resources.arsc` uncompressed + aligned, and quoting
> `-DANDROID_ABI`.

### 6.3 Localization generation

Single source of truth stays the web client:

```
client/src/en.json + client/public/l10n/*.json
        │  tools/gen-l10n-json.mjs   (new)
        ├─► Content/l10n/*.json            (already staged; C++ Localization)
        └─► proj.android/app/res/values*/strings.xml   (Java static strings)
```
Android resource names may only contain `[a-z0-9_]`, so keys are transformed
(`index-play-solo` → `index_play_solo`); keep the transform in the generator and
document it in the header. Dynamic strings keep going through
`SurvBridge.getL10nJson()` / `Localization::translate` so there is exactly one
per-locale table in memory.

### 6.4 FairyGUI export

`tools/build-fgui.ps1` runs the FairyGUI Editor's headless publish into
`Content/fgui/` (the editor itself is a separate install, like the Android SDK —
the script locates it via `FGUI_EDITOR` or a documented default path).
`tools/vendor-fairygui.ps1` vendors the runtime into `third_party/fairygui`
(mirroring `tools/vendor-axmol.ps1`: pin the version, patch if needed, log the
size). `Content/` is gitignored, so the APK build must fail loudly (like the
missing `axslc`) when `Content/fgui/HUD.bin` is absent rather than shipping an
empty HUD. The FairyGUI C++ runtime builds for the Android target directly (our
`build-native.ps1` already passes the extra defines axmol uses) — if it ever
needs patching, `vendor-fairygui.ps1` hosts the patch instead of editing the
engine in place, matching the `1k/fetch.cmake` precedent.

---

## 7. Migration order (each step keeps a runnable APK)

| Step | Work | Gate |
|---|---|---|
| **S1** | This document; `UiHost` interface + `UiBridge.cpp` skeleton + `SurvBridge.java` with `getConfigJson`/`setPlayerName` only; `JavaUiHost` adds the `surv_java_ui` `FrameLayout` and a trivial "Survev (Java UI)" text overlay on top of the running game. | APK installs; logcat shows the bridge round-trip; `surv_tests` green; game still renders and plays. |
| **S2** | FairyGUI vendored + `HUD_pkg`/`Touch_pkg` exported; `GameScene`'s `UiOverlay` HUD replaced by `HUD_pkg` (health/boost/ammo/killfeed/alive). Touch pads swapped to `Touch_pkg`. | On-device: same HUD information as today, joystick/aim line look correct; host tests unchanged. |
| **S3** | Java **MenuFragment** (start menu, settings, help, join-team) + the C++ `StartMenu` deleted; matchmaking HTTP moved to `SurvApi.java`; match start/end driven by `onNativeMatchStarted`/`onNativeMatchEnded`. | Menu → Play Solo → HUD → pause → Quit → menu works end-to-end with **no axmol menu code left**; l10n and config round-trip. |
| **S4** | Java **LoadoutFragment** (and the crosshair pane) + `LoadoutMenu` deleted; determinism checked against `ui::Loadout::validated`. | Loadout selection reaches the `JoinMsg`; emotes/crosshair round-trip. |
| **S5** | Java **TeamFragment** + `TeamSocket.java`; the C++ team screen deleted; `GameScene` leaving a team game calls back to Java. | Create/join room, roster, region/mode/auto-fill, kick, rename, Play. |
| **S6** | Java **GamePauseDialog** (sliders/language/quit) + `UiOverlay` deleted entirely; audio volumes driven from Java. | Pause/settings parity; mute persists. |
| **S7** | Cleanup: delete `Ui.*`, `Layout.*`, the axmol HTTP paths (`Api.cpp`/`SiteInfo.cpp` for the app build), update `plan.md`/`REMAININGS.md`/`M6_AUDIO.md`/`PLATFORM` docs; refresh `ARCHITECTURE.md`'s status section. | Dead-code grep is clean; `surv_tests` green; APK builds from a fresh `Content/` staging. |

Each step: `. tools/android-env.ps1`, `build-native`, `sync-content`,
`build-fgui`, `build-apk`, install on the x86_64 emulator, screenshot,
`logcat -b crash` empty, and `surv_tests` **≥ current count, 0 failures**.

---

## 8. Risks

| Risk | Mitigation |
|---|---|
| Two UI worlds drift visually (Java menus vs FairyGUI HUD). | Shared palette/fonts/9-slices on both sides; the HUD package is authored from the web CSS values (`.btn-green` etc. already in `StartMenu.cpp` are the reference). Screenshot diff each step. |
| JNI surface grows into a second protocol. | One `SurvBridge` class, one `UiHost` interface, no struct marshalling beyond JSON strings/ints; every new method needs a host-testable engine-free counterpart. |
| GL-thread vs UI-thread mistakes (touching Views off-thread / engine state on the UI thread). | Rules in §4.2 are mandatory; `AXLOG` assertions in debug builds around thread-sensitive calls. |
| FairyGUI runtime cost/axmol version compatibility. | Vendor it like axmol (`tools/vendor-fairygui.ps1`), keep the backend additive to `PixiLike` (`GRoot` above the world layers), measure the first on-device frame before committing the HUD to it. |
| OkHttp cannot be added to the Gradle-free APK. | §6.2: vendor jars into `app/libs` first; fallback is a tiny hand-rolled WS client. |
| Losing C++-side host tests while porting logic to Java. | Only *presentation* moves to Java. Config/Loadout/Localization/team protocol parsers stay in C++ and keep their tests until the Java equivalents have their own (JVM unit tests) — then delete one side deliberately, not by accident. |
| Session/login (M8) assumed Java-only. | Session cookie handling lives in `SurvApi.java`; the C++ `/play` socket does not need it (the join token carries identity). Revisit if accounts are added to the game socket. |

---

## 9. Explicit non-goals

* No WebView (the original D1 Option A stays rejected).
* No screen that is *half* axmol and *half* Java widgets inside one canvas
  region; the seam is the whole canvas: Java UI covers it, FairyGUI draws in it.
* No second widget toolkit: after S7 the C++ side has **no** general-purpose UI
  toolkit, only the FairyGUI gameplay packages.
* The game protocol, `PixiLike`/`AxmolPixi` adapter and the barns are **not**
  re-architected by this change.

---

## 10. Status / changelog

* **2026-10-14** — document created; decisions A1–A4 recorded; supersedes
  `plan.md` §5.6 D1 and `M7_REMAININGS.md`'s "native axmol UI" scope.
* **2026-10-15** — **S1 implemented**. `src/app/UiHost.{h,cpp}` (engine-free
  `UiHost`/`UiBridge` interfaces, no-op defaults, `installUiBridge`),
  `src/app/UiBridge.cpp` (Android-only: JNI entry points for
  `dev.axmol.app.SurvBridge`, the `AndroidUiHost` C++→Java callbacks, and the
  config/l10n/loadout/audio writes into the shared `ui::Config`),
  `src/ui/{L10nJson,NewsJson,SiteInfoJson}.*` (the documents Java reads),
  `proj.android/app/src/dev/axmol/app/{SurvBridge,JavaUiHost,MenuFragment,Json}.java`
  and the `AppActivity` container, plus `tests/test_bridge.cpp`. The S1 gate
  (bridge round-trip visible in logcat) is exercised by
  `JavaUiHost.probeBridge()` + `MenuFragment`'s self-test button. Gate status:
  APK installs/builds, `surv_tests` green except the pre-existing
  `update_full` fixture drift (see `REMAININGS.md` §6). **S2 is next.**