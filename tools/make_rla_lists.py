"""Makes the Recomp Achievements lists (RecompLauncher's own achievements) for the three games:
docs/recomp-achievements/<id>.json, in the list format of github.com/elripalda/RecompAchievements
(docs/SPEC.md, format 1, "events"), plus docs/recomp-achievements/LISTS.md to read them.

    python tools/make_rla_lists.py          (writes them; checks every game's points are 1000)

Titles, descriptions and points live here: rename or rewrite anything below and run it again.
The event names are the port's (src/eba_rla.cpp) and must not change once players have unlocks.
Points are 5, 10, 25 or 50 (the spec). The four credits rolls are "win" and nothing is
"progression", so a game is Beaten as soon as its finale is cleared and the credits roll, on any
difficulty (RecompLauncher: every progression achievement, none here, and one win). The bonus
songs' first clears are fired by the game but left out of the lists: unlocking them is the moment.
"""
import json
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "docs", "recomp-achievements")
VERSIONS = ">=1.3.0"        # the recomp versions the events were checked on (the release that brings them)
CONSOLE = "gba"             # a stand-in: the hub only takes SPEC.md's consoles, no "nds" yet (events lists don't use it)
TIERS = 6                   # the map's "New songs" waves before the finale (the port fires 1..7)
# RecompLauncher lists the release under one id for now: the three lists in one, the ids 1000, 2000
# and 3000 on, each description starting with its game (the hub's list today; the three per-game
# lists are for when each game has its own tile)
COMBINED = "ebaouendan-musiclives"
COMBINED_ORDER = [("elitebeatagents", 1000, "EBA"), ("ouendan1", 2000, "Ouendan"), ("ouendan2", 3000, "Ouendan 2")]

