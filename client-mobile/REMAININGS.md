# client-mobile — what is left to implement (1:1 web → axmol port)

Consolidated status of the native Android port of `client/` (`client-mobile/`).
This file is a **summary/index**; the detailed work orders live in:

- `plan.md` — module map, milestones M0–M9, decisions D1–D5.
- `M4_REMAINING.md` — rendering (sessions 1–6), verified object-render parity.
- `M6_AUDIO.md` — audio port notes.
- `ARCHITECTURE.md` — UI architecture: gameplay = axmol + FairyGUI, every
  other screen = Android Java, one JNI bridge (`SurvBridge`). Read §5 (move
  matrix) and §7 (migration steps S1–S7) before touching UI code.

Latest recorded state: **2026-10-27 (P1.8 + P2 passes)** — team matches
return to their room, room props persist, in-room rename and invite/join links
work, and the pass reloads the profile for new items. The P2 pass adds
real additive blending, the V minimap toggle, shell casings, newest/item-source/
NEW-badge loadout polish, plus the P1.4–P1.7 HUD/menu/input/audio set. The
second P2 pass adds eased player body radius, `maxVisualRadius` 3.75, def-driven
bush rustle/particles, button useImg/offImg + useParticle, once-only obstacle
destroy debris, and ceiling destroy particles. The third adds per-particle
HSV color variance and the map's river-shore footstep surface. The fourth adds
shot/reload casing parity (shell offset/scale/forward/reverse and revolver
reload casings). The fifth adds live-HUD gear level badges/tap-to-drop, squad
group-colour dots + downed pulse, the cyan group-colour fix, and in-place
Settings re-localization. The sixth adds role promoted/dead kill-feed lines.
The seventh adds the gas/danger HUD icon with its moving-gas pulse. The eighth
adds the clientTheme menu splash background. The ninth adds the rasterized
quick-play mode icons. The tenth adds the news GUI icon and unread badge. The twelfth adds the
post-match teammate stats list. The thirteenth adds the live in-game
leaderboard bridge. The fourteenth adds the perk-mode class picker (role art,
perk list, border colour, 20 s confirm/countdown, `PerkModeRoleSelectMsg` +
saved preference). The fifteenth adds map-atlas unloading on map change. The
sixteenth adds the menu character preview (outfit/melee layers, live loadout
polling, idle bob + right-hand swing). The seventeenth adds NDK crash capture and
native asset-error reports. The twenty-second adds the keybind remap editor and
the web-compatible bind code. The twenty-third adds the pass reward art and
level-up pulse plus the profile stats link. The twenty-fourth completes the
crosshair editor (hex colour + web-range size/stroke sliders, seven stroke
buckets). The twenty-fifth fixes the health-bar colour ramp and adds the
low-health danger pulse. **219 host tests pass, 0 fail.**

---

## 1. Milestone scorecard

| # | Milestone | Status | Notes |
|---|---|---|---|
| M0 | Toolchain + axmol builds | **done** | axmol vendored, Gradle-free APK pipeline (`tools/build-apk.ps1`). |
| M1 | Asset pipeline | **done** | atlases, GUI PNGs (4x), Roboto Condensed TTF, l10n. |
| M2 | Shared core ports (bit-exact) | **done** | `BitBuffer`, `Vec2/MathUtil/Collider`, message classes, TS-generated fixtures. |
| M3 | Net + headless sim | **done (host)** | join/update/kill/gameover/pickup/map dispatch, `snapshotText()`; multi-URL join fallback. |
| M4 | Rendering | **done (major FX)** | terrain/objects/buildings/particles/decals/gas/skeletal player; active layer + interpolation; bullet tracers/impacts, explosions, projectiles, loot rings, player patches/nametags and airstrike zones. Remaining exact-normal/colour nuances are P2. |
| M5 | Input + touch | **done (touch + hardware)** | move/aim/fire + reload/interact/equip/use/scope/drop HUD controls; P1.6 keyboard/mouse defaults + change-gated sends; keybind remap editor + web-compatible bind code (P2 pass 22). |
| M6 | Audio | **mostly done** | engine/mixer/ambience/reverb/EQ/interior sounds, channel semantics, startup volume/mute, menu music and the real sound keys; notification/team-join cues are web-focus-only and exact EQ timbre remains P2. |
| M7 | UI layer | **done** | Java menu/account/pass/team/loadout live; menu chrome is site_info-driven and localized (P1.7), settings language switch re-renders, death/result + spectate overlay live in the C++ HUD, live HUD has the full P1.4 set incl. the perk-mode class picker and live leaderboard; remaining menu art is P2. |
| M8 | Accounts / API | **done (intentional deltas)** | device login + profile, quests/pass, team links, room props/rename/deep links, authenticated quick play, correct disconnect reasons, pass-new-items profile refresh; captcha is unsupported for signed-out users (server skips it for the device session), OAuth is an intentional platform delta. |
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
| `client/src/renderer.ts` → `render/Renderer.*` | ✅ transform/z-sort/mask ported; `GameWorld::update` calls `setActiveLayer`/`setUnderground` before `m_update` |
| `client/src/map.ts` → `game/Map.*` | ✅ terrain, surfaces, pools, camera emitter, baked minimap texture, big map (`game/MapView`); river-shore surface comes from `mapDef.biome.sound.riverShore` |
| `client/src/gas.ts` → `game/Gas.*` | ✅ |
| `client/src/game.ts` (main loop, net dispatch) → `game/Game.*`, `game/GameWorld.*` | ✅ dispatch/net, match-over latch + death/spectate, player/camera/projectile interpolation (P0.3), keyboard/mouse input + change-gated InputMsg (P1.6), perk-mode role selection (PerkModeRoleSelectMsg + config `perkModeRole`); local rotation remains desktop-only (P2) |
| `client/src/objects/objectPool.ts` → `game/objects/GameObject.h` | ✅ |
| `client/src/objects/player.ts` + `animData.ts` → `objects/PlayerRender.cpp` + `Barns.*` | ✅ poses/animations faithful; faction patch, last-stand/bleed particles, same-group nametags, real action-sound keys, eased `m_bodyRad` and `maxVisualRadius` 3.75; minor easing nuances P2 |
| `client/src/objects/bullet.ts` → `objects/Barns.*` (BulletBarn) | ✅ travelling tracer sprite, per-def palette/width/length, suppression, collision FX + hit groups, blood/pan sparks, stairs layer |
| `client/src/objects/explosion.ts` → `objects/Barns.*` (ExplosionBarn) + `GameWorld.cpp` | ✅ generated burst/scatter/ripple/shake/lifetime/indoor suppression; physics particles |
| `client/src/objects/shot.ts` → `objects/Barns.*` (ShotBarn) | ✅ fire sound, shot casings (`createCasingParticle`: shell offset/scale/forward/reverse) and reload-timing casings (`caseTiming == "reload"`, maxReload shells); last/alt/team variants and cycle/pull timing remain P2 |
| `client/src/objects/obstacle.ts` → `objects/Barns.*` | ✅ door/button/destroy sounds, button useParticle + useImg/offImg swap, once-only destroy debris (def explodeParticle, 2-item pick) |
| `client/src/objects/building.ts` → `objects/Barns.*` | 🔶 ceiling reveal/sounds/residue + destroy particles; puzzle FX remain partial (P2) |
| `client/src/objects/loot.ts` → `objects/Barns.*` | ✅ rarity border/outer ring, inner-art scale/rotation/mirror, tint |
| `client/src/objects/decal.ts` → `objects/Barns.*` | ✅ |
| `client/src/objects/deadBody.ts` → `objects/Barns.*` | ✅ (interpolation P0.3) |
| `client/src/objects/projectile.ts` → `objects/Barns.*` | ✅ spin/drag, throwable trail, water alpha, stairs z-order, always-on-top bomb; collision bounce sounds remain P2 |
| `client/src/objects/smoke.ts` → `objects/Barns.*` | ✅ |
| `client/src/objects/airdrop.ts` → `objects/Barns.*` | ✅ retained fall-loop handle, chute/crash FX, ripples |
| `client/src/objects/plane.ts` → `objects/Barns.*` | ✅ plane engine loop and airstrike zones instantiated |
| `client/src/objects/flare.ts` → `objects/Barns.*` | 🔶 additive blend is a no-op (P2) |
| `client/src/objects/structure.ts` → `objects/Structure.*` | ✅ bounds/masks/stairs/map icons + `interiorSound` ambience tracks |
| `client/src/objects/particles.ts` → `objects/Barns.*` (ParticleBarn + Emitter) | ✅ near line-for-line; per-particle HSV color variance from the TS colour closures |
| `client/src/audioManager.ts` → `audio/AudioManager.*` | ✅ startup volumes/mute, master folded into live type volumes, maxInstances/coalesce/delay/detune, ambient retention and stopAll semantics; Java UI cues reach the mixer through `UiBridge::playSound` (`SurvBridge.playSound`) |
| `client/src/soundDefs.ts` → `audio/GeneratedSoundDefs.cpp` (codegen) | ✅ generated |
| `client/src/ambiance.ts` → `audio/Ambiance.*` | ✅ in-game tracks + menu music (GameScene ticks it while the Java menu is up), retained across match end, completed-track restart |
| `client/lib/createJS.ts` → `audio/AxmolAudioBackend.h` | ✅ channel semantics enforced in AudioManager; per-source pitch (detune), mute-safe new loops, EAX reverb |
| `client/src/ui/touch.ts` → `ui/Touch.h`, `game/AimLine.h` | ✅ pads/aim-line good; config move/aim style + aim line applied live and persisted; gameplay actions from `game/InputActions` + `game/Hud` |
| `client/src/config.ts` → `ui/Config.*`, `ui/ConfigStorage.cpp` | ✅ schema + settings screens write real values; runtime listeners applied at startup/on change (highResTex maps to the GFX tier, local rotation is desktop-only) |
| `client/src/device.ts` → `ui/Device.*` | 🔶 simplified (no tablet/pixelRatio/MAX_TEXTURE_SIZE tiers) |
| `client/src/localization.ts` → `ui/Localization.*`, `ui/L10nJson.*`, Java `L10n.java` | ✅ C++ table good; Java menu/settings/pause/help labels read the flattened table (session P1.7), settings language list is `ui::kLocales` (18 codes/names) via the bridge; remaining loadout/item chrome strings are P2 |
| `client/src/main.ts` (app bootstrap) → `app/AppDelegate.*`, `app/GameScene.*`, `app/MenuScene.*` | ✅ join/reconnect, quickplay auth/captcha message, reason-string mapping and error dialogs (match-over latch done) |
| `client/src/api.ts`/`siteInfo.ts` → `net/Api.*`, `net/SiteInfo.*`, Java `SurvApi.java` | ✅ Bearer find_game + protocol version; Java menu builds modes/regions from site_info (enabled/teamMode/l10n) and requires the fetch before Play (P1.7); mode/region icons remain P2 |
| `client/src/ui/teamMenu.ts` → Java `TeamFragment.java` + `TeamSocket.java` + `TeamLinks.java` | ✅ real `/team_v2` protocol, invite links (copy/share/paste, config `webBaseUrl`), `survev://` + `https://…/#CODE` + `?u=&jt=` links, kick/mode/region/auto-fill, in-room rename, room-prop persistence, team match returns to the room |
| `client/src/ui/loadoutMenu.ts` → Java `LoadoutFragment.java` | 🔶 real owned items from `shared/defs` (`getLoadoutItemsJson`) with the web's image tiles, 6 emote slots, crosshair type picker + preview, new-item confirm/ack; source badges and the per-item lore modal remain P2 |
| `client/src/ui/ui.ts` (HUD) → `game/Hud.*` (pix adapter) | ✅ health/boost, weapon/ammo, alive/kills, kill feed (incl. role lines), kill leader, gas timer + gas/danger icon, death/result + spectate overlay, post-match teammate stats, inventory v2 strip, interaction prompt/touch target, toasts, squad group-colour dots + downed pulse, and the perk-mode class picker are live |
| `client/src/ui/touch.ts` (pad gfx) → `ui/PadGraphicsAxmol.*` | ✅ real ring + touch dot at the web's scale |
| `client/src/emote.ts` (pings + emote wheel + indicators) | ✅ world emotes (def textures) + ping indicators + minimap ping sprites + emote wheel UI/input/throttle + team-ping wheel (pings, medical/ammo emotes) |
| `client/src/objects/mapIndicator.ts` / `mapSprite.ts` | ✅ indicators: `MapIndicatorBarn` (data) + `Hud::updateMapIndicators` and `MapView::setIndicators` draw the def sprite/tint/pulse ring at the world position on both map views (equipped markers sort over the dots); `mapSprite.ts` itself is unused by the live HUD (dots/pings/indicators are `MapView` sprites) |
| `client/src/input.ts`/`inputBinds.ts` (mouse/keyboard/binds) | `game/InputBinds.h` + `game/HardwareInput.h` + `game/InputSync.h` | ✅ full `BindDefs` table (36 `Input` actions), web-compatible `binds` code (LSB-first stream + CRC-16) and the Java Key Binds screen (capture/clear/restore/share/import); keys, mouse buttons/wheel and the unbound-arrow fallback resolve through the table. Android gamepads (native-only) map sticks/triggers/buttons onto the same state with fixed actions. |
| `client/src/ui/opponentDisplay.ts` (menu character preview / opponent display) | 🔶 animated preview: `CharacterPreviewView` draws the outfit's body/hands and the selected melee on the animData.ts skeleton (`GeneratedPreviewAnims`: idle pose + melee deploy/idle/attack keyframes), framed to fit the art, and plays the outfit pickup + melee anim sounds through `SurvBridge.playSound`; heal/boost particle emitters are N/A while the preview has no heal/boost tab |
| `client/src/resources.ts` (per-map load/unload, audio preload, MAX_TEXTURE_SIZE) | 🔶 map atlases load/unload on map change like `loadMapAssets`; audio is preloaded globally at startup (no per-map preload needed), `MAX_TEXTURE_SIZE` tiers remain P2 |
| Player action sounds (footsteps, door, bush, weapon deploy/switch, heal, pickup, dry fire, haste) | ✅ keyed from the defs (`sound.reload/reloadAlt/deploy/use/pickup`); doors/buttons/puzzles/ceiling/plane loops and victory music wired; dry fire is approximate (P2) |
| UI sounds (ambient lab, pickup pings, notifications, victory) | 🔶 victory music + pickup/door/puzzle/plane cues wired; notification/team-join sounds are web focus-only (not applicable to the Android foreground) |
| Reverb (`setReverbProperties`) | ✅ `ReverbModel` maps cathedral/cave to EAX props; applied to every active handle (web convolver is global) |
| `client/src/ui/ui2.ts` (inventory v2: weapon cards, perk icons, throwable arc) | ✅ live `game/Hud` has reserve ammo, bag strip, scopes, interaction prompt, pickup/kill messages and gear level badges + tap-to-drop (P1.4/P2); the dead `ui/FguiHud` is not built |
| `client/src/emote.ts` (emote/ping system) | ✅ see the emote row above |
| `client/src/ui/pieTimer.ts` (action progress circle) | ✅ arc/countdown + `RELOADING`/`USING`/`REVIVING` labels from the active player action (P1.4) |
| `client/src/ui/profileUi.ts` / `pass.ts` (account/profile/battle pass) | ✅ `ProfileFragment` + `PassFragment` (real level/XP, quest reroll); the pass screen shows the next unlock's art (outfit tint), category icon/title and the level-up pulse, and the profile `/stats/?slug=` link opens the web stats page |
| `client/src/account.ts` | ✅ Java `AccountManager` + `SurvApi` device login/session/profile/username/logout/reset/delete/loadout push, item-status ack/confirm and pass `newItems` profile reload; OAuth/linking intentionally N/A |
| `client/src/crosshair.ts` (custom crosshair) | ✅ sprite pipeline good; the editor has the 30-type picker, 7 colour swatches + the web's hex input, and size (0.25..1.0) / stroke (0..1.5) sliders; the sprite bakes the stroke in 0.25 steps (7 buckets) |
| `client/src/pingTest.ts` (pre-game latency check) | ✅ `net/PingTest` (echo cadence/retry/complete + `bestRegion`/zones, host-tested); the probe list comes from `net/Regions` (`tools/codegen_regions.mjs`, the native `PING_TEST_URLS`), and the Java menu shows the ms and auto-selects the closest region |
| `client/src/errorLogs.ts` (client error reporting) | ✅ `net/ErrorLog` (sample/budget/throttle host-tested), Java uncaught-exception reporting in `AppActivity`, NDK signal capture (`NativeCrash.cpp` writes a PC backtrace reported on the next launch) and C++ `mapDefMissing`/`atlasMissing` reports; `debug.surv_debug=1` sends a startup probe, `debug.surv_crash=1` exercises the native handler |
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

