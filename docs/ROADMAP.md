# Release roadmap

The three recomps (Elite Beat Agents, Osu! Tatakae! Ouendan, Moero! Nekketsu
Rhythm Damashii) released together, on GitHub Releases and in the Quiver
Launcher catalog (quiverlauncher.com: it installs and updates apps from a
GitHub or GitLab repository's release assets, a zip or an MSI).

**Target: published and submitted to Quiver by Monday 2 November 2026.**
(What is in our hands is the submission; Quiver's review time is not.)

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
- **Look:** HD approach circles (normal, thin, thinner).
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
- Skins other than the HD approach circles (a player's own packs still load
  from a `skins` folder; none ship)
- Replays saved to disk

## The week

The three game executables cannot be merged into one (each is a different
game's translated code, with the same function names), so "one package" is
one zip with a small launcher: a window with the three games, each showing
whether its ROM is set up, a Play button, and where its saves live. Quiver
runs that launcher.

| Day | Work | Done when |
|---|---|---|
| **Mon 26 Oct** | Decide: repo name, our licence, launcher name. Release build option that leaves Customs, RetroAchievements, skin packs and replays out. Version numbers (exe, About, RA user agent). Saves and settings move to `%APPDATA%` (portable mode: beside the exe). | `ndsrun` builds in release mode for all three; no Customs/RA menus; saves in `%APPDATA%` |
| **Tue 27 Oct** | The launcher: pick each game's ROM, check its hash (a clear message for a wrong or modified dump), remember the path, start the game with it. | A fresh folder: launcher, pick ROM, Play, game runs |
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
