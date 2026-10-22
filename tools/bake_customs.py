"""Customs' assets, for the Customs screen and the HD sound of Customs mode
(src/eba_customs.cpp, src/eba_customs_ui.cpp), into customs/<tag>/ (Ouendan 2:
customs_oe2/<tag>/) and its custom.ini:

  stage_card.bmp       the replaced stage's card as the game shows it (its upper
                       screen: its art, title, song), from the stock cartridge
  preview_original.wav that card's preview music, as the game plays it
  preview_custom.wav   the custom's preview: its card's music found in the HD audio
  art.bmp              placeholder art for the custom
  hd.wav               the custom's song at its source's quality (ffmpeg), with
  hd_gain              the HD song's gain to sit in the game's mix as the game's own song does
  hd_offset            the HD time (s) of the start of the audio the custom was built from,
  clock0, clock_rate   the song clock (games.cpp's) at that start, and its frames a second:
                       HD time = hd_offset + (clock - clock0) / clock_rate

The measures run the game's ndsrun headless: the menu route to the first
stage's card on the pristine save, the stage's index set as the card opens; and
Customs mode's own route into the song, with its clock logged and its sound
recorded. The HD source must be the very edit the custom was built from: it is
checked across the whole song, and a custom whose source strays plays the
game's own sound.

    python tools/bake_customs.py [--game OE2] TAG [TAG ...]
"""
import argparse
import json
import os
import subprocess
import sys
import wave

import numpy as np
from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
WORK = os.path.join(ROOT, "build", "bake")
RH = r"F:\dxvk_remix\Claude\EBAHD Old\rom_hacking"
TESTRUN = RH + r"\testrun01"
TESTRUN2 = RH + r"\ouendan2\testrunouendan2"
PATCHED = RH + r"\patched_roms\Elite Beat Agents (FG_%s).diffs.json"
FPS = 59.8261                               # VBlanks a second
RATE = 32728                                # ndsrun's WAV rate

GAMES = {
    "EBA": dict(
        exe=os.path.join(ROOT, "build", "recomp", "b", "ndsrun.exe"), rom=os.path.join(ROOT, "roms", "EBA.nds"),
        customs=os.path.join(ROOT, "customs"),
        # to The Anthem's card; the session's stage (*0x020CD248 + 8) set as it opens
        route="950:tap:128:150,1300:tap:128:66,2450:tap:236:178,3650:tap:126:103", poke=(0x020DDCF0, 3662),
        card_shot=4250, preview=(3700, 4900), skip=5000, song_until=9600,
        # each custom's source, the exact edit it was built from at its own quality; and that edit at 15 kHz
        hd={"SS02_Movies": "Alien Ant Farm Movies v2.mp3",
            "SS05_AllStar": "SS05_AllStarFixedEnding.mp3",
            "SS09_LifeIsAHighway": "SS09_LifeIsAHighway.mp3",
            "SS10_ComeAndGet": "SS10_ComeAndGet.mp3",
            "SS11_GirlsJustWanna": "Girls Just Want to Have Fun.mp3"},
        built=lambda tag: os.path.join(TESTRUN, os.path.basename(json.load(open(PATCHED % tag))["wav"])),
        hd_dir=TESTRUN),
    "OE2": dict(
        exe=os.path.join(ROOT, "build", "oe2", "recomp", "b", "ndsrun.exe"), rom=os.path.join(ROOT, "roms", "OE2.nds"),
        customs=os.path.join(ROOT, "customs_oe2"),
        # to Samurai Blue's card (the map's book); the session's stage (*0x020A7A88 + 8) set as it opens
        route="1850:tap:128:150,2650:tap:65:55,3350:tap:238:183,4350:tap:238:178", poke=(0x020D7104, 4364),
        card_shot=4740, preview=(4420, 5620), skip=5200, song_until=10400,
        hd={"OS02_Hohoemi": "OSO2_Hohoemi.mp3",
            "OS03_Madder": "madder.mp3",
            "OS05_Takedown": r"C:\Users\behon\Downloads\Huntrix - Takedown (Lyrics)   K-Pop Demon Hunters.mp3",
            "OSB02_IWantYou": "OMB02_IWantYou.mp3"},
        built=lambda tag: os.path.join(TESTRUN2, {
            "OS02_Hohoemi": "OSO2_Hohoemi_15401.wav",
            "OS03_Madder": "madder_15401.wav",
            "OS05_Takedown": "Huntrix - Takedown (Lyrics)   K-Pop Demon Hunters_15401.wav",
            "OSB02_IWantYou": "OMB02_IWantYou_15401.wav"}[tag]),
        hd_dir=TESTRUN2),
}
G = GAMES["EBA"]
CUSTOMS = G["customs"]