- [x] **P0.1 Send the gameplay action inputs and add their HUD buttons.**
      `InputMsg.addInput`/`useItem` exist (`net/Messages.h:118-132`) and
      `Input_Interact/Reload/Use/UseBandage/...`/`Action_UseItem` are in
      `core/GameConfig.h:69-101`, but **nothing called them**:
      `TouchInput::build` only filled move/aim/shoot (`src/game/TouchInput.h:44-70`)
      and `GameScene::updateInput` sent that (`src/app/GameScene.cpp:1489-1514`).
      **Done** (port of `game.ts:565-656`/`ui.ts:388-399`/`ui2.ts:266-331`):
      - `game/InputActions.h` queues one-shot actions and drains them into the
        next `InputMsg` (a tap between the 33 Hz sends is not lost); the
        `InputMsg` 7-entry cap/dedupe matches the web.
      - `game/Hud` now draws a RELOAD pill, a USE pill (Interact+Cancel, like the
        web touch listener), quick-use HEAL/BOOST pills (best available
        bandage/medkit and soda/pills from `ActivePlayerData.inventory`), a
        SCOPE pill (next scope), a SWAP pill, and highlights the equipped weapon
        card.
      - Tap a weapon card to equip slots 1–4 (throwable taps twice to throw);
        long-press a card/pill to `DropItemMsg`; dropping gun slot 1 onto 2 (or
        vice versa) sends `SwapWeapSlots`, matching the web desktop drag.
      - `GameScene::onTouchBegan` routes the hit test before the joystick pads.
        A weapon-card touch that turns into a drag is handed to `Touch`, so the
        cards can overlap the bottom-right aim pad without breaking aiming.
      - Not on touch yet: next/prev/last-weapon buttons and stow (tap the melee
        slot to stow); these stay with the keyboard/bind work in P1.6. Auto
        reload is server-side (no client input).
      - Acceptance is device-checked in the next pass: loot, reload, heal,
        revive, switch/throw/drop weapons and scopes reach the server.

- [x] **P0.2 Wire `Renderer::setActiveLayer` (underground/bunker rendering).**
      **Done.** `GameWorld::update` now applies the active player's `layer` and
      `Map::isUnderground(pos, layer, rad)` to the renderer (and the audio
      manager) **before** `renderer.m_update`, matching game.ts
      `m_processGameUpdate` (`game.ts:1162-1167`). This makes `layers[1]` fade in,
      the ground rect fade in/out, the stairs `layerMask` stencil release, and
      the per-object `activeLayer` checks (`obstacle.render`) see the same frame.
      Host tests cover the layer/`layerAlpha`/`groundAlpha`/`underground` wiring
      and the camera following the interpolated position.
      - Acceptance still needs a device pass: enter/exit a bunker and confirm the
        interior objects, player, ground fade and tree/building mask. A
        quick-play run on the x86_64 emulator joined a match and rendered
        normally with no crash.

- [x] **P0.3 Interpolate entities and the camera.**
      **Done (web-matching subset).** Ported `player.ts:799-821` and
      `game.ts:426`:
      - `Player` now carries `visualPos`/`visualPosOld`/`posInterpTicker` and
        `visualDir`/`visualDirOld`/`dirInterpolationTicker`, seeded in
        `m_updateData` exactly like the web (a duplicate update re-seeds from the
        current net position). The container, aura container and body rotation
        use the visual values.
      - `Projectile` carries `visualPosOld`/`posInterpTicker`; its barn render
        pass lerps `pos` over `camera.m_interpInterval`.
      - `GameWorld::update` sets the camera to the active player's `visualPos`
        (zoom/shake unchanged).
      - `interpolation` is applied from config: `GameScene::setInterpolationEnabled`
        (kept across per-match world rebuilds) + `UiBridge` startup/`setConfigValue`
        hook. The live Java toggle is still P0.7.
      - Bullets, planes, airdrops and dead bodies do **not** interpolate on the
        web either (each advances by its own per-frame ticker / is static), so
        the port leaves them on the same timing.
      - Acceptance: motion is smooth at 33 Hz net sync; `interpolation=false`
        snaps (host-tested). Device: a quick-play match ran for 12 s with no
        crash; a bunker entry/motion comparison is still pending.

- [x] **P0.4 Death → stats → spectate.**
      **Done.** `Game` now stores `PlayerStatsMsg` (`localStats()`,
      `isLocalDead()`) and forwards `SpectateMsg` Begin/Next/Prev
      (`Game::sendSpectate`, the Java pause bridge and `game/Hud`).
      - The C++ HUD (`showDeathPanel`) shows **YOU DIED** with kills, damage,
        damage taken and survival time as soon as the team-death PlayerStats
        arrives, with **SPECTATE** and **LEAVE GAME**.
      - Once the server moves `activePlayerId` to a teammate/killer,
        `Hud::update` shows the spectating overlay: **SPECTATING** plus the
        target name, `N SPECTATING`, **PREV/NEXT** (team modes), **MATCH STATS**
        toggle and **LEAVE GAME**. The camera follows the active player and the pads/input
        are gated off while dead/spectating (`isLocalDead`, `isSpectating`).
      - The final `GameOverMsg` replaces all of that with the result panel
        (VICTORY/DEFEAT, rank, team kills, local stats) and a **LEAVE GAME**
        button. The Java pause dialog's Spectate button now sends Begin too.
      - Host tests cover the death panel, overlay hit tests, spectate name/count,
        stats toggle and the final-result replacement.
        Device: solo death → result panel → Leave verified; the team-death
        spectate path needs a duo/squad teammate for a full on-device run.

- [x] **P0.5 Latch match-over so a finished match is not a "disconnect".**
      **Done.** A normal end sends `GameOverMsg` and the server closes with
      **3000/0** (`stop()` → `client.disconnect()`); `GameScene::update` now keeps
      the world/result whenever `_game->gameOver() != nullptr` and never runs the
      reconnect/URL-fallback/error path. The liveness watchdog also skips once
      `_hasGameOver` is set. The result screen stays until **LEAVE GAME**.
      Device: a solo match death showed the result panel while the socket closed
      (`Disconnected: code=0 reason=`) instead of "Host closed the connection".

