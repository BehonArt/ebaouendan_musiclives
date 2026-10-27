"""Validate a custom: the recomp plays it through headless and the run is turned into a report.

The recomp's button-mode test player (--port buttons-auto) hits every note on its frame, so a
run tells what a chart does in the real game code: whether it loads, how many notes are judged
and how, the score, whether the stage is cleared, and where the song ends (the recomp logs the
song's last clock frame), so how much of the song follows the last note with nothing to hit.
Under a minute, no window (headless runs are uncapped); --port customs-quit ends the run 2 s after the song is over.

    validate_custom(game, custom, difficulty, recomp_root, work_dir) -> report (dict)
    report_text(report) -> the report in words
"""
import os
import re
import shutil
import subprocess

import recomp_bridge as B

SONG_SCENE = {"EBA": 0x020B65A0, "OE2": 0x020B0888, "OE1": 0x02088A2C}   # games.cpp's scenes.song
FPS = 60.0
DEAD_MAX_S = 25                         # stock stages end 10 to 22 s after their last note
DEAD_MIN_S = 5


def tc(frames):
    s = frames / FPS
    return "%d:%04.1f" % (s // 60, s % 60)


def validate_custom(game, custom, difficulty, recomp_root, work_dir, max_vblanks=40000, say=None):
    """custom: {dir, tag} of a custom's folder (customs/<tag>/ with custom.ini and files/)."""
    recomp_root, work_dir = os.path.abspath(recomp_root), os.path.abspath(work_dir)   # (the run starts in its folder)
    custom = dict(custom, dir=os.path.abspath(custom["dir"]))
    rf = B.recomp_folder(recomp_root, game)
    customs = os.path.dirname(custom["dir"])
    os.makedirs(work_dir, exist_ok=True)
    pristine = os.path.join(customs, "pristine.sav")
    if not os.path.exists(pristine):                     # the save the Customs route is known on
        shutil.copyfile(os.path.join(rf["customs"], "pristine.sav"), pristine)
    session = os.path.join(work_dir, "validate_session.sav")
    shutil.copyfile(pristine, session)
    campaign = os.path.join(work_dir, "validate_campaign.sav")   # a copy: the player's own is never touched
    shutil.copyfile(rf["save"] if os.path.exists(rf["save"]) else pristine, campaign)
    args = [rf["exe"], rf["rom"], "--save", session, "--out", work_dir, "--vblanks", str(max_vblanks),
            "--no-dump", "--port", "customs-dir=" + customs, "--port", "custom=" + custom["tag"],
            "--port", "difficulty=" + difficulty, "--port", "campaign-save=" + campaign,
            "--port", "buttons-auto", "--port", "buttons-log", "--port", "scene-log", "--port", "customs-quit"]
    with open(os.path.join(custom["dir"], "custom.ini"), encoding="utf-8-sig") as f:
        for line in f:
            if line.startswith("file="):
                name = line[5:].strip()
                args += ["--replace-file",
                         "%s=%s" % (name, os.path.join(custom["dir"], "files", name.replace("/", "__")))]
    log_path = os.path.join(work_dir, "validate.log")
    if say:
        say("playing %s in the recomp, headless (under a minute)..." % custom["tag"])
    with open(log_path, "w") as log:
        proc = subprocess.run(args, cwd=rf["dir"], stdout=log, stderr=subprocess.STDOUT,
                              creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    with open(log_path, errors="replace") as f:
        r = parse(game, f.read(), proc.returncode)
    r["log"] = log_path
    return r


def parse(game, text, returncode=0):
    r = dict(game=game, loaded=False, notes=0, last_note=0, score=None, grades=None, cleared=None,
             song_frames=None, dead_s=None, crashed=False, problems=[])
    song = "%08X" % SONG_SCENE[game]
    for m in re.finditer(r"eba customs: scene ([0-9A-F]{8}) at VBlank \d+", text):
        if m.group(1) == song:
            r["loaded"] = True
    # the test player's presses: "press at song frame F: #N kind, time T (+d), state S"
    times = [int(m.group(1)) for m in re.finditer(r"press at song frame \d+: #\d+ \w+, time (\d+)", text)]
    r["notes"] = len(set(times))
    r["last_note"] = max(times) if times else 0
    m = re.search(r"eba customs: .*?: score (\d+) \(300 x(\d+), 100 x(\d+), 50 x(\d+), miss x(\d+)\)(, cleared)?", text)
    if m:
        r["score"] = int(m.group(1))
        r["grades"] = [int(m.group(i)) for i in range(2, 6)]
        r["cleared"] = bool(m.group(6))
    m = re.search(r"eba customs: song over at frame (\d+)", text)
    if m:
        r["song_frames"] = int(m.group(1))
        if r["last_note"] and r["cleared"]:            # (a failed stage ends where it failed)
            r["dead_s"] = (r["song_frames"] - r["last_note"]) / FPS
    if re.search(r"returned to|fatal|unhandled exception|undefined instruction", text):
        r["crashed"] = True
    p = r["problems"]
    if "eba customs:" not in text:
        first = text.strip().splitlines()[0] if text.strip() else "no output"
        p.append("the recomp did not start: " + first)
    elif r["crashed"]:
        p.append("the recomp stopped with an error before the results (see the log)")
    elif not r["loaded"]:
        p.append("the song scene never started: the custom did not load (see the log)")
    elif r["score"] is None:
        p.append("the song did not end within the time limit")
    if r["cleared"] is False:
        p.append("FAILED by a player that hits every note on time: the life gauge drains faster "
                 "than the chart refills it")
    if r["grades"]:
        g = r["grades"]
        if g[3]:
            p.append("%d notes missed by a perfect-timing player: notes too close together or "
                     "unreachable" % g[3])
        if g[1] or g[2]:
            p.append("%d notes judged 100 or 50 by a perfect-timing player: check sliders and "
                     "spinners" % (g[1] + g[2]))
    if r["dead_s"] is not None:
        if r["dead_s"] > DEAD_MAX_S:
            p.append("the chart ends %.0f s before the song (stock stages: 10 to 22 s): chart the "
                     "rest of the song, or trim the audio" % r["dead_s"])
        elif r["dead_s"] < DEAD_MIN_S:
            p.append("only %.1f s of song after the last note: the clear cutscene may be cut short"
                     % r["dead_s"])
    return r


def report_text(r):
    out = ["%s: %s" % (r["game"], "loaded" if r["loaded"] else "DID NOT LOAD")]
    if r["notes"]:
        out.append("  %d notes played, the last at %s" % (r["notes"], tc(r["last_note"])))
    if r["score"] is not None:
        g = r["grades"]
        out.append("  score %d (300 x%d, 100 x%d, 50 x%d, miss x%d), %s"
                   % (r["score"], g[0], g[1], g[2], g[3], "cleared" if r["cleared"] else "FAILED"))
    if r["song_frames"] is not None:
        out.append("  the %s at %s%s" % (
            "song ended" if r["cleared"] else "stage failed", tc(r["song_frames"]),
            "" if r["dead_s"] is None else ", %.0f s after the last note" % r["dead_s"]))
    out.append("  " + ("no problems found" if not r["problems"] else "problems:"))
    out += ["   - " + p for p in r["problems"]]
    return "\n".join(out)
