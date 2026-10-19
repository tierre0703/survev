# M6 — Audio (implementation notes)

Port of the web client's audio stack onto `ax::AudioEngine`. Verified on the
x86_64 emulator (OpenAL/OpenSL initialize, game connects, no crash).

## What was added

```
src/audio/
  SoundDefs.h              SoundDef/ChannelDef/SoundGroupDef/ReverbDef +
                           SoundDefProvider (engine-independent)
  SoundDefs.cpp            process-wide provider
  GeneratedSoundDefs.h/.cpp  codegen'd tables (421 sounds, 7 channels, 45 groups,
                           2 reverbs, 617 registered channel/sound pairs)
  AudioBackend.h           backend interface (SoundHandle = int) + PlayOptions
  AudioManager.h/.cpp      port of client/src/audioManager.ts
  Ambiance.h/.cpp          port of client/src/ambiance.ts (weighted track mixer)
  AxmolAudioBackend.h      ax::AudioEngine backend (app build only)
tools/codegen_sound_defs.mjs   emits GeneratedSoundDefs.cpp from soundDefs.ts
```

`soundDefs.ts` has no imports, so the codegen loads it directly with
`node --experimental-transform-types`.

## Key behaviours ported
- **Channels**: `activePlayer`/`otherPlayers` (both the `players` list), `hits`,
  `sfx`, `ambient`, `ui`, `music`; per-channel volume + `maxRange`.
- **playSound**: distance attenuation `pow(1 - clamp(|dist|/range), 1 + fallOff*2)`,
  stereo pan, layer multiplier (0.5 across audio layers), min-volume gate
  (`0.003`), `startSilent`/`forceStart`/`loop`.
- **playGroup**: random member of a `Groups` entry.
- **Volumes**: `baseVolume(0.5) * masterVolume(0.5) * typeVolume * volumeScale`
  (matches CreateJS.Sound.volume starting at 0.5); master/sound/music setters.
- **Ambiance**: music/wind/river/waves weighted tracks (weights consumed
  high→low), immediate-mode interior tracks, intro music→wind fade-in.

## Wiring (app)
- `GameScene` installs the defs, creates `AxmolAudioBackend` →
  `AudioManager` → `Ambiance`, calls `preloadSounds()`, and hands them to
  `GameWorld` (`setAudio`). On the first playing update it calls
  `Ambiance::onGameStart()`; background pauses/`stopAll()`s audio.
- `GameWorld::loadMap` sets the map's `biome.ambience` tracks; `update` drives
  `cameraPos`, `activeLayer`, `underground` and the manager/mixer.
- SFX wired: explosions (`explosion_01`, positioned, `sfx`), destroyed building
  ceilings (`ceiling_break_01`).

## Codegen
```powershell
node --experimental-transform-types tools/codegen_sound_defs.mjs
```

## Remaining / not ported (updated 2026-10-19)
- All player action sounds are now wired:
  - ✅ **Footsteps** (grass/sand/water surface groups, step distance tracking)
  - ✅ **Weapon deploy/switch** (on activeWeapon change, melee/throwable/gun)
  - ✅ **Bleeding sound** (downed players, periodic `player_bullet_hit_02`)
  - ✅ **Haste ability sound** (`ability_stim_01` on haste seq change)
  - ✅ **Action sounds** (UseItem/Reload/ReloadAlt via `updateActions`)
  - ✅ **Pickup sound** (via GameScene `onPickupSound` callback)
  - ✅ **Bush enter/exit** (`bush_enter_01` + leaf particles on transition)
  - ✅ **Door error** (`door_error_01` on locked/one-way door contact)
  - ✅ **Dry fire** (`empty_fire_01` when firing an empty gun)
- ✅ **Explosion sounds** (per-type grass/water surface selection in `GameWorld::applyUpdate`)
- ✅ **Bullet whiz** group (close bullets from other players, `BulletBarn::update`)
- ✅ **Shot sounds** (weapon shoot with splinter/AP/high-velocity detune variants, `ShotBarn::update`)
- ✅ **Reverb/filters**: `AudioManager::update()` computes cathedral reverb volume from `activeLayer`/`underground`; `AxmolAudioBackend::update()` creates a silent reverb carrier and calls `ax::AudioEngine::setReverbProperties` with EAX parameters.
- **UI sounds** (ambient lab, pickup pings, notifications) are not wired — no UI system in the C++ port yet (the Java UI screens handle this).
- Volume/persisted settings UI (config) is M7 and lives in Java.
