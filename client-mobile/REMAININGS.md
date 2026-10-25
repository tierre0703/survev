# client-mobile — what is left to implement (1:1 web → axmol port)

Consolidated status of the native Android port of `client/` (`client-mobile/`).
This file is a **summary/index**; the detailed work orders live in:

- `plan.md` — module map, milestones M0–M9, decisions D1–D5.
- `M4_REMAINING.md` — rendering (sessions 1–6), verified object-render parity.
- `M6_AUDIO.md` — audio port notes.
- `ARCHITECTURE.md` — UI architecture: gameplay = axmol + FairyGUI, every
  other screen = Android Java, one JNI bridge (`SurvBridge`). Read §5 (move
  matrix) and §7 (migration steps S1–S7) before touching UI code.

Latest recorded state: **2026-10-25 (1:1 audit)** — a full read of every `client/src`
module against the live native code found that the protocol/simulation/render
primitives, account/pass/team, audio engine and release pipeline are close to
1:1, but the client is **not yet a 1:1 playable game**: gameplay action inputs
(loot/reload/heal/equip/drop), the renderer active layer, entity/camera
interpolation, death/spectate/result, a real loadout editor and applied settings
are missing. The old checklist in this file was too optimistic (some ✅ features
only exist in the dead `FguiHud`); **§3 has been rewritten from the audit as the
new P0/P1/P2 TO-DO list**. Store packaging (§45) remains verified. **163 host
tests pass, 0 fail.**

---

## 1. Milestone scorecard

