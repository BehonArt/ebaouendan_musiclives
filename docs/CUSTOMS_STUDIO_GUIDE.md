# Customs Studio guide

Customs Studio is the chart editor for **Elite Beat Agents**, **Osu! Tatakae!
Ouendan** and **Ouendan 2**. Use it to put your own songs on the games' stages,
play them straight away in Customs, and make ROM patches for a real DS or an
emulator.

It is new and experimental, so expect rough edges. For help, to share your
customs, or to report a problem, come to the **Cheers Chooms Discord**:
https://discord.gg/XGPhewDEtn. A sample custom is posted there (**Flower in a
Hurricane over Canned Heat**), questions about the customs process are welcome, and
video tutorials are on the way.

**You need your own cartridge dumps.** Nothing of the games is included. The
first time you open the Studio, it reads the stock charts out of *your* dumps.

---

## 1. Set up

1. In the launcher (`eba-recomp.exe`), choose your dump of each game you want to
   chart for. One is enough to start, and Elite Beat Agents is the best tested.
2. **Audio:** the Studio converts WAV, MP3, OGG and FLAC itself. If you have
   **ffmpeg** installed it uses that instead, which also opens other formats
   (`winget install Gyan.FFmpeg` in a terminal, then restart the Studio).

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
| Put your song on a stage the quick way: the stage's chart, stretched to your song's tempo | **➕ New song** (asks for the stage, your song and its BPM, then builds) |
| The same, placing each section of the stage in your song yourself | **Section Editor** (the proven path: start here for full control) |
| Move, add or delete notes, draw sliders, make a chart your own | **Note Editor** |

**Finding a chart.** In front of the chart list, **Show** picks what it lists and
**Game** picks the game:

- *The games' own charts*: every stage, with its song's title. Opening one always
  gives you the game's own chart and song, even if you saved a copy of it.
- *My charts*, *My song on a stage*, *Auto-made difficulties*: what you've made.
  Each shows your song's name and the stage it replaces, like
  `s12_n_retime · Everything You Want over Over the Distance`.
- *Everything*: all of them.

The line under the toolbar tells you what's loaded, which audio is playing, and
where **Save** writes. Each of your charts loads with the audio you made it with.

## 3. Your first custom (Section Editor)

The goal: your song playing on an Elite Beat Agents stage.

1. **Pick a stage about as long as your song.** That matters most. Choose a chart
   from the list, such as **SS03_n** (the `_n` means Normal). The file
   `SONG_REFERENCE.md` lists every stage's length and BPM (section 7 says where).
2. **Add your song.** Put the path of your song (MP3, FLAC, WAV…) in the *source
   audio* box and click **🎚 Convert**. That makes `<name>_15401.wav` next to it
   and fills in the WAV field.
3. **Line up the beat.** Set **BPM** to your song's tempo, and **offset ms** so
   the grid's first beat lands on your song's first beat. **▶ Play** lets you hear
   the song against the chart.
4. **Place the sections.** Drag the section boundaries so the stage's sections sit
   where you want them in your song. Leave *extend chart to song end* ticked: it
   fills out a song that's longer than the stage. Leave *end short sections at a
   combo* ticked too: a section you make shorter then keeps its notes at their normal
   speed and ends after the last combo that fits, instead of squeezing them together.
5. **Click 💾 Build ROM.** The status line says what it did. Every chart is cleaned
   up as it's built (notes back on the beat, nothing on a cutscene, room after every
   slider). If one would still hang the game, the build stops and lists the problems
   with their times in the song: give those sections more room and build again.

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
- **Cover art:** choose a picture of yours (PNG, JPG, BMP or WebP) and the Customs
  menu shows it instead of a plain title card. It goes into the custom folder and its
  .zip; a .bps patch can't carry it. Only use art you have the right to share.
