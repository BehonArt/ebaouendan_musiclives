# Release roadmap

The three recomps (Elite Beat Agents, Osu! Tatakae! Ouendan, Moero! Nekketsu
Rhythm Damashii) released together, on GitHub Releases and in the Quiver
Launcher catalog (quiverlauncher.com: it installs and updates apps from a
GitHub or GitLab repository's release assets, a zip or an MSI).

**Target: v1.0 published and submitted to Quiver by Sunday 8 November 2026** (first target, 2 November, missed: see the re-planned week).
(What is in our hands is the submission; Quiver's review time is not.)

## Next update (v1.0.1)

From the first report on the Discord (8 Nov): a player set Z and X as hit keys, with Tap mode off.

- **Input menu:** grey out the hit keys while Tap mode is off, with a line saying they need it on; no tick on Rumble or DualSense trigger resistance while they are greyed out (a tick there reads as Tap mode's).
- **Keys click at the mouse** (new Input option, osu!-style): a hit key acts as pressing the stylus at the cursor, so the player aims with the mouse; held, it follows sliders and spinners like the mouse button.
- **Achievements:** using that option pauses them, as Tap mode does, from the first key press in a song until the next song. They are only for the intended input method: the mouse button as the stylus (user's rule, 8 Nov).
- Docs for players: README.txt, the changelog.

## Rules

- The package carries the recompiled executables and nothing of the games:
  the player supplies their own ROM dump (checked by hash), as other recomps
  do. No ROMs, saves, custom charts or third-party skins. (Checked: the
  executables hold the translated ARM code and nothing else of the ROM; the
  game's data is read from the player's ROM at run time.)
- No generative AI art, audio or video, and no graphics made from scratch
  that the games never had. Enhancements heighten what the games show (the
  Ouendan fire, sharper approach circles). EBA's spotlights were removed for
  this (2026-10-25). Anything new on screen reuses the games' own sprites.

## v1.0: bare bones

The games themselves, plus the enhancements that matter most:

- **Display:** resolution (to 4K), layout, frame rate (DS, 60, 120, 240,
  uncapped), the smoothing filters, Vsync (auto, on, off), fullscreen, low
  latency.
- **Look:** the picture filter.
- **Effects:** the fire behind the squad (all three games).
- **Controllers:** button mode, hit keys, rumble (high, medium, low) and its
  strength, spinners by the left stick or the triggers, DualSense trigger
  resistance, SKIP by button in the comics and song intros.
- **Timing:** the offset and its calibration (a PC's sound and picture lag).
- **Assists:** a slower or faster approach, a more lenient hit circle, and
  No-Fail (the life gauge held, its bar hidden), in all three games.

Not in v1 (kept in the code, left out of the release build):

- Customs and Customs Studio
- RetroAchievements
- Skins and the HD approach circles (switched off, 4 Nov, until a full HD
  skin is made: `kSkins` in `src/eba.h`)
- Replays saved to disk

## The week (re-planned Monday 2 November)

The first week got the release build, the version numbers, the save folders
and a working launcher done (26 and 27 October); packaging, the crash log,
the player README and the public repo did not happen. The plan from here:

| Day | Work | Done when |
|---|---|---|
| **Mon 2 Nov** | **The launcher, finished**: the project's name (Elite Beat Ouendan: Music LIVES), each game's own icon and title read from the player's dump (nothing of the games ships), a clear line for a wrong file dropped, a controller can pick a game and press Play, Play greyed out until a dump checks out. | Screenshot of the launcher with 0, 1 and 3 dumps set |
| **Tue 3 Nov** ✓ (on Mon 2) | **Packaging**: release builds of all three games (`EBA_RELEASE=ON`), a script that makes the zip (launcher, EBA.exe, OE1.exe, OE2.exe, SDL3 and the MinGW DLLs, the three HD approach-circle packs, `LICENSE`, `THIRD_PARTY.txt`), and a test in Windows Sandbox (a PC that never had MSYS2). | The zip, unpacked in the Sandbox, plays all three |
| **Wed 4 Nov** | **When it goes wrong**: a crash log the player can attach to an issue, and "Copy diagnostics" in the launcher (version, GPU and driver, OpenGL version, settings). A clear message when the PC has no OpenGL 4.5. **You**: the controller pass (DualSense: rumble tiers, trigger resistance; Xbox or Switch if at hand). | A forced crash leaves a readable log |
| **Thu 5 Nov** | **Going public**: the player README (setup, controls, options, known issues), CHANGELOG, screenshots. The public GitHub repo (you create it, or I install the GitHub CLI and you sign in), push, publish **v1.0.0-rc1** as a pre-release. Testers on AMD and Intel graphics get the link. | rc1 on GitHub |
| **Fri 6, Sat 7 Nov** | Tester feedback, fixes only. The Quiver listing ready (name, description, tags, screenshots, the release asset). | No blocking issue open |
| **Sun 8 Nov** | Publish **v1.0.0**; submit to Quiver (your sign-in). | Released; submission in |

v1.0's scope stays as above: all three games, button (Tap) mode and its
controller options, the display and look options, timing, the fire effect,
the assists and No-Fail. Out: Customs and Customs Studio, RetroAchievements,
saved replays, skins and the HD approach circles.

What needs you: the GitHub repo (or a sign-in for the CLI), the controller
pass, two or three testers on other GPUs, the Quiver sign-in, and a yes on
the release day.

Risks: a graphics driver that dislikes the OpenGL 4.5 renderer (why rc1 goes
out Thursday); the SmartScreen warning on an unsigned exe (the README
explains "More info > Run anyway"; signing can wait); Quiver's review time.

Packaging notes (done Mon 2 Nov): `tools\make_release.ps1 -Version 1.0.0` builds the
three games in release mode and the launcher, stages and zips them (24.6 MB), then
puts the build trees back in development mode. This PC runs Windows 11 Home, which
has no Windows Sandbox, so the zip was tested unpacked into a fresh portable folder
with only Windows' own folders on the PATH: all three games boot to their titles
and the launcher opens. A truly clean PC is the testers' job (rc1).
Gap found: only EBA has HD approach-circle packs; Ouendan 1 and 2 have no
`skins_dir`, so their Look menu has no rings yet.

**Wed 4 Nov (done):** re-scoped to the bare essentials: all three games
playable by touch or Tap mode, and the skips
at parity (Ouendan 1's song intro and every game's outro fast-forward on a
button press; the games' own SKIPs stay). `dist\eba-recomp-v1.0.0-windows-x64.zip`
(25.7 MB) rebuilt; unpacked with only Windows on the PATH, all three games
run 500 frames in a window and exit clean, and the launcher opens. Not done:
crash log, "Copy diagnostics", the OpenGL 4.5 message. Still needs you: the
DualSense pass, the public repo, testers on other GPUs, the Quiver sign-in.
Later the same day, after the user's DualSense test: slider ticks' rumble
panned left to right by the ball's place on the screen, and skins and the HD
approach circles switched off until a full HD skin exists.

**Sun 8 Nov (done):** rc1 published Wed 4 Nov (GitHub + Quiver; a Linux
"runs well" report Nov 7). The user chose to put **Customs and
RetroAchievements into 1.0** (Customs as its own menu of individual tracks,
apart from the campaign's progress). Done: the start save Customs plays from
is made by the game itself on the first Play (its own first boot on an empty
save, headless, through the tutorial to the main menu: `Game::first_boot`,
`--port customs-first-boot`), so no save ships and progress does not matter;
the autopilot's route waits for a late scene (a new save's first-time screen
before Ouendan 2's map) and lets it settle; Ouendan 2's route taps the first
stage's character (a fresh map has no book); the customs folder is the
game's data folder in the release, with Open the customs folder and the
imports on the Customs screen; `kRelease` no longer hides Customs or RA.
**Bug found on the way, fixed:** saves the launcher-started games made from
nothing were 8 KiB for every game; EBA and Ouendan 2 have 256 KiB flash
(`Game::save_size`), and a too-small save is padded with erased bytes. All
three games checked headless: start save made in 3-5 s, a custom played and
cleared from it. Still to do before v1.0: the user's windowed checks (Customs
import + Play from an empty data folder, RetroAchievements sign-in in each
game), the release zip, the notes.

## The first week (26 October to 2 November)

The three game executables cannot be merged into one (each is a different
game's translated code, with the same function names), so "one package" is
one zip with a small launcher: a window with the three games, each showing
whether its ROM is set up, a Play button, and where its saves live. Quiver
runs that launcher.

| Day | Work | Done when |
|---|---|---|
| **Mon 26 Oct** ✓ | Decided: repo and launcher name eba-recomp, licence MIT. Release build option that leaves Customs, RetroAchievements, skin packs and replays out. Version numbers (exe, About, RA user agent, window title). Saves and settings move to `%APPDATA%` (portable mode: beside the exe). | `ndsrun` builds in release mode for all three; no Customs/RA menus; saves in `%APPDATA%` |
| **Tue 27 Oct** ✓ | The launcher: pick each game's ROM, check its hash (a clear message for a wrong or modified dump), remember the path, start the game with it. | A fresh folder: launcher, pick ROM, Play, game runs |
| **Wed 28 Oct** | Packaging script: the zip (launcher, three exes, SDL3 and the MinGW DLLs, the HD ring pack), `THIRD_PARTY.txt` (SDL3 zlib, Dear ImGui MIT, rcheevos MIT if linked, ndskit MIT, MinGW runtime), `LICENSE`. Test on a clean Windows user (no MSYS2 on the PATH). | The zip runs on a PC that never had the toolchain |
| **Thu 29 Oct** | Crash log (a file the player can attach to an issue), a "copy diagnostics" button (GPU, driver, settings). Controller pass: DualSense (yours), Xbox; Switch Pro or Joy-Con if one is at hand. | A forced crash leaves a readable log |
| **Fri 30 Oct** | Player README (setup, controls, options, known issues), CHANGELOG, screenshots for the listing. Create the public repo, push, publish **v0.1.0-rc1** as a pre-release. Ask a few testers (other GPUs: AMD, Intel; OpenGL 4.5 is the minimum). | rc1 on GitHub; testers have the link |
| **Sat 31 Oct, Sun 1 Nov** | Tester feedback; fixes only. Prepare the Quiver listing (name, description, tags: Nintendo DS, recompilation, rhythm; screenshots; the repo and its release asset). | Known issues listed, none blocking |
| **Mon 2 Nov** | Publish **v1.0.0**. Submit to Quiver (your sign-in). | Released; submission in |

What needs you: the repo (a GitHub account; `gh` is not installed here, so
either you create the empty repo or I install the CLI and you sign in), the
choice of licence for our code, hardware tests (DualSense, and any Switch
controller), a couple of testers on other GPUs, and the Quiver sign-in.

Risks: a graphics driver that dislikes the OpenGL 4.5 renderer (that is why
rc1 goes out two days early); the Windows SmartScreen warning on an unsigned
exe (explained in the README; signing costs money and can wait); Quiver's
review time.

## After v1.0

- Customs (and Customs Studio)
- RetroAchievements (casual; paused for Customs, button mode, assists)
- An installer and code signing (Windows SmartScreen)
- Linux and Steam Deck builds
- The ndskit patches offered upstream (drafts in `upstream/`)

## An alternate play mode (v1.1 candidate)

osu! turns one beatmap into mania, Catch and Taiko; the same can be done to
the games' charts. What makes it possible here: when a song loads, the game
holds every note in RAM (time, kind, position, combo number), so a mode can
read the whole chart ahead of time, and button mode already turns a button
into a stylus tap the game judges itself (same windows, score, combo, life
gauge, cutscenes). A new mode then needs only a new picture and a rule for
which button counts.

1. **Lane mode (mania / Guitar Hero).** Notes fall down 4 or 5 lanes (by the
   note's x position, or by the direction the chart moves, as osu!'s
   converter does). Taps are gems, sliders are holds (their length is known),
   spinners are a mash. A press counts only in the lane of the note due, so
   the lane is the skill. Gems reuse the games' own circles (their combo
   colours and numbers), the judgement sprites and fire are the games'; only a
   dark lane backdrop is new. Rumble tiers carry over. Largest, the best fit:
   about a week of its own.
2. **Catch mode.** A paddle moves left and right with the stick or d-pad and
   catches notes at their x position; sliders are held; spinners are a mash.
   Cheap once lane mode's overlay exists (a couple of days).
3. **Squad mode (local multiplayer).** Each player owns a combo colour (EBA
   has four, played in groups of 1 to 15), or players alternate groups. No
   new art at all; needs a second controller to test.
4. **Taiko-style, 2 buttons.** Two lanes, alternating. A day on top of lane
   mode.

Recommendation: ship v1.0 as above, make lane mode the headline of v1.1
(the work starts as soon as v1.0 is submitted), and let Catch and Squad
follow from its overlay. A plain prototype (coloured gems, no sprites) can
be built in a day to see whether it plays well before any polish.