| # | Milestone | Status | Notes |
|---|---|---|---|
| M0 | Toolchain + axmol builds | **done** | axmol vendored, Gradle-free APK pipeline (`tools/build-apk.ps1`). |
| M1 | Asset pipeline | **done** | atlases, GUI PNGs (4x), Roboto Condensed TTF, l10n. |
| M2 | Shared core ports (bit-exact) | **done** | `BitBuffer`, `Vec2/MathUtil/Collider`, message classes, TS-generated fixtures. |
| M3 | Net + headless sim | **done (host)** | join/update/kill/gameover/pickup/map dispatch, `snapshotText()`; multi-URL join fallback. |
| M4 | Rendering | **mostly done** | terrain/island/obstacles/buildings/particles/decals/gas/skeletal player; missing active-layer wiring, interpolation, bullet/explosion/projectile/loot FX (§3 P0.2/P0.3/P1.3). |
| M5 | Input + touch | **partial** | move/aim/fire only; all loot/reload/heal/equip/scope/drop inputs are missing (§3 P0.1). |
| M6 | Audio | **mostly done** | engine/mixer/ambience/reverb/EQ/interior sounds; startup volume/mute, pause/menu restart and many action cues are missing (§3 P1.5). |
| M7 | UI layer | **partial** | Java menu/account/pass/team live; live HUD lacks inventory v2/spectate/settings; `FguiHud` is dead on Android (§3 P0.4/P0.7, P1.4). |
| M8 | Accounts / API | **mostly done** | device login + profile, quests/pass, team links; quickplay is unauthenticated, captcha unsupported, error reasons wrong (§3 P1.1/P1.2); OAuth is an intentional platform delta. |
| M9 | Polish & release | **started** | release APK/AAB pipeline verified (§45); waiting on the 1:1 gameplay gate (§3.7), a physical-device soak and final store upload. |

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
| `client/src/renderer.ts` → `render/Renderer.*` | 🔶 transform/z-sort/mask ported; `setActiveLayer` is never called (P0.2) |
| `client/src/map.ts` → `game/Map.*` | ✅ terrain, surfaces, pools, camera emitter, baked minimap texture, big map (`game/MapView`); river-shore surface is always sand (P2) |
| `client/src/gas.ts` → `game/Gas.*` | ✅ |
| `client/src/game.ts` (main loop, net dispatch) → `game/Game.*`, `game/GameWorld.*` | 🔶 dispatch/net is good; no match-over latch (P0.5), no entity/camera interpolation (P0.3) |
| `client/src/objects/objectPool.ts` → `game/objects/GameObject.h` | ✅ |
| `client/src/objects/player.ts` + `animData.ts` → `objects/PlayerRender.cpp` + `Barns.*` | 🔶 poses/animations faithful; team patch, heal/revive/AoE emitters, bleed particles, nametag gating, action-sound keys, radius/easing missing (P1.3) |
| `client/src/objects/bullet.ts` → `objects/Barns.*` (BulletBarn) | 🔶 whiz sound only; per-def tracers + all impact FX missing (P1.3) |
| `client/src/objects/explosion.ts` → `objects/Barns.*` (ExplosionBarn) + `GameWorld.cpp` | 🔶 sound map only; visuals are one orange circle, no particles/shake/ripples (P1.3) |
| `client/src/objects/shot.ts` → `objects/Barns.*` (ShotBarn) | 🔶 fire sound; no last/alt/team variants, cycle/pull timing or casings (P1.3/P2) |
| `client/src/objects/obstacle.ts` → `objects/Barns.*` | 🔶 door/button/destroy sounds, particles and button sprite swap missing (P1.3/P2) |
| `client/src/objects/building.ts` → `objects/Barns.*` | 🔶 ceiling reveal/sounds partial; puzzle FX and ceiling debris particles missing (P2) |
| `client/src/objects/loot.ts` → `objects/Barns.*` | 🔶 no rarity border/outer ring or drop sound (P1.3) |
| `client/src/objects/decal.ts` → `objects/Barns.*` | ✅ |
| `client/src/objects/deadBody.ts` → `objects/Barns.*` | ✅ (interpolation P0.3) |
| `client/src/objects/projectile.ts` → `objects/Barns.*` | 🔶 static sprite; spin/trail/bounce/water/ondemand ordering missing (P1.3) |
| `client/src/objects/smoke.ts` → `objects/Barns.*` | ✅ |
| `client/src/objects/airdrop.ts` → `objects/Barns.*` | 🔶 fall loop restarted every frame; ripple delay missing (P1.3) |
| `client/src/objects/plane.ts` → `objects/Barns.*` | 🔶 engine sound unused; airstrike zones never instantiated (P1.3) |
| `client/src/objects/flare.ts` → `objects/Barns.*` | 🔶 additive blend is a no-op (P2) |
| `client/src/objects/structure.ts` → `objects/Structure.*` | ✅ bounds/masks/stairs/map icons + `interiorSound` ambience tracks |
| `client/src/objects/particles.ts` → `objects/Barns.*` (ParticleBarn + Emitter) | ✅ near line-for-line; static colour per def (P2) |
| `client/src/audioManager.ts` → `audio/AudioManager.*` | 🔶 startup volumes/mute, master routing to ambience/music, pause restart and channel semantics missing (P1.5) |
| `client/src/soundDefs.ts` → `audio/GeneratedSoundDefs.cpp` (codegen) | ✅ generated |
| `client/src/ambiance.ts` → `audio/Ambiance.*` | 🔶 in-game tracks faithful; no menu ambience, no restart after pause (P1.5) |
| `client/lib/createJS.ts` → `audio/AxmolAudioBackend.h` | 🔶 channel `maxInstances`/coalesce/delay/detune/loop retention ignored (P1.5) |
| `client/src/ui/touch.ts` → `ui/Touch.h`, `game/AimLine.h` | 🔶 pads/aim-line good; move/aim style + sensitivity never applied (P0.7) |
| `client/src/config.ts` → `ui/Config.*`, `ui/ConfigStorage.cpp` | 🔶 schema ported; most settings never written/applied (P0.7) |
| `client/src/device.ts` → `ui/Device.*` | 🔶 simplified (no tablet/pixelRatio/MAX_TEXTURE_SIZE tiers) |
| `client/src/localization.ts` → `ui/Localization.*`, `ui/L10nJson.*` | 🔶 C++ table good; Java menu is hardcoded English and the language list uses invalid `zh`/`ja` (P0.7) |
| `client/src/main.ts` (app bootstrap) → `app/AppDelegate.*`, `app/GameScene.*`, `app/MenuScene.*` | 🔶 join/reconnect good; quickplay auth/captcha, reason mapping and match-over latch gaps (P0.5/P1.1/P1.2) |
| `client/src/api.ts`/`siteInfo.ts` → `net/Api.*`, `net/SiteInfo.*`, Java `SurvApi.java` | 🔶 Bearer/find_game and mode/region metadata gaps (P1.1/P1.7) |
| `client/src/ui/teamMenu.ts` → Java `TeamFragment.java` + `TeamSocket.java` + `TeamLinks.java` | ✅ real `/team_v2` protocol, invite links (copy/share/paste) + `survev://team` deep links, kick/mode/region controls, team match stays connected |
| `client/src/ui/loadoutMenu.ts` → Java `LoadoutFragment.java` | 🔶 grid permanently hidden + invented item IDs; only slot 0 emote; no crosshair types/item art (P0.6) |
| `client/src/ui/ui.ts` (HUD) → `game/Hud.*` (pix adapter) | 🔶 health/boost, basic weapon/ammo, alive/kills (from `GameOverMsg` only), kill feed, kill leader, gas timer, result panel, squad bars, gear icons; most inventory v2/interaction/spectate pieces live only in dead `FguiHud` (P1.4) |
| `client/src/ui/touch.ts` (pad gfx) → `ui/PadGraphicsAxmol.*` | ✅ real ring + touch dot at the web's scale |
| `client/src/emote.ts` (pings + emote wheel + indicators) | ✅ world emotes (def textures) + ping indicators + minimap ping sprites + emote wheel UI/input/throttle + team-ping wheel (pings, medical/ammo emotes) |
| `client/src/objects/mapIndicator.ts` / `mapSprite.ts` | 🔶 partial: barn exists but is constructed with a **null** sprite barn; `MapSprite` z-order/pulse not applied (P2) |
| `client/src/input.ts`/`inputBinds.ts` (mouse/keyboard/binds) | ❌ **not ported** (BT keyboard/mouse; only Escape handled) — P1.6 |
| `client/src/ui/opponentDisplay.ts` (menu character preview / opponent display) | ❌ **not ported** (P1.7) |
| `client/src/resources.ts` (per-map load/unload, audio preload, MAX_TEXTURE_SIZE) | 🔶 atlases load once and never unload; no map-audio preload (P2) |
| Player action sounds (footsteps, door, bush, weapon deploy/switch, heal, pickup, dry fire, haste) | 🔶 many wired, but reload/deploy use gunshot keys, pickup uses wrong keys, dry fire is approximate, plane/doors/buttons silent (P1.5) |
| UI sounds (ambient lab, pickup pings, notifications, victory) | ❌ **not wired** (P1.5/P2) |
| Reverb (`setReverbProperties`) | ✅ `ReverbModel` maps cathedral/cave to EAX props; applied to every active handle (web convolver is global) |
| `client/src/ui/ui2.ts` (inventory v2: weapon cards, perk icons, throwable arc) | 🔶 inventory v2 is only in the dead `ui/FguiHud`; live HUD lacks ammo reserve, loot grid, scopes, interaction prompt, pickup/kill messages (P1.4) |
| `client/src/emote.ts` (emote/ping system) | ✅ see the emote row above |
| `client/src/ui/pieTimer.ts` (action progress circle) | 🔶 arc/countdown works; action label (Reloading/Using/Reviving) missing (P2) |
| `client/src/ui/profileUi.ts` / `pass.ts` (account/profile/battle pass) | ✅ `ProfileFragment` + `PassFragment` (real level/XP, next unlock, quest reroll); stats link inert, no unlock image/animations (P2) |
| `client/src/account.ts` | ✅ Java `AccountManager` + `SurvApi` device login/session/profile/username/logout/reset/delete/loadout push; OAuth/linking intentionally N/A |
| `client/src/crosshair.ts` (custom crosshair) | 🔶 sprite pipeline good; editor offers 7 colours/3 sizes/3 strokes and no type picker (P1.7) |
| `client/src/pingTest.ts` (pre-game latency check) | ✅ `net/PingTest` (echo cadence/retry/complete + `bestRegion`/zones, host-tested); the probe list comes from `net/Regions` (`tools/codegen_regions.mjs`, the native `PING_TEST_URLS`), and the Java menu shows the ms and auto-selects the closest region |
| `client/src/errorLogs.ts` (client error reporting) | ✅ `net/ErrorLog` (sample/budget/throttle host-tested) + `SurvApi.reportError` crash handler in `AppActivity`; `debug.surv_debug=1` sends a startup probe |
| `client/src/account.ts` / `proxy.ts` | 🔶 device login/session/profile (see above); `proxy.ts` is web-only |
| `client/src/sdk/*` (ad/partner SDK) | ❌ out of scope |
| `client/src/debug/*` | ❌ **not ported** (dev tooling) |
| `client/stats/*` (leaderboard web pages) | ❌ separate app |

