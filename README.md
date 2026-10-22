# eba-recomp

Static recompilation of Elite Beat Agents (USA, AOSE), Osu! Tatakae!
Ouendan (Japan, AOSJ) and Moero! Nekketsu Rhythm Damashii (Japan, AO2J) to
native PC code with [ndskit](https://github.com/vs-sr-dev/ndskit)
(submodule at `ndskit/`).

## Status (2026-10-22)

All three games boot headless with no interpreted functions, through the
title, menus, stage map, a stage's intro comic, gameplay and the fail screen,
with the real saves loaded and the music in the WAV.

| | EBA | OE1 | OE2 |
|---|---|---|---|
| ARM9 functions | 4,517 (6 refuse passes) | 3,226 | 4,086 program + 1,191 in 7 overlays |
| ARM7 functions | 1,026 | 1,004 | 1,026 (EBA's ARM7) |
| unresolved calls / jumps (ARM9 program) | 841 / 23 | 437 / 3 | 691 / 15 |
| self-test vectors, game functions | 24,346 (1,614 functions) | 18,280 (1,183) | 28,690 (1,866; 4,690 through the overlay dispatch) |
| failures | 0 | 0 | 0 |
| save | 256 KiB flash | 8 KiB EEPROM | 256 KiB flash |
| stage played | "The Anthem" | "ループ&ループ" | "サムライブルー" |
| windowed play | timing and audio right (user) | timing and audio right (user) | works great (user), through the overlay swaps into a song |

Plus ndskit's instruction test: 57,781 vectors, 0 failures. OE2's overlays
change hands as the game goes (6 then 1 at boot, 0 for the stage, 5 for
gameplay, 6 again at the fail screen), each found by the runtime. Known
runtime gap seen: 3D edge marking is not drawn (the EBA and OE2 stage maps).

## Display options (EBA's window; any ndskit game)

    build/recomp/b/ndsrun roms/EBA.nds --save build/play/EBA.sav --window --scale 3 --fps 60

| option | what it does |
|---|---|
| `--scale N` | composes both screens at N times the DS's resolution (1-6): 1x 256x384, 2x 512x768, 3x 768x1152, 4x 1024x1536, 5x 1280x1920, 6x 1536x2304 (the first to cover a 4K display's height). The 3D draws at that resolution; the 2D layers stay the DS's pixels, sharp. The window opens at that size, fitted to the display; a larger window gets a sharp-bilinear upscale, a smaller one the mean of the pixels it covers (supersampling). Without `--scale` the picture follows the window's size, as before. |
| `--fullscreen` | the whole display, letterboxed at the DS's shape. F11 or Alt+Enter toggles it. |
| `--fps ds` | the default: the DS's 59.83 frames a second, presented with the vertical sync (on a 60 Hz display one frame shows twice every 5.8 s). |
| `--fps 60` / `120` / `240` | the game at exactly 60 frames a second (0.29% faster than the DS, the sound resampled with it: about 5 cents, inaudible), each frame shown 1, 2 or 4 times. On a display at that refresh rate the vertical sync is the clock, so every frame lasts the same: no judder. On another display, paced by the clock. |
| `--fps uncapped` | the DS's pace, pictures as fast as the GPU draws them, without the vertical sync. |

The title bar shows the last second's frames and pictures. The log ends with
the run's averages (after its first 60 frames), how many frames came late,
the longest, and the longest the game itself took to run one; any stall
over 50 ms is noted as it happens. F12 also saves the composed picture at
full scale (`shotN-3x.png`).

Measured on this PC (RTX 4060, display 2715x1527 at 60.000 Hz, NVIDIA's
"Vertical sync" left to the application), 20 s from the boot each:

| `--fps` | game frames a second | pictures a second |
|---|---|---|
| `ds` | 59.80 to 59.83 | 60, with the sync |
| `60` | 60.00, no frame late | 60.0, the sync the clock |
| `120` | 59.95 | 120.0 |
| `240` | 59.95 | 235.8 |
| `uncapped` (fullscreen, 4x) | 59.73 | 2,166 |

The game itself never took more than 6 ms for a frame. On this 60 Hz display,
120 and 240 run without the sync, so they can tear; on a display at 120 or
240 Hz they lock to it the way 60 does here. Above 60, each extra picture
repeats the game's last frame: the DS draws nothing between frames. The
planned HD note layer will draw the hit circles, approach circles, slider
balls and spinners between frames, which is what makes 120/240/uncapped
smoother. If a driver keeps the vertical sync on against the runtime's
request (NVIDIA Control Panel's "Vertical sync: On" did), the log says so.

## Local ndskit patches

In `patches/`, each applying cleanly to upstream ndskit (5ee24af):

- `ndskit-display-options.patch`: the display options above (runtime
  `display.cpp`, `audio.cpp`, `main.cpp`, `nds.h`, `display.h`). Headless
  runs are byte-identical with and without it.
- `ndskit-discover.patch`: two fixes to `recomp/discover.py`, both to report
  upstream (drafts in `upstream/`):

1. `Program.run()` gave up after 4 refuse passes and wiped everything, so
   EBA's ARM9 discovered 0 functions. It now allows 40 passes and never wipes
   after the last one.
2. The rule of opposite conditions ("`bxpl` then `popmi`: nothing runs
   after") was applied across calls. CodeWarrior's soft-float compares
   return their result in the flags, so `bne ...; bl _dcmpeq; beq ...` lost
   the fall-through after the `beq`, emitted as an `arm_jump` to a
   non-entry: a fatal "returned to X" if it ran. A call now ends the stretch
   of same flags. Before and after on all 14 programs of the three games
   (`tools/compare_funcs.py`): only OE2's two affected functions grew
   (0x020234B8, overlay 6's 0x020B50CC); none lost, split or shrunk; 0
   conflicts.

Re-apply after updating the submodule:

    git -C ndskit apply ../patches/ndskit-discover.patch ../patches/ndskit-display-options.patch

## Toolchain

MSYS2 UCRT64 (`C:\msys64`): `mingw-w64-ucrt-x86_64-{clang,sdl3,cmake,ninja}`.
Python 3 for the ndskit layers. ROMs go in `roms/` (EBA.nds, OE1.nds,
OE2.nds; not committed). Each game gets its own tree: EBA in
`build/recomp`, OE1 in `build/oe1/recomp`, OE2 in `build/oe2/recomp`.

## Commands

    python tools/phase0_check.py roms/EBA.nds --game eba        # discovery + hand-found addresses
    python -m ndskit.recomp roms/EBA.nds --out build/recomp --optest
    python -m ndskit.recomp.selftest roms/EBA.nds --cpu 9 --auto --out build/recomp/selftest_eba9.txt
    C:\msys64\usr\bin\bash.exe -lc "/f/dxvk_remix/Claude/eba-recomp/tools/build.sh"
    build/recomp/b/selftest build/recomp/selftest/*.txt build/recomp/selftest_eba9.txt

OE1 and OE2 are the same with their ROM, `--out build/oe1/recomp` or
`build/oe2/recomp` (no `--optest` needed) and `RECOMP_DIR=build/oe1/recomp`
(or oe2) before `build.sh`. OE2's overlays get their own vectors:
`--cpu 9 --overlay N --auto` for N in 0-6.

`tools/run_headless.ps1` runs a build headless from a fresh copy of the real
save. These scripts go from boot into a stage on the middle difficulty (taps
are lower-screen coordinates):

    tools\run_headless.ps1 -Game EBA -VBlanks 9000 -Wav -Shots 5600,7500 `
      -Script "950:tap:128:150,1300:tap:128:66,2450:tap:236:178,3650:tap:126:103,4300:tap:236:178,5000:tap:238:183"
    tools\run_headless.ps1 -Game OE1 -VBlanks 7500 -Wav -Shots 7000 `
      -Script "700:tap:128:150,1350:tap:150:48,2050:tap:236:183,3050:tap:160:76,3450:tap:236:183"
    tools\run_headless.ps1 -Game OE2 -VBlanks 12600 -Wav -Shots 10200,12600 `
      -Script "1850:tap:128:150,2650:tap:65:55,3350:tap:238:183,4350:tap:238:178,4750:tap:238:183"

Map notes: on OE1's town map the face by the red arrow goes to the globe
view; the stage is the man in green. On OE2's 3D map, taps on the people did
nothing headless; the book icon at the bottom right moves to a stage and
opens its card.

In a window (put `C:\msys64\ucrt64\bin` on PATH; mouse = stylus, Esc quits):

    build/recomp/b/ndsrun roms/EBA.nds --save build/play/EBA.sav --window --scale 2

## Next

EBA only for now:

1. Button mode: a controller's face buttons or a key in place of the
   stylus, reading the live notes from RAM (the judge at 0x020681D8 runs
   every frame; note objects, vtable 0x020B4648, hold x and y): tap on time
   for a hit circle, hold for a slider, mash for a spinner.
2. HD note layer: the notes drawn natively between frames, for 120/240.
3. melonDS oracle: savestate at the same VBlank, `python -m ndskit.melonds STATE.ml1 --compare build/run`.
4. Post the discover.py bug reports (drafts in `upstream/`).
5. 3D edge marking (ndskit runtime), seen on the EBA and OE2 stage maps.
