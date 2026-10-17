# client-mobile — what is left to implement (1:1 web → axmol port)

Consolidated status of the native Android port of `client/` (`client-mobile/`).
This file is a **summary/index**; the detailed work orders live in:

- `plan.md` — module map, milestones M0–M9, decisions D1–D5.
- `M4_REMAINING.md` — rendering (sessions 1–6), verified object-render parity.
- `M6_AUDIO.md` — audio port notes.
- `ARCHITECTURE.md` — UI architecture: gameplay = axmol + FairyGUI, every
  other screen = Android Java, one JNI bridge (`SurvBridge`). Read §5 (move
  matrix) and §7 (migration steps S1–S7) before touching UI code.

Latest recorded state: **2026-10-16** — M6 audio wiring (per-def explosion,
bullet whiz, weapon shot sounds) + reverb volume computation implemented.
**Plus Priority 1a — player action sounds** (footsteps, bleeding, haste
ability, weapon deploy/switch, action sound start/stop) wired into the
`Player::update()` / `m_updateData()` / `updateActions()` cycle.
**93 host tests pass** on `surv_tests` (pre-existing `update-full` fixture
drift is unrelated — `test_messages.cpp:212`).

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
| M6 | Audio | **mostly done** | engine + mixer + ambiance + explosion/bullet/shot sounds + reverb volume computation. Player action sounds and UI sounds still unwired (see §2). |
| M7 | UI layer | **S1–S7 landed** | The JNI `SurvBridge` seam + Java menu/loadout/team/pause + FairyGUI HUD stubs are all in place. `FguiHud.*`/`FguiPadGraphics.*` are stubs needing implementation. |
| M8 | Accounts / API | **not started** | login/profile/team-link/quests/pass. |
| M9 | Polish & release | **not started** | lifecycle, perf, soak, store-ready AAB. |

---

## 2. Module-by-module port status

