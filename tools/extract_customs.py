"""Customs from the custom ROMs built by rom_hacking: each file that differs
from the stock ROM's, by its name, into <customs>/<tag>/files/, and
<customs>/<tag>/custom.json + custom.ini (the stage it replaces, its title and
artist, the files). ndsrun's --replace-file takes them and builds that
custom's cartridge from the stock ROM at the start.

    python tools/extract_customs.py roms/EBA.nds TAG [TAG ...]
    python tools/extract_customs.py roms/OE2.nds --game OE2 TAG [TAG ...]

EBA's go to customs/, Ouendan 2's to customs_oe2/.
"""
import argparse
import json
import os
import shutil
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from ndskit.rom import Rom  # noqa: E402

RH = r"F:\dxvk_remix\Claude\EBAHD Old\rom_hacking"
OE2_ROMS = RH + r"\ouendan2\patched_roms"

GAMES = {
    "EBA": {
        "dir": "customs",
        # the save the menu route of Customs mode is known on (games.cpp's route)
        "save": RH + r"\original_roms\Elite Beat Agents (USA).sav",
        # the story's stages in the game's order (its table at 0x020B3EE4): a stage's index
        "stages": ["SS01", "SS02", "SS06", "SS04", "SS05", "SS09", "SS10", "SS03", "SS08", "SS11", "SS13", "SS12",
                   "SSB03", "SS07", "SS15A", "SS15B", "SSB02", "SSB01", "SS14"],
        # tag: (the custom ROM, title, artist, the stage replaced, its song)
        "customs": {
            "SS02_Movies": (RH + r"\patched_roms\Elite Beat Agents (FG_SS02_Movies).nds",
                            "Movies", "Alien Ant Farm", "SS02", "Makes No Difference"),
            "SS05_AllStar": (RH + r"\patched_roms\Elite Beat Agents (FG_SS05_AllStar).nds",
                             "All Star", "Smash Mouth", "SS05", "Rock This Town"),
            "SS09_LifeIsAHighway": (RH + r"\patched_roms\Elite Beat Agents (FG_SS09_LifeIsAHighway).nds",
                                    "Life Is a Highway", "Rascal Flatts", "SS09", "Highway Star"),
            "SS10_ComeAndGet": (RH + r"\patched_roms\Elite Beat Agents (FG_SS10_ComeAndGet).nds",
                                "Come and Get Your Love", "Redbone", "SS10", "YMCA"),
            "SS11_GirlsJustWanna": (RH + r"\patched_roms\Elite Beat Agents (FG_SS11_GirlsJustWanna).nds",
                                    "Girls Just Want to Have Fun", "Cyndi Lauper", "SS11", "Material Girl"),
        },
    },
    "OE2": {
        "dir": "customs_oe2",
        "save": RH + r"\original_roms\Moero! Nekketsu Rhythm Damashii - Osu! Tatakae! Ouendan 2 (Japan).sav",
        # the story's stages (alone, ひとりで応援), its table at 0x02092020
        "stages": ["OS01", "OS02", "OS04", "OS06", "OS08", "OS10", "OSB02", "OS11", "OS03", "OS07", "OS14", "OS12",
                   "OS13", "OS05", "OS15A", "OS15B", "OSB03", "OSB01", "OS09"],
        # the names as the options bar's font has them (no kana, no macrons)
        "customs": {
            "OS02_Hohoemi": (OE2_ROMS + r"\Ouendan 2 (Hohoemi).nds",
                             "Hohoemi no Bakudan", "Matsuko Mawatari", "OS02", "Real Life Real Heart"),
            "OS03_Madder": (OE2_ROMS + r"\Ouendan 2 (Madder_Kibun).nds",
                            "Madder", "", "OS03", "Kibun Jojo"),
            "OS05_Takedown": (OE2_ROMS + r"\Keep These\Ouendan 2 (Takedown_MusicHour).nds",
                              "Takedown", "HUNTR/X", "OS05", "Music Hour"),
            "OSB02_IWantYou": (OE2_ROMS + r"\Keep These\Ouendan 2 (IWantYou2X).nds",
                               "I Want You", "CZR", "OSB02", "VISTA"),
        },
    },
}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("rom")
    ap.add_argument("tags", nargs="+")
    ap.add_argument("--game", default="EBA", choices=sorted(GAMES))
    a = ap.parse_args()
    g = GAMES[a.game]
    stock = Rom(open(a.rom, "rb").read())
    out_root = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), g["dir"])
    os.makedirs(out_root, exist_ok=True)
    pristine = os.path.join(out_root, "pristine.sav")
    if not os.path.exists(pristine):            # the save Customs mode starts from, each time
        shutil.copyfile(g["save"], pristine)
    for tag in a.tags:
        path, title, artist, stage, replaces = g["customs"][tag]
        r = Rom(open(path, "rb").read())
        if r.arm9.plain != stock.arm9.plain or len(r.fat) != len(stock.fat):
            raise SystemExit("%s: not the stock program or file table" % tag)
        d = os.path.join(out_root, tag, "files")
        os.makedirs(d, exist_ok=True)
        files = []
        for i, ((x, y), (c, e)) in enumerate(zip(stock.fat, r.fat)):
            if stock.data[x:y] == r.data[c:e]:
                continue
            name = stock.names[i]
            with open(os.path.join(d, name.replace("/", "__")), "wb") as f:
                f.write(r.data[c:e])
            files.append({"id": i, "name": name, "size": e - c, "stock_size": y - x})
        index = g["stages"].index(stage)
        man = {"tag": tag, "title": title, "artist": artist, "stage": stage, "stage_index": index,
               "replaces": replaces, "rom": path, "files": files}
        with open(os.path.join(out_root, tag, "custom.json"), "w") as f:
            json.dump(man, f, indent=2)
        ini = os.path.join(out_root, tag, "custom.ini")
        keep = []                               # the bake's keys, kept across a new extraction
        if os.path.exists(ini):
            base = ("title=", "artist=", "stage=", "stage_index=", "replaces=", "file=")
            keep = [l for l in open(ini, encoding="utf-8").read().splitlines() if l and not l.startswith(base)]
        with open(ini, "w", encoding="utf-8", newline="\n") as f:
            f.write("title=%s\nartist=%s\nstage=%s\nstage_index=%d\nreplaces=%s\n" % (title, artist, stage, index, replaces))
            for x in keep:
                f.write(x + "\n")
            for x in files:
                f.write("file=%s\n" % x["name"])
        print("%-22s %d files, %.1f MB" % (tag, len(files), sum(x["size"] for x in files) / 1e6))


if __name__ == "__main__":
    main()