---

## 3. 1:1 parity audit (2026-10-25) and next TO-DO list

**Scope.** Every `client/src` module (excluding `sdk/`, `debug/`, `stats/` and the
browser-only `proxy.ts`) was read against the **live** native code, not against
the status docs. The audit found that the protocol, map/simulation, rendering
primitives, account/pass/team, audio engine and release pipeline are close to
1:1, but the port is **not yet a 1:1 playable client**: a small number of core
loops are missing, and several previously "done" items are stubs or only live in
the dead `ui/FguiHud` (Android builds only `GameScene`; `MenuScene`/`FguiHud`/
`UiOverlay` are not created).

### 3.1 Status corrections (the old checklist was too optimistic)

| Claim in this file / earlier sessions | Audit result |
|---|---|
| `ui2.ts` inventory v2 is live in the HUD | ❌ only in the dead `FguiHud`; live `game/Hud` has no ammo reserve, loot grid, scopes, interaction prompt, pickup/kill messages |
| `bullet.ts`, `explosion.ts`, `projectile.ts`, `loot.ts` are ✅ | 🔶 whiz/explosion sound only; tracers/impact FX, explosion visuals, projectile spin/trail/bounce and loot rarity borders are missing |
| `player.ts` player rendering is ✅ incl. team patches | 🔶 poses/animations are faithful, but team patches, heal/revive/AoE emitters, bleed particles, nametag gating and action sounds are missing/wrong |
| Player action sounds ✅ | 🔶 reload/deploy play `"<weapon>_01"` (gunshot) and pickup plays `"<item>_pickup_01"`; only some cues work |
| Settings/config are ✅ | ❌ most settings toggles only call `saveConfig()` and never write/apply the value; touch styles never reach `Touch`; audio is not restored at launch |
| HUD is ✅ health/ammo/killfeed/result | 🔶 basic HUD only; result screen ignores `GameOverMsg.gameOver`, no spectate, no Play Again |
| Input/touch is ✅ move/aim/fire | 🔶 only move/aim/fire; **no interact/reload/use/equip/scope/stow/drop inputs are ever sent** |
| Team/accounts done | ✅ team protocol/links and device account are real; quickplay is unauthenticated and captcha is unsupported (see P1) |
| Store-ready done | ✅ scripts verified; a real upload still needs the production keystore (intentional) |

