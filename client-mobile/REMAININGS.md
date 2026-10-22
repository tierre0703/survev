# client-mobile — what is left to implement (1:1 web → axmol port)

Consolidated status of the native Android port of `client/` (`client-mobile/`).
This file is a **summary/index**; the detailed work orders live in:

- `plan.md` — module map, milestones M0–M9, decisions D1–D5.
- `M4_REMAINING.md` — rendering (sessions 1–6), verified object-render parity.
- `M6_AUDIO.md` — audio port notes.
- `ARCHITECTURE.md` — UI architecture: gameplay = axmol + FairyGUI, every
  other screen = Android Java, one JNI bridge (`SurvBridge`). Read §5 (move
  matrix) and §7 (migration steps S1–S7) before touching UI code.

Latest recorded state: **2026-10-22** — Session 11 (`M4_REMAINING.md` §16) implemented the emote wheel end to end: emote defs are codegen'd from `shared/defs/gameObjects/emoteDefs.ts`, the on-screen emote button opens a four-wedge loadout wheel (icons, labels, teamOnly gating, soft/hard emote cooldowns), and the selection is queued through `EmoteManager` → `Game::sendEmote`. The same pass fixed the pads rendering ~4x too small (`PadGraphicsAxmol` scale) and world-space name labels rendering vertically mirrored (counter-flipped `pix::Text`). **116 host tests pass, 0 fail.** See §3 for the next-priority gaps (team-ping wheel input, ammo-specific ping emotes, reverb, M8 accounts).

---

## 1. Milestone scorecard

| # | Milestone | Status | Notes |
|---|---|---|---|
| M0 | Toolchain + axmol builds | **done** | axmol vendored, Gradle-free APK pipeline (`tools/build-apk.ps1`). |
| M1 | Asset pipeline | **done** | atlases, GUI PNGs (4x), Roboto Condensed TTF, l10n. |
| M2 | Shared core ports (bit-exact) | **done** | `BitBuffer`, `Vec2/MathUtil/Collider`, message classes, TS-generated fixtures. |
| M3 | Net + headless sim | **done (host)** | join/update/kill/gameover/pickup/map dispatch, `snapshotText()`; multi-URL join fallback. |
| M4 | Rendering | **done, latest verified** | terrain/island hole, obstacles, buildings + ceilings, particles, decals, gas, skeletal player. |
| M5 | Input + touch | **mostly done** | move/aim/fire → `InputMsg`; dual pads render at the web's scale (`PadGraphicsAxmol`), aim-line dots wired. |
| M6 | Audio | **mostly done** | engine + mixer + ambiance + explosion/bullet/shot sounds + reverb volume computation. Player action sounds and UI sounds still unwired (see §2). |
| M7 | UI layer | **S1–S7 landed** | The JNI `SurvBridge` seam + Java menu/loadout/team/pause are in place; the in-game HUD is the plain pix adapter (`game/Hud`), and the emote wheel (`game/EmoteWheel`) is live with loadout icons + input. |
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
| `client/src/ui/touch.ts` → `ui/Touch.h`, `game/AimLine.h` | ✅ data model + aim line + real pad sprites (`PadGraphicsAxmol`) |
| `client/src/config.ts` → `ui/Config.*`, `ui/ConfigStorage.cpp` | ✅ |
| `client/src/device.ts` → `ui/Device.*` | 🔶 simplified |
| `client/src/localization.ts` → `ui/Localization.*`, `ui/L10nJson.*` | ✅ |
| `client/src/main.ts` (app bootstrap) → `app/AppDelegate.*`, `app/GameScene.*`, `app/MenuScene.*` | ✅ |
| `client/src/api.ts`/`siteInfo.ts` → `net/Api.*`, `net/SiteInfo.*`, Java `SurvApi.java` | ✅ |
| `client/src/ui/teamMenu.ts` → `ui/TeamMenu.*`, Java `TeamFragment.java` + `TeamSocket.java` | ✅ |
| `client/src/ui/loadoutMenu.ts` → Java `LoadoutFragment.java` | ✅ |
| `client/src/ui/ui.ts` (HUD) → `game/Hud.*` (pix adapter) | ✅ health/boost, weapon cards, ammo, alive/kills, kill feed, pickup message |
| `client/src/ui/touch.ts` (pad gfx) → `ui/PadGraphicsAxmol.*` | ✅ real ring + touch dot at the web's scale |
| `client/src/emote.ts` (pings + emote wheel + indicators) | ✅ world emotes (def textures) + ping indicators + emote wheel UI/input/throttle; **team-ping wheel + ammo pings + map ping sprites remain** |
| `client/src/objects/mapIndicator.ts` / `mapSprite.ts` | ❌ **not ported** |
| `client/src/input.ts`/`inputBinds.ts` (mouse/keyboard/binds) | ❌ **not ported** (BT keyboard/controller) |
| Player action sounds (footsteps, door, bush, weapon deploy/switch, heal, pickup, dry fire, haste) | ❌ **not wired** — `Player` lacks surface/step/action state |
| UI sounds (ambient lab, pickup pings, notifications) | ❌ **not wired** — UI is Java |
| Reverb convolution (`setReverbProperties`) | 🔶 volume computed + stored; axmol API not called |
| `client/src/ui/ui2.ts` (inventory v2: weapon cards, perk icons, throwable arc) | ✅ weapons/ammo in `game/Hud`, gear/perks in `ui/FguiHud` (not built by the live scene yet) |
| `client/src/emote.ts` (emote/ping system) | ✅ see the emote row above |
| `client/src/ui/pieTimer.ts` (action progress circle) | ✅ `ui/FguiPieTimer` driven by `ActivePlayerData.actionTime` |
| `client/src/ui/profileUi.ts` / `pass.ts` (account/profile/battle pass) | ❌ **not ported** (M8) |
| `client/src/crosshair.ts` (custom crosshair) | ✅ screen-space crosshair overlay (default style; loadout-specific art is a follow-up) |
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
- [x] **Pickup sound** — finish wiring `playItemPickupSound` via `Game` pickup callback → `GameScene::onPickupSound()` plays the `item + "_pickup_01"` sound on the `"ui"` channel
- [x] **Emote/ping system** (`client/src/emote.ts`) — port to C++ (with remaining gaps)