- [x] **P0.6 Loadout editor: real items + all emote slots.**
      `LoadoutFragment.java` used to permanently hide the item grid
      (`mGrid.setVisibility(isCrosshair ? GONE : GONE)`), invented IDs
      (`outfitWoodland/outfitSnow/katana_rusted/emote_gg/heal_advanced`) that the
      validator rejected, and replaced the emote array with one slot. **Done**
      (port of `ui/loadoutMenu.ts`):
      - `codegen_render_defs.mjs` now emits a `LoadoutItemDef` table (type,
        category, name, lore, sprite, rarity) from `shared/defs`; the bridge
        returns the owned subset as `getLoadoutItemsJson()`.
      - The grid is live for every category and uses the real names/rarities/
        lore; crosshair entries form the 30-type picker (`001`/`005`/... labels).
      - Six emote slot buttons (Top/Right/Bottom/Left/Win/Death); selecting an
        item merges that slot and preserves the other five.
      - Crosshair type writes the nested `{"crosshair":{"type":...}}` shape (the
        old flat write was ignored by the validator); color/size/stroke still
        round-trip through the same editor.
      - `AccountManager` feeds `account.items` into the C++ validator and keeps
        its status; new items show a confirm banner (`set_item_status` 1) and
        opening a category acks its items (status 2).
      - Item art is generated from the web SVGs by `tools/build-loadout-icons.mjs`
        (`Content/img/loadout/<type>.png`); the Java grid renders 3×3 image tiles
        with the web's 64px art / 96px tile look, a green selected border and
        outfit tint via the same multiply colour matrix the web SVG filter uses.
        Melee loot rotation/mirroring is baked into the PNG. Category tabs use
        the web's `gui/loadout-*.png` icons, the crosshair preview/tiles show the
        real crosshair art, and emote slots show their emote art.
      - Not done (tracked in P1.7): item source badges and the full per-item lore
        modal. Acceptance: outfit/melee/heal/boost/emotes/crosshair change,
        persist across an app restart, and join with them (verified on the
        x86_64 emulator).

- [x] **P0.7 Make settings real (persist + apply).**
      **Done.** `UiBridge::applyGameSettings` reads the saved config once at
      install and after any handled key changes; `GameScene` keeps the values for
      each rebuilt match world.
      - Volumes/mute: Java uses `Json.peekFloat`/`peekBool` (numbers no longer
        read as strings); `UiBridge::setAudio` persists on the game thread;
        startup applies master/sound/music/mute. The pause dialog's mute mapping
        is fixed (`1` = muted, matching the menu).
      - Interpolation, screen shake, anon names and local rotation are applied
        to the world camera/player barns (`GameScene` + `GameWorld`). `localRotation`
        remains desktop-only (control hidden), `screenShake` gates
        `Camera::m_applyShake` and is now called every frame; `anonPlayerNames`
        hides non-active names in the player barn and in the kill feed/leader/
        spectate labels (`Game::displayName`).
      - Move/aim style + aim line: the settings buttons write
        `touchMoveStyle`/`touchAimStyle`/`touchAimLine`; `Touch` defaults to web
        `anywhere` and `GameScene::setTouchSettings` applies each change live.
      - Volumes are folded into ambience/music updates (`AudioManager::setVolume`
        now includes master) and existing loops are rescaled on a master change.
      - Language list comes from `SurvBridge.getLocalesJson()` (`ui::kLocales`:
        da/de/en/es/fr/it/nl/pl/pt/ru/sv/vn/tr/jp/ko/th/zh-cn/zh-tw) with native
        display names; `Reset` returns to English.
      - Removed the dead controls: highResTex (= the GFX surface tier, P2 asset
        pipeline), local rotation (desktop mouse only) and the invented HUD scale.
      - Acceptance: toggles/volumes/styles persist across reopening the screen
        (device-verified); app-restart persistence is through the same config
        save path.

### 3.3 P1 — missing 1:1 features / correctness (playable but wrong)

- [x] **P1.1 Authenticated quick play + captcha + correct find_game body.**
      **Done.**
      - `SurvApi.findGame` takes the device session token and sends
        `Authorization: Bearer`; `server/src/api/index.ts` `find_game_v2` now
        accepts Bearer as well as the web `session` cookie.
      - `MenuFragment` sends `playerCount: 1`, `version:
        SurvBridge.getProtocolVersion()` (bridge -> `defs::kProtocolVersion`) and
        the selected region's fastest-first zones.
      - `TeamFragment.startGame` sends the same protocol version and
        `JavaUiHost.zonesJson(region)` (from the completed ping test); the Play
        button no longer blocks on captcha when the device account is signed in
        (the server skips captcha for authenticated users).
      - Captcha is intentionally not implemented (no WebView/Play SafetyNet
        integration): a signed-out player on a captcha deployment gets the
        localized `index-invalid-captcha` dialog plus a "sign in" hint. A
        signed-in device session bypasses it server-side.
      - Acceptance: a device-session quick play is attributed to the account
        (needs a dev server with the Bearer change; the stock bundle server
        predates it). Region/zones and version are verified in logcat.

- [x] **P1.2 Correct disconnect reasons and error modals.**
      **Done.** `game/DisconnectReason.h` maps the server's close *reason*
      (`invalid_token`, `invalid_protocol`, `invalid_packet`, `behind_proxy`,
      `player_not_found`, `ip_banned`, `rate_limited`, `server_crashed`,
      `server_restart`, `full`) like `main.ts:839-858`; the old 4001–4011 code
      table is gone. A late `invalid_token` after a rejoin attempt stays
      `index-host-closed`, and a pre-join failure shows
      `index-failed-joining-game`.
      - `JavaUiHost.returnToMenu` shows an `AlertDialog` for
        invalid-protocol/IP-ban/behind-proxy and a status/message for the rest;
        `find_game` errors map through `MenuFragment.handleFindGameError`
        (ban detail dialog with reason/duration, captcha hint, proxy/protocol
        modal). `account_banned` from the profile fetch also shows a dialog.
      - Host test `game_disconnect_reason_mapping` covers every reason.
      - Acceptance: force-reason runs still need a dev server; the mapping and
        dialogs are unit-verified / compile-verified, and the stock bundle's
        normal host-closed path stays localized.

- [x] **P1.3 Rendering feedback parity (biggest visual gap).**
      **Done (minor exact-normal/timbre details stay P2).**
      - Bullet tracers: `BulletVisual` is a real travelling sprite
        (`player-bullet-trail-02.img`) with per-def length/width, tracer palette
        (regular/AP/chambered/saturated), suppressed alpha fade, combat-stim
        particles and revenant-style whiz audio. It now collision-tests obstacles
        and players (`colliderIntersectSegmentRes`), spawns wall dust + the
        def bullet hit group, parented blood/`player_bullet_hit` and
        pan/steelskin sparks, and updates the stairs layer like bullet.ts.
      - Explosions: the shared `explosionEffectDefs.ts` table is generated into
        `ExplosionEffectRenderDef` (burst particle/scale, grass/water sounds,
        detune/volume, scatter count/speed, ripple count, shake/lifetime).
        `ExplosionBarn` spawns bursts/scatter physics particles, water ripples,
        camera shake, indoor `bomb_iron` suppression and lifetime, replacing the
        old orange circle.
      - Projectiles: sprint rotation/drag, throwable trail (length/width/alpha/
        tint), water alpha 0.3, `posZ` scale/order, stairs layer and
        always-on-top bomb_iron behaviour.
      - Loot: generated `lootBorder`/`lootBorderTint`/`lootInnerScale`/`lootRot`/
        `lootMirror`; the outer ring is a child sprite behind the item art.
      - Player: same-group-only nametags (30 px below), faction patch sprite,
        last-stand/adrenaline and bleed particles (heal/haste emitters were
        already wired).
      - Airdrop/plane/airstrike: a retained falling-loop handle (chute once),
        plane engine loop, and `msg.airstrikeZones` now instantiate map zones;
        camera shake is triggered from explosions.
      - Host tests cover the generated explosion table, bullet travel/collision,
        loot border and projectile spin/trail. Side-by-side screenshots remain a
        manual/device check.

- [x] **P1.4 Live inventory v2 / interaction HUD.**
      **Done (perk-mode role picker deferred to P1.7/P2).** The live HUD now
      has reserve ammo (`clip/reserve`), an ammo/med/boost/nade bag strip and a
      live kill counter (`Game::localKills`, not just GameOver); weapon cards,
      scopes, quick-use pills, gas timer, spectator count and death/result
      overlays were already live.
      - Interaction prompt: nearest loot on the active audio layer
        (`PICK UP <TYPE>`) above the health bar; the prompt itself is a touch
        target (tap = Interact + Cancel) like ui.ts `interactionElems`.
      - Toasts: `PickupMsg` failures map to the ui2.ts messages (full bag,
        already owned/equipped, better item, cannot fire, max perks); XP drops
        show RARE LOOT; new perks show `PERK: <TYPE>`; local credited kills show
        `YOU KILLED <name>`.
      - Kill feed: six individually coloured rows (local killer green, local
        target red, otherwise orange) rather than one monochrome text block.
      - Faction modes show `R n   B n`; team arrows sit 300 px from the screen
        centre for living/downed off-screen teammates; role announcements appear
        under the kill leader; `WAITING FOR PLAYERS` is shown until
        `JoinedMsg.started`; the pie timer now gets `RELOADING`/`USING`/
        `REVIVING` labels from the active player action.
      - Perk-mode role picker landed in the fourteenth P2 pass (roles list,
        selection/confirm, 20 s countdown); exact CDN role art beyond the
        rasterized `gui/role-*.png` remains P2.

- [x] **P1.5 Audio correctness and coverage.**
      **Done (web-focus-only cues excepted).**
      - Saved `masterVolume`/`soundVolume`/`musicVolume`/`muteAudio` are applied
        at bridge install (before the first menu track starts).
      - Master volume is part of `AudioManager::setVolume` and is folded into
        ambience/music/interior/emitter updates; changing it rescales live
        voices (CreateJS global gain).
      - Pause/background no longer `stopAll()`; leaving a match uses
        `stopAll(retainAmbient=true)` (createJS default), so music/ambience loops
        survive. `GameScene` ticks `AudioManager` + `Ambiance` every frame (not
        just inside the world), giving the Java menu its menu music, and
        `Ambiance::onGameComplete` fades the music back in.
      - Sound keys now come from the defs: `GunDef.sound.reload/reloadAlt/deploy`,
        `Heal/Boost.sound.use`, `Loot.sound.pickup` (codegen fields in
        `GameObjRenderDef`), door change/open/close/unlock, button on/off,
        obstacle explode, puzzle fail/complete, ceiling destroy, and the plane
        engine loop (position/pan updated at 10 Hz). A win plays the map music
        after 1.3 s (`GameOverMsg`).
      - Channel semantics implemented in `AudioManager`: `maxInstances` (oldest
        voice killed), `canCoalesce` (30 ms equal-power merge), `delay`
        (scheduled in seconds), `detune` (axmol pitch = 2^(cents/1200)) and
        ambient/loop retention. `AxmolAudioBackend` keeps PAUSED voices alive
        and pauses loops started while muted.
      - Not ported because they are web-focus-only: `notification_start_01`
        (tab focus) and `notification_join_01` (team screen only plays it when
        the browser window is unfocused). Exact EQ/convolution timbre stays P2.
      - Acceptance: device run shows menu music at boot, settings volume/mute
        survive the session; a full sound checklist on real hardware is still a
        P2 polish item.

