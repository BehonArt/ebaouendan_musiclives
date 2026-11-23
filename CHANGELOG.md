# Changelog

## v1.3.0

Achievements in RecompLauncher. The games, Customs and Customs Studio are otherwise unchanged; if you have 1.2.1, update: your saves, settings, customs and charts are kept.

### New: Recomp Achievements in RecompLauncher
Play the three games from **RecompLauncher** and they earn its own achievements: a set made for each game, **1000 points** each, with pop-ups, Beaten and Mastered, and the leaderboards, all handled by the launcher. Recomp Achievements is in RecompLauncher's beta, for its testers, for now: it reaches everyone as the launcher opens it up.
- **Every story song you clear**, every **"New songs!"** wave on the map on each difficulty, **the credits** on each difficulty (Normal and Hard unlock the next one and its leads), each **overall rank** your scores add up to, **unlocking EBA's and Ouendan 2's bonus songs**, and **S ranks**: your first, 5, 10, 20, then every song on every difficulty.
- **Beaten** the first time the credits roll, on any difficulty; **Mastered** with every achievement.
- **Play your way:** the mouse, a touch screen, Tap mode, a gamepad and the assists (No-Fail too) all count.
- **Autoplay and replays never earn anything.** Customs are a mode of their own and never earn or affect them.
- Achievements are earned as you play: loading a save you already finished earns nothing by itself, and every new clear, rank or S from then on does.
- Started outside RecompLauncher, the games are as before.

## v1.2.1

Customs Studio fixes from the first week of 1.2 customs, plus two new options. The games themselves are unchanged.

### Your favourite songs, charted for you: osu! beatmaps and guitar-game charts become customs
**➕ New song** in Customs Studio turns a song you already have a chart for into a custom in a few clicks:
- **An osu! beatmap:** pick the `.osu` file and its notes, sliders and spinners become the chart, timed by the beatmap itself, over your whole song.
- **A Clone Hero / Rock Band / YARG song folder:** the stems are mixed into the song, the vocal line leads and the band fills in, and all four difficulties come from the song's own Easy to Expert charts.

Both are charted the way the three games are, learned from their own charts: their slider shapes, their hit-sound patterns, the stage's final spinner. Pick a stage, build, and play it in the game. This arrived in 1.2.0; **1.2.1 keeps conversions in sync** after you edit and rebuild them (see Fixed). The guide's section 6 walks through it.