### 3.2 P0 — blockers for a correct 1:1 game

These make the game unplayable or visibly wrong in normal play. Do them first.

- [ ] **P0.1 Send the gameplay action inputs and add their HUD buttons.**
      `InputMsg.addInput`/`useItem` exist (`net/Messages.h:118-132`) and
      `Input_Interact/Reload/Use/UseBandage/...`/`Action_UseItem` are in
      `core/GameConfig.h:69-101`, but **nothing calls them**:
      `TouchInput::build` only fills move/aim/shoot (`src/game/TouchInput.h:44-70`)
      and `GameScene::updateInput` sends that (`src/app/GameScene.cpp:1489-1514`).
      Port the web's `game.ts:565-656`/`ui.ts:388-399`/`ui2.ts:266-331`:
      - Reload (tap reload, auto-reload) and reload-alt.
      - Interact (loot/open/revive), Use (heal/boost), Use bandage/medkit/soda/pills.
      - Equip slot 1–4, next/prev/last weapon, next scope, stow, swap weapon slots (drag).
      - Drop item (`DropItemMsg`), use-item (`useItem`), cancel action.
      - Add the on-screen reload + interaction buttons and hit-test/route them from
        `GameScene::onTouchBegan` (web `index.html:174-179`, `ui.ts:388-399`).
      - Acceptance: loot, reload, heal, revive, switch/throw/drop weapons and
        scopes all work on device and the server sees the action inputs.

- [ ] **P0.2 Wire `Renderer::setActiveLayer` (underground/bunker rendering).**
      `Renderer::setActiveLayer` exists (`src/render/Renderer.h:38`) but has **zero
      call sites**; `m_update` keeps `layer = 0`, so `layers[1]` stays alpha 0 and
      invisible, `groundAlpha` never fades, and the stairs stencil is never
      released (`src/render/Renderer.cpp:250-305`). Call it from the active
      player's `layer` every frame (web `game.ts:1162-1163`) and verify:
      - bunker/cellar interior objects and the player are visible underground;
      - ground alpha and the tree/building mask release correctly on stairs;
      - `activeLayer` also feeds audio filtering (`GameWorld.cpp:477`).
      - Acceptance: enter/exit a bunker on device with the camera/layer correct.

- [ ] **P0.3 Interpolate entities and the camera.**
      Only loot/obstacle skins interpolate (`Barns.cpp:958-964,423-429`); the
      player is drawn at raw `pos` (`PlayerRender.cpp:452-459`), the camera at raw
      `pos` (`GameWorld.cpp:443-445`), and `Game.cpp:426` still has
      `TODO(M4+): interpolate objects`. Port `player.ts:799-821`
      (`m_visualPos`/`posInterpTicker`) and `game.ts:426`:
      - player + camera + bullets + projectiles + planes + airdrops + dead bodies
        interpolate over `camera.m_interpInterval`;
      - respect the `interpolation` setting;
      - the camera follows the visual position (not the raw net position).
      - Acceptance: motion is smooth at 33 Hz net sync; toggling interpolation
        changes behaviour after P0.7.

- [ ] **P0.4 Death → stats → spectate.**
      `PlayerStatsMsg` is parsed and discarded (`Game.cpp:270`); `SpectateMsg`
      exists but is never sent (`Messages.h:261-273`); `JavaUiHost.showResult()`
      is a no-op (`JavaUiHost.java:203-206`) and the pause-dialog Spectate button
      is a stub (`GamePauseDialog.java:240-244`). Port `game.ts:1507-1513,709-724`
      and `ui.ts:1633-1775`:
      - show the death/team-eliminated panel with kills/damage/survival;
      - send `SpectateMsg` Begin/Next/Prev and move the camera to teammates;
      - add "Spectating <name>", spectator count, View Match Stats, Leave Game;
      - gate input while spectating.
      - Acceptance: die in duo/squad → panel → spectate a teammate → leave.

- [ ] **P0.5 Latch match-over so a finished match is not a "disconnect".**
      The server sends `GameOverMsg` on every death and closes with **3000 +
      reason** (`server/src/game/objects/player.ts:2546-2577`,
      `server/src/game/gameProcess.ts:291-295`). The native client has no
      `m_gameOver` latch: when the socket closes it runs the reconnect/error path
      (`GameScene.cpp:1102-1149`), tears the world down and shows
      "Host closed the connection" even after a normal game over. Port
      `game.ts:188-200` (`connected && !m_gameOver && !displayingStats`) and keep
      the result screen/world until the player leaves. Acceptance: finish a match
      and see the result screen, not a disconnect error.

- [ ] **P0.6 Loadout editor: real items + all emote slots.**
      `LoadoutFragment.java:416` permanently hides the item grid
      (`mGrid.setVisibility(isCrosshair ? GONE : GONE)`), the category lists are
      invented IDs (`outfitWoodland/outfitSnow/katana_rusted/emote_gg/heal_advanced`
      at `:697-734`) that the validator rejects (`ui/Loadout.cpp:93-107`), and
      emote selection writes only slot 0 and replaces the array
      (`LoadoutFragment.java:625-631`). Port `ui/loadoutMenu.ts` item lists from
      `shared/defs` (unlocked/owned state), the emote-slot picker (4 wheel + win/
      death), the item confirm flow and the crosshair type picker. Acceptance:
      change outfit/melee/heal/boost/4 emotes/crosshair and join with them.

