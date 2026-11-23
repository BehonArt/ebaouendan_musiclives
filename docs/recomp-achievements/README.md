# Recomp Achievements (RecompLauncher)

RecompLauncher's own achievements for the three games: a set of its own per game, made for
Recomp Achievements (RetroAchievements' set is not carried over: RecompLauncher's rule). Each
set is worth **1000 points**. Any input counts: the mouse or a touch screen as the stylus, Tap
mode, a gamepad, the assists (No-Fail included). The full lists, with titles, descriptions,
points and event names: [LISTS.md](LISTS.md).

| Game | RecompLauncher id | Achievements |
|---|---|---|
| Elite Beat Agents | `elitebeatagents` | 63 |
| Osu! Tatakae! Ouendan | `ouendan1` | 59 |
| Moero! Nekketsu Rhythm Damashii Osu! Tatakae! Ouendan 2 | `ouendan2` | 64 |

## What each set has

| Kind | Each | Points |
|---|---|---|
| Clear a story song on any difficulty (16 in EBA and Ouendan 2, 15 in Ouendan 1) | named after the song | 5 (Ouendan 1: 10) |
| A "New songs" wave on the map, 6 per difficulty | easy / normal / hard / insane | 5 / 5 / 10 / 25 |
| Roll the credits on a difficulty (normal and hard unlock the next one and its leads) | *win* | 25 / 50 / 50 / 50 (Ouendans: 25 / 25 / 50 / 50) |
| Each overall rank (EBA, Ouendan 1: 11; Ouendan 2: 12) | | 10 for the first (EBA, Ouendan 2: 4; Ouendan 1: 1), then 25; the top rank 50 |
| Unlock a bonus song (EBA, Ouendan 2: 3 each), the game's own unlock animation | | 25 |
| S ranks: the first, 5, 10, 20, every song on every difficulty (bonus songs included) | | 10, 25 (Ouendan 1: 10), 25, 50, 50 |

A game is **Beaten** as soon as its finale (Jumpin' Jack Flash, Ready Steady Go, Sekai wa Sore
o Ai to Yobundaze) is cleared and the credits roll, on any difficulty: the four credits
achievements are *win* and none is *progression*. **Mastered** once every achievement is unlocked.
The bonus songs' first clears are fired by the game (`P_clear_STAGE`) but are not in the lists:
unlocking them is the moment.

Titles other than the song clears are the games' own references: the agents, Commander Kahn and
the Divas; the Ouendan, the comet and the dying sun; and each difficulty's leads, easy to insane.
EBA: Agent Spin, Agent J, Agent Chieftain, the Elite Beat Divas (Starr, Missy and Foxx).
Ouendan 1 and Ouendan 2's red Yuuhi Town Ouendan: Hajime, Ryuta, Kai, the Cheer Girls (Sayaka, Aoi
and Anna). Ouendan 2's blue Asahi Town Ouendan: Shinta, Hayato, Kaoru, the Cheer Maidens (Rin,
Honoka and Reika).

## Editing the lists

Titles, descriptions and points are in `tools/make_rla_lists.py`; run it and it writes the three
JSON lists here and LISTS.md, and checks that every game comes to exactly 1000 points in 5s, 10s,
25s and 50s, with no title used twice. Each difficulty's leads (`leads`, `unlocks`) go into the
credits' descriptions. **Event names never change** once players have unlocks: they are
the game's (`src/eba_rla.cpp`).

The lists are hosted by RecompLauncher (the developer hub, recompstore.com/hub, PC recomps,
Achievements): nothing in the game needs to change to rename or re-point an achievement.

## The events

The game fires them (`src/eba_rla.cpp`), with its prefix `eba`, `oe1` or `oe2`:

