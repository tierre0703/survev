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
- ✅ **Shot sounds** (def-driven `sound.shoot` with the faction/last/alt
  precedence, `fallOff`/`ignoreDetune`, splinter/AP/high-velocity detune layers
  and the local single-fire cycle/pull rechamber cue, `ShotBarn::update`)
- ✅ **Bullet whiz** group (close bullets from other players, `BulletBarn::update`)
- ✅ **Thrown-projectile impacts** (wall `sound.bullet` hit group + particles once
  per obstacle, and the `frag_grass`/`frag_sand`/`frag_water` or
  `footstep_*` landing group on first ground contact, `ProjectileBarn::update`)
- ✅ **Reverb/filters**: `AudioManager::update()` computes cathedral reverb volume from `activeLayer`/`underground`; `AxmolAudioBackend::update()` creates a silent reverb carrier and calls `ax::AudioEngine::setReverbProperties` with EAX parameters.
- **UI sounds** (ambient lab, pickup pings, notifications) are not wired — no UI
  system in the C++ port yet. Java can now play one-shot cues through
  `SurvBridge.playSound` (see the 2026-10-27 update below); the classic UI
  buttons are still silent.
- Volume/persisted settings UI (config) is M7 and lives in Java.

## Structure interior ambience (2026-10-23)

`structure.ts updateInteriorSounds` is now ported (M4_REMAINING §30.6):
`MapObjectDef::interiorSound` is codegen'd from `structureDefs.ts` and
`Structure::update` drives the immediate-mode `interior_0`/`interior_1` Ambiance
tracks (distance + ceiling fade + underground/stair weights, `soundAlt`
transition, enable fade, `filter`). `Ambiance` exposes `setTrackFilter` and
`trackSound/trackWeight/trackFilter` accessors.

Building `soundEmitters` (bathhouse steam, oasis waves) are also ported: the
defs codegen into `MapObjectDef::soundEmitters` and `Building::update` runs the
web's 10 Hz distance/fade/layer volume model, playing looped start-silent and
stopping on `m_free()`.

## Per-source EQ + audio trace (2026-10-24)

The web's `muffled`/`club` biquad chains are approximated by an OpenAL EFX
low-pass, added to the vendored engine by
`tools/patch-axmol-audio-filter.ps1` (run automatically by
`tools/vendor-axmol.ps1`):

- `ax::AudioEngine::setFilterProperties(audioId, enabled, gain, gainHF)` ->
  `AudioPlayer::setFilterProperties` (EFX `AL_FILTER_LOWPASS`, released in
  `clearEffects`).
- `audio/AudioFilter.h` maps muffled -> gain 0.90/gainHF 0.05 and club ->
  0.85/0.02; never/reverb leave the filter off (reverb is EAX).
- `AudioManager` resolves the filter like `audioManager.ts` (different layer or
  `forceFilter` -> EQ, otherwise `reverb`) and passes it via
  `PlayOptions.filter`; `AxmolAudioBackend` applies it at play.
- With `adb shell setprop debug.surv_debug 1`, every play is logged as
  `[audio] play <sound><channel> filter=<name> [eq=lowpass gain=.. gainHF=..]
  vol=.. loop=..`, so footsteps/interior tracks/EQ can be checked from
  `logcat` without listening. A debug-only match-start self-test plays
  `emote_01` muffled and club to prove the EFX path.

Still not ported: human timbre verification (the call trace now proves the
parameters) and UI sounds (the Java UI owns those screens).

## Update 2026-10-25 (P0.7/P1.5)

- Startup volumes/mute are applied from config by `UiBridge::applyGameSettings`.
- `AudioManager::setVolume` now folds in the global master volume (and the
  per-instance pre-master base), so ambience/music/interior/emitter updates no
  longer drop it; changing master rescales live voices (including 0 -> 1).
- `stopAll(retainAmbient = true)` matches createJS and keeps ambient/music loops;
  pausing/backgrounding no longer stops audio. `GameScene` ticks
  `AudioManager`/`Ambiance` every frame (menu music included), and
  `Ambiance::onGameComplete` fades the menu music back in.
- Channel semantics live in `AudioManager`: maxInstances (oldest killed),
  canCoalesce (30 ms equal-power merge), delay (seconds; shot.ts pre-converts),
  detune (cents -> axmol pitch 2^(c/1200)) and ambient/loop retention.
- `AxmolAudioBackend::isPlaying` keeps PAUSED voices (mute = pauseAll) and pauses
  loops started while muted.
- Sound keys now come from generated defs: gun reload/reloadAlt/deploy,
  heal/boost use, loot pickup (new `GameObjRenderDef` fields), door
  change/open/close/unlock, button on/off, obstacle explode, building puzzle
  fail/complete + ceiling destroy, plane engine loop, and victory music
  (`GameOverMsg` + 1.3 s delay).
- Not applicable on Android: `notification_start_01` (tab focus) and
  `notification_join_01` (web team screen only when unfocused). Exact EQ timbre
  remains P2.
- Host tests: 181 tests / 2,794 assertions, 0 failures.

## Update 2026-10-27 (Java UI cues)

The Java screens had no way to reach the mixer, so the menu avatar played
silently. `SurvBridge.playSound(name, channel)` (JNI) ->
`UiBridge::playSound` -> `AndroidUiBridge` (queued onto the GL thread) ->
`AudioManager::playSound(name, {channel})` now plays one-shot cues from Java;
unknown sound keys/channels, mute and the volume tiers are all handled in the
native mixer. `MemoryUiBridge` records calls for the host tests.

Ported cues (`ui/opponentDisplay.ts`): the outfit change plays the new outfit's
`sound.pickup` on `ui`, and the selected melee's animData.ts `animPlaySound`
effects (`swing`/`deploy`/`idle`, resolved through the item's sound table) play
on `sfx` inside the player.ts effect window. `GeneratedPreviewAnims` carries the
per-item `SOUNDS` table and the per-anim effects.

`audio_ui_cues_follow_the_channel_type_volume` locks the channel-type volumes:
only the `music` channel follows `musicVolume`; `ui`/`sfx` and the web's
`ambient` (map music) quirk follow `soundVolume`. Device check with
`debug.surv_debug=1`: `[audio] play punch_swing_01sfx vol=0.500` and
`[audio] play clothes_pickup_01ui vol=0.375` with master 1 / sound 1.