def env():
    e = dict(os.environ)
    e["PATH"] = r"C:\msys64\ucrt64\bin;" + e["PATH"]
    return e


def ini_read(tag):
    vals, files = {}, []
    with open(os.path.join(CUSTOMS, tag, "custom.ini"), encoding="utf-8") as f:
        for line in f:
            k, _, v = line.rstrip("\n").partition("=")
            if k == "file":
                files.append(v)
            elif k:
                vals[k] = v
    return vals, files


def ini_write(tag, vals, files):
    with open(os.path.join(CUSTOMS, tag, "custom.ini"), "w", encoding="utf-8", newline="\n") as f:
        for k, v in vals.items():
            f.write("%s=%s\n" % (k, v))
        for x in files:
            f.write("file=%s\n" % x)


def replace_args(tag, files):
    out = []
    for name in files:
        out += ["--replace-file", "%s=%s" % (name, os.path.join(CUSTOMS, tag, "files", name.replace("/", "__")))]
    return out


def run(out, args, vblanks):
    os.makedirs(out, exist_ok=True)
    with open(os.path.join(CUSTOMS, "pristine.sav"), "rb") as s, open(os.path.join(out, "s.sav"), "wb") as d:
        d.write(s.read())
    cmd = [G["exe"], G["rom"], "--out", out, "--save", os.path.join(out, "s.sav"), "--vblanks", str(vblanks), "--no-dump"] + args
    with open(os.path.join(out, "log.txt"), "w") as log:
        subprocess.run(cmd, stdout=log, stderr=subprocess.STDOUT, env=env(), check=True)
    return open(os.path.join(out, "log.txt"), encoding="utf-8", errors="replace").read()


def read_wav(path):
    with wave.open(path) as w:
        rate, ch, n = w.getframerate(), w.getnchannels(), w.getnframes()
        a = np.frombuffer(w.readframes(n), dtype=np.int16).astype(np.float32).reshape(-1, ch)
    return a, rate


def write_wav(path, a, rate):
    a = np.clip(a, -32768, 32767).astype(np.int16)
    with wave.open(path, "wb") as w:
        w.setnchannels(a.shape[1])
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(a.tobytes())


def mono(a, rate, to=8000):
    m = a.mean(axis=1)
    t = np.arange(0, len(m) / rate, 1 / to)
    return np.interp(t, np.arange(len(m)) / rate, m).astype(np.float32)


def lag(hay, needle, to=8000):
    """Where (s) `needle` (mono at `to` Hz) starts in `hay`, and how well (0-1)."""
    n = 1 << int(np.ceil(np.log2(len(hay) + len(needle))))
    c = np.fft.irfft(np.fft.rfft(hay, n) * np.conj(np.fft.rfft(needle, n)), n)[:len(hay)]
    i = int(np.argmax(c))
    seg = hay[i:i + len(needle)]
    score = float(np.dot(seg, needle[:len(seg)]) / (np.linalg.norm(seg) * np.linalg.norm(needle[:len(seg)]) + 1e-9))
    return i / to, score


def stage_card(tag, vals, files):
    """The replaced stage's card and preview from the stock cartridge, and the custom's preview."""
    idx = int(vals["stage_index"])
    addr, at = G["poke"]
    p0, p1 = G["preview"]
    common = ["--input", G["route"], "--port", "poke=%08X:%d@%d" % (addr, idx, at), "--port", "customs-dir=" + CUSTOMS]
    out = os.path.join(WORK, tag, "card_stock")
    shot = G["card_shot"]
    run(out, common + ["--shots", str(shot), "--wav", os.path.join(out, "a.wav")], p1)
    img = Image.open(os.path.join(out, "v%d.png" % shot)).crop((0, 0, 256, 192))
    img.save(os.path.join(CUSTOMS, tag, "stage_card.bmp"))
    a, rate = read_wav(os.path.join(out, "a.wav"))
    write_wav(os.path.join(CUSTOMS, tag, "preview_original.wav"), a[int(p0 / FPS * rate):int(p1 / FPS * rate)], rate)
    out = os.path.join(WORK, tag, "card_custom")
    run(out, common + replace_args(tag, files) + ["--wav", os.path.join(out, "a.wav")], p1)
    c, crate = read_wav(os.path.join(out, "a.wav"))
    return c[int(p0 / FPS * crate):int(p1 / FPS * crate)], crate


