# client-mobile — what is left to implement (1:1 web → axmol port)

Consolidated status of the native Android port of `client/` (`client-mobile/`).
This file is a **summary/index**; the detailed work orders live in:

- `plan.md` — module map, milestones M0–M9, decisions D1–D5.
- `M4_REMAINING.md` — rendering (sessions 1–6), verified object-render parity.
- `M6_AUDIO.md` — audio port notes.
- `ARCHITECTURE.md` — UI architecture: gameplay = axmol + FairyGUI, every
  other screen = Android Java, one JNI bridge (`SurvBridge`). Read §5 (move
  matrix) and §7 (migration steps S1–S7) before touching UI code.

Latest recorded state: **2026-10-25** — Session 38 added **mid-match reconnection**:
the server keeps a rejoin ticket per player (registered at join time, so a
reconnect works even before the dropped socket is detected) and re-attaches a new
socket to the same player, kicking the stale one and resending Joined + Map +
full state; the client retries the same join token 5 times (immediate, then
1-4 s) through `GameScene::tryReconnect` and falls back to the localized
"connection lost" path. Server and shared types were changed for this.
Earlier: §37 find_game retry + FPS overlay, §36 background/pause socket
continuity + watchdog + BACK pause, §35 error reporting, §34 region latency test.
**163 host tests pass, 0 fail.**

---

## 1. Milestone scorecard

