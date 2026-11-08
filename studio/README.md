# Customs Studio

The EBA / Ouendan chart editor (the toolkit's Note Editor and Section Editor,
`rom_hacking/tools/seq_editor`) as a companion app of the recomp, with a bar
in both editors for the recomp's side of a custom:

- **Test in recomp** (and **Build + test**): the custom just built, played at
  once in the recomp's Customs mode, straight into its stage, on any
  difficulty, with autoplay if wanted.
- **✔ Validate**: the recomp plays the custom through headless with its
  button-mode test player, every note on time (under a minute, no window;
  `validate.py`), and reports: loaded, notes played, score and grades,
  cleared or failed, and how many seconds of the song are left after the last
  note (more than 25 s is flagged: stock stages leave 10 to 22). From a
  shell: `py -3 tools\validate_custom.py customs\<tag>`. Checked on All Star
  (120/120 at 300, cleared, 23 s after the last note) and Girls Just Want to
  Have Fun (182/182, cleared, flagged: 40 s).
- **Export custom folder** (or **...as .zip**): `customs\<stage>_<title>` in
  the recomp's play folder, where Customs lists it; the .zip is for sharing
  (Import custom folder on the other side).
- **Export patch (.bps)**: a BPS patch of the stock ROM (3-4 MB for a song:
  moved files and shifted audio are copied from the stock ROM), to share a
  custom without the game. Any BPS patcher (Flips, Rom Patcher JS) applies it.

The toolkit's editor runs as it is, from the toolkit's folder (its charts,
projects and builds stay where they are); the Studio adds `recomp_bridge.py`.

    py -3 studio.py              # from source, in its own window
    py -3 studio.py --browser    # the default browser instead
    studio\build_studio.ps1      # the .exe: studio\dist\Customs Studio\

`studio.ini` beside it says where the toolkit's `tools` folder and the recomp are. A
development tree uses the recomp's `play` folders. The release (`tools/make_release.ps1`)
has `deploy = 1`: the shipped toolkit is copied to the player's data folder on start and
runs from there, the dumps are the launcher's, and customs go to the games' data folders.
The player's guide is `docs/CUSTOMS_STUDIO_GUIDE.md`.

## The .exe

PyInstaller, one folder, a console program that restarts itself with no console
(`CREATE_NO_WINDOW`, which its builders inherit) for its own window (pywebview
on WebView2; closing the window stops it). Without WebView2, or with `--browser`, it opens in the default browser and
the console stays, closing it stops the Studio. The page is at
http://127.0.0.1:8574/ (a free port if something else has that), so a tab keeps
working. Started again while it runs, it opens another window on it. The
toolkit stays `.py` on disk; what it imports is bundled (`make_deps.py` lists
it, unicorn whole for Ouendan 2's decompressor). The toolkit runs its
builders as `[sys.executable, builder.py, ...]`: the .exe is Python for them
(`python_mode`). `patch_exe.py` turns off Control Flow Guard in its header, as
python.exe has it: unicorn's JIT-made code fails CFG (0xC0000409).

Checked: the packaged Studio serves both editors with the bar, loads charts
of all three games (Ouendan 2's through unicorn), builds a custom ROM through
the toolkit's builder (4 s), and that ROM's custom folder plays in the
recomp's Customs mode into its song.
