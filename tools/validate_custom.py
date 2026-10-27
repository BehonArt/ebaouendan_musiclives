"""Validate a custom from a shell: the recomp plays it through headless and reports (studio/validate.py).

    py -3 tools/validate_custom.py customs/SS05_AllStar [--difficulty n] [--game EBA]

Under a minute, no window. The report: loaded, notes played, the score and grades, cleared or
failed, and how many seconds of the song follow the last note. Exit code 1 if it found problems.
Needs the game's play folder (tools/make_play_folder.ps1).
"""
import argparse
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "studio"))
import recomp_bridge as B  # noqa: E402
import validate as V  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("custom", help="a custom's folder (customs/<tag>)")
    ap.add_argument("--difficulty", default="n", choices=list("enhx"))
    ap.add_argument("--game", default="", choices=["", "EBA", "OE2", "OE1"])
    ap.add_argument("--recomp", default=ROOT, help="the recomp's root (its play folders)")
    ap.add_argument("--work", default=os.path.join(ROOT, "build", "validate"))
    a = ap.parse_args()
    d = os.path.abspath(a.custom)
    tag = os.path.basename(d)
    game = a.game
    if not game:                                         # by the stage the custom replaces
        with open(os.path.join(d, "custom.ini"), encoding="utf-8-sig") as f:
            stage = next((l.split("=", 1)[1].strip() for l in f if l.startswith("stage=")), "")
        game = next((g for g, info in B.GAMES.items() if stage in info["stages"]), "EBA")
    r = V.validate_custom(game, dict(dir=d, tag=tag), a.difficulty, a.recomp, a.work, say=print)
    print(V.report_text(r))
    print("  (log: %s)" % r["log"])
    sys.exit(1 if r["problems"] else 0)


if __name__ == "__main__":
    main()
