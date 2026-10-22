"""Customs from the custom ROMs built by rom_hacking (patched_roms/Elite Beat
Agents (FG_<tag>).nds): each file that differs from the stock ROM's, by its
name, into customs/<tag>/files/, and customs/<tag>/custom.json (the stage it
replaces, its title and artist, the files). ndsrun's --replace-files takes
the folder and builds that custom's cartridge from the stock ROM at the start.

    python tools/extract_customs.py roms/EBA.nds TAG [TAG ...]
"""
import json
import os
import shutil
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from ndskit.rom import Rom  # noqa: E402

PATCHED = r"F:\dxvk_remix\Claude\EBAHD Old\rom_hacking\patched_roms\Elite Beat Agents (FG_%s).nds"
# the save the menu route of Customs mode is known on (eba_customs.cpp)
PRISTINE_SAVE = r"F:\dxvk_remix\Claude\EBAHD Old\rom_hacking\original_roms\Elite Beat Agents (USA).sav"
# the story's stages in the game's order (its table at 0x020B3EE4): a stage's index
STAGES = ["SS01", "SS02", "SS06", "SS04", "SS05", "SS09", "SS10", "SS03", "SS08", "SS11", "SS13", "SS12",
          "SSB03", "SS07", "SS15A", "SS15B", "SSB02", "SSB01", "SS14"]

# title, artist, the stage replaced and its song
INFO = {
    "SS02_Movies": ("Movies", "Alien Ant Farm", "SS02", "Makes No Difference"),
    "SS05_AllStar": ("All Star", "Smash Mouth", "SS05", "Rock This Town"),
    "SS09_LifeIsAHighway": ("Life Is a Highway", "Rascal Flatts", "SS09", "Highway Star"),
    "SS10_ComeAndGet": ("Come and Get Your Love", "Redbone", "SS10", "YMCA"),
    "SS11_GirlsJustWanna": ("Girls Just Want to Have Fun", "Cyndi Lauper", "SS11", "Material Girl"),
}


def main():
    stock = Rom(open(sys.argv[1], "rb").read())
    out_root = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "customs")
    os.makedirs(out_root, exist_ok=True)
    pristine = os.path.join(out_root, "pristine.sav")
    if not os.path.exists(pristine):            # the save Customs mode starts from, each time
        shutil.copyfile(PRISTINE_SAVE, pristine)
    for tag in sys.argv[2:]:
        r = Rom(open(PATCHED % tag, "rb").read())
        if r.arm9.plain != stock.arm9.plain or len(r.fat) != len(stock.fat):
            raise SystemExit("%s: not the stock program or file table" % tag)
        d = os.path.join(out_root, tag, "files")
        os.makedirs(d, exist_ok=True)
        files = []
        for i, ((a, b), (c, e)) in enumerate(zip(stock.fat, r.fat)):
            if stock.data[a:b] == r.data[c:e]:
                continue
            name = stock.names[i]
            with open(os.path.join(d, name.replace("/", "__")), "wb") as f:
                f.write(r.data[c:e])
            files.append({"id": i, "name": name, "size": e - c, "stock_size": b - a})
        title, artist, stage, replaces = INFO.get(tag, (tag, "", tag.split("_")[0], ""))
        index = STAGES.index(stage)
        man = {"tag": tag, "title": title, "artist": artist, "stage": stage, "stage_index": index,
               "replaces": replaces, "files": files}
        with open(os.path.join(out_root, tag, "custom.json"), "w") as f:
            json.dump(man, f, indent=2)
        with open(os.path.join(out_root, tag, "custom.ini"), "w", encoding="utf-8", newline="\n") as f:
            f.write("title=%s\nartist=%s\nstage=%s\nstage_index=%d\nreplaces=%s\n" % (title, artist, stage, index, replaces))
            for x in files:
                f.write("file=%s\n" % x["name"])
        print("%-22s %d files, %.1f MB" % (tag, len(files), sum(x["size"] for x in files) / 1e6))


if __name__ == "__main__":
    main()