- [ ] **P0.7 Make settings real (persist + apply).**
      `SettingsModal.java:198-296` only relabels toggles and calls
      `saveConfig()`; it never calls `SurvBridge.setConfigValue`. Volume readback
      uses `Json.peekString` on numbers (`:518-541`) → always fallback; the mute
      toggle is inverted (`GamePauseDialog.java:384-389` vs
      `UiBridge.cpp:203-207`); `Touch` is constructed with `Locked/Locked`
      (`Touch.h:62-64`) although the config default is `anywhere`
      (`Config.cpp:53-55`) and `GameScene` never applies it. Wire:
      - highResTex/interpolation/localRotation/screenShake/anon names;
      - move/aim style + aim line → live `Touch` and persistence;
      - master/sound/music/mute applied at startup and to ambience/music;
      - HUD scale (or remove the placeholder control);
      - correct language list (`da/de/en/es/fr/it/nl/pl/pt/ru/sv/vn/tr/jp/ko/th/
        zh-cn/zh-tw`; currently `zh`/`ja` are invalid and fall back to English).
      - Acceptance: kill/restart the app; every setting persists and changes
        behaviour.

### 3.3 P1 — missing 1:1 features / correctness (playable but wrong)

- [ ] **P1.1 Authenticated quick play + captcha + correct find_game body.**
      Java `findGame` sends no Bearer (`SurvApi.java:138-157`), the server
      `find_game_v2` reads only the cookie (`server/src/api/index.ts:118-162`),
      `MenuFragment` sends `playerCount: teamMode` (2/4 instead of 1), a hardcoded
      `version:1028` and no Turnstile token (`MenuFragment.java:695-712`).
      Team `playGame` sends `zones:["<region>"]` instead of the region's zones
      (`TeamFragment.java:762-771` vs `teamMenu.ts:355-380`). Add Bearer auth to
      find_game (web already authenticates via cookie), read
      `GameConfig.protocolVersion`, and either implement Turnstile or show a
      clear unsupported message and allow logged-in players (server skips captcha
      for authenticated users). Acceptance: quick-play stats/XP/quests count, and
      a captcha deployment works for logged-in players.

- [ ] **P1.2 Correct disconnect reasons and error modals.**
      Native maps invented codes 4001-4011 (`GameScene.cpp:1122-1143`); the
      server always uses 3000 + reason string. Map the reason (`invalid_token`,
      `invalid_protocol`, `ip_banned`, `behind_proxy`, `rate_limited`,
      `server_restart`, `player_not_found`, ...) like `main.ts:839-858`, and add
      the invalid-protocol/IP-ban/behind-proxy/account-banned modals
      (`main.ts:881-912`, `profileUi.ts:332-347`). Acceptance: force each close
      reason and see the localized message.

- [ ] **P1.3 Rendering feedback parity (biggest visual gap).**
      - Bullet tracers: per-def colour/length/width/comet trail, AP/sat/chambered
        tints, suppression fade (`bullet.ts:92-181,235-242`); native is a hardcoded
        2 px line (`Barns.cpp:1497-1535`).
      - Bullet impact FX: wall dust + `sound.bullet`, blood parented to players,
        pan/steelskin sparks, hit sounds (`bullet.ts:33-54,377-458`); native never
        collision-tests bullets.
      - Explosions: burst/scatter particles, water ripples, per-type lifetime,
        camera shake, indoor bomb suppression (`explosion.ts:110-255,355-716`);
        native is one orange circle (`Barns.cpp:1559-1588`).
      - Projectiles: spin/drag, trail, bounce sound/particles, water alpha 0.3,
        stair z-order, always-on-top/indoor hide (`projectile.ts:28-398`); native
        is a static sprite (`Barns.cpp:1288-1304`).
      - Loot: rarity border/outer ring + drop sound (`loot.ts:106-151`); native has
        no border (`Barns.cpp:923-936`).
      - Player: heal/revive/AoE emitters, last-stand aura, team patch, bleed
        particles, nametags only for same-group teammates
        (`player.ts:856-857,992-1027,1984-2074`; native
        `PlayerRender.cpp:729-741,804-859`, `Barns.cpp:1605-1608`).
      - Airdrop/plane/airstrike: airdrop fall loop handle (native restarts the
        sound every frame, `Barns.cpp:1729-1753`), plane engine loop, and
        instantiate `msg.airstrikeZones` (parsed at `Messages.h:1314`, never used;
        `GameWorld::applyUpdate`). Add camera shake triggering
        (`explosion.ts:249-251`; `Camera::m_addShake` unused).
      - Acceptance: screenshot/side-by-side each effect; no sound spam.