| # | Milestone | Status | Notes |
|---|---|---|---|
| M0 | Toolchain + axmol builds | **done** | axmol vendored, Gradle-free APK pipeline (`tools/build-apk.ps1`). |
| M1 | Asset pipeline | **done** | atlases, GUI PNGs (4x), Roboto Condensed TTF, l10n. |
| M2 | Shared core ports (bit-exact) | **done** | `BitBuffer`, `Vec2/MathUtil/Collider`, message classes, TS-generated fixtures. |
| M3 | Net + headless sim | **done (host)** | join/update/kill/gameover/pickup/map dispatch, `snapshotText()`; multi-URL join fallback. |
| M4 | Rendering | **done, latest verified** | terrain/island hole, obstacles, buildings + ceilings (obstacle-filtered reveal + stairs suppression), particles, decals, gas, skeletal player. |
| M5 | Input + touch | **mostly done** | move/aim/fire → `InputMsg`; dual pads render at the web's scale (`PadGraphicsAxmol`), aim-line dots wired; emote + team-ping wheels own their touches. |
| M6 | Audio | **mostly done** | engine + mixer + ambiance + explosion/bullet/shot sounds + per-source EAX reverb + per-source EFX low-pass EQ (muffled/club) + structure interior ambience + building `soundEmitters`. UI sounds still unwired (see §2). |
| M7 | UI layer | **S1–S7 landed** | The JNI `SurvBridge` seam + Java menu/loadout/team/pause are in place; the in-game HUD is the plain pix adapter (`game/Hud`), and the emote wheel (`game/EmoteWheel`) + team-ping wheel (`game/PingWheel`) are live with loadout/ammo icons + input. |
| M8 | Accounts / API | **started** | region latency test (`net/PingTest` + `net/Regions` codegen) and client error reporting (`net/ErrorLog` + Java crash handler); login/profile/quests/pass still to do. |
| M9 | Polish & release | **started** | lifecycle: background/pause continuity (§36), mid-match reconnection (§38); perf/soak/store-ready AAB still to do. |

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
| `client/src/map.ts` → `game/Map.*` | ✅ terrain, surfaces, pools, camera emitter, baked minimap texture, big map (`game/MapView`) |
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
| `client/src/objects/structure.ts` → `objects/Structure.*` | ✅ bounds/masks/stairs/map icons + `interiorSound` ambience tracks |
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
| `client/src/ui/ui.ts` (HUD) → `game/Hud.*` (pix adapter) | ✅ health/boost, weapon cards, ammo, alive/kills, kill feed, pickup message, kill leader, gas timer, game-over result overlay, squad health bars, gear/perk icons |
| `client/src/ui/touch.ts` (pad gfx) → `ui/PadGraphicsAxmol.*` | ✅ real ring + touch dot at the web's scale |
| `client/src/emote.ts` (pings + emote wheel + indicators) | ✅ world emotes (def textures) + ping indicators + minimap ping sprites + emote wheel UI/input/throttle + team-ping wheel (pings, medical/ammo emotes) |
| `client/src/objects/mapIndicator.ts` / `mapSprite.ts` | ❌ **not ported** |
| `client/src/input.ts`/`inputBinds.ts` (mouse/keyboard/binds) | ❌ **not ported** (BT keyboard/controller) |
| Player action sounds (footsteps, door, bush, weapon deploy/switch, heal, pickup, dry fire, haste) | ❌ **not wired** — `Player` lacks surface/step/action state |
| UI sounds (ambient lab, pickup pings, notifications) | ❌ **not wired** — UI is Java |
| Reverb (`setReverbProperties`) | ✅ `ReverbModel` maps cathedral/cave to EAX props; applied to every active handle (web convolver is global) |
| `client/src/ui/ui2.ts` (inventory v2: weapon cards, perk icons, throwable arc) | ✅ weapons/ammo in `game/Hud`, gear/perks in `ui/FguiHud` (not built by the live scene yet) |
| `client/src/emote.ts` (emote/ping system) | ✅ see the emote row above |
| `client/src/ui/pieTimer.ts` (action progress circle) | ✅ `ui/FguiPieTimer` driven by `ActivePlayerData.actionTime` |
| `client/src/ui/profileUi.ts` / `pass.ts` (account/profile/battle pass) | ❌ **not ported** (M8) |
| `client/src/crosshair.ts` (custom crosshair) | ✅ screen-space crosshair sprite from the loadout style/colour/size/stroke (rasterized `crosshairDefs` styles, 0/1/2 stroke buckets) |
| `client/src/pingTest.ts` (pre-game latency check) | ✅ `net/PingTest` (echo cadence/retry/complete + `bestRegion`/zones, host-tested); the probe list comes from `net/Regions` (`tools/codegen_regions.mjs`, the native `PING_TEST_URLS`), and the Java menu shows the ms and auto-selects the closest region |
| `client/src/errorLogs.ts` (client error reporting) | ✅ `net/ErrorLog` (sample/budget/throttle host-tested) + `SurvApi.reportError` crash handler in `AppActivity`; `debug.surv_debug=1` sends a startup probe |
| `client/src/account.ts` / `proxy.ts` | ❌ **not ported** (M8 accounts; `proxy.ts` is web-only) |
| `client/src/sdk/*` (ad/partner SDK) | ❌ out of scope |
| `client/src/debug/*` | ❌ **not ported** (dev tooling) |
| `client/stats/*` (leaderboard web pages) | ❌ separate app |

---

## 3. Remaining work — prioritized TO-DO list

### Priority 1 — Core gameplay gaps (most noticeable)

