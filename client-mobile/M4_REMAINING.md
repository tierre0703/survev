# M4 rendering — remaining work (handoff)

This is a self-contained work order for finishing **M4 (rendering port)** in a
new session. It records what already works, the critical fixes that must not be
regressed, the exact build/test workflow, and the remaining tasks with enough
spec to implement them. Read `plan.md` §"M4 rendering port" for the original
module map.

**Latest implementation/verification: see §7.** Sections 1–6 record the prior
handoff; their outstanding player/polygon tasks have now been implemented.

---

## 1. Current status (verified on-device)

The M4 code exists and host-tests pass, but it had **never been run on device**.
This session fixed the blockers and verified the app runs:

- App installs, launches, connects to the dev server, and stays stable.
- Renders: terrain (water/beach/grass), order-0/1 ground patches, grid,
  joystick overlay, name labels.
- Does **not** render yet: sprite-based objects (obstacles, loot, players,
  particles), buildings (floor/ceiling), polygon holes, underground fade,
  full player pose.

### Critical fixes already made (do NOT regress)
1. **Reparenting in both adapters** (`AxmolPixi.h`, `NullPixi.h`):
   `addChild`/`addChildAt` remove the child from its old parent first (PIXI
   `Container.addChild` semantics). axmol's `Node::addChild` asserts (no-op in
   release) when a child already has a parent; `Renderer::addPIXIObj` re-adds
   objects whose `zIdx`/layer changes. Host test: `renderer_reparents_between_layers`.
2. **Wrappers own their native node** (`AxNodeImpl::ownNode()` + destructor
   `release()`). `create()` nodes are autoreleased; unparented wrappers (the
   layer mask, used only as a `ClippingNode` stencil) were freed with the frame
   pool. `AxRenderTexture` retains/releases too.
3. **`drawSolidPoly` uses the convex-fan path** (`isconvex=true`) — axmol's
   default `isconvex=false` runs poly2tri CDT, which crashed on the layer mask.
   The layer mask is drawn as separate `drawRect`s (canvas fallback), not one
   polygon.
4. **`axslc` shaders are packaged**: `build-apk.ps1` ships
   `proj.android/build/runtime/axslc/**` as `assets/axslc/**`. Without them
   axmol compiles empty shaders → black screen + `The vertex attribute
   'a_position' ... not exist` in logcat.
5. `Renderer::addPIXIObj` clamps `layerIdx` to `0..3`.
6. `build-native.ps1` quotes `-DANDROID_ABI=$abi` (was passed literally on a
   fresh build dir).

---

## 2. Build & test workflow (unchanged from this session)

```powershell
# Environment (must dot-source first; sets ANDROID_HOME/NDK/CMake 3.22.1/ninja)
cd E:\work\survev\client-mobile
. .\tools\android-env.ps1

# Host tests (engine-independent; no axmol)
cmake -S tests -B build-tests -G "Visual Studio 17 2022" -A x64
cmake --build build-tests --config Release
.\build-tests\Release\surv_tests.exe        # currently 41 tests, 0 failures

# Native lib (one build dir per ABI; first build compiles the engine)
.\tools\build-native.ps1 -Abis "arm64-v8a"          # device / Houdini emulator
.\tools\build-native.ps1 -Abis "x86_64"             # native emulator (best for crash stacks)

# APK (Gradle-free; includes resources.arsc fix + axslc)
.\tools\build-apk.ps1 -Abis "arm64-v8a"
.\tools\build-apk.ps1                                # both arm ABIs
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
The dev server was already running in this session (API `:8000`, game server
`:8001`, game processes `:9000+`). To start it: `pnpm dev:server` (needs Postgres
on `:5432` + Redis on `:6379`, both present). Note `pnpm` may prompt to reinstall
deps; if so run the servers directly with
`server\node_modules\.bin\tsx.cmd watch ... src/api/index.ts` / `src/gameServer.ts`.

Point the emulator at the host with `adb reverse` (no server config change):
```powershell
& $adb reverse tcp:8000 tcp:8000
& $adb reverse tcp:9000 tcp:9000
```
`DevConfig.h` has `kApiBaseUrl = "http://127.0.0.1:8000"`; the app calls
`/api/find_game_v2` and joins the returned `ws://127.0.0.1:9000/play`. Join
tokens are single-use and expire in 10s, so always use the discovery path.

### Debugging crashes
The arm64 emulator runs under **Houdini ARM translation** → backtraces are
useless. Build/install the **x86_64** APK (the emulator is x86_64) for real
stacks. Symbolize with the NDK:
```powershell
$sym = Join-Path $env:ANDROID_NDK_HOME 'toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-symbolizer.exe'
& $sym --obj=build-android-x86_64\libSurvevMobile.so --functions=linkage --inlines <addr>
```

---

## 3. Remaining tasks (priority order)

### T1. Sprite atlas pipeline — HIGHEST (unblocks all sprites)
**Problem:** every sprite name is `"<name>.img"` (e.g. `loot-shirt-01.img`).
The web client builds **virtual atlases** from `client/public/img/**/*.svg` via
`client/atlas-builder/vitePlugin.ts` (devDeps: `maxrects-packer`, `sharp`,
`svgo`, `canvas`). The native app only stages the raw `.svg` files and never
loads a `SpriteFrameCache`, so `AxSprite::setFrame` falls back to
`setTexture("loot-shirt-01.img")` which fails → nothing draws.

**Plan:**
1. Generate the atlas: `cd client && pnpm build` (production mode enables
   `atlasBuilderPlugin(true)`); inspect its output dir/format. (If the full
   Vite build is heavy, run the atlas builder directly.)
2. Convert to axmol: either a `.plist`+`.png` spritesheet
   (`SpriteFrameCache::addSpriteFramesWithFile`) or register each frame manually
   from the atlas JSON. Prefer the plist path.
3. Load it at startup in `GameScene::init` (before `installGeneratedDefs`).
4. Map the `*.img` names to atlas frame names in `AxSprite::setFrame`
   (`getSpriteFrameByName(name)` where `name` is the `.img` key).
5. **Acceptance:** obstacles/loot/player/particles render on-device.

### T2. Building floor/ceiling rendering + vision fade
Buildings currently render **nothing** (`Building::update` only ticks the
ceiling timer). Spec from `client/src/objects/building.ts`:
- Images come from the def: `def.floor.imgs[]` (per-image `{sprite, pos, rot,
  scale, alpha, tint, mirrorX, mirrorY}`) and `def.ceiling.imgs[]` (adds
  `removeOnDamaged`). Each image is a sprite created once when `isNew`.
- Z-order: floor `zOrd = def.zIdx`, ceiling `zOrd = 750 - def.zIdx`;
  `zIdx = __id * 100 + i`. Render via `renderer.addPIXIObj(sprite, layer, zOrd, zIdx)`.
- Layer: `layer = this.layer`; if `img.isCeiling && (layer == activePlayer.layer
  || (activePlayer.layer & 2 && layer == 1))` then `layer |= 2`.
- Ceiling fade: `visible = ceiling.visionTicker > 0`;
  `fadeAlpha += step(fadeAlpha, visible ? 0 : 1, dt * (visible ? 12 : vision.fadeRate))`
  where `step(cur,target,rate) = { d=target-cur; s=d*rate; return |s|<0.001?d:s; }`,
  `vision` defaults `{dist:5.5,width:2.75,linger:0,fadeRate:12}`, `fadeAlpha` starts `1`.
  Ceiling sprites use `alpha = imgAlpha * fadeAlpha`; floors use `alpha = imgAlpha`.
  `ceilingDamaged` hides ceiling imgs with `removeOnDamaged`.
  `ceilingDead` forces `canSeeInside=true` and spawns a residue sprite
  (`def.ceiling.destroy.residue`) on `imgs[0]`.
- Per-frame transform: `screenPos = camera.m_pointToScreen(pos + rotate(imgDef.pos, rot))`,
  `screenScale = camera.m_pixels(scale * defScale)`, mirror X/Y, `rotation = -rot + oriToRad(imgDef.rot)`.

**Blocker:** `Defs.h::MapObjectDef` only has a single `img`. Extend it with
`std::vector<BuildingImageDef> floorImgs/ceilingImgs` and update
`tools/codegen_render_defs.mjs` → `src/render/GeneratedDefs.cpp`
(`RawMapObj` needs floor/ceiling arrays). Also add `ceiling.destroy.residue`
and `ceiling.vision`.

### T3. particles.ts emitters
`ParticleBarn::update` is a stub. Full spec from `client/src/objects/particles.ts`:
- **Particle**: `pos,vel,rot,rotVel,rotDrag,delay,life,drag,scale,scaleEnd,
  scaleExp,alpha,alphaEnd,alphaExp,alphaIn*, tint, sprite`.
  Init: `life=range(def.life)`, `drag=range(def.drag)`,
  `rotVel=range(def.rotVel)*(rand<.5?-1:1)`, `rotDrag=range(def.drag)/2`,
  `scale=range(def.scale.start)*scaleParam`, etc.
  Update (after `ticker>=delay`): `t=min((ticker-delay)/life,1)`;
  `vel *= 1/(1+dt*drag)`; `pos += vel*dt`; `rotVel *= 1/(1+dt*rotDrag)`;
  `rot += rotVel*dt`; `scale = remap(t, scale.lerp.min,max, scale, scaleEnd)`;
  `alpha = remap(t, alpha.lerp.min,max, alpha, alphaEnd)`; alphaIn override;
  `if !hasParent: scale = camera.m_pixels(scale)`; free at `t>=1`.
  No gravity.
- **Emitter**: `{active,enabled,type,pos,dir,scale,layer,duration,radius,ticker,
  nextSpawn,spawnCount,parent,alpha,rateMult,zOrd,color}`. Spawn loop:
  `rad=scale*radius; pos+=randomPointInCircle(rad); dir=rotate(dir,(rand-.5)*angle);
  vel=dir*range(speed); rot=range(def.rot); addParticle(...); nextSpawn +=
  range(rate)*rateMult; spawnCount++`. Free at `ticker>=duration`.
- Emitters are added by `map.ts` (biome camera particles: `falling_leaf`,
  `falling_snow_*`, `falling_potato`, ...), `building.ts` (`cabin_smoke_parent`,
  `campfire_smoke`, `bathhouse_steam`, `bunker_bubbles_01`), `loot.ts`
  (`xp_*`), and `player.ts` (heal/boost/haste emitters). Direct `addParticle`
  callers: bullet casings/muzzle, blood, explosions, water ripple.
- Render: `renderer.addPIXIObj(sprite, layer, zOrd)` (auto `zIdx`); `layer |= 2`
  stairs logic as elsewhere; emitter zOrd = `EmitterDef.zOrd ?? ParticleDef.zOrd ?? 20`.

**Blocker:** the emitter/particle defs aren't exposed. Either extend the codegen
or hardcode the common particle/emitter defs in `Barns.cpp`.

### T4. Full player.ts pose/outfit rendering
`player.ts` is ~3000 lines with a skeletal system (`animData.ts`: `Bones`,
`Poses`, `Animations`): body + handL/handR + footL/footR containers, backpack,
chest, helmet, visor, melee, gun sprites, per-frame pose interpolation.
A subset is done: `Player` renders the outfit body sprite at scale 0.25 and
rotates it to face aim (`Barns.cpp`), name label upright. Full pose/anim is a
large port — scope it separately.

### T5. True polygon holes in `Graphics`
`AxGraphics::beginHole/endHole` are no-ops; `Map::renderTerrain` uses the canvas
fallback (water first, beach/grass on top). Implement real holes via a
`ClippingNode` stencil or an earcut triangulation in the adapter. Affects the
water/beach island shape and `Renderer::redrawLayerMask` (already approximated).

### T6. Underground ground fade
`Renderer::m_update` fades `ground` via `setAlpha`, but `DrawNode` bakes vertex
alpha, so the node opacity doesn't fade the drawn geometry. Options: rebuild the
ground geometry's vertex colors on fade, use a full-screen tint/`LayerColor`, or
a shader uniform. Currently approximate.

---

## 4. Key files & gotchas
- `src/render/AxmolPixi.h` — adapter; wrappers own native nodes; `addChild`
  reparents; `endFill`/`drawPolygon` force `isconvex=true`.
- `src/render/NullPixi.h` — host-test adapter; mirrors reparenting.
- `src/render/PixiLike.h` — adapter interface (add new methods here + both impls).
- `src/render/Defs.h` — render def provider; extend for new data.
- `tools/codegen_render_defs.mjs` → `src/render/GeneratedDefs.cpp` — regenerate
  when defs change (`node tools/codegen_render_defs.mjs`? check its header).
- `src/render/Renderer.{h,cpp}` — 4 layers, `addPIXIObj`, layer mask.
- `src/game/Map.cpp`, `src/game/GameWorld.cpp`, `src/game/objects/*` — barns.
- `src/app/GameScene.cpp` — wires factory/world/defs; loads atlas here.
- `tools/build-apk.ps1` — must keep packaging `assets/axslc/**` and storing
  `resources.arsc` uncompressed+aligned (Android R+ requirement).
- `Content/` is gitignored; `tools/sync-content.ps1` stages `client/public`.
  `build-apk.ps1` excludes `*.gz`.

## 5. Acceptance for M4
- `surv_tests` pass (add tests for new adapter/def behavior).
- On-device screenshot shows the map **and** objects/players (sprites), with
  buildings occluding/ceiling-fading as in the web client.
- No regressions to the fixes in §1.

---

## 6. Implementation status (this session)

Verified on the x86_64 emulator against the dev server (find_game → join, stable
update stream, no crash):

| Task | Status | Notes |
|---|---|---|
| T1 atlas pipeline | **done** | `tools/build-atlas.mjs` converts the web atlas cache (`client/node_modules/.atlas-cache`) into per-sheet PNG + cocos2d `.plist` under `Content/atlas/` (frame keys are the `*.img` names). `GameScene::loadAtlasesForMap` loads the atlases a map def lists via `SpriteFrameCache`, with `setContentScaleFactor(0.5)` so the low-res sheets render at full size. Logcat shows `Loaded atlas loadout/shared/main`; obstacles/loot/player/particles now draw. |
| T2 buildings | **done** | `MapObjectDef` gained floor/ceiling image defs, `ceilingZoomIn`, vision, `ceilingDestroyResidue`, `occupiedEmitters`, `zIdx`; codegen emits them. `Building` creates its sprites on `isNew`, positions them per frame, applies ceiling fade/`removeOnDamaged`/residue, and drives occupied emitters. |
| T3 particles | **done** | Full `Particle`/`Emitter`/`ParticleBarn` port. `codegen_render_defs.mjs` extracts the `ParticleDefs`/`EmitterDefs` literals from `client/src/objects/particles.ts` (evaluated with the shared utils) and emits 153 particle + 38 emitter defs. Map camera emitter, building occupied emitters and loot xp emitters are wired. |
| T4 full player pose | **not started** | Still the render subset (outfit body sprite rotated to aim + name label). `animData` bones/poses/animations are a large separate port. |
| T5 polygon holes | **done (rect holes)** | `AxGraphics::beginHole/endHole` now subtract axis-aligned hole rects at `endFill` (no poly2tri). `Renderer::redrawLayerMask` uses the real hole path. Polygon holes (water island) keep the equivalent canvas fallback (water first, beach/grass on top). |
| T6 underground fade | **done** | `Renderer` rebuilds the ground rect with the faded vertex alpha instead of `setAlpha` (DrawNode bakes vertex alpha). |

Host tests: 49 pass (`surv_tests`), including new `building_floor_ceiling_render`,
`particles_emitter_spawns`, `renderer_ground_fade`, and expanded
`generated_defs_provider`. The x86_64 `libSurvevMobile.so` builds and links.
(M6 audio adds `test_audio.cpp`; see `M6_AUDIO.md`.)

### Regenerating
```powershell
node --experimental-transform-types tools/codegen_render_defs.mjs   # defs
node tools/build-atlas.mjs --res low                                # sprite atlases -> Content/atlas
```
`Content/` is gitignored, so run `build-atlas.mjs` before packaging the APK
(`build-apk.ps1` ships everything under `Content/`).

### Workflow used in session 4
```powershell
# Host tests
cmake -S tests -B build-tests -G "Visual Studio 17 2022" -A x64
cmake --build build-tests --config Release
.\build-tests\Release\surv_tests.exe          # 63 tests, 1451 assertions

# Device (x86_64 emulator; dev server on :8000/:9001 already running)
.\tools\build-native.ps1 -Abis "x86_64"
.\tools\build-apk.ps1 -Abis "x86_64"
adb install -r .\SurvevMobile-debug.apk
adb reverse tcp:8000 tcp:8000; adb reverse tcp:9000 tcp:9000
adb shell am force-stop com.survev.mobile
adb shell am start -n com.survev.mobile/dev.axmol.app.AppActivity
# drive with: adb shell input swipe 480 300 300 470 400
```

## 7. Remaining rendering implementation (2026-10-07)

- **T4 skeletal pose/outfit rendering implemented** in
  `src/game/objects/PlayerRender.cpp`. The renderer now uses the outfit's
  `skinImg` (not the loot shirt), with body, separate hand/foot containers,
  backpack, chest, helmet, role visor, hip pan, melee, gun/magazine and throwable
  sprites. Equipment ordering, dual weapons, downed/revive hiding, ghillie
  colors, perk armor, player scale, stairs sorting and upright names are handled.
- **Player-local offsets are PIXI y-down.** `animData.ts`/`player.ts` keep bone
  poses and gun/mag/left-hand/held-item offsets in PIXI's y-down local frame;
  the native player body is y-up. `PlayerRender` now converts every y offset
  before use (rotations stay numeric because PIXI and axmol are both
  clockwise-positive), which fixes the left/right hands being swapped across
  the body axis. The Java menu preview uses the y-down convention directly and
  applies `opponentDisplay.ts`'s `dir = (0,-1)` body rotation, so it faces down
  like the web. Covered by `player_hand_side_matches_web`.
- `codegen_render_defs.mjs` imports `client/src/animData.ts` and emits the idle
  poses and animation keyframes/easing, plus item skin/held-image definitions.
  Melee attack/deploy/inspect, cooking/throwing, crawl and revive animations are
  interpolated per frame. Network sequence changes restart animations; repeated
  full updates do not. Partial updates preserve equipment, and pooled players
  reuse their sprites without duplicate children. Shot messages drive gun recoil;
  heal/haste emitter positions follow the player.
  Regeneration is reproducible (including deterministic particle-color
  sampling); two successive runs produce the same generated-file hash.
- **T5 general polygon holes implemented** in `src/render/FillGeometry.h`,
  shared by both adapters. Concave contours and overlapping/clipped holes are
  tessellated directly into convex triangles, without poly2tri. The native map
  uses an actual island hole; canvas retains its overpaint fallback. Rectangular
  layer masks remain supported. Adapter child insertion now updates native
  z-order, and masked nodes retain their clipping wrapper and preserve its
  z-order when masking/reparenting changes.
- **Preserved:** native-node ownership, reparenting, layer clamping, shader APK
  packaging and vertex-alpha underground fade. Container opacity cascades to
  equipment sprites so ceiling-parented particles and render-layer fades work.
  The ground node keeps opacity one so axmol's `u_alpha` does not multiply the
  rebuilt vertex alpha by its old, permanently-zero node opacity.

### Verification

- **60 host tests, 1,430 assertions, zero failures.** Added coverage for polygon
  hole area/coverage, concavity, overlapping/intersecting holes, negative-size
  rectangles, skin versus loot images, gear, pose interpolation, animation
  sequences, pool reuse, throwing/melee, dual recoil, emitters and missing defs.
- x86_64 native library builds/links; APK packages, signs and installs.
- Verified on the x86_64 emulator via API discovery → join: all three map
  atlases load, updates remain stable, and the native crash log is empty.
  Screenshots show terrain/objects and the body/hands player, then the building
  floor revealed after joystick movement into a nearby building.
- Specialized browser-player presentation (e.g. aura UI, submerge overlays,
  frozen/team-patch overlays and animation-triggered collision/audio effects)
  is not part of this skeletal render port. Gear/animation variants are covered
  by host tests; not every variant has been exercised on-device.

## 8. Session 3 — object-render parity pass (2026-10-07, later)

A fresh on-device check of the §7 build showed the world rendering **terrain
and trees only**: buildings, loot and most obstacles were missing and every
obstacle sprite that did draw was oversized. Comparing the ported barns against
the web client found the following gaps; they are now fixed.

### Fixed in this session

- **`Obstacle` was drawing the sprite directly, without the `container` the web
  client uses.** `obstacle.ts` puts the sprite in a `container`, creates it with
  `PIXI.Sprite()` (i.e. **anchor 0,0** until `m_updateData` sets
  `sprite.anchor = 0.5`), gives the sprite an inner scale of `0.8`, and adds
  `-0.5 * width * scale` to the container position. The native port walked the
  sprite at anchor 0.5 and camera scale, so loose obstacles drew at **1/0.8 =
  25 % too large** and the wrong origin. We now bake the container semantics
  into the sprite: `scale = 0.8 * camera.m_pixels(scale * imgScale)`.
- **`Obstacle` sprite was created in `m_updateData` before `isNew` was reached**
  and de-referenced `sprite->` even when `m_getPool()` handed back a recycled
  instance. The sprite is now created lazily on first `render()`, the frame is
  stored in `Obstacle::frame`, and `applyFrame()` sets anchor/image/visibility
  only once the node exists. This was the cause of the **release-only SIGSEGV**
  (`Obstacle::m_updateData` pc `+1525`) observed when the first barrel/obstacle
  arrived.
- **`deadObstacleIds` / `deadCeilingIds` / `solvedPuzzleIds` / `lootDropSfxIds`**
  now live on `Map` (`Map.h`), matching `client/src/map.ts`. Obstacles push to
  `deadObstacleIds` on death and clear the entry on respawn; buildings track
  `deadCeilingIds`/`solvedPuzzleIds`; loot suppresses replayed drop SFX. This
  stops join-time destroy/drop effects from firing for objects that were already
  gone before the client connected.
- **`Obstacle` now ports the rest of `obstacle.ts`**: `randomRotation`
  (`imgRot = deg2rad(__id % 360)`), `imgMirrorX/Y`, the `dead ? 5 : img.zIdx`
  z-order, the `zOrd >= 50 && layer 0` tree/stairs bump, door position/rotation
  interpolation (`pos`/`rot` towards the network target at `15 * scale`), the
  barrel health-smoke emitter (`smoke_barrel`, enabled while `healthT < 0.5`),
  the destroy particle spray, and the `img.residue` swap when destroyed. The
  def provider (`Defs.h` + `codegen_render_defs.mjs` → `GeneratedDefs.cpp`) now
  carries `randomRotation`, `hasExplosion`/`explosionParticle` and
  `doorSlideOffset`.
- **`Loot` now renders like `loot.ts`**: `zOrd 13` (was 10), the container is
  scaled by `camera.m_pixels(imgScale * easeOutElastic(delerp(ticker,0,1),0.75))`
  for the pop-in, `lootRadius`-derived interaction radius, and the sprite inner
  scale of 0.8.
- **Camera interpolation interval** is now measured (`GameWorld::applyUpdate`
  records the wall-clock spacing between `UpdateMsg`s into
  `camera.m_interpInterval`); loot uses it for position interpolation.
