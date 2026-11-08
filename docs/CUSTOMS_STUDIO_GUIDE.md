# Customs Studio guide

Customs Studio is the chart editor for **Elite Beat Agents**, **Osu! Tatakae!
Ouendan** and **Ouendan 2**. Use it to put your own songs on the games' stages,
play them straight away in Customs, and make ROM patches for a real DS or an
emulator.

It is new and experimental, so expect rough edges. For help, to share your
customs, or to report a problem, come to the Discord:
https://discord.gg/XGPhewDEtn

**You need your own cartridge dumps.** Nothing of the games is included. The
first time you open the Studio, it reads the stock charts out of *your* dumps.

---

## 1. Set up

1. In the launcher (`eba-recomp.exe`), choose your dump of each game you want to
   chart for. One is enough to start, and Elite Beat Agents is the best tested.
2. **Optional but recommended:** install **ffmpeg**, so the editor can convert an
   MP3 or other audio file for you. In a terminal, run
   `winget install Gyan.FFmpeg`, then restart the Studio. Without it, give the
   editor a 16-bit WAV file.

Nothing else needs installing: the Studio carries its own Python.

## 2. Open Customs Studio

Press **Customs Studio** at the bottom of the launcher. In Quiver, choose
`Customs Studio.exe` when you press Play. Or run it directly from the
`Customs Studio` folder.

- Windows may warn about an unknown publisher, because the programs aren't
  signed. Choose **More info**, then **Run anyway**.
- A terminal window may flash for a moment, and the first start takes a little
  longer. That's normal.
- It opens as its own window, and closing the window stops the Studio. On a PC
  without Microsoft's WebView2, it opens in your default browser instead, with a
  small console window: keep that open while you work, and close it to stop.
- If you added a dump while the Studio was open, restart the Studio.

You land in the **Note Editor**. The buttons at the top of the **Customs Studio**
panel switch between it and the **Section Editor**.

| You want to… | Use |
|---|---|
| Put a new song on a stage, keeping that stage's chart but retimed to your song | **Section Editor** (the easy, proven path: start here) |
| Move, add or delete notes, draw sliders, make a chart your own | **Note Editor** |

## 3. Your first custom (Section Editor)

The goal: your song playing on an Elite Beat Agents stage.

1. **Pick a stage about as long as your song.** That matters most. Choose a chart
   from the list, such as **SS03_n** (the `_n` means Normal). The file
   `SONG_REFERENCE.md` lists every stage's length and BPM (section 7 says where).
2. **Add your song.** Put the path of your song (MP3, FLAC, WAV…) in the *source
   audio* box and click **🎚 Convert**. That makes `<name>_15401.wav` next to it
   and fills in the WAV field. No ffmpeg? Put the path of a 16-bit WAV in the WAV
   field directly.
3. **Line up the beat.** Set **BPM** to your song's tempo, and **offset ms** so
   the grid's first beat lands on your song's first beat. **▶ Play** lets you hear
   the song against the chart.
4. **Place the sections.** Drag the section boundaries so the stage's sections sit
   where you want them in your song. Leave *extend chart to song end* ticked: it
   fills out a song that's longer than the stage.
5. **Click 💾 Build ROM.** The status line says what it did.

Then use the **Customs Studio** panel:

- **▶ Test in recomp** opens the game straight into your stage. Tick *Autoplay* to
  watch it played perfectly. The first test of a game takes a few seconds longer.
- **✔ Validate** plays the stage through with no window, in under a minute. It
  reports whether it loaded, how every note was judged, the score, whether it
  cleared, and how much silence follows the last note. **Do this before sharing:**
  a chart that fails here could hang the real game.
- **Charted (E N H X):** untick the difficulties you didn't chart. Players can then
  choose only the ones you made. A custom with fewer than all four is fine to
  share, but can't go into a full game conversion or fan campaign.
- **Export custom folder** adds the song to the Customs menu of your games. Type a
  title and artist first.
- **Export patch (.bps)** makes a ROM patch of about 3–4 MB. This is how to share
  your custom.

**Copy timing to other diffs** builds the other three difficulties from the one you
retimed, so every difficulty plays your song.

## 4. Editing notes (Note Editor)

- Click a note to select it, drag to move, and use the arrow keys to nudge. Drag on
  empty space to box-select. `Ctrl`+`A` selects all and `Del` deletes.
