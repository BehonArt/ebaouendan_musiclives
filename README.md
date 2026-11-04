# Elite Beat Ouendan: Music LIVES (eba-recomp)

**Elite Beat Agents, Osu! Tatakae! Ouendan and Moero! Nekketsu Rhythm Damashii
Osu! Tatakae! Ouendan 2 on Windows**, recompiled from their own code to native
PC programs, with a launcher, Tap mode for controllers (rumble, DualSense
triggers), HD approach circles, display and look options, assists and
No-Fail, all three games alike.

**You need your own cartridge dumps.** Nothing of the games is in this
repository or its releases: the launcher asks for each game's `.nds` dump
(EBA USA, Ouendan 1 and 2 Japan) and checks it. The release zip is on the
[Releases](../../releases) page: unpack it anywhere, run `eba-recomp.exe`,
point each game at its dump, play. Windows 10 or 11, 64-bit, a graphics
driver with OpenGL 4.5. The README in the zip covers controls and options.

The project's own code is MIT (`LICENSE`). Built on
[ndskit](https://github.com/vs-sr-dev/ndskit) (a static recompiler and
runtime for the Nintendo DS, MIT), SDL3, Dear ImGui and rcheevos. Not
affiliated with Nintendo or iNiS.

Everything below is the developer's notes.

---

# eba-recomp

Static recompilation of Elite Beat Agents (USA, AOSE), Osu! Tatakae!
Ouendan (Japan, AOSJ) and Moero! Nekketsu Rhythm Damashii (Japan, AO2J) to
native PC code with [ndskit](https://github.com/vs-sr-dev/ndskit)
(submodule at `ndskit/`). The ndskit submodule needs the patches in
`patches/` applied before building (see "Local ndskit patches").

## Status (2026-10-22)

All three games boot headless with no interpreted functions, through the
title, menus, stage map, a stage's intro comic, gameplay and the fail screen,
with the real saves loaded and the music in the WAV.

| | EBA | OE1 | OE2 |
|---|---|---|---|
| ARM9 functions | 4,517 (6 refuse passes) | 3,226 | 4,086 program + 1,191 in 7 overlays |
| ARM7 functions | 1,026 | 1,004 | 1,026 (EBA's ARM7) |
| unresolved calls / jumps (ARM9 program) | 841 / 23 | 437 / 3 | 691 / 15 |
| self-test vectors, game functions | 24,346 (1,614 functions) | 18,280 (1,183) | 28,690 (1,866; 4,690 through the overlay dispatch) |
| failures | 0 | 0 | 0 |
| save | 256 KiB flash | 8 KiB EEPROM | 256 KiB flash |
| stage played | "The Anthem" | "ループ&ループ" | "サムライブルー" |
| windowed play | timing and audio right (user) | timing and audio right (user) | works great (user), through the overlay swaps into a song |

Plus ndskit's instruction test: 57,781 vectors, 0 failures. OE2's overlays
change hands as the game goes (6 then 1 at boot, 0 for the stage, 5 for
gameplay, 6 again at the fail screen), each found by the runtime. Known
runtime gap seen: 3D edge marking is not drawn (the EBA and OE2 stage maps).

## The options bar (all three games)

EBA's window has a menu bar (`src/eba_ui.cpp`, Dear ImGui in
`third_party/imgui`, v1.92.9b):

- **Game:** screenshot (F12), quit (Esc).
- **Display:** resolution (Auto, or 1x to 6x), layout (vertical, side by
  side, top screen only, bottom screen only; F9 cycles), frame rate (DS,
  60, 120, 240, uncapped), Vsync (Auto: on when the display's refresh rate
  suits the frame rate; On: always, no tearing; Off: never, the least
  latency; `--vsync auto|on|off`), fullscreen (F11, Alt+Enter), low latency.
- **Look:** the filter; skins and HD approach circles (off, normal, thin,
  thinner), all three games.

No menu has submenus (in a small window ImGui puts one over its menu): each
choice is a list under a grey heading.
- **Input:** button mode on or off, rumble and its strength, up to four hit keys (click one, then
  press a key; right-click clears), the connected gamepads, the key map.
- **Timing:** the offset (-8 to +8 frames), the sound's latency (1 to 6
  frames), and **Calibrate...**, a window that counts every tap the game
  judges by how many frames after its note it came (before the offset):
  a histogram, the average and spread, the grades, and **Apply** to set the
  offset to the average (from 20 taps).

Every change is made at once and saved in `%APPDATA%\eba-recomp\EBA\settings.ini`
(SDL's folder for the user's settings); ndsrun's own options win for a run
that gives them, and headless runs ignore the file. In a window the bar sits
above the screens and the window makes room for it; in fullscreen it shows
while the mouse is near the top; F1 hides or shows it. While the mouse is on
the bar or a menu, it is not the stylus, and Esc closes the menu, not the
game.