- **`Gas` overlay ported** (`src/game/Gas.{h,cpp}`): the screen-covering quad
  with a 512-segment circular hole, scaled by the interpolated safe radius, and
  the `getCircle`/`setProgress`/`setFullState` interpolation from `gas.ts`. It
  renders above the world layers in `GameWorld::attachTo`.

### Resolved in session 5
See §10 (and §9 for the object-render work). These were the outstanding items
listed in the original pass:

- **T4**: the skeletal pose/outfit render in `PlayerRender.cpp` was the
  base + hands/feet + gear subset. Full `animData` bone/pose fidelity and the
  browser-only player presentation (aura UI, submerge, frozen/team patches,
  animation-triggered collision/audio) were added in §10.
- **T5**: the water island now uses a real polygon hole on native (§10).
- Host tests for the object-render changes were added in §9/§10.

### Verification (this session)

- `surv_tests`: **60 tests, 1430 assertions, 0 failures** (unchanged count —
  the new code paths are exercised on-device instead).
- x86_64 `libSurvevMobile.so` builds and links clean; `build-apk.ps1` packages
  and signs the APK; installs on the x86_64 emulator.
- The release-only SIGSEGV in `Obstacle::m_updateData` that a **debug** build
  masked (`addChild` assertion) is fixed. The app now connects, loads the three
  atlases, and renders for a continuous capture with an empty `logcat -b crash`.
- Screenshots show the **beach biome** (sand/water), palm trees, pecans, logs,
  iron-skull obstacles and `saloon`/door sprites, confirming obstacles/sprites
  now render at the correct size and origin.

## 9. Session 4 — obstacle particle lifetime, door casing, gas scale (2026-10-07)

A second parity pass against the web client's `obstacle.ts` and `gas.ts`.

### Fixed in this session

- **Obstacle render now controls its own visibility** (obstacle.ts
  `if (this.sprite.visible && this.img) {...}`). The old port baked the frame in
  `m_updateData` and returned early from the wrong guard, so the sprite drew from
  the first network delta (before `isNew`) and the **destroyed-obstacle particle
  spray and `img.residue` swap fired on the `update` path every frame**. The
  spray moved to `render`, which is the only place the web client runs it.
- **`sprite.zIdx = floor(scale * 1000) * 65535 + __id`** is ported; the sprite
  now carries a stable sort index instead of `__id`. `sprite.zOrd`/`imgAlpha`
  are read from the sprite (explicit `Img` state) rather than the def at render
  time, and dead sprites use `alpha = 0.75`.
- **`img.ori`** (image rotation in degrees) is now emitted by the def codegen and
  folded into `imgRot` together with `randomRotation`; the sprite rotates by
  `-rot + imgRot`.
- **Biome `valueAdjust` tinting**: `map.update` stores the map's `valueAdjust`
  once and obstacles re-tint (`util.adjustValue`) only when it changes, matching
  map.ts/obstacle.ts. Player skins keep their red health tint instead.
- **Player skins** (`isSkin`) now: track `visualPosOld`/`posInterpTicker` and
  interpolate position with the camera interval, keep the higher skin health
  threshold for smoke (`0.3`), and inherit the skinned player's
  `renderLayer`/`renderZOrd`/`renderZIdx` (+262144). `Player` publishes those.
- **Door casing sprite** (`def.door.casingImg`): emitted by codegen and drawn
  behind the door at `closedPos + rotate(casingImg.pos, rot + π/2)`, with its
  own scale/tint/alpha, hidden when dead, z-order `doorZOrd + 1`.
- **Player stairs/world bounds match `GameConfig.player.maxVisualRadius`**
  (the port used a radius of 1.0).
- **Gas overlay world scale**: the gas quad is authored in world units
  (100000), so `GasRenderer::render` now scales by `camera.m_zoom` as well as
  the screen-space safe radius; previously the overlay was ~100000× too large.
- **Animation keyframes with omitted bones** (`noMask`) no longer interpolate a
  bone across a gap — the affected bone falls back to the idle pose, matching
  player.ts's per-frame bone-set semantics.

### Verification (this session)

- `surv_tests`: **63 tests, 1451 assertions, 0 failures.** New tests:
  `obstacle_render_parity` (frame/zIdx/zOrd/scale/randomRotation),
  `obstacle_door_interp_and_casing` (door casing sprite + interpolation) and
  `gas_overlay_world_scale`; `generated_defs_provider` checks door casing defs.
- x86_64 `libSurvevMobile.so` builds, `build-apk.ps1` signs/verifies the APK,
  and it installs and runs on the x86_64 emulator with an **empty crash log**.
- Screenshots show the beach and grass biomes with buildings (floor, walls,
  furniture/counters, toilet, rugs), wooden crates, bushes, boulders, palm
  trees, logs and the player, all at the correct size and origin; walking into
  `saloon` reveals the interior while the ceiling occludes.

### Resolved in session 5
See §10. The three items below were the remaining work at the time of §9 and are
now closed: §10 implements them and keeps their notes for context.

- **T4**: the skeletal pose/outfit render in `PlayerRender.cpp` remains the
  base + hands/feet + gear subset. Bone/pose fidelity for every `animData`
  variant, and the browser-only player presentation (aura UI, submerge,
  frozen/team patches, animation-triggered collision/audio), are unchanged.
- **T5**: the water island keeps the canvas overpaint fallback on native; the
  real-polygon-hole tessellation path is used for rectangular layer masks.
- Per-obstacle **door `spriteAnchor`** is approximated at 0.5 (every shipped
  door uses 0.5); `randomRotation` uses the stable id as source of truth for
  presentation.

## 10. Session 5 — T4 presentation, T5 island hole, door anchor (2026-10-07, later)

Closes the three items carried in §9.

### T4 — browser-only player presentation (`PlayerRender.cpp`, `Barns.h`)

- **Aura UI** (`updateAura`): the medic heal/revive circle is rendered from the
  action item's `aura` def (`GameObjRenderDef::auraSprite/auraTint`, emitted by
  the codegen) or the default `part-aura-circle-01.img`/magenta for revive;
  radius from `GameConfig.player.medicHealRange`/`medicReviveRange` × 0.125.
  It renders under the player container at `zOrd - 1` with the special layer
  visibility rule (`activeLayer & 2 || activeLayer & 1 == 1 ||
  this.layer & 1 == 0`), view-edge fade (`auraViewFade`) and pulsing alpha
  (`easeOutExpo`). Only the active player's `UseItem`/`Revive` action with the
  `aoe_heal` perk shows it.
- **Submersion** (`updateSubmersion` + `Map::getGroundSurface`): a port of
  `map.ts getGroundSurface` (buildings/rivers/terrain) provides the water
  surface + water colour. The player gets the `player-wading-01.img` body
  overlay (alpha `submersion * 0.8`, scale `(0.9 - submersion*0.4)*2`, tinted by
  the water colour) plus downed hand/foot overlays, driven by the shore distance
  (`remap(dist, 0, 16, 0.6, 1)`).
- **Frozen overlay** (`updateFrozenState`): a random `frozenSprites` entry of the
  frozen explosion def (`def.frozenSprites`, now emitted by the codegen) is
  drawn as a `bodyEffectSprite` with frozen-orientation jitter; it fades out over
  0.25 s once the freeze ends.
- **Team patches** (`updateVisuals`): faction-mode players draw
  `player-patch-01/02` (potato variants) rotated `oriToRad(3)+π/2`, tinted by
  `teamColors` (or white in potato mode).
- Bone-level fidelity is complete: every `animData` animation is codegen'd with
  its keyframes, per-keyframe `noMask` and easing; mirrored/omitted bones fall
  back to the idle pose exactly like `player.ts updateAnim`.
- **Animation-triggered effects** are driven from the animation timeline:
  `animPlaySound` (melee/throwable sound keys via the game def),
  `animMeleeCollision` timing, and `animThrowableParticles` (frag pin/lever
  spray).

### T5 — real polygon holes for the water island (`Map::renderTerrain`)

- Native now draws the water as a full-map rect with the **shore contour as a
  real polygon hole** (`beginHole`/`endHole`), and the beach as a ring (shore
  contour minus the grass contour). The grass is drawn last. `FillGeometry`
  hands `AxGraphics` convex triangles, so no poly2tri/CDT is entered and the
  island shape is exact.
- The canvas branch keeps the water/beach/grass overpaint ordering (canvas
  `Graphics` has no holes), matching the web client's canvas mode.

### Door `spriteAnchor` (`obstacle.ts`)

- `def.door.spriteAnchor` is emitted by the codegen and applied to the door
  sprite in `Obstacle::render`, including when the object changes type on pool
  reuse. Shipped doors use `(0.5, 0.5)`, but the non-default path is ported and
  covered by a host test.

### Verification (this session)

- `surv_tests`: **66 tests, 1,485 assertions, 0 failures.** New tests:
  `map_ground_surface_and_island_hole`, `player_aura_submerge_frozen`,
  `obstacle_door_sprite_anchor`.
- x86_64 `libSurvevMobile.so` builds; `build-apk.ps1` signs/verifies; the APK
  installs and runs on the emulator with an **empty crash log**; terrain and
  objects render correctly with the new island hole and presentation overlays.

### Remaining (not defects)

Both items below were implemented in §11.

## 11. Session 6 — animation collision/audio + decals (2026-10-07)

Closes the two §10 "Remaining (not defects)" items.

### Animation-driven melee collision + audio (`player.ts`)

- `animData.ts` animation **effects** are now codegen'd (`PlayerAnimationDef.effects`
  → `AnimEffect` / `AnimEffectFn`): `animPlaySound`, `animMeleeCollision` and
  `animThrowableParticles` (plus the throwable-state `animSetThrowableState`
  marker, which has no runtime side effect because `throwableState` is derived
  each frame).
- `Player::update` tracks `(prevTicker, ticker]` and calls `runAnimEffects`
  once per crossed effect, including the +1 s final-frame window like
  `player.ts`.
- **`animMeleeCollision` is a full port**: it builds the melee attack capsule
  (`getMeleeCollider`, now using `def.attack.offset/rad` emitted by the
  codegen), scans `map.obstaclePool` (with `height`/`collidable`/`isWindow`/
  layer filters, the `cleave` wall-check via `collisionHelpers.intersectSegment`)
  and the player pool (`intersectCircleCircle`, LOS wall check, team priority),
  sorts by `(prio, -pen)`, respects the non-cleave single-hit cap, sprays the
  hit particle (`bloodSplat`/obstacle `hitParticle`) and plays the hit sound
  (`playGroup` for obstacle punch groups, `playSound` for player hits) with
  `channel:"hits"`.
- `animPlaySound` resolves the melee/throwable `sound` table and plays it with
  `channel:"sfx"`, `soundPos`, `fallOff:3`, layer and `muffled` filter.
- `animThrowableParticles` spawns the `fragPin`/`fragLever` spray when the
  throwable def has `useThrowParticles` (now emitted by the codegen).
- New def fields: `MapObjectDef.height/collidable/isWindow/hitParticle/
  punchSound` and `GameObjRenderDef.hasAttack/attackOffset/attackRad/cleave/
  playerHit/punchSound/useThrowParticles`. `Obstacle` stores the transformed
  `collider` and the def flags for the scan. `Player::rad`
  (`scale * GameConfig.player.radius`) and `PlayerConfig.meleeHeight` are
  ported.
- `clamp`-style push/pen coldet helpers (`colliderIntersectCircleRes`,
  `colliderIntersectSegmentRes`, segment distance) were added to
  `core/Collider.h`.

### Decals (`decal.ts`) + `getGroundSurface`

- **Full `Decal`/`DecalRender`/`DecalBarn` port** in `Barns.{h,cpp}`: the pooled
  decal object transforms its collider from the def, copies the `surface`, and
  allocates a pooled `DecalRender` on `isNew`; the render node draws the sprite
  (`camera.m_pointToScreen`, `camera.m_pixels`, `-rot`, water alpha ×0.3, fade
  alpha), supports `flicker` on light decals and fades out over `lifetime`.
- Decals register as `ObjectType_Decal` in `GameWorld`, update in
  `GameWorld::update` before the renderer flush, and `Map` gets the barn via
  `setDecalBarn` (matching `map.ts` constructor injection).
- **`Map::getGroundSurface` checks decals first** (after `util.sameLayer` +
  `collider.intersectCircle`), returning the decal `surface` water colour with
  the gore fade (`decal.ts update` → `surface.data.waterColor/rippleColor`),
  so `player-wading` submersion in the bathhouse pool now uses the decal water.
- The codegen emits decal `surface`, `gore` (fade fields + optional
  `tint/alpha/waterColor/rippleColor`), `img.flicker*`, `img.ignoreAdjust`,
  `lifetime` and `height`. `util.lerpColor` (sRGB↔linear) is ported in
  `Barns.cpp`.

### Verification (this session)

- `surv_tests`: **67 tests, 1,512 assertions, 0 failures.** New test
  `player_anim_effects_and_decals` covers the codegen'd animation effects
  (`fists` swing + melee-collision timing), the melee attack capsule defs
  (`woodaxe` offset/rad, `cleave`), the obstacle collision/impact defs
  (`barrel_01` height/collidable/`barrelChip`/`barrel_bullet`), the
  `decal_bathhouse_pool_01` surface/gore defs, and the decal-first
  `Map::getGroundSurface` water colour (with the terrain surface unchanged
  outside the pool).
- x86_64 `libSurvevMobile.so` builds and links clean.



---

## 12. Session 7 - join crash fixed (2026-10-20)

Starting the app and pressing **Play Solo** killed the process every time.
Three distinct native crashes were found in sequence; the first is fixed, the
other two share one remaining bug.

### 12.1 `SIGSEGV` in `GameWorld::loadMap` (fixed)

```
#00 GameWorld::loadMap+33        mov 0x3b8(%rdi), %r15   (fault 0x3b8 = nullptr+_creator)
#01 GameScene::init()::lambda    (the map callback)
```

`GameScene::init()` leaves `_world` null and the map callback dereferenced it.
The world was only built by `resetWorld()` when `_inGame` was already true,
but `enterWithJoin()` opened the socket (`tryJoinUrls()` pumps frames) *before*
any later `resetWorld()` could run, so the first match's MapMsg always reached
a null world.

Fix:
- `enterWithJoin()` now calls `resetWorld()` **before** `tryJoinUrls()`, so the
  world exists when the first MapMsg arrives (and it is even loaded at once by
  `Game::hasMap()/map()` → `GameWorld::replayMap()` when the server answered
  before the socket opened).
- The map/update callbacks still guard `_world` (frames can arrive between
  joins / after `leaveGame()`).
- `GameWorld::setScreenSize()` no longer requires `mapLoaded()`: the layers are
  created before the map, so terrain would otherwise have rendered at the
  default camera size. `GameScene::update()` now re-applies the real viewport
  size once the GL view exists (`_worldSized/_worldSizeDirty`).
- `leaveGame()` hides `_worldRoot` (game.ts `free()` hides the canvas).
- Host tests: `tests/test_join_map.cpp`
  (`game_map_without_world_callback`, `gameworld_replay_cached_map`).

### 12.2 / 12.3 FairyGUI widgets freed after the frame that built them (root cause found, per-frame use still disabled)

Symptoms: `SIGSEGV` in `fairygui::GObject::setVisible()` from
`FguiHud::setVisible`/`refreshHealth`, and in `GObject::setPosition()` from
`Touch::m_update` → `FguiPadGraphics`. The fault address is an unmapped heap
page (`SEGV_ACCERR`), i.e. the object itself was freed.

Root cause: FairyGUI nodes are ref-counted (`CREATE_FUNC` → `autorelease()`)
and `GComponent::addChild()`/`GRoot::addChild()` do **not** retain their
children. The engine's `Object::release()` only runs the destructor when the
reference count is exactly `1`, so an autoreleased widget that was never
retained has no valid owner: it is destroyed when the frame's autorelease pool
drains (`GRoot`/`FguiHud` then keep raw dangling pointers). An `ax::Node`
probe survives the same frame (retain count 2), so this is specific to how the
FGUI widgets are created.

Fixes applied:
- `FguiHud`: `addChild()` re-parents the `create()` reference into the HUD
  root, and `keep()` retains objects that stay detached (the minimap frame);
  the HUD root is owned by the member that holds it.
- `FguiEmoteWheel`: owns its container (`retain()` in `build()`, released in
  the destructor); wedges are children of it.
- `FguiPadGraphics`: owns its ring/dot (`retain()` in `ensureFx()`, released
  in the destructor); the dot is raised above the ring in the `GRoot` order.

Still open: with the per-frame writes enabled the app still faults as soon as
a widget is touched, so `FguiHud::update()` is gated off and
`GameScene::update()` no longer shows the HUD or feeds the pads (both marked
`TEMP`, with the reason inline). The ownership change above is kept because it
is correct on its own; the remaining lifetime problem is in the FairyGUI
object stack and still needs a fix (build with ASan on an arm64 image, or
mirror what `central-package` does with its widgets).

Bisect results (x86_64 RP build, one tap at a time):
- Bootstrap verified: the `GGraph`s created with the hand-rolled ownership in
  `FguiPadGraphics` - which are **not** attached anymore (all pad calls are
  gated) - stayed alive; `isVisible()` on `_healthFill` also reads fine.
- Failure point: the **first write** to a HUD widget after layout faults in
  `GObject::setVisible()` (`fault 0x88`, i.e. `_parent->setBoundsChangedFlag()`
  on a deleted parent, or the object itself in an unmapped page).
- The `GRoot` is alive with an empty child list, so the whole HUD tree was
  detached/destroyed once. Neither `GRoot::onExit()` nor a second `GRoot` is
  involved (`FairyGUI GRoot created` logs once).
- The pads crash even untouched: `enterWithJoin()` -> `_movePad->setVisible()`
  faults in the same function, so the corruption happens before the first
  match frame.

Next suspect: the FGUI display chain (`FUIContainer`/`FUIInnerContainer`),
i.e. the axmol-node `CCNode::removeFromParent()`/`_running` handling or the
node-ownership changes in `AxmolPixi.h`; that needs an ASan arm64 run.

Note: the `AppDelegate.cpp` simplification from the previous pass (no C++
`MenuScene`, bridge without `_menu`) was **reverted** - the bridge dereferences
the menu for config/region, which crashed in `ui::Config::setString` on Play.

### 12.4 `AxPixiFactory` teardown use-after-free (known, still open)

`AxPixiFactory::~AxPixiFactory()` destroys its `_nodes`; an `AxNodeImpl`
destructor walks its parent's `_children`, so parents destroyed earlier in the
same batch leave dangling pointers. Fixed in two steps: the factory now pops
its node list back-to-front (children were created/re-parented after their
parents) and `GameScene::resetWorld()` destroys the factory only through
`_pixiFactory.reset()`. This is why `resetWorld()` keeps its `if (_inGame)`
guard (the factory is only replaced on a join, never in `init()`).

Still open: the **second** join (menu -> match 1 -> back -> match 2) still
faults in `AxNodeImpl<Container>::removeChild` from `AxPixiFactory::~...`: a
sprite is destroyed whose parent wrapper is already gone (the parents are
destroyed later in the back-to-front order, so the sprite's `_parentNode`
points at a live parent whose `_children` no longer holds it, or at a dead
one). Needs the parent/child ownership in `AxNodeImpl` reworked
(`removeFromParent()` in the destructor, as in the pre-"game play changes"
version, was removed in the last commit - that is the likely trigger).

### Verification

- `surv_tests`: **105 tests, 1,903 assertions, 0 failures.**
- On device (x86_64 emulator + bundle server via `adb reverse`): launch, Java
  menu, `Play Solo` -> `find_game` -> connect -> `Map` + `Update` handled, match
  world renders, pause and back to the menu both work, process stays alive;
  the HUD/pad writes are still gated off (12.2) and the second join hits the
  `AxPixiFactory` teardown above.

## 13. Session 8 - world camera, HUD and pads (2026-10-22)

After the last few UI commits a join rendered a flat water/background split and
no HUD/pad. Three independent regressions were found and fixed.

### 13.1 World camera never scrolled (`Renderer::applyCamera`)

`applyCamera()` set the content container to `camera.m_pointToScreen(m_pos)`,
which is always the viewport centre, and then scaled world-space children by
`m_z()`. The world origin therefore stayed pinned to the centre: at the default
zoom the viewport showed only the map corner (water rect + background surround),
which looked like a flat two-colour screen. `map.ts` puts the container at
`p0 = camera.m_pointToScreen(0, 0)`, so applyCamera now uses world (0,0). The gas
overlay moved under the same camera-transformed container (world-space geometry)
instead of being transformed separately.

The chunked terrain workaround from the previous pass was removed: it was
diagnosing this camera bug (the "only the water rect survived" symptom), and it
re-drew the full terrain 64 times with a white outside band. The map is back to
one `display.ground` node like `map.ts`; `AxGraphics`/`DrawNode` has no buffer
cap. The hole/clip branch in `buildGroundChunks` was also a no-op (a
`beginHole()` without an open fill).

### 13.2 Active-player view radius never reached the camera

`game.ts` applies `msg.activePlayerData.zoom` to the active player
(`m_setLocalData`) each update. The port only read `ObjectData.zoom`, so
`Player::zoom` stayed 0, `getZoom()` returned 1, and the camera settled at
~40x instead of the 1x-scope radius 32 (m_z ~20). `GameWorld::applyUpdate()` now
applies `activePlayerData.zoom` when `zoomDirty`.

### 13.3 HUD was vertically mirrored and the pads were off-screen

`Hud` laid out in CSS coordinates (y down) but the HUD container is a positive-
scale axmol node (y up), so the health bar appeared at the top. The layout is
now done in design-space bottom-left coordinates and converted to the centred
container; `AxText` also applies its anchor to the glyph sprite (previously only
the empty wrapper node, so every anchored label was top-left). `AxText` now
rasterises at 2x and scales the sprite back down: the app's 0.5 content scale
otherwise produced half-resolution HUD glyphs, which looked blurry when the
design root was fitted to the screen. (axmol's global SDF option does not apply
here because `AxText` bypasses `FontFreeType`/`Label`.)

Pads: `Touch` was fed axmol touch points (y up) as if they were web points
(y down) - the listener now flips y. `GameScene` also passed the player's view
*radius* (32) to `Touch::setPadOffsetScale`, which multiplied the locked pad
offsets to ~(4032, 3200) and moved both pads off-screen; that call is gone (the
web client does not scale pad offsets with zoom). `FguiPadGraphics` is replaced
by `PadGraphicsAxmol`: two ordinary axmol nodes on the game root (`gui/pad.png`
ring + a DrawNode touch dot), so no FairyGUI lifetime issue.

### Verification

- `surv_tests`: **109 tests, 1,924 assertions, 0 failures.** New/renamed:
  `map_ground_covers_island` (terrain tessellation covers the island centroid)
  and the corrected `renderer_world_root_transform` / HUD position expectations.
- x86_64 native lib + APK build, sign, install and run on the emulator against
  the local `pnpm dev` API (`adb reverse` 8000/9000/9001). `Play Solo` joins,
  the terrain scrolls with the camera, the health bar sits bottom-centre, the
  minimap frame sits top-right, both joystick rings/dots render, and a swipe on
  the move pad scrolls the world (`logcat -b crash` empty).

## 14. Session 9 - per-match world rebuild, back to the menu (2026-10-22)