- [x] **Active player full-object desync** — cause was a stale
      `src/net/generatedDefs.inc` (757 game types vs the server's 676), so every
      `gameType` id mapped to a different name. Regenerated with
      `node --experimental-transform-types tools/codegen_defs.mjs
      src/net/generatedDefs.inc` and refreshed `tests/ReferenceData.h`; ids now
      match `GameObjectDefs.typeToId` exactly (e.g. outfitBase 409, backpack00
      253). Loot/gear/outfit/weapon/emote decoding verified on device.

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
  - Live HUD: victory/defeat, rank, team kills, local kills/damage/survival
    (`Hud::showGameOver`), shown while `GameOverMsg` is pending
- [x] **Minimap** (render from `RenderTexture`) — live HUD: baked 1024×1024 map texture under the top-right frame (`Map::generateMinimapTexture` + `Hud::setMinimap`)
- [x] **Gas timer** (`#ui-gas-timer`) — countdown under the minimap from `Gas::duration`/`circleT`, red while moving, hidden when inactive
- [x] **Squad health bars** (4 team member health bars with status indicators) — live HUD: `Hud::setSquadState`, names, colour bars, Dead/Down/DC; hidden in solo
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
- [x] **Squad health bars** (4 team member health bars with status indicators) — live HUD: `Hud::setSquadState`, names, colour bars, Dead/Down/DC; hidden in solo:
  - FguiHud.buildSquadBars() builds 4 slots with name, health bar, status text
  - Driven by Game::getTeamPlayerIds() + groupStatus() + playerStatus
  - Shows green/amber/grey bar based on health, "Dead"/"Down"/"DC" status labels
  - Panel hidden in solo mode, shown top-left in team/squad modes
  - Wired into FguiHud::update() per frame
- [x] **Crosshair** (`crosshair.ts` - custom crosshair rendering):
  - Screen-space sprite at the viewport centre above the world
  - Loadout style (`crosshair_*`), colour, size and stroke (0/1/2 buckets)
    from the Java Customize editor via the persisted config
  - Styles rasterized from `crosshairDefs.ts` to `Content/gui/crosshair/`
- [x] **Touch pad sizing** — `PadGraphicsAxmol` was 4x too small; the ring is
  now sized to the web's `pad.img` (208 px * 0.6) and the dot to 208 px * 0.25
- [x] **World-space name labels** — player/dead-body labels were vertically
  mirrored under the camera's negative-Y root; `pix::Text::setFlipY()` counters
  the flip about the anchor (`M4_REMAINING` §16)
- [x] **Minimap** (baked RenderTexture + live HUD `MapView`):
  - Map::generateMinimapTexture() renders terrain + static `MapMsg.objects` (obstacles/buildings)
    into a 1024×1024 pix::RenderTexture
  - Map::getMinimapTexture() / getMinimapScale() for runtime access
  - Hud::setMinimap() displays it under the top-right frame
  - Zoomed to the web client's 25.6%-of-map view, centred on the active player
    and clipped to the 140px frame (big map still fits the whole map)
  - Player/team dots + gas/safe-zone circles (`updateMapOverlay`)
  - Role-specific icons from `roleDefs.ts` (leader star, captain, medic, last
    man; dead variants) and the web dead/downed scale/z matrix
  - Minimap ping sprites (`ping-map-*` + pulse) clipped to the frame
- [x] **Big map** (`game/MapView`, 640×640 centred):
  - Minimap expand button opens it; X or tapping outside closes it
  - Same texture/dots/gas/pings as the minimap, with the HUD dimmed behind
  - Tap inside the map opens the team-ping wheel; the ping is sent at the
    tapped world position (`bigmapCollision`/`getWorldPosFromMapPos`)
  - Airstrike warning zones (`plane.ts AirstrikeZone`) and place names draw on
    both views
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
  - [x] **Team-ping wheel** (`game/PingWheel`): on-screen ping button + six
        selectors in `emote.ts` order (danger/coming/help pings, medical and
        ammo team emotes, centre close)
  - [x] **Ping wheel input**: touch on the button opens the wheel; dragging past
        the dead zone sends immediately (shares the emote throttle/grey-out)
  - [x] **Ammo-specific ping emotes**: equipped weapon `ammoType` → `emote_ammo*`
        (`emote.ts AmmoTypeToEmote`), refreshed when the weapon changes
  - [x] **Ping defs codegen** (`PingRenderDef`): texture/mapTexture/sound/
        soundLeader/lifetimes/worldDisplay/mapEvent/tint from `pingDefs.ts`;
        `EmoteManager::addPing` now resolves the group indicator slot, group
        tint, edge indicator visibility, and resets fade-out alpha on reuse
  - [x] **Map ping sprites** (minimap dot from ping sprite barn) — `EmoteManager`
        stores def map textures + pulse rings, `Hud` draws them in a container
        clipped to the minimap (session 13)
  - [x] Emote sound playback on addEmote

- [x] **FairyGUI touch pads** (`FguiPadGraphics.*`) — implement:
  - Real pad sprites (`pad.img` with alpha/scaling)
  - Aim-line dots (`LineSprites`: dot sprite every 1.5 units, ray-cast against obstacles)

- [x] **Inventory UI v2** (`ui2.ts` features — weapon cards, perk icons, loot pickup feed, throwable arc preview, team inventory panel)

- [x] **Pie timer** (`pieTimer.ts` — action progress circle)

- [x] **Crosshair** (`crosshair.ts` — custom crosshair rendering)

### Priority 3 — Audio polish

- [x] **Reverb volume computation** — done in `AudioManager::update` (layer/underground)
- [x] **Reverb mapping** — `audio/ReverbModel.h` maps the cathedral/cave `ReverbDef` to EAX params; `AxmolAudioBackend` applies them to all active handles (and new ones in `play()`), replacing the old silent-carrier stub
- [x] **UI/pickup sounds** — wired: `Game::handlePickup` → `GameScene::onPickupSound` plays `item + "_pickup_01"` on `"ui"`; emote/ping sounds play on `"ui"` via `EmoteManager`
- [x] **Reverb** — mapped and applied per active sound handle (see §3); audible check underground is the remaining verification
- [x] **Role announcements / kill leader** — `RoleAnnouncementMsg` sounds (role `soundAssign`/`soundDead`, `ui` channel) and a top-centre kill-leader label driven by `UpdateMsg.killLeaderDirty`

### Priority 4 — Minor simulation/render parity

- [x] **Map::getGroundSurface** — `SurfaceType` now covers every web material
      (water/sand/grass/tile/asphalt/brick/stone/carpet/bunker/container/
      warehouse/house/shack/snow). `Building::surfaces` is codegen'd from
      `def.floor.surfaces` (with per-surface water colours) and transformed per
      building, so `footstep_${surface.type}` matches the web client on floors.
- [x] **Structures** — `mapHelpers.getBoundingCollider` is ported into the def
      codegen (deterministic; weighted map-object candidates are unioned), so
      building/structure bounds and minimap fallbacks are real. Structure layer
      `underground` keeps the `i == 1` default, and `Map::isUnderground` matches
      `player.ts` (layer 1 only, second layer, player radius, default true).

### Priority 5 — Platform / release (M8 + M9)

- [x] **Region latency test** (`client/src/pingTest.ts`, the first M8 slice):
      - `net/Regions.{h,cpp}` + `generatedRegions.inc`: the probe list is
        generated from the server deployment's `regions` config by
        `tools/codegen_regions.mjs` — the native counterpart of the web's
        `PING_TEST_URLS` (`client/vite.config.mts`). **No server change**: the
        client never asks the API for addresses.
      - `net/PingTest.{h,cpp}`: `ConnectionFactory`-injected port of the echo
        probe (1 byte on `/ptc`, 6 replies, 1 retry, 8 Hz cadence), with
        `bestRegion()`/`zones()` and a `resultsJson()` snapshot. Host-tested
        against `FakeConnection` (`tests/test_ping_test.cpp`).
      - `GameScene` starts the probes from the generated table on init and ticks
        them while the menu is up; results go back through
        `UiHost::pingResults` → `SurvBridge.onNativePingResults` →
        `MenuFragment.onPingResults` (no Java→C++ command needed).
      - Java menu: renders `· 29 ms` in the region label + dropdown,
        auto-selects the fastest region when the player has not chosen one
        (`config.regionSelected`), and passes the region's zones (fastest first)
        to `find_game`.
      - Regenerate after changing `regions` in `survev-config.hjson`:
        `node --experimental-transform-types tools/codegen_regions.mjs --production`
        (omit `--production` for the dev defaults used by `pnpm dev`).
- [x] **Error reporting** (`client/src/errorLogs.ts`):
      - `net/ErrorLog.{h,cpp}` ports the policy exactly: 1% sampling for
        `storeGeneric` (with `parent == "account"` always dropped), five reports
        per budget with a 180 s throttle that every suppressed report re-arms,
        and at most two `windowOnError` reports per process. Poster/clock/random
        are injected; `tests/test_error_log.cpp` covers body shape, budget decay,
        throttle/re-arm, sampling and the two-error cap.
      - `net/Api::postJson` posts through the existing axmol `HttpClient`
        (fire-and-forget, like the web `fetch`).
      - `AppActivity` installs a default uncaught-exception handler: a main-thread
        crash posts `windowOnError` synchronously via `SurvApi.reportError`
        (1.5 s join, because the process is about to die); a worker-thread crash
        uses the budgeted C++ reporter through `SurvBridge.reportError`.
      - `debug.surv_debug=1` (the EQ trace switch) sends one `startup_probe`
        error at launch, which is how the pipeline is verified on device.
- [ ] **M8 Accounts/API** (rest): login/profile, team links, quests/pass
      (`PassFragment` is still static), proxy detection (not applicable to
      native)
- [x] **Lifecycle hardening (mid-match continuity)**:
      - Backgrounding and the in-game pause dialog keep the game socket.
        `Game::pause()` used to `close("background")` and `resume()` re-joined
        with the same join token, but the server deletes a token on join
        (`server/src/game/client.ts`), so every short app switch lost the match.
        `pause()` now only freezes input (`Game::sendInput` no-ops while paused)
        and `resume()` keeps playing on the surviving socket.
      - A 12 s liveness watchdog (`Game::kConnectionTimeoutSeconds`) closes a
        socket that stops delivering frames after joining - half-open mobile
        connections never send a close - so `GameScene` takes the existing
        localized "connection lost" path instead of freezing.
      - `WebSocketConnection` caps its queued events at 4096 and turns an
        overflow into a `1006 backlog` close (buffered protocol deltas cannot be
        skipped), so a long background cannot grow the queue unbounded.
      - Back button: in a match, BACK opens/closes the Java pause dialog
        (`UiHost::togglePause` -> `SurvBridge.onNativeTogglePause`); the
        dialog's Quit calls `onQuitGame`. Hosts without that UI fall back to
        leaving the match.
- [x] **Mid-match reconnection**:
      - Server (`server/src/game/{game,client}.ts`): a **rejoin ticket** per
        player, keyed by the join token and registered at join time (the socket
        close can lag behind the client's reconnect by ~10 s on a lost link).
        `ClientBarn.rejoinClient()` marks the old client `replaced`, closes its
        socket and re-attaches a new one; `Client.beginRejoin()` resends Joined +
        Map + every visible object, so the client rebuilds from a full snapshot.
        Tickets are pruned when the player dies/despawns; `GameWsDisconnectReason`
        gained `"rejoined"` for the kicked socket.
      - Client (`GameScene::tryReconnect`): only transport closures (0/1006/1011)
        are retried - immediate, then 1 s, 2 s, 3 s and 4 s apart - each attempt a
        full `enterWithJoin()` on the same token. A rejection after the attempts
        (or the window closing) shows `index-host-closed`; a join that never
        opens now synthesizes `close(1006)` (`Game::onError`) so retries/URL
        fallbacks are never stuck.
      - Trade-off: the ticket makes the join token a bearer credential for the
        match (single use only for the first join), so a leaked token could steal
        the seat until the player dies.
      - The other lifecycle item (menu HTTP retries) landed in §37.
- [x] **Perf tooling (start of Perf/polish)**: the pause dialog's "Show FPS"
      now toggles the engine stats overlay (`Director::setStatsDisplay`), persists
      `showFps` in the config JSON, applies it at startup in
      `AndroidUiBridge::setScenes` and is also enabled by `debug.surv_debug=1`;
      the "Low GFX" toggle is still a stub (a resolution tier needs an EGL
      surface scale)
- [ ] **Perf/polish (rest)**: 60 fps on mid-range devices, notch/insets, battery/resolution tiers, crash-free soak
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
# host tests (163 tests, 0 failures expected)
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

Regenerate the latency-test targets after changing `regions`:
```powershell
node --experimental-transform-types tools/codegen_regions.mjs --production
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
- Door sprite anchors are PIXI (top-left) values converted to axmol (bottom-left) with `y' = 1 - y`; `doorInterpPos` seeds from `data.pos`, `doorClosedPos` updates every door update, and door position/rotation use `def.doorOpenSpeed` (not a constant)
- `AxRenderTexture::resize` recreates the framebuffer (axmol `setContentSize` does not)
- `AxSprite::setRenderTexture` sets the texture rect (axmol `setTexture` keeps a zero-size quad)
- Minimap ping sprites stay in the masked layer (the 512px pulse ring spills otherwise)
- The masked ping layer and the minimap clip must be created **lazily** (`MapView::setPings` / `setCenter`, not HUD build): a ClippingNode created at build time crashed axmol's GL renderer on device
- The minimap bakes the **static** `MapMsg.objects` (not the live pools, which are empty during `loadMap`); the big map uses the same texture fitted to 640px
- `Hud` hit tests (`minimapExpandHit`, `bigMapContains/CloseHit`) take design-space coords; widget positions are centred HUD coords
- `MapMsg` place positions are normalized (0..1); do not divide by map width/height again
- Place names are drawn as HUD text in `MapView`, not baked into the RenderTexture (text-in-RT did not show on device)
- Map dot styles (role icon/dead/downed scale + outer ring + z bump) come from `mapDotStyle`; keep it in sync with `ui.ts updatePlayerMapSprites`
- Reverb is per-source OpenAL EAX: `AxmolAudioBackend` iterates active handles (no global convolver bus); `ReverbModel` is the host-tested mapping
- The camera root is **positive-Y** (`Renderer::applyCamera`): axmol y-up origin = `screenH - pointToScreen(0,0).y`; world +y is screen-up. World labels do not `setFlipY`
- Web/PIXI rotations on **world** nodes must be negated for the positive-Y root (obstacle/dead-body/flare/plane/bullet-ping sites already do)
- Ground surfaces use the full `SurfaceType` enum + `surfaceTypeName` (the web's lowercase string is the `footstep_<type>` group); building floors come from codegen'd `def.floor.surfaces`, not terrain
- Structure layer `underground` is tri-state: the def may omit it, in which case `i == 1` (structure.ts); do not collapse it to a bool at codegen time
- `MapObjectDef::boundingCollider` is the deterministic `computeBoundingCollider` port (structures add a 1-unit margin; buildings union floor/zoom/map-object bounds); do not fall back to `def.collision` for buildings
- Ceiling visibility comes from `ObstacleScan.h::scanCollider` (obstacle-filtered rays), not a circle/zoomIn overlap; `Player::noCeilingRevealTicker` must stay wired to `noCeilingReveal` stairs
- `ObstacleScan.h` is the shared port of `collisionHelpers.ts` (player melee/LOS + building vision); add new helpers there instead of duplicating them in a `.cpp` anonymous namespace
- Structure interior ambience is immediate-mode: `Map::update` (structure updates) must run before `Ambiance::update` each frame, and `Structure::update` re-sets `interior_0/1` sound/filter/weight every frame
- `SurvBridge` JNI methods run on the Java UI thread: scene/world/audio mutations must go through `UiBridge::runOnGameThread` (`Scheduler::runOnAxmolThread`); calling `enterWithJoin`/`leaveGame` inline races the GL thread and crashes in the player pool
- Building `soundEmitters` are owned by `Building`: created on `isNew`, stopped in `m_free()` via the cached `audioOwner`, and updated at 10 Hz with `setVolume(..., "sound")`
- Per-source EQ is an axmol patch: re-vendoring **must** run `tools/vendor-axmol.ps1` (which calls `patch-axmol-audio-filter.ps1`) or `AudioEngine::setFilterProperties` disappears; `audio/AudioFilter.h` maps muffled/club to the EFX low-pass
- Android has **no C++ MenuScene** (`installUiBridge(nullptr, gameScene)`): bridge config/name/loadout/crosshair writes use the standalone `UserDefaultConfigStorage` Config, and `joinUrls` must set the `Game` join info via `_game->getGame()`; `GameScene::enterWithJoin` must not overwrite it
- Crosshair sprites come from `Content/gui/crosshair/<type>_s<0|1|2>.png` (`build-crosshair-icons.mjs`, staged by `sync-content.ps1`); the nested `crosshair` config shape wins over stale flat keys
- `AudioManager` resolves `filter` like the web (different layer or `forceFilter` → EQ, otherwise `reverb`); the app-only backend trace is enabled with `debug.surv_debug=1` and prints `[audio] play ... filter=...`
- `SurvevMobile-debug.apk.idsig` is a tracked build artifact refreshed by every
  `build-apk.ps1` run; commit the refreshed file along with the build.
- Emote/ping wheel frames are re-applied by `refreshFrames()` after the map
  atlases load (the wheels exist before the first `MapMsg`), and `Content/atlas`
  must be restaged (`build-atlas.mjs`) after `client/atlas-builder/defs/*.ts`
  changes or new icons render blank.
- The region latency test is C++ (`net/PingTest`) and ticked by `GameScene`
  (`update()`), not Java: the menu is Java-only, so results come back
  asynchronously through `UiHost::pingResults` ->
  `SurvBridge.onNativePingResults`. Do not create probe sockets on the UI thread.
- `GameScene::init()` starts the probes from `net/Regions` (`generatedRegions.inc`,
  produced by `tools/codegen_regions.mjs` from `Config.regions` — the native
  `PING_TEST_URLS`). Do not fetch region addresses from `site_info`: the server
  does not expose them and must not be changed for this.
- Regenerate `src/net/generatedRegions.inc` after editing `regions` in
  `survev-config.hjson` (`--production`) or when the dev defaults change; the
  host test `ping_regions_generated_table` guards the output.
- `PingTest` must never handle a close/error by destroying its own `Connection`
  inside the callback (`needReopen` defers it to after `pump()`).
- `UiBridge::setRegion` also persists `regionSelected = true` (main.ts config
  listener), so the ping auto-selection does not override an explicit choice on
  the next launch.
- Error reporting is `net/ErrorLog` (client/src/errorLogs.ts): do not send
  reports ad hoc, and keep the web's budget semantics (five reports, 180 s
  re-armed throttle, 1% `storeGeneric`, two `windowOnError`). The main-thread
  crash handler deliberately bypasses it (`SurvApi.reportError`, synchronous)
  because the game thread dies with the process.
- `debug.surv_debug=1` (`adb shell setprop debug.surv_debug 1`) sends one
  `startup_probe` report at launch and enables the find_game/audio dev traces;
  it must not send anything in normal runs.
- Background pause / the in-game pause dialog must **not** close the game socket:
  join tokens are single-use, so a rejoin is impossible and closing always lost
  the match. `Game::pause()` only freezes input; `Game::resume()` keeps the
  surviving socket. Do not reintroduce `close("background")` or a rejoin retry.
- The liveness watchdog (`Game::kConnectionTimeoutSeconds`, 12 s) is the only
  thing that closes a silently dead (half-open) socket; it synthesizes
  `close(1006, "timeout")` so the existing localized disconnect path runs.
- `WebSocketConnection`'s event queue is capped at 4096 frames and turns an
  overflow into `close(1006, "backlog")` (deltas cannot be skipped); do not
  raise the cap without bounding background memory some other way.
- Rejoin tickets are registered **at join time** (`Game.addRejoinToken`), not on
  socket close: the server can take ~10 s to notice a dead link, and the client
  reconnects sooner. Do not move that registration back into `handleSocketClose`.
- A rejoin marks the previous `Client` as `replaced` and closes it; its
  `handleSocketClose` must return before the despawn/input-reset logic or the
  seat is torn down mid-reconnect.
- The client only retries transport closes (0/1006/1011) and never a rejection
  reason (`invalid_token`/`ip_banned`/`rate_limited`/`behind_proxy`); server
  rejections must fall through to the localized error.
  `Game::onError` synthesizes `close(1006, "error")` when an open fails so the
  reconnect/URL fallback is not stuck on a socket that never opened.