# Recomp Achievements: the three lists

Made by `tools/make_rla_lists.py` (edit the titles, descriptions and points there, then run it again).
The event is what the game fires (`src/eba_rla.cpp`); RecompLauncher's list maps it to the achievement.
A game is Beaten when the credits roll on any difficulty (the *win* ones), and Mastered when every achievement is unlocked.

## Elite Beat Agents (`elitebeatagents`): 63 achievements, 1000 points

| id | Title | Description | Points | Type | Event |
|---|---|---|---|---|---|
| 1 | Walkie Talkie Man | Clear Walkie Talkie Man (Steriogram) on any difficulty. | 5 |  | `eba_clear_ss01` |
| 2 | Makes No Difference | Clear Makes No Difference (Sum 41) on any difficulty. | 5 |  | `eba_clear_ss02` |
| 3 | Sk8er Boi | Clear Sk8er Boi (Avril Lavigne) on any difficulty. | 5 |  | `eba_clear_ss06` |
| 4 | I Was Born to Love You | Clear I Was Born to Love You (Freddie Mercury) on any difficulty. | 5 |  | `eba_clear_ss04` |
| 5 | Rock This Town | Clear Rock This Town (Stray Cats) on any difficulty. | 5 |  | `eba_clear_ss05` |
| 6 | Highway Star | Clear Highway Star (Deep Purple) on any difficulty. | 5 |  | `eba_clear_ss09` |
| 7 | YMCA | Clear YMCA (Village People) on any difficulty. | 5 |  | `eba_clear_ss10` |
| 8 | September | Clear September (Earth, Wind & Fire) on any difficulty. | 5 |  | `eba_clear_ss03` |
| 9 | Canned Heat | Clear Canned Heat (Jamiroquai) on any difficulty. | 5 |  | `eba_clear_ss08` |
| 10 | Material Girl | Clear Material Girl (Madonna) on any difficulty. | 5 |  | `eba_clear_ss11` |
| 11 | La La | Clear La La (Ashlee Simpson) on any difficulty. | 5 |  | `eba_clear_ss13` |
| 12 | You're the Inspiration | Clear You're the Inspiration (Chicago) on any difficulty. | 5 |  | `eba_clear_ss12` |
| 13 | Let's Dance | Clear Let's Dance (David Bowie) on any difficulty. | 5 |  | `eba_clear_ssb03` |
| 14 | The Anthem | Clear The Anthem (Good Charlotte) on any difficulty. | 5 |  | `eba_clear_ss07` |
| 15 | Without a Fight | Clear Without a Fight (Hoobastank) on any difficulty. | 5 |  | `eba_clear_ss15a` |
| 16 | Jumpin' Jack Flash | Clear Jumpin' Jack Flash (The Rolling Stones) on any difficulty. | 5 |  | `eba_clear_ss15b` |
| 101 | Fresh Recruit | Open new songs on the map on Breezin' (wave 1 of 6). | 5 |  | `eba_tier_e_1` |
| 102 | Light Breeze | Open new songs on the map on Breezin' (wave 2 of 6). | 5 |  | `eba_tier_e_2` |
| 103 | Spin Cycle | Open new songs on the map on Breezin' (wave 3 of 6). | 5 |  | `eba_tier_e_3` |
| 104 | Tailwind | Open new songs on the map on Breezin' (wave 4 of 6). | 5 |  | `eba_tier_e_4` |
| 105 | Gale Force | Open new songs on the map on Breezin' (wave 5 of 6). | 5 |  | `eba_tier_e_5` |
| 106 | Eye of the Storm | Open new songs on the map on Breezin' (wave 6 of 6). | 5 |  | `eba_tier_e_6` |
| 111 | Cruise Control | Open new songs on the map on Cruisin' (wave 1 of 6). | 5 |  | `eba_tier_n_1` |
| 112 | Agent J on the Case | Open new songs on the map on Cruisin' (wave 2 of 6). | 5 |  | `eba_tier_n_2` |
| 113 | Open Road | Open new songs on the map on Cruisin' (wave 3 of 6). | 5 |  | `eba_tier_n_3` |
| 114 | Need for Speed | Open new songs on the map on Cruisin' (wave 4 of 6). | 5 |  | `eba_tier_n_4` |
| 115 | Overdrive | Open new songs on the map on Cruisin' (wave 5 of 6). | 5 |  | `eba_tier_n_5` |
| 116 | Final Approach | Open new songs on the map on Cruisin' (wave 6 of 6). | 5 |  | `eba_tier_n_6` |
| 121 | Breaking a Sweat | Open new songs on the map on Sweatin' (wave 1 of 6). | 10 |  | `eba_tier_h_1` |
| 122 | Chieftain's Orders | Open new songs on the map on Sweatin' (wave 2 of 6). | 10 |  | `eba_tier_h_2` |
| 123 | Turning Up the Heat | Open new songs on the map on Sweatin' (wave 3 of 6). | 10 |  | `eba_tier_h_3` |
| 124 | Hot Streak | Open new songs on the map on Sweatin' (wave 4 of 6). | 10 |  | `eba_tier_h_4` |
| 125 | Feel the Burn | Open new songs on the map on Sweatin' (wave 5 of 6). | 10 |  | `eba_tier_h_5` |
| 126 | Boiling Point | Open new songs on the map on Sweatin' (wave 6 of 6). | 10 |  | `eba_tier_h_6` |
| 131 | Enter the Divas | Open new songs on the map on Hard Rock (wave 1 of 6). | 25 |  | `eba_tier_x_1` |
| 132 | Starr Power | Open new songs on the map on Hard Rock (wave 2 of 6). | 25 |  | `eba_tier_x_2` |
| 133 | Foxx Trot | Open new songs on the map on Hard Rock (wave 3 of 6). | 25 |  | `eba_tier_x_3` |
| 134 | Missy Impossible | Open new songs on the map on Hard Rock (wave 4 of 6). | 25 |  | `eba_tier_x_4` |
| 135 | Rock Steady | Open new songs on the map on Hard Rock (wave 5 of 6). | 25 |  | `eba_tier_x_5` |
| 136 | Showstoppers | Open new songs on the map on Hard Rock (wave 6 of 6). | 25 |  | `eba_tier_x_6` |
| 201 | Gone with the Wind | Clear Jumpin' Jack Flash on Breezin' with Agent Spin and roll the credits. | 25 | win | `eba_credits_e` |
| 202 | Calling Agent Chieftain | Clear Jumpin' Jack Flash on Cruisin' with Agent J and roll the credits, unlocking Sweatin', led by Agent Chieftain. | 50 | win | `eba_credits_n` |
| 203 | Cue the Divas | Clear Jumpin' Jack Flash on Sweatin' with Agent Chieftain and roll the credits, unlocking Hard Rock, led by the Elite Beat Divas: Starr, Missy and Foxx. | 50 | win | `eba_credits_h` |
| 204 | Commander Kahn Approves | Clear Jumpin' Jack Flash on Hard Rock with the Elite Beat Divas and roll the credits. | 50 | win | `eba_credits_x` |
| 301 | Trainee | Reach overall rank 1 of 11: 400,000 points of best scores, every song and difficulty added up. | 10 |  | `eba_rank_1` |
| 302 | Field Agent | Reach overall rank 2 of 11: 1,500,000 points of best scores, every song and difficulty added up. | 10 |  | `eba_rank_2` |
| 303 | Special Agent | Reach overall rank 3 of 11: 3,000,000 points of best scores, every song and difficulty added up. | 10 |  | `eba_rank_3` |
| 304 | Secret Agent Man | Reach overall rank 4 of 11: 6,000,000 points of best scores, every song and difficulty added up. | 10 |  | `eba_rank_4` |
| 305 | Licensed to Thrill | Reach overall rank 5 of 11: 10,000,000 points of best scores, every song and difficulty added up. | 25 |  | `eba_rank_5` |
| 306 | Undercover Groove | Reach overall rank 6 of 11: 15,000,000 points of best scores, every song and difficulty added up. | 25 |  | `eba_rank_6` |
| 307 | Classified | Reach overall rank 7 of 11: 20,000,000 points of best scores, every song and difficulty added up. | 25 |  | `eba_rank_7` |
| 308 | Top Secret | Reach overall rank 8 of 11: 25,000,000 points of best scores, every song and difficulty added up. | 25 |  | `eba_rank_8` |
| 309 | Rhombulan Repeller | Reach overall rank 9 of 11: 35,000,000 points of best scores, every song and difficulty added up. | 25 |  | `eba_rank_9` |
| 310 | Kahn's Right Hand | Reach overall rank 10 of 11: 50,000,000 points of best scores, every song and difficulty added up. | 25 |  | `eba_rank_10` |
| 311 | Truly Elite | Reach overall rank 11 of 11, the top rank: 80,000,000 points of best scores, every song and difficulty added up. | 50 |  | `eba_rank_11` |
| 401 | Auto-Tuned In | Unlock the bonus song Believe (Cher). | 25 |  | `eba_bonus_ssb02` |
| 402 | Learn Your ABCs | Unlock the bonus song ABC (The Jackson 5). | 25 |  | `eba_bonus_ssb01` |
| 403 | Last Agent Standing | Unlock the bonus song Survivor (Destiny's Child). | 25 |  | `eba_bonus_ss14` |
| 501 | Sharp-Dressed Agent | Get your first S rank. | 10 |  | `eba_s_1` |
| 502 | Smooth Operator | Get S ranks on 5 songs (each song on each difficulty counts once). | 25 |  | `eba_s_5` |
| 503 | Sunglasses at Night | Get S ranks on 10 songs (each song on each difficulty counts once). | 25 |  | `eba_s_10` |
| 504 | S-Class Agent | Get S ranks on 20 songs (each song on each difficulty counts once). | 50 |  | `eba_s_20` |
| 505 | Elite of the Elite | Get an S rank on every song on every difficulty, bonus songs included (76). | 50 |  | `eba_s_all` |

## Moero! Nekketsu Rhythm Damashii Osu! Tatakae! Ouendan 2 (`ouendan2`): 64 achievements, 1000 points

| id | Title | Description | Points | Type | Event |
|---|---|---|---|---|---|
| 1 | Zenryoku Shounen | Clear Zenryoku Shounen (Sukima Switch) on any difficulty. | 5 |  | `oe2_clear_os01` |
| 2 | Real Life Real Heart | Clear Real Life Real Heart (Kaela Kimura) on any difficulty. | 5 |  | `oe2_clear_os02` |
| 3 | Okuru Kotoba | Clear Okuru Kotoba (FLOW) on any difficulty. | 5 |  | `oe2_clear_os04` |
| 4 | Pop Star | Clear Pop Star (Ken Hirai) on any difficulty. | 5 |  | `oe2_clear_os06` |
| 5 | Go My Way | Clear Go My Way (Hitomi Yaida) on any difficulty. | 5 |  | `oe2_clear_os08` |
| 6 | Julia ni Heartbreak | Clear Julia ni Heartbreak (The Checkers) on any difficulty. | 5 |  | `oe2_clear_os10` |
| 7 | Going Under Ground | Clear Going Under Ground (VISTA) on any difficulty. | 5 |  | `oe2_clear_osb02` |
| 8 | Shounen Heart | Clear Shounen Heart (Home Made Kazoku) on any difficulty. | 5 |  | `oe2_clear_os11` |
| 9 | Kibun Joujou | Clear Kibun Joujou (mihimaru GT) on any difficulty. | 5 |  | `oe2_clear_os03` |
| 10 | Bambina | Clear Bambina (Tomoyasu Hotei) on any difficulty. | 5 |  | `oe2_clear_os07` |
| 11 | Bang! Bang! Vacances! | Clear Bang! Bang! Vacances! (SMAP) on any difficulty. | 5 |  | `oe2_clear_os14` |
| 12 | Believe | Clear Believe (AI) on any difficulty. | 5 |  | `oe2_clear_os12` |
| 13 | Zoku | Clear Zoku (Kishidan) on any difficulty. | 5 |  | `oe2_clear_os13` |
| 14 | Music Hour | Clear Music Hour (Porno Graffitti) on any difficulty. | 5 |  | `oe2_clear_os05` |
| 15 | Countdown | Clear Countdown (HYDE) on any difficulty. | 5 |  | `oe2_clear_os15a` |
| 16 | Sekai wa Sore o Ai to Yobundaze | Clear Sekai wa Sore o Ai to Yobundaze (Sambomaster) on any difficulty. | 5 |  | `oe2_clear_os15b` |
| 101 | Hajime, Round Two | Open new songs on the map on Kigaru (wave 1 of 6). | 5 |  | `oe2_tier_e_1` |
| 102 | Enter Shinta | Open new songs on the map on Kigaru (wave 2 of 6). | 5 |  | `oe2_tier_e_2` |
| 103 | Rookie Rivalry | Open new songs on the map on Kigaru (wave 3 of 6). | 5 |  | `oe2_tier_e_3` |
| 104 | Seeing Red | Open new songs on the map on Kigaru (wave 4 of 6). | 5 |  | `oe2_tier_e_4` |
| 105 | Feeling Blue | Open new songs on the map on Kigaru (wave 5 of 6). | 5 |  | `oe2_tier_e_5` |
| 106 | The Countdown Begins | Open new songs on the map on Kigaru (wave 6 of 6). | 5 |  | `oe2_tier_e_6` |
| 111 | Ryuta Returns | Open new songs on the map on Kakan (wave 1 of 6). | 5 |  | `oe2_tier_n_1` |
| 112 | Hayato Takes the Lead | Open new songs on the map on Kakan (wave 2 of 6). | 5 |  | `oe2_tier_n_2` |
| 113 | Red vs. Blue | Open new songs on the map on Kakan (wave 3 of 6). | 5 |  | `oe2_tier_n_3` |
| 114 | Hot Blood, Cool Heads | Open new songs on the map on Kakan (wave 4 of 6). | 5 |  | `oe2_tier_n_4` |
| 115 | Two Flags, One Sky | Open new songs on the map on Kakan (wave 5 of 6). | 5 |  | `oe2_tier_n_5` |
| 116 | T-Minus Cheer | Open new songs on the map on Kakan (wave 6 of 6). | 5 |  | `oe2_tier_n_6` |
| 121 | Kai Meets Kaoru | Open new songs on the map on Gekiretsu (wave 1 of 6). | 10 |  | `oe2_tier_h_1` |
| 122 | Battle of the Presidents | Open new songs on the map on Gekiretsu (wave 2 of 6). | 10 |  | `oe2_tier_h_2` |
| 123 | Clash of Squads | Open new songs on the map on Gekiretsu (wave 3 of 6). | 10 |  | `oe2_tier_h_3` |
| 124 | Blue Blazes | Open new songs on the map on Gekiretsu (wave 4 of 6). | 10 |  | `oe2_tier_h_4` |
| 125 | Red Inferno | Open new songs on the map on Gekiretsu (wave 5 of 6). | 10 |  | `oe2_tier_h_5` |
| 126 | Showdown Approaching | Open new songs on the map on Gekiretsu (wave 6 of 6). | 10 |  | `oe2_tier_h_6` |
| 131 | Girls vs. Maidens | Open new songs on the map on Karei (wave 1 of 6). | 25 |  | `oe2_tier_x_1` |
| 132 | Rin's Resolve | Open new songs on the map on Karei (wave 2 of 6). | 25 |  | `oe2_tier_x_2` |
| 133 | Honoka's Harmony | Open new songs on the map on Karei (wave 3 of 6). | 25 |  | `oe2_tier_x_3` |
| 134 | Reika's Rally | Open new songs on the map on Karei (wave 4 of 6). | 25 |  | `oe2_tier_x_4` |
| 135 | Sayaka Answers Back | Open new songs on the map on Karei (wave 5 of 6). | 25 |  | `oe2_tier_x_5` |
| 136 | Grace Under Pressure | Open new songs on the map on Karei (wave 6 of 6). | 25 |  | `oe2_tier_x_6` |
| 201 | The World Calls It Love | Clear Sekai wa Sore o Ai to Yobundaze on Kigaru with Hajime and Shinta and roll the credits. | 25 | win | `oe2_credits_e` |
| 202 | Kai and Kaoru, You're Up! | Clear Sekai wa Sore o Ai to Yobundaze on Kakan with Ryuta and Hayato and roll the credits, unlocking Gekiretsu, led by Kai and Kaoru. | 25 | win | `oe2_credits_n` |
| 203 | Enter the Cheer Squads | Clear Sekai wa Sore o Ai to Yobundaze on Gekiretsu with Kai and Kaoru and roll the credits, unlocking Karei, led by the Cheer Girls (Sayaka, Aoi and Anna) and the Cheer Maidens (Rin, Honoka and Reika). | 50 | win | `oe2_credits_h` |
| 204 | Here Comes the Sun (Again) | Clear Sekai wa Sore o Ai to Yobundaze on Karei with the Cheer Girls and the Cheer Maidens and roll the credits. | 50 | win | `oe2_credits_x` |
| 301 | Rookie Cheerer | Reach overall rank 1 of 12: 400,000 points of best scores, every song and difficulty added up. | 10 |  | `oe2_rank_1` |
| 302 | Red or Blue? | Reach overall rank 2 of 12: 1,500,000 points of best scores, every song and difficulty added up. | 10 |  | `oe2_rank_2` |
| 303 | Flag Bearer | Reach overall rank 3 of 12: 3,000,000 points of best scores, every song and difficulty added up. | 10 |  | `oe2_rank_3` |
| 304 | Taiko Thunder | Reach overall rank 4 of 12: 6,000,000 points of best scores, every song and difficulty added up. | 10 |  | `oe2_rank_4` |
| 305 | Megaphone Maestro | Reach overall rank 5 of 12: 10,000,000 points of best scores, every song and difficulty added up. | 25 |  | `oe2_rank_5` |
| 306 | Encouraging Nobility | Reach overall rank 6 of 12: 15,000,000 points of best scores, every song and difficulty added up. | 25 |  | `oe2_rank_6` |
| 307 | Vice Captain | Reach overall rank 7 of 12: 20,000,000 points of best scores, every song and difficulty added up. | 25 |  | `oe2_rank_7` |
| 308 | Squad Captain | Reach overall rank 8 of 12: 25,000,000 points of best scores, every song and difficulty added up. | 25 |  | `oe2_rank_8` |
| 309 | Respected by Rivals | Reach overall rank 9 of 12: 35,000,000 points of best scores, every song and difficulty added up. | 25 |  | `oe2_rank_9` |
| 310 | Pride of Both Squads | Reach overall rank 10 of 12: 50,000,000 points of best scores, every song and difficulty added up. | 25 |  | `oe2_rank_10` |
| 311 | Living Legend | Reach overall rank 11 of 12: 70,000,000 points of best scores, every song and difficulty added up. | 25 |  | `oe2_rank_11` |
| 312 | Heart of the Ouendan | Reach overall rank 12 of 12, the top rank: 100,000,000 points of best scores, every song and difficulty added up. | 50 |  | `oe2_rank_12` |
| 401 | Magic Monkey My Hero | Unlock the bonus song Monkey Magic (Orange Range). | 25 |  | `oe2_bonus_osb03` |
| 402 | ...As It Speeds You From My Life | Unlock the bonus song Glamorous Sky (NANA starring Mika Nakashima). | 25 |  | `oe2_bonus_osb01` |
| 403 | He Didn't Ghost You | Unlock the bonus song Samurai Blue (ZZ). | 25 |  | `oe2_bonus_os09` |
| 501 | Stamp of Approval | Get your first S rank. | 10 |  | `oe2_s_1` |
| 502 | Senpai Is Impressed | Get S ranks on 5 songs (each song on each difficulty counts once). | 25 |  | `oe2_s_5` |
| 503 | Synchronized Shouting | Get S ranks on 10 songs (each song on each difficulty counts once). | 25 |  | `oe2_s_10` |
| 504 | Sweat, Spirit and S Ranks | Get S ranks on 20 songs (each song on each difficulty counts once). | 50 |  | `oe2_s_20` |
| 505 | Undying Fighting Spirit | Get an S rank on every song on every difficulty, bonus songs included (76). | 50 |  | `oe2_s_all` |

## Osu! Tatakae! Ouendan (`ouendan1`): 59 achievements, 1000 points

| id | Title | Description | Points | Type | Event |
|---|---|---|---|---|---|
| 1 | Loop & Loop | Clear Loop & Loop (Asian Kung-Fu Generation) on any difficulty. | 10 |  | `oe1_clear_s01` |
| 2 | Koi no Dance Site | Clear Koi no Dance Site (Morning Musume) on any difficulty. | 10 |  | `oe1_clear_s02` |
| 3 | Guts Da Ze!! | Clear Guts Da Ze!! (Ulfuls) on any difficulty. | 10 |  | `oe1_clear_s03` |
| 4 | Melody | Clear Melody (175R) on any difficulty. | 10 |  | `oe1_clear_s04` |
| 5 | Linda Linda | Clear Linda Linda (The Blue Hearts) on any difficulty. | 10 |  | `oe1_clear_s05` |
| 6 | Kokoro Odoru | Clear Kokoro Odoru (nobodyknows+) on any difficulty. | 10 |  | `oe1_clear_s06` |
| 7 | Atsuki Kodou no Hate | Clear Atsuki Kodou no Hate (B'z) on any difficulty. | 10 |  | `oe1_clear_s07` |
| 8 | Thrill | Clear Thrill (Tomoyasu Hotei) on any difficulty. | 10 |  | `oe1_clear_s08` |
| 9 | Taisetsu na Mono | Clear Taisetsu na Mono (Road of Major) on any difficulty. | 10 |  | `oe1_clear_s09` |
| 10 | Neraiuchi | Clear Neraiuchi (Linda Yamamoto) on any difficulty. | 10 |  | `oe1_clear_s10` |
| 11 | One Night Carnival | Clear One Night Carnival (Kishidan) on any difficulty. | 10 |  | `oe1_clear_s11` |
| 12 | Over the Distance | Clear Over the Distance (Hitomi Yaida) on any difficulty. | 10 |  | `oe1_clear_s12` |
| 13 | Taiyou ga Moeteiru | Clear Taiyou ga Moeteiru (The Yellow Monkey) on any difficulty. | 10 |  | `oe1_clear_s13` |
| 14 | Shanghai Honey | Clear Shanghai Honey (Orange Range) on any difficulty. | 10 |  | `oe1_clear_s14` |
| 15 | Ready Steady Go | Clear Ready Steady Go (L'Arc-en-Ciel) on any difficulty. | 10 |  | `oe1_clear_s15` |
| 101 | Hajime! | Open new songs on the map on Kigaru (wave 1 of 6). | 5 |  | `oe1_tier_e_1` |
| 102 | Osu, Senpai! | Open new songs on the map on Kigaru (wave 2 of 6). | 5 |  | `oe1_tier_e_2` |
| 103 | Raise the Flag | Open new songs on the map on Kigaru (wave 3 of 6). | 5 |  | `oe1_tier_e_3` |
| 104 | Hot-Blooded Rookie | Open new songs on the map on Kigaru (wave 4 of 6). | 5 |  | `oe1_tier_e_4` |
| 105 | Nekketsu Rising | Open new songs on the map on Kigaru (wave 5 of 6). | 5 |  | `oe1_tier_e_5` |
| 106 | Lungs of Steel | Open new songs on the map on Kigaru (wave 6 of 6). | 5 |  | `oe1_tier_e_6` |
| 111 | Ryuta Takes Charge | Open new songs on the map on Kakan (wave 1 of 6). | 5 |  | `oe1_tier_n_1` |
| 112 | Bold Cheers | Open new songs on the map on Kakan (wave 2 of 6). | 5 |  | `oe1_tier_n_2` |
| 113 | Fists to the Sky | Open new songs on the map on Kakan (wave 3 of 6). | 5 |  | `oe1_tier_n_3` |
| 114 | Tireless Cheering | Open new songs on the map on Kakan (wave 4 of 6). | 5 |  | `oe1_tier_n_4` |
| 115 | Burning Spirit | Open new songs on the map on Kakan (wave 5 of 6). | 5 |  | `oe1_tier_n_5` |
| 116 | Eyes on the Sky | Open new songs on the map on Kakan (wave 6 of 6). | 5 |  | `oe1_tier_n_6` |
| 121 | The President Steps In | Open new songs on the map on Gekiretsu (wave 1 of 6). | 10 |  | `oe1_tier_h_1` |
| 122 | Fierce Fervor | Open new songs on the map on Gekiretsu (wave 2 of 6). | 10 |  | `oe1_tier_h_2` |
| 123 | No Holding Back | Open new songs on the map on Gekiretsu (wave 3 of 6). | 10 |  | `oe1_tier_h_3` |
| 124 | Thunderous Applause | Open new songs on the map on Gekiretsu (wave 4 of 6). | 10 |  | `oe1_tier_h_4` |
| 125 | Unbreakable | Open new songs on the map on Gekiretsu (wave 5 of 6). | 10 |  | `oe1_tier_h_5` |
| 126 | Brace for Impact | Open new songs on the map on Gekiretsu (wave 6 of 6). | 10 |  | `oe1_tier_h_6` |
| 131 | Enter the Cheer Girls | Open new songs on the map on Karei (wave 1 of 6). | 25 |  | `oe1_tier_x_1` |
| 132 | Sayaka Sets the Pace | Open new songs on the map on Karei (wave 2 of 6). | 25 |  | `oe1_tier_x_2` |
| 133 | Aoi Keeps Time | Open new songs on the map on Karei (wave 3 of 6). | 25 |  | `oe1_tier_x_3` |
| 134 | Anna-ther Level | Open new songs on the map on Karei (wave 4 of 6). | 25 |  | `oe1_tier_x_4` |
| 135 | Pom-Poms of Fury | Open new songs on the map on Karei (wave 5 of 6). | 25 |  | `oe1_tier_x_5` |
| 136 | Graceful Under Fire | Open new songs on the map on Karei (wave 6 of 6). | 25 |  | `oe1_tier_x_6` |
| 201 | Ready, Steady, Cheer! | Clear Ready Steady Go on Kigaru with Hajime and roll the credits. | 25 | win | `oe1_credits_e` |
| 202 | Kai, You're Up! | Clear Ready Steady Go on Kakan with Ryuta and roll the credits, unlocking Gekiretsu, led by Kai. | 25 | win | `oe1_credits_n` |
| 203 | Bring On the Cheer Girls | Clear Ready Steady Go on Gekiretsu with Kai and roll the credits, unlocking Karei, led by the Cheer Girls: Sayaka, Aoi and Anna. | 50 | win | `oe1_credits_h` |
| 204 | The Comet Never Stood a Chance | Clear Ready Steady Go on Karei with the Cheer Girls and roll the credits. | 50 | win | `oe1_credits_x` |
| 301 | Osu! | Reach overall rank 1 of 11: 400,000 points of best scores, every song and difficulty added up. | 10 |  | `oe1_rank_1` |
| 302 | Fresh Gakuran | Reach overall rank 2 of 11: 1,200,000 points of best scores, every song and difficulty added up. | 25 |  | `oe1_rank_2` |
| 303 | Flag Bearer | Reach overall rank 3 of 11: 2,500,000 points of best scores, every song and difficulty added up. | 25 |  | `oe1_rank_3` |
| 304 | Taiko Pounder | Reach overall rank 4 of 11: 5,000,000 points of best scores, every song and difficulty added up. | 25 |  | `oe1_rank_4` |
| 305 | Megaphone Wielder | Reach overall rank 5 of 11: 8,000,000 points of best scores, every song and difficulty added up. | 25 |  | `oe1_rank_5` |
| 306 | Hot-Blooded | Reach overall rank 6 of 11: 12,000,000 points of best scores, every song and difficulty added up. | 25 |  | `oe1_rank_6` |
| 307 | Vice Captain | Reach overall rank 7 of 11: 17,000,000 points of best scores, every song and difficulty added up. | 25 |  | `oe1_rank_7` |
| 308 | Squad Captain | Reach overall rank 8 of 11: 25,000,000 points of best scores, every song and difficulty added up. | 25 |  | `oe1_rank_8` |
| 309 | Pride of Yuuhi Town | Reach overall rank 9 of 11: 35,000,000 points of best scores, every song and difficulty added up. | 25 |  | `oe1_rank_9` |
| 310 | Cheers Heard Round the World | Reach overall rank 10 of 11: 45,000,000 points of best scores, every song and difficulty added up. | 25 |  | `oe1_rank_10` |
| 311 | Ouendan Supreme | Reach overall rank 11 of 11, the top rank: 60,000,000 points of best scores, every song and difficulty added up. | 50 |  | `oe1_rank_11` |
| 501 | S Is for Spirit | Get your first S rank. | 10 |  | `oe1_s_1` |
| 502 | Sweat and Tears | Get S ranks on 5 songs (each song on each difficulty counts once). | 10 |  | `oe1_s_5` |
| 503 | Shouts in Unison | Get S ranks on 10 songs (each song on each difficulty counts once). | 25 |  | `oe1_s_10` |
| 504 | Spotless Spirit | Get S ranks on 20 songs (each song on each difficulty counts once). | 50 |  | `oe1_s_20` |
| 505 | Flawless Fighting Spirit | Get an S rank on every song on every difficulty (60). | 50 |  | `oe1_s_all` |