Closes §12.4 (second join) and the missing match-end notification that made it
impossible to reach the menu again.

### 14.1 Second join double-free (`GameScene::resetWorld`)

The previous pass had already fixed the factory side (`AxPixiFactory::clearNodes`
deletes the wrappers while its own list is alive, and `~AxNodeImpl` no longer
walks a dead parent). The remaining crash was **double ownership of the two
scene roots**: `_worldRoot`/`_hudRoot` were `unique_ptr<pix::Container>` *and*
every adapter node is owned by the factory.

On a second join `resetWorld()` replaced `_pixiFactory` (whose `clearNodes()`
deleted the old root wrappers) and then called `_worldRoot.reset(newRoot)`,
which deleted the already-freed old wrapper - a use-after-free that surfaced as
`removeChild`/`setVisible` faults. That is why the function still early-returned
when `_world` existed ("TEMP" in §12.2).

Fix (`src/app/GameScene.h`, `GameScene.cpp`):
- `_worldRoot`/`_hudRoot` are now raw `pix::Container*` handles owned by the
  factory; GameScene never deletes them.
- `resetWorld()` tears down in dependency order: detach the root's axmol node
  from `_gameRoot`, drop `_hud`/`_world` (they only hold raw node pointers),
  null the handles, replace the factory (which frees the previous match's whole
  adapter tree), then build the new world/HUD.
- The `if (_world) return;` TEMP guard is gone, so every join gets a fresh
  `GameWorld` + renderer + HUD like game.ts `free()` + re-init.

### 14.2 Match end never reached the Java menu

`GameScene::onMatchEnded` was only subscribed by the C++ `MenuScene`, which the
Android build no longer creates (AppDelegate runs `GameScene` directly). The
Java fragment hidden by `enterGame()` was therefore never shown again: hardware
back / quit left a black canvas with no way to start a second match.

`AppDelegate` now wires `gameScene->onMatchEnded` to
`uiHost()->returnToMenu(errorL10nKey)` (the `SurvBridge.onNativeMatchEnded`
path), so a clean exit, a socket close and a failed join all restore the menu.

### Verification

- `surv_tests`: **109 tests, 1,924 assertions, 0 failures.**
- x86_64 native lib + APK, installed on the emulator against the bundle API
  (`adb reverse` 8000/8001).
- Three consecutive **Play Solo** joins with **Back** between them: each join
  got a new `activePlayerId` (4512 → 4513 → 4514), terrain/minimap/HUD rendered
  every time, the Java menu came back after each back press, and
  `logcat -b crash` stayed empty.

## 15. Session 10 - object sprites were 16x too large (2026-10-22, later)

Symptom: after a join the map rendered but **no objects or player** were
visible, and the world looked far more zoomed in than the web client. The HUD
(plain pix nodes on `_gameRoot`) was fine.

### 15.1 Root cause: screen-space scales under a world-space camera

`applyCamera()` puts the camera on the content container (`scale = m_z() =
m_ppu * m_zoom`) and the port keeps every object in **world units**. The web
client instead projects each object to screen space and scales it with
`camera.m_pixels(x) = x * m_zoom` (see `obstacle.ts`, `player.ts`, `loot.ts`,
`deadBody.ts`, `projectile.ts`, `flare.ts`, `plane.ts`, `particles.ts`,
`emote.ts`).

The §13 refactor moved the *positions* into the world container but kept those
screen-space scale factors, so the container multiplied them by `m_ppu` (16)
again. Every world sprite was 16x too large: obstacles/player covered the
viewport (the visible "white shirt" blobs) or were clipped away, so the map
looked empty and over-magnified. HUD/terrain were correct because they are not
`m_pixels()`-sized sprites.

### 15.2 Fix - divide world-space sprite scales by `m_ppu`

`worldScale = webScreenScale / m_ppu` (and the sizes assembled as
`rad / m_ppu` need `m_ppu^2`):

- `Obstacle::update` sprite + door casing (was `scale * imgScale`)
- `Building` images (`scale * img.defScale`)
- `Loot` container (`imgScale * scale`)
- `DeadBody` container (`m_pixels(1)` -> `1/m_ppu`)
- `Projectile` container - also restored the missing `imgScale * remap(posZ,
  0, maxHeight, 1, 4.75)` factor
- `Player`/aura containers (`m_pixels(1)` -> `1/m_ppu`); body/bone sprites keep
  their relative 0.25/0.175/... scales
- `Flare` container (`1/m_ppu`, flare factor on the flare sprite)
- particles (`camera.m_pixels(scale)`)
- smoke/airdrop/plane (`m_pixels(rad / m_ppu)` -> `rad / m_ppu^2`)
- world emotes and team ping indicators; the ping indicators still used
  `m_pointToScreen`/`m_pixels` after §13, so their positions/scales are now
  converted with `m_screenToPoint()` and `/m_ppu`

`AxSprite::setFrame` now logs a throttled `[pix] sprite frame missing` warning
(frames are registered by the per-map atlas load; a none/missing frame silently
drew nothing before).

### Verification

- `surv_tests`: **109 tests, 1,924 assertions, 0 failures** (the obstacle scale
  expectation now includes `/ m_ppu`).
- x86_64 native lib + APK installed on the emulator against the bundle server.
  On join: terrain, roads, trees, rocks, crates, doors, loot, the player sprite
  and its name render at the web's scale; both pads and the live minimap render;
  the player moves with the joystick (world pos 388.7,667.4 -> 396.1,666.1); no
  crash.
- Fixed in §16: the camera transform is a negative-Y scale, so world-space
  **text** (player/dead-body name labels) rendered vertically mirrored.
  `pix::Text::setFlipY(true)` now counter-flips the glyph sprite about its
  anchor (the anchor point is preserved, so placement is unchanged).


---

## 16. Session 11 - emote wheel, emote defs, pad scale, upright labels (2026-10-22)

The next Priority-2 gap from `REMAININGS.md` was the emote wheel. It is now
implemented end to end for touch, along with three defects found while verifying
on the emulator.

### 16.1 Emote defs are codegen'd (`EmoteRenderDef`)

`shared/defs/gameObjects/emoteDefs.ts` is now part of
`tools/codegen_render_defs.mjs`: `Defs.h` gains `EmoteRenderDef`
(texture/sound/channel/name/teamOnly/category) and `DefProvider::emote(type)`.
`EmoteManager::addEmote` uses the def's atlas frame and sound/channel; before
this every world emote drew the same `emote-circle-outer.img` placeholder.

Regenerate with:

```sh
node --experimental-transform-types tools/codegen_render_defs.mjs
```

The emote frames live in the `loadout` atlas, which every map's atlas list
already includes.

### 16.2 Emote wheel (`game/EmoteWheel.{h,cpp}`)

Engine-independent (pix adapter only, host-tested with `NullPixi`), in design
space next to `game/Hud`:

- On-screen emote button (`#ui-emote-button`) one wheel radius in from the
  right edge, so the wheel it opens stays fully on screen.
- Four wedges in `gameConfig.ts` `EmoteSlot` order (top/right/bottom/left),
  each showing the loadout emote's atlas icon and name; coloured slot rings and
  a centre close icon.
- Touch on the button opens the wheel around it and claims that touch id, so
  the pads do not also move/aim. Dragging past `distMinLength` (35 design px)
  highlights a wedge and sends immediately, like `emote.ts`'s touch path.
- `teamOnly` emotes are disabled in solo (`TeamMode.Solo == 1`).
- Throttle mirrors `emote.ts`: soft every 2 s*1.5, hard 9 s after the 6th send;
  the wheel greys out during the hard cooldown.

Selections are queued on `EmoteManager::queueEmote`; `GameScene` forwards the
queue to `Game::sendEmote` each frame (the `game.ts` newEmotes loop), so the
world emote comes back through the server like a remote player's.

### 16.3 Touch pads were 4x too small

`PadGraphicsAxmol` used `kPadSpriteScale = 0.0625` against the 832 px
`gui/pad.png`, i.e. a ~31 px ring. `touch.ts` sizes the ring as `pad.img`
(208 px) * `padScaleBase * padScaleDown` = 124.8 px, so the multiplier must be
`124.8 / (0.6 * 832) = 0.25`; the touch dot likewise needs a 26 px radius at
scale 0.25 (`kDotBaseRadius = 104`). Both pads now match the web's footprint.

### 16.4 World-space text was vertically mirrored (closes §15's follow-up)

> **Superseded by §23:** the camera root is now positive-Y, so world labels are
> upright without a counter-flip. This section records the earlier state only.

The camera root carries `scale(s, -s)`, which mirrors y-up glyphs (sprite art
is authored y-down, text is not). `pix::Text::setFlipY(true)` draws the glyph
sprite with `scaleY = -1`; because the flip happens about the anchor line, the
anchor position is unchanged. Player and dead-body name labels enable it;
HUD/wheel text stays positive-scale.

### 16.5 Crosshair frame never existed

`GameScene` requested `crosshair000.img`, which is not an atlas frame (the
default crosshair is inline SVG in `crosshairDefs.ts`), so the crosshair logged
`[pix] sprite frame missing` and drew nothing. It is now two white bars drawn
with `pix::Graphics` at the screen centre (64 px art: 32 px long, 4 px thick).

### Verification

- `surv_tests`: **116 tests, 1,979 assertions, 0 failures** (new:
  `emote_defs_provider`, `emote_wheel_wedge_geometry`,
  `emote_wheel_button_and_loadout`, `emote_wheel_selects_and_closes`,
  `emote_wheel_team_only_wedges_are_disabled_in_solo`,
  `emote_wheel_throttle_matches_web_cooldowns`, `emote_wheel_opens_and_updates`;
  `player_skin_and_equipment` now asserts the name label counters the flip).
- x86_64 native lib + APK installed on the emulator against the bundle server:
  `Play Solo` joins, pads render at the web's size, the player name reads
  upright, the emote button shows the first loadout emote, holding it opens the
  wheel with the four loadout icons, and dragging up sends `Happy Face` which
  the server echoes above the player (screenshot-verified). `logcat -b crash`
  empty.

### Still open from this area (as of session 11)

- ~~Team-ping wheel input, ammo-specific ping emotes~~ — closed in §17.
- Minimap ping sprites (closed in §18; the minimap is now baked and displayed).
- ~~Crosshair style/colour from the loadout (only the default style is drawn)~~ -
  closed in §32.
- ~~`ui/FguiHud.*` remains unused~~ — removed in §65 (the live HUD is `game/Hud`).

---

## 17. Session 12 - team-ping wheel, ping defs, indicator fade bug (2026-10-23)

Closes the next Priority-2 gap from `REMAININGS.md` §3: the team-ping wheel.
The web client opens it from the `TeamPingMenu` bind (mouse/keyboard) or via
the big map on touch; the native touch client now has its own button next to
the emote button and sends at the crosshair.

### 17.1 Ping defs are codegen'd (`PingRenderDef`)

`shared/defs/gameObjects/pingDefs.ts` is now emitted by
`tools/codegen_render_defs.mjs` alongside the emote table. `Defs.h` gains
`PingRenderDef` (texture/mapTexture/sound/soundLeader/pingMap/pingLife/
mapLife/worldDisplay/mapEvent/tint) and `DefProvider::ping(type)`.

`EmoteManager::addPing` is now data-driven:

- Indicator slot mirrors `emote.ts addPing`: airdrop/airstrike use slots 4/5;
  team pings use the sender's index in our sorted group member list; pings
  from other groups play their sound but draw no indicator.
- Group membership comes from `UpdateMsg.playerInfos` (`GameWorld` now keeps
  `playerId -> groupId`, sorted like `playerBarn.getGroupInfo`).
- Sprites use the def's world texture; the ring/edge indicator use the group
  colour (`GameConfig.groupColors`) or the def tint for map events; the sound
  is the def's `sound` on the `ui` channel.
- Our own edge-of-screen indicator is hidden unless the ping is a map event.

### 17.2 Team-ping wheel (`game/PingWheel.{h,cpp}`)

Engine-independent (pix adapter only, host-tested with `NullPixi`), design
space next to `game/EmoteWheel`:

- On-screen ping button one 56 px gap left of the emote button; the wheel opens
  around it so it stays fully on screen.
- Six selectors in `emote.ts teamPingData` order and sector bounds
  (`isAngleBetween` is ported verbatim): middle close, top `ping_danger`, right
  `ping_coming`, bottom `ping_help`, bottom-left `emote_medical`, top-left
  `emote_ammo*`. Labels/icons come from the ping/emote defs.
- Ammo selector follows the equipped weapon: `ammoType` from
  `GameObjRenderDef` → `emote.ts AmmoTypeToEmote`
  (`9mm`→`emote_ammo9mm`, ..., fallback `emote_ammo`).
- `teamOnly` ammo/medical emotes are disabled in solo (`TeamMode.Solo`); the
  whole wheel greys during the shared emote hard cooldown (the counter lives in
  `EmoteWheel`, and pings call `incrementEmote()` like the web).
- `GameScene` routes the button touch id away from the pads, queues
  `EmoteManager::queuePing/queueEmote`, and forwards the queue to
  `Game::sendEmote` (the `game.ts newPings/newEmotes` loop already existed).

### 17.3 Bug found on device: reused indicators inherited fade-out alpha

The first ping on a slot drew correctly, but a ping arriving after that slot had
fully faded out was nearly invisible: `EmoteManager` set the indicator container
alpha to `fadeOut / kPingFadeOut` (→0) and `addPing` never reset it. `addPing`
now restores container/sprite alpha to 1 and the edge indicators to 0, and the
fade-in branch re-asserts container alpha 1. Host test:
`ping_manager_resets_indicator_alpha_on_reuse`.

### Verification

- `surv_tests`: **122 tests, 2,062 assertions, 0 failures** (new:
  `ping_defs_provider`, `ping_wheel_selector_sectors`,
  `ping_wheel_button_and_ammo_mapping`,
  `ping_wheel_selects_pings_and_team_emotes`,
  `ping_wheel_solo_gating_and_throttle_grey`,
  `ping_manager_resets_indicator_alpha_on_reuse`).
- x86_64 native lib + APK installed on the emulator against the bundle server:
  `Play Solo` joins; the ping button shows the danger icon next to the emote
  button; holding it opens the six-selector wheel (medical/ammo greyed in solo);
  dragging up/right/down sends `danger`/`coming`/`help`, and the server echo
  draws the matching icon with the yellow group ring at the crosshair — including
  a ping sent after the slot had faded out (alpha fix verified). Dragging to the
  greyed medical wedge in solo sends nothing. `logcat -b crash` empty.

### Still open from this area

- ~~**Minimap ping sprites**~~ - closed in §18 (the minimap is baked and the pings
  are drawn over it).
- ~~Airstrike ping falloff in non-faction mode (`emote.ts rangeMult: 20`)~~ -
  closed in §31.
- ~~Faction-leader ping sound (`pingDef.soundLeader`)~~ - closed in §31
  (`GameWorld` now exposes a `PlayerStatus.role` lookup).

---

## 18. Session 13 - live minimap + map ping sprites (2026-10-23)

Closes the minimap blocker from §17 and finishes the emote/ping feature set.

### 18.1 RenderTexture adapter gaps

The port already had `pix::RenderTexture`/`pix::Renderer`, but nothing had ever
baked or displayed one. Two axmol details were required:

- `AxRenderTexture::resize()` must **recreate** the framebuffer
  (`ax::RenderTexture::create(w,h)`); axmol's `setContentSize` leaves the
  texture at its constructor size (1×1 here), so every bake came out blank.
- `AxSprite::setRenderTexture()` uses `Sprite::setTexture()` (a `SpriteFrame`
  is not created for RenderTextures and `setSpriteFrame(nullptr)` crashes) and
  must set the **texture rect** to the target size, otherwise the quad stays
  zero-sized and draws nothing.

`Factory::createRenderer()` was added (Ax/Null implementations) and
`Sprite::setRenderTexture` / `RenderTexture::width()/height()` joined
`PixiLike.h`.

### 18.2 Minimap bake (`Map::generateMinimapTexture`)

Ports `map.ts renderMap()` into a 256×256 texture:

- Terrain is rendered at world scale into a node flipped around `y = height`
  (`position.y = height; scale.y = -1`), with the grid thickness set to
  `1 / mapScale` world units so it lands on ~1 texture pixel, plus the 2px map
  border.
- Obstacles/buildings use `MapObjectDef::mapShapes` (or the collision/bounding
  fallback) through `colliderTransform(def collider, pos, oriToRad(ori),
  objScale)`, sorted by `zIdx` (`def.img.zIdx` for obstacles, `750 + zIdx` for
  buildings). The old code double-applied `shape.scale` and mixed texture-space
  and world-space coordinates; both are fixed.
- The whole graph is scaled by `256 / max(width, height)` and rendered by the
  factory's `pix::Renderer`.

`Hud::setMinimap()` displays the texture under the top-right frame (scale
`kMinimapSize / 256`), called from the MapMsg callback after `loadMap`.

### 18.3 Minimap ping sprites

`EmoteManager` now keeps `MapPing` entries (the `ui.ts createPing` data):

- Every ping spawns the def's `mapTexture` icon (`mapLife`) plus a
  `ping-map-pulse.img` ring (`pingLife`, growing at `dt / 2.5`).
- Player pings cancel that player's previous map sprites; map events
  (airdrop/airstrike) stack. Tint is the group colour for our group and the
  def tint for map events.
- `Hud::updateMapPings()` draws them in `_mapPingLayer`, anchored at the
  minimap corner and **masked to the 140×140 frame** (the 512px pulse art
  otherwise spills across the screen), with the `mapSprite.ts` 0.1s fade-in /
  0.5s fade-out.