GAMES = {
    "elitebeatagents": {
        "prefix": "eba", "name": "Elite Beat Agents",
        "difficulties": ["Breezin'", "Cruisin'", "Sweatin'", "Hard Rock"],
        # who leads each difficulty (the credits' descriptions), and who a credits roll unlocks
        "leads": ["Agent Spin", "Agent J", "Agent Chieftain", "the Elite Beat Divas"],
        "unlocks": {2: "Agent Chieftain", 3: "the Elite Beat Divas: Starr, Missy and Foxx"},
        # the story's songs, in the game's own stage order (src/games.cpp), then the bonus songs
        "songs": [
            ("SS01", "Walkie Talkie Man", "Steriogram"),
            ("SS02", "Makes No Difference", "Sum 41"),
            ("SS06", "Sk8er Boi", "Avril Lavigne"),
            ("SS04", "I Was Born to Love You", "Freddie Mercury"),
            ("SS05", "Rock This Town", "Stray Cats"),
            ("SS09", "Highway Star", "Deep Purple"),
            ("SS10", "YMCA", "Village People"),
            ("SS03", "September", "Earth, Wind & Fire"),
            ("SS08", "Canned Heat", "Jamiroquai"),
            ("SS11", "Material Girl", "Madonna"),
            ("SS13", "La La", "Ashlee Simpson"),
            ("SS12", "You're the Inspiration", "Chicago"),
            ("SSB03", "Let's Dance", "David Bowie"),
            ("SS07", "The Anthem", "Good Charlotte"),
            ("SS15A", "Without a Fight", "Hoobastank"),
            ("SS15B", "Jumpin' Jack Flash", "The Rolling Stones"),
        ],
        "bonus": [
            ("SSB02", "Believe", "Cher", "Auto-Tuned In"),
            ("SSB01", "ABC", "The Jackson 5", "Learn Your ABCs"),
            ("SS14", "Survivor", "Destiny's Child", "Last Agent Standing"),
        ],
        "finale": "Jumpin' Jack Flash",
        "tiers": [
            ["Fresh Recruit", "Light Breeze", "Spin Cycle", "Tailwind", "Gale Force", "Eye of the Storm"],
            ["Cruise Control", "Agent J on the Case", "Open Road", "Need for Speed", "Overdrive", "Final Approach"],
            ["Breaking a Sweat", "Chieftain's Orders", "Turning Up the Heat", "Hot Streak", "Feel the Burn",
             "Boiling Point"],
            ["Enter the Divas", "Starr Power", "Foxx Trot", "Missy Impossible", "Rock Steady", "Showstoppers"],
        ],
        "credits": ["Gone with the Wind", "Calling Agent Chieftain", "Cue the Divas", "Commander Kahn Approves"],
        "ranks": [
            (400000, "Trainee"), (1500000, "Field Agent"), (3000000, "Special Agent"),
            (6000000, "Secret Agent Man"), (10000000, "Licensed to Thrill"), (15000000, "Undercover Groove"),
            (20000000, "Classified"), (25000000, "Top Secret"), (35000000, "Rhombulan Repeller"),
            (50000000, "Kahn's Right Hand"), (80000000, "Truly Elite"),
        ],
        "s": ["Sharp-Dressed Agent", "Smooth Operator", "Sunglasses at Night", "S-Class Agent", "Elite of the Elite"],
        "points": {
            "song": 5, "tier": [5, 5, 10, 25], "credits": [25, 50, 50, 50],
            "rank": lambda k, n: 50 if k == n else 10 if k <= 4 else 25,
            "bonus": 25, "s": [10, 25, 25, 50, 50],
        },
    },
    "ouendan2": {
        "prefix": "oe2", "name": "Moero! Nekketsu Rhythm Damashii Osu! Tatakae! Ouendan 2",
        "difficulties": ["Kigaru", "Kakan", "Gekiretsu", "Karei"],
        # two squads: the red Yuuhi Town Ouendan (the first game's roster) and the blue Asahi Town Ouendan
        "leads": ["Hajime and Shinta", "Ryuta and Hayato", "Kai and Kaoru", "the Cheer Girls and the Cheer Maidens"],
        "unlocks": {2: "Kai and Kaoru",
                    3: "the Cheer Girls (Sayaka, Aoi and Anna) and the Cheer Maidens (Rin, Honoka and Reika)"},
        "songs": [
            ("OS01", "Zenryoku Shounen", "Sukima Switch"),
            ("OS02", "Real Life Real Heart", "Kaela Kimura"),
            ("OS04", "Okuru Kotoba", "FLOW"),
            ("OS06", "Pop Star", "Ken Hirai"),
            ("OS08", "Go My Way", "Hitomi Yaida"),
            ("OS10", "Julia ni Heartbreak", "The Checkers"),
            ("OSB02", "Going Under Ground", "VISTA"),
            ("OS11", "Shounen Heart", "Home Made Kazoku"),
            ("OS03", "Kibun Joujou", "mihimaru GT"),
            ("OS07", "Bambina", "Tomoyasu Hotei"),
            ("OS14", "Bang! Bang! Vacances!", "SMAP"),
            ("OS12", "Believe", "AI"),
            ("OS13", "Zoku", "Kishidan"),
            ("OS05", "Music Hour", "Porno Graffitti"),
            ("OS15A", "Countdown", "HYDE"),
            ("OS15B", "Sekai wa Sore o Ai to Yobundaze", "Sambomaster"),
        ],
        "bonus": [
            ("OSB03", "Monkey Magic", "Orange Range", "Magic Monkey My Hero"),
            ("OSB01", "Glamorous Sky", "NANA starring Mika Nakashima", "...As It Speeds You From My Life"),
            ("OS09", "Samurai Blue", "ZZ", "He Didn't Ghost You"),
        ],
        "finale": "Sekai wa Sore o Ai to Yobundaze",
        "tiers": [
            ["Hajime, Round Two", "Enter Shinta", "Rookie Rivalry", "Seeing Red", "Feeling Blue",
             "The Countdown Begins"],
            ["Ryuta Returns", "Hayato Takes the Lead", "Red vs. Blue", "Hot Blood, Cool Heads", "Two Flags, One Sky",
             "T-Minus Cheer"],
            ["Kai Meets Kaoru", "Battle of the Presidents", "Clash of Squads", "Blue Blazes", "Red Inferno",
             "Showdown Approaching"],
            ["Girls vs. Maidens", "Rin's Resolve", "Honoka's Harmony", "Reika's Rally", "Sayaka Answers Back",
             "Grace Under Pressure"],
        ],
        "credits": ["The World Calls It Love", "Kai and Kaoru, You're Up!", "Enter the Cheer Squads",
                    "Here Comes the Sun (Again)"],
        "ranks": [
            (400000, "Rookie Cheerer"), (1500000, "Red or Blue?"), (3000000, "Flag Bearer"),
            (6000000, "Taiko Thunder"), (10000000, "Megaphone Maestro"), (15000000, "Encouraging Nobility"),
            (20000000, "Vice Captain"), (25000000, "Squad Captain"), (35000000, "Respected by Rivals"),
            (50000000, "Pride of Both Squads"), (70000000, "Living Legend"), (100000000, "Heart of the Ouendan"),
        ],
        "s": ["Stamp of Approval", "Senpai Is Impressed", "Synchronized Shouting", "Sweat, Spirit and S Ranks",
              "Undying Fighting Spirit"],
        "points": {
            "song": 5, "tier": [5, 5, 10, 25], "credits": [25, 25, 50, 50],
            "rank": lambda k, n: 50 if k == n else 10 if k <= 4 else 25,
            "bonus": 25, "s": [10, 25, 25, 50, 50],
        },
    },
    "ouendan1": {
        "prefix": "oe1", "name": "Osu! Tatakae! Ouendan",
        "difficulties": ["Kigaru", "Kakan", "Gekiretsu", "Karei"],
        "leads": ["Hajime", "Ryuta", "Kai", "the Cheer Girls"],
        "unlocks": {2: "Kai", 3: "the Cheer Girls: Sayaka, Aoi and Anna"},
        "songs": [
            ("s01", "Loop & Loop", "Asian Kung-Fu Generation"),
            ("s02", "Koi no Dance Site", "Morning Musume"),
            ("s03", "Guts Da Ze!!", "Ulfuls"),
            ("s04", "Melody", "175R"),
            ("s05", "Linda Linda", "The Blue Hearts"),
            ("s06", "Kokoro Odoru", "nobodyknows+"),
            ("s07", "Atsuki Kodou no Hate", "B'z"),
            ("s08", "Thrill", "Tomoyasu Hotei"),
            ("s09", "Taisetsu na Mono", "Road of Major"),
            ("s10", "Neraiuchi", "Linda Yamamoto"),
            ("s11", "One Night Carnival", "Kishidan"),
            ("s12", "Over the Distance", "Hitomi Yaida"),
            ("s13", "Taiyou ga Moeteiru", "The Yellow Monkey"),
            ("s14", "Shanghai Honey", "Orange Range"),
            ("s15", "Ready Steady Go", "L'Arc-en-Ciel"),
        ],
        "bonus": [],
        "finale": "Ready Steady Go",
        "tiers": [
            ["Hajime!", "Osu, Senpai!", "Raise the Flag", "Hot-Blooded Rookie", "Nekketsu Rising", "Lungs of Steel"],
            ["Ryuta Takes Charge", "Bold Cheers", "Fists to the Sky", "Tireless Cheering", "Burning Spirit",
             "Eyes on the Sky"],
            ["The President Steps In", "Fierce Fervor", "No Holding Back", "Thunderous Applause", "Unbreakable",
             "Brace for Impact"],
            ["Enter the Cheer Girls", "Sayaka Sets the Pace", "Aoi Keeps Time", "Anna-ther Level", "Pom-Poms of Fury",
             "Graceful Under Fire"],
        ],
        "credits": ["Ready, Steady, Cheer!", "Kai, You're Up!", "Bring On the Cheer Girls",
                    "The Comet Never Stood a Chance"],
        "ranks": [
            (400000, "Osu!"), (1200000, "Fresh Gakuran"), (2500000, "Flag Bearer"), (5000000, "Taiko Pounder"),
            (8000000, "Megaphone Wielder"), (12000000, "Hot-Blooded"), (17000000, "Vice Captain"),
            (25000000, "Squad Captain"), (35000000, "Pride of Yuuhi Town"),
            (45000000, "Cheers Heard Round the World"), (60000000, "Ouendan Supreme"),
        ],
        "s": ["S Is for Spirit", "Sweat and Tears", "Shouts in Unison", "Spotless Spirit", "Flawless Fighting Spirit"],
        "points": {
            "song": 10, "tier": [5, 5, 10, 25], "credits": [25, 25, 50, 50],
            "rank": lambda k, n: 50 if k == n else 10 if k == 1 else 25,
            "bonus": 0, "s": [10, 10, 25, 50, 50],
        },
    },
}


