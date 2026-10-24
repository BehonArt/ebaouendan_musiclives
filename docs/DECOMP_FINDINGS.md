# Findings from the EBA decompilation that the recomp can use

Source: the matching decomp at `F:\dxvk_remix\Claude\eba-decomp` (sections 1–10 at
commit `64179bc`, 188 functions byte-matched; sections 11–17 added at `04b4a78`, 276
functions; section 18 at `3f2fa9b`, 618 functions). Sections 1–10 are read from code that compiles to the exact original
bytes, unless marked *unverified*. Sections 11–17 are mostly traced from the
disassembly (and, for the scene table, by running the game's own factory under
unicorn); each says which. Addresses are EBA USA ARM9.

## 1. The game has its own autoplay (session +0x234)

`Note::Judge` (0x020681d8) checks `session->autoplay` after the touch test:

```cpp
if (gSession->autoplay != 0 && now >= time)
    result = func_02055d34(gSession);
```

`func_02055d34` turns the flag into a judgement:

| `*(session + 0x234)` | Result |
|---|---|
| 0 | off (normal play) |
| 1 / 2 / 3 | always 300 / 100 / 50 (`value - 1`) |
| 4 or more | random 300/100/50 from the game's LCG (section 7) |

The hit lands on the note's exact frame (`now >= time`), so value 1 is a perfect run
with no input emulation at all. This could replace or cross-check the `--buttons-auto`
input-driven autoplay in `eba_buttons.cpp` for oracle runs.

*Unverified:* this is the tap/slider-head path. Spinner and slider-body judging are
not decompiled yet, so check those still complete under this flag.

## 2. A built-in replay recorder (session +0xA0 / +0x138)

Every judged tap is recorded unless the game is replaying (`NoteTap::v10`, 0x02067384).
Recording also requires `session +0xC8 == 0` and `difficulty != 4`.

- **Event array:** `session +0xA0` is an engine `Array<ReplayEvent>` (`data`, `capacity`, `count` at +0xA0/+0xA4/+0xA8).
- **Cursor:** `session +0xAC` is the playback cursor.
- **Event layout (8 bytes):** `u16 frame` (when judged), `u16 noteTime`, `u8 kind`, `s8 result`, `u8 newGroup`.
- **Writing:** `func_02055f44(session, now, time, kind, result, newGroup)` appends an event.
- **Playback:** when `session +0x138` is nonzero, `Note::Judge` skips events whose note time has passed, then returns the stored result once `event.frame <= now`. Taps use the next event in `v10`. Playback also uses the second combo counter at `session +0x100` in place of +0xCC.

Likely uses: the attract-mode demo, or a debug feature. For the recomp:
1. Dump the array after a human run to get exact hit frames and results.
2. Feed it back with +0x138 set to get a deterministic re-run of a real performance. That is a stronger oracle than autoplay.

## 3. Hidden debug performance overlay (session +0x230)

In `NitroMain`'s frame loop (0x02003000), when `session +0x230 != 0`, pressing Y
toggles `gDebugPerfDisplay` (0x020cd230). The overlay prints two lines to the debug
console:

- `FRAME:%07d,TIME:%06d/%06d`: frame count, then microseconds this frame and over the last two frames.
- `HP:%06x/%06x SH:%05x/%05x`: main heap free/size, then sound heap free/size.

Writing 1 to 0x020cd230 directly skips the Y toggle. This gives free heap and timing
stats from the game's own point of view.

## 4. Session struct (`*0x020CD248`), beyond +0 / +8 / +0xF8