### Also new in Customs Studio
- **No break cutscenes for your song on a stage** (➕ New song, *My song on a stage's chart*): every section and its checkpoint is kept, the break cutscenes are left out, and only the intro and the outro play, with your song running straight through between them. Each section keeps its opening bars at your song's BPM.
- **Ouendan 2's finale customs line up with your song's own intro** (OS15A *Countdown*, OS15B *Sekai wa...*): the new **finale note1@** box in the editors is how many seconds into your song the first note should hit. The stage's opening plays up to that point, fading out under your intro, so the song is in sync on note 1. Left empty, it uses the *First note at* you gave ➕ New song.

### Fixed
- **No-break customs flashed every break cutscene for a second at each section change** once they were rebuilt from an editor: the rebuild put the break shots back. Every build now leaves them out of a no-break chart and holds section 1's background and top screen, and building one difficulty of a no-break set uses the no-break charts for the others too.
- **osu! conversions started late after Build+Test.** When a map's first note comes before the stage's countdown ends, the import pads the song with silence and moves the notes later, but the editors kept the unpadded song and Build+Test used it, so the music ran 9-12 s ahead of the notes. The editors now use the padded copy, and conversions made before the fix are repaired automatically.
- **osu! and MIDI import could stay switched off** for anyone who had tried a pre-release test build of Customs Studio (New song said they "come in a later update"). Updating now clears that.
- **Clicking ahead while the song plays in an editor no longer plays every note in between at once.**
- **Ouendan 2's two-part finale was missing from every stage list** (OS15A *Countdown* and OS15B *Sekai wa Sore o Ai to Yobundaze*, in the Note Editor, the Section Editor and New song).

## v1.2.0

The big one: osu! beatmaps and guitar-game charts become customs right in Customs Studio, customs keep your song at full quality, and the games get a Volume option, faster Customs loading and no story screens after a custom. (v1.0.2 was never released; everything prepared for it is here.)

### New: osu! beatmaps and guitar-game charts become customs
**➕ New song** has two more ways in, beside putting your song on a stage's own chart:
- **From an osu! beatmap:** pick a `.osu` file (its song beside it). Its notes, sliders and spinners become the chart, timed by the beatmap itself, with no BPM to find. The stage is laid over the whole song at its true speed, and its break cutscenes go on the beatmap's quietest stretches (usually its own breaks), so nothing is squeezed and the song doesn't play on after the last note. The stage's other difficulties are made from it.
- **From a guitar-game chart (MIDI):** pick a Clone Hero / Rock Band / YARG song folder (`notes.mid` and its audio stems). The stems are mixed into the song; the vocal line leads and the band fills where it's silent; held notes, chords and drum rolls become sliders; combos follow the song's phrases. All four difficulties come from the song's own Easy to Expert charts. Break cutscenes go on the song's quietest stretches, or at the times you give, or there are none.
- **Both are charted the way the games are,** from what the three games' own charts do: sliders drawn in the games' own shapes, hit sounds in their patterns (a cymbal to open a combo, a cheer to close it, the big cheer on a spinner and into a chorus or solo), the stage's own final spinner, and now and then a mid-song spinner where the song holds a long note or goes quiet, about as often as the games have one. Every chart is checked like the editors' own before it's built.


### Fixed
- **A custom whose chart started with a slider crashed the game** as its song began, in all three games. Such charts now load. Rebuild any custom of yours that starts with a slider.
- **Customs Studio couldn't play Elite Beat Agents' own songs** in the editors, only the Ouendan games'.
- **Opening one of the games' own charts could open a copy you'd saved instead**, or play another song over it. It now opens the game's own chart with its own song.
- **The Section Editor couldn't work on a chart you'd saved** (such as `SS03_n_retime`), and building replaced older charts of the same name without a trace. It works on them now, and keeps the three newest older versions in `_prefresh`.
- **Building in Customs Studio failed when its folder or your Windows user name had Japanese or other non-Latin letters** (Build ROM in the Section Editor, among others).
- **No-breaks customs ended with a few seconds of draining life bar** after their final spinner. *(The `nobreak` tool.)*
- **The Section Editor could build charts that hang or break the game.** Making a section shorter squeezed its notes together: sliders ran into the next note and notes landed on cutscenes, and the build went through anyway. Every chart it writes is now cleaned up the way the Note Editor's Fix density does it, and a build that would still hang stops and says why.
- **Save + Build ROM left the other difficulties on the stage's stock notes** over your song (so did Build + test). With your song set, it now builds all four, each from its newest saved chart (or the one pinned in the Difficulty set). A saved chart of another difficulty that would hang the game is repaired for the build, the way the Section Editor repairs its own, and the build says what it changed; the files you saved stay as they are.
- **Save + Build ROM in Ouendan 2 named a ROM that didn't exist** (`Ouendan2 (…)` for `Ouendan 2 (…)`), so the ROM audit said "not a ROM" and Customs Studio could test or export an older build.
- **The ROM audit never ran in the release Studio** ("No module named rom_audit"), and its panel was hidden behind the Studio panel.
- **A song whose first note came early rushed the stage's intro.** Giving a start time shorter than the stage's intro (5 s into a song, on a stage that needs 15–20 s) squeezed the title card and countdown to fit. Now the intro plays at its normal speed and your song starts a little later. *(New song, and the `make_song` command-line tools.)*

### New in the games
- **Volume:** a slider for the games' sound (songs, voices, effects and customs' songs) in the menu bar's **Sound & Timing** menu (it was called Timing). It starts at **75%**: the games are mixed louder than most of what you play beside them.
- **Customs load faster:** from Play on the Customs screen to the stage takes about half the time (about 4 s instead of 9 on our PC). While the game fast-forwards to the stage, it no longer draws the frames it doesn't show.
- **Story screens no longer follow a custom.** A custom's results used to lead into the story's own screens, such as Ouendan 2's rank-up (格付アップ). Customs mode now skips past them, behind the loading screen, to the Customs screen.
- **Ouendan 2 customs on a Blue Ouendan stage brought the Red Ouendan** (Monkey Magic's stage, among others). Each custom now gets its stage's own team, from the game's own table: Blue on OS02, OS03, OS05, OS06, OS09, OS12, OS14, OSB02 and OSB03; Red on the rest.

### New in Customs Studio
- **No ffmpeg needed:** the Studio converts WAV, MP3, OGG and FLAC itself (for 🎚 Convert, ➕ New song, the imports and full-quality customs). With ffmpeg installed it still uses that, which also opens other formats.
- **The chart list is sorted:** choose to show the games' own charts or yours (your charts, your songs on a stage, auto-made difficulties), and which game. The games' charts show their song titles, and yours show your song and the stage it replaces.
- **A line under the toolbar** says what's loaded, which audio is playing and where Save writes.
- **Each chart plays its own audio** when you load it: your song for your charts, the game's song for the game's.
- **Hit sounds by name:** select a note to see and hear its hit sound from the stage's own set of 16, and choose another from the list. Tick 🎵 to hear them while the song plays.
- **➕ New song:** puts your song on a stage in one step: pick the stage, your song and its BPM (or tap the beat along with it), and it builds. It has the same volume controls as the editors (normalize, then louder or quieter in dB). A song longer than the stage is played to the end: the chart's busiest closing bars repeat on the beat until shortly before the song ends, in all three games.
- **End short sections at a combo** (Section Editor, on by default): a section you make shorter keeps its notes at their normal speed and ends after the last combo that fits, instead of squeezing every note in and speeding them up.
- **Cover art for customs:** choose a picture when you export a custom, and the Customs menu shows it instead of a plain title card.
- **The games' slider shapes** (Note Editor): select a slider and pick any of the 109 shapes the three games use (arcs, S-curves, waves, zigzags), most used first; then flip, turn or resize it. It keeps its start, direction, length and timing.
- **🥁 Auto hit sounds** (Note Editor): gives every note the hit sounds the games would, learned from their own charts: a cymbal to open a combo, a cheer to close it, the big cheer on a spinner and before a break. Undo brings the old ones back.
- **Volume is back on both editors' toolbars** (🔊 volume: norm and dB); it had been tucked into Build options.
- **The Studio panel follows the stage, in both editors.** Title, artist, charted difficulties, cover art and song audio are kept per stage, so they're still there when you switch between the Note Editor and the Section Editor; it tests and exports the newest ROM built for the stage, from either editor; and a project you've opened (such as `MySong_n.oe2seq`) shows its own game.
- **A warning before exporting or testing an out-of-date ROM.** *Copy timing to other diffs* and Save write charts without building them, so an export straight after could carry the older ROM (the Section Editor's shortened sections were missing that way). The Studio now names the charts saved since the ROM was built and suggests Build ROM first.
- **Customs keep your song at full quality.** Export custom folder, its .zip and Test in recomp now carry your song's own file (WAV, FLAC, MP3 or OGG) and play it in place of the DS's 15401 Hz sound, lined up with the chart automatically. The ROM and .bps patches keep the DS's sound, as before. **Song audio** in the Studio panel shows which file it uses (the one behind the WAV you built with, or one you choose); untick **full quality** to use the game's sound. 

## v1.0.1

A bug-fix release.

### Fixed
- **A button press could fast-forward the rest of a song.** Pressing a controller button or hit key after the last note skips a song's outro. The game could mistake the middle of a song for its outro when the song before it was shorter. This affected Tap mode most, and Space or a gamepad button with Tap mode off.
- **Customs:** if the first-time setup failed, it could leave a bad start save that was kept. It is now made again.
- **Customs Studio:** Open builds folder did nothing before the first build.

## v1.0.0

The first full release of Elite Beat Ouendan: Music LIVES.

### New
- **Customs:** play custom songs from a menu of their own (Game > Customs, or the CUSTOMS button on the main menu). Any stage can be played whatever your campaign progress, and scores are kept separately. No songs are included.
  - Import a custom ROM or a custom's folder from the Customs screen.
  - A custom can chart one, two, three or all four difficulties. Only the charted ones can be played.
- **Customs Studio** (experimental): a chart editor for making customs and ROM patches. Open it from the launcher, or choose `Customs Studio.exe` in Quiver. Its guide is `CUSTOMS STUDIO GUIDE.md`.
  - Retime a stage's chart to your song, or edit its notes directly.
  - Test a custom in the game, or check it automatically with Validate.
  - Export to your Customs menu, or as a `.bps` ROM patch to share.
- **RetroAchievements** (casual mode): sign in from Game > RetroAchievements. Achievements count when a game is played as made, by touch with no assists. Customs, or using Tap mode's buttons in a song, pauses them for that song.

### Changed
- **Tap mode is now off by default,** so the mouse plays the game as made. To use a controller or keyboard, press F1 and turn on Input > Tap mode.
- "Button mode" is now called Tap mode everywhere.
- The launcher has a Customs Studio button.

### Fixed
- **Saves:** rc1 created saves that were too small for Elite Beat Agents and Ouendan 2. They are now enlarged automatically and keep what's in them, but progress made under rc1 in those two games may not have saved fully.
- **Paths:** folders and Windows user names with accents or Japanese characters now work.

## v1.0.0-rc1

The first release candidate.

- All three games: Elite Beat Agents (USA), Osu! Tatakae! Ouendan and Ouendan 2 (Japan), recompiled to run natively on Windows. Bring your own cartridge dumps.
- A launcher that checks your dumps, with each game's icon and menu music taken from your own dump.
- Touch play with the mouse, and Tap mode for controllers and keyboards.
- Rumble, DualSense trigger resistance and left-stick spinners, with slider ticks that move across the controller with the ball.
- Skips for comics, intros and outros.
- Display options: resolution up to 4K, layouts, frame rate, Vsync, filters and fullscreen.
- Timing offset and calibration, assists and No-Fail, and the fire effect.