## Timing offset (all three games)

The judge (`0x020681D8`, every tap and slider head; spinners are not timed)
is recompiled with a hook (`--hook 020681D8`); `src/eba_timing.cpp` gives it
the song's clock minus the offset, so a tap made N frames late is judged as
on time. The picture and the music are untouched, so it suits a player who
follows the sound or the picture alike. Checked headless with the button
mode's test player pressing every note 4 frames late: offset 0, every note
a 100 and the stage failed; offset 4, rank S, 258 of 258 at 300. Options:
`--port offset=N`, `--port timing-log` (every judged tap). The playtest that
asked for it: 241 presses averaged 1.8 frames late (about 30 ms), 99 of them
100s.

## Display options (EBA's window; any ndskit game)

    build/recomp/b/ndsrun roms/EBA.nds --save build/play/EBA.sav --window --scale 3 --fps 60 --layout side

All of these also change while the window runs (the options bar, or F9 and
F11).

| option | what it does |
|---|---|
| `--layout stacked\|side\|top\|bottom` | the screens one above the other (default), side by side (for wide displays), or one alone. The stylus follows the lower screen wherever the layout puts it (none with the upper alone). |
| `--low-latency` | waits for each picture to be drawn before the next (glFinish), so the driver queues none ahead of the display. |
| `--audio-latency F` | the sound queued ahead of the device, in frames (3 by default; 1.5 held without a dropout at 60). |
| `--scale N` | composes both screens at N times the DS's resolution (1-6): 1x 256x384, 2x 512x768, 3x 768x1152, 4x 1024x1536, 5x 1280x1920, 6x 1536x2304 (the first to cover a 4K display's height). The 3D draws at that resolution; the 2D layers stay the DS's pixels, sharp. The window opens at that size, fitted to the display; a larger window gets a sharp-bilinear upscale, a smaller one the mean of the pixels it covers (supersampling). Without `--scale` the picture follows the window's size, as before. |
| `--filter sharp\|bilinear\|bicubic\|soft` | the picture's filter, on the DS's pixels (Display > Filter): sharp as before; bilinear soft; bicubic (Catmull-Rom) smooth but fairly crisp; soft (a cubic B-spline) the most blended. The 3D is resampled with the 2D. |
| `--fullscreen` | the whole display, letterboxed at the DS's shape. F11 or Alt+Enter toggles it. |
| `--fps ds` | the default: the DS's 59.83 frames a second, presented with the vertical sync (on a 60 Hz display one frame shows twice every 5.8 s). |
| `--fps 60` / `120` / `240` | the game at exactly 60 frames a second (0.29% faster than the DS, the sound resampled with it: about 5 cents, inaudible), each frame shown 1, 2 or 4 times. On a display at that refresh rate the vertical sync is the clock, so every frame lasts the same: no judder. On another display, paced by the clock. |
| `--fps uncapped` | the DS's pace, pictures as fast as the GPU draws them, without the vertical sync. |

The title bar shows the last second's frames and pictures. The log ends with
the run's averages (after its first 60 frames), how many frames came late,
the longest, and the longest the game itself took to run one; any stall
over 50 ms is noted as it happens. F12 also saves the composed picture at
full scale (`shotN-3x.png`).

Measured on this PC (RTX 4060, display 2715x1527 at 60.000 Hz, NVIDIA's
"Vertical sync" left to the application), 20 s from the boot each:

| `--fps` | game frames a second | pictures a second |
|---|---|---|
| `ds` | 59.80 to 59.83 | 60, with the sync |
| `60` | 60.00, no frame late | 60.0, the sync the clock |
| `120` | 59.95 | 120.0 |
| `240` | 59.95 | 235.8 |
| `uncapped` (fullscreen, 4x) | 59.73 | 2,166 |

The game itself never took more than 6 ms for a frame. On this 60 Hz display,
120 and 240 run without the sync, so they can tear; on a display at 120 or
240 Hz they lock to it the way 60 does here. Above 60, each extra picture
repeats the game's last frame: the DS draws nothing between frames. The
planned HD note layer will draw the hit circles, approach circles, slider
balls and spinners between frames, which is what makes 120/240/uncapped
smoother. If a driver keeps the vertical sync on against the runtime's
request (NVIDIA Control Panel's "Vertical sync: On" did), the log says so.

## Button mode (all three games)

For players without a touchscreen: any face button of an Xbox, PlayStation
or Switch controller, or Space on the keyboard, in place of the stylus during
a song. It is on in EBA's build (`src/eba_buttons.cpp`, linked into `ndsrun`
by `cmake/eba.cmake`).

- **Hit circle:** press on the beat. The stylus touches the note the game is
  judging, for two frames; the game judges it as its own tap, with its own
  windows (300 at +-2 frames, 100 at +-5, 50 at +-8, and a press more than 8
  frames early is a miss, as with a stylus).