- [ ] **P1.4 Live inventory v2 / interaction HUD.**
      Port `ui2.ts` into `game/Hud` (not the dead `FguiHud`):
      reserve ammo (`N/∞`), weapon slot strip with icons/binds/ammo and tap-to-
      switch + drag swap, loot/ammo/heal/throwable counts (orange at max, dim at
      0), scopes strip, interaction prompt (icon/text/tap), pickup/error toasts,
      "You killed X" + in-match kill counter (currently only from `GameOverMsg`),
      rare-loot/perk toasts, kill feed colours, off-screen teammate arrows, faction
      alive counters, gas/role announcements, spectator count, "waiting for
      players", pie-timer label and role-selection menu (`ui.ts`/`ui2.ts`
      references in the audit; `Hud.cpp:650-802`, `GameScene.cpp:1153-1218`).
      Acceptance: full HUD matches `ui.ts`/`ui2.ts` at 1280×720.

- [ ] **P1.5 Audio correctness and coverage.**
      - Apply saved `masterVolume`/`soundVolume`/`musicVolume`/`muteAudio` at boot
        (`AudioManager.h:49-54` starts at 0.5; only the sliders call
        `UiBridge::setAudio`): no saved volume is honored on launch.
      - Route master volume through ambience/music (`Ambiance.cpp:161-167` calls
        `AudioManager::setVolume`, which multiplies only the type volume).
      - Do not `stopAll()` on pause/leave without restarting (`GameScene.cpp:709,
        788`); `Ambiance` tracks never restart and there is no menu ambience.
      - Fix sound keys: reload (`GunDef.sound.reload`), deploy/switch
        (`GunDef.sound.deploy`), pickup (`item.sound.pickup`), plane loop,
        doors/buttons/puzzles, ceiling debris, notification/victory/team-join
        sounds; native currently uses `<weapon>_01`/`<item>_pickup_01`
        (`PlayerRender.cpp:502-517,777-784`, `GameScene.cpp:797-805`,
        `Barns.cpp:1942-1944`).
      - Add channel semantics (`maxInstances`, `canCoalesce`, `delay`, `detune`,
        `ambient`, loop retention) which `AxmolAudioBackend::play` ignores
        (`AxmolAudioBackend.h:59-98`).
      - Acceptance: a scripted sound checklist passes on device; volume/mute
        survive restart.

- [ ] **P1.6 Keyboard/mouse/binds + input policy.**
      Only `KEY_ESCAPE` is handled (`GameScene.cpp:364-370`); `inputBinds.ts` and
      mouse aiming/wheel cycling are absent; `InputMsg` is sent at 33 Hz
      unconditionally instead of on change + 1 s keepalive
      (`GameScene.cpp:1507-1514` vs `game.ts:731-769`). Add the bind model/CRC
      (or document BT keyboard as unsupported), arrows/Escape/G/L, mouse aim and
      wheel, and change-gated input sends. Acceptance: BT keyboard/mouse and
      emulator mouse work; idle upstream traffic matches the web.

- [ ] **P1.7 Menus / metadata / localization polish.**
      - Quick-play buttons and team availability should come from
        `site_info.modes` (labels/icons/`enabled`/teamMode) instead of hardcoded
        Solo/Duo/Squad + `gameModeIdx 0/1/2` (`MenuFragment.java:274-320,705`).
      - Region list should use `site_info` with localized names and require a
        successful fetch before enabling Play; currently falls back to `"na"`.
      - Localize the Java menu/settings/pause/loadout (all hardcoded English) and
        fix the language list (P0.7).
      - News images/date/badge, play spinner/lockout (web backoff), leaderboard
        entry point (`LeaderboardModal` exists but is unreachable), Help content
        (currently describes non-existent features), attributions/HoF/changelog.
      - Crosshair type picker (30 types) + item art/rarity/lore and the item
        confirm-on-acquire flow; menu character preview (`opponentDisplay.ts`).
      - Use `GameConfig.protocolVersion` instead of `"version":1028`.
      - Acceptance: menu looks/behaves like the web at 1280×720 and honours
        `site_info` + locale.

- [ ] **P1.8 Team/account refinements.**
      - Quitting a team match must return to the team room, not close it
        (`JavaUiHost.quitToMenu` calls `showMenu()` first; web
        `main.ts:351-358`).
      - Persist room props on leave (`gameModeIdx`, `teamAutoFill`, `region`).
      - `changeName` in the room; invite link should use the local web origin
        (config `webBaseUrl`) rather than always `https://survev.io`.
      - Feed `account.items` into the loadout, call `set_item_status` (ack/confirm
        + new-item badge), and refresh the profile/pass when `newItems` is set.
      - Handle `?u=<b64>&jt=` join links and `https://…/#CODE` App Links
        (`main.ts:816-837`; native only handles `survev://team/`).
      - Acceptance: full team/account flow matches the web (except OAuth).

### 3.4 P2 — polish / smaller parity gaps

- **Rendering:** faction patch sprite, shell casings, body-radius easing, bleed
  particles, bush particle/sound per def, `maxVisualRadius` 3.75 vs 1.0,
  additive blend (currently a no-op), particle colour variance, river-shore
  `"stone"` surface, map label alpha/stroke, `MapSprite` z-order, ceiling destroy
  particles, obstacle button image swap, plane/airstrike visuals.
