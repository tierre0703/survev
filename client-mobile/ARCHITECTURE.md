# SurvMobile architecture — status (2026-10-15)

## UI split complete: S1–S7 landed

The migration from a single C++ (axmol) UI to the two-world architecture is
complete:

| Step | Status |
|------|--------|
| **S1** | `UiHost`/`UiBridge` seam + JNI `SurvBridge` + Java UI container |
| **S2** | FairyGUI `GRoot` + `FguiHud`/`FguiPadGraphics` (programmatic HUD + pads) |
| **S3** | Java `MenuFragment` + `SurvApi` HTTP client + matchmaking flow |
| **S4** | Java `LoadoutFragment` with selection round-trip through C++ validator |
| **S5** | Java `TeamFragment` + `TeamSocket` WebSocket client for `/team_v2` |
| **S6** | Java `GamePauseDialog` (sliders, toggles, quit) — C++ overlay disabled |
| **S7** | Cleanup: `CssColor.*` extracted, `Layout.*` trimmed, dead-code comments |

### What was deleted from C++
| File | Status |
|------|--------|
| `src/ui/Ui.{h,cpp}` | **Remains for host tests** (Button/Panel/Modal needed by test_ui.cpp) |
| `src/ui/UiOverlay.{h,cpp}` | **Remains for host tests** (StartMenu+HUD debug test) |
| `src/ui/StartMenu.cpp` | **Remains for host tests** (needed by MenuScene) |
| `src/ui/LoadoutMenu.{h,cpp}` | **Remains for host tests** |
| `src/ui/TeamMenu.{h,cpp}` | **Remains for host tests** (team protocol tests) |
| `src/ui/Layout.{h,cpp}` | Trimmed to constants only; CSS parser moved to `CssColor.*` |
| `src/net/Api.{h,cpp}` | **Kept** (host tests + desktop build) |
| `src/net/SiteInfo.{h,cpp}` | **Kept** (host tests + desktop build) |

The C++ UI toolkit is **dead code on Android** — the Java screens are the
primary UI. The C++ version is kept only because `surv_tests` hosts
90+ tests that exercise the widget toolkit, layout constants, CSS colour
parser, and team menu protocol against in-memory connections.

### Remaining Java files
```
proj.android/app/src/dev/axmol/app/
  AppActivity.java         — Activity entry point
  SurvBridge.java          — JNI bridge (native methods)  
  JavaUiHost.java          — Owns all Java screens (menu/loadout/team/pause)
  MenuFragment.java        — Start menu with matchmaking
  LoadoutFragment.java     — Loadout editor
  TeamFragment.java        — Team room (real /team_v2 protocol, invite links)
  TeamSocket.java          — raw-socket WebSocket client for /team_v2
  TeamLinks.java           — invite link build/parse (web link + survev:// deep link)
  AccountManager.java      — device login/session/profile (M8, no OAuth)
  ProfileFragment.java     — account name/stats/delete/logout screen
  PassFragment.java        — pass level/XP + quests/reroll (M8)
  GeneratedPassData.java   — generated pass/quest tables (codegen_pass_defs.mjs)
  GamePauseDialog.java     — In-game pause overlay
  SurvApi.java             — HTTP client (site_info + find_game + user/device)
  Json.java                — Minimal JSON reader
```

### Notable design decisions
- **No OkHttp** — `SurvApi.java` and `TeamSocket.java` use pure Java stdlib
  (`java.net.HttpURLConnection`) to keep the Gradle-free APK pipeline working.
- **No WebView** — all UI is native Android Views.
- **Accounts are device-scoped, not OAuth** — the native app cannot complete a
  browser OAuth callback; `AccountManager` persists a device id (ANDROID_ID with
  a random fallback) and exchanges it for a Bearer session at
  `/api/auth/device`. The web keeps its Discord/Google flow.
- **Java l10n is served by a standalone C++ table** — Android has no C++
  `MenuScene`, so `UiBridge::l10nJson()` builds its own `Localization` from the
  bundled `l10n/*.json` (config `language`). Do not remove that fallback: the
  Java UI would fall back to raw keys.
- **Team links are web links** - `https://<webBaseUrl>/#CODE` (config key,
  default `https://survev.io`) so browser players can join, plus a
  `survev://team/CODE` deep link handled by `AppActivity`. The team socket is a
  hand-written WebSocket client (`TeamSocket`) because no WS library is
  vendored; it sends the device Bearer token so `/team_v2` sees the account.
- **The API origin is manifest-driven** - `surv_apiUrl` is read once by
  `SurvApi.initialize(Context)` for the Java HTTP/team clients (and by C++ as the
  dev override). `tools/build-apk.ps1 -ApiUrl` substitutes it for release builds;
  the manifest default is the loopback used with `adb reverse`.
- **HUD stays in axmol + FairyGUI** (the world canvas); Java only covers
  out-of-game screens.
- **C++ `Config`/`Loadout`/`Localization`/`Touch`** stay in C++ as the source
  of truth; Java reads/writes them through `SurvBridge` JSON methods.