- [x] **P1.6 Keyboard/mouse/binds + input policy.**
      **Done (defaults + the remap editor; see the twenty-second P2 pass).**
      - `game/HardwareInput.h` ports the default `inputBinds.ts` bind set and the
        state model from `input.ts` (key down/pressed edges, mouse buttons/wheel,
        movement and fire queries). Defaults: WASD + arrows move,
        left mouse fire, R reload, F interact, X cancel, 1/2/3/4 equip
        primary/secondary/melee/throwable, wheel up/down previous/next weapon,
        Q last weapon, E stow, T swap gun slots, 7/8/9/0 use bandage/medkit/
        soda/pills, M or G open the large map. Escape keeps its pause/leave path.
      - Mouse aim/fire: cursor offset from the screen centre becomes
        `toMouseDir`/`toMouseLen` through the camera zoom, left click sets
        `shootStart`/`shootHold`; a touch event hands aim back to the pads
        (`noteTouchInput`). Spectating uses Left/Right to cycle targets.
      - `game/InputSync.h` is the web's `game.ts:731-771` send policy: send on a
        changed message, aim direction >0.1 deg, aim length >0.5, any non-empty
        `inputs`, `shootStart` while set, and a 1 s keepalive. Idle upstream
        traffic no longer streams at 33 Hz. The seq/ack handshake is unchanged.
      - The HUD's queued actions (P0.1) and quick-tap `shootStart` latch drain
        through the same send gate, so a tap between sends is never lost.
      - The keybind remap editor + web-compatible code landed in the
        twenty-second P2 pass (§3.4): `game/InputBinds.h` and the Java Key Binds
        screen. The defaults are still the web defaults on a fresh profile.
        Android gamepads landed in the twenty-first P2 pass (see §3.4).
      - Android BT keyboards: axmol's `AxmolGLSurfaceView` only forwards
        DPAD/BACK to the engine, so `AppActivity` sends gameplay letters/digits
        through `SurvBridge.hardwareKey`; `UiBridge` marshals them onto the GL
        thread and `GameScene::onHardwareKey` maps Android `AKEYCODE_*` values
        into `HardwareInput`. Desktop builds use the axmol key listener directly.
      - Acceptance: host tests cover bind edges/order, wheel/use-item mapping,
        mouse state/touch handoff, the angle/length/action/keepalive gates; the
        x86_64 native and debug APK builds pass. Device check on the x86_64
        emulator: `adb shell input keyevent KEYCODE_R` / `KEYCODE_1` produced
        InputMsg actions 5 (Reload) / 11 (EquipPrimary) in logcat. Emulator mouse
        is delivered as touch by Android, so the touch path covers it.

- [x] **P1.7 Menus / metadata / localization polish.**
      **Done (core menu metadata + localization; the residual art polish moved to
      P2).**
      - Quick play is driven by `site_info.modes`: `MenuFragment` builds the
        buttons from `mapName`/`teamMode`/`enabled`, labels them with
        `index-play-<buttonText>` (`index-play-solo/duo/squad/50v50`), sends the
        site_info index as `gameModeIdx`, hides disabled modes, and only shows
        Join/Create Team when an enabled mode has `teamMode > 1`.
      - Play is unavailable until the fetch succeeds: an indeterminate spinner
        runs, failures show a tappable retry status, and play buttons dim/disable
        while `find_game` is in flight (the web spinner/lockout).
      - Regions use each pop's `l10n` key (e.g. `index-local`) and the localized
        `index-players`, with the persisted `regionSelected` choice preserved.
      - `L10n.java` caches the C++ flattened table and reloads it whenever
        `SurvBridge.getLanguage()` changes; `UiBridge::l10nJson()` also re-syncs
        its standalone table with the saved config language. The main menu
        re-renders immediately when Settings closes, and Settings/Help/Pause
        labels are localized when each screen is created (verified en/es incl.
        cold start) and the Settings modal now rebuilds in place on a language
        change/Reset (session P2 pass 5).
      - Help content now documents the real touch HUD and the default
        keyboard/mouse binds instead of sprint/pinch/building features the port
        does not have.
      - News shows the entry date, title and up to two paragraph summaries from
        `NewsJson`; attributions/HOF/changelog and the Leaderboards entry open
        the corresponding web pages (the in-game `LeaderboardModal` still needs a
        live-stats bridge, tracked in P2).
      - `MenuFragment` uses `SurvBridge.getProtocolVersion()` for find_game (the
        old hard-coded `1028` was already gone).
      - Acceptance: x86_64 debug APK on the emulator shows site_info-driven
        Solo/Duo/Squad, localized region/player counts and Spanish menu strings
        after changing the language; attributions/HOF/changelog open the site.

- [x] **P1.8 Team/account refinements.**
      **Done.**
      - Quitting a team match no longer drops to the main menu: `quitToMenu`
        leaves `mInTeam` intact, the C++ `returnToMenu` reaches
        `TeamFragment.onGameComplete()`, and the room re-appears with its
        socket alive (device-verified: Quit Game from a team match returned to the
        same invite code).
      - Room props persist on leave: `TeamFragment.hide()` writes
        `gameModeIdx`, `teamAutoFill` and (leader only) `region` through the
        config bridge, mirroring `teamMenu.leaveCb`; a new room now starts from
        the saved mode/auto-fill/region (device-verified: Mode Squad + Auto-fill
        OFF survived an app restart).
      - `changeName` is wired into the room: a Change-name button opens an edit
        dialog, validates 1-16 chars, sends `{"type":"changeName"}`, updates the
        roster and persists `playerName` (device-verified; the renamed player
        survived a restart).
      - Invite links use the config `webBaseUrl` (default `https://survev.io`)
        through `TeamLinks`, and the Settings web links do too.
      - Deep links: `AppActivity` now handles `?u=<base64 urls>&jt=<token>`
        (base64 URL-safe fallback, comma-split URLs, direct join), `survev://team/CODE`
        and `https://<web>/#CODE` (fragment or `TeamLinks.extractCode`). The
        manifest adds a BROWSABLE https filter for `survev.io`.
        Device-verified: `https://survev.io/#ABCD` opened the join screen, and a
        real `?u=&jt=` link joined a match.
      - Account/pass: `getPass` now reloads the profile when
        `pass.newItems` is set (account.ts:284), so freshly unlocked items reach
        the loadout/new-item badges; the existing `set_item_status` ack/confirm
        flow and loadout badge remain in place.
      - Acceptance: team quit/room persistence, rename, invite/join links and
        the pass-new-items profile refresh are code-complete and device-checked;
        a full two-device team match and a pass-unlock grant still need a live
        account/device pass.

### 3.4 P2 — polish / smaller parity gaps

Done in the first P2 pass (2026-10-26):
- Additive blend now reaches axmol (`BlendProtocol`: flares, projectile trails and
  particles use `BlendFunc::ADDITIVE` instead of normal alpha blending).
- `CycleUIMode` (V) hides/shows the minimap (frame, map, expand affordance)
  without opening the big map, matching `ui.ts:1900-1912`.
- Loadout "Newest" sorts by account `timeAcquired`; item tiles show a gold NEW
  badge for status==0 items, and the detail panel shows `Acquired: <source>`
  (source/timeAcquired parsed from `account.items`).
- Shell casings: shot.ts casing ejection is ported (ammo/custom casing particle,
  per-shot velocity/rotation, reload-timing guns excluded) and covered by the
  shot host test.

Done in the second P2 pass (2026-10-26):
- Player body radius now eases (`m_bodyRad`, `dt * 6`, snap on `isNew`) and the
  body/gun/mag/melee scales follow it; `maxVisualRadius` is the web's 3.75
  (the old 1.0 hardcode was used for the stairs sort).
- Bush enter/exit FX is def-driven: `isBush`, `sound.enter` and `hitParticle`
  (3–4 particles) instead of `isTree`/`bush_enter_01`/`leaf`.
- Obstacles swap `button.useImg`/`offImg` sprites with button state, spawn
  `button.useParticle` on a seq change, and emit destroy debris (def
  `explodeParticle`, string or 2-item random pick) exactly once — the old code
  spawned it every render frame and only for explodable defs.
- Building ceilings spawn `ceiling.destroy.particle` × `particleCount` inside
  the first surface collider; the destroy sound now comes from
  `ceiling.destroy.sound` (the old codegen wrote the whole `sound` object).
- Partial obstacle updates (pos/ori/scale) no longer clear full-update state
  (type/button/door/health/dead): the def is resolved from the persisted type
  and the sprite swap/interpolation survive delta ticks.
- Host tests: `player_body_radius_eases`,
  `obstacle_button_swap_and_destroy_fx`,
  `building_ceiling_destroy_particles`.

Done in the third P2 pass (2026-10-26):
- Particle colors: codegen samples each `particles.ts` colour closure at the
  `util.random` endpoints and emits HSV ranges (`colorVariance`); each spawned
  particle picks a fresh hue/sat/value (blood splats, leaves, wood/glass chips,
  explosion bursts, ...). Plain numeric colours stay exact.
- River-shore footstep surface comes from `mapDef.biome.sound.riverShore`:
  `stone` on `main_spring`/`woods_spring`, `sand` elsewhere (was always sand).
- Host tests: `hsv_to_rgb_matches_web`, `particle_color_variance`,
  `river_shore_surface_from_map_def`.

Done in the fourth P2 pass (2026-10-26):
- `shot.ts createCasingParticle` is now the single shell helper for shot and
  reload casings: gun `particle.shellOffset`/`shellOffsetY`/`shellScale`/
  `shellForward`/`shellReverse` and `maxReload` are generated, and
  `caseTiming == "reload"` guns (revolvers/pistols) eject `maxReload` shells
  from the `Action_Reload`/`Action_ReloadAlt` start instead of on every shot.
- Shot casings now use the player position and `renderZOrd + 1` like the web.
- Host test: `gun_casing_fields_and_reload_shells`.

Done in the fifth P2 pass (2026-10-26):
- Live HUD gear/perk slots carry `Lvl. N` badges (red 4 / orange 3 / white
  otherwise) and are tap-to-drop: helmet/chest and droppable perks send
  `DropItemMsg`, backpack stays non-droppable like the web.
- Squad bars get the fixed group-colour dot per slot (yellow/magenta/cyan/
  orange), dead/disconnected names fade to 30%, and the downed row pulses
  (2 s black -> red, `icon-pulse`).
- Fixed the shared group colour table: slot 2 is cyan (`0x00ffff`), not blue;
  `GameConfig.h` now owns `kGroupColors`/`kTeamColors` for HUD, map dots and
  pings.
- Settings re-localizes **in place**: changing language (or Reset) rebuilds the
  open modal and re-syncs config, so every label updates without closing it.
- Host test: `hud_gear_levels_and_squad_polish`.

Done in the sixth P2 pass (2026-10-26):
- Role promoted/dead lines now reach the kill feed like game.ts: role defs carry
  English display names (plus Red/Blue Commander), `killFeed.assign/dead` and
  `killFeed.color`; `game/RoleFeed.h` builds the line/colour (role colour or
  team colour), GameScene appends it via `Game::addKillFeedText`, and the HUD
  renders custom rows.
- Role announcements now use the def's display name (`ROLE ASSIGNED: Commander`)
  instead of the raw role id.
- Host test: `role_killfeed_lines`.

Done in the seventh P2 pass (2026-10-26):
- Gas HUD icon: `gui/gas.png` while waiting, `gui/danger.png` while the red zone
  moves, with the 2 s danger pulse (GUI PNGs load through the adapter's texture
  fallback; no atlas entry needed).
- Host test: `hud_gas_icon_state`.

Done in the eighth P2 pass (2026-10-26):
- Menu splash background: `site_info.clientTheme` selects
  `MapDefs[theme].desc.backgroundImg` (generated `GeneratedMenuData.java`), drawn
  CENTER_CROP under the web's 30% black scrim (defaults to `main.webp`, decodes
  the web `.webp` assets directly on Android).
- Host tests: all existing suites still pass (Java-only change).
- Build/install: x86_64 APK built and installed on `emulator-5554`; launched to
  the menu with site_info loaded and no crash log.

Done in the ninth P2 pass (2026-10-26):
- Quick-play mode icons: `tools/build-menu-icons.mjs` rasterizes
  `MapDef.desc.icon` (15 maps: faction, halloween, potato, snow, desert, woods,
  beach, birthday, ...) to `Content/img/menu/<map>.png`; `MenuFragment` attaches
  the icon as a left compound drawable on each mode button.
- Verified the APK packages all `assets/img/menu/*.png`; the local site_info
  only exposes `main` modes (no icon), so themed icons need a dev server/site
  with themed modes for a visual check.
- Host tests still pass (Java/assets change only).

Done in the tenth P2 pass (2026-10-26):
- News chrome: the News button uses the rasterized `gui/news.png` icon (no
  emoji), and shows the web's red unread dot when the newest news date is newer
  than config `lastNewsTimestamp`; expanding the panel marks it read (same as
  `main.ts`).
- APK rebuilt/installed on `emulator-5554`; menu launched, site_info loaded,
  no drawable/crash errors.