- **HUD/menus:** kill-feed colours/role lines, squad bar role/team colours and
  downed pulse, gas icon/pulse, gear order/levels/tap-to-drop, minimap alpha
  fade + body tap-to-expand + hide toggle, big-map dim, pie-timer label, menu
  splash/background/clientTheme, mode icons, news images/badges, social/streamer
  blocks, loadout sorting/confirm, stats links, settings links.
- **Audio:** `maxInstances`/coalesce/delay/detune, music channel volume quirk
  (web routes music through `soundVolume`), exact EQ/convolution timbre.
- **Platform/perf:** unload map atlases on map change and preload map audio
  (`resources.ts:137-246`), device/tablet/perf tiers, 33 Hz input policy,
  keyboard/gamepad bind editor, C++ error-report call sites + NDK signal handler.
- **Release:** real upload keystore/Play upload, physical mid-range 60 fps soak,
  final UI polish from device testing.

### 3.5 Intentional platform differences (not TODO)

- Device login instead of Google/Discord OAuth (`AccountManager`); no web account
  linking. If store users need web-account linking, that is a product decision
  (server OAuth + deep link), not a missing port.
- `highResTex` is replaced by the EGL surface GFX tiers (`RenderScale`); atlases
  stay low-res. This is an accepted native adaptation.
- `proxy.ts`, `sdk/*` (ads/SpellSync), `debug/*` tools/editors, `stats/*` pages,
  desktop fullscreen/history/hash behaviour and iOS/iPhoneX layout are out of
  scope.
- Touch-only controls (no local rotation on mobile, on-screen pads instead of
  mouse) are intended; BT keyboard/mouse support is still tracked in P1.6.

### 3.6 Suggested session order (each ends with a green `surv_tests` + device check)

1. **P0.1 + P0.6** — action inputs + real loadout: makes it a playable BR.
2. **P0.4 + P0.5** — death/stats/spectate + match-over latch.
3. **P0.2 + P0.3** — active layer + interpolation: fixes bunkers and game feel.
4. **P0.7 + P1.5** — settings/audio correctness (restart-verified).
5. **P1.1 + P1.2** — authenticated quick play/captcha + correct errors.
6. **P1.3 + P1.4** — render feedback + live inventory HUD.
7. **P1.6–P1.8** — input/metadata/team/account parity.
8. **P2** — polish, then a physical-device soak and store upload.

### 3.7 Acceptance for "1:1 playable" (next milestone gate)

- Loot, reload, heal/boost, revive, equip/switch/throw/drop, scopes and weapon
  slots all work with on-screen buttons and server feedback.
- Bunker/cellar interiors are visible and the layer/mask/ground fade is correct.
- Player, camera and projectiles are interpolated at 33 Hz net sync.
- Death shows per-player stats and spectate; a team win shows VICTORY; a normal
  match end never shows a disconnect error.
- Loadout and all settings persist/apply after an app restart.
- Quick play is attributed to the account; captcha deployments work for logged-in
  players; disconnect reasons are localized.
- The live HUD includes inventory v2, interaction prompt, messages and spectator
  state; the major visual/audio effects from P1.3/P1.5 are present.

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

# store release (arm64; upload keystore + bundletool)
.\tools\build-native.ps1 -Abis "arm64-v8a"
.\tools\build-apk.ps1 -Release -Abis "arm64-v8a" -ApiUrl "https://survev.io" -KeystorePath upload.jks
.\tools\build-aab.ps1 -Abis "arm64-v8a" -BundletoolJar C:\tools\bundletool-all.jar
```

Regenerate after touching `shared/defs` or web sources:
```powershell
node --experimental-transform-types tools/codegen_defs.mjs src/net/generatedDefs.inc
node --experimental-transform-types tools/codegen_render_defs.mjs
node --experimental-transform-types tools/codegen_sound_defs.mjs
node --experimental-transform-types tools/codegen_pass_defs.mjs
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
- The GFX tiers are an EGL **surface** scale (`RenderScale.apply` ->
  `SurfaceHolder.setFixedSize`), not a design-resolution change: Full (0,0)
  clears it back to the view size, Medium 0.85 / Low 0.75 / Potato 0.60 are the
  cycle. It must run on the UI thread and needs the view to be measured (it
  retries 10x50 ms). Startup applies the persisted scale **even at Full** so a
  stale fixed surface from a previous launch is cleared.
- `SurvBridge.setConfigValue` keeps the JSON type (number/bool/string) and saves;
  Java settings screens depend on it and the C++ config reads the keys with
  getFloat/getBool.
- `[perf]` fps/frame/soak logs are gated on `debug.surv_debug=1`; applying a
  tier logs one line (`RenderScale` + `[perf] render scale`) even without the
  switch, which is how the persisted tier is verified. The 60 s soak summary
  uses the 5 s fps samples (not per-frame `secondsPerFrame` spikes).