| Web source (`client/src/`, `shared/`) | Native target | Status |
|---|---|---|
| `shared/lib/bitBuffer.ts` → `core/BitBuffer.h` | ✅ bit-exact |
| `shared/utils/v2.ts`/`math.ts` → `core/Vec2.h`/`MathUtil.h` | ✅ |
| `shared/utils/terrainGen.ts`/`river.ts`/`spline.ts` → `render/Terrain.*` | ✅ |
| `shared/utils/collider.ts`/`coldet.ts` → `core/Collider.h` | ✅ |
| `shared/gameConfig.ts` → `core/GameConfig.h` | ✅ |
| `shared/net/net.ts` → `net/Net.*` | ✅ |
| `shared/net/*Msg.ts` → `net/Messages.h` | ✅ |
| `shared/net/connection.ts` → `net/Connection.h`, `net/WebSocketConnection.*` | ✅ |
| `shared/defs/*` → `render/GeneratedDefs.cpp` (codegen) | ✅ generated |
| `client/src/camera.ts` → `render/Camera.h` | ✅ |
| `client/src/renderer.ts` → `render/Renderer.*` | ✅ |
| `client/src/map.ts` → `game/Map.*` | ✅ terrain, surfaces, pools, camera emitter; **minimap/big map not ported** |
| `client/src/gas.ts` → `game/Gas.*` | ✅ |
| `client/src/game.ts` (main loop, net dispatch) → `game/Game.*`, `game/GameWorld.*` | ✅ |
| `client/src/objects/objectPool.ts` → `game/objects/GameObject.h` | ✅ |
| `client/src/objects/player.ts` + `animData.ts` → `objects/PlayerRender.cpp` + `Barns.*` | ✅ bones/poses/animations/aura/submerge/frozen/team patches/anim effects |
| `client/src/objects/bullet.ts` → `objects/Barns.*` (BulletBarn) | ✅ whiz sound wired |
| `client/src/objects/explosion.ts` → `objects/Barns.*` (ExplosionBarn) + `GameWorld.cpp` | ✅ per-def sound map wired |
| `client/src/objects/shot.ts` → `objects/Barns.*` (ShotBarn) | ✅ weapon fire sounds + detune variants wired |
| `client/src/objects/obstacle.ts` → `objects/Barns.*` | ✅ |
| `client/src/objects/building.ts` → `objects/Barns.*` | ✅ |
| `client/src/objects/loot.ts` → `objects/Barns.*` | ✅ |
| `client/src/objects/decal.ts` → `objects/Barns.*` | ✅ |
| `client/src/objects/deadBody.ts` → `objects/Barns.*` | ✅ |
| `client/src/objects/projectile.ts` → `objects/Barns.*` | ✅ |
| `client/src/objects/smoke.ts` → `objects/Barns.*` | ✅ |
| `client/src/objects/airdrop.ts` → `objects/Barns.*` | ✅ |
| `client/src/objects/plane.ts` → `objects/Barns.*` | ✅ |
| `client/src/objects/flare.ts` → `objects/Barns.*` | ✅ |
| `client/src/objects/structure.ts` → `objects/Structure.*` | ✅ |
| `client/src/objects/particles.ts` → `objects/Barns.*` (ParticleBarn + Emitter) | ✅ |
| `client/src/audioManager.ts` → `audio/AudioManager.*` | ✅ |
| `client/src/soundDefs.ts` → `audio/GeneratedSoundDefs.cpp` (codegen) | ✅ generated |
| `client/src/ambiance.ts` → `audio/Ambiance.*` | ✅ |
| `client/lib/createJS.ts` → `audio/AxmolAudioBackend.h` | ✅ |
| `client/src/ui/touch.ts` → `ui/Touch.h`, `game/AimLine.h` | ✅ data model + aim line; **pad sprites are placeholders**, **aim-line dots missing** |
| `client/src/config.ts` → `ui/Config.*`, `ui/ConfigStorage.cpp` | ✅ |
| `client/src/device.ts` → `ui/Device.*` | 🔶 simplified |
| `client/src/localization.ts` → `ui/Localization.*`, `ui/L10nJson.*` | ✅ |
| `client/src/main.ts` (app bootstrap) → `app/AppDelegate.*`, `app/GameScene.*`, `app/MenuScene.*` | ✅ |
| `client/src/api.ts`/`siteInfo.ts` → `net/Api.*`, `net/SiteInfo.*`, Java `SurvApi.java` | ✅ |
| `client/src/ui/teamMenu.ts` → `ui/TeamMenu.*`, Java `TeamFragment.java` + `TeamSocket.java` | ✅ |
| `client/src/ui/loadoutMenu.ts` → Java `LoadoutFragment.java` | ✅ |
| `client/src/ui/ui.ts` (HUD) → `ui/FguiHud.*` (FairyGUI stub) | ⏳ stub — most HUD elements missing |
| `client/src/ui/touch.ts` (pad gfx) → `ui/FguiPadGraphics.*` (FairyGUI stub) | ⏳ stub — placeholder shapes |
| `client/src/emote.ts` (pings + emote wheel + indicators) | ❌ **not ported** |
| `client/src/objects/mapIndicator.ts` / `mapSprite.ts` | ❌ **not ported** |
| `client/src/input.ts`/`inputBinds.ts` (mouse/keyboard/binds) | ❌ **not ported** (BT keyboard/controller) |
| Player action sounds (footsteps, door, bush, weapon deploy/switch, heal, pickup, dry fire, haste) | ❌ **not wired** — `Player` lacks surface/step/action state |
| UI sounds (ambient lab, pickup pings, notifications) | ❌ **not wired** — UI is Java |
| Reverb convolution (`setReverbProperties`) | 🔶 volume computed + stored; axmol API not called |
| `client/src/ui/ui2.ts` (inventory v2: weapon cards, perk icons, throwable arc) | ❌ **not ported** |
| `client/src/ui/opponentDisplay.ts` (spawn preview) | ❌ **not ported** |
| `client/src/ui/pieTimer.ts` (action progress circle) | ❌ **not ported** |
| `client/src/ui/profileUi.ts` / `pass.ts` (account/profile/battle pass) | ❌ **not ported** (M8) |
| `client/src/crosshair.ts` (custom crosshair) | ❌ **not ported** |
| `client/src/pingTest.ts` (pre-game latency check) | ❌ **not ported** |
| `client/src/account.ts` / `proxy.ts` / `errorLogs.ts` | ❌ **not ported** |
| `client/src/sdk/*` (ad/partner SDK) | ❌ out of scope |
| `client/src/debug/*` | ❌ **not ported** (dev tooling) |
| `client/stats/*` (leaderboard web pages) | ❌ separate app |

---

## 3. Remaining work — prioritized TO-DO list

### Priority 1 — Core gameplay gaps (most noticeable)