- **Slider:** press on its head and hold; the stylus follows the ball, a path
  point a frame, through every pass and the tail.
- **Spinner:** mash. Each press spins the stylus faster around the spinner
  (2 turns a second per press, halving in 0.4 s without presses, at most 10
  a second); about 6 presses a second keeps up with the hardest spinners.
  Or turn the **left stick** round (past 60% of its throw): the stylus turns
  with it, 1.5 times as far, either way, on top of any presses (at most 18
  turns a second). Checked headless (`stick-auto=N`, a stick turning N times
  a second): The Anthem with the stick at 5 a second scores 3,349,920, the
  same as mashing; held still, 3,322,660 (the spinner's bonus lost). Ouendan
  2's first stage: 2,947,250 by stick, 2,949,450 by mashing.
  Or mash the **triggers** (L2, R2: a press past 60% of the pull, released
  under 35%), counted as presses on a spinner only. A DualSense's adaptive
  triggers push back while a spinner turns (Input > DualSense trigger
  resistance, on by default): continuous resistance (SDL's DualSense effect
  report, mode 1) of 120 (of 255) at the start, easing slowly with the
  spinner's speed (smoothed over about 2 seconds) to none at full speed;
  off after it. Headless (`buttons-log`): at 7.5 presses a second it eases
  120 to 40 over 115 frames and is still easing when the finale spinner
  ends (it was 200 to 0 over 80 frames).
- The mouse still works and wins while it is down. Outside a song the face
  buttons are the DS's (south B, east A, west Y, north X), for the menus.

Options, each as `--port ...`: `buttons=off`; `button-keys=Space,F,J` (SDL
key names); `buttons-log` (every press, the note it hit and how many frames
off); `buttons-auto` (a test player: presses on every note's frame, holds
the sliders, mashes the spinners; with `stick-auto[=N]` it spins them with a
left stick turning N times a second instead).

    build/recomp/b/ndsrun roms/EBA.nds --save build/play/EBA.sav --window --scale 3 --fps 60 --port buttons-log

Checked headless: `buttons-auto` from the boot through The Anthem on Hard
Rock, everything through button mode: rank S, 258 of 258 at 300, max combo
408, the stage cleared. With no button pressed, runs are byte-identical to
before. What it reads of the game (the song clock, the notes' manager and
each note's fields) is at the top of `src/eba_buttons.cpp`;
`tools/eba_notes.py` inspects a RAM dump.

**Rumble** (Input menu, on by default, with a strength; `--port rumble=off`,
`rumble-strength=N`, `rumble-log`; `src/eba_rumble.cpp`), in button mode,
through SDL's rumble (a DualSense's haptics, a Switch Pro Controller's or
Joy-Con's HD rumble, Switch 2 controllers where SDL 3.4 drives them, Xbox
motors):

- high (both motors, 150 ms fading) for the note that ends a combo (the
  next note is number 1: a tap, a slider's tail) and every spinner's end;
- medium (60%, 100 ms) for every other note hit, a slider's head or tail, and
  a miss (its X);
- low (30%, 65 ms) for slider ticks;
- nothing while a spinner turns (a DualSense's triggers push back instead).

Taps, slider heads and misses come from the judge's hook; ticks are the
combo going up without a judged tap; a tail is felt as the ball reaches its
end while held (Ouendan 1 leaves some tails out of its combo, before a
break). Checked headless with `rumble-log` on all three games' first
stages, every note accounted for. The Anthem: high 40 combo ends (13 of
them slider tails) and the finale spinner; medium 202 hits, 28 heads, 15
tails; low 122 ticks. Ouendan 2: high 35 and its spinner; medium 252, 16,
3; low 19. Ouendan 1: high 59; medium 168, 6; low 14 (its first stage has
no spinner). Pressing 10 frames late (`buttons-auto=10`): a medium pulse
for each of the 8 misses.

A face button (or a hit key) also taps SKIP at the lower right in a stage's
opening and ending comics (all three games), and in a song's intro after GO!
(the Agents' arrival, the episode's card, ...) up to its first note, where
the game has a SKIP there (EBA, Ouendan 2). Where it has none, the same
press **fast-forwards** instead (`fast_forward`, button mode or not): the
recomp's turbo runs the game at full speed, frames neither drawn nor paced,
the sound dropped, until 1.5 s before the first note appears (Ouendan 1's
intro) or until the song is over (every game's outro, after the last note:
EBA and Ouendan 1 never let the player skip it, Ouendan 2 does). Checked
headless: Ouendan 1 from song frame 479 to 767, the first note (948) then
played; its outro from 10006 to the results; EBA's outro from 9718 to the
ending comic and results. In a window the screen holds its last picture for
the second or two the fast-forward takes.

The d-pad hits like the face buttons (in songs and comics), so a stream can
alternate between the two sets; keyboard hit keys have bits of their own.

## RetroAchievements (all three games, `src/eba_achievements.cpp`)

**Game > RetroAchievements...**: sign in with a retroachievements.org account
(only RA's token is kept, in `%APPDATA%\eba-recomp\RetroAchievements\login.ini`,
shared by the three games; never the password), then each game's own set
loads, with its list and progress, and unlocks pop up at the lower right.

- **rcheevos** v12.5.0 (`third_party/rcheevos`, a submodule, MIT), its
  rc_client, built as a static library by `cmake/eba.cmake`. The network is
  WinHTTP (Windows' own, on a thread of its own): no DLL to ship.
- **Memory:** RA's DS addresses are the recomp's own buffers: 0 to 0x3FFFFF
  main RAM (0x02000000), 0x1000000 to 0x1003FFF the ARM9's DTCM. Checked once
  a game frame, on the RAM the game left.
- **The game:** RA's hash of the ROM (header, ARM9, ARM7, icon). All three
  stock ROMs are in RA's Nintendo DS hash list: EBA `bbf10cada0dcc72068179c600fe79267`
  (game 9232), Ouendan 2 `899b9c8dd0128097aebb009b49b2a2d5` (17353), Ouendan 1
  `999654092d72821ef56a7c8de260cc64` (1226).
- **Casual only.** RA counts hardcore from emulators it has approved (six
  months public, its compliance list); this one is not.
- **The game as made.** Achievements are paused (not checked; when the
  pause ends they start over from their own start conditions, `rc_client_reset`)
  in Customs, in button mode, with the approach or hit circle assist or
  No-Fail, the game's autoplay, a replay played back, or a test's `--port`
  options. A pop-up says so. The timing offset, the sound's latency, the
  picture's options, skins, effects and rumble change nothing the game judges.
- User agent: `eba-recomp/0.1.0 (Windows NT 10.0) eba/0.1.0` plus rcheevos's.
- `--port ra-log` logs rc_client; `--port ra-spectator` checks achievements and
  sends nothing (tests; a headless run then signs in with the saved token).

## The game's own play (`src/eba_play.cpp`, from the decomp)

The matching decompilation (`eba-decomp`, `docs/DECOMP_FINDINGS.md`) showed
what the judge (`Note::Judge`) reads of the session besides the touch:

- **Autoplay:** session `+0x234` (EBA) / `+0x27C` (Ouendan 2, the same load
  in its judge): 1, 2, 3 judge every note 300, 100, 50 on its own frame; 4 at
  random. `--port autoplay=N`. Checked: EBA's The Anthem, 257 at 300 and
  cleared (spinners too); Ouendan 2's Samurai Blue, 362 at 300 and cleared.
- **Replays:** every judged tap goes into the session's replay `Array` (`+0xA0`:
  data, capacity, count; 8-byte events: frame judged, the note's time, kind,
  result, a new group). A window's run saves each song's to
  `<out>/replays/GAME_TAG_stage_time.rep` as it ends (`--port replay-save=FILE`
  headless); `--port replay=FILE` loads one into the buffer the game makes
  for its own (as the song starts) and sets the replaying field (`+0x138` /
  `+0x15C`). Checked: a run pressing 4 frames late, recorded and played back
  with no input, gives the same 241 judgements. The game's replay mode
  does not drain the life gauge: a recorded fail plays back to a clear.
- **Assists (all three, the Assist menu):** the frames a note shows and the
  hit test's extra radius are .data the game sets for each stage (EBA 75
  frames for The Anthem, 99 for Makes No Difference; Ouendan 2's first stage
  72, Ouendan 1's 90; the radius +3 in all three): the approach is scaled
  from the game's value (+30%, +65%, -20%), the radius widened by 5 or 11 px.
  Off, they are left alone. `--port approach=PCT`, `--port hit-extra=PX`.
- **No-Fail (all three, the Assist menu):** in a song, the life gauge's
  switch (an s32 the song's setup sets to 1; EBA's Training turns it off) is
  held at 0, so no judgement fills or drains it; off mid-song, it is set
  back to 1. **Hide the life bar** (on by default) leaves out its four
  textures meanwhile: the 気/Yes markers, the bar, and the red and yellow
  fills (the fills are the same textures in all three games).
  Customs keep their score with it. Checked headless with no input: The
  Anthem fails (Regroup?) by frame 8,000 without it, plays on at 13,000
  with it; Ouendan 2 fails (応援失敗) by 8,000, plays to the stage's end with
  it; Ouendan 1 fails by 7,000, plays on at 11,500. `--port no-fail`,
  `--port hide-life=off`.

  | | approach | hit radius | gauge switch | its function (EBA's) |
  |---|---|---|---|---|
  | EBA | `0x020B4704` | `0x020B22E0` | `0x020B40C8` | `0x02067AD8` |
  | Ouendan 2 | `0x02092644` | `0x0209003C` | `0x02092200` | `0x020BAAD0` |
  | Ouendan 1 | `0x020891B0` | `0x02086184` | `0x02088A7C` | `0x0202F340` |

  Ouendan 2's were found by matching its code to EBA's: the function before
  each judge sets a note's appear frame from the approach (EBA `0x020681B8`,
  Ouendan 2 `0x020BB294`); the judges load the radius at the same place; the
  gauge function loads the gauge table by judgement, then tests the switch.

## Skins and texture packs (`src/eba_skins.cpp`, ndskit `texpack.cpp`)

The 3D renderer can draw a PNG in place of any DS texture (patch
`ndskit-texpacks.patch`). A texture is named by a hash of its texels (and the
palette entries it uses); a pack is a folder of pictures:

- `<hash>.png`: that texture exactly;
- `s<shape>.png`: its texels under any palette (the shape hash, without it);
- `t<shape>.png`: likewise, multiplied by the colour the texture shows most
  (a glow whose palette cycles keeps cycling).

Any of them may start with a label and two underscores
(`notes_approach_ring_small__sbb1d0471b237cda1.png`), so a redraw keeps
readable names.

Any size: the picture is sampled (mipmapped, smooth) over the texture's
coordinates and wrap. `--port tex-dump=DIR` writes every texture drawn, as
PNG, with `index.txt` (hash, format, size, frame, shape); `--port
tex-pack=DIR[;DIR]` loads packs (the first wins).

**Skins** (Display > Skin) are packs in `skins/<name>/` beside the ROM (a
play folder's) or here; `skins/_hd-rings/` is **HD approach circles**, the
three approach rings redrawn smooth (under a skin's own): the ring textures'
shapes are the same in all three games (hashes `bfc0096910488515`,
`40239154a3f1c691`, `bb1d0471b237cda1`), so the one pack serves EBA, Ouendan
1 and Ouendan 2, with Thin and
Thinner variants in `_hd-rings-thin/` (0.6x as thick) and
`_hd-rings-thinner/` (0.35x). Pictures only: the game's sounds stay.
`tools/make_texture_pack.py` makes both:

    py -3 tools/make_texture_pack.py skin "F:\osu!\Skins\ebo-hd" "skins/Elite Beat Osu! HD"
    py -3 tools/make_texture_pack.py hd-rings skins/_hd-rings
    py -3 tools/make_texture_pack.py hd-rings skins/_hd-rings-thin --width 0.6
    py -3 tools/make_texture_pack.py hd-rings skins/_hd-rings-thinner --width 0.35

From an osu! skin's @2x images it builds EBA's four circle atlases (hitcircle
tinted by the skin's combo colour nearest each atlas's, its overlay, its
numbers 1-15, the reverse arrow), the approach rings, the slider ball and
glow, the judgements, the miss X, the spinner and SPIN!, each fitted to the
DS picture's box (from a dump of all four difficulties, `--dump`). EBA's
textures are the same on every difficulty. `skins/` is not in git (a skin's
art is its author's).

## Effects (all three games, optional, `src/eba_fx.cpp`)

Off by default (Effects menu; `--port fx`, `--port fx-intensity=N`). Nothing
shows before a 30x combo (the first combo burst); a broken combo fades it out
until the next 30x. The combo is the game's own counter (session `+0xE0` EBA,
`+0xF4` Ouendan 2, `+0xBC` Ouendan 1: each Combo's count, 0x2C before its
score; slider ticks counted, as shown). EBA's combo bursts (the Agent
portrait) come at 30, 60, 100 and 150, not 90.

- **Fire** (all three): fire rising behind the squad or the Agents, taller
  as the combo grows (full by 150x), flaring with each hit, a burst on an
  all-300 group (a toggle): the Ouendan games' own fire, heightened. EBA had
  spotlights for a while; they were removed (2026-10-25): new graphics made
  from scratch do not suit the games.
- **Where:** in the 3D frame, a pass after the polygons at a depth just in
  front of the farthest opaque one (the stage's background), so the Agents,
  the notes (both 3D) and the HUD are not lit; and under the 3D where a 2D
  background shows through it (the compose pass). `--port fx-debug` draws it
  in front of everything; `--port combo-log` logs the combo.

## Customs (all three games)

A mode of its own, apart from the campaign: songs charted over story stages,
each with its own scores and HD audio. An orange **CUSTOMS** button sits at
the top of the main menu's lower screen (also Game > Customs...). The screen
lists each custom: its art and an HD preview, its best score per difficulty,
**Play**, and the stage it replaces (that stage's card and its own preview).

- **Play** starts a new run of ndsrun (the same window settings, fullscreen
  too) on the stock ROM with the custom's files in place (`--replace-file
  NAME=PATH`: the files are appended to the image and the FAT points at
  them) and a save copied from `customs/pristine.sav`. An autopilot (turbo,
  behind a loading screen) takes the menus to the card of the stage, sets
  the session's stage (`*0x020CD248 + 8`) and difficulty (`+0`: 0 Breezin'
  .. 3 Hard Rock), and hands over at the stage's comic. Each step checks the
  scene it expects (the scene's vtable); a wrong one stops it and says so.
- **HD audio:** the song's original recording (44.1 or 48 kHz) plays in
  place of the DS's 15 kHz stream (channels 6 and 7 muted), kept on the
  game's song clock: `tools/bake_customs.py` measures the stream's offset
  and the clock's rate per custom. `--port hd=off` plays the DS stream.
- **Scores** go to `customs/scores.ini`, never the campaign's save: the best
  per custom and difficulty, with the grades and whether it was cleared
  (the ending comic follows a clear, Regroup a fail). None is kept for a run
  with autoplay, a replay or a wider hit circle.
- **Autoplay** (a checkbox on the Customs screen): the game's own, to watch
  or hear a custom.
- **Import** (two buttons on the Customs screen): a custom ROM of the game
  (`.nds`): the files that differ from the stock cartridge's (its FNT and
  FAT) go to `customs/<stage>_<title>/files/`, the stage from the story
  charts it changes, the title from the file's name (`Elite Beat Agents
  (FG_SS04_ReadyToGo).nds`: Ready To Go); or a custom's folder someone shared
  (`custom.ini` and `files/`), copied in. An imported custom has no art,
  previews or HD audio until `tools/bake_customs.py` makes them: its title
  stands in. `--port customs-import=PATH` does the same headless. Checked:
  Ready To Go imported from its ROM and played through to a clear.
- **Again, without the reboot:** Play on the same custom from the map after
  its results (any difficulty, autoplay or not) skips the new run: the map
  is centred on the stage just played, so the autopilot taps it (EBA its
  marker, Ouendan 2 its character), sets the stage and difficulty as the
  card shows, and presses OK, about 11 s of game time in turbo. Another
  custom still needs a run of its own (each replaces the same sound data).
- **Back to campaign** (on the Customs screen and in the Game menu) starts a
  run on the campaign's save again.

The five: All Star (over Rock This Town, SS05), Movies (Makes No Difference,
SS02), Life Is a Highway (Highway Star, SS09), Come and Get Your Love (YMCA,
SS10), Girls Just Want to Have Fun (SS11). `tools/extract_customs.py` takes
each custom's files from its built ROM; `tools/bake_customs.py` makes the
placeholder art, the stage cards and both previews, and the HD timing.
`customs/` is not committed (the songs' audio).

Ouendan 2's four, in `customs_oe2/` (`--game OE2` for both tools): Hohoemi no
Bakudan (over Real Life Real Heart, OS02), I Want You (VISTA, OSB02), Madder
(Kibun Jojo, OS03), Takedown (Music Hour, OS05). Its facts (clock, notes,
judge, scenes, session) are in `src/games.cpp`.

Ouendan 1's five, in `customs_oe1/` (`--game OE1`): Danger Zone (over Guts Da
Ze, s03), I Just Can't Wait to Be King (Thrill, s08), I Think I'm OKAY (One
Night Carnival, s11), Home (Over the Distance, s12), Through the Night
(Shanghai Honey, s14), all with HD audio. Ouendan 1's engine is older: its
notes are EBA's objects with the fields from +0x14 on 4 bytes later (the
profile's `shift`), its manager's Array at +0x6C; its judge (`0x0202EE30`,
`--hook 0202EE30`) is `judge(note, now)` as EBA's, with autoplay at session
`+0x1F8` and no replays. Its stage is the session's `+4` (its name built as
`"%c%02d"`: index 0 is s01), and its card shows the map's man tapped
whatever the session says, so the Customs screen shows the replaced stage's
opening comic, and the original preview is its song's start. The bake's
clock fit is now a median-based line (a window matched at the wrong place no
longer pulls it).

## Local ndskit patches

In `patches/`, applied in this order to upstream ndskit (5ee24af); together
they reproduce the submodule's working tree exactly:

- `ndskit-discover.patch`: two discovery fixes (below).
- `ndskit-display-options.patch`: the display options above (runtime
  `display.cpp`, `audio.cpp`, `main.cpp`, `nds.h`, `display.h`). Headless
  runs are byte-identical with and without it.
- `ndskit-input-hook.patch`: one input path for the window and scripts
  (`nds_apply_input`) with a hook for a port's own code; SDL3 gamepads by
  the place of each button (south B, east A, west Y, north X, the d-pad,
  Start, Back, shoulders); `PAD_*` names in input scripts; `--port
  KEY[=VALUE]` options for the port.
- `ndskit-display-live.patch`: screen layouts (`--layout`, F9); every
  display setting changeable while the window runs (`display_change`);
  hooks for a port's UI (its first look at each event, drawing over the
  picture, a margin kept above the screens); `--low-latency` and
  `--audio-latency`. The screens are now always composed into a picture
  at the scale and laid out from it.
- `ndskit-function-hooks.patch`: `python -m ndskit.recomp --hook ADDR`
  recompiles a function as `f_ADDR_orig` with `f_ADDR` in its place,
  calling the hook a port sets with `arm_set_hook` (ArmModule::hooks). One
  of ndskit's own known gaps ("no hooks yet").
- `ndskit-filters.patch`: `--filter` and the out pass's smoothing filters;
  `display_settings()` reports a change not yet applied (the options bar
  saved the old value).
- `ndskit-effects.patch`: a port's light behind the 3D (`DisplayFx`,
  `display_set_fx`): its GLSL shared by the compose pass and a pass in
  the 3D renderer after the polygons.
- `ndskit-customs-runtime.patch`: what Customs needs of the runtime:
  `--replace-file NAME=PATH`, muting sound channels (`sound_set_muted`) and
  reading one (`sound_channel`), turbo (`display_set_turbo`: frames neither
  drawn nor paced, the sound dropped), where a screen is in the window
  (`display_screen_rect`), the sound's playback ratio and queue
  (`audio_ratio`, `audio_queued_seconds`), and a port's input hook running
  headless without a script. Headless runs are byte-identical before and
  after it (EBA, 3,000 VBlanks through the menus: RAM, VRAM, save and
  screenshots).
- `ndskit-texpacks.patch`: texture packs and dumps (`runtime/texpack.cpp`,
  the fragment shader's `u_hdtex`), hidden textures
  (`display_set_texture_hidden`), `display_set_texture_packs`; a file may
  carry a label (`label__<hash>.png`).
- `ndskit-vsync.patch`: the vertical sync as a setting (`DisplaySettings::vsync`,
  `--vsync auto|on|off`): auto as before, or always, or never.
- `ndskit-config-hook.patch`: a port's look at the options once they are
  read, before the cartridge boots (`nds_set_config_hook`: where the save
  and the run's files go), and a window title of its own (`NdsConfig::title`).

The discovery fixes, both to report upstream (drafts in `upstream/`):
1. `Program.run()` gave up after 4 refuse passes and wiped everything, so
   EBA's ARM9 discovered 0 functions. It now allows 40 passes and never wipes
   after the last one.
2. The rule of opposite conditions ("`bxpl` then `popmi`: nothing runs
   after") was applied across calls. CodeWarrior's soft-float compares
   return their result in the flags, so `bne ...; bl _dcmpeq; beq ...` lost
   the fall-through after the `beq`, emitted as an `arm_jump` to a
   non-entry: a fatal "returned to X" if it ran. A call now ends the stretch
   of same flags. Before and after on all 14 programs of the three games
   (`tools/compare_funcs.py`): only OE2's two affected functions grew
   (0x020234B8, overlay 6's 0x020B50CC); none lost, split or shrunk; 0
   conflicts.

Re-apply after updating the submodule:

    cd ndskit
    git apply ../patches/ndskit-discover.patch ../patches/ndskit-display-options.patch ../patches/ndskit-input-hook.patch ../patches/ndskit-display-live.patch ../patches/ndskit-function-hooks.patch ../patches/ndskit-customs-runtime.patch ../patches/ndskit-filters.patch ../patches/ndskit-effects.patch ../patches/ndskit-texpacks.patch ../patches/ndskit-vsync.patch ../patches/ndskit-config-hook.patch

## The launcher (`launcher/`, `tools/build_launcher.sh`)

`eba-recomp.exe`: the three games in one window. Each shows whether its
executable (`EBA.exe`, `OE1.exe`, `OE2.exe` beside it) is in the package,
the player's dump (Choose dump..., or drop a `.nds` on the window: the
launcher tells which game it is), Play, and Save folder. A dump is checked
by RetroAchievements' hash of the ROM (header, ARM9, ARM7, icon: the same for
every good dump of a game, trimmed or not): EBA's USA cartridge (AOSE),
Ouendan 1's (AOSJ) and Ouendan 2's (AO2J); anything else is refused with
the reason. The paths are kept in `launcher.ini` in the launcher's data
folder (`%APPDATA%\eba-recomp\Launcher\`, or `userdata\Launcher\` when
portable). Play starts the game with `--window`, its output in
`<its folder>\<game>.log`; a game that stops with an error says so, with
Open log. It draws with SDL's 2D renderer, not OpenGL 4.5, so it opens on
any PC. Test options: `--rom FILE`, `--play ID`, `--shot FILE.bmp`.

Checked (a portable folder: the launcher, a release `EBA.exe`, the DLLs):
first run asks for each dump; EBA's and Ouendan 2's dumps are recognised
and kept, any other file is refused; Play starts EBA in its window, titled
"Elite Beat Agents (eba-recomp 1.0.0)", its save and log in
`userdata\EBA\`. Release game executables are Windows programs (no console
window); the launcher keeps their output.

## Release builds and where the player's files go

`EBA_RELEASE=ON tools/build.sh` (or `cmake -DEBA_RELEASE=ON`) builds the
release: Customs, RetroAchievements and saved replays stay in the code with
their menus and calls off (`kRelease`, `src/eba.h`), the window is titled with
the game and the version, and a window's run writes no memory dumps. The
version is `-DEBA_VERSION` (1.0.0): the About box, the title, RetroAchievements'
user agent, and the executable's version resource (`cmake/version.rc.in`).
Both apply to the port's sources only: switching rebuilds those, not the
recompiled game. Unset, `build.sh` builds the development build.

A window's run given no `--save` (`src/eba_paths.cpp`) keeps its save, its
settings, screenshots and log in `%APPDATA%\eba-recomp\<game>\` (EBA, OE2,
OE1); with a file `portable.txt` beside the executable, in
`userdata\<game>\` beside it. **Game > Open the save folder** opens it. A run
given `--save` (the play folders, tests) keeps the paths it was given, and
headless runs are unchanged. Skins are found beside the executable first,
then beside the ROM.

## Toolchain

MSYS2 UCRT64 (`C:\msys64`): `mingw-w64-ucrt-x86_64-{clang,sdl3,cmake,ninja}`.
Python 3 for the ndskit layers. ROMs go in `roms/` (EBA.nds, OE1.nds,
OE2.nds; not committed). Each game gets its own tree: EBA in
`build/recomp`, OE1 in `build/oe1/recomp`, OE2 in `build/oe2/recomp`.

## Commands

    python tools/phase0_check.py roms/EBA.nds --game eba        # discovery + hand-found addresses
    python -m ndskit.recomp roms/EBA.nds --out build/recomp --optest --hook 020681D8
    python -m ndskit.recomp.selftest roms/EBA.nds --cpu 9 --auto --out build/recomp/selftest_eba9.txt
    C:\msys64\usr\bin\bash.exe -lc "/f/dxvk_remix/Claude/eba-recomp/tools/build.sh"
    build/recomp/b/selftest build/recomp/selftest/*.txt build/recomp/selftest_eba9.txt

OE1 and OE2 are the same with their ROM, `--out build/oe1/recomp` or
`build/oe2/recomp` (no `--optest` needed) and `RECOMP_DIR=build/oe1/recomp`
(or oe2) before `build.sh`. OE2's overlays get their own vectors:
`--cpu 9 --overlay N --auto` for N in 0-6.

`tools/run_headless.ps1` runs a build headless from a fresh copy of the real
save. These scripts go from boot into a stage on the difficulty each save
starts on (EBA's: Hard Rock); taps are lower-screen coordinates:

    tools\run_headless.ps1 -Game EBA -VBlanks 9000 -Wav -Shots 5600,7500 `
      -Script "950:tap:128:150,1300:tap:128:66,2450:tap:236:178,3650:tap:126:103,4300:tap:236:178,5000:tap:238:183"
    tools\run_headless.ps1 -Game OE1 -VBlanks 7500 -Wav -Shots 7000 `
      -Script "700:tap:128:150,1350:tap:150:48,2050:tap:236:183,3050:tap:160:76,3450:tap:236:183"
    tools\run_headless.ps1 -Game OE2 -VBlanks 12600 -Wav -Shots 10200,12600 `
      -Script "1850:tap:128:150,2650:tap:65:55,3350:tap:238:183,4350:tap:238:178,4750:tap:238:183"

Map notes: on OE1's town map the face by the red arrow goes to the globe
view; the stage is the man in green. On OE2's 3D map, taps on the people did
nothing headless; the book icon at the bottom right moves to a stage and
opens its card.

In a window (put `C:\msys64\ucrt64\bin` on PATH; mouse = stylus, Esc quits):

    build/recomp/b/ndsrun roms/EBA.nds --save build/play/EBA.sav --window --scale 2

## Next

EBA only for now:

1. Playtest the options bar and calibrate the offset (Timing > Calibrate).
2. HD note layer: the notes drawn natively between frames, for 120/240
   (the same note reading as button mode).
3. OE1 and OE2: re-emit and rebuild on the current ndskit (their trees
   predate the input, display and hook changes).
4. melonDS oracle: savestate at the same VBlank, `python -m ndskit.melonds STATE.ml1 --compare build/run`.
5. Post the discover.py bug reports (drafts in `upstream/`).
6. 3D edge marking (ndskit runtime), seen on the EBA and OE2 stage maps.