def hd_audio(tag, vals, files, custom_preview):
    src = os.path.join(G["hd_dir"], G["hd"][tag])
    hd = os.path.join(CUSTOMS, tag, "hd.wav")
    subprocess.run(["ffmpeg", "-v", "error", "-y", "-i", src, "-ac", "2", "-sample_fmt", "s16", hd], check=True)
    h, hrate = read_wav(hd)
    hm = mono(h, hrate)
    # the audio the custom was built from, found in the HD file
    b, brate = read_wav(G["built"](tag))
    bm = mono(b, brate)
    hd_offset, q1 = lag(hm, bm[:8000 * 40])
    # the same edit all along: every 20 s of the built audio where the offset puts it in the HD file
    strays = []
    for k in range(0, int(len(bm) / 8000) - 10, 20):
        seg = bm[8000 * k:8000 * (k + 8)]
        at = int((hd_offset + k) * 8000)
        hs = hm[at:at + len(seg)]
        n = min(len(seg), len(hs))
        q = float(np.dot(seg[:n], hs[:n]) / (np.linalg.norm(seg[:n]) * np.linalg.norm(hs[:n]) + 1e-9)) if n else 0.0
        if q < 0.6:
            strays.append((k, round(q, 2)))
    if q1 < 0.6 or strays:
        print("  %s is not the edit the custom was built from (start match %.2f; strays at %s s): no HD audio"
              % (os.path.basename(G["hd"][tag]), q1, strays))
        os.remove(hd)
        for key in ("hd", "hd_offset", "clock0", "clock_rate", "hd_gain"):
            vals.pop(key, None)
        p, prate = custom_preview
        write_wav(os.path.join(CUSTOMS, tag, "preview_custom.wav"), p, prate)
        vals.update(art="art.bmp", stage_card="stage_card.bmp", preview_original="preview_original.wav",
                    preview_custom="preview_custom.wav")
        return
    # the preview the custom's card plays (VBlank 3700 on), found in the HD file: the best of a few
    # windows (its first moments cross-fade from the map's music; later it loops)
    p, prate = custom_preview
    pm = mono(p, prate)
    tries = []
    for s in np.arange(0.5, 6.0, 0.5):
        at, q = lag(hm, pm[int(s * 8000):int((s + 5) * 8000)])
        tries.append((q, at - s))
    q2, pv_at = max(tries)
    if q2 < 0.5:        # the card plays no part of the custom (its preview entry kept the stock one): the song's start
        print("  the custom's card preview is not in its song (best match %.2f): the preview starts the song" % q2)
        pv_at = 0.0
    n = int(len(p) / prate * hrate)
    start = max(0, int(pv_at * hrate))
    write_wav(os.path.join(CUSTOMS, tag, "preview_custom.wav"), h[start:start + n], hrate)
    # Customs mode's route into the song: the clock against the sound it plays
    out = os.path.join(WORK, tag, "song")
    # (the button mode's test player keeps the song going to its end)
    log = run(out, replace_args(tag, files) + ["--input", "%d:PAD_SOUTH:3" % G["skip"], "--port", "customs-dir=" + CUSTOMS,
                                               "--port", "custom=" + tag, "--port", "difficulty=n",
                                               "--port", "campaign-save=none", "--port", "clock-log",
                                               "--port", "buttons-auto", "--wav", os.path.join(out, "a.wav")], G["song_until"])
    clocks = np.array([[int(x) for x in l.split()[-2:]] for l in log.splitlines() if "eba clock" in l], dtype=float)
    g, grate = read_wav(os.path.join(out, "a.wav"))
    gm = mono(g, grate)
    # where each 5 s of the built audio plays in the run's sound, and the clock then: a line through the good ones
    pts = []
    for k in range(0, 60, 5):
        t, q = lag(gm, bm[8000 * k:8000 * (k + 6)])
        if q > 0.5:
            pts.append((k, float(np.interp(t * FPS, clocks[:, 0], clocks[:, 1]))))
    if len(pts) < 3:
        raise SystemExit("%s: the song's clock could not be measured (%d windows matched)" % (tag, len(pts)))
    # the HD song's level against the game's own (same moments of the song): its gain in the mix
    t0, _ = lag(gm, bm[: 8000 * 30])
    ds = gm[int(t0 * 8000):int((t0 + 30) * 8000)]
    hs = hm[int(hd_offset * 8000):int((hd_offset + 30) * 8000)]
    gain = float(np.sqrt(np.mean(ds ** 2)) / (np.sqrt(np.mean(hs ** 2)) + 1e-9))
    vals["hd_gain"] = "%.3f" % min(gain, 1.0)   # the run's hits raise its level: never above unity
    k, c = np.array(pts).T
    rate, clock0 = np.polyfit(k, c, 1)
    resid = float(np.abs(c - (clock0 + rate * k)).max())
    vals.update(art="art.bmp", stage_card="stage_card.bmp", preview_original="preview_original.wav",
                preview_custom="preview_custom.wav", hd="hd.wav", hd_offset="%.4f" % hd_offset,
                clock0="%.2f" % clock0, clock_rate="%.4f" % rate)
    print("  HD %s %d Hz: built audio at %.3f s of it (match %.2f); preview from %.2f s (%.2f); clock0 %.2f,"
          " %.4f frames/s over %d points (worst %.2f frames off); gain %s" % (
              os.path.basename(G["hd"][tag]), hrate, hd_offset, q1, pv_at, q2, clock0, rate, len(pts), resid, vals["hd_gain"]))