- [x] **Player action sounds** — extend `Player` struct with:
  - `surface` type (from `Map::getGroundSurface`)
  - `stepDistance` counter
  - `action` type/item (reload/useItem)
  - `cycleSoundInstance`/`actionSoundInstance` handles
  - Wire `playGroup("footstep_${surface.type}")` / `playGroup("footstep_water")` on step threshold
  - Wire weapon deploy/switch sound (`weaponDef.sound.deploy` via `"gun_switch_01"` fallback)
  - Wire reload/useItem action sounds (`playActionStartEffect`)
  - Wire item pickup sound (`playItemPickupSound` → `"ui"` channel) — **handlePickup placeholder**
  - Wire dry fire sound (`itemDef.sound.empty`) — **not wired (needs input/ammo tracking)**
  - Wire haste ability sound (`ability_stim_01`)
  - Wire healing effect/action sounds (`bandage_use_01`/`healthkit_use_01`/`soda_use_01`/`pills_use_01`)
  - Wire door error sound (`door_error_01`) — **not wired (no door interaction in C++ yet)**
  - Wire bush enter/exit sound (`obstacleDef.sound.enter`) — **not wired (obstacle bush tracking not ported)**
  - Wire bleeding sound (`player_bullet_hit_02`)
  - Wire bullet hit sounds (`playGroup("player_bullet_hit")` and obstacle `sound.bullet` groups) — **handled by hit barns**
- [ ] **Pickup sound** — finish wiring `playItemPickupSound` via `GameWorld` pickup callback to play the correct sound on `PickupMsgType_Success`
- [ ] **Emote/ping system** (`client/src/emote.ts`) — port to C++ (unstarted)

### Priority 2 — HUD/UI gaps (visible during gameplay)

- [x] **FairyGUI HUD** (`FguiHud.*`) — **Health/boost bars**:
  - Health bar with color interpolation (8-step grey→white→pink→red)
  - Health numeric text alongside bar
  - Boost bar with 4 colored segments (orange shades)
  - Semi-transparent panel background
  - Wired into `GameScene::update()` per frame
- [x] **FguiHud — Weapon/ammo display**:
  - Weapon name text (from `ActivePlayerData.weapons[curWeapIdx].type`)
  - Ammo clip + reserve count (from weapon ammo + inventory lookup)
  - Handles gun (clip/reserve), melee (no ammo), throwable categories
- [x] **FguiHud — Alive counter**:
  - Top-right alive count label from `Game::aliveCounts()`
  - Faction/squad mode handling
- [x] **FguiHud — Kill feed**:
  - Kill feed text built from `Game::killFeed()` entries
  - Kill messages: "killerName killed targetName (weapon)"
  - Non-player kills: "targetName - died/gas"
  - Knocked-out vs killed distinction
- [x] **FguiHud — Game result overlay**:
  - Centered semi-transparent panel
  - Rank #N + local player kills + damage
  - Toggled via `Game::gameOver()` presence
- [ ] **Minimap** (render from `RenderTexture`)
- [ ] **Squad health bars** (4 team member health bars with status indicators)
- [x] **FguiPadGraphics touch pads** — real pad sprites (FGUI GGraph circles):
  - Center ring (GGraph drawEllipse, blue for move, orange for aim, semi-transparent)
  - Touch dot (GGraph drawEllipse solid fill, white/light orange)
  - Position/scale set reactively from Touch.m_update()
  - Wired into GameScene init as the active pad renderer
- [x] **Aim-line dots** (`LineSprites`: dot sprite every 1.5 units, ray-cast against obstacles)
  - Already fully ported in AimLine.h; wired into GameWorld::update()
  - Was dormant — now enabled: GameScene passes Touch.touchingAim + .touchAimLine
    to GameWorld::setAimLineState() each frame
- [x] **Pie timer** (`pieTimer.ts` — action progress circle):
  - FguiPieTimer class with FGUI GGraph circle + countdown + label
  - Driven by ActivePlayerData.actionTime/actionDuration
  - Wired into GameScene::update() per frame
  - Stops when action completes
- [x] **Squad health bars** (4 team member health bars with status indicators):
  - FguiHud.buildSquadBars() builds 4 slots with name, health bar, status text
  - Driven by Game::getTeamPlayerIds() + groupStatus() + playerStatus
  - Shows green/amber/grey bar based on health, "Dead"/"Down"/"DC" status labels
  - Panel hidden in solo mode, shown top-left in team/squad modes
  - Wired into FguiHud::update() per frame
- [x] **Crosshair** (`crosshair.ts` — custom crosshair rendering):
  - PIXI Sprite rendered at screen center above the game world
  - Uses "crosshair000.img" atlas frame from GeneratedDefs
  - Created once in GameScene::update(), shown during gameplay
  - Future: read the crosshair type/color/size from Config loadout
