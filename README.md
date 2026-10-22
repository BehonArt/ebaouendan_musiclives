# eba-recomp

Static recompilation of Elite Beat Agents (USA, AOSE) and Osu! Tatakae!
Ouendan (Japan, AOSJ) to native PC code with
[ndskit](https://github.com/vs-sr-dev/ndskit) (submodule at `ndskit/`).
OE2 (AO2J) is next.

## Status (2026-10-22)

Both games boot headless with no interpreted functions, through the title,
menus, stage map, a stage's intro comic, gameplay and the fail screen, with
the real saves loaded and the music in the WAV.

| | EBA | OE1 |
|---|---|---|
| ARM9 functions | 4,517 (6 refuse passes) | 3,226 (no refuse passes) |
| ARM7 functions | 1,026 | 1,004 |
| unresolved calls / jumps (ARM9) | 841 / 23 | 437 / 3 |
| self-test vectors, game functions | 24,346 (1,614 functions) | 18,280 (1,183 functions) |
| failures | 0 | 0 |
| save | 256 KiB flash | 8 KiB EEPROM |
| stage played | "The Anthem" | "ループ&ループ" |
| windowed play | timing and audio right (user) | not yet |

Plus ndskit's instruction test: 57,781 vectors, 0 failures. Known runtime gap
seen: 3D edge marking is not drawn (EBA's stage map).

## Local ndskit patch

`ndskit-discover-refuse-passes.patch`: `Program.run()` in
`recomp/discover.py` gave up after 4 refuse passes and wiped everything, so
EBA's ARM9 discovered 0 functions. The patch allows 40 passes and never wipes
after the last one. Re-apply it after updating the submodule:

    git -C ndskit apply ../ndskit-discover-refuse-passes.patch

## Toolchain

MSYS2 UCRT64 (`C:\msys64`): `mingw-w64-ucrt-x86_64-{clang,sdl3,cmake,ninja}`.
Python 3 for the ndskit layers. ROMs go in `roms/` (EBA.nds, OE1.nds,
OE2.nds; not committed). Each game gets its own tree: EBA in
`build/recomp`, OE1 in `build/oe1/recomp`.

## Commands

    python tools/phase0_check.py roms/EBA.nds --game eba        # discovery + hand-found addresses
    python -m ndskit.recomp roms/EBA.nds --out build/recomp --optest
    python -m ndskit.recomp.selftest roms/EBA.nds --cpu 9 --auto --out build/recomp/selftest_eba9.txt
    C:\msys64\usr\bin\bash.exe -lc "/f/dxvk_remix/Claude/eba-recomp/tools/build.sh"
    build/recomp/b/selftest build/recomp/selftest/*.txt build/recomp/selftest_eba9.txt

OE1 is the same with `roms/OE1.nds`, `--out build/oe1/recomp` (no
`--optest` needed) and `RECOMP_DIR=build/oe1/recomp` before `build.sh`.

Run (put `C:\msys64\ucrt64\bin` on PATH for the DLLs). The input scripts go
from boot into a stage on the middle difficulty with the saves we have (taps
are lower-screen coordinates):

    build/recomp/b/ndsrun roms/EBA.nds --out build/run --save build/run/EBA.sav --vblanks 9000 --wav build/run/a.wav ^
      --input 950:tap:128:150,1300:tap:128:66,2450:tap:236:178,3650:tap:126:103,4300:tap:236:178,5000:tap:238:183 ^
      --shots 5600,7500
    build/oe1/recomp/b/ndsrun roms/OE1.nds --out build/oe1/run --save build/oe1/run/OE1.sav --vblanks 7500 ^
      --input 700:tap:128:150,1350:tap:150:48,2050:tap:236:183,3050:tap:160:76,3450:tap:236:183 --shots 7000
    build/recomp/b/ndsrun roms/EBA.nds --save build/play/EBA.sav --window --scale 2

## Next

1. Play OE1 in the window.
2. OE2: 7 overlays (standard BLZ), run through the overlay dispatch.
3. melonDS oracle: savestate at the same VBlank, `python -m ndskit.melonds STATE.ml1 --compare build/run`.
4. File the discover.py bug upstream.