- `S` opens the slider designer (osu!-style curves), and `E` reshapes a slider.
- **Renumber** sets the combo numbers and colours for the whole chart. Use it rather
  than numbering by hand.
- The last note of a chart **must be a spinner**, notes must be at least 7 frames
  apart, and a note can't sit inside a break cutscene. Saving checks these and tells
  you, because the game could hang otherwise.
- **Colours:** the game draws a note in the colour of the note *before* it. Renumber
  handles this for you, and the editor shows each note as the game will. If you set
  a colour group (`grp`) by hand, remember that it colours the next note.
- **Export project** saves a `.ebaseq` (`.oe1seq`, `.oe2seq`) that you can come back
  to. **Save .seq_** writes the chart the builder uses.

The full reference is `EDITOR_GUIDE.md`, in the `tools` folder (section 7).

## 5. Sharing your custom

**Share the patch (`.bps`).** It contains your chart and song but none of the game's
files. To play it, someone needs their own dump of the game:

1. Apply the patch to their dump with [Flips](https://github.com/Alcaro/Flips) or
   [Rom Patcher JS](https://www.marcrobledo.com/RomPatcher.js/).
2. Then either play that ROM on a DS or emulator, or use **Import custom ROM** on the
   game's Customs screen.

**Please don't post** ROMs, custom folders (they contain the game's whole sound
archive) or the Studio's `rom_extract` folder. A patch still contains your song, so
only share songs you have the right to share.

## 6. Importing an osu! beatmap or a MIDI

This is command-line for now, and needs Python 3 (`winget install Python.Python.3.12`,
then `pip install -r requirements.txt` in the toolkit folder from section 7). The
tools are `osu2eba.bat` and `midi2eba.bat`, or `osu2oe1` / `osu2oe2` for the Ouendan
games, in the `tools` folder. They find the dumps you chose in the launcher.
`WORKFLOW.md` describes them.

## 7. Where your files are

Everything you make is kept in your user folder, so updating the game never touches
it: `%APPDATA%\eba-recomp\Studio\toolkit\rom_hacking\`

| What | Where in there |
|---|---|
| Built ROMs and patches (**Open builds folder** in the panel goes here) | `patched_roms\` (Ouendan games: `ouendan1\patched_roms\`, `ouendan2\patched_roms\`) |
| Saved charts | `charts\edited\` |
| Exported projects | `charts\projects\` |
| Guides: `EDITOR_GUIDE.md`, `WORKFLOW.md` | `tools\` |
| Stage lengths and BPMs: `SONG_REFERENCE.md` | here |

Your exported customs and their scores are in `%APPDATA%\eba-recomp\EBA\customs\`
(`OE1`, `OE2` for the Ouendan games). With `portable.txt` beside `eba-recomp.exe`, all
of this is in `userdata\` beside it instead of `%APPDATA%\eba-recomp`.

## 8. Known limits

- **Ouendan 1** customs are the least tested. A chart that hangs at "OUENDAN!" is
  likely the dance track, a known gap.
- **Ouendan 2** VS and multiplayer charts are wrong on customs. Story mode is what to
  test.
- Elite Beat Agents stage **SS04** hangs when rebuilt, so avoid it.
- Difficulties made by *Copy timing to other diffs* or auto-fill pass every check, but
  few people have played them.
- Exported customs play at DS audio quality. One-file custom packs with HD audio are
  planned.

## 9. When something breaks

Post on the [Discord](https://discord.gg/XGPhewDEtn) with:

1. What you did, step by step, and what you expected.
2. The Studio's log: `%LOCALAPPDATA%\eba-recomp\studio\studio.log`
3. For a game problem, the game's log from its save folder (the launcher's **Save
   folder** button). For a test from the Studio, `recomp.log` in
   `%LOCALAPPDATA%\eba-recomp\studio\run_eba\` (`run_oe1`, `run_oe2`).
4. For a build problem, the whole status text and the `.diffs.json` next to the built
   ROM.
5. Your graphics card and Windows version.

Please don't post ROMs or the `rom_extract` folder.

---

*A fan project, not affiliated with Nintendo or iNiS. The Studio and the games'
recompilation are MIT (`LICENSE.txt`), with third-party notices in `THIRD_PARTY.txt`.
The toolkit's own licence is `toolkit\LICENSE`.*