- [x] **Minimap** (baked RenderTexture + FGUI overlay):
  - Map::generateMinimapTexture() renders terrain + obstacles + buildings
    into a pix::RenderTexture using the PIXI adapter
  - Map::getMinimapTexture() / getMinimapScale() for runtime access
  - FguiHud::buildMinimap() FGUI frame + gas ring + player dot overlays
  - onFactoryReset() clears minimap texture on map reload
- [ ] **Inventory UI v2** (`ui2.ts` features — weapon cards, perk icons, loot pickup feed, throwable arc preview, team inventory panel)

- [ ] **FairyGUI touch pads** (`FguiPadGraphics.*`) — implement:
  - Real pad sprites (`pad.img` with alpha/scaling)
  - Aim-line dots (`LineSprites`: dot sprite every 1.5 units, ray-cast against obstacles)

- [ ] **Inventory UI v2** (`ui2.ts` features — weapon cards, perk icons, loot pickup feed, throwable arc preview, team inventory panel)

- [ ] **Pie timer** (`pieTimer.ts` — action progress circle)

- [ ] **Crosshair** (`crosshair.ts` — custom crosshair rendering)

### Priority 3 — Audio polish

- [ ] **Reverb convolution** — call `ax::AudioEngine::setReverbProperties()` from `AxmolAudioBackend::update()` with proper parameter mapping from `ReverbDef` data (cathedral/cave)
- [ ] **UI sounds** (pickup pings `"gun_pickup_01"`/`"ammo_pickup_01"` etc., kill leader assignment, notification sounds) — wire into the FairyGUI/Java UI layer

### Priority 4 — Minor simulation/render parity

- [ ] **Map::getGroundSurface** — verify `footstep_${surface.type}` group names match all web surface types: grass, sand, water, tile, asphalt, brick, stone, carpet, bunker, container, warehouse, house, shack
- [ ] **Structures** — verify `insideStructureMask`/`isUnderground` covers every shipped map

### Priority 5 — Platform / release (M8 + M9)

- [ ] **M8 Accounts/API**: login, profile, team links, quests/pass, proxy detection, region ping test, error reporting
- [ ] **Lifecycle hardening**: background pause/resume rejoin, reconnect/backoff on flaky mobile networks, back-button handling
- [ ] **Perf/polish**: 60 fps on mid-range devices, notch/insets, battery/resolution tiers, crash-free soak
- [ ] **Store-ready APK/AAB**

### Out of scope (deliberately not ported)

- `client/src/debug/*` (dev tooling — only if needed for development)
- `client/src/sdk/*` (ad/partner SDK — not applicable on native)
- `client/src/proxy.ts` (not applicable on native)
- `client/stats/*` (separate web app)
- Desktop keybinding UI

---

## 4. Verification workflow

```powershell
cd E:\work\survev\client-mobile
. .\tools\android-env.ps1
# host tests (93 tests, 0 failures expected)
cmake -S tests -B build-tests -G "Visual Studio 17 2022" -A x64
cmake --build build-tests --config Release
.\build-tests\Release\surv_tests.exe
# device (x86_64 emulator + dev server via adb reverse)
.\tools\sync-content.ps1
.\tools\build-native.ps1 -Abis "x86_64"
.\tools\build-apk.ps1 -Abis "x86_64"
& $adb reverse tcp:8000 tcp:8000; & $adb reverse tcp:9000 tcp:9000
& $adb install -r .\SurvevMobile-debug.apk
```

Regenerate after touching `shared/defs` or web sources:
```powershell
node --experimental-transform-types tools/codegen_defs.mjs src/net/generatedDefs.inc
node --experimental-transform-types tools/codegen_render_defs.mjs
node --experimental-transform-types tools/codegen_sound_defs.mjs
node tools/gen_reference.mjs
node tools/build-atlas.mjs --res low
node tools/build-gui-icons.mjs
```

**Do NOT regress:**
- Adapter reparenting (`addChild` removes from old parent first)
- Wrapper native-node ownership (`ownNode()` + destructor)
- `drawSolidPoly` convex-fan path (`isconvex=true`)
- `axslc` shaders packaged in APK
- `resources.arsc` stored uncompressed + aligned
- Layer-index clamping (`0..3`)
- Quoted `-DANDROID_ABI` in `build-native.ps1`
- `src/app/UiBridge.cpp` excluded from host build