- The HUD is fitted/centred in `RenderView::getSafeAreaRect()`, not the viewport:
  keep that in `GameScene::layoutHud` (build + every resize) so notches and
  system bars never cover HUD widgets. The world stays full-bleed and the touch
  pads keep screen-space offsets; do not "fix" those into the safe rect.
- The native account is a **device credential, not OAuth**: `AccountManager`
  persists `deviceId`/`sessionToken`/`signedOut`/`playerNameAccount` in the
  `survev_account` SharedPreferences, *not* `SurvBridge` config (its float/bool
  coercion truncates an all-digit ANDROID_ID). A reinstall must keep resolving
  to the same user via ANDROID_ID (random UUID is only the fallback).
- `authMiddleware` accepts `Authorization: Bearer <session>` **in addition to**
  the web's httpOnly `session` cookie; do not drop either path.
  `POST /api/auth/device` is rate limited by `AuthRouter` (5/min/IP) like the
  other auth routes, and returns the token in the body because native has no
  cookie jar.
- The first account login prompts for a name (`usernameSet == false`) and the
  profile screen closes itself after it is set (`mProfileOpenedForName`); the
  name prompt must never fire while in a match. The menu field only follows the
  account name while `playerNameAccount` matches it — a hand-typed
  `playerName` is user-owned and must not be overwritten.
- Log Out must latch `signedOut` and skip the automatic device login; delete
  account starts a **fresh** device account (the device id outlives the user).
  Both behaviours mirror the web reload + login menu without an OAuth flow.
- `GeneratedPassData.java` is generated from `shared/defs` by
  `tools/codegen_pass_defs.mjs`; never hand-edit it. Regenerate after any change
  to `passDefs.ts`/`questDefs.ts` (XP curve, unlock table, quest XP/timed).
  Display names are l10n keys (`pass_*`, `quest_*`, `game-<item>`), not defs.
- Android has **no C++ MenuScene**, so `UiBridge::l10nJson()` owns a standalone
  `Localization` (English + `l10n/<language>.json` via `Files::readText`). Do not
  revert it to `_menu ? ... : "{}"`: every Java label would silently fall back
  to English keys. `setLanguage` updates that table before notifying Java.
- Pass XP/level come from `/api/user/get_pass`; `tryRefreshQuests` must be true
  when opening the screen so the server rerolls expired slots. Reroll
  eligibility mirrors the web: `(!rerolled && !complete) || expired`; the
  server remains authoritative (`refresh_quest` can still return false).
- `TeamSocket` is hand-written (no WS library): keep the writer queue
  (`mOutgoing` + writer thread) — writing from the UI thread throws
  `NetworkOnMainThreadException` — and keep the frame buffer/fragment/ping
  handling. The server sends unmasked frames; client frames must be masked.
- `/team_v2` is on the **API** port (`SurvApi.teamWsUrl()`), not the game port.
  Join/state `roomUrl` is `#CODE` in state but the `join` message uses the code
  **without** `#` (server rooms are keyed by code).
- Invite links are web links (`https://<config webBaseUrl>/#CODE`), not the old
  hardcoded download text; deep links are `survev://team/<code>`. Keep
  `TeamLinks.extractCode` tolerant (bare code, `#`, web link, app link, `?code=`).
- `server/src/teamMenu.ts` must accept the device session as a Bearer token;
  the native team socket has no cookie jar. Keep both auth paths.
- The team socket must survive a team match: `TeamFragment.hideForGame()` keeps
  it, `JavaUiHost.returnToMenu` re-shows the room and `onGameComplete()` sends
  the `gameComplete` message. Do not route that path through `showMenu()` (it
  closes the socket and dissolves the room).
- The Java team screen gets site_info from `MenuFragment.siteInfoJson()`:
  `SurvBridge.getSiteInfoJson()` is empty on Android (no C++ MenuScene). Keep the
  default Solo/Duo/Squad fallback for deep links that arrive before the fetch.
- **Store builds go through `tools/build-apk.ps1 -Release`** (stripped native
  libs, upload keystore, version from `package.json`, v2/v3 signing, signer
  printed) and `tools/build-aab.ps1` (bundletool module from that APK). Do not
  hand-package or reuse the debug signing key for a real upload; keep the debug
  output/signing path unchanged.
- `surv_apiUrl` (manifest meta-data) is the **single API origin**: Java reads it
  in `SurvApi.initialize` (HTTP + `/team_v2`) and C++ uses it as the
  `surv_apiUrl` override. Release uses `-ApiUrl` (default
  `https://survev.io`); the manifest default is `http://127.0.0.1:8000` for
  `adb reverse`. Never hardcode the store origin in `SurvApi`/`DevConfig` only.
- Version code = `major*10000 + minor*100 + patch` from the repo `package.json`
  (0.4.3 -> 403); bump it (or `-VersionCode`) for every Play upload. The debug
  build uses the same values but a different output name.
- `build-aab.ps1` must keep the module zip entry names forward-slashed
  (`manifest/`, `resources.pb`, `res/`, `dex/`, `lib/`, `assets/`) — .NET's
  `CreateFromDirectory` writes backslashes on Windows and bundletool rejects it.
  Proto resources come from `aapt2 link --proto-format`; the rest is copied from
  the release APK.