Done in the twelfth P2 pass (2026-10-26):
- Result screen: the final VICTORY/DEFEAT panel now lists each teammate's
  kills/damage (sorted by kills, then damage) with the local row green and
  faction rows red/blue; the panel grows to 300 px to fit four rows.
- Host test extended: `hud_gas_timer_and_result_overlay`.

Done in the thirteenth P2 pass (2026-10-26):
- Live in-game leaderboard: the pause dialog has a Leaderboard button ->
  `SurvBridge.onRequestLeaderboard` -> `Game::leaderboardJson()`
  (activePlayerId + each player's name/team/kills/dead, tracked from
  `KillMsg.killerKills`) -> `SurvBridge.onNativeLeaderboard` -> the Java
  `LeaderboardModal` (rank/name/kills/alive, local row green).
- Host test: `game_leaderboard_snapshot`.

Done in the fourteenth P2 pass (2026-10-27):
- Perk-mode class picker (`ui.ts` setRoleMenuOptions / setRoleMenuInfo): on a
  perk map with no local role the HUD shows the 1200x770 panel at the web's
  0.8 scale - class tabs from `gameMode.perkModeRoles`, role `guiImg` art,
  role border colour, perk ring + loot icon + display names, "SELECT A CLASS"
  and the "ENTER GAME (20)" confirm/countdown. Confirm/timeout sends
  `PerkModeRoleSelectMsg` (128-bit) and persists config `perkModeRole`; the
  ambient_lab_01 cue starts/stops with the panel and a RoleAnnouncement closes
  it. Codegen now emits role `guiImg`/`color`/perk lists and map
  `perkModeRoles`, and pins `Math.random` so regenerated particle/role tables
  are deterministic.
- Host test: `hud_perk_role_menu_selects_and_confirms` (defs, preference
  selection, option/confirm hit-testing, 20 s auto-confirm, modal action bar).

Done in the fifteenth P2 pass (2026-10-27):
- Map atlas lifetime now matches `resources.ts loadMapAssets`: `GameScene`
  records every sheet a map loaded (`atlas/<name>/index.txt`) and, on the next
  MapMsg, removes the sheets of atlases not in the new map's list, then purges
  unused textures. `loadout`/`shared` stay resident because every map list
  includes them; the previous map's own atlas is released before the next one
  loads. Map audio needs no per-map preload here because `AudioManager` already
  preloads the whole sound table at startup.
- Device check (x86_64 emulator, temporary two-mode bundle config): playing
  `main` then quitting and joining `woods` logged `Unloaded atlas main`,
  `Loaded atlas woods`, and the woods match rendered normally.

Done in the sixteenth P2 pass (2026-10-27):
- Menu character preview (`ui/opponentDisplay.ts`, static-pose delta): the Java
  main menu has a `CharacterPreviewView` over the splash on the right.
  `tools/build-character-preview.mjs` rasterizes the outfit skin art
  (`player-base`/hands/`player-circle` backpack, PIXI-multiply tints) in the
  `fists` idle pose, the right hand as its own layer, and one melee layer per
  `worldImg` pivoted on the right hand (4x canvas, 512 px). The view polls
  `getLoadoutJson()` so tapping an outfit/melee in the loadout panel updates it
  live, and plays the web menu's idle bob plus a fists-keyframe swing every
  1.8-3.6 s (right hand and melee move together). `sync-content.ps1`
  regenerates the layers.
- Device check (x86_64 emulator): menu shows Basic/Semi-Pro outfit art at the
  right of the card, switching outfits in the open loadout updates the preview
  immediately, captured frames show the hand/weapon swing offset, and no
  crash/asset warnings appear.
- Not ported from `opponentDisplay.ts`: melee-specific idle/attack anim
  keyframes, particles and camera framing (P2).

Done in the seventeenth P2 pass (2026-10-27):
- NDK crash capture (`src/app/NativeCrash.*`): installs SIGSEGV/SIGABRT/SIGBUS/
  SIGILL/SIGFPE handlers on an alternate stack, writes
  `<files>/native-crash.txt` with the signal, fault address and up to 64 raw
  PCs via `_Unwind_Backtrace` (works below API 33 where bionic's `backtrace()`
  is unavailable), then restores the default action and re-raises so the
  platform still emits its tombstone. `AppActivity` installs it after
  `super.onCreate` and reports a previous run's file through
  `SurvApi.reportError("nativeCrash", ...)` before deleting it.
- C++ report call sites: `GameScene` reports `mapDefMissing` and (once per
  atlas) `atlasMissing` through the budgeted `ErrorLog` bridge.
- Diagnostic hook: `adb shell setprop debug.surv_crash 1` injects a native
  fault at startup; clear it afterwards.
- Device check (x86_64 emulator): the injected fault wrote a 149-byte report
  (`signal=11`, fault address, PCs), the next launch logged and deleted it,
  and `/api/report_error` received the request.

Done in the eighteenth P2 pass (2026-10-27):
- Menu preview melee animation (`ui/opponentDisplay.ts`): the Java avatar now
  holds the selected melee in that melee's `anim.idlePose` (two-handed axe,
  katana, machete, ... — previously everything rested in the fists pose) and
  plays the real melee animations instead of a hardcoded fists swing.
  `tools/codegen_preview_anims.mjs` emits `GeneratedPreviewAnims.java` from
  `animData.ts` + the melee defs: IdlePoses, each melee's
  attack/deploy/idle anim lists and the keyframes of every animation those
  lists reference (bones, easing, mask). `CharacterPreviewView` reimplements the
  `LoadoutDisplay.update` scheduling (deploy after a melee swap, 50% idle, else
  attack, `1.5 + random` s delay) and `player.ts` keyframe interpolation
  (blend from the idle pose, mirror for the single-entry fists attack), and
  draws the left/right hands as separate bone layers with the melee parented to
  MeleeR (anchor-centred sprite: `R(worldImg.rot + MeleeR.rot) *
  (MeleeR.pos + worldImg.pos) * worldImg.scale - MeleeR.pivot`, below or above
  the hand per `worldImg.renderOnHand`).
- `tools/build-character-preview.mjs` now emits `body_<outfit>`,
  `hand_l_<outfit>` and `hand_r_<outfit>` layers (both hands centred on their
  own canvas so bones can move them; the body layer no longer bakes the left
  hand) and clears stale layers from older naming schemes. The melee layer is
  baked anchor-centred (`worldImg.rot` + `scale`, no pivot) like the web's
  `createSprite()` anchor (0.5,0.5); the earlier pivot bake shifted the weapon
  by half a sprite.
- Framing: the view measures the body/melee alpha bounds and fits their union
  into the view (the web's `getCameraTargetZoom`/`getCameraLoadoutOffset`
  equivalent for a fixed Java slot), so long melees (katana/naginata) are never
  clipped.
- Device check (x86_64 emulator): the `fists` avatar animates (hands/weapon move
  between frames), and a temporary forced `katana` loadout rendered the blade
  anchor-centred in the `meleeKatana` idle stance and swung it around the hand
  with no `CharacterPreview` asset warnings and an empty crash buffer. Host
  tests: 3509 assertions / 207 tests, 0 failures.
- Not ported (still P2, and not applicable to the Java avatar): the heal/boost
  use-item particle emitters and the web's modal-relative camera framing (the
  view is laid out by the menu, not by a camera offset). The preview keeps the
  y-up local frame + static portrait facing of the port (no body rotation).

Done in the nineteenth P2 pass (2026-10-27):
- Java UI -> C++ one-shot sound bridge: `SurvBridge.playSound(name, channel)` ->
  `UiBridge::playSound` -> `AndroidUiBridge` (queued onto the GL thread like the
  other scene/audio mutations) -> `AudioManager::playSound`. Unknown keys and
  channels are dropped natively and the user's mute/volume settings still apply,
  so Java never has to know whether audio is available; the seam is recorded by
  `MemoryUiBridge` for host tests. This is the first UI-sound path in the native
  client (M6 listed UI sounds as unported because the Java screens had no way to
  reach the mixer).
- Menu avatar cues (`ui/opponentDisplay.ts`): the outfit change plays the new
  outfit's `sound.pickup` on the `ui` channel, and the animData.ts
  `animPlaySound` effects (melee `swing`/`deploy`/`idle`, fired in the same
  `(prevTicker, ticker]` window as player.ts) resolve through the item's sound
  table and play on `sfx`. `GeneratedPreviewAnims` gained a per-item SOUNDS
  table and per-anim sound effects; `renderOnHand`-style melee rows supply the
  sound keys.