def art(tag, vals):
    """Placeholder art: the title and artist on a band of colour of its own."""
    hue = (sum(map(ord, tag)) * 47) % 360
    img = Image.new("RGB", (256, 192))
    d = ImageDraw.Draw(img)
    for y in range(192):
        c = Image.new("HSV", (1, 1), (hue * 255 // 360, 170, 110 + y * 120 // 192)).convert("RGB").getpixel((0, 0))
        d.line([(0, y), (255, y)], fill=c)
    for i in range(-192, 256, 24):
        d.line([(i, 192), (i + 192, 0)], fill=(255, 255, 255), width=1)
    try:
        big, small = ImageFont.truetype("arialbd.ttf", 26), ImageFont.truetype("arial.ttf", 15)
    except OSError:
        big = small = ImageFont.load_default()
    d.rectangle([0, 12, 256, 34], fill=(20, 20, 20))
    d.text((128, 23), "CUSTOM  -  %s" % vals["stage"], font=small, fill=(255, 210, 60), anchor="mm")
    words, lines, cur = vals["title"].split(), [], ""
    for w in words:
        if len(cur + " " + w) > 16 and cur:
            lines.append(cur)
            cur = w
        else:
            cur = (cur + " " + w).strip()
    lines.append(cur)
    y = 100 - 16 * (len(lines) - 1)
    for line in lines:
        d.text((128, y), line, font=big, fill=(255, 255, 255), anchor="mm", stroke_width=3, stroke_fill=(0, 0, 0))
        y += 32
    d.text((128, 168), vals["artist"], font=small, fill=(255, 255, 255), anchor="mm", stroke_width=2,
           stroke_fill=(0, 0, 0))
    img.save(os.path.join(CUSTOMS, tag, "art.bmp"))


def main():
    global G, CUSTOMS
    ap = argparse.ArgumentParser()
    ap.add_argument("tags", nargs="+")
    ap.add_argument("--game", default="EBA", choices=sorted(GAMES))
    a = ap.parse_args()
    G = GAMES[a.game]
    CUSTOMS = G["customs"]
    for tag in a.tags:
        print(tag)
        vals, files = ini_read(tag)
        art(tag, vals)
        preview = stage_card(tag, vals, files)
        hd_audio(tag, vals, files, preview)
        ini_write(tag, vals, files)


if __name__ == "__main__":
    main()