The world ping indicator also had two fixes while verifying: map-event pings
apply their `worldDisplay` visibility immediately (a doomed world sprite could
flash for a frame), and the indicator scales divide by `camera.m_z()` (the
camera root's actual scale) instead of `m_ppu`.

### Verification

- `surv_tests`: **125 tests, 2,098 assertions, 0 failures** (new:
  `map_minimap_texture_is_baked`, `map_pings_spawn_on_minimap_and_expire`,
  `hud_minimap_positions_ping_sprites`).
- x86_64 native lib + APK on the emulator: the top-right frame now shows the
  baked map (water/beach/grass, rivers, buildings, grid); a locally injected
  map-event + player ping (the dev server in this session stopped echoing
  emotes entirely, including `emote_happyface`) drew the clipped icon/pulse in
  the minimap and the world indicator without spilling. `logcat -b crash` empty.
- Server echo of pings/emotes could not be re-verified after the mid-session
  server restart (it also dropped ordinary emotes); the client-side add/send
  path and the host tests are green.

### Still open

- Player/teammate dots and the gas ring on the minimap (`ui.ts` player map
  sprites / `getMinimapRender` name labels).
- Big map (full-screen) view.
- ~~Airstrike ping falloff and faction-leader ping sounds~~ - closed in §31.

---

## 19. Session 14 - minimap dots/gas + big map (2026-10-23)

Finished the map UI on top of §18's baked minimap.

### 19.1 Shared `MapView` (`game/MapView.{h,cpp}`)

One clipped square view of the whole map, used by the 140px minimap and the
640px big map:

- The baked texture sprite plus gas, dots and pings, projected from world
  units (`project`/`unproject`), with `contains()` for hit tests.
- `MapDot` mirrors `ui.ts updatePlayerMapSprites` (pos/scale/tint/texture/
  alpha/z), `MapGas` mirrors `ui.ts m_render` (current + safe circle, player
  line).
- The ping layer is created lazily and masked to the square (the 512px pulse
  art grows past it). **A ClippingNode created during `Hud::build` crashed
  axmol's GL renderer when the HUD became visible; creating the same mask from
  the first `setPings()` call is safe.** Also `clear()` the gas DrawNode at
  creation so a contentless DrawNode never submits an empty command.
- `setIndicators()` draws `UpdateMsg.mapIndicators` (mapIndicator.ts): the def's
  `mapIndicator.sprite` tinted `mapIndicator.tint` at its world position, plus
  the `part-pulse-01.img` ring (def `mapIndicator.pulse`, tint `pulseTint`,
  scale from `MapIndicatorBarn`'s 0.5..1 ticker) for the equipped holder. They
  share the sorted dot layer: equipped markers sort above the player dots
  (`MapView::indicatorZOrder` 655350 vs 1, ring one lower). The frames live in
  the always-resident `loadout` atlas.

### 19.2 Player/team dots + gas

`GameScene` builds the dots each frame from `Game::allPlayerInfo` +
`allPlayerStatus` (the web's `playerBarn.playerStatus`):

- Own dot is drawn from the live active `Player` position, with the
  `player-map-outer.img` ring and `player-map-inner.img` dot (group colour).
- Other statuses use `status.visible`: `skull-outlined.img` when dead,
  `player-group-downed.img` / `player-map-downed.img` when downed, team colour
  outside our group (`GameConfig.teamColors`), group colour inside it
  (`GameConfig.groupColors` by sorted group index).
- Gas: `Gas::getCircle(1)` fills the map red with a circular hole (the web's
  GasRenderer), `circleNew` draws the white safe-zone circle, and a line runs
  from the player to it.

### 19.3 Big map

- A small expand affordance sits in the minimap's bottom-right corner
  (`#ui-map-expand-mobile`); tapping it opens a 640×640 centred map over a dim
  backdrop, hiding the minimap/expand button.
- Same texture, dots, gas and pings as the minimap (`Hud::updateMapOverlay` /
  `updateMapPings` push to both views).
- Closes from the X button or by tapping outside the map; `GameScene` swallows
  the whole touch so it never moves/fires. Hit tests convert the design-space
  touch into the centred HUD space (they previously compared the wrong spaces,
  so the expand tap did nothing).

### Verification

- `surv_tests`: **127 tests, 2,124 assertions, 0 failures** (new:
  `map_view_dots_gas_and_pings`, `hud_big_map_toggle_and_hits`; the minimap
  ping/dot test now goes through `MapView`).
- x86_64 native lib + APK on the emulator: minimap shows the baked map, player
  dot and expand button (plus the white safe-zone ring when gas is active); the
  expand tap opens the big map with the dot, terrain and X; both the X and a
  tap outside close it and restore the minimap. `logcat -b crash` empty.

### Still open

- Role-specific map icons (`roleDef.mapIcon` alive/dead) — the role def table
  is not codegen'd into the native render defs.
- Big-map tap-to-ping (web `bigmapCollision` → team ping at the tapped map
  position), and airstrike zone drawing on the map.
- Map place name labels in the baked texture (`map.ts renderMap` nameContainer).

---

## 20. Session 15 - map airstrikes, place names, big-map tap-to-ping (2026-10-23)

### 20.1 Airstrike warning zones

`MapView` gained a second Graphics layer (`setAirstrikes`) mirroring
`plane.ts AirstrikeZone.render`: a yellow `#eaff00` circle with 20% fill and a
1.5px outline, faded in/out over 0.5s. `GameScene` builds the list from
`PlaneBarn::airstrikeZones` (active zones + `ticker`/`duration`) and passes it
through `Hud::updateMapOverlay`, so the warning shows on both the minimap and
the big map.

### 20.2 Place names

`MapView::setLabels` ports `map.ts renderMap`'s nameContainer. `Hud::setMapLabels`
is called once from the MapMsg callback (`msg.places`).

- **MapMsg place positions are normalized (0..1)**, unlike every other map
  position; the label is placed at `(pos.x * size, pos.y * size)` directly.
- Labels are HUD `pix::Text` over the map, sized `14 * size/256` (7px on the
  minimap, 35px on the big map), under the dots and over the gas. Rendering the
  text into the RenderTexture itself did not appear on device, so this keeps
  the same visual result through the proven text path.

### 20.3 Big-map tap-to-ping

Ports `ui.ts bigmapCollision` + `triggerPing`:

- Tapping inside the big map converts the design-space tap through
  `MapView::unproject`, clamps it to the map bounds and stores it as
  `_pingTarget`; the big map closes and the team-ping wheel opens at the screen
  centre (the web's `emoteScreenPos`).
- The next touch claims the open wheel (so it never moves/fires), and the
  selected ping is sent at `_pingTarget`; emotes still send at the player.
- An untouched wheel closes after 10s (`emote.ts emoteTimeout`), which clears
  the pending target; a send/dismiss clears it too.

### Verification

- `surv_tests`: **128 tests, 2,133 assertions, 0 failures** (new:
  `ping_wheel_idle_timeout_closes`; `map_view_dots_gas_and_pings` now covers
  airstrike graphics and place labels).
- x86_64 native lib + APK on the emulator: the big map shows all 8 place names
  (Pineapple, Todesfelde, The Killpit, ...), tapping it closes the map and opens
  the wheel at the centre, dragging up sent the ping at the tapped position and
  closed the wheel; the active gas paints the red outside-fill + safe ring on
  both views. `logcat -b crash` empty.

### Still open

- ~~Role-specific map icons (`roleDef.mapIcon`)~~ — closed in §21.
- Airstrike zone visuals on device (host-only so far).
- Other-team spectator markers on the big map.

---

## 21. Session 16 - role-specific map icons (2026-10-23)

### 21.1 `RoleRenderDef` codegen

`tools/codegen_render_defs.mjs` now imports `shared/defs/gameObjects/roleDefs.ts`
and emits `RoleRenderDef { mapIconAlive, mapIconDead }` plus
`DefProvider::role(type)`. Only four shipped roles have icons — leader
(`player-star.img` / `skull-leader.img`), captain (`player-captain.img`), medic
(`player-medic.img` / `skull-leader.img`) and last man (`player-last-man.img`);
the other roles resolve to empty icon strings.

### 21.2 Dot style parity (`mapDotStyle`)

`MapView::mapDotStyle(sameGroup, dead, downed, aliveIcon, deadIcon, dotScale)`
is a pure helper porting `ui.ts updatePlayerMapSprites`:

- Texture priority: `skull-outlined` (or the role's dead icon) → group/map
  downed art → role alive icon → `player-map-inner`.
- Scale: `dotScale` (0.2) × (same group: dead 1.5, role 1.25, else 1;
  other: dead/downed/role 1.25, else 0.75).
- The white `player-map-outer` ring is hidden for custom role icons and drawn
  one z below otherwise.
- Role icons get `zBump = 65535` (the web's `zOrder += 65535`).

`GameScene` applies it to both the live active player (role from
`ActivePlayerData.role`, downed from negative health) and every visible
`PlayerStatus` (`status.role`). In faction mode custom icons use the team
colour, otherwise group colour inside our group and team colour outside.

### Verification

- `surv_tests`: **130 tests, 2,167 assertions, 0 failures** (new:
  `role_defs_provider`, `map_dot_style_matches_web`).
- x86_64 native lib + APK smoke-tested on the emulator: the normal minimap dot
  path (inner + outer, group colour, place names, expand button) is unchanged
  and `logcat -b crash` is empty. A leader/captain role was not available in
  the solo test match, so the icon branch is currently host-tested only.

### Still open

- Verify role icons on device in a faction/role match.
- Airstrike zone visuals on device (host-only) and other-team spectator
  markers on the big map.

---

## 22. Session 17 - reverb convolution mapping (2026-10-23)

### 22.1 Why the old code was silent

`AudioManager` already computed the web's cathedral volume
(`underground ? layerVolumeMap[activeLayer] : 0`) and called
`AudioBackend::setReverbVolume`. `AxmolAudioBackend` attached the EAX
properties to a **silent looping carrier at volume 0**, so nothing audible ever
got reverb. The web instead mixes every sound through one global
`CreateJS.Sound.setReverbs` convolver; axmol only exposes per-source EAX reverb.

### 22.2 `ReverbModel` + per-handle application

- New `audio/ReverbModel.h` (engine-independent, host-tested) maps the
  `cathedral`/`cave` `ReverbDef` and the 0..1 layer amount to EAX parameters:
  gain `0.3 * v * def.volume`, decay `0.2 + 1.5v` s, reflections `0.2v`, late
  reverb `0.5v`, and the cave echo (`echoVolume`/`echoDelay`).
- `AxmolAudioBackend` now applies those properties to **every active
  `AudioEngine` handle**, prunes finished handles in `update`, re-applies only
  when the target changes by >0.01, and applies to new sounds in `play()` while
  underground. The silent carrier was removed.

### Verification

- `surv_tests`: **131 tests, 2,187 assertions, 0 failures** (new:
  `reverb_model_matches_cathedral_and_cave` covering inactive/off, half/full
  scaling, clamping, the cave echo and a missing def).
- x86_64 native lib + APK built and launched to a live match: OpenAL
  initialises normally, no reverb/openal errors and `logcat -b crash` empty.
  An audible underground comparison was not possible in this session (the test
  match never enters a bunker), so that remains the verification follow-up.

### Still open

- ~~Audible reverb check in a bunker/underground layer on device.~~ Verified on
  real hardware (2026-10-28).
- Kill leader/notification sounds and remaining UI sounds (Java UI layer).
- ~~The web's true IR convolution cannot be reproduced by axmol's EAX reverb;
  the parameter model is the closest available.~~ Accepted and verified as the
  native EAX equivalent on real hardware (2026-10-28).

---

## 23. Session 18 - world was vertically mirrored (2026-10-23)

### 23.1 Symptom

With the baked minimap in place it became obvious that the game world was
vertically mirrored: the minimap had north up, the world had north down, and
pushing the move pad up moved the player **down** on screen.

### 23.2 Cause

`Renderer::applyCamera` put the camera root at `m_pointToScreen(0,0)` and
scaled it `(z, -z)`. `m_pointToScreen` returns **PIXI screen coordinates (y
down)**, but axmol's scene graph is **y up**, so the transform mapped world +y
to screen down. The old text counter-flip (`setFlipY(true)`) hid only the glyph
symptom, not the world mirror.

### 23.3 Fix

```cpp
root->setScale(s, s);
root->setPosition(p.x, camera.m_screenHeight - p.y);
```

The origin is converted to axmol's y-up space and the world keeps a positive
scale. This makes world +y screen-up, matching the minimap and
`Camera::m_pointToScreen`, so the joystick/aim directions are no longer
inverted. Player/dead-body name labels no longer enable `Text::setFlipY`
(world text is upright), and the `Renderer.h`/`PixiLike.h` comments were
updated. Rotations that the port already negated for the web's y-down
convention render correctly with the positive root (verified with the aim line
and the player's forward pose).

### Verification

- `surv_tests`: **131 tests, 2,189 assertions, 0 failures**
  (`renderer_world_root_transform` now asserts `sy > 0` and that world +y maps
  to a smaller visual y; `player_skin_and_equipment` asserts labels are not
  flipped).
- x86_64 native lib + APK on the emulator: world terrain now matches the
  minimap/big-map orientation; an up swipe on the move pad moves the minimap
  dot up and the world scrolls accordingly; the aim line points up when the
  aim pad is pushed up; player names read upright; `logcat -b crash` empty.

### Still open

- Re-check world **rotations** on less-common sprites (doors/planes/particles)
  now that the camera is positive-Y; the player aim and buildings look correct.
- Ping edge-indicator `rot` was tuned under the old mirror; verify when a ping
  is visible on device.

---

## 24. Session 19 - role announcements + kill leader (2026-10-23)

### 24.1 Role sounds

`RoleAnnouncementMsg` was parsed and dropped. Now:

- `RoleRenderDef` also carries `soundAssign`/`soundDead` from `roleDefs.ts`
  (leader `leader_assigned_01`/`leader_dead_01`, captain/medic/etc. assign
  sounds, etc.).
- `Game` queues each announcement; `GameScene` consumes the queue and plays
  `assigned ? soundAssign : soundDead` on the `ui` channel, mirroring
  `game.ts RoleAnnouncement` (without the spooky-map `kill_leader_assigned`
  group variant — the direct role sound is used).

### 24.2 Kill-leader HUD

`UpdateMsg.killLeaderDirty` (`killLeaderId`/`killLeaderKills`) is now stored by
`Game`. The live HUD gained a top-centre `pix::Text` label
`"Kill Leader: <name> (<kills>)"` (hidden while no leader), matching
`ui.ts updateKillLeader`.

### Verification

- `surv_tests`: **132 tests, 2,201 assertions, 0 failures** (new:
  `game_role_announcement_and_kill_leader` covers the queue + kill-leader
  fields; `role_defs_provider` asserts the role sounds).
- x86_64 native lib + APK built and smoke-tested to a live match (building
  interior renders correctly with the positive-Y camera; no kill leader was
  assigned in that match so the label stayed hidden); `logcat -b crash` empty.

### Still open

- Hear/see a role assignment and kill-leader label in a real (faction/role)
  match on device; spooky-map kill-leader sound group; role announcement text.

---

## 25. Session 20 - remaining world rotations (2026-10-23)

Follow-up to §23: world nodes are now positive-Y, so every rotation copied from
PIXI (y-down, clockwise) must be negated. Most sites already were (`-renderRot`,
`-rot`, `-atan2`); three were still positive:

- `flare.ts` flare container: `atan2(dir.x, dir.y) - PI/2`
- `plane.ts` plane sprite: `atan2(planeDir.x, planeDir.y)`
- `emote.ts` ping edge indicator: `atan2(dir.y, -dir.x) + PI/2`

All three are negated now, with comments pointing at the web source. Host tests
extended: `plane_reconcile_and_elevate` asserts the plane's rendered rotation
and `flare_bullet_visual` asserts the flare container's rotation (a northbound
flare now expects `+PI/2`, not `-PI/2`).

### Verification

- `surv_tests`: **132 tests, 2,203 assertions, 0 failures**.
- x86_64 native lib + APK built and launched into a live match; no crash and the
  world still matches the minimap. A plane was not on screen in that match, so
  the plane rotation is host-tested; the ping edge indicator still needs a
  visible ping on device.

### Still open

- Visual check of a plane flyover and a ping edge indicator on device.
- Particle/smoke rotations are random-angle cosmetics and were left alone.

---

## 26. Session 21 - gas timer and game-over overlay (2026-10-23)

### 26.1 Gas timer

Added the live HUD `#ui-gas-timer` counterpart: `GasHudState { active, moving,
secondsLeft }` is pushed from `GameScene` (`Gas::duration * (1 - circleT)`) and
`Hud::setGasState` formats it as `m:ss` under the minimap (white while waiting,
red while moving, hidden while the gas is inactive), mirroring
`ui.ts updateGasDisplay`.

### 26.2 Result overlay

`Hud::showGameOver(GameOverMsg, localTeamId, localPlayerId)` builds the
`ui.ts displayStats` summary: VICTORY/DEFEAT, `Rank #N`, total team kills, and
the local player's kills/damage/survival (`Hud::formatTime`). It renders a dim
backdrop + centred panel above the HUD.

The HUD was previously hidden as soon as `Game::_playing` went false, i.e. the
frame `GameOverMsg` arrived, so the result could never be seen. `GameScene` now
keeps the HUD visible while `_game->gameOver()` is set (`isPlaying() ||
gameOver()`), and the overlay latches until the next match.

### Verification

- `surv_tests`: **133 tests, 2,219 assertions, 0 failures** (new:
  `hud_gas_timer_and_result_overlay` covering `m:ss` formatting, gas
  visibility and the defeat/victory result texts).
- x86_64 native lib + APK built and launched into a live match; the gas was
  still inactive during the short solo session so the timer stayed hidden (as
  intended) and no crash occurred. The result overlay needs a match-over event
  to be seen on device.

### Still open

- See the gas timer and result overlay in a match on device; decide whether the
  result should wait for a tap before returning to the Java menu (today the
  back button leaves).

---

## 27. Session 22 - live squad health bars (2026-10-23)

Ported the `ui.ts` team health display (previously only in the unused
`ui/FguiHud`) into the live `game/Hud`:

- `SquadMember { name, health, dead, downed, disconnected, active }` is built
  each frame by `GameScene` from `getTeamPlayerIds()` (sorted ascending, like
  `groupInfo.playerIds`), `groupStatus()` (health/DC) and `allPlayerStatus()`
  (dead/downed).
- `Hud::setSquadState` draws up to four rows to the right of the minimap,
  top-aligned with it: a translucent row backing (`208x44`, the 200px bar plus
  4px padding and extra name/bottom space), a 16px bold name above the bar, a
  200px colour-coded bar and the web status icons (`gui/close.png` offline at
  50%, `gui/skull-team.png` dead at 50%, `gui/down.png` downed with pulse) to
  the right of the block. Rows have an 8px vertical gap and 6px below the bar
  inside the block. The group-colour dot
  sits above the bar's left end, so it never covers the bar's right end. Rows
  are hidden in solo or while the HUD is hidden, and repaint only when the
  name/health/status key changes. The bar uses the exact `ui.ts
  updateHealthBar` palette (grey 100, white 75..99, darkpink->lightpink below
  75, red downed, 1px minimum for living members) and the `<=25` black/red
  danger pulse; see the twenty-fifth P2 pass in `REMAININGS.md`.
- The layout reuses the HUD's design-space `X/Y` helpers (squad rows are built
  after the static layout pass).

### Verification

- `surv_tests`: **134 tests, 2,231 assertions, 0 failures** (new:
  `hud_squad_health_bars` covering the four rows, names, Dead/Down/DC and
  hiding for solo/hidden HUD).
- Device follow-up found the first layout overlapped: the health bar was drawn
  over the lower half of the name. Rows are now 32px apart with the bar at
  `rowY - 20` (name above it). A temporary build that included the solo row
  confirmed the fixed layout ("Player" above a separate green bar).
- x86_64 native lib + APK built and a **Duo** match joined on the emulator
  (`playerCount=2, gameModeIdx=1`). The auto-fill teammate had not joined during
  the short sessions, so the real team rows still need a teammate to appear
  (`teamIds.size() > 1`). The gas timer was visible on the minimap ("0:49",
  red while moving) during the same match.

### Still open

- See the bars with a real/auto-filled teammate (the layout/colours are
  host-tested; the active-player highlight is not styled because the web's
  `ui-team-member` has no active class).
- Role icons next to squad names are not rendered by the web either (its
  `.ui-team-member-icon` selector is unused), so this is not a port gap.

---

## 28. Session 23 - gear/perk HUD + player full-object desync (2026-10-23)

### 28.1 Gear/perk icons

The live HUD now draws the `ui2.ts` gear/perk slots: `GameScene` resolves the
active player's `helmet`/`chest`/`backpack` and each perk type through
`DefProvider::gameObject(type)->img.sprite`, then `Hud::setGearState` shows up to
seven 26px icons along the top-left (squad bars sit right of the minimap). The icon
frames are the codegen'd gear/perk frames (`loot-helmet-*`, `loot-pack-*`,
`loot-perk-*`). Host test: `hud_gear_and_perk_icons`.

### 28.2 New bug: player full-object fields are misaligned

> **Fixed in §29:** the cause was the stale `generatedDefs.inc` type table, not
> the bit offsets. Regenerating it restored the ids.

On device the gear row stayed empty. A temporary log of the active player's
`fullObjects` entry showed:

```
full obj id=4498 type=1 outfit=usas backpack=emote_pumpkin helmet= chest=
         weap=helmet04_last_man_red
```

Those are valid game types but not a player's gear: `usas` is a gun,
`helmet04_last_man_red` a helmet skin, `emote_pumpkin` an emote. Positions, IDs
and the partial stream (pos/dir) are correct, so the **full stream** is being
read at the wrong offset (or the server writes a different full layout). This is
pre-existing (not caused by the gear HUD) and likely affects the player's
outfit/skin/held weapon/role/perks too; the top-down art made it subtle.

`Game::handleJoined` now seeds `_activePlayerId` from the local player id (and
the extraction falls back to `_localPlayerId`), which is correct but did not fix
the offset.

**Next step:** dump one server `UpdateMsg` frame and decode it with
`shared/net/updateMsg.ts` to compare the full-object byte/bit offsets against
`ObjectSerializeFns.h`. This is recorded as the top Priority-1 item in
`REMAININGS.md`.

### Verification

- `surv_tests`: **135 tests, 2,244 assertions, 0 failures** (gear/perk icons and
  the active-player gear extraction assertions).
- x86_64 APK built/installed; the HUD change is inert when gear is empty and the
  desync remains the blocker for the initial icons.

---

## 29. Session 24 - stale game-type ids fixed (2026-10-23)

### 29.1 Diagnosis

The §28 "full-object desync" was not a bit-offset bug: `UpdateMsg` positions,
IDs, sections and the partial stream were all correct. Dumping the packed
`Joined+Map+AliveCounts+Update` frame and decoding it with the **shared TS**
code showed the server's player object was correct
(`outfitBase`, `backpack00`, `fists`, ...), while the native parser produced
`outfit=usas`, `backpack=emote_pumpkin`, `activeWeapon=helmet04_...`.

The cause was `src/net/generatedDefs.inc`: it contained **757** game types while
`GameObjectDefs.getAllTypes()` has **676**. Every `readGameType()` id therefore
resolved to a different name:

```
outfitBase   server 409   native 489
backpack00   server 253   native 333
fists        server 362   native 442
usas         server 329   native 409   <- why outfitBase became "usas"
```

This silently affected every game type: loot, weapons, outfits, skins, emotes,
roles and perks. Map types were still aligned, which is why terrain/obstacles
looked right.

### 29.2 Fix

```sh
cd client-mobile
node --experimental-transform-types tools/codegen_defs.mjs src/net/generatedDefs.inc
node --experimental-transform-types tools/gen_reference.mjs
```

The regenerated table has 676 game types (check: `outfitBase=409`,
`backpack00=253`, `fists=362`, `usas=329`) and the reference fixtures were
regenerated from the shared TS code. No parser changes were needed (the
`handleJoined` active-id seeding/fallback from §28 stays as hardening).

### Verification

- `surv_tests`: **135 tests, 2,244 assertions, 0 failures**. The protocol
  fixtures fail against the stale table and pass after regeneration.
- x86_64 APK on the emulator: the default backpack icon now appears in the
  gear HUD, and world loot renders as the correct items (previously mapped to
  unrelated defs). `logcat -b crash` empty.

### Still open

- Verify loot/weapon/emote/role UI against the web client in a longer match;
  the id mapping itself now matches the server exactly.
- Keep `codegen_defs.mjs` in the build/verification checklist whenever
  `shared/defs` changes (a stale inc is silent but broad).

---

## 30. Session 25-28 - surfaces, bounds, ceilings and interior audio (2026-10-23/24)

Closes the two Priority-4 parity items in `REMAININGS.md` §3.

### 30.1 Full ground-surface types + building floors

`Map::getGroundSurface` only knew `Water`/`Sand`/`Grass`, so every building
interior (tile, carpet, wood/container, bunker, ...) fell back to grass for the
`footstep_<type>` sound group, and `Building::surfaces` was never populated from
the defs (the struct existed but stayed empty).

- `render/Defs.h` now defines the complete web `SurfaceType` enum
  (`water/sand/grass/tile/asphalt/brick/stone/carpet/bunker/container/warehouse/
  house/shack/snow`) plus `surfaceTypeName` / `surfaceTypeFromName`. `Map` and
  `Player` alias it instead of keeping their own 3-value enums.
- `codegen_render_defs.mjs` emits `def.floor.surfaces[]` (type, collision AABBs,
  optional water/ripple colours, e.g. the bunker pools) into
  `MapObjectDef::surfaces`; `Building::m_updateData` transforms them to world
  space exactly like `building.ts` ("Create floor surfaces").
- `Map::getGroundSurface` maps the building surface string through
  `surfaceTypeFromName` and honours the surface's own water colours; the player
  footstep group is now `"footstep_" + surfaceTypeName(surfaceType)`.

### 30.2 Structure/building bounding colliders

`MapObjectDef::boundingCollider` was only filled from `def.collision` /
`mapObstacleBounds`, which are undefined for buildings and structures. Buildings
therefore had no AABB (ceiling proximity reveal disabled, no minimap fallback),
and structure `layers[]` collisions were zero-sized, breaking the underground
test.

- The codegen now ports `mapHelpers.computeBoundingCollider`: structures union
  their layers' bounds + stairs and add the web's 1-unit margin; buildings union
  floor-surface collisions, ceiling zoom regions and map objects. Weighted
  map-object type maps are unioned over every candidate (the web picks one at
  random) so the generated file is byte-reproducible; two successive runs hash
  identically.
- `Building::getDistanceToBuilding` now matches `building.ts` (penetration of
  the transformed ceiling `zoomIn` regions) instead of an AABB distance, and the
  minimap prefers `ceiling.zoomRegions[0].zoomIn` like `map.ts renderMap`.

### 30.3 Structure layer default + `isUnderground`

`structure.ts` computes `underground = layer.underground !== undefined ?
layer.underground : i == 1`, i.e. the second layer is underground when the def
omits the flag. The codegen had collapsed it to a bool, and
`Map::isUnderground` checked every layer for an explicitly-true flag with a
hard-coded radius of 1, returning false when nothing matched. The web default is
the opposite (true), so the underground render fade/reverb never activated.

- `StructureLayerDef` now keeps `hasUnderground` alongside the value, and
  `Structure::m_updateData` applies the `i == 1` default.
- `Map::isUnderground(pos, layer, rad)` ports `player.ts` exactly: layer 1 only,
  the second layer only, `Player::rad`, and `true` when no structure matches.

### 30.4 Stale sprite atlases + emote/ping wheel icon frames

The on-device ping wheel showed no icons. Two causes:

- `Content/atlas/**` was staged on 2026-10-15, before the `gui/ping-team-*.svg`
  and `emotes/ammo-box.svg` entries entered
  `client/atlas-builder/defs/loadout.ts`, so the plists had no such frames.
  `node tools/build-atlas.mjs --res low` restages them (16 sheets, 1706 frames).
- `GameScene::resetWorld()` builds the emote/ping wheels before the first
  `MapMsg` registers the map atlases, so their build-time `setFrame()` calls
  missed and the on-screen button icons stayed blank until the wheel was
  opened. `EmoteWheel::refreshFrames()` / `PingWheel::refreshFrames()` re-resolve
  the frames; the MapMsg callback calls them right after
  `loadAtlasesForMap()`.

### 30.5 Obstacle-filtered ceiling reveal + stairs suppression

`Building::update` still used a circle/zoomIn overlap plus an AABB distance to
decide whether a ceiling should fade out, so ceilings could reveal through
walls, and the `noCeilingReveal` stair hack was missing.

- The `collisionHelpers` segment/scan helpers moved into
  `game/objects/ObstacleScan.h` (shared by `PlayerRender.cpp` melee/LOS and
  `Barns.cpp`), with a full port of `scanCollider` (5 parallel rays, obstacle
  filters for `active/dead/collidable/isWindow/height/layer`, the stairs hack).
- `Building::update` now matches `building.ts`: `scanCollider(zoomIn, obstacles,
  playerPos, player.layer, 0.5, vision.width*2, vision.dist, 5)`, then
  `noCeilingRevealTicker` zeroes the vision ticker (unless the ceiling is dead)
  and suppresses the immediate stairs reveal.
- `Player::noCeilingRevealTicker` is ported: decremented per frame and set to
  0.25 s when the active player (layer != 0) overlaps a stair whose def has
  `noCeilingReveal`, mirroring `player.ts updateRenderLayer`.

### Verification

- `surv_tests`: **139 tests, 2,320 assertions, 0 failures** (new:
  `building_floor_surfaces` checks a bank's tile surface transform + tile
  footstep group and the bunker water surface data; `structure_mask_and_underground`
  checks mask/stairs queries, non-zero layer bounds, the `i == 1` default and
  the layer/radius/default-underground semantics; `building_ceiling_scan_and_no_reveal`
  checks the scan reveal, wall blocking, the window/height/collidable filters
  and ticker suppression; `player_stairs_no_ceiling_reveal` checks the 0.25 s
  latch, the layer-0 rule and that non-active players never latch; the wheel
  tests assert the button icon frames after `refreshFrames()`).
- `src/render/GeneratedDefs.cpp` regenerates deterministically (same SHA-256 on
  two runs); the x86_64 `libSurvevMobile.so` builds and links.
- x86_64 APK installed on the emulator against the bundle server: `Play Solo`
  joins, atlases load, the player moves with the joystick, the minimap dot
  tracks, the ping/emote buttons show their icons before opening, holding the
  ping button shows all six selectors (medical/ammo greyed in solo), and a
  released ping draws the yellow ring on the player. A match was walked through
  for ~40 s (including into the gas) with no crash; `logcat -b crash` empty.

### 30.6 Structure interior ambience

`Structure` kept `soundTransitionT`/`soundEnabledT`, but nothing drove the
immediate-mode `interior_0`/`interior_1` Ambiance tracks, so bunkers/clubs/
saloons were silent.

- The def codegen emits `structureDefs.ts interiorSound` (sound/soundAlt/
  filter/transitionTime/outsideMaxDist/outsideVolume/undergroundVolume) into
  `MapObjectDef::interiorSound`.
- `Structure::update` ports `structure.ts updateInteriorSounds`: outside vs
  layer 1, building distance (`getDistanceToBuilding`), ceiling fade
  (`weight0 = w*(1-fade)`, `weight1 = w*fade*outside/undergroundVolume`), the
  soundAlt transition (`|t-0.5|*2`), the disabled fade and the club filter.
- `Map::update` calls it for every active structure before `Ambiance::update`
  (immediate-mode tracks are reset after each mixer tick).
- `Ambiance` gained `setTrackFilter` plus read-only track accessors.
- Note: the `club` filter name is plumbed through `PlaySoundOptions.filter`, but
  `AxmolAudioBackend` has no per-source EQ (OpenAL EAX is reverb-only), so the
  track plays unfiltered; the web's `muffled`/`club` EQ remains a backend gap.

### Verification

- `surv_tests`: **144 tests, 2,400 assertions, 0 failures** (new:
  `structure_interior_ambience` checks the def fields, the opaque-ceiling
  filtered track (`outsideVolume` 0.25 + `club` filter), the revealed
  unfiltered track, layer-1 `undergroundVolume` 0.707, the `reserve_music_02`
  soundAlt transition and the enable/disable fade).
- x86_64 native lib + APK rebuilt, installed on the emulator, `Play Solo` joins
  and runs with OpenAL initialized; `logcat -b crash` empty.

### 30.7 Building sound emitters (building.ts)

`buildingDefs.ts soundEmitters` (bathhouse steam, potato oasis waves) were not
ported, so those looping positional ambiences never played.

- The def codegen emits `soundEmitters` (sound/channel/local pos/range/
  falloff/volume) into `MapObjectDef`.
- `Building` creates the world-space emitter points on `isNew`, runs the web's
  10 Hz volume update (`baseVolume * emitter.volume * remap(dist, range) **
  falloff * lerp(ceilingFadeAlpha, 1, 0.25)`, same-audio-layer gate, 0.003
  threshold), plays them looped/start-silent and stops them in `m_free()`.
- `setVolume(..., "sound")` supplies the web's `getTypeVolume("sound")` factor;
  `baseVolume` is used explicitly like `building.ts`.
- `EmoteManager.h` moved its `audio::AudioManager` forward declaration inside
  `namespace surv` (it was leaking a global `audio` namespace and made
  `audio::` ambiguous in TUs that also use `using namespace surv`).

### 30.8 JNI bridge thread marshalling (second-join SIGSEGV)

The on-device second join intermittently crashed in
`Pool<Player>::rebuild()` (`GameWorld::update` → `PlayerBarn::update`) with a
destroyed world. Root cause: `SurvBridge` JNI methods run on the Java UI thread
and called `enterWithJoin()`/`leaveGame()`/pause/audio directly; `resetWorld()`
then destroyed the `GameWorld` while the GL thread was inside
`GameScene::update()`.

- `UiBridge.cpp` now queues scene-mutating calls (`quickStart`, `joinUrls`,
  `cancelPending`, `quitGame`, `pauseGame`, `resumeGame`, `setAudio`) through
  `ax::Scheduler::runOnAxmolThread`, so teardown/rebuild happens between frames
  on the axmol thread (ARCHITECTURE.md A4.2).
- Verified with three consecutive **Play Solo → Back → Play Solo** joins
  (`activePlayerId` 4596 → 4597 → 4598): each joins, renders and returns to the
  menu with an empty `logcat -b crash`.

### 30.9 Per-source EQ (muffled/club) + automated audio trace

> **Superseded (2026-10-28):** the low-pass approximation below was replaced by
> the web's exact 10-band biquad cascade on decoded PCM; the low-pass remains
> only as the streaming fallback. See `REMAININGS.md` pass 37 and
> `M6_AUDIO.md` "Per-source EQ".

The web client filters sounds through biquad EQ chains (`muffled`, `club`); the
native port only plumbed the filter name and dropped it.

- `tools/patch-axmol-audio-filter.ps1` (called by `tools/vendor-axmol.ps1`)
  adds `ax::AudioEngine::setFilterProperties(audioId, enabled, gain, gainHF)` on
  top of the vendored engine's existing EFX helpers (`AUDIO_FILTER_LOWPASS`,
  `AL_LOWPASS_GAIN`, `AL_LOWPASS_GAINHF`). `AudioPlayer::clearEffects` releases
  the filter with the source.
- `audio/AudioFilter.h` maps the web names onto the low-pass (muffled
  gain 0.90/gainHF 0.05; club 0.85/0.02; none/reverb disabled — reverb is the
  EAX path). `AudioManager` resolves the filter exactly like `audioManager.ts`
  (different layer or `forceFilter` uses the EQ, otherwise the reverb send) and
  passes it in `PlayOptions.filter`; `AxmolAudioBackend` applies it at play.
- `AxmolAudioBackend` logs every play/`filter`/EQ parameter when
  `debug.surv_debug=1` (system property or UserDefault), so audio can be
  verified automatically from `adb logcat`. A debug-only match-start self-test
  plays `emote_01` once muffled and once club, proving the EFX path on-device.
- Automated checks: `audio_filter_mapping`, `audio_filter_routing` (layer rule,
  forceFilter, club/none) and `player_footstep_uses_floor_surface` (the player
  on a bank tile requests `footstep_tile_*`, never grass), plus the device trace
  below.

### Verification

- `surv_tests`: **144 tests, 2,400 assertions, 0 failures**.
- On-device (x86_64 emulator, `debug.surv_debug=1`):
  - `[audio] play emote_01ui filter=muffled eq=lowpass gain=0.90 gainHF=0.05`
  - `[audio] play emote_01ui filter=club eq=lowpass gain=0.85 gainHF=0.02`
  - footsteps walked grass → sand → water with
    `[audio] play footstep_{grass,sand,water}_* filter=reverb`, confirming the
    surface groups and the layer/reverb routing without listening.
  - no `AL error` lines and `logcat -b crash` empty.

### Still open

- ~~Human listening is still the only way to judge the EQ's timbre; the call
  trace now proves the filter chain runs and the exact parameters.~~ Verified on
  real hardware (2026-10-28).
- Structure interior tracks and building `soundEmitters` were not reachable in
  the short device runs, so their on-device trace lines remain unobserved
  (host tests cover the selection/volume).

---

## 31. Session 30 - leader ping sounds + airstrike falloff (2026-10-24)

Closes the last two ping parity items from §17/§20.

### 31.1 `PlayerStatus.role` lookup

`GameWorld` now exposes `setPlayerRoleLookup(fn)` / `playerRole(playerId)`.
`GameScene::resetWorld()` binds it to `Game::allPlayerStatus()`, so `EmoteManager`
can read the sender's role without importing `Game` (the status table already
comes from `UpdateMsg.playerStatus`).

### 31.2 `EmoteManager::addPing`

- Leader/captain/last-man pings use `pingDef.soundLeader` when it is set;
  everyone else uses `pingDef.sound`.
- Non-faction `ping_airstrike` sounds use `rangeMult 20` + `fallOff 1` +
  `soundPos = ping.pos` (distance-attenuated/ panned); faction mode and all other
  pings play at full volume, matching `emote.ts addPing`.

### Verification

- `surv_tests`: **145 tests, 2,414 assertions, 0 failures** (new
  `ping_sound_leader_and_airstrike_falloff`: leader/captain/last-man soundLeader,
  normal sender sound, attenuated/panned airstrike and full-volume faction
  airstrike).
- x86_64 native lib + APK built and installed; two Play Solo joins (with a
  backtrack) stayed connected and `logcat -b crash` was empty. The leader sound
  needs a faction/role match and the airstrike needs a plane event, so those
  remain host-tested until such a match is available.

---

## 32. Session 31 - crosshair loadout pipeline (2026-10-24)

Closes the "crosshair style/colour from loadout" item from §16 and fixes the
Java config bridge that silently dropped every loadout write.

### 32.1 The bridge had no config (Java writes were discarded)

`AppDelegate` installs the bridge as `installUiBridge(nullptr, gameScene)` (Java
owns the menu), so every `AndroidUiBridge` method guarded by `if (_menu)` was a
no-op: name, region, loadout, crosshair and volume writes never persisted.

- `AndroidUiBridge` now owns a standalone `ui::Config` over
  `UserDefaultConfigStorage` when `_menu` is null, and all config reads/writes
  (`configJson`, name/region/loadout/language/audio, `saveConfig`) go through
  `config()`. Loadout edits persist and reach the `JoinMsg`.
- `joinUrls` sets the **Game** join info via `_game->getGame()->setJoinInfo()`
  (`_game` is a `GameScene*`, not a `Game*`); `GameScene::enterWithJoin` no
  longer overwrites it with its default. This also restores the player name and
  outfit/heal/boost/emote selection from the Java menu.

### 32.2 Crosshair config shape + validation

The Java editor sent `{"crosshair":"custom","crosshairColor":...}` (flat,
partial), which replaced the whole loadout and was ignored by the validator.

- `UiBridge::setLoadoutJson` merges partial documents into the current loadout
  (nested `crosshair` fields merge, other categories survive) before
  validating/storing them.
- `ui::Loadout::validated` validates the nested
  `crosshair.{type,color,size,stroke}` shape, accepts `#RRGGBB` or a number for
  the colour, clamps size/stroke, and migrates the legacy flat
  `crosshairColor/crosshairSize/crosshairStroke` shape (0..2 indices ->
  0.7/1.0/1.4 and 0/1/2). A nested object always wins over stale flat keys.
- `Game::JoinInfo` gained the local-only crosshair fields; `Loadout::joinInfo`
  fills them. The Java editor now sends the nested shape with float size/stroke
  and merges into the full loadout.

### 32.3 Crosshair rendering

- `tools/build-crosshair-icons.mjs` rasterizes all 30 `CrosshairDefs` styles
  into `Content/gui/crosshair/<type>_s<0..6>.png` (the stroke slider's
  0.0..1.5 range in 0.25 steps; frame index `round(stroke * 4)`), wired into
  `tools/sync-content.ps1`.
- `GameScene` draws the selected style as a screen-space sprite at the viewport
  centre, tinted by `crosshairColor`, scaled from `size` (64px * size *
  screenHeight/720), with the old two-bar `Graphics` fallback when the icon is
  missing. Hidden whenever the HUD is not playing.

### Verification

- `surv_tests`: **146 tests, 2,438 assertions, 0 failures** (new
  `ui_loadout_crosshair_validation`: nested type/colour/size/stroke, invalid
  type/limits, legacy migration, nested-wins-over-flat, and the validated JSON
  shape).
- Device (x86_64 emulator): the Java Customize -> Scope editor changed the
  colour to green and size to small; the next match logged/rendered
  `crosshair_default`, `color=00ff00`, `size=0.7` (screenshot-verified), with an
  empty `logcat -b crash`.
- `Content/` is gitignored; run `node --experimental-transform-types
  tools/build-crosshair-icons.mjs` (or `sync-content.ps1`) before packaging.

### Still open

- The Java editor has colour/size/stroke but no style picker (native config can
  carry any `crosshair_*` type; the style grid is empty because `SurvBridge`
  does not expose the available crosshair items yet).
- Stroke is bucketed to 0/1/2; the web accepts arbitrary float widths.

---

## 33. Session 32 - door anchor and open-state position (2026-10-24)

Fixes the reported "door is positioned one door-width next to the hinge" bug.

### 33.1 PIXI vs axmol sprite anchors

`obstacle.ts` pivots doors at `def.door.spriteAnchor`, which is a **PIXI**
(top-left origin) anchor, usually `(0.5, 1)` (bottom-centre). The port passed it
straight to `ax::Node::setAnchorPoint` (bottom-left origin), so `y = 1` put the
pivot at the door's **top** and the slab extended one door-length past the hinge
(the opposite side/along the wall). The port now converts `(x, y)` to
`(x, 1 - y)` when applying a door anchor in `Obstacle::applyFrame`/`render`.

### 33.2 Open-door seed and casing

Two more `obstacle.ts` mismatches in `Obstacle::m_updateData`:

- A door first streamed in while already open was seeded at
  `closedPos = data.pos + slideOffset` instead of `data.pos`, so it appeared
  one slide offset (about one door width) away before interpolating back.
  `doorInterpPos`/`doorInterpRot` now seed from `data.pos`/`rot`, matching
  `m_updateData` (`interpPos = data.pos`).
- `doorClosedPos` (the casing/slot position) was only set on the first update;
  for sliding doors that open later the casing stayed at the open position.
  It is now recomputed on every door update as
  `data.doorOpen ? data.pos + rotate(slideOffset, rot + PI/2) : data.pos`.

### Verification

- `surv_tests`: **146 tests, 2,453 assertions, 0 failures** (new door-anchor
  conversion checks, open-door seed/casing position checks, and open-speed
  interpolation checks).
- Device (x86_64 emulator): the Killpit/prison gate doors render centred in
  their wall slots after the anchor fix; no native crash.

### 33.3 Open speed

`Obstacle::update` hardcoded `15` for both door position and rotation
interpolation. `def.door.openSpeed` is now generated into
`MapObjectDef::doorOpenSpeed` (default 2) and used exactly like `obstacle.ts`
(`moveSpd = openSpeed`, `rotSpd = PI * openSpeed`, no scale factor), so house
doors/hinged doors/lab doors animate at their configured speeds instead of
snapping at a fixed rate.

---

## 34. Session 33 - clipped/zoomed minimap + obstacles (2026-10-24)

### 34.1 The minimap squeezed the whole map into 140px

`MapView::setTexture` scaled the whole baked map to the view size, so the
in-game minimap showed the entire map in a 140px square. Web `ui.ts` does the
opposite: its map sprite is `screenScaleFactor * 1600 / 1.2` (~1000px) inside a
256px (`Sm`: 192px) window, centred on the player and clipped by
`container.mask` - i.e. it shows ~25.6% of the map.

`MapView` now has two modes:

- `setZoomed(true)` (Hud's `_miniMap`): map sprite = `size / 0.256`, local
  offset `size/2 - player/map * mapScale`; `setCenter()` (called every frame
  from `GameScene` with the active player position) keeps the player centred.
  A ClippingNode clips the view to the `size x size` window; it is created
  lazily in `setCenter()` because creating a ClippingNode during HUD build
  crashes axmol.
- `setZoomed(false)` (Hud's `_bigMap`): the matrix is unchanged (whole map
  fitted to 640px, no clip).

`project`/`unproject`, dots, pings, gas/safe-zone, airstrikes and place labels
all use the view transform. The ping-layer stencil is offset to the view origin
too (it previously sat at 0,0 while the layer was at `_pos`).

### 34.2 Obstacles were missing from the baked texture

`Map::generateMinimapTexture` walked `obstaclePool`/`buildingPool`, but the
texture is baked during `loadMap()`, before the first `UpdateMsg` spawns the
pools, so the map had terrain only. It now walks the static `MapMsg.objects`
like `map.ts renderMap()` (`getMinimapRender`), using the def's
`map.shapes`/collision/bounding collider and z-order. The texture is 1024x1024
(was 256) so the zoomed minimap is not blurry; the big map benefits too.

### Verification

- `surv_tests`: **147 tests, 2,470 assertions, 0 failures** (new
  `map_view_minimap_is_zoom_centred_and_clipped`; updated baked-texture size and
  minimap centre/scale/mask checks).
- Device (x86_64 emulator): minimap shows a zoomed player-centred window with
  trees/rocks/buildings and a clean clipped border; the big map still shows the
  whole map with the same obstacles; `logcat -b crash` empty.

## 35. Session 34 - region latency test, the first M8 slice (2026-10-24)

### 35.1 What was missing

`REMAININGS.md` §3 Priority 5 opens with M8 (accounts/API). Of its list
(login/profile/team links/quests/pass/proxy/region ping test/error reporting),
the only piece that needs neither OAuth, a Java account screen nor a database
round-trip is the pre-game latency check - `client/src/pingTest.ts` was still
unported and the Java menu picked the first region in `site_info.pops`.

### 35.2 The probe targets are generated, not fetched

The web client bakes `Config.regions` into the bundle (`PING_TEST_URLS` in
`client/vite.config.mts`). The native build does the same:
`tools/codegen_regions.mjs` imports the repo's `config.ts` (`getConfig`), writes
`src/net/generatedRegions.inc`, and `net/Regions.{h,cpp}` exposes it as
`std::vector<PingTestEntry>` (`zone == region`, like `PING_TEST_URLS`).

**The server is untouched.** `site_info` still only carries populations; the
native client never asks the API for region addresses (the first iteration of
this session added `pops[].address` to `getSiteInfo`, which was reverted).

```sh
# deployment config (survev-config.hjson), matching `vite build`
node --experimental-transform-types tools/codegen_regions.mjs --production
# dev defaults (adds the `local` region), matching `pnpm dev`
node --experimental-transform-types tools/codegen_regions.mjs
```

### 35.3 net/PingTest - the engine-independent port

`src/net/PingTest.{h,cpp}` mirrors `pingTest.ts` exactly:

- one probe per region: `ws(s)://<address>/ptc`, one byte in flight at a time,
  8 Hz, `recvCountMax = 6`, `retryCountMax = 1` (retries exhausted -> the test
  completes with no measurement);
- `ping = min(elapsed)`; the native port keeps **milliseconds** (the web stores
  seconds because it only logs the value);
- `bestRegion()` reproduces the web's `sort(a.ping - b.ping)[0]` (stable: ties
  keep the earlier entry, unanswered regions only win when nothing answered);
- `zones(region)` returns the region's zones fastest-first for `find_game`;
- `resultsJson()` is the `{"complete","bestRegion","tests":[...]}` snapshot the
  Java menu consumes.

The transport is injected (`ConnectionFactory`), so `tests/test_ping_test.cpp`
drives the whole state machine with `FakeConnection` + an injected clock
(cadence, minimum, completion, retry, stop, JSON shape, `entriesFromJson`).

A close/error callback must not destroy its own `Connection` (`PingTestEntry::
needReopen` defers the reset to after `pump()` returns) - destroying it inside
the callback frees the object whose member function is running.

### 35.4 Wiring (Android)

- `GameScene` (the only always-ticking C++ owner while the Java menu is up)
  starts the probes from `pingTestRegions()` in `init()` and owns the
  `PingTest`, ticked from `update()`; it stops when a match starts
  (`enterWithJoin`) or the app backgrounds (`pauseGame`), and `resumeGame()`
  restarts the probes that never finished.
- Results are pushed through `UiHost::pingResults` ->
  `SurvBridge.onNativePingResults` -> `JavaUiHost.pingResults` ->
  `MenuFragment.onPingResults` (never polled). There is no Java -> C++ start
  command, so nothing races the engine startup.
- `MenuFragment` renders `Region [N players] · 32 ms` in the label and the
  dropdown, auto-selects the fastest region when the player has not chosen one
  (`config.regionSelected`; `UiBridge::setRegion` now persists it like main.ts's
  config listener), and sends the selected region's zones with `find_game`
  instead of `"zones":[]`.

### 35.5 Verification

- `surv_tests`: **155 tests, 2,546 assertions, 0 failures** (7 ping tests,
  including `ping_regions_generated_table`, which guards the codegen output).
- `build-native.ps1 -Abis x86_64`: links.
- `build-apk.ps1 -Abis x86_64`: packages (javac + JNI symbols resolve).

### 35.6 On-device check (x86_64 emulator, unchanged bundle server)

```
I/axmol:       [ping] start region probes: 1 entries
I/axmol:       [ping] results {"complete":true,"bestRegion":"local","tests":[{...,"pingMs":32,...}]}
I/MenuFragment: pingTest results {"complete":true,...,"pingMs":32,...}
```

- The region row/dropdown renders `Local [0 players] · 32 ms` (verified with
  `uiautomator dump`) against the stock bundled API, i.e. without the reverted
  `site_info` change.
- Play Solo sends `"zones":["local"]` (fastest-first zones), joins the match
  (`Connected to game server`, `Receiving game updates`).
- Results are reported once per start (a `stop()` after a completed test no
  longer re-reports it), and `logcat -b crash` stays empty.

## 36. Session 35 - client error reporting (M8, 2026-10-24)

### 36.1 Why this slice

The rest of M8 (login/profile/team links/quests/pass) needs OAuth, account
screens and database round-trips, and the server must stay untouched, so the
last client-only M8 item was `client/src/errorLogs.ts` - unported, which meant
native crashes and client errors were invisible to the server.

### 36.2 net/ErrorLog - the policy port

`src/net/ErrorLog.{h,cpp}` reproduces `errorLogs.ts` exactly:

- five reports per budget, then a 180 s throttle; `store()` re-arms the window
  on every suppressed report (the web behaves the same way);
- `update()` decays one request per second, or lifts an expired throttle;
- `storeGeneric(parent, child)` is 1% sampled and always drops
  `parent == "account"` (those only mean a request failed);
- `logWindowOnError()` sends at most two per process;
- body is `{"loc": ..., "data": ...}` (`data` omitted when absent).

Poster, clock and random are injected, so `tests/test_error_log.cpp` covers the
document shape, budget decay, throttle/re-arm, sampling and the two-error cap.

### 36.3 Wiring

- `net/Api::postJson` posts through axmol's `HttpClient` (fire and forget, like
  `fetch`). `isDevLogEnabled()` now also reads the `debug.surv_debug` system
  property, the same switch as the audio trace, so no rebuild is needed.
- `GameScene` owns the `ErrorLog`, ticks it at 1 Hz, resolves the API origin like
  find_game (`surv_apiUrl` UserDefault else `DevConfig::kApiBaseUrl`) and with
  `debug.surv_debug=1` sends one `startup_probe` report at launch. Nothing is
  sent in normal runs.
- `SurvBridge.reportError(loc, dataJson)` -> `UiBridge::reportError` ->
  `GameScene::reportError` for non-fatal Java errors (`windowOnError` goes
  through the two-report cap, anything else becomes an `errorLog` entry).
- `AppActivity` installs a default uncaught-exception handler. A main-thread
  crash posts `windowOnError` synchronously through `SurvApi.reportError` (a
  short-lived sender thread joined for 1.5 s) because the process - and the
  game-thread reporter - is about to die. A worker-thread crash leaves the app
  alive, so it uses the budgeted bridge path instead.
- `SurvApi.DEFAULT_API_BASE_URL` is now the single Java copy of the dev API
  origin (`MenuFragment` uses it too).

### 36.4 Verification

- `surv_tests`: **161 tests, 2,588 assertions, 0 failures** (6 new error-log
  tests).
- Debug probe: `adb shell setprop debug.surv_debug 1` + relaunch logs
  `[dev] POST http://127.0.0.1:8000/api/report_error` and the API log gains one
  `start: /api/report_error`; with `debug.surv_debug=0` no request is sent.
- Crash handler: a temporary `throw` in `AppActivity.onCreate` (removed again)
  produced the `FATAL EXCEPTION ... surv crash test` and exactly one
  `/api/report_error` request before the process died.
- `build-native.ps1 -Abis x86_64` + `build-apk.ps1 -Abis x86_64`: green; the
  final APK boots to the menu with `logcat -b crash` empty.

## 37. Session 36 - lifecycle hardening (2026-10-24)

### 37.1 Backgrounding lost the match

`Game::pause()` closed the game socket with the "background" reason and
`Game::resume()` re-joined with the same join token. The server deletes a token
as soon as it is used (`server/src/game/client.ts`), so the rejoin was rejected
and `GameScene` dropped to the menu (`returnToMenu error=index-host-closed`).
On-device this reproduced with a 3 s app switch.

Fix: `pause()` only freezes input (`Game::sendInput` no-ops while paused) and
keeps the socket; `resume()` keeps playing on the surviving socket. The pause
dialog uses the same path, so opening settings no longer leaves the match.

### 37.2 Half-open mobile sockets

A cell handover can leave a TCP socket open on both ends while no frames flow
(`GameScene` would sit in a frozen match forever). `Game::update` now tracks
`_elapsed`/`_lastMsgTime` and, after joining, synthesizes
`close(1006, "timeout")` when 12 s pass without any server frame
(`kConnectionTimeoutSeconds`). `GameScene` already maps a non-zero close code to
the localized "connection lost" path.

### 37.3 Bounded event backlog

While backgrounded the director is stopped, so nothing pumps the WebSocket event
queue while the socket keeps receiving updates. `WebSocketConnection` now caps
its queue at 4096 events and turns an overflow into `close(1006, "backlog")`
(buffered protocol deltas cannot be skipped without corrupting state), so a long
background cannot grow memory without bound.

### 37.4 Back button

Android BACK is delivered to the C++ key listener (axmol maps it there), not to
`AppActivity.onKeyDown`, so the match used to be abandoned. `handleBackPressed`
now calls the new `UiHost::togglePause()` -> `SurvBridge.onNativeTogglePause` ->
`JavaUiHost.togglePause()`: BACK opens/closes the Java pause dialog; its Quit
button still calls `onQuitGame`. Hosts without the dialog (desktop) fall back to
leaving the match.

Outside a match the engine path still owns the key and now closes the Java menu
screens too (pass 41): `MenuScene::handleBackPressed` and `GameScene`'s
menu-state fallthrough call the new `UiHost::handleBack()` ->
`SurvBridge.onNativeHandleBack` -> `JavaUiHost.handleBackPressed()`, and
`showBackBtn()` brings the `<` arrow to the container front (fragments added
later used to bury it under their full-screen dim roots and swallow its clicks,
which made the arrow dead over the team screen).

### 37.5 Verification

- `surv_tests`: **163 tests, 2,599 assertions, 0 failures** - new
  `game_pause_keeps_the_socket_and_resume_does_not_rejoin` (replaces the old
  close/rejoin test), `game_watchdog_closes_a_silent_socket` and
  `game_watchdog_is_reset_by_traffic`.
- Device (x86_64 emulator, stock bundle): join a match, press HOME, wait 40 s,
  return - still in the same match, no disconnect and no menu (verified with
  `uiautomator dump`); BACK in a match opens the pause dialog ("Return to Game",
  volume/sensitivity), BACK again closes it and play continues; `logcat -b crash`
  empty.

## 38. Session 37 - find_game retry + FPS overlay (2026-10-24)

### 38.1 Menu find_game retry

`client/src/main.ts findGameImpl()` retries the discovery request on a network
error (two attempts, 500 ms apart) before surfacing the failure. The Java menu
used to give up after one request, so a single flaky packet showed "Failed to
find game". `MenuFragment.onPlay` now delegates to `findGameAttempt(body, n)`:
a `null` response (transport failure - `SurvApi` logs
`POST ... failed`) retries once after 500 ms, keeping "Finding game..." up;
server-reported errors (`{"type":"error"}`) are not retried, matching the web.

### 38.2 Working "Show FPS" toggle

The pause dialog's "Show FPS" button only toggled its own label. It now calls
`SurvBridge.setShowFps(bool)` -> `UiBridge::setShowFps` ->
`Director::setStatsDisplay` (axmol's fps/draw-call/vertex overlay) and persists
`showFps` in the config; `seedFromConfig()` restores the button state from it, so
the choice survives dialog reopen and app restarts. The persisted value is applied
at startup by `AndroidUiBridge::setScenes` (the config is one JSON blob behind the
bridge, so `UserDefault::getBoolForKey` cannot read it), and the dev switch
(`debug.surv_debug=1`) forces the overlay on for profiling. Both the toggle and
startup log the applied state (`[perf] stats display on/off`) so the path is
verifiable from logcat.

The "Low GFX" button is still a visual stub: a real battery/resolution tier needs
an EGL surface scale (`GLSurfaceView.holder.setFixedSize`), not just a smaller
design resolution, and is left for the next perf session.

### 38.3 Verification

- `surv_tests`: **163 tests, 2,599 assertions, 0 failures** (unchanged; the work
  is Java + app wiring).
- `build-native.ps1 -Abis x86_64` + `build-apk.ps1 -Abis x86_64`: green.
- Device, API stopped: Play Solo logs
  `find_game transport failure, retrying after 500ms (attempt 2/2)` with two
  `POST ... failed` lines, then "Failed to find game"; with the API back up the
  first attempt succeeds.
- Device, pause dialog: tapping "Show FPS" logs `[perf] stats display on` and
  marks the button as checked; tapping again logs `stats display off`. The value
  persists: reopening the dialog re-seeds it, and a restart logs
  `[perf] stats overlay enabled at startup` only when it is on.
- `adb shell setprop debug.surv_debug 1` + restart logs
  `[perf] stats overlay enabled by debug.surv_debug`. `logcat -b crash` stays
  empty.

## 39. Session 38 - mid-match reconnection (2026-10-25)

### 39.1 Why the first attempt failed

A reconnect cannot reuse the join token: `joinTokens` entries are deleted on the
first join. The first design registered a separate rejoin ticket from
`handleSocketClose`, but on-device (iptables RST for 2 s) the client was back
within ~1 s while the server only noticed the dead socket ~10 s later, so every
retry hit "invalid_token" before the ticket existed.

### 39.2 Server: a ticket from join time

`server/src/game/game.ts` now keeps `rejoinTokens` keyed by the join token, and
`ClientBarn.addClientWithPlayer` registers it as soon as the player exists. A
rejoin is therefore accepted even while the previous socket still looks alive:

- `ClientBarn.rejoinClient()` marks the old `Client` as `replaced`, removes it
  from the client list and closes it (`"rejoined"`), then attaches the new
  socket. Its late `handleSocketClose` returns before the despawn/input-reset
  path, so the seat keeps playing.
- `Client.beginRejoin()` resets `ack`, visibility caches and `_firstUpdate`, so
  the next `sendMsgs()` resends Joined + the Map stream + every visible object;
  the native client rebuilds its world from that full snapshot.
- `player.client` is swapped to the new socket (pickup/stats/game-over messages
  follow it) and the group statuses are recomputed.
- Tickets are pruned in `Game.update` when the player dies or despawns;
  `GameWsDisconnectReason` gained `"rejoined"` for the kicked socket.

Trade-off (documented in the code): the token is a bearer credential for the
match rather than strictly single-use, so a leaked token could steal the seat
until the player dies.

### 39.3 Client: retry with backoff

`GameScene::tryReconnect()` runs when an in-match socket closes unexpectedly:

- retryable closes only (`0`, `1006`, `1011`) and never a server rejection
  reason (`invalid_token`, `ip_banned`, `rate_limited`, `behind_proxy`); those
  fall through to the localized error;
- up to five attempts - immediate, then 1 s, 2 s, 3 s and 4 s apart - each one a
  `Game::free()` + `enterWithJoin()` on the same token, so the rejoin goes
  through the normal JoinMsg path and the server sends a fresh snapshot;
- success clears the state in the `hasJoined()` branch; giving up shows
  `index-host-closed` (a late `invalid_token` from an exhausted window is still
  reported as a lost connection, not a bad token);
- `Game::onError` now synthesizes `close(1006, "error")` when a socket fails to
  open, because axmol only reports the error: without it the retry loop and the
  URL fallback could sit on a connection that never produced a close.

### 39.4 Verification

- Server `tsc` typecheck, root `lint:ci`, vitest (11,768 tests) and
  `surv_tests` (163 tests / 2,599 assertions) are green.
- Against the dev server (`dev:api` + `dev:game`, the bundle server predates the
  change), forcing an RST on the game port for 2 s:
  - client: `[net] connection lost in a match`, `rejoin attempt 1..3`,
    `Connected to game server`, `Receiving game updates` (same
    `activePlayerId`); still in the match, no menu;
  - server: `"Player" rejoined` (no `left` - the stale socket was replaced).
- Blocking the port for 20 s: five attempts, then
  `returnToMenu error=index-host-closed` and the menu - no frozen match.
- `logcat -b crash` empty in both runs. The bundle stack was restored afterwards
  and adb un-rooted.

## 40. Session 39 - Low GFX tier, perf instrumentation, config JNI (2026-10-25)

### 40.1 "Low GFX" is now a real resolution tier

The pause dialog's Low GFX button was a pure visual stub. Native has no
`highResTex` reload, but the same fill-rate saving is available at the surface:
`RenderScale.apply()` calls `AxmolGLSurfaceView.getHolder().setFixedSize(w*0.75,
h*0.75)` so the GL surface covers 44% of the pixels and the compositor upscales
it to the view; layout, HUD and touch coordinates are untouched. `(0, 0)` clears
the fixed size again.

- applied at launch by `AndroidUiBridge::setScenes` from the `renderScale` config
  key (the C++ config is the source of truth),
- toggled in the pause dialog, persisted through `SurvBridge.setConfigValue`,
- seeded into the button state from the config like "Show FPS".

### 40.2 Perf instrumentation

With `debug.surv_debug=1`, `GameScene::update` logs every 5 s:

```
[perf] fps=59.6 frame=960x540
```

and logs `[perf] frame size WxH` whenever the GL frame size changes (so toggling
Low GFX is visible in logcat). The axmol stats overlay from �37 remains the
on-screen version.

### 40.3 Missing setConfigValue JNI

`SurvBridge.setConfigValue` was declared in Java and called by the pause dialog's
sensitivity sliders, but no C++ symbol existed - dragging a slider would throw
`UnsatisfiedLinkError`. It is now implemented on `UiBridge`, preserving the JSON
type (number/bool/string) and saving the config. The values persist; the touch
code does not consume them yet (native-only sliders, the web has no equivalent).

### 40.4 Verification

- `surv_tests` 163 tests / 2,599 assertions; `lint:ci`; x86_64 native + APK
  (39 Java sources) all green.
- Device, `debug.surv_debug=1`, match running at 960x540: `[perf] fps=59.6
  frame=960x540`. Low GFX on logs
  `RenderScale: render scale 0.75 (720x405 from 960x540)` and
  `[perf] frame size 720x405`; off logs `render scale FULL (960x540)` and the
  frame returns. After a restart the tier is applied before the first frame
  (`[perf] render scale 0.75 at startup`, `[perf] frame size 720x405`).
- Dragging the "Move" sensitivity slider no longer crashes and the value
  persists into the next dialog open (`Move 96%`). `logcat -b crash` empty.

## 41. Session 40 - safe-area insets for the HUD (2026-10-25)

### 41.1 Why

The HUD was fitted to the whole viewport. On devices with a display cutout,
rounded corners or a system bar the outermost widgets (minimap/kill feed on the
right, squad bars on the left, health/boost at the bottom) could sit under the
cutout; the app deliberately draws under it (`cutoutMode=shortEdges`).

### 41.2 Fix

`GameScene::safeAreaRect()` reads `RenderView::getSafeAreaRect()` (axmol folds
cutout/rounded-corner/soft-key insets into it, in view points) with a fallback to
the visible rect for degenerate values. `layoutHud()` now fits the 1280x720 HUD
into that rect and centres it there; it runs both when the HUD is built and on
every resize. The world stays full-bleed, and the touch pads keep their
screen-space offsets (126/100 design units from the edges, well clear of a
cutout).

With `debug.surv_debug=1` the safe rect is logged only when it differs from the
viewport:

```
[perf] HUD fitted to safe area x=128 y=0 1152x720
```

### 41.3 Verification

- `surv_tests` 163 tests / 2,599 assertions; `lint:ci`; x86_64 native + APK.
- Emulator (960x540 surface, Low GFX on -> 720x405), cutout emulation
  (`cmd overlay enable ...cutout.emulation.corner`): the log above appears and
  the HUD is scaled to fit the 1152-wide safe rect (fit 0.9) instead of the full
  viewport. Disabling the overlay removes the log (rect == viewport).
- 3-minute in-match run with `debug.surv_debug=1`: 36 fps samples, 58.4-60.7 fps
  at 720x405 (Low GFX on), no crash. The socket then closed (bundle server, which
  predates the rejoin support) and the client took the expected
  `index-host-closed` path back to the menu; `logcat -b crash` empty.

## 42. Session 41 - M8 device login and profile (2026-10-25)

### 42.1 Scope

M8 was the last unchecked priority. Its web shape (Discord/Google OAuth) does
not exist on Android: there is no browser callback into the app and no cookie
jar in the native HTTP client. The adopted identity is therefore **the install**:
a persisted device id creates/looks up one account, and the session token is a
Bearer credential. Everything else mirrors `client/src/account.ts` +
`client/src/ui/profileUi.ts`.

### 42.2 Server

- `POST /api/auth/device` (`server/src/api/routes/user/auth/device.ts`):
  validates `deviceId` (`[A-Za-z0-9_-]{6,128}`), finds the user by
  `authId = "device:<deviceId>"`, creates one on first use via
  `createNewUser` (generated `Player xxxxxx` + unique slug, default unlocks,
  `linked: false` - no external provider), and answers
  `{ "success": true, "token": "<session>" }`. It inherits the AuthRouter's
  database and 5/min/IP rate-limit middleware.
- `authMiddleware` accepts the token as `Authorization: Bearer <session>` as
  well as the web's httpOnly `session` cookie. `setSessionTokenCookie` now
  returns `{ session, token }` so device login can put the token in the body.
- `generateAvailableUsername()` was extracted from `handleAuthUser` so OAuth and
  device users share the same "Player xxxxxx" collision loop.
- CORS allows the `Authorization` header (harmless for web, correct for any
  non-cookie client).

### 42.3 Java client

| File | Role |
|---|---|
| `AccountManager.java` | device id + session + profile state machine; profile/username/logout/reset/delete/loadout calls; listeners |
| `ProfileFragment.java` | account screen: name (first-login prompt + cooldown), reset stats, delete, log out/sign in |
| `SurvApi.java` | `deviceLogin` + `userPost` (status-aware `StatusCallback`, Bearer header) |
| `JavaUiHost.java` | owns/hides the profile screen, back handling, one-time name prompt, auto-close after naming |
| `MenuFragment.java` | top-bar account button, account error line, player-name mirroring |
| `LoadoutFragment.java` | pushes the loadout on close (`AccountManager.pushLoadout`) |

Key behaviours:

- Credentials live in the `survev_account` SharedPreferences, deliberately not
  in `SurvBridge` config: `setConfigValue` parses numeric strings as floats, so
  an all-digit ANDROID_ID would be corrupted. ANDROID_ID is the stable id
  (random UUID fallback) - **reinstall resolves to the same account**.
- Startup: existing token -> `/api/user/profile`; 401 -> clear + device login;
  no token -> device login. A network failure leaves the token and offers
  "Retry sign-in" in the profile.
- Log Out invalidates the server session, clears the token and latches
  `signedOut` so the next launch does not silently log in again; Sign In
  clears the latch and device-logs in.
- Delete Account posts `/api/user/delete`, then device-logs in again, creating a
  fresh account (the device id outlives the user) and prompting for a name.
- `/api/user/profile` returns the account loadout; it is applied through
  `SurvBridge.setLoadout` (the C++ validator still normalizes it). Loadout edits
  are pushed once when the loadout screen closes (avoids the 40/min endpoint
  limit while dragging the crosshair controls).
- The menu name field (`playerName`) is separate from the account name, like the
  web. It is auto-filled while empty and follows renames only while
  `playerNameAccount` still matches it; editing the field unbinds it. The
  first login opens a "Set your account name" dialog (never during a match) and
  closes the profile screen once the name is set.

### 42.4 Verification

- `pnpm --filter @survev/server typecheck`, root `pnpm lint:ci` green.
- `surv_tests`: 163 tests / 2,599 assertions, 0 failures.
- x86_64 `libSurvevMobile.so` + `SurvevMobile-debug.apk` (41 Java sources);
  `logcat -b crash` empty.
- Live API (dev `tsx watch` on `:8000`, bundle Postgres):
  - `POST /api/auth/device` twice for one id -> same user (same slug);
    invalid id -> `400 {"success":false,"error":"invalid_device"}`.
  - `Bearer` profile: 200 with profile/items/loadout; bogus token -> 401.
  - username: `success`, then `change_time_not_expired`; profile shows
    `usernameSet=true` and the 10 day counter.
  - logout invalidates the token (profile 401); delete removes the user and the
    next device login creates a new slug; test accounts cleaned up afterwards.
- Emulator (`x86_64`, `adb reverse tcp:8000`), UI dumps via `uiautomator`:
  - first launch: device id + login, "Set your account name" dialog; entering a
    name sets it and the profile screen closes; the menu field follows it.
  - profile: name/slug/cooldown ("Name change available in 9d 23h"), reset,
    delete, log out all render and work; Log Out -> "Not signed in" + Sign In;
    Sign In restores the name; delete (typo `DELETE`) validates then recreates.
  - restart keeps the session (`profile loaded` only, no device login);
    signed-out restart logs `device login skipped (signed out)`.
  - uninstall/reinstall recovers the same account through ANDROID_ID.

## 43. Session 42 - M8 quests/battle pass + Java localization (2026-10-25)

### 43.1 Scope

The remaining M8 data surface (`client/src/ui/pass.ts`): real pass level/XP,
the next unlock item, both server quest slots and the reroll flow. The old
`PassFragment` was a static mock (invented 50-tier rewards, fake quests and a
demo premium purchase) and the web client has no premium purchase at all.

### 43.2 Pass + quests

- `AccountManager.getPass(tryRefreshQuests, cb)` calls
  `/api/user/get_pass` with the Bearer session and parses
  `{ pass: {type,level,xp,unlocks,newItems}, quests: [...] }` into
  `PassInfo`/`QuestInfo`; `refreshQuest(idx, cb)` posts
  `/api/user/refresh_quest` and reports the server's `success`.
- `tools/codegen_pass_defs.mjs` generates
  `proj.android/app/src/dev/axmol/app/GeneratedPassData.java` from the real
  `shared/defs` (`PassDefs`, `QuestDefs`, `passUtil.getPassMaxLevel`):
  per-level XP curve, sparse level -> item unlock table, quest XP and the
  `timed` flag. Display names are l10n (`pass_*`, `quest_*`, `game-<item>`).
- `PassFragment` was rewritten around the real data:
  - pass name + `LEVEL n` + `xp/next XP` bar (two weighted segments, so the fill
    is `xp/next`, not `ratio/(1+ratio)`);
  - "Next reward (Lv n+1): <item>" using `getNextPassUnlockItemId` semantics;
  - quest rows with name, XP, progress bar/counter, completion state and the
    web's refresh eligibility `(!rerolled && !complete) || expired`; a confirm
    dialog calls `refresh_quest` and re-fetches;
  - a 1 s ticker updates the quest countdown labels while the screen is open;
  - logged-out state shows the localized `quest-login-prompt` instead of fake
    data. Opening the screen always sends `tryRefreshQuests=true`.
- The demo premium purchase / claim buttons and the invented tier list were
  removed (the server's `set_pass_unlock` is a no-op, so there is nothing to
  claim).

### 43.3 Java localization fix

`UiBridge::l10nJson()` returned `"{}"` whenever `_menu == nullptr` - and Android
installs the bridge with `installUiBridge(nullptr, gameScene)` (no C++ menu).
Every Java label silently used its English fallback; the pass screen exposed it
as raw keys (`PASS_SURVIVR2`, `quest_club_kills`). The bridge now lazily builds
a standalone `uikit::Localization`:

- `l10n/en.json` registered eagerly, `l10n/<language>.json` loaded through
  `Files::readText` (the same loader MenuScene uses);
- locale from the config `language`; `setLanguage` updates the table **before**
  `config.setString`, so the config listener's Java refresh sees the new table.

### 43.4 Verification

- `surv_tests` 163 tests / 2,599 assertions, 0 failures; root `lint:ci`;
  x86_64 native + APK (42 Java sources) green; `logcat -b crash` empty.
- Live API (dev `tsx watch`, bundle Postgres), emulator `x86_64`:
  - pass open logs `pass loaded: type=pass_survivr2 level=1 xp=0 quests=2`;
    the UI shows `SURVEVR PASS 2`, `LEVEL 1`, `0/50 XP`,
    `Next reward (Lv 2): Rainy Day`, `Kill enemies at: Club` (+40 XP) and
    `Destroy furniture` (+30 XP);
  - tapping Refresh -> confirm rerolls idx 0 (`Kill enemies at: Club` ->
    `Kill enemies at: Docks`), the row shows "Rerolled once this season." and
    loses its Refresh button;
  - after Log Out the pass screen shows `LEVEL -` and the localized
    `Log in to get quests!`; Sign In restores the quests.

## 44. Session 43 - M8 team links + the real team protocol (2026-10-25)

### 44.1 Why the team screen had to be rebuilt

The Java team screen called `/api/team/create` and `/api/team/join`, which the
server does not have, connected to a fabricated `ws://host:9001/team/<id>`, and
parsed invented `roster`/`playerJoined` messages. The real protocol is the
`/team_v2` JSON WebSocket on the **API** port (`server/src/teamMenu.ts`,
`shared/types/team.ts`). Invite links could not work on top of that, so the
screen was ported to the real protocol before adding links.

### 44.2 Client

- `TeamSocket` was rewritten without any WS library (the Gradle-free APK vendors
  none): raw `Socket`/`SSLSocket`, HTTP Upgrade with `Sec-WebSocket-Key` +
  Accept validation, masked client frames, server-frame parsing with
  continuation reassembly, ping/pong/close, and a writer thread/queue. This
  also fixes `NetworkOnMainThreadException` (send is called from UI actions) and
  the old close-frame buffer overrun; `android.util.Base64` keeps API 24.
- `TeamFragment` now speaks the real schema:
  - `create`/`join` on open (`roomUrl` without `#`), player name from config;
  - `state` -> room code/link, region, auto-fill, mode/max players, roster,
    leader, in-game/finding flags, `lastError`;
  - leader controls send `setRoomProps` (region cycle, mode cycle, auto-fill),
    `kick` and `playGame` (version 1028 + region/zones);
  - `joinGame` -> `SurvBridge.onJoinUrls(urls, joinToken)` and the match starts
    through the normal C++ path;
  - `kicked`/`error` -> localized message and back to the menu; `keepAlive`
    every 10 s.
  - the leader/non-leader Play states follow the web ("Waiting for leader",
    "Finding game...", captcha-disabled).
- The team socket stays connected through the match (`hideForGame`) so
  `JavaUiHost.returnToMenu` can re-show the room and send `gameComplete`; other
  paths (Leave, BACK, menu) close it.

### 44.3 Invite links

- `TeamLinks` builds `https://<webBaseUrl>/#CODE` (config `webBaseUrl`, default
  `https://survev.io`), which browser players can open. Copy puts the link on
  the Android clipboard; Share opens an `ACTION_SEND` chooser with the link.
- The join field accepts the web link, `survev://team/CODE`,
  `survev://team?code=CODE`, `#CODE` or a bare code
  (`extractCode`). The manifest adds a `VIEW`/`BROWSABLE` filter for
  `survev://team`; `AppActivity` handles both cold-start and `onNewIntent`
  (`singleTask`) and calls `JavaUiHost.openTeamInvite`, which opens the join
  screen for that code (ignored while in a match).
- `server/src/teamMenu.ts` accepts the device session as
  `Authorization: Bearer` (the native WS client has no cookie jar), so team
  players keep their account stats just like the web.

### 44.4 Verification

- `surv_tests` 163 tests / 2,599 assertions; `lint:ci`; server `tsc`; x86_64 APK
  (43 Java sources) green; `logcat -b crash` empty.
- Emulator + dev API (`/team_v2` on :8000):
  - Create Team -> `state` with `#5ifA`, link `https://survev.io/#5ifA`,
    `Team Members (1/2)`, Play enabled; a second Node WS client joined and the
    device roster updated to `2/2` with the joiner.
  - Mode cycle on the leader (Squad/4 -> Duo/2) sent `setRoomProps`; a client
    present in the room received the updated `maxPlayers`; Kick sent the
    target `kicked` and the roster returned to 1/2.
  - `adb shell am start -a VIEW -d survev://team/ukpi` -> `onNewIntent` ->
    Join Team with `NodeLeader` (crown) + `RenamedPlayer (YOU)`, Play disabled
    with "Waiting for leader".
  - Paste `https://survev.io/#ukpi` into the join field -> same room/roster.
  - Play -> `joinGame` -> match `4614` connected and received updates; the team
    UI hid while the socket stayed open.

## 45. Session 44 - render tiers + soak instrumentation (2026-10-25)

### 45.1 Battery/render tiers

The single "Low GFX" toggle is now a four-step surface scale in
`RenderScale.java`:

| Tier | Scale | Pixels vs Full (16:9) | Pause button |
|---|---|---|---|
| Full | 1.00 | 100% | `GFX: Full` |
| Medium | 0.85 | ~72% | `GFX: Medium` |
| Low | 0.75 | ~56% | `GFX: Low` |
| Potato | 0.60 | ~36% | `GFX: Potato` |

- `RenderScale.tierName`/`next` drive the pause dialog's `GFX: <tier>` button
  (`GamePauseDialog`), which cycles and persists `renderScale` through
  `SurvBridge.setConfigValue` + `saveConfig`; the button is green below Full.
- `AndroidUiBridge::setScenes` now calls `RenderScale.apply` **unconditionally**
  at startup (previously only when <1): Full therefore clears a stale fixed
  surface from a previous launch, and every launch logs
  `[perf] render scale <scale> at startup` (C++) and
  `render tier <name> [scale] (surface from view)` (Java).
- `RenderScale.apply` reports the tier name and both resolutions in logcat, so a
  device run shows the fill-rate change directly.

### 45.2 Soak instrumentation

With `debug.surv_debug=1`, `GameScene::update` keeps the 5 s
`[perf] fps=... frame=WxH` line and adds a 60 s summary built from those 5 s
samples (12 per window, so per-frame stalls do not distort it):

```
[perf] soak 60s fps min=57.6 avg=59.3 max=60.3 frame=816x459
```

If the average is below 45 fps it also logs a hint to choose a lower GFX tier.
Frame-size changes are logged as they happen (tier changes or a resize).

### 45.3 Verification

- `surv_tests` 163 tests / 2,599 assertions; `lint:ci`; x86_64 native + APK
  (43 Java sources); `logcat -b crash` empty.
- Emulator (960x540 view) with `debug.surv_debug=1`:
  - Pause dialog cycles `GFX: Full -> Medium -> Low -> Potato -> Full` and logs
    `render tier Medium 0.85 (816x459 from 960x540)`, `Low 0.75 (720x405)`,
    `Potato 0.6 (576x324)`, `Full (960x540)`, each followed by
    `[perf] frame size ...`.
  - Force-stop/restart with Medium persisted: `[perf] render scale 0.85 at
    startup` + `render tier Medium ...` before the first frame; the saved Full
    restores `960x540`.
  - 60 s solo soak at Medium: `fps min=57.6 avg=59.3 max=60.3
    frame=816x459` (emulator, not representative of real mid-range hardware).
- Still to do: validate the soak on a physical mid-range device (the only
  remaining perf/polish item) and the store-ready AAB.

## 46. Session 45 - store-ready release APK + Play AAB (2026-10-25)

### 46.1 Release APK (`tools/build-apk.ps1 -Release`)

- Parameters: `-Release`, `-Abis` (default `arm64-v8a` for release,
  `armeabi-v7a,arm64-v8a` for debug), `-VersionName`, `-VersionCode`,
  `-ApiUrl`, `-KeystorePath/-KeystoreAlias/-KeystorePassword/-KeyPassword`
  (env fallbacks `SURVEV_KEYSTORE`, `SURVEV_KS_ALIAS`, `SURVEV_KS_PASS`,
  `SURVEV_KEY_PASS`), `-Output`.
- Version defaults: repo `package.json` `0.4.3` -> `versionName=0.4.3`,
  `versionCode=403` (major*10000 + minor*100 + patch); overridable for Play.
- `-ApiUrl` replaces the manifest `surv_apiUrl` meta-data (release default
  `https://survev.io`). `SurvApi.initialize(Context)` now reads that meta-data
  once before the Java UI is built, so `SurvApi.apiBaseUrl()` drives
  `site_info`, `find_game`, accounts/pass and `teamWsUrl`; the C++ side already
  used the same key. Debug/manifest default is `http://127.0.0.1:8000` (matches
  `adb reverse`).
- Release packaging strips `libSurvevMobile.so` + `libopenal.so` with
  `llvm-strip` (copies in `build-apk/stripped/<abi>`), signs with the upload
  keystore (secrets via `apksigner`'s `env:` prefix), prints the signer
  certificate, and keeps the `resources.arsc` uncompressed/aligned check.
- Debug output/signing is unchanged (`SurvevMobile-debug.apk`, debug keystore).

### 46.2 Play AAB (`tools/build-aab.ps1`)

Gradle cannot run here, and aapt2 itself does not emit AABs, so the script
constructs bundletool's module zip and calls `bundletool build-bundle`:

- builds a proto manifest/resource table with
  `aapt2 link --proto-format` (same package/extractNativeLibs/API/version
  substitutions as the APK);
- copies `classes*.dex`, `assets/**` and `lib/**` from the **release APK** (so
  the dex/libs/assets are the exact store build), moves the proto manifest to
  `manifest/AndroidManifest.xml`;
- writes `base.zip` with forward-slash entry names (`.NET`'s
  `CreateFromDirectory` emits `\` on Windows and bundletool rejects that);
- runs `bundletool build-bundle --modules=base.zip --output=...` and optionally
  JAR-signs the AAB with `jarsigner` (upload key).
- Version/API/ABIs are read back from the release APK unless overridden;
  `bundletool-all.jar` is external (`-BundletoolJar`/`BUNDLETOOL_JAR`).

### 46.3 Verification

- `surv_tests` 163 tests / 2,599 assertions; `lint:ci`; server `tsc`.
- x86_64 release APK (debug keystore for testing, `-ApiUrl
  http://127.0.0.1:8000`): installs, `SurvApi: api base url
  http://127.0.0.1:8000`, account/profile/site_info work; `aapt2 dump badging`
  shows `versionCode='403' versionName='0.4.3'`, and the manifest holds the
  substituted `surv_apiUrl`.
- arm64 native build (full engine) + release APK (`-ApiUrl
  https://survev.io`, 42.9 MB, stripped, arm64-only, signer printed). Launching
  the x86_64 release build logs both `SurvApi: api base url
  https://survev.io` and `[net] api base https://survev.io`, proving the Java
  and C++ halves resolve the same store origin.
- arm64 AAB (29.6 MB signed / 29.4 MB unsigned) contains
  `base/manifest/AndroidManifest.xml`, `base/resources.pb`, `base/dex/classes.dex`
  and `base/lib/arm64-v8a/*`; `bundletool validate` accepts it and
  `build-apks --mode=universal` produces an arm64 APK (`versionCode 403`).
- x86_64 AAB built earlier from the x86_64 release APK produced a universal APK
  that installed and ran (account/site_info), proving the bundle pipeline.
- The test signatures use the debug keystore; a real upload needs the production
  upload keystore/passwords (never committed).

## 47. Session 46 - active layer + interpolation (P0.2/P0.3, 2026-10-25)

### 47.1 Bunker/cellar rendering

`Renderer::setActiveLayer` had no callers, so every frame ran with `layer = 0`:
`layers[1]` stayed transparent, `groundAlpha` never rose, and the stairs mask was
never released. `GameWorld::update` now mirrors `game.ts m_processGameUpdate`:
the active player's `layer` and `Map::isUnderground(pos, layer, rad)` are applied
to the renderer (and `AudioManager`) **before** `renderer.m_update`, so the layer
alpha, ground fade, mask transition, obstacle `activeLayer` checks and audio
filtering all use the same frame's state.

### 47.2 Player/camera/projectile interpolation

- `Player` gained `visualPos`/`visualPosOld`/`posInterpTicker` and
  `visualDir`/`visualDirOld`/`dirInterpolationTicker` seeded in `m_updateData`
  exactly like `player.ts:508-515` (duplicate updates re-seed from the current
  net position). The container, aura container and `bodyContainer` rotation use
  the visual values.
- `Projectile` gained `visualPosOld`/`posInterpTicker`; `ProjectileBarn::update`
  lerps the render position over `camera.m_interpInterval` like
  `projectile.ts:383-395`.
- The camera follows `activePlayer->visualPos` (game.ts:426).
- `config.interpolation` is applied to each new world
  (`GameScene::setInterpolationEnabled` + `UiBridge` startup/setConfigValue).
- Bullets, planes, airdrops and dead bodies are left on their own per-frame
  timing: the web client does not interpolate them either.

### 47.3 Verification

- `surv_tests`: **175 tests, 2,750 assertions, 0 failures** (new
  `player_visual_pos_interpolates`, `projectile_visual_pos_interpolates`,
  `world_applies_active_layer_and_underground`).
- `build-native.ps1 -Abis x86_64`: links; `build-apk.ps1 -Abis x86_64` packages.
- Device (x86_64 emulator): quick-play joined a match and ran for 12 s with
  `logcat -b crash` empty. Bunker entry and an interpolation on/off comparison
  are still pending.

## 48. Session 47 - P1.3 render feedback parity (2026-10-25)

- `bullet.ts`: real travelling tracer sprite (per-def palette/width/length,
  AP/chambered/saturated, suppressed fade), obstacle/player segment collision,
  wall dust + def bullet hit group, parented blood/pan sparks, stairs layer.
- `explosion.ts`: the `ExplosionEffectDefs` table moved to
  `shared/defs/explosionEffectDefs.ts` and is generated into
  `ExplosionEffectRenderDef`; `ExplosionBarn` now spawns burst/scatter physics
  particles, water ripples, camera shake and indoor bomb suppression instead of
  the old orange circle.
- `projectile.ts`: spin/drag, trail sprite (length/width/alpha/tint), water
  alpha, stairs z-order and always-on-top bomb_iron.
- `loot.ts`: generated outer ring (border/borderTint), inner scale/rotation/
  mirror.
- `player.ts`: same-group-only nametags, faction patch, last-stand/bleed
  particles.
- `airdrop.ts`/`plane.ts`: retained fall loop, plane engine loop, airstrike
  zones instantiated from `UpdateMsg.airstrikeZones`.
- Rotation convention: PIXI radians are passed unchanged to
  `AxmolPixi::setRotation` (see the player body). Bullet/flare/projectile/plane
  and loot rotations no longer negate the web numeric; negating mirrored
  diagonal tracers into a "slash" and put the projectile trail in front of the
  throw.
- Player gun grip: the web `gunBarrel.anchor=(0.5,1)` is PIXI bottom-centre.
  Axmol anchors originate bottom-left, so the native equivalent is
  `(0.5,0)`; the old direct `(0.5,1)` put the grip at the muzzle and turned
  the held gun 180 degrees around the hand.

Verification: `surv_tests` 187 tests / 2,846 assertions, 0 failures; client
vitest 11,768 pass; `lint:ci`; x86_64 native + APK; device quick play joined with
an empty crash buffer.

## 49. Session 48 - P1.4 live HUD completion (2026-10-25)

- Interaction prompt for the nearest loot on the active audio layer, and the
  prompt text itself is a touch target (tapping it sends Interact + Cancel, like
  ui.ts `interactionElems`) in addition to the action-bar USE pill.
- Pickup-error toasts (`PickupMsg` types), rare-loot/perk toasts, local
  `YOU KILLED <name>` messages, and six individually coloured kill-feed rows.
- Faction `R n / B n` alive counts, off-screen teammate arrows, role
  announcements, waiting-for-players text, and pie-timer action labels.
- Host tests cover the prompt (text + touch target), announcement expiry,
  faction counts, waiting text, pickup error callback, and local kill
  feed/toast.

Verification: `surv_tests` 190 tests / 2,870 assertions, 0 failures; x86_64
native and APK builds installed and launched with an empty crash buffer.

---

## 50. Session 49 - full HUD rework to the mobile mockup layout (2026-10-28)

Reworks the live HUD (`game/Hud.{h,cpp}`) to the requested touch layout (the
web client's mobile layout as the reference): settings gear top-left, minimap
under it (tap = big map), gas/alive/kills info bar, scope strip on the left,
messages centred at half a minimap down from the top with the reload progress
bar above them, top bag counters, per-ammo column, weapon cards on the right,
and the gear + emote/ping wheels repositioned.

### What changed

1. **Settings gear (inside the minimap's top-left corner), opens the game
   menu.** `TapKind::Settings` on the 44px `#ui-menu-display`-style box routes
   to `uiHost()->togglePause()` (the Java `GamePauseDialog`, same path as
   BACK); hosts without the bridge get a Hud panel (RESUME/LEAVE,
   `showGameMenu()` + `menuActionAt()`). The icon uses the direct
   `gui/cog.png` file because the HUD builds before the map atlases are
   registered. It is created after `_miniMap.build()` so it draws over the map
   texture at the minimap's (0,0).
2. **Minimap pinned to the top-left (3,3); tapping it opens the big map.** The
   old 22px expand button is gone; `minimapTapHit()` covers the whole 140px
   box. The info bar follows the map (`kInfoBarTop = kMinimapTop +
   kMinimapSize + 4`).
3. **Gas timer + alive count (+ kills) in an info bar under the minimap**
   (mirrors `.ui-map-wrapper-mobile #ui-map-info`). The gas icon and `0:00`
   are visible from the first frame, before the first zone starts.
4. **Scope strip on the left side** (`#ui-top-center-scopes-wrapper` mobile):
   one pill per owned scope (1x…15x), tap equips the tapped scope via
   `InputActions::use(type)` (game.ts "use" scope), long-press drops it. The
   equipped one is highlighted. The old scope-cycle pill is gone.
5. **Messages ("Waiting for players...", announcements) horizontally centred,
   half a minimap down from the top** (`kMessageTop = kMinimapTop +
   kMinimapSize * 0.5`). The kill-leader line stays hidden like the web's
   `hide-on-mobile`.
6. **Reload/use/revive progress bar above the messages** (pieTimer.ts port,
   now a Graphics bar + label at `kActionTop`). The FairyGUI `PieTimer`
   (`ui/FguiPieTimer.*`) is no longer built or updated per frame — that path
   rasterised its counter/label text every frame and is the most likely source
   of the on-device "large string flickering" report. Remaining per-frame HUD
   text writes were also change-gated (health number, ammo/loot counter
   strings) so they only re-rasterise on real changes. Needs a re-test on
   device to confirm the flicker is gone.
7. **Interact prompt next to the item.** `GameScene` feeds the nearest loot's
   screen position (camera `m_pointToScreen` → `screenToDesign`) into
   `setInteractionPrompt(text, designPos)`; `Hud::updatePrompt()` clamps it and
   puts a 52px finger-press button (`gui/tap.png`) plus the item-name label
   beside the loot (flipped to the left near the right edge). Tapping either
   still sends Interact + Cancel; the USE pill is gone.
8. **Top bag counters: bandage / medkit / soda / pills** with loot-def icons,
   tap = use, long-press = drop (`TapKind::LootUse`).
9. **Ammo column top right** (`#ui-ammo-interactive`): one row per owned ammo
   type with the count and a per-type colour chip (game.css overlay colours).
10. **Weapon cards on the right** (primary/secondary/fists/throwable)
    including loot-def icons; tap = equip, hold = drop, drag = aim handoff
    unchanged.
11. **Emote button in the bottom-right corner** (`EmoteWheel::setButtonPosition`,
    wheel centre clamped into the safe box; `EmoteWheel::wheelContains()`).
12. **Ping button right of the health bar** (`PingWheel::setButtonPosition`).
    Pressing outside an open emote/ping wheel ring now closes it instead of
    moving/aiming (GameScene outside-touch handler).
13. **Airdrop/ping markers are small again.** `MapView::setPings()` no longer
    multiplies the ping scale by the 256-based `artScale` (the minimap factor
    was 0.55 and the big map factor 2.5, i.e. the icons were drawn ~3-6x too
    large); it now fits each sprite to `ping.scale * kPingDisplayPx (36px)`,
    matching `ui.ts createPing`. Player map pings use the web's raw 0.15 Sm
    scale. The world edge indicators (airdrop direction arrow) were shrunk
    from 0.4/0.5 to 0.22/0.28 in `EmoteManager`. Their scale is now also
    converted from screen space to the camera-transformed world root each
    frame (`camera.m_pixels(kPingEdgeScale) / camera.m_z()`); applying the raw
    screen scale at creation made the orange arc/airdrop icon `m_z` times too
    large (it covered the weapon cards).
14. Gear/perk slots moved beside the health bar (web `#ui-bottom-center-left`),
    squad rows under the scope strip, pickup toast under the messages.

### Wiring/verification

- `GameScene` feeds the action progress (`setActionProgress`), prompt
  position, and routes the new touches (gear, minimap tap, bag counters /
  scope slots via long-press drop, wheel outside-close). `GamePauseDialog`
  handles the settings open.
- Host tests updated to the new layout geometry (reload/swap pills, weapon
  cards, bag counters, scope strip, prompt target, minimap tap, ping scales);
  `HelpModal.java` touch-rows rewritten for the new controls.
- `surv_tests`: 233 tests / 3,913 assertions, 0 failures.
- x86_64 emulator verification against the bundle server (local bots fill the
  match): the HUD renders per the mockup — gear, minimap + info bar, gas
  countdown, scope pills, centred messages, weapon cards with item icons,
  ammo chip counts, health/boost, emote + ping buttons. Interactively
  verified: the minimap tap opens the big map (X closes), the gear opens the
  Java pause dialog, holding the emote button opens the loadout wheel and
  dragging into a wedge sends the emote (thumbs-up above the player), holding
  the ping button opens the team-ping wheel (Danger/Coming/Help/Medical/Ammo),
  picking up a gun fills the weapon card + ammo chip and adds its scope to the
  strip. Both wheels gained an intent guard in `select()`: a wedge only sends
  when the finger is inside the ring and moved past `kDeadZone` from the press
  point, so the button press can't send by itself (emote.ts drags into the
  wedge). `logcat -b crash` empty. Not reproducible here: airdrop markers
  (needs a server airdrop; scale fix covered by tests) and a final on-device
  confirmation that the per-frame text changes removed the "large string
  flickering".

## 51. Session 50 - touch-select wheels: tap-to-send, sticky wheel (2026-10-29)

Replaces the drag-to-select emote/ping wheel flow requested by playtesting:

1. **The wheels are no longer tied to the touch that opened them.** The button
   press opens the wheel and it STAYS open after that touch lifts
   (`GameScene::onTouchEnded` only releases the `_emoteTouchId`/
   `_pingTouchId` claim now; `_pingTargetValid` also survives so a big-map
   ping target reaches the next tap).
2. **Tap-to-send.** `EmoteWheel::select()` -> `EmoteWheel::tap()` /
   `PingWheel::tap()` (return `TapResult` None/Close/Send): a tap on a usable
   wedge sends instantly (throttle, team-only and greyed gates unchanged); a
   tap on the centre close selector closes; unusable taps (empty wedge,
   team-only in solo, hard cooldown) leave the wheel open and swallow the
   touch. The old "moved past `kDeadZone` from the press point" intent guard
   and `_pressPos` tracking are gone.
3. **Tap outside dismisses.** Any touch outside all open rings closes them
   and is swallowed (cannot move/aim behind the wheel). The wheel buttons sit
   outside the rings, so pressing a wheel's own button while it is open
   dismisses it (toggle); pressing it while closed opens it. Opening one
   wheel closes the other if the tap was outside its ring.
4. **Emote button icon is the static gas icon** (`Content/gui/gas.png`,
   `EmoteWheel::kButtonIconFrame`) instead of the first loadout emote sticker.
   The ping button keeps the danger-ping icon.
5. **1.5x wheels at the screen centre.** `EmoteWheel::kGeometryScale` and
   `PingWheel::kGeometryScale` (1.5) multiply the ring/selector geometry
   visually (container scale, open ease: 0.85*1.5 -> 1.5) and hit tests
   (`tap()` divides the tap offset back into selector-space,
   `wheelContains()`/`open()` use the scaled radius for the clamp). GameScene
   opens both the emote and the ping wheel at the design centre (640x360),
   not around their buttons; the buttons stay in place.
6. **Crosshair removed.** The M7 always-on screen-centre crosshair overlay
   (`_crosshairSprite`/`_crosshairFallback`, drawn over the player's
   position) is gone per playtesting; wheels/pings mark the aim instead.
7. **Crosshair pings sit just above the head.** A ping wheel send without a
   big-map target targets `activePlayer` + `PlayerConfig::radius * 2.0f` on
   y (the web draws pings exactly at their pos; the touch flow pings the
   camera centre, so the raised position keeps the message off the body).
8. **Bigger game pads, slightly up/inward.** `Touch::padSizeScale` (1.75)
   multiplies the drawn ring/thumb scales only — the web sizes its pads in
   device CSS px, so on the phone they are proportionally larger than in the
   fixed 1280x720 design space; `padPosRange` (thumb travel, move/aim
   normalization) stays at the web value so control sensitivity is unchanged.
   The locked centres moved to fit: landscape (160, 140) instead of
   (126, 100), portrait (120, 176) instead of (96, 160) — a little above the
   bottom edge and a little further inside. Playtesting then doubled the
   visible gap between the drawn ring and the side/bottom screen edges, so
   the centres are derived from the ring radius + 2x the old margin (e.g.
   landscape ~210.8, ~170.8 around the 104px-radius ring).
9. **Team arrows sit on the screen edge and use `indicator.svg`.** The port
   used to draw a small custom chevron `dir * 300` from the (centred) HUD
   origin, so an off-screen teammate's indicator hovered next to the local
   player's head, and its rotation matched the art's local +x axis instead of
   the web's up-pointing art. `GameScene` now mirrors `ui.ts 808-857`: the
   teammate's group slot is kept (sorted `groupInfo.playerIds` order), the
   arrow is shown only once `testCircleAabb` says the whole player radius has
   left the `camAabb`, the ray from the camera centre is clamped to a 16px
   inset (small/mobile layout), and the rotation is the web's
   `atan2(dir.y, -dir.x) - PI/2` for the up-pointing `/img/gui/indicator.svg`
   art. `Hud` renders the rasterised `gui/indicator.png` (32px on the small
   layout) plus `.ui-team-indicator-pos`'s 32x32 group-colour dot at 75%
   opacity, one node per group slot; faction mode suppresses the arrows and a
   big-map toggle cannot revive hidden slots. Covered by
   `hud_team_arrow_positions` and `hud_team_arrow_rotation`.

Tests updated to tap semantics (`emote_wheel_tap_selects_and_closes`,
`ping_wheel_tap_selects_pings_and_team_emotes`, cooldown/solo gates, etc)
and to the 1.5x centred flow (`emote_wheel_geometry_scale_1_5x`).

## 52. Session 51 - held-melee placement + team socket loss (2026-10-29)

### 52.1 Melee weapons were placed outside the hand

`player.ts updateRotation` sets `meleeSprite.position = -MeleeR.pivot` and
`meleeSprite.pivot = -(MeleeR.pos + worldImg.pos)`; PIXI applies the sprite's
own scale to the pivot, so the rendered offset is `-R*scale*pivot`. The port
added the pivot unscaled and with the wrong sign, which put the woodaxe (and
other melee defs with a non-zero `worldImg.pos`) away from the hand.
`PlayerRender.cpp` now computes `position - R*(scale*pivot)`; the woodaxe idle
offset is `(3.6341, 6.1069)` in the hand frame. Host test
`player_melee_hold_position` pins the woodaxe position/rotation/scale.

`EmoteManager::kPingEdgeScale` is public so the pass-50 edge-scale test
compiles (it was left private). Host tests: 4007 assertions / 240 tests,
0 failures.

### 52.2 Team room socket closed during a match

`TeamFragment.handleSocketError` used `returnToMenu`, which re-showed the dead
room over the still-running match. It now mirrors `teamMenu.ts` while
`joiningGame`: `JavaUiHost.dropTeam()` forgets the room without switching
screens, the independent match socket keeps playing, and the match-end
callback opens the main menu. A localized toast reports the lost room.

## 53. Session 52 - HUD action bar placement/lifetime (2026-10-29)

The commit that replaced the pie timer with the HUD action bar
(`9a85af1e`, "rework the hud, emote wheel and ping wheel interactions")
left a bordered box sitting on the player whenever a reload/use/revive
action ran. Two bugs combined:

### 53.1 The action bar background was drawn at the HUD origin

`Hud::paintBox()` positions its node from the rect it draws (that is how
the settings box, the touch prompt and the counter rows place themselves),
but `setActionProgress()` passed `Rect{0, 0, 240, 14}`. The background node
therefore jumped to the centred container's origin - which is the screen
centre, i.e. the player - while the fill and label stayed at the top
centre. The 240x14 background is exactly the long thin bordered box players
saw next to the player. `Hud` now keeps the absolute `_actionBarRect` set
in `build()` and paints from it; host test
`hud_action_bar_background_stays_above_the_messages` pins its origin rect.

### 53.2 The bar never advanced and never disappeared

`activePlayerData.actionDirty` only carries the action *start*; the server's
`cancelAction()` clears the dirty flag (the end reaches clients through the
player object's `actionType`/`actionSeq`, which `setDirty()` sends as a full
object). The port only read `actionDuration` and `actionTime`, so the bar
stayed at 0 progress and remained on screen for the rest of the match after
the first action. `GameScene` now mirrors `PieTimer`: it gates the bar on
the active player's `actionType`, advances the elapsed time locally, and
restarts it from the player's `actionSeq` (added to `Player`). The bar is
also cleared when the match ends, the local player dies or spectating
starts. Verified on the x86_64 emulator (reload fills the top bar, then it
disappears). Host tests: 4019 assertions / 241 tests, 0 failures.

## 54. Session 53 - pickup prompt styling (2026-10-29)

The near-loot prompt ("PICK UP <item>") now matches the web's
`#ui-interaction` / `#ui-interaction-outer` composition: the item name is
white instead of amber and sits in its own dark box (black 50% fill, white
border) beside the finger-press button, sized from the label (`10px` inner
padding, `8px` gap, `34px` tall). The name box is part of the same tap
target, and it flips to the left of the button near the right screen edge.

The prompt's screen clamp was also wrong: container coordinates are centred,
but `updatePrompt()` clamped `cx/cy` to `[+box/2, ...]`, so every loot left
of or above the player had its prompt pinned near the middle of the screen.
It now clamps to `+/- (design half - box/2)`. Tests:
`hud_prompt_announcement_and_faction_counts` pins the white fill, the name
box origin/size/visibility and the combined hit rect; `NullText` now records
`setColor` for that assertion. Host tests: 4030 assertions / 241 tests,
0 failures.

## 55. Session 54 - atlas trim offset sign (M1100 grip gap) (2026-10-29)

The M1100 (and other trimmed sprites) sat ahead of the hand when held:
the gun's grip was ~12 world units in front of the palm with the support
hand left behind on the pump.

The web renders these sprites through PIXI, which applies the spritesheet's
`spriteSourceSize` (trimmed rect top-left) in the texture's y-down space.
`tools/build-atlas.mjs` converts the same data to a cocos2d plist, whose
frame `offset` is the trimmed centre relative to the original centre in
**y-up** points, but it copied the y value unchanged:

    y: sss.y - (source.h - frame.h) / 2      // web (y down)
    y: (source.h - frame.h) / 2 - sss.y      // cocos (y up) - fixed

For `gun-m1100-01.img` (frame 22x91 of a 28x116 source, top trim 25) this
wrote +12.5 instead of -12.5, i.e. twice the trim distance (25 atlas px =
50 points at the 0.5 content scale, 12.5 world units at the 0.25 gun
scale) shifted the barrel forward. The same error moved every trimmed
sprite (guns, melee with trims, helmets/chests, loot) vertically in its
local frame; symmetric sprites (body, hands, woodaxe) were unaffected,
which is why the earlier melee fix held.

Fixed in `frameOffset()` and the atlas regenerated with
`node tools/build-atlas.mjs --res low` (all 1706 frames; Content/atlas is
generated, do not hand-edit). Verified on the x86_64 emulator: the M1100
grip now sits in the right hand and the support hand lands on the pump.
Host tests unaffected (4030 assertions / 241 tests, 0 failures).

## 56. Session 55 - equipped ammo counter above the health bar (2026-10-29)

The bottom-centre HUD now draws the web client's `#ui-equipped-ammo` block
(`client/index.html:559-569`, `game.css:2147-2195`, `ui2.ts:896-910` /
`1226-1246`) instead of the old `30/90` text to the right of the health
bar: a `175x56` bullet counter centred above the bar, with a `100x50` clip
box (40px bold), a `65x32` reserve box at left 82% / top 18px, and the round
reload button (`gui/bullets.png` at 20px inside a 32px `rgba(0,0,0,.4)`
circle) at left 212px / top 16px. The wrapper sits 62px above the bottom edge
plus the web's 12px margin, so the boxes land 80..130px above the design
bottom (unit-tested positions: clip `(-50,-280)`, reserve `(56,-280)`).

State follows `ui2.ts`: the clip is the equipped weapon's `ammo` (the
throwable slot counts the bag instead, `ui2.ts:863`), the reserve is
`inventory[gunDef.ammo]`, and infinite guns (`ammoInfinite` or the
`endless_ammo` perk unless the gun sets `ignoreEndlessAmmo`) print ∞.
Melee hides the clip; a 0 clip is red; an empty reserve hides the reserve box
and the reload button. The bullet icon replaces the old mobile `RELOAD`
pill but keeps `#ui-reload-button-container` as the `TapKind::Reload` hit
rect (`ui.ts:395` binds touchstart on it); the `SWAP` pill is unchanged.

`gunDefs.ignoreEndlessAmmo` was missing from the render defs, so
`src/render/Defs.h` and `tools/codegen_render_defs.mjs` now carry it and
`GeneratedDefs.cpp` was regenerated. Tests:
`hud_equipped_ammo_matches_web_client` (clip/reserve text and colours,
melee hiding, ∞ reserve, box origins, reload tap geometry) and
`gun_fire_cue_defs` (ak47 vs flare_gun `ignoreEndlessAmmo`); host tests
4059 assertions / 242 tests, 0 failures. Verified on the x86_64 emulator (the
boxes render centred above the health bar with the bullet icon).

## 56. Session 55 - emote/ping wheel labels removed (2026-10-29)

The emote wheel drew each loadout emote's name under its wedge and the ping
wheel drew `Danger`/`Coming`/`Help`/`Medical`/`Ammo` beside its selectors.
Both wheels are icon-based now: the `pix::Text` label nodes (and their
`labelOffsets`/`labelAnchors`/`kLabels` data and the `shortLabel()` helper)
are gone, so only the sprite art, coloured rings and highlight remain. Disabled
(greyed/team-only-in-solo) selectors still dim through the icon alpha. The
centre close selector keeps its `X` glyph. Host tests unchanged: 4059
assertions / 242 tests, 0 failures.

## 57. Session 56 - equipped weapon card highlight (2026-10-29)

The HUD already painted a brighter fill + 2px border for the selected weapon
card (`Hud::updateActionButtons`, keyed on `_equippedSlot`), but the
`9a85af1e` HUD rework dropped the `_equippedSlot = apd.curWeapIdx;`
assignment that fed it, so no card ever highlighted. `Hud::update()` sets it
again from `activePlayerData.curWeapIdx` (clamped to the live slot count;
-1 = no highlight), matching `ui2.ts:867-868 weaponState.equipped`. An unused
`ammoText` local from the same rework is gone. `hud_equipped_ammo_matches_web_client`
now asserts the equipped card's white fill/border and that a weapon swap moves
the highlight. Host tests: 4065 assertions / 242 tests, 0 failures.

## 58. Session 57 - pickup prompt follows the touch pickup range (2026-10-29)

The "PICK UP" prompt used `loot.rad + 2.5` as its reach, while the server's
`Player.getClosestLoot` accepts a touch player within
`player.rad + loot.rad * GameConfig.player.touchLootRadMult` (1.4). The prompt
therefore appeared up to ~1 unit beyond the pickup range (e.g. a 1.0-radius
loot: prompt at 3.5, pickup at 2.4), so tapping it did nothing. `GameScene`
now mirrors `client/src/objects/loot.ts m_update` and the server: the reach is
`PlayerConfig::touchLootRadMult` (new) through the shared helper
`touchLootReach`, plus the same owner filter (`ownerId == 0` or the local
player). Host test
`touch_loot_reach_matches_web_and_server` pins the radius.

The prompt's anchor also moved: it used to sit on the loot's own screen
position, now it follows the player, two body widths (`4 * rad`) to the right
of the player centre (same y), so the icon + "PICK UP ..." box sit beside the
body. `Hud::updatePrompt()` still clamps the icon into the design space and
flips the name box to the left near the right edge.

The prompt's name box is now the same height as the finger-press box
(`kPromptSize`, 52) and its text uses `Hud::kPromptFontSize` (24px, the web's
`#ui-interaction-description` size) so it fills the box; the box width is
estimated from that font size (0.6 x size per glyph). Verified on the x86_64
emulator. Host tests: 4071 assertions / 243 tests, 0 failures.

### 58.1 Weapon card art/name fill the card

The weapon cards kept 13px names and 30px icons in a 42px row. They now use
`Hud::kWeaponCardFontSize` (18px, the web's `.ui-weapon-name`) and
`Hud::kWeaponCardIconSize` (40px, just inside the 42px card). The HUD action
test asserts the 18px name font and that the icon size fits the row. Host
tests: 4073 assertions / 243 tests, 0 failures.

## 59. Session 58 - ammo column pill styling (2026-10-29)

`#ui-ammo-interactive` now matches the game.css `.ui-ammo` look: each row is a
rounded dark pill (`Hud::paintRoundedBox`, a convex corner-arc polygon; 6px
radius, no white border) with an 18px bold count on the left and a 16px
ammo-coloured chip on the right (web overlay colours, 0.75 alpha). Rows follow
ui2.ts:1269-1277 instead of hiding zero counts: non-special ammo stays visible
dimmed to 25% (`kAmmoZeroAlpha`) while its count is 0, `AmmoDef.special` rows
(`45acp`, `308sub`, `50AE`, `flare`) stay hidden until picked up. The hardcoded
overlay colours gained the web's 308sub/50AE values. Covered by
`hud_equipped_ammo_matches_web_client` (row text/alpha/visibility, 762mm vs
9mm/45acp, rounded polygon instead of a square rect, 18px count). Host tests:
4085 assertions / 243 tests, 0 failures.

## 60. Session 59 - top bag counter capsules (2026-10-29)

The four top bag counters (bandage / med kit / soda / pills) were 44x32 square
dark boxes that disappeared entirely at count 0. They are now 80x32 capsule
pills whose left edge starts at the local player's left edge (design centre
minus `kTopLootPlayerInset`, 26px) and grow to the right, matching the
reference art:

- Filled pill: rounded dark capsule (`rgba(0,0,0,0.4)`, 7px radius) with an
  18px count on the left and the 26px loot icon on the right.
- Empty pill: still on screen as a light ghost capsule (white at
  `kTopLootZeroFillAlpha` 0.16) with a dimmed, centred `0`
  (`kTopLootZeroTextAlpha` 0.6) and no icon, so the strip keeps its layout.
- Only filled pills are tap/hold targets (`LootUse`); ghost pills fall through.

`Hud::paintRoundedBox` now also draws the counter background (the old square
`paintBox` had a white outline). Covered by `hud_action_bar_shows_inventory_state`
(empty pill visible + dimmed, icon hidden, dark vs light fill colours, rounded
polygon, 18px font, ghost pill not tappable). Host tests: 4097 assertions /
243 tests, 0 failures.

## 61. Session 60 - SPECTATE on the game-over screen (2026-10-30)

The web client's result button is `ui.ts beginSpectating()`: it only sets
`specAction = SpectateAction.Begin`, and `game.ts` sends `SpectateMsg` with that
action (the server then picks the killer or the next alive player). The button
is added in `showStats` when the match is not over and players remain alive
(`gameOver || alive === 0` hides it). The native result panel had only LEAVE,
so a solo death (GameOverMsg with `gameOver == false`) could not spectate.

`Hud::showGameOver` now mirrors that condition (`_overCanSpectate`:
`!over.gameOver` and the live `aliveCounts()` sum non-zero) and paints
SPECTATE beside LEAVE; the touch path already routes
`OverlayAction::SpectateBegin` to `Game::sendSpectate(1)`. After the server
moves `activePlayerId`, the result panel is dismissed and the spectating
overlay (label / count / PREV / NEXT / MATCH STATS / LEAVE) takes over,
matching `ui.ts setSpectating -> hideStats`. New `Game::gameOverSeq()` lets the
HUD tell a fresh GameOverMsg from the latched one, so the dismissed panel does
not immediately reappear; a new message still reopens it, except for another
team's death while spectating a running match (`ui.ts showStats` line 1388
suppression). Final results (`gameOver == true`) keep the single centred LEAVE.

Covered by `hud_gameover_spectate_matches_web_client` (solo result buttons,
Begin -> spectating overlay + label, other-team result suppression, final
result back to centred LEAVE). Host tests: 4112 assertions / 244 tests,
0 failures.

## 62. Session 61 - spectate settings gear + match stats rows (2026-10-30)

Two spectate-mode regressions:

- **Settings gear.** `GameScene::onTouchBegan` returned early for
  `isSpectating() || isLocalDead()` before running `Hud::hitTest`, which also
  disables itself while spectating, so the top-left gear did nothing. The gear
  now has its own `Hud::settingsTapHit` (usable while spectating and while the
  HUD fallback menu is open, but not under the big map / result panels) and
  `GameScene` routes it before the dead/spectating early return through the new
  `openGameMenu()` helper (Java `GamePauseDialog`, or the HUD menu without a
  bridge). The same helper replaces the dead `Settings` case in the HUD-menu
  block, so tapping the gear again closes the fallback menu.

- **MATCH STATS blank panel.** The HUD's own `_localStats` copy was only filled
  in the no-GameOverMsg branch, so a solo death (where the result arrives as
  GameOverMsg) opened an empty panel. `Hud::update` now syncs
  `game.localStats()` into the HUD before the result/death state machine, and
  `refreshSpectateStats()` (re)writes the web-style `YOUR RESULTS` rows
  (KILLS / DAMAGE / TAKEN / SURVIVED) on toggle or when late stats arrive.

Covered by `hud_gameover_spectate_matches_web_client` (gear hit while
spectating, MATCH STATS text, title row) and the team death-panel test. Host
tests: 4120 assertions / 244 tests, 0 failures.

### 62.1 MATCH STATS text fits its panel

The panel's rows used a 15px font anchored to the panel's top-left, which read
undersized and could clip as the title/row count grew. The panel now exposes
`kSpecStatsPanelW/H` (250x190), `kSpecStatsPad` (14) and
`kSpecStatsFontSize` (18, the same size as the rest of the HUD's body text);
the text is left-padded and vertically centred (`anchor 0, 0.5`), so the five
`YOUR RESULTS` rows sit inside the box regardless of line count. The action
test pins the 18px font. Host tests: 4121 assertions / 244 tests, 0 failures.

## 63. Session 62 - ammo column drops the bullets icon (2026-10-30)

`#ui-ammo-interactive` rows no longer draw the shared `bullets.img` sprite
(it overlapped the right-hand ammo-colour chip and added noise at 26px). Each
row is now just the rounded dark pill, the 18px count on the left and the
ammo-coloured chip on the right. `AmmoRow::icon`/`lastFrame` and their
create/paint code are gone; the row geometry, zero-dim alpha and special-ammo
hiding are unchanged. Host tests: 4121 assertions / 244 tests, 0 failures.

## 64. Session 63 - scope strip label size (2026-10-30)

The scope pills (`1x` .. `15x`) are 48x34 but their labels were 15px, leaving
the text visually lost inside the box. They now use
`Hud::kScopeLabelFontSize` (20px, close to the web mobile `.ui-zoom-level`
proportion) and stay centred in the pill; `15x` at 20px is ~36px wide, so it
still fits the 48px width. The inventory test pins the label size. Host tests:
4122 assertions / 244 tests, 0 failures.

## 65. Session 64 - remove the dead C++ UI (2026-10-30)

Removed the UI implementations that the live app never instantiated:

- `src/ui/UiOverlay.{h,cpp}` — the old C++ HUD + pause menu (the pause menu is
  the Java `GamePauseDialog`).
- `src/ui/FguiHud.{h,cpp}`, `FguiEmoteWheel.{h,cpp}`, `FguiPieTimer.{h,cpp}`,
  `FguiPadGraphics.{h,cpp}`, `FguiText.{h,cpp}`, `FguiRoot.h`,
  `FguiOwnership.h` — the FairyGUI path, replaced by `game/Hud`,
  `game/EmoteWheel`/`PingWheel`, `PadGraphicsAxmol` and `AxText`.
- `GameScene::_overlay`/`setOverlay`, `MenuScene::_overlay`/`overlay()` and the
  dead `if (_overlay)` error/quit paths, plus the unused FairyGUI `GRoot` /
  `fguiRoot()` kept alive only for the widgets above.

`StartMenu` (previously declared in `UiOverlay.h`) now has its own
`src/ui/StartMenu.h`; the CMake source GLOB picks it up automatically. The live
paths are unchanged: `game/Hud`, the Java `GamePauseDialog`, and
`Hud::showGameMenu` as the no-bridge fallback. Host tests: 4122 assertions /
244 tests, 0 failures.

## 66. Session 65 - Java result screen (2026-10-30)

`Hud::showGameOver` no longer draws the C++ result panel. It only latches the
modal/spectate state, and `GameScene` mirrors `Hud::gameOverShown()` to the Java
`GameOverDialog` once per `Game::gameOverSeq()` via
`UiHost::showGameOver(Game::gameOverJson())`.

`GameOverDialog` follows `ui.ts showStats`: gold victory/defeat title
(`game-chicken`/`game-turkey`/`game-You` + `game-you-died`/
`game-team-eliminated`), the underlined `Solo|Duo|Squad Rank #n` overview
(plus Team Kills, or the faction Red/Blue counts), one 125px-style card per
`GameOverMsg.playerStats` entry with the gold italic name and the four stat rows
(`ui.ts humanizeTime`, e.g. `4m 14s`), and PLAY NEW GAME (purple, calls
`JavaUiHost.quitToMenu`) plus SPECTATE while the match is still running
(`SurvBridge.spectate(1)`). The scrim/cards/buttons match the Android result
reference; the C++ panel nodes remain for the team-death `showTeamAd` screen.

`L10n.table()` also stopped caching an empty table (the Java menu is built
before axmol's asset paths are ready, so the first lookup can see `{}`); this
is what put the result screen's rank label on its fallback.