- Host tests: `bridge_ui_host_roundtrip` records the sound name/channel, and
  `audio_ui_cues_follow_the_channel_type_volume` locks the channel-type mixer
  semantics the cues rely on (only the `music` channel follows `musicVolume`;
  `ui`/`sfx` and the web's ambient/map-music quirk follow `soundVolume`).
- Device check (x86_64 emulator, `debug.surv_debug=1`): the menu avatar's melee
  anims logged `[audio] play punch_swing_01sfx` (fists swing) with the expected
  volume, no crash/warning output; host tests 3526 assertions / 208 tests.

Done in the twentieth P2 pass (2026-10-27):
- Map indicators (`objects/mapIndicator.ts`): `UpdateMsg.mapIndicators` finally
  reaches the map views. `MapIndicatorBarn` is a data-only model now (id/type/
  pos/equipped + the pulse ticker; the never-wired `MapSpriteBarn` dependency is
  gone) and `GameScene` builds `MapIndicatorMarker`s from the def
  (`mapIndicator.sprite`/`tint`/`pulse`/`pulseTint`) for
  `Hud::updateMapIndicators` -> both `MapView`s. Marker sprites go into the
  existing sorted dot layer: equipped markers sort above the player dots
  (655350), plain ones below (1), and the `part-pulse-01.img` ring sits one lower
  (only for defs with `mapIndicator.pulse`) — the web's exact z-order rule
  (`MapView::indicatorZOrder`). `GameWorld::update` ticks the pulse
  (0.5..1 at 0.3/s, flipping at both ends, as in mapIndicator.ts), and a new map
  clears the barn. The frames (`player-the-hunted.img`, `player-king-woods.img`,
  `part-pulse-01.img`) are in the always-resident `loadout` atlas.
- Host tests: `map_indicator_barn_data_and_pulse` (add/update/free/slot reuse +
  the pulse ease), `map_indicator_defs_have_sprite_and_pulse` (the_hunted /
  helmet03_forest defs, plain item has none) and
  `map_view_indicator_markers_and_pulse_ring` (frame/tint/pos/scale/pulse ring +
  hiding when cleared). 3583 assertions / 211 tests.
- Device check (x86_64 emulator): with a temporary hook injecting a hunted
  marker at the active player, the big map drew `player-the-hunted.img` (orange
  tint) with its `part-pulse-01.img` ring above Todestfelde; the hook was
  removed for the final build.

Done in the twenty-first P2 pass (2026-10-27):
- Android gamepad support (native-only; the web has no controller code). The
  sticks/triggers arrive as `MotionEvent` axes: `AppActivity.onGenericMotionEvent`
  normalises them (device ranges, `AXIS_X/Y`, `AXIS_Z/RZ` with an `RX/RY`
  fallback, `AXIS_RTRIGGER`/`AXIS_GAS`) and sends
  `SurvBridge.gamepadAxes(moveX, moveY, aimX, aimY, fire)` -> `UiBridge` ->
  GL thread -> `GameScene::onGamepadMotion` -> `HardwareInput`.
  - Left stick = analog move via the same `InputMsg.touchMoveDir/Len` fields as
    the touch pads (radial deadzone 0.2 with the remaining travel rescaled).
  - Right stick = aim: the direction latches like the mouse cursor (a touch
    takes it back, `noteTouchInput`), the pull maps to `toMouseLen`
    (`MouseMaxDist`).
  - RT/R2 = fire (`shootStart`/`shootHold`, trigger threshold 0.5, one-frame
    press edge). Pads that report R2 as a button also work.
  - Buttons map onto fixed bind actions (`setPadAction`), so every action/edge/
    HUD rule is reused: A interact, B cancel, X reload, Y switch gun slots,
    L1/R1 previous/next weapon (wheel), L2 bandage, L3 stow, R3 large map,
    D-pad move/spectate and Start = pause (`showPause()`, since axmol only maps
    the Android BACK key). `isGameKey` now forwards
    `KEYCODE_BUTTON_A..BUTTON_MODE`.
  - Host tests: `gamepad_axes_map_to_move_aim_and_fire` (deadzone rescale, aim
    latch/touch handoff, trigger threshold/edge, reset) and the bridge recording
    in `bridge_ui_host_roundtrip`. 3613 assertions / 212 tests.
  - Device check (x86_64 emulator, `adb shell input keyevent`): `KEYCODE_BUTTON_X`
    / `A` / `B` / `R1` / `L1` produced InputMsg actions 5 (Reload) / 7 (Interact)
    / 6 (Cancel) / 17 (EquipNextWeap) / 18 (EquipPrevWeap), and `BUTTON_START`
    opened the pause dialog. Stick/trigger axes need a physical controller (the
    emulator cannot synthesise MotionEvents).

Done in the twenty-second P2 pass (2026-10-27):
- Keybind remap editor (`inputBinds.ts`): the native client now has the web's
  full bind model instead of hard-coded defaults.
  - `game/InputBinds.h` ports `BindDefs` (34 entries over the 36
    `shared/gameConfig.ts` `Input` actions), `setBind` exclusivity,
    `isKeyBound`, `loadDefaultBinds`, the `toArray`/`fromArray` bit stream
    (version byte + 2-bit type + 8-bit code per action, LSB-first) and the
    CRC-16/IBM trailer, so the `binds` config value is the **same base64 string**
    the web client stores and shares. Labels match `InputValue.toString()`
    (letters/digits/arrows/F-keys/mouse/wheel, `Key N` fallback).
  - `HardwareInput` resolves every action through the table
    (`bindDown`/`bindPressed`): movement (with the web's unbound-arrow
    fallback), fire, the `game.ts checkInputs` list, the Interact →
    Revive/Use/Loot split, swap slots and the use-item chain. Physical keys are
    stored by `input.ts Key` code; wheel events use the `MouseWheel` codes.
    Down events latch a one-frame press so a tap that lands between two frames
    is never lost (keyboard, mouse and gamepad buttons).
  - `GameScene` maps axmol keys and Android `KEYCODE_*` values onto `Key`
    codes; gamepad buttons now call `setPadAction` (fixed actions, independent
    of the keyboard table), so a remap cannot break the pad.
  - Bridge/Java: `SurvBridge.getBindsJson/getBindsCode/setBind/resetBinds/
    setBindsCode` -> `AndroidUiBridge` (UI-thread table, persisted to `binds`,
    copy pushed to the scene). `SettingsModal` has a Key Binds button and the
    new `BindEditorModal` lists every action, captures the next key (Backspace
    clears, Back cancels), restores defaults, copies the share code and imports
    a pasted code (CRC-validated in C++). The `EmoteMenu`/`TeamPingMenu` rows are
    listed for web parity but the native HUD buttons drive the wheels;
    `Fullscreen`/`HideUI` stay desktop-only (the same inert rows as the web's
    non-touch build).
  - Host tests: `input_binds_defaults_match_the_web_table` (base64/CRC fixture
    generated from the real `client/src/lib/crc.ts` + BitStream layout),
    `input_binds_code_roundtrip_and_corruption`,
    `input_binds_set_bind_is_exclusive_and_labels_match`,
    `hardware_input_resolves_actions_through_the_bind_table`,
    `hardware_input_interact_split_and_pad_actions`,
    `hardware_input_subframe_taps_are_not_lost`,
    `android_key_codes_map_to_input_key_codes`. 3722 assertions / 219 tests.
  - Device check (x86_64 emulator): startup wrote the default code; the editor
    captured K for Move Left (persisted across restart), Restore Defaults reset
    it, an invalid paste showed "Invalid bind code" and a valid shared code
    loaded ("Loaded!"). In a started match, `KEYCODE_G` (Reload remapped to G)
    produced `[input] actions=5` while `KEYCODE_R` produced nothing, and the
    gamepad buttons still produced `actions=7` (A) / `actions=5` (X).

Done in the twenty-third P2 pass (2026-10-27):
- Pass reward art + level-up pulse (`ui/pass.ts setPassUnlockImage` /
  `animatePassLevelUp`): `PassFragment` now shows the next unlock's generated
  art tile (`img/loadout/<type>.png`), the outfit tint through the same
  multiply colour matrix as the customize grid, the loadout category icon
  (`gui/loadout-<category>.png`) and the localized `loadout-title-<category>`
  label next to the existing "Next reward" text. When a refresh reports a higher
  level the art pops in (fade + overshoot scale) and the level label pulses,
  matching the web's level-up feedback instead of only swapping text.
  `tools/codegen_pass_defs.mjs` now also emits the unlock items' `ItemDef`
  metadata (category/tint/display name) from `shared/defs/register.ts`; the
  output stays deterministic.
- Item art is shared: the new `ItemArt` helper owns the 4 MB `LruCache` of
  decoded `Content/img/loadout/<type>.png` bitmaps and the outfit tint matrix,
  and `LoadoutFragment` delegates to it (no duplicated decode cache).
- Profile stats link (`ui/profileUi.ts .account-stats-link`): the
  `/stats/?slug=<slug>` label is now a blue link that opens
  `<webBaseUrl>/stats/?slug=<slug>` (or `/stats` when signed out is hidden).
- Device check (x86_64 emulator): the Pass screen rendered the `pass_survivr2`
  Lv 2 reward ("OUTFIT SKIN" + Rainy Day outfit art + the shirt category icon)
  with the real XP bar/quests; tapping the profile slug started
  `ACTION_VIEW https://survev.io/...` (the emulator had no default browser, so
  the system resolver appeared). Host tests unchanged: 3722 assertions / 219
  tests, 0 failures. The level-up pulse itself needs an actual level gain (not
  exercised on-device).

Done in the twenty-fourth P2 pass (2026-10-27):
- Crosshair editor parity (`ui/loadoutMenu.ts` #modal-content-right-crosshair):
  the Java Scope tab now matches the web controls instead of 3 presets.
  - Colour: the 7 swatches stay as shortcuts, plus the web's `#RRGGBB` text
    input (`#color-picker-hex`) applied on Done/focus loss; an unknown colour
    reverts to the current value. The custom colour is written to the nested
    `crosshair.color` (the C++ validator already accepts hex or a number).
  - Size: a SeekBar over the web range 0.25..1.0 (step 0.01) instead of
    Small/Medium/Large (the old 0.7/1.0/1.4 values are clamped into the range
    when an existing config is loaded).
  - Stroke: a SeekBar over the web range 0.0..1.5 in 0.25 steps.
    `tools/build-crosshair-icons.mjs` now bakes seven stroke buckets
    (`<type>_s0..s6`) and `GameScene` selects `round(stroke * 4)`, so the
    in-game sprite follows the slider (the old three buckets quantized 1.0/2.0
    to the same art).
  - Host test extended (`ui_loadout_crosshair_validation`): the web slider
    ranges round-trip through `ui::Loadout` (size 0.25 / stroke 1.5 / custom
    hex). 3727 assertions / 219 tests.
  - Device check (x86_64 emulator): the Scope editor accepted `#12AB34`, size
    0.63 and stroke 1.25; after a restart the match logged
    "[ui] crosshair type=crosshair_default color=12ab34 size=0.63 stroke=1.25
    frame=gui/crosshair/crosshair_default_s5.png found=1" and the green crosshair
    rendered at the screen centre. The editor was reset to white / size 1.00 /
    stroke 0.00 afterwards.

Done in the twenty-fifth P2 pass (2026-10-27):
- Health-bar colour parity (`ui2.ts` main bar + `ui.ts` squad bars):
  - `Hud::healthColor` now walks the `ui2.ts healthColourSteps` exactly (strict
    `>` on the ceiled health, `math.delerp`, `math.lerp` + `Math.floor` in
    double). The old walk used `>=` and skipped descending ranges, so 25..74
    was stuck on the 75-step colour (50 health showed `#ff9e9e` instead of
    `#ff7878`) and the 100/75 boundaries were wrong (100 is grey `#b3b3b3`,
    75..99 white).
  - The main bar adds the CSS `ui-bar-danger` pulse at 25 health or less: the
    fill alternates black <-> red (`rgba(255,0,0,0.8)`) over a 1 s cycle
    (`@keyframes pulse`, 0.5 s alternate), instead of a static red.
  - Squad bars use the `ui.ts updateHealthBar` palette instead of a fixed green:
    grey at 100, white at 75..99, darkpink -> lightpink interpolation below 75,
    solid red for downed at >25, no fill when dead, the same <=25 pulse, and the
    web's 1px minimum width for living members.
  - Host tests: `hud_health_color_steps` now covers the exact ramp boundaries
    (100/99/75/74/50/25/24/0), `healthDangerColor` (0/0.5/1 + clamping) and the
    squad palette; `hud_squad_health_bars` asserts the fill colour/alpha per
    row (grey 100, pink 50, dead none, downed 1px danger). Also fixed a dead
    assertion line that had been swallowed by a literal `\n` in a comment.
    3754 assertions / 219 tests.
  - Device check (x86_64 emulator): a quick-play match rendered the HUD with the
    grey full-health bar (the web's `#b3b3b3` 100 step), the health counter at
    100 and no GL/audio/crash errors.

Remaining:
- **Rendering:** exact normal/colour nuances (`M4_REMAINING.md` §1). `plane.ts`
  and `AirstrikeZone` are implemented; `MapSprite` (mapSprite.ts) is now only
  used by nothing at all (the live HUD draws dots/pings/indicators in
  `MapView`), so its z-order path is dead code rather than a gap.
- **HUD/menus:** menu preview heal/boost particle emitters (`opponentDisplay.ts`;
  the Java avatar has no heal/boost view, so this only matters if the preview
  gains those tabs).
- **Audio:** exact EQ/convolution timbre (the web biquad chains are approximated
  by an OpenAL low-pass); the web's channel-type volume quirk (ambient/map music
  follows `soundVolume`) is reproduced and locked by
  `audio_ui_cues_follow_the_channel_type_volume`.
- **Platform/perf:** device/tablet UI tiers (keybind remapping landed in the
  twenty-second pass); a physical gamepad soak.
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
- Web social/creator columns (Discord/Wiki/Ko-fi row, featured YouTuber/streamer
  blocks) are intentionally omitted from the native menu; there is no social UI
  requirement for the Android client.
- Touch-only controls (no local rotation on mobile, on-screen pads instead of
  mouse) are intended; BT keyboard/mouse and gamepad support is live (P1.6 +
  the P2 input passes, remappable in Settings → Key Binds).

### 3.6 Suggested session order (each ends with a green `surv_tests` + device check)

1. **P0.6** — real loadout (done; see §3.2).
2. **P0.4 + P0.5** — death/stats/spectate + match-over latch (done).
3. **P0.2 + P0.3** — active layer + interpolation: fixes bunkers and game feel
   (done; see §3.2).
4. **P0.7 + P1.5** — settings/audio correctness (done; see §3.2/§3.3).
5. **P1.1 + P1.2** — authenticated quick play/captcha + correct errors
   (done; see §3.3).
6. **P1.3 + P1.4** - render feedback + live inventory HUD (done; only the
   perk-mode role picker moved to P1.7/P2).
7. **P1.6-P1.8** - input (done), menu metadata/localization (done, P1.7),
   team/account refinements (done, P1.8).
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
- BT keyboard/mouse (or desktop/emulator mouse) play through the default
  `inputBinds.ts` set with cursor aim/wheel, remappable in Settings → Key Binds
  (shared/imported code matches the web); idle upstream traffic matches the
  web's change-gated InputMsg (1 s keepalive).
- The Java menu's quick-play buttons/regions come from site_info, Play waits for
  the fetch, and menu/settings/help/pause labels render in the saved locale
  (verified en/es on the emulator).
- Team: quitting a team match returns to the room, room mode/auto-fill/region
  persist, in-room rename works, and `survev://` / `https://…/#CODE` /
  `?u=&jt=` links open the correct screen (device-verified).

## 4. Verification workflow

```powershell
cd E:\work\survev\client-mobile
. .\tools\android-env.ps1
# host tests (219 tests, 0 failures expected)
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
node --experimental-transform-types tools/codegen_menu_defs.mjs
node --experimental-transform-types tools/codegen_preview_anims.mjs
node tools/gen_reference.mjs
node tools/build-atlas.mjs --res low
node tools/build-gui-icons.mjs
node --experimental-transform-types tools/build-loadout-icons.mjs
node --experimental-transform-types tools/build-menu-icons.mjs
node --experimental-transform-types tools/build-character-preview.mjs
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
- Health colours follow the web exactly: the main bar uses `ui2.ts healthColourSteps` (strict `>` walk on the ceiled health, `delerp`/`lerp` in double, so 100 = grey `#b3b3b3`, 75..99 white, 50 = `#ff7878`, 25 = `#ff5252`, <=24 red) and the squad bars use `ui.ts updateHealthBar` (grey 100 / white 75..99 / darkpink->lightpink below 75 / red downed). At <=25 both add the CSS `ui-bar-danger` pulse (black <-> red, 0.8 alpha, 1 s cycle). Do not "simplify" these back to a flat colour or the `>=` walk
- P0.1 action taps are one-shot edits queued in `game/InputActions` and cleared only after `Game::sendInput` runs (clearing at tap time loses taps that land between the 33 Hz sends)
- A normal match end is `GameOverMsg` followed by the server's `stop()` close (3000/0): `GameScene::update` must keep the world/result when `Game::gameOver() != nullptr` and skip reconnect/URL fallback/error, and the liveness watchdog skips once `_hasGameOver` is set
- `PlayerStatsMsg` (team death, teammates alive) is the death-panel source (`Game::localStats`); the final `GameOverMsg.playerStats` entry is used for the result. Do not discard it again (`Game.cpp` used to)
- Spectating is server-driven: `Game::isSpectating()` is `activePlayerId != localPlayerId`; camera/active player follow `UpdateMsg.activePlayerIdDirty`. The HUD's PREV/NEXT send `SpectateMsg` 2/3 and are hidden in solo
- `GamePauseDialog` starts `GONE` (ensurePause() can create it before the user opens it); `show()`/`hide()` own visibility. `showResult()` only enables the Spectate button
- While `Game::isLocalDead()`/`isSpectating()`, gameplay inputs/pads and the emote/ping wheels are disabled and the HUD action bar is hidden; only the overlay buttons/minimap respond
- Loadout item lists come from `codegen_render_defs.mjs` (`LoadoutItemDef` table) via `SurvBridge.getLoadoutItemsJson()`; never hardcode item IDs in `LoadoutFragment.java` again
- Loadout item art is `Content/img/loadout/<type>.png`, generated by `tools/build-loadout-icons.mjs` from the web SVGs (`Content/` is gitignored; `sync-content.ps1` runs the tool). Java reads it as assets path `img/loadout/<type>.png`; melee rotation/mirror is baked into the PNG, outfit `lootImg.tint` is applied with a `ColorMatrix` (the web's SVG multiply filter)
- The emote loadout is **6** slots (Top/Right/Bottom/Left/Win/Death): editing one merges into the existing array (`UiBridge::setLoadoutJson` only merges top-level keys) and must not replace the other five
- The crosshair type is the nested `{"crosshair":{"type":...}}` shape; a flat `{"crosshair":"..."}` is ignored by `ui::Loadout::validated`
- `AccountManager` re-pushes `account.items` when the loadout opens (`LoadoutFragment.show`): the profile fetch can finish before `installUiBridge`, so the early `setAccountItems` call is a no-op and the C++ owned set would stay at `unlock_default`
- In the profile handler, parse/set `items` **before** `setLoadout`: the server loadout is validated against the owned set, so a pass item selected on another device would otherwise be reset to the stock default
- New-item confirm/ack goes through `AccountManager.setItemStatus` (status 1 = Confirmed, 2 = Ackd); the local `ItemInfo.status` is bumped first so a refresh does not re-POST
- P0.1 weapon-card taps equip, but a card touch that moves >`kHudDragHandoffPx` is handed to `Touch` (the cards overlap the bottom-right aim pad, so claiming every drag would break aiming)
- `MapMsg` place positions are normalized (0..1); do not divide by map width/height again
- Place names are drawn as HUD text in `MapView`, not baked into the RenderTexture (text-in-RT did not show on device)
- Map dot styles (role icon/dead/downed scale + outer ring + z bump) come from `mapDotStyle`; keep it in sync with `ui.ts updatePlayerMapSprites`
- Reverb is per-source OpenAL EAX: `AxmolAudioBackend` iterates active handles (no global convolver bus); `ReverbModel` is the host-tested mapping
- The camera root is **positive-Y** (`Renderer::applyCamera`): axmol y-up origin = `screenH - pointToScreen(0,0).y`; world +y is screen-up. World labels do not `setFlipY`
- `GameWorld::update` must call `renderer.setActiveLayer(_activePlayer->layer)` and `setUnderground(...)` **before** `renderer.m_update` (a one-frame-late layer/underground makes the bunker fade, ground fade and mask release lag/desync)
- Player/projectile interpolation is `visualPos(Old)` + `posInterpTicker / camera.m_interpInterval` seeded in `m_updateData` (`isNew ? data.pos : pos/dir`); the camera follows `activePlayer->visualPos`. Do not draw or follow raw `pos`
- A duplicated net update re-seeds `visualPosOld`/`visualDirOld` from the current net position and restarts the ticker (web `player.ts:508-515`); it must not slide back to the previous sample
- `config.interpolation` lives on the scene (`GameScene::_interpolationEnabled`) and is re-applied by `resetWorld()`, because each match builds a new `GameWorld` camera; `UiBridge::setConfigValue("interpolation")` pushes it through `runOnGameThread`
- Bullet rendering is `BulletVisual::trail` (`player-bullet-trail-02.img`) with world scale divided by `camera.m_z()`; do not go back to a Graphics line. Collision/impact FX belongs in `BulletBarn::update` (obstacles + players, blood parented to the victim), because the server only sends the spawn event.
- Explosion visuals come from the generated `ExplosionEffectRenderDef` table (`shared/defs/explosionEffectDefs.ts`); do not reintroduce the old orange-circle `ExplosionBarn` drawing.
- Loot's outer ring is a separate `borderSprite` child at index 0; the item art keeps `lootInnerScale`/mirror/rotation and the ring tint comes from `lootBorderTint` (ammo's `tintDark` fallback at codegen).
- Player nametags are visible only for same-group, non-active teammates (30 px below); faction patches are only drawn in `factionMode` and use the 2 team colors.
- Airdrops own one looping `airdrop_fall_01` handle (chute once); plane/fighter loops and `msg.airstrikeZones` are created from the UpdateMsg, and `ExplosionBarn` drives `Camera::m_addShake`.
- All runtime game.ts settings (interpolation/screenShake/localRotation/anonPlayerNames/touch styles/aim line + volumes/mute) are applied by `AndroidUiBridge::applyGameSettings` at install and after `setConfigValue`; `GameScene`/`GameWorld` keep them so a rebuilt match world gets the same flags
- Settings JSON values are numbers/bools: Java must use `Json.peekFloat`/`peekBool` (not `peekString`); the language list is `SurvBridge.getLocalesJson()` (`ui::kLocales`), never a hand-written Java array
- `Touch` defaults to web `anywhere`; `GameScene::setTouchSettings` validates `locked`/`anywhere` and applies move/aim style + aim line live (the Java buttons persist the same keys)
- `AudioManager::setVolume` multiplies master *and* type volume; per-instance `base` is the pre-master volume so setting master from 0 -> 1 restores loops (do not scale only by a ratio)
- `AudioManager` enforces `maxInstances` (kill oldest), `canCoalesce` (30 ms equal-power merge), `delay` (scheduled in seconds; shot.ts pre-converts ms), `detune` (cents -> axmol pitch) and ambient retention; `stopAll(true)` keeps ambient/music, `stopAll(false)` is the hard stop
- Pause/background must not stop audio; leaving a match keeps ambient/music loops (`stopAll(true)`), and `GameScene` ticks `AudioManager` + `Ambiance` every frame so the Java menu keeps its music
- `AxmolAudioBackend::isPlaying` treats PAUSED as alive (mute uses pauseAll) and pauses handles started while muted; otherwise a muted menu loop would play after `resumeAll`
- Sound keys come from the generated defs (`GameObjRenderDef.soundReload/ReloadAlt/Deploy/Use/Pickup`, obstacle door/button/explode, building puzzle/ceiling); do not reintroduce `<weapon>_01`/`<item>_pickup_01` guesses
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
- Crosshair sprites come from `Content/gui/crosshair/<type>_s<0..6>.png` (`build-crosshair-icons.mjs`, staged by `sync-content.ps1`): seven 0.25 stroke steps over the web slider's 0.0..1.5 range, frame index `round(stroke * 4)`. The Java Scope editor writes the web float ranges (size 0.25..1.0, stroke 0..1.5) and a hex colour; the nested `crosshair` config shape wins over stale flat keys
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
- `find_game_v2` reads the session from the cookie **or** `Authorization: Bearer`
  (native); keep both. Quick play must send `playerCount: 1` (not teamMode),
  `SurvBridge.getProtocolVersion()` (not a literal 1028) and the selected
  region's zones; team playGame does the same.
- Captcha is not implemented in the native client (no WebView/Turnstile). A
  signed-out player gets the localized `index-invalid-captcha` dialog; an
  authenticated device session bypasses the server check. Do not silently send
  an empty token for signed-out players.
- Disconnect reasons are the server's 3000 + reason string, mapped by
  `game/DisconnectReason.h` (`disconnectReasonKey`); never reintroduce invented
  4001–4011 close codes. Invalid-protocol/IP-ban/behind-proxy and account-banned
  use `MenuFragment.showErrorModal` dialogs, not just the status line.
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
  to `passDefs.ts`/`questDefs.ts` (XP curve, unlock table, quest XP/timed) or
  to the unlock items' defs (the generated `ItemDef` table carries the loadout
  category, outfit `lootImg.tint` and display name used by the reward art).
  Display names are l10n keys (`pass_*`, `quest_*`, `game-<item>`), not defs.
- Decoded loadout art is cached in `ItemArt` (4 MB `LruCache`) and shared by the
  customize grid and the pass reward art; the outfit tint is the web's
  multiply `ColorMatrix`, applied only to `category == "outfit"`. Do not decode
  `img/loadout/**` per screen again (the pass screen used to have no art, and a
  second cache would double the bitmap memory).
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
- Obstacle destroy debris is spawned **once** from `Obstacle::update` (guarded by
  `dead && !exploded && !isNew`) using `colliderToAabb(collider)` and the def's
  `explodeParticle`/`explodeParticleAlt`; do not move it back into `render` (the
  old render-time condition spawned 5-11 particles every frame).
- Button sprites come from `def.buttonUseImg` (on) / `def.buttonOffImg`
  (not usable), and `def.buttonUseParticle` spawns on the button seq change.
  `buttonSeqOld` advances even without an audio backend.
- `Obstacle::m_updateData` resolves the def from the persisted `type` and only
  reads full state (type/layer/health/dead/button/door/puzzle/skin) when
  `fullUpdate` is set: partial updates carry pos/ori/scale only. Do not read
  `data.type`/`data.dead`/`data.healthT` there — a partial ObjectData is empty
  and would clear the button sprite, door interpolation and health tint.
- Player visual scale is `m_bodyRad / PlayerConfig::radius` (eased at `dt * 6`
  in `Player::update`, snapped on `isNew`), not the raw net `scale`; the body,
  gun/mag and melee sprites all use it. `maxVisualRadius` is 3.75.
- Bush rustle uses `def.isBush` + `def.soundEnter` + `def.hitParticle`; do not
  reintroduce the `isTree`/`bush_enter_01`/`leaf` hardcodes.
- Ceiling destroy FX uses `def.ceilingDestroyParticle` �
  `def.ceilingDestroyParticleCount` inside `surfaces[0]` and the sound from
  `ceiling.destroy.sound` (codegen: never emit `def.sound`, it is an object).
- Particle colours with a TS closure become HSV ranges in codegen
  (`ParticleDef.colorVariance/colorHueMin..colorValMax`) and
  `Particle::m_init` picks a fresh `math::hsvToRgbInt` colour per particle; plain
  numeric defs keep `ParticleDef.color`. Do not reintroduce the codegen-time
  averaged RGB (it removed all per-particle variance).
- `Map::getGroundSurface` uses `mapDef.riverShoreSurface` on river banks
  (codegen from `mapDef.biome.sound.riverShore`); only `main_spring` and
  `woods_spring` are `stone`, every other map is `sand`.
- Shell casings go through `createCasingParticle` (Barns.cpp): gun codegen
  supplies `shellOffset/shellOffsetY/shellScale/shellForward/shellReverse`,
  `maxReload` and the resolved casing type. Shot casings use `renderZOrd + 1`;
  `casingOnReload` guns spawn `maxReload` shells once per reload action start,
  never per shot.
- Group/team colours live in `GameConfig.h` (`kGroupColors` slot 2 is cyan
  `0x00ffff`); HUD squad dots, map dots and ping indicators all use it. Do not
  reintroduce the old blue `0x0000ff` local tables.
- Gear/perk HUD state is `Hud::GearSlot` (frame + type + level + droppable) via
  `setGearSlots`; `setGearState(frames)` is only the test/back-compat wrapper.
  Helmet/chest and droppable perks are tap-to-drop (`TapKind::GearSlot` ->
  `DropItemMsg`), backpack is excluded like ui2.ts. `Lvl. N` badges: red at 4,
  orange at 3, white otherwise.
- `SettingsModal` is rebuilt (`rebuildUi()`: clear root -> `buildUi()` ->
  `syncFromConfig()` -> `applyTouchStyleButtons()`) after a language change or
  Reset; keep widget fields non-final so the rebuild can reassign them.
- Role kill-feed lines are built by `game/RoleFeed.h` from `RoleRenderDef`
  (`name`, `nameRed/nameBlue`, `killFeedAssign/Dead`, `killFeedColor`) and
  appended with `Game::addKillFeedText`; custom entries render via
  `KillFeedEntry.custom/text/color` (do not force them through `KillMsg`).
  Role defs are generated from `roleDefs.ts` + `en.json` names.
- `Hud::_gasIcon` uses `createSprite("gui/gas.png")` / `"gui/danger.png"`:
  `AxSprite::setFrame` falls back to `setTexture` for non-atlas names, so GUI
  PNGs work without atlas entries. Keep the 20 px scale and 2 s pulse.
- `GeneratedMenuData.java` is generated from `shared/defs/maps/*` by
  `tools/codegen_menu_defs.mjs` (never hand-edit): the Java menu looks up
  `site_info.clientTheme` for the splash background and the map's mode icon.
  Android decodes the packaged `.webp` splash directly under the 30% SCRIM.
- Mode-button art comes from `MapDef.desc.icon`:
  `tools/build-menu-icons.mjs` rasterizes it to `Content/img/menu/<map>.png`
  (run by `sync-content.ps1`), and `MenuFragment` attaches it as a left compound
  drawable when `GeneratedMenuData.modeIcon()` is non-empty.
- News button chrome: `gui/news.png` is the left compound drawable; the right
  compound drawable is the red unread dot while the newest news ISO date is
  newer than config `lastNewsTimestamp` (UTC date parse). Expanding the panel
  writes `System.currentTimeMillis()` through `SurvBridge.setConfigValue` and
  re-renders the badge.
- The result panel includes up to four teammate stat rows (`_overStatRow`,
  `_overStatRowHas`) fed by `GameOverMsg.playerStats`, sorted by kills/damage;
  `Hud::showGameOver` takes an optional `const Game*` for names/team colours.
  Death-panel mode hides the rows. The panel is 300 px tall.
- The in-game leaderboard is Java-only UI fed by the C++ snapshot:
  `SurvBridge.onRequestLeaderboard` -> `UiBridge::requestLeaderboard` (game
  thread) -> `Game::leaderboardJson()` -> `SurvBridge.onNativeLeaderboard` ->
  `JavaUiHost.showLeaderboard(String)`. Kills come from the latest
  `KillMsg.killerKills`; there is no live damage field, so the modal shows
  rank/name/kills/alive only.
- `GeneratedPreviewAnims.java` is generated from `animData.ts` + the melee defs
  by `tools/codegen_preview_anims.mjs`; never hand-edit it (re-run after any
  change to IdlePoses/Animations or a melee's `anim` lists). Rows are
  `"pivotX,pivotY,rot,posX,posY"` per bone in animData.ts Bones order, and the
  `MeleeR` spin belongs in the melee layer, not in the hand.
- The menu avatar layers are `img/preview/body_<outfit>`,
  `hand_l_<outfit>`, `hand_r_<outfit>` and `melee_<melee>` (tools/
  `build-character-preview.mjs`, staged by `sync-content.ps1`). The hand layers
  are centred on their own canvas so the bone pose can move them; do not bake
  the left hand into the body layer again. The melee layer is **anchor-centred**
  (web `createSprite()` sets anchor 0.5,0.5) with `worldImg.rot` + `scale`
  baked; its worldImg pos/rot/scale come from
  `GeneratedPreviewAnims.melee(...)`, and Java applies
  `R(rot) * (MeleeR.pos + worldImg.pos) * scale - MeleeR.pivot`. Do not go back
  to the old pivot bake (it shifted the weapon by half a sprite and hid long
  melees).
- The preview fits the measured body+melee alpha bounds into the view instead of
  using a fixed zoom (the web's loadout camera framing): keep the melee box
  union in `updateFraming()` or a katana/naginata gets clipped by the 240 px
  slot.
- The preview's local frame is **y-up** like the C++ player root (canvas y =
  centre - worldY * 4, rotations negated), and the body is drawn in the port's
  static portrait facing (no web body rotation). Keep positions/rotations in
  that frame or the hands/melee will mirror.
- `CharacterPreviewView` idles only through the generated melee anims: deploy
  after a melee swap, 50% idle / else attack, `1.5 + random` s delay, and the
  final keyframe ends the animation back into the idle pose (LoadoutDisplay
  `animIdleTicker` + player.ts `updateAnim`). Do not reintroduce the old
  hardcoded fists ATTACK_X/ATTACK_Y swing.
- Java UI sounds go through `SurvBridge.playSound(name, channel)` ->
  `UiBridge::playSound` -> `AndroidUiBridge` (queued with `runOnGameThread`) ->
  `AudioManager::playSound`; do not call the audio backend or a JNI mixer
  directly from Java, and do not add a Java volume mapping (mute/volumes are
  applied natively, unknown keys/channels are dropped).
- Channel-type volume semantics are the web's: only the `music` channel follows
  `musicVolume`; `ui`/`sfx`/`ambient` follow `soundVolume` (the web quirk that
  in-game map music on the `ambient` channel uses the sound slider).
  `audio_ui_cues_follow_the_channel_type_volume` locks it.
- The avatar's anim sounds come from `GeneratedPreviewAnims` (`animEffects` +
  the per-item `SOUNDS` table), fired in the player.ts `(prevTicker, ticker]`
  window; the outfit pickup fires on `ui` when the outfit changes (never on the
  first load, like `opponentDisplay.ts` skipEffects).
- Map indicators are **data** in `MapIndicatorBarn` (id/type/pos/equipped +
  pulse ticker) and **drawn by `MapView`**: `GameScene` builds
  `MapIndicatorMarker`s from the def (`mapIndicator.sprite/tint/pulse/pulseTint`)
  and calls `Hud::updateMapIndicators`. Do not re-add a `MapSpriteBarn`/factory
  dependency to the barn, and keep the web's z-order rule
  (`MapView::indicatorZOrder`: equipped 655350, plain 1, pulse one lower) and the
  0.5..1 @ 0.3/s pulse (`MapIndicatorBarn::updatePulses`).
- The indicator frames (`player-the-hunted.img`, `player-king-woods.img`,
  `part-pulse-01.img`) live in the always-resident `loadout` atlas; the map's
  atlas unload must not release them (it only unloads per-map sheets).
- `GameWorld::loadMap` clears the indicator barn, so a new match never inherits
  the previous match's markers.
- Android gamepads are native-only: axes arrive through
  `SurvBridge.gamepadAxes` -> `UiBridge::gamepadMotion` -> GL thread ->
  `GameScene::onGamepadMotion` -> `HardwareInput` (`+Y` up, deadzone 0.2 with the
  travel rescaled, aim latched like the mouse until `noteTouchInput`, trigger
  threshold 0.5). Buttons call `HardwareInput::setPadAction` with their fixed
  action (A interact, B cancel, X reload, Y swap slots, L2 bandage, L3 stow,
  R3 map), independent of the keyboard/mouse bind table; L1/R1 emit wheel
  steps. `AppActivity.isGameKey` must keep forwarding
  `KEYCODE_BUTTON_A..BUTTON_MODE`, and `onGenericMotionEvent` must stay guarded by
  `isInGame()`/`isPauseOpen()`.
- Key/mouse binds are `game/InputBinds.h` (port of `inputBinds.ts`). Keep the
  `binds` config value bit-compatible with the web: version byte 1, 36 actions
  (one per `shared/gameConfig.ts` `Input`, including the unbound
  frag/smoke-grenade slots), 10 bits per action (2-bit `InputType` + 8-bit code,
  LSB-first inside each byte), CRC-16/IBM big-endian trailer. `toArray()` is 48
  bytes / 64 base64 chars for the default table; the host fixture in
  `tests/test_input_binds.cpp` is generated from the real `client/src/lib/crc.ts`.
- The bind table is UI-thread owned by `AndroidUiBridge`; every edit persists
  `binds` and pushes a **copy** to the scene (`GameScene::setInputBinds`), so the
  game thread never reads a table the Java UI is mutating. Do not move the table
  into `GameScene` without marshalling the Java calls.
- `HardwareInput` stores physical keys by `input.ts Key` code (0..255), not by a
  native enum; `GameScene` maps axmol keys and Android `KEYCODE_*` values through
  `input::webKeyFromAndroidKeyCode`. Movement keeps the web's unbound-arrow
  fallback (`Key.Left` moves only while that key is not itself bound).
- Key/mouse/gamepad **down** events latch a one-frame press
  (`_keyPressedLatch`/`_mousePressedLatch`/`_padPressedLatch`, cleared in
  `flush()`): Android can deliver a down+up between two rendered frames, and
  dropping that edge loses a reload/interact tap.
- `GameScene::updateHardwareUi` uses the raw `Key.Left`/`Key.Right` presses for
  spectating (game.ts does not use the movement binds there) and the
  `ToggleMap`/`CycleUIMode` binds with the unbound `Key.G` fallback for the map.
- Gamepad Start opens the pause dialog directly (`JavaUiHost.showPause()`, which
  also calls `SurvBridge.onPauseGame()`): axmol only maps the Android BACK key, so
  forwarding Start as a game key would do nothing. Its key-up is swallowed.