### Priority 2 — HUD/UI gaps (visible during gameplay)

> The live in-game HUD is the plain pix-adapter `game/Hud.*` (design space,
> host-tested). `ui/FguiHud.*` is the older FairyGUI implementation and is not
> constructed by `GameScene`; treat the "FguiHud" bullets below as feature
> descriptions, not the shipping widget tree.

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
- [x] **Minimap** (render from `RenderTexture`)
- [x] **Squad health bars** (4 team member health bars with status indicators)
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
  - Screen-space overlay drawn at the screen centre above the world
  - Default crosshair art is an inline SVG in `crosshairDefs.ts` (not an atlas
    frame), so it is drawn as two white bars via `pix::Graphics`
  - Created once in `GameScene::update()`, shown during gameplay
  - Future: read the crosshair type/color/size from the Config loadout
- [x] **Touch pad sizing** — `PadGraphicsAxmol` was 4x too small; the ring is
  now sized to the web's `pad.img` (208 px * 0.6) and the dot to 208 px * 0.25
- [x] **World-space name labels** — player/dead-body labels were vertically
  mirrored under the camera's negative-Y root; `pix::Text::setFlipY()` counters
  the flip about the anchor (`M4_REMAINING` §16)
- [x] **Minimap** (baked RenderTexture + FGUI overlay):
  - Map::generateMinimapTexture() renders terrain + obstacles + buildings
    into a pix::RenderTexture using the PIXI adapter
  - Map::getMinimapTexture() / getMinimapScale() for runtime access
  - FguiHud::buildMinimap() FGUI frame + gas ring + player dot overlays
  - onFactoryReset() clears minimap texture on map reload
- [x] **Inventory UI v2** (`ui2.ts` features — weapon cards, gear slots, perk slots, scope inventory, loot counts, pickup/kill messages, interaction prompt) — implemented 2026-10-17
- [x] **Emote/ping system** (`client/src/emote.ts`) — **emote wheel landed 2026-10-22**:
  - [x] Game::sendEmote() wire → server (MsgType_Emote); wheel output is queued
        on `EmoteManager` and forwarded by `GameScene` per frame (game.ts loop)
  - [x] EmoteManager C++ class: addPing/addEmote from UpdateMsg.emotes
  - [x] Emote defs codegen (`EmoteRenderDef`: texture/sound/channel/name/teamOnly)
        — world emotes now draw their real face/food/logo frame + sound
  - [x] World-space emote sprites (container + circle + sprite + animation)
  - [x] Team ping indicators (border + ping + edge-of-screen indicators)
  - [x] **Emote wheel UI** (`game/EmoteWheel`): on-screen emote button + four
        EmoteSlot wedges with loadout icons/labels, centre close, open animation
  - [x] **Emote wheel input**: touch on the button opens the wheel around it;
        dragging past the dead zone sends the highlighted wedge immediately
  - [x] **Emote throttle logic** (soft 2s / hard 6s at 6 sends; wheel greys out)
  - [x] Solo gating for `teamOnly` emotes
  - [ ] Ping wheel input triggering (touch/bind)
  - [ ] Ammo-specific ping emotes (ammo type → emote mapping)
  - [ ] Map ping sprites (minimap dot from ping sprite barn)
  - [x] Emote sound playback on addEmote

- [x] **FairyGUI touch pads** (`FguiPadGraphics.*`) — implement:
  - Real pad sprites (`pad.img` with alpha/scaling)
  - Aim-line dots (`LineSprites`: dot sprite every 1.5 units, ray-cast against obstacles)

- [x] **Inventory UI v2** (`ui2.ts` features — weapon cards, perk icons, loot pickup feed, throwable arc preview, team inventory panel)

- [x] **Pie timer** (`pieTimer.ts` — action progress circle)

- [x] **Crosshair** (`crosshair.ts` — custom crosshair rendering)

### Priority 3 — Audio polish

- [x] **Reverb volume computation** — done; `AxmolAudioBackend::update` stub remains for the actual `setReverbProperties` call
- [x] **UI/pickup sounds** — wired: `Game::handlePickup` → `GameScene::onPickupSound` plays `item + "_pickup_01"` on `"ui"`; emote/ping sounds play on `"ui"` via `EmoteManager`
- [ ] **Reverb convolution** — call `ax::AudioEngine::setReverbProperties()` from `AxmolAudioBackend::update()` with proper parameter mapping from `ReverbDef` data (cathedral/cave)
- [ ] **Kill leader / notification sounds** — wire into the FairyGUI/Java UI layer

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
# host tests (116 tests, 0 failures expected)
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