def make(gid, g):
    P, pts, D = g["prefix"], g["points"], g["difficulties"]
    out = []

    def add(aid, title, desc, points, event, typ=None):
        a = {"id": aid, "title": title, "description": desc, "points": points}
        if typ:
            a["type"] = typ
        a["event"] = event
        out.append(a)

    for i, (stem, song, artist) in enumerate(g["songs"]):
        add(1 + i, song, "Clear %s (%s) on any difficulty." % (song, artist), pts["song"],
            "%s_clear_%s" % (P, stem.lower()))
    for d, diff in enumerate(D):
        for k in range(1, TIERS + 1):
            add(100 + d * 10 + k, g["tiers"][d][k - 1],
                "Open new songs on the map on %s (wave %d of %d)." % (diff, k, TIERS), pts["tier"][d],
                "%s_tier_%s_%d" % (P, "enhx"[d], k))
    for d, diff in enumerate(D):
        lead = g["leads"][d]
        desc = "Clear %s on %s%s and roll the credits" % (g["finale"], diff, " with " + lead if lead else "")
        if d + 1 in g["unlocks"]:
            desc += ", unlocking %s, led by %s" % (D[d + 1], g["unlocks"][d + 1])
        add(201 + d, g["credits"][d], desc + ".", pts["credits"][d], "%s_credits_%s" % (P, "enhx"[d]), "win")
    n = len(g["ranks"])
    for k, (score, title) in enumerate(g["ranks"], 1):
        add(300 + k, title,
            "Reach overall rank %d of %d%s: %s points of best scores, every song and difficulty added up." % (
                k, n, ", the top rank" if k == n else "", "{:,}".format(score)),
            pts["rank"](k, n), "%s_rank_%d" % (P, k))
    for j, (stem, song, artist, title) in enumerate(g["bonus"]):
        add(401 + j, title, "Unlock the bonus song %s (%s)." % (song, artist), pts["bonus"],
            "%s_bonus_%s" % (P, stem.lower()))
    total = (len(g["songs"]) + len(g["bonus"])) * 4
    for j, (cnt, title) in enumerate(zip(["1", "5", "10", "20", "all"], g["s"])):
        desc = ("Get an S rank on every song on every difficulty%s (%d)." % (
                    ", bonus songs included" if g["bonus"] else "", total) if cnt == "all"
                else "Get your first S rank." if cnt == "1"
                else "Get S ranks on %s songs (each song on each difficulty counts once)." % cnt)
        add(501 + j, title, desc, pts["s"][j], "%s_s_%s" % (P, cnt))
    return {"format": 1, "game": gid, "source": "events", "console": CONSOLE, "versions": VERSIONS,
            "achievements": out}


