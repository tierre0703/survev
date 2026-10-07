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

## 7. Remaining rendering implementation (2026-10-07)

- **T4 skeletal pose/outfit rendering implemented** in
  `src/game/objects/PlayerRender.cpp`. The renderer now uses the outfit's
  `skinImg` (not the loot shirt), with body, separate hand/foot containers,
  backpack, chest, helmet, role visor, hip pan, melee, gun/magazine and throwable
  sprites. Equipment ordering, dual weapons, downed/revive hiding, ghillie
  colors, perk armor, player scale, stairs sorting and upright names are handled.
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

### Still open (carried over)

- **T4**: the skeletal pose/outfit render in `PlayerRender.cpp` remains the
  base + hands/feet + gear subset. Full `animData` bone/pose fidelity, and the
  browser-only player presentation (aura UI, submerge, frozen/team patches,
  animation-triggered collision/audio), are unchanged from §7.
- **T5**: the water island uses the canvas overpaint fallback on native; the
  destructive `AxGraphics` tessellation path (real polygon holes) is only used
  for rect masks.
- **No new host tests were added this session.** The object-render changes are
  verified on-device only; `surv_tests` stayed at the existing suite.

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