- **Song audio:** customs play your song's own file at full quality instead of the
  DS's 15401 Hz sound (a ROM and its .bps patch keep the DS's sound). It's found for
  you: the file behind the WAV you built with (`<name>.mp3` beside
  `<name>_15401.wav`, or the WAV itself when it's better than 15401 Hz). **Choose…**
  picks another; **auto** goes back to the one found. Untick **full quality** to use the
  game's sound. It must be the same song you built with: the Studio lines the two up
  and uses the game's sound if they don't match (the message after Export says which).
- **Export patch (.bps)** makes a ROM patch of about 3–4 MB. This is how to share
  your custom.

**Copy timing to other diffs** makes the other three difficulties from the one you
retimed, so every difficulty plays your song. It saves them but doesn't build: click
**💾 Build ROM** afterwards to put them in the ROM. If you export or test a ROM older
than charts you've saved, the Studio names those charts and suggests Build ROM first.

**Ouendan 2's two teams.** Each Ouendan 2 stage has its own Ouendan, and your custom
gets that stage's: pick the stage with the team you want.

| Team | Stages |
|---|---|
| **Blue** | OS02, OS03, OS05, OS06, OS09, OS12, OS14, OSB02, OSB03 |
| **Red** | OS01, OS04, OS07, OS08, OS10, OS11, OS13, OS15A, OS15B, OSB01 |

The Section Editor always starts from the stage's own chart, even when you open one
of your saved charts, and saves as `<stage>_section` (for example `SS03_n_section`).
A chart of that name from before is kept in the `_prefresh` folder beside it (the
three newest).

**➕ New song** does the retime in one step: pick the stage and difficulty (or all
four), your song, and its BPM. Don't know the BPM? Play your song and click **Tap
the beat** along with it. It builds the ROM and opens the new chart with your song.

- **A song longer than the stage** is charted to the end: with *Stretch the chart to the song's
  end* ticked, the busiest closing bars repeat on the beat until shortly before your song ends.
- **First note at** is where in your song the first note should land. If that's sooner than the
  stage's intro needs (its title card and countdown), your song simply starts a little later, so
  the intro still plays at its normal speed.

## 4. Editing notes (Note Editor)

- Click a note to select it, drag to move, and use the arrow keys to nudge. Drag on
  empty space to box-select. `Ctrl`+`A` selects all and `Del` deletes.
- `S` opens the slider designer (osu!-style curves), and `E` reshapes a slider.
- **Renumber** sets the combo numbers and colours for the whole chart. Use it rather
  than numbering by hand.
- **Hit sounds:** each stage has its own 16 hit sounds (a clap, a whistle, a shout…),
  and a note's `sfx` number picks one. Select a note to see its sound by name, choose
  another from the list, and press ▶ to hear it. Tick 🎵 to hear every note's sound
  while the song plays.
- The last note of a chart **must be a spinner**, notes must be at least 7 frames
  apart, and a note can't sit inside a break cutscene. Saving checks these and tells
  you, because the game could hang otherwise.
- **Colours:** the game draws a note in the colour of the note *before* it. Renumber
  handles this for you, and the editor shows each note as the game will. If you set
  a colour group (`grp`) by hand, remember that it colours the next note.
- **Export project** saves a `.ebaseq` (`.oe1seq`, `.oe2seq`) that you can come back
  to. **Save .seq_** writes the chart the builder uses.
- **Save + Build ROM** builds all four difficulties when your song is in the WAV field:
  the one you have open, and each other difficulty's newest saved chart, so none of them
  plays the stage's own notes over your song. A saved chart of another difficulty that would
  hang the game is repaired for the build (the status line says what changed). The
  **Difficulty set** panel shows which file each difficulty comes from; a pin there wins over
  newer files (**Clear pins** goes back to the newest). One that can't be repaired stops
  the build, with its name and the problem.
- **Volume:** *Norm* brings your song up to full level (customs sound quiet otherwise), and
  the dB field then makes it louder (+) or quieter (−). The Section Editor and ➕ New song
  have the same two controls.

The full reference is `EDITOR_GUIDE.md`, in the `tools` folder (section 7).

## 5. Sharing your custom

**Share the patch (`.bps`).** It contains your chart and song but none of the game's
files. To play it, someone needs their own dump of the game:

1. Apply the patch to their dump with [Flips](https://github.com/Alcaro/Flips) or
   [Rom Patcher JS](https://www.marcrobledo.com/RomPatcher.js/).
2. Then either play that ROM on a DS or emulator, or use **Import custom ROM** on the
   game's Customs screen.

**Where to share:** the [Cheers Chooms Discord](https://discord.gg/XGPhewDEtn), where
customs are posted and played. Post the `.bps` with the song's title and artist, the
stage it replaces and which difficulties you charted.

**Please don't post** ROMs, custom folders (they contain the game's whole sound
archive) or the Studio's `rom_extract` folder. A patch still contains your song, so
only share songs you have the right to share.

## 6. Importing an osu! beatmap or a guitar-game chart

Click **➕ New song** in the Note Editor and choose a tab. Both kinds build all four
difficulties into one ROM and open the new chart with its song, ready for the Customs
Studio panel (Test, Validate, Export).

**First, pick the stage.** The stage you replace decides the look, the Ouendan or Agents
(Ouendan 2's Blue and Red teams: section 3), the hit-sound kit and how many break
cutscenes there are. With the stage laid over the whole song, its length matters less
than it does for a retime.

**From an osu! beatmap**
1. **Beatmap:** pick the `.osu` file of the difficulty you want as the stage's base.
   Its song file must be beside it (as in any osu! song folder).
2. Leave **No break cutscenes** unticked to keep the stage's cutscenes: each goes on
   the beatmap's quietest stretch near where the stage has it (most beatmaps mark their
   breaks, and those are where they land). Tick it and the song plays straight through.
3. **Build.** The other three difficulties are made from this one, thinned to each
   difficulty's own note count.

**From a guitar-game chart (MIDI)**
1. **Chart:** pick `notes.mid` in a Clone Hero / Rock Band / YARG song folder; its audio
   stems (`song.ogg`, `guitar.ogg`, `vocals.ogg`…) are mixed into the custom's song.
2. **No break cutscenes** is ticked by default. Untick it to keep the cutscenes: give
   the song time each should **start** at (`1:05, 2:10`…), or leave it on *auto* for the
   quietest stretches.
3. **Song starts at** trims an intro; **volume** sets the stems' mix level, before the
   usual **normalize** and dB.
4. **Build.** Each difficulty comes from the song's own chart of it (its Easy, Medium,
   Hard and Expert parts).

**What the conversion does for you**
- **Sliders** come from held notes, chords and drum rolls (MIDI) or the beatmap's own
  (osu!), drawn in the shapes the three games use.
- **Hit sounds** follow the games' patterns: a cymbal or hat opens a combo, a cheer
  closes it, the big cheer goes on a spinner and before a break, and a chorus or a solo
  (from the song's section markers, or a beatmap's "finish" sounds) opens on a cymbal.
  Change any of them afterwards, or click **🥁 Auto hit sounds** on a chart of your own.
- **Spinners:** the stage's own final spinner ends the chart, as in the games, and a
  long held note or a quiet stretch can become a mid-song spinner, about as often as
  the games have one.
- **Safety:** every chart is checked like the editors' own (spacing, slider room,
  cutscenes, the game's note-group limit) and repaired where it can be before it's built.

**After the build:** open each difficulty in the Note Editor to tidy it, use **Test in
recomp** and **Validate**, then **Export custom folder** (with your song at full quality)
or **Export patch (.bps)**. Only convert beatmaps and charts you have the right to share,
and credit their makers.

The same imports are in the toolkit's `tools` folder as command-line tools for
batches: `osu2eba.bat`, `osu2oe1.bat`, `osu2oe2.bat` and `midi2eba.bat` (they need
Python 3; see `WORKFLOW.md`).

## 7. Where your files are

Everything you make is kept in your user folder, so updating the game never touches
it: `%APPDATA%\eba-recomp\Studio\toolkit\rom_hacking\`

| What | Where in there |
|---|---|
| Built ROMs and patches (**Open builds folder** in the panel goes here) | `patched_roms\` (Ouendan games: `ouendan1\patched_roms\`, `ouendan2\patched_roms\`) |
| Saved charts | `charts\edited\` (older versions the Studio replaced: `charts\edited\_prefresh\`) |
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
- An osu! beatmap that stops before its song does (an unmapped outro) leaves the rest of
  the song with nothing to hit; Validate tells you how long. Trim the song, or chart the
  end in the Note Editor.

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
