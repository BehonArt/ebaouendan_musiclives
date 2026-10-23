# Findings from the EBA decompilation that the recomp can use

Source: the matching decomp at `F:\dxvk_remix\Claude\eba-decomp` (commit `64179bc`,
188 functions byte-matched). Everything below is read from code that compiles to
the exact original bytes, unless marked *unverified*. Addresses are EBA USA ARM9.

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
| +0x000 | difficulty (4 = a mode that skips replay recording) |
| +0x008 | stage index into `gStageTable` 0x020b3ee4 |
| +0x0A0 | replay `Array<ReplayEvent>` (section 2); +0xAC = cursor |
| +0x0B4 | when nonzero, suppresses the hit sound/effect call in `Judge` |
| +0x0C8 | play mode: 1 changes which note is "current"; 2 = per-stage `btl/%s` judgement-mark graphics |
| +0x0CC | Combo: +0x08 x, +0x10 z, +0x14 count (s16), +0x16 max (s16), +0x30 broken flag |
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
| Input | `gInput` 0x020be8a8 (+0 pad trigger, touch x/y, previous touch; 0xffff = no touch) |

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
methods first, about 120 functions so far. This is the "recomp as test oracle" plan
in reverse. It is also where a source-level fix for the 60-group chart limit would go
once that code is decompiled.