| Event | When |
|---|---|
| `P_clear_STAGE` | the stage (its chart's stem, lower case: `ss01`, `os15b`, `s07`) cleared on any difficulty |
| `P_tier_D_N` | the map's tier N reached on D (`e n h x`), the game's own table: 1 to 6 are the "New songs" waves; 7 is EBA and Ouendan 2's finale part 2 opening, and Ouendan 1's whole story cleared (not in the lists) |
| `P_credits_D` | the finale (EBA's SS15B, Ouendan 2's OS15B, Ouendan 1's s15) cleared on D |
| `P_rank_N` | the overall rank N: every stage's best score on every difficulty summed, against the game's table |
| `P_bonus_STAGE` | a bonus song unlocked (its state no longer 0 on any difficulty) |
| `P_s_N`, `P_s_all` | N S ranks (1, 5, 10, 20), each song on each difficulty once; an S on all of them |

**When they fire.** What the progress meets is taken as each song starts; after the song (the
results, the map), each event it meets that it did not is fired, once. So loading a save fires
nothing, a player with an old save earns what they do from then on (an S on a song already
S-ranked earns nothing new), and a song played with **autoplay or a replay fires nothing** (the
next song starts from what it brought). **Customs** are a mode of their own: their runs never
connect. Nothing is sent as "casual": RecompLauncher judges Ranked or Casual itself.

## Where the progress is (the save, in RAM)

| | EBA | Ouendan 2 | Ouendan 1 |
|---|---|---|---|
| the progress | `*0x020CD244` | `*0x020A7A84` | `*0x02090160` (0x0208EF70) |
| records from +0x10 | 0x1C a difficulty, 5 a stage (0x8C) | 0x28, 4 a stage (0xA0) | 8, 5 a stage (0x28) |
| a record | +0 best rank (0 S .. 4 D, INT_MAX not cleared), +4 best score, +0x1A state | +0 three best scores (-1 none), +0x1D state, +0x1F their ranks (0xFF none) | +0 best rank, +4 best score |
| tiers by the first stage not cleared | 0x020B1F28 | 0x0208FB4C | 0x02089774 |
| overall rank scores | 0x020B1F9C (11) | 0x0208FBC0 (12) | 0x020862F8 (11) |

EBA's are from the decompilation (`UnkStruct_020548e8`, `func_02054ab0`, `func_02054d3c`); the
Ouendans' were matched to them in RAM dumps of real saves (2026-11-22). The stage's state is 0
locked, 1 or 2 open, 3 cleared.

## Testing

Started without RecompLauncher, the game still logs each event (`eba rla: event ...` in its log).

```
ndsrun ROM --save COPY.sav --port rla-print=2400 --vblanks 2401   (the events a save meets, in the log)
python tools/rla_fake_launcher.py OUT.jsonl "eba_clear_ss01,!eba_s_1" -- ndsrun ROM ...
```

`rla_fake_launcher.py` opens the pipe as RecompLauncher does, runs the game with `RL_SESSION`
set, and checks the events it sends (`!NAME`: must not; `offline`: must not connect). Headless,
`--input` taps through the menus to the first song and `--port buttons-auto` plays it. Checked
2026-11-22, each on a fresh save: EBA, Ouendan 1 and 2's first song send `*_clear_*`,
`*_tier_n_1` and `*_s_1` once; with `--port autoplay=1` nothing; a Customs run never connects.
On real saves (a 100% EBA and Ouendan 1, a nearly full Ouendan 2, a half-played EBA) the events
met match the saves' clears, S ranks, tiers (the game's own tier byte) and rank.

## Open with RecompLauncher

- **Three tiles from one release:** `recomplauncher.json`'s `games` (RecompLauncher 0.90 Beta 36
  or newer), each with its own id, saves and achievements; the launcher already starts a game
  directly with `--play EBA|OE1|OE2`:
  `"games": [{"id": "elitebeatagents", "name": "Elite Beat Agents", "args": ["--play", "EBA"]}, ...]`
- **The console:** the lists say `"console": "nds"`, which is not in the spec's list yet.
- **Versions:** the lists say `>=1.3.0`, the release that brings the events.
