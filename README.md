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

## Local ndskit patch

`ndskit-discover-refuse-passes.patch` holds two fixes to `recomp/discover.py`,
both to report upstream:

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

    git -C ndskit apply ../ndskit-discover-refuse-passes.patch

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

1. melonDS oracle: savestate at the same VBlank, `python -m ndskit.melonds STATE.ml1 --compare build/run`.
2. File both discover.py bugs upstream.
3. 3D edge marking (ndskit runtime), seen on the EBA and OE2 stage maps.
