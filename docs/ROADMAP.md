# Release roadmap

The three recomps (Elite Beat Agents, Osu! Tatakae! Ouendan, Moero! Nekketsu
Rhythm Damashii) released together, on GitHub Releases and in the Quiver
Launcher catalog (quiverlauncher.com: it installs and updates apps from a
GitHub or GitLab repository's release assets, a zip or an MSI).

## Rules

- The package carries the recompiled executables and nothing of the games:
  the player supplies their own ROM dump (checked by hash), as other recomps
  do. No ROMs, saves, custom charts or third-party skins.
- No generative AI art, audio or video, and no graphics made from scratch
  that the games never had. Enhancements heighten what the games show (the
  Ouendan fire, sharper approach circles). EBA's spotlights were removed for
  this (2026-10-25).

## v1: bare bones

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
  No-Fail (the life gauge held, its bar hidden). Today: the approach and hit
  circle and No-Fail in all three games.

Not in v1 (kept in the code, left out of the release build):

- Customs and Customs Studio
- RetroAchievements
- Skins (osu! skins; only the player's own could be used anyway)
- Replays saved to disk

## Before v1 ships

1. A release build that leaves the later features out (a CMake option, not
   deleted code).
2. ~~The assists for all three games~~: done 2026-10-25 (Ouendan 2's approach,
   hit radius and gauge switch; Ouendan 1's gauge switch; both life bars).
3. First run: point each game at its ROM, checked by hash; a clear message
   for a wrong or modified dump.
4. Saves and settings in `%APPDATA%`, not beside the executable (an install
   folder may not be writable); a portable mode keeping them beside it.
5. One package for the three games (one launcher with a game picker, or three
   executables in one folder: to be decided).
6. A clean Windows machine: no MSYS2, other GPUs (AMD, Intel; OpenGL 4.5),
   high DPI. Every DLL the package needs is in it.
7. Real controllers: DualSense, Switch Pro or Joy-Con, Xbox.
8. A crash leaves a log the player can attach to an issue.
9. Version numbers (the executables, About), a CHANGELOG.
10. Third-party licences: SDL3 (zlib), Dear ImGui (MIT), the MinGW runtime,
   ndskit (check its licence before release).
11. A player's README: setup, controls, the options bar, known issues.
12. A public repository with GitHub Releases (zip; an installer later), then
    the Quiver submission (the user's account).

## Later releases

- Customs (and Customs Studio)
- RetroAchievements (casual; paused for Customs, button mode, assists)
- An installer and code signing (Windows SmartScreen)
- Linux and Steam Deck builds
- The ndskit patches offered upstream (drafts in `upstream/`)
