# Customs Studio

The EBA / Ouendan chart editor (the toolkit's Note Editor and Section Editor,
`rom_hacking/tools/seq_editor`) as a companion app of the recomp, with a bar
in both editors for the recomp's side of a custom:

- **Test in recomp** (and **Build + test**): the custom just built, played at
  once in the recomp's Customs mode, straight into its stage, on any
  difficulty, with autoplay if wanted.
- **Export custom folder** (or **...as .zip**): `customs\<stage>_<title>` in
  the recomp's play folder, where Customs lists it; the .zip is for sharing
  (Import custom folder on the other side).
- **Export patch (.bps)**: a BPS patch of the stock ROM (3-4 MB for a song:
  moved files and shifted audio are copied from the stock ROM), to share a
  custom without the game. Any BPS patcher (Flips, Rom Patcher JS) applies it.

The toolkit's editor runs as it is, from the toolkit's folder (its charts,
projects and builds stay where they are); the Studio adds `recomp_bridge.py`.

    py -3 studio.py              # from source (an Edge app window)
    py -3 studio.py --browser    # the default browser instead
    studio\build_studio.ps1      # the .exe: studio\dist\Customs Studio\

`studio.ini` beside it says where the toolkit's `tools` folder and the recomp
(its `play`, `play_oe2`, `play_oe1` folders) are.

## The .exe

PyInstaller, one folder, a console program whose console is hidden. The
toolkit stays `.py` on disk; what it imports is bundled (`make_deps.py` lists
it, unicorn whole for Ouendan 2's decompressor). The toolkit runs its
builders as `[sys.executable, builder.py, ...]`: the .exe is Python for them
(`python_mode`). `patch_exe.py` turns off Control Flow Guard in its header, as
python.exe has it: unicorn's JIT-made code fails CFG (0xC0000409).

Checked: the packaged Studio serves both editors with the bar, loads charts
of all three games (Ouendan 2's through unicorn), builds a custom ROM through
the toolkit's builder (4 s), and that ROM's custom folder plays in the
recomp's Customs mode into its song.
