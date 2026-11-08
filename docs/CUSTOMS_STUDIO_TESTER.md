# Customs Studio

The chart editor for **Elite Beat Agents**, **Osu! Tatakae! Ouendan** and
**Ouendan 2**, with the three games recompiled for Windows so you can play what
you make straight away, and the tools that build a patched ROM (a ROM hack) for
a real DS or an emulator.

It is new and experimental: expect rough edges. Share your customs, get help and
report problems on the Discord: https://discord.gg/XGPhewDEtn

**You need your own cartridge dumps.** Nothing of the games is in this download: no
ROM, no charts, no audio. The first time you open the editor it reads the stock
charts out of *your* dumps into a cache (`toolkit\rom_extract\`). Never share
that cache or a built ROM; share a **patch** (`.bps`).

---

## 1. Set up (once)

1. Unzip this anywhere (a path without special characters is safest, like
   `C:\Games\customs-studio`). Don't put it inside Program Files.
2. Put your dumps in, **named exactly like this**:

   | Game | Dump | Where |
   |---|---|---|
   | Elite Beat Agents (USA) | `EBA.nds` | `play\EBA.nds` |
   | Osu! Tatakae! Ouendan (Japan) | `OE1.nds` | `play_oe1\OE1.nds` |
   | Ouendan 2 (Japan) | `OE2.nds` | `play_oe2\OE2.nds` |

   One game is enough to start; EBA is the best-tested. The games must be the
   unmodified dumps (the editor and the games both check them).
3. Optional but recommended: **ffmpeg**, so the editor can convert an MP3 or
   any audio file for you. In a terminal: `winget install Gyan.FFmpeg`, then
   open a new terminal. Without it, give the editor a WAV at 15401 Hz stereo
   that you converted yourself (Audacity: Tracks ▸ Resample).

That's it. No Python or anything else to install for the Studio: it carries
its own. Windows may warn about an unknown publisher (the programs aren't signed):
choose "More info", then "Run anyway".

## 2. Play a game (check the games run first)

Double-click `play\Play EBA.bat` (or `Play OE1.bat`, `Play OE2.bat`). The game
opens in a window; **F1** shows its options bar (resolution, fullscreen, Tap
mode for a controller, and so on). Play with the mouse like a stylus.

The main menu has an orange **CUSTOMS** button: that's where the songs you make
will show up. These copies keep their own saves in their folders, apart from the
launcher's games.

If a game doesn't open, read `play\EBA.log` (same folder as the bat), and post it
on the Discord.

## 3. Open Customs Studio

Double-click `Customs Studio\Customs Studio.exe`.

- The first start takes a while (it extracts the stock charts from your
  dumps). After that it opens in a few seconds.
- It opens as its own window. Closing the window stops the Studio. (On a PC
  without Microsoft's WebView2 it opens in your default browser instead, with a
  small console window: keep that open while you work, and close it to stop.)
- If it fails, it tells you why in a box, and the log is in
  `%LOCALAPPDATA%\eba-recomp\studio\studio.log`. Post that on the Discord.

You land in the **Note Editor**. The buttons at the top of the orange **Customs
Studio** panel switch between it and the **Section Editor**.

**Which one to use:**

| You want to… | Use |
|---|---|
| Put a new song on an existing stage, keeping the stage's chart but retimed to your song | **Section Editor** (the easy, proven path — start here) |
| Move, add or delete individual notes, draw sliders, make a chart your own | **Note Editor** |

## 4. Your first custom (Section Editor)

Goal: a song of yours playing on an EBA stage, in ten minutes.

1. In the Section Editor, pick a chart from the list, e.g. **SS03_n** (Rain
   On Me, Normal; see `toolkit\rom_hacking\SONG_REFERENCE.md` for every
   stage's length and BPM — **pick a stage about as long as your song**, that
   matters most).
2. In the *source audio* box, put the path of your song (MP3, FLAC, WAV…) and
   click **🎚 Convert**. It makes `<name>_15401.wav` next to it and fills the
   WAV field. (No ffmpeg? Put a 15401 Hz stereo WAV's path in the WAV field
   directly.)
3. Set **BPM** to your song's tempo and **offset ms** so the first beat of
   the grid lands on your song's first beat. ▶ Play lets you hear it against
   the chart.
4. Drag the section boundaries so the stage's eight sections sit where you
   want them in your song. Leave *extend chart to song end* ticked: it fills
   a song that's longer than the stage.
5. Click **💾 Build ROM**. The status line says what it did. Builds go to
   `toolkit\rom_hacking\patched_roms\` (`ouendan1\patched_roms\`,
   `ouendan2\patched_roms\` for the Ouendan games).
6. In the orange **Studio bar** at the bottom of the page:
   - **▶ Test in recomp** opens the game straight into your stage. Tick
     *Autoplay* to watch it played perfectly. (The very first test of a game
     takes a few seconds longer: the game makes the start save Customs plays
     from, by running its own first boot in the background.)
   - **✔ Validate** plays the stage through headless (under a minute, no
     window) and reports: did it load, every note judged, score, cleared, and
     how much silence follows the last note. **Do this before sharing**; a
     chart that fails here would hang the real game.
   - **Charted (E N H X)**: untick the difficulties you didn't chart. Players
     can then choose only the ones you made. A custom without all four is fine
     to share, but can't go into a full game conversion or fan campaign.
   - **Export custom folder** puts the song into `play\customs\` so it shows
     up in the CUSTOMS menu of this download's games (type a title and artist
     first). To play it in the launcher's games too, use **Import custom folder**
     on their Customs screen and pick that folder.
   - **…as .zip** makes a zip of that folder, for your own backups or your other
     PCs (it holds game files, so don't post it).
   - **Export patch (.bps)** makes a ROM-hack patch of the stock ROM (3–4 MB)
     for a DS or emulator. Anyone applies it with Flips or
     https://www.marcrobledo.com/RomPatcher.js/ to their own dump.

Then **Copy timing to other diffs** builds the other three difficulties
from the one you retimed, so every difficulty plays your song.

## 5. Editing notes (Note Editor)

Read `toolkit\rom_hacking\tools\EDITOR_GUIDE.md`; the short version:

- Click a note to select it, drag to move, arrows to nudge. Drag on empty
  space to box-select; `Ctrl`+`A` all; `Del` deletes.
- `S` opens the slider designer (osu!-style curves); `E` reshapes a slider.
- **Renumber** sets combo numbers for the whole chart — don't hand-number.
- The last note of a chart **must be a spinner**, notes must be at least 7
  frames apart, and a note can't sit inside a break cutscene. The save checks
  these and tells you; the game would hang otherwise.
- **Export project** saves a `.ebaseq` (`.oe1seq` / `.oe2seq`) you can come
  back to; **Save .seq_** writes the chart the builder uses; **Build ROM**
  then the Studio bar as above.

## 6. Importing an osu! beatmap or a MIDI

That path is command-line for now (`toolkit\rom_hacking\tools\osu2eba.bat`,
`midi2eba.bat`, and `osu2oe1` / `osu2oe2` for the Ouendan games). The batch
files want Python 3 installed (`winget install Python.Python.3.12`, then
`pip install -r toolkit\requirements.txt`); they find your dumps in the
`play` folders by themselves. `toolkit\README.md` and
`toolkit\rom_hacking\tools\WORKFLOW.md` describe them.

## 7. Known limits (don't report these)

- **Ouendan 1** customs are the least tested; a chart that hangs at
  "OUENDAN!" is likely the dance track, a known gap.
- **Ouendan 2** VS/multiplayer charts are wrong on customs; Story is what to
  test.
- EBA stage **SS04** hangs when rebuilt. Avoid it.
- Derived difficulties (from *Copy timing to other diffs* / auto-fill) pass
  every check, but few people have played them.
- **Sharing:** a custom folder holds files from the game (its whole sound
  archive), and every custom holds its song. Share patches (`.bps`) rather than
  folders, and only songs you have the right to share.

## 8. What to send when something breaks

1. What you did, step by step, and what you expected.
2. The Studio's log: `%LOCALAPPDATA%\eba-recomp\studio\studio.log`.
3. For a game problem: `play\EBA.log` (or the other game's), and if it was a
   test run from the Studio, the `recomp.log` in
   `%LOCALAPPDATA%\eba-recomp\studio\run_eba\`.
4. For a build problem: the whole status text, and the `.diffs.json` next to
   the built ROM in `patched_roms`.
5. Your GPU and Windows version.

Don't post ROMs, built ROMs, custom folders, or the `rom_extract` cache.

---

*A fan project, not affiliated with Nintendo or iNiS. The Studio and the games'
recompilation are MIT (LICENSE.txt); third-party notices in THIRD_PARTY.txt;
the toolkit's own licence is in `toolkit\LICENSE`.*
