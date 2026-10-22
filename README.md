# eba-recomp

Static recompilation of Elite Beat Agents (USA, AOSE) to native PC code with
[ndskit](https://github.com/vs-sr-dev/ndskit) (submodule at `ndskit/`).
OE1 (AOSJ) and OE2 (AO2J) are next.

## Status (2026-10-22)

EBA boots headless with no interpreted functions: title, menus, the 3D stage
map, a stage's intro comic, gameplay ("The Anthem") and the fail screen, with
the real 256 KiB flash save loaded and the music in the WAV.

- Discovery: 4,517 ARM9 functions (6 refuse passes, 0 conflicts), 1,026 ARM7.
  841 unresolved calls through registers (virtual calls), 23 jumps.
- Self-test: 57,781 instruction vectors + 24,346 vectors of EBA's own
  functions (1,614 functions), 0 failures.
- Known runtime gap seen: 3D edge marking is not drawn (stage map).

## Local ndskit patch

`ndskit-discover-refuse-passes.patch`: `Program.run()` in
`recomp/discover.py` gave up after 4 refuse passes and wiped everything, so
EBA's ARM9 discovered 0 functions. The patch allows 40 passes and never wipes
after the last one. Re-apply it after updating the submodule:

    git -C ndskit apply ../ndskit-discover-refuse-passes.patch

## Toolchain

MSYS2 UCRT64 (`C:\msys64`): `mingw-w64-ucrt-x86_64-{clang,sdl3,cmake,ninja}`.
Python 3 for the ndskit layers. ROMs go in `roms/` (EBA.nds, OE1.nds,
OE2.nds; not committed).

## Commands

    python tools/phase0_check.py roms/EBA.nds --game eba        # discovery + hand-found addresses
    python -m ndskit.recomp roms/EBA.nds --out build/recomp --optest
    python -m ndskit.recomp.selftest roms/EBA.nds --cpu 9 --auto --out build/recomp/selftest_eba9.txt
    C:\msys64\usr\bin\bash.exe -lc "/f/dxvk_remix/Claude/eba-recomp/tools/build.sh"
    build/recomp/b/selftest build/recomp/selftest/*.txt build/recomp/selftest_eba9.txt

Run (put `C:\msys64\ucrt64\bin` on PATH for the DLLs). The input script
goes from boot to "The Anthem" on Sweatin' with a cleared save (taps are
lower-screen coordinates):

    build/recomp/b/ndsrun roms/EBA.nds --out build/run --save build/run/EBA.sav --vblanks 9000 --wav build/run/a.wav ^
      --input 950:tap:128:150,1300:tap:128:66,2450:tap:236:178,3650:tap:126:103,4300:tap:236:178,5000:tap:238:183 ^
      --shots 5600,7500
    build/recomp/b/ndsrun roms/EBA.nds --save build/run/EBA.sav --window --scale 3

## Next

1. Play in the window (`--window`): timing, audio sync, input feel.
2. melonDS oracle: savestate at the same VBlank, `python -m ndskit.melonds STATE.ml1 --compare build/run`.
3. Decision gate: stay on ndskit, or a hybrid (recompiled ARM9 + melonDS core) if audio/timing fidelity is poor.
4. OE1, then OE2 (7 overlays, standard BLZ).