| Off | Meaning |
|---|---|
| +0x000 | difficulty; **4 = Training** (the main menu's TRAINING button sets it; Training skips replay recording) |
| +0x008 | stage index into `gStageTable` 0x020b3ee4 (the chart loader uses it unless +0xB4 or +0xC8 == 2; Training uses -1) |
| +0x010 | zeroed by the attract demo and Training before their songs (meaning unknown) |
| +0x090 | stylus-trail recorder: `Array` of 6-byte `{u16 frame, u16 x, u16 y}`, appended in the song scene whenever the touch position changes (not while replaying, and only if 0x020cd460 != 0) |
| +0x0A0 | replay `Array<ReplayEvent>` (section 2); +0xAC = cursor |
| +0x0B4 | **attract-demo flag**: the demo sets it (section 11); it also suppresses the hit sound/effect call in `Judge` and fades the BGM stream in |
| +0x0C8 | play mode: 1 changes which note is "current"; 2 = per-stage `btl/%s` judgement-mark graphics, per-player Combos at +0x150 |
| +0x0CC | Combo: +0x04 **KIAI (life) gauge** (fx32; the debug line prints it /4096), +0x08 x, +0x10 z, +0x14 count (s16), +0x16 max (s16), +0x30 broken flag (section 12) |
| +0x0F8 | score (= Combo +0x2C) |
| +0x100 | second Combo, used while replaying |
| +0x138 | replaying flag |
| +0x1B8 | players - 1 (u8) |
| +0x22C | heap/arg passed to animation-curve loads |
| +0x230 | debug enabled (Y toggles the perf overlay) |
| +0x234 | autoplay (section 1) |

## 5. Note objects (full layout, from matching code)

**Base Note.** The vtable has 17 slots:

| Slot(s) | Method |
|---|---|
| 0–1 | dtors |
| 2 | `GetPos` (pure virtual) |
| 3 | `GetJudgeRange` (time ±10) |
| 4 | `SetTime` |
| 5–8 | v05–v08 |
| 9 | `v09(now)` (pure virtual) |
| 10 | `v10(now)` (pure virtual): judge and react |
| 11 | `v11(frame)` |
| 12 | `v12(now)`: fade-out after judging |
| 13 | `SetPosition(yOffset)` |
| 14 | `SetColor(u16)` |
| 15 | `SetAlpha` |
| 16 | `v16` |

Base Note fields:

| Off | Field |
|---|---|
| +0x04 | 4 signed-byte bitfields: kind (0 tap / 1 slider / 2 spinner), state, group, combo number |
| +0x0A | frame of the last state change |
| +0x10 | previous note |
| +0x14 | `appearFrame` = time - approach (section 6) |
| +0x16 | `time` (a spinner's *end*) |
| +0x18 / +0x1C | x, y (lower-screen pixels) |
| +0x24 | main Sprite |
| +0x28 / +0x2C | attached Effects |
| +0x30 | `Array<Sprite*>` of extra sprites |
| +0x3E | when 0, alpha is capped at 20 instead of 31 (dimmed note) |

- **States (`SetState`, 0x02068550):** 0 not spawned, 1 fading in, 2 waiting, 3 (a tap enters it from `v09`, 30 frames after its previous state change), 4 being judged, 5 judged-hit, 6 judged-miss (3 = result miss), 7 judged, 9 gone.
- **Judge results:** 0/1/2 = 300/100/50, 3 = miss, 4 = nothing yet.
- **NoteTap:** +0x40 `judgedFrame` (0x7fff until judged), +0x42 result. The hit pop scales the sprite by 0xb33/4096 (about 0.7) for 3 frames, then hides it.
- **NoteSlider:**
  - +0x40 `Array<PathPoint>`: one point per frame of one pass, as `{s16 x*16, s16 y*64/3}`. Screen px = `x*256/4096`, `y*192/4096`.
  - +0x4C passes remaining (counts down).
  - +0x60 frames per pass.
  - +0x64 `Array<s16>`.
  - +0x70 `Array<Effect*>`: tick effects.
- **NoteSpinner:** +0x40 duration (spins over `[time - duration, time]`), +0x44 turns required (u16).
- **Engine `Array<T>`:** `{T *data; u32 capacity; u32 count}` everywhere (the NoteManager +0x70 list, the paths, the replay).

## 6. Tuning constants in .data (patchable at boot)

| Addr | Value | Meaning |
|---|---|---|
| 0x020b4704 | 100 | approach: frames a note shows before its time (`appearFrame = time - this`); `NoteTap::v09` also waits `approach*30/100` frames after a state change before moving to state 3 |
| 0x020b46fc | 16 | note radius (px): hit-test margin and the sprite's base scale |
| 0x020b4700 | 4096 | note scale (fx 1.0); hit test = `dist - margin*scale < 3*scale` |
| 0x020b22e0 | 3 | extra hit radius (px) |
| 0x020b2264 | 2, 5, 8, 65535 | judge windows (frames) for 300 / 100 / 50 / any |
| 0x020b222c+0x40 | 8 | late window: past `time + 8` = miss |

These are the knobs for an approach-rate or hit-radius option in the PC port.

## 7. Other useful addresses

| What | Where |
|---|---|
| Frame-wide judge driver | `Scheduler_Tick` 0x02059c20 |
| Per-note judge | `Note::Judge` 0x020681d8, `NoteTap::v10` 0x02067384 |
| Note parser | `Chart_CreateNote` 0x02069ffc |
| Game object | `gGame` 0x020cd22c (0x11e0 bytes, built in `NitroMain`) |
| Frame pacing | `OS_WaitVBlankIntr` 0x0200acf8, `OS_WaitIrq` 0x020078dc |
| Game RNG (LCG) | `*(0x020c28b0) + 0x59cc` state, +0x59d0 multiplier, +0x59d4 increment; seed/pin it for deterministic tests |
| Input | `gInput` 0x020be8a8 (+0 pad trigger, touch x/y, previous touch; 0xffff = no touch). A new touch is `prevTouchX == 0xffff && touchX != 0xffff` |
| Game per-frame update | `Game::func_02054318` 0x02054318 (runs the scene, decodes its result word, section 11) |
| Scene factory / switch | `func_02052ca4` (id → `new` scene) / `func_020536c8(game, id, flagA, flagB)` |
| Chart loader | in tu_020b4158 (~0x0205ca00): builds `data/seq/%s_%s.seq`, the `coop` variants and `demo%02d.seq`, then reads the header tunables (section 13) |

## 8. Sprite/Effect layout (for an HD layer)

**3D sprite object:**

| Off | Field |
|---|---|
| +0x04 / +0x05 | shown flags |
| +0x08 | position `VecFx32`: `x << 12`, `(192 - y) << 12`, z = draw layer (e.g. 0x49d000) |
| +0x18 | u16 rotation |
| +0x1C | scale `VecFx32` |
| +0x28 | color (RGB555) |
| +0x2A | alpha (0–31) |
| +0x30 | palette/colour word |
| +0x48 | sub-object with more scale at +0x10 / +0x18 |

**Effect** (vtable 0x020b393c, 0x1c bytes): +0x04 Sprite*, +0x08 alpha `{value, target}` (steps by 1 a frame), +0x18 / +0x1A width/height.

## 9. A symbol map you can import

`eba-decomp/config/usa/arm9/symbols.txt` has exact boundaries for all 4,477 ARM9
functions; 1,245 are named:

- NitroSDK 3.1, NitroSystem, MSL C, the CodeWarrior runtime and the C++ EH runtime.
- Game classes with C++-mangled names.

Library ranges are in `config/usa/arm9/LAYOUT.md`. Importing it would put real names
on generated functions (debugging, profiling) and cross-check ndskit's function
discovery.

## 10. Swapping in decompiled code

Byte-matched C from `eba-decomp/src/` is equivalent by construction. The recomp can
swap generated functions for it one at a time: Note/NoteTap/NoteSlider/NoteSpinner
methods first (the note system is now nearly complete), then the scene base class
and the first three small scenes. This is the "recomp as test oracle" plan
in reverse. It is also where a source-level fix for the 60-group chart limit would go
once that code is decompiled.

## 11. Scenes: ids, the factory, and a direct route into a song

*Traced from the disassembly; the id table comes from running the game's factory
under unicorn (`eba-decomp/tools/scene_ids.py`). The scene base class and three small
scenes are byte-matched.*

**Scene base** (vtable 0x020b3b38, 10 slots): dtors, `v02` load (TRUE when ready),
`v03` start, `v04(frames)` run a frame, `v05`, `v06` next-scene result (0 = stay),
`v07(s32)`, `v08`, `v09`. Fields: +0x08 the scene's own id, +0x10 frame counter,
+0x14 the result `v06` hands back.

**Result word** (`Game::func_02054318` 0x02054318 decodes it every frame):

| Bits | Effect |
|---|---|
| `(r & 0xff0000) == 0x10000` | switch to scene `r & 0xffff` via `func_020536c8` |
| 0x40000000 | flag passed to the switch (`func_0205376c` on the game object) |
| 0x20000000 | reset the display: `func_02063050(0x020cd37c)`, `func_020331b4/02033738(*0x020c2f1c)` |
| 0x80000000 | start the new scene (`func_02052374`) |

Every transition in the game uses all four flags, so scene ids appear as
`0xe001xxxx`. Built scenes are cached by id (`func_02052b8c`).

**Id → scene** (`func_02052ca4`):

| Id | Scene (vtable) | Notes |
|---|---|---|
| 0x20b | picture (0x020b4ae4) | **first scene at boot**: shows `misc/msg`, tap or timeout → 0x01 |
| 0x80 | (0x020b5114) | first scene instead of 0x20b when booted as a Download Play child (`*(u16*)0x027FFC40 == 2`) |
| 0x01 | (0x020b3cf0) | after the boot picture |
| 0x03 | SceneTitle (0x020b3d78) | |
| 0x04 | SceneMainMenu (0x020b7288) | |
| 0x05 | SceneDifficulty (0x020b736c) | main menu `CHRSEL` |
| 0x06 | SceneMap (0x020b7528) | |
| 0x07 | options (0x020b7c74) | main menu `OPTION` |
| 0x100 | SceneSong (0x020b65a0) | every song, including the attract demo |
| 0x101 | Training song (0x020b9b50, a SceneSong subclass) | main menu `TRAINING` (sets session +0 = 4); also the title's tap target while flag 0x020cd234 is set (first run?) |
| 0x102 | SceneComicOpening (0x020b64f0) | map → comic → song |
| 0x103 / 0x104 / 0x105 | SceneComicEnding / SceneRegroup / SceneResults | chosen by the song scene |
| 0x200 | 2D animation by difficulty (0x020b4a4c) | → 0x201 |
| 0x202 | multiplayer (0x020b4dac) | main menu `M_SELECT` |
| 0x205 | Download Play host (0x020b4bdc) | sends `data/rom/btlnchild.srl` + `data/mbicon.nbfc/nbfp`; `BTN_SEND` / `BTN_BACK`; goes to 0x88 if the wireless worker `*0x020c2e44` is NULL |
| 0x88 | wireless error/child scene | one of two scenes, picked by boot type |
| 0x0B / 0x0E / 0x0F | records / gallery / ghosts | byte-matched; see section 18 |
| 0x14 / 0x15 | wireless difficulty / host song select | byte-matched (near-misses) |
| 0x81 / 0x84 | host waiting for players / joiner waiting | byte-matched (near-misses) |
| 0x204 | picking a ghost to play against | byte-matched (one near-miss) |
| others | 0x08–0x13, 0x82–0x8a, 0x106–0x108, 0x201–0x208 | many now decompiled in eba-decomp; not summarised here |

**The attract demo** (`SceneTitle_v04` 0x02053d78; the main menu has the same code):
after 2400 idle frames (40 s) it sets session +0xB4 = 1, +0x234 = 4 (random
autoplay), +0 = 1 (Cruisin'), +0x10 = 0, its own +0x14 = 0xe0010100, and starts a
fade to black with `func_02032080(*0x020c2f1c, -16, 0)`. `SceneTitle_v06` releases
0xe0010100 only once the main screen brightness `*(s16*)(*0x020c2f1c + 0x17C)` reaches
-16. The demo plays `data/seq/demo%02d.seq`. A touch after its first 60 frames
fades the music out over 15 frames and returns to the title (0xe0010003).

**Lead for Customs (unverified):** the same jump with the demo flags left at 0
(session +0xB4 = 0, +0x234 = 0) and Customs' usual +0 / +8 pokes should start the
chosen stage straight from the title. That would skip the menu, difficulty, map and
card taps. The demo shows the song scene runs without the menus. What it doesn't
prove is that nothing the map or comic sets (+0x10, +0xC8, players) is needed for a
normal stage, so check a headless run against the current route. Recipe: while the
current scene's vtable is 0x020b3d78, write session +0 / +8, set
`*(u32*)(scene + 0x14) = 0xe0010100`, and call or force the fade.

## 12. Life (KIAI) gauge and combo rules

*From byte-matched `Note::func_02067ad8`, `func_02067c3c`, `func_02067fa4`; the gauge
flag's writers are traced.*

- **Gauge per judgement:** delta = `table[result]` from 0x020b3ec4 (defaults 300:
  +10, 100: +5, 50: 0, miss: -50; ×4096 as fx32). It is added to Combo +0x10, and
  Combo +0x08 + delta is passed to `func_020550b4` (sets the gauge target). The
  current value the debug line shows is Combo +0x04.
- **Miss:** an extra drain of `combo count × *0x020cd238` (combo-break penalty),
  then count = 0 and the broken flag is set. The top screen shakes
  (`func_0205dae4(0x020cd2fc, clock, 0x5000, 3, 1, 20, 0)`), the agents' break
  reaction plays, and sound 0x15 is used (0x16 on Hard Rock).
- **Combo** caps at 1000; the max is kept at Combo +0x16.
- **Group end:** the worst result in the finished group goes to `func_02055260`
  (the group bonus). It plays sound 11 if every note was a 300, else 10, then a
  pop-up (`func_02069d38`).
- **Gauge on/off:** every gain and drain above is skipped while
  `*(s32*)0x020b40c8 == 0`. The song setup (0x0205cdd8) sets it to 1; the Training
  scene turns it off and on. **A No-Fail assist is writing 0 there after the song
  starts.** Unverified: whether the gauge's passive drain over time (not found
  yet) also checks it.
- Hit sounds: per-note sound index (note +0x08) into the s16 table 0x020cd2b4,
  played on channel `*0x020cd2ac`.

## 13. Each chart's header carries gameplay tuning (Customs Studio)

*Traced from the disassembly.* At load, the chart loader reads a version from the
`.seq` stream, then `func_02054688(stream, version)` copies a run of header values
into globals. Each chart can carry its own difficulty tuning; the .data values are
only defaults.

Write order: 0x020b3ec4 (16 bytes: the four gauge deltas, section 12), 0x020cd238
(combo-break drain), then 0x020b3e6c, 3e60, 3e70, 3e74, 3e64, 3e98, 3e94, 3e90
(version ≥ 8), 0x020cd23c, 3eac, 3eb8, 3e84, 3e80, 3e7c, 3e78, 3e8c, 3e88, 3ea4,
3e68 (version ≥ 9), 3e9c (version ≥ 11, two of its values doubled). Only the first
two are identified.

For the Studio this explains why a custom's drain follows the chart it was built
from. A per-custom gauge setting would be an edit to these header fields.

## 14. Debug features inside songs

*Traced.*

- With session +0x230 (debug) set, **X cycles the game's autoplay 0→4** during a
  song (`func_02082e0c`: `autoplay = (autoplay + 1) % 5`).
- With the perf overlay on (`gDebugPerfDisplay` 0x020cd230, section 3), the song
  scene prints `DBG:%d KIAI:%4d BGM:%07d` on debug-console row 21: the autoplay
  value, the KIAI gauge (Combo +0x04 / 4096) and the BGM stream position.

## 15. Fades and visible planes (resources `*0x020c2f1c`)

*Byte-matched in the three small scenes.*

- +0x17C / +0x180: main / sub screen brightness (s16; -16 = black, 0 = normal).
  `func_02032080(res, target, screen)` starts a fade. Scenes wait on these values
  before switching, so the HD layer can read them to fade its own overlays in step
  with the game.
- +0x587C: the planes shown (DISPCNT bits 8–12, BG0–3 + OBJ), mirrored at +0x174;
  `ShowPlanes(mask)` ORs bits in and writes DISPCNT.

## 16. Wireless paths the recomp will hit

`M_SELECT` → 0x202 (multiplayer) and the Download Play host 0x205 need the
wireless worker at `*0x020c2e44`. With no worker, 0x205's frame function returns
0xe0010088 (scene 0x88) at once. If wireless stays stubbed, those menu entries
lead to that scene; hiding or redirecting them is a recomp-side choice.

## 17. The approach circle (for an HD redraw)

*Byte-matched / near-matched (class at vtable 0x020b49a4, methods 0x0206f3b0–0x0206f5b8).*

- Between `appear` and `time` the ring scale is
  `size * (time - now) / (time - appear) + 1.0`, times the note's `scale` field.
- It hides itself once the scale reaches 0 or below.
- It swaps between three circle textures: the smallest whose limit (`circle +0x08`)
  covers the current scale (`func_0206f5b8`).
- An HD layer can draw one high-res ring at that scale and ignore the texture swaps.

## 18. The song scene (0x100) and the menus around it

*Byte-matched unless marked: SceneSong (`eba-decomp/src/game/tu_020b6530.cpp`, 14 of 19
functions exact, the rest register/scheduling near-misses), gallery 0x0E, ghost
picker 0x204, the multiplayer lobby scenes 0x14/0x15/0x81.*

**SceneSong layout** (0x144 bytes, vtable 0x020b65a0, an 11th virtual `v10(reset, a)` =
(re)start the song, also called by the Training subclass at 0x020b9b50):

| Off | Field |
|---|---|
| +0x38 | overlays: the 3 pause-menu buttons, then [3] the skip button |
| +0x44 | the song player (0x84 bytes): +0 current frame, +0x0C timeline (`order`/`items` arrays, cursor +0x18), +0x28 the movie (`UnkClass_0205a198`, its picture at +0x20) |
| +0xF0 | score number |
| +0x128 | pause backdrop sprite; its alpha eases toward +0x13C (31 while `*0x020cd250` is 1, 2 or 10) |
| +0x12C / +0x134 | replay/demo/review banners and the demo's "touch" hint (blink 50 on / 30 off) |
| +0x138 | each screen's brightness saved while paused |
| +0x140 | skip limit = movie length - 120 frames |

**Per frame (`v04`):** the scene's frame counter (+0x10) is set from the player's
frame each tick, and the delta goes to `*0x020b3b24` (frames advanced this tick).
Start (`trigger & 8`) pauses unless `*0x020cd250 == 10`; the skip button stops
working after +0x140. While replaying, the recorded stylus position
(`func_02055f00`) is written straight into `gInput` +0x0C/+0x0E, so the touch
test sees the replay's stylus. The song player's per-frame call
(`func_0205d0e4(player, 1, comboCount)`) returns an event: 2 = failed, 3 = over.

**Where a song goes next** (`func_02082e70`):

| Event | Condition | Next scene |
|---|---|---|
| 3 (over) | attract demo (+0xB4) | 0x01 (music fades over 30 frames) |
| 3 | Training (+0 == 4) | 0x04 |
| 3 | stage == `func_02054680(0)` (the stage just unlocked?) | 0x0F if session +0x13C else 0x105 (results) |
| 3 | otherwise | 0x103 (ending comic) |
| 2 (failed) | `func_02055c94(session, frame)` true | 0x104 (regroup), or 0x01 in the demo; plays sound `*0x020b4330` id 0x28 |

`v06` then fades the screen to white over 120 frames (to black over 15 in the
demo) and releases the result once brightness reaches ±16.

**Replay review:** with session +0x138 (replaying) and **+0xB0 = a frame**, the song
restarts 600 frames (10 s) before that frame, stepping back past slider/spinner
bodies, and restores the gauge from the replay. The pause menu's middle button
then goes to 0x105 instead of 0x06 (0x0F when session +0x13C is set).

**Resources:** +0x1BC = paused flag, +0x2A0 = !paused (both written by the song's
`v09`); +0x234 / +0x2A8 take the two screens' full-screen rects on exit.

**Gallery (0x0E):** buttons by difficulty (`data/menu/gallery_lvbtn_n/h/x.gui`),
movies per difficulty (`gallery_movbtn_<e,n,h,x>.gui`; button i shown only if
`i <= func_02054ab0(save, difficulty)`, button 8 needs a stage-clear check). Special
buttons: "4" = ending (`misc/endroll1` + `data/bgm/ending`, removed unless
`func_0209eae4()`), "5" = rankings (`rnk/LV_10`, needs `func_02054d3c(save) >= 11`).
Other movies use `func_0209de04(movie, difficulty, name)`.

**Multiplayer lobby (for whoever wires wireless):** the host broadcasts a
0x1C-byte message from scene +0x70 every frame: every 10th frame a player slot
(type 2: slot, joined, name from the peer's beacon), otherwise the setup (type 4:
roles by AID, ranks, AIDs, stage, difficulty, AID count - 1). Joining players get
the first free AID in their team's range (`data_020b258c` / `data_020b2594`). The
host's beacon game info (`*(connection +0x14)`) carries mode at +0x53,
difficulty +0x54, stage +0x55, status +0x56. In co-op the host gives up after
1800 frames if everyone left.

