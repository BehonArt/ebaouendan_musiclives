# Changelog

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