def main():
    os.makedirs(OUT, exist_ok=True)
    ok = True
    md = ["# Recomp Achievements: the three lists", "",
          "Made by `tools/make_rla_lists.py` (edit the titles, descriptions and points there, then run it again).",
          "The event is what the game fires (`src/eba_rla.cpp`); RecompLauncher's list maps it to the achievement.",
          "A game is Beaten when the credits roll on any difficulty (the *win* ones), and Mastered when every"
          " achievement is unlocked.", ""]
    for gid, g in GAMES.items():
        lst = make(gid, g)
        a = lst["achievements"]
        pts = sum(x["points"] for x in a)
        bad = [x for x in a if x["points"] not in (5, 10, 25, 50)]
        ids = [x["id"] for x in a]
        titles = [x["title"] for x in a]
        dup = sorted({t for t in titles if titles.count(t) > 1})
        if pts != 1000 or bad or len(set(ids)) != len(ids) or dup:
            print("%s: %d points, %d bad points, ids unique: %s, titles twice: %s" % (
                gid, pts, len(bad), len(set(ids)) == len(ids), dup))
            ok = False
        with open(os.path.join(OUT, gid + ".json"), "w", encoding="utf-8", newline="\n") as f:
            json.dump(lst, f, indent=2, ensure_ascii=False)
            f.write("\n")
        md += ["## %s (`%s`): %d achievements, %d points" % (g["name"], gid, len(a), pts), "",
               "| id | Title | Description | Points | Type | Event |", "|---|---|---|---|---|---|"]
        for x in a:
            md.append("| %d | %s | %s | %d | %s | `%s` |" % (x["id"], x["title"], x["description"], x["points"],
                                                          x.get("type", ""), x["event"]))
        md.append("")
        print("%s: %d achievements, %d points" % (gid, len(a), pts))
    with open(os.path.join(OUT, "LISTS.md"), "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(md))
    allof = []
    for gid, base, short in COMBINED_ORDER:
        for x in make(gid, GAMES[gid])["achievements"]:
            allof.append(dict(x, id=base + x["id"], description=short + ": " + x["description"]))
    ids = [x["id"] for x in allof]
    if len(set(ids)) != len(ids) or len({x["event"] for x in allof}) != len(allof):
        print("the combined list repeats an id or an event")
        ok = False
    with open(os.path.join(OUT, COMBINED + ".json"), "w", encoding="utf-8", newline="\n") as f:
        json.dump({"format": 1, "game": COMBINED, "source": "events", "console": CONSOLE, "versions": VERSIONS,
                   "achievements": allof}, f, indent=2, ensure_ascii=False)
        f.write("\n")
    print("%s (combined): %d achievements, %d points" % (COMBINED, len(allof), sum(x["points"] for x in allof)))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
