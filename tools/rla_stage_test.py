"""Recomp Achievements: every stage of the three games cleared headless, each checked to send its
own song's event (P_clear_STAGE), through tools/rla_fake_launcher.py.

    python tools/rla_stage_test.py [eba oe1 oe2] [--jobs 6] [--stage N]

Each run starts from a copy of the game's Customs start save (play*/customs/pristine.sav: the
game's own first boot), taps through the menus to the first card on Normal, plays the stage
`--port rla-stage=N` sets during the opening comic (as Customs set theirs) with
`--port buttons-auto`, and expects P_clear_<that stage's stem>. Windows only; writes under
build/rla_stage_test/.
"""
import argparse
import os
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GAMES = {   # the stems in each game's stage order (src/games.cpp), the build, the start save, the route
    "eba": ("SS01 SS02 SS06 SS04 SS05 SS09 SS10 SS03 SS08 SS11 SS13 SS12 SSB03 SS07 SS15A SS15B SSB02 SSB01 SS14",
            "build/recomp/b/ndsrun.exe", "roms/EBA.nds", "play/customs/pristine.sav",
            "950:tap:128:150,1300:tap:128:66,2400:tap:128:153,2450:tap:236:178,3650:tap:126:103,4300:tap:236:178"),
    "oe1": ("s01 s02 s03 s04 s05 s06 s07 s08 s09 s10 s11 s12 s13 s14 s15",
            "build/oe1/recomp/b/ndsrun.exe", "roms/OE1.nds", "play_oe1/customs/pristine.sav",
            "700:tap:128:150,1350:tap:150:48,2050:tap:236:183,3050:tap:160:76,3450:tap:236:183"),
    "oe2": ("OS01 OS02 OS04 OS06 OS08 OS10 OSB02 OS11 OS03 OS07 OS14 OS12 OS13 OS05 OS15A OS15B OSB03 OSB01 OS09",
            "build/oe2/recomp/b/ndsrun.exe", "roms/OE2.nds", "play_oe2/customs/pristine.sav",
            "1850:tap:128:150,2650:tap:65:55,3350:tap:238:183,5400:tap:130:72,5900:tap:238:183"),
}


def run(game, i):
    stems, exe, rom, save, route = GAMES[game]
    stem = stems.split()[i]
    out = os.path.join(ROOT, "build", "rla_stage_test", "%s_%02d_%s" % (game, i, stem))
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(out)
    shutil.copy(os.path.join(ROOT, save), os.path.join(out, "test.sav"))
    event = "%s_clear_%s" % (game, stem.lower())
    cmd = [sys.executable, "-I", os.path.join(ROOT, "tools", "rla_fake_launcher.py"), os.path.join(out, "msgs.jsonl"),
           event, "--", os.path.join(ROOT, exe), os.path.join(ROOT, rom), "--save", os.path.join(out, "test.sav"),
           "--out", out, "--vblanks", "45000", "--no-dump", "--mute", "--input", route,
           "--port", "buttons-auto", "--port", "rla-stage=%d" % i]
    r = subprocess.run(cmd, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    ev = next((l[8:] for l in r.stdout.splitlines() if l.startswith("events: ")), "")
    return game, i, stem, r.returncode == 0, ev


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("games", nargs="*", default=list(GAMES))
    ap.add_argument("--jobs", type=int, default=6)
    ap.add_argument("--stage", type=int, default=None)
    a = ap.parse_args()
    todo = [(g, i) for g in a.games for i in range(len(GAMES[g][0].split())) if a.stage in (None, i)]
    bad = 0
    with ThreadPoolExecutor(a.jobs) as ex:
        for game, i, stem, ok, ev in ex.map(lambda t: run(*t), todo):
            print("%s %s %2d %-6s %s" % ("PASS" if ok else "FAIL", game, i, stem, ev), flush=True)
            bad += not ok
    print("%d of %d stages send their song's event" % (len(todo) - bad, len(todo)))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
