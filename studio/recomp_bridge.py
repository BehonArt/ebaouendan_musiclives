"""What Customs Studio adds to the chart editor: the recomp's side of a custom.

A custom ROM the editor built is turned into a custom's folder of the recomp
(customs/<tag>/: custom.ini and files/, the files that differ from the stock
cartridge, by name), which the recomp plays in Customs mode; and into a BPS
patch of the stock ROM, the way to share a custom without sharing the game.

    make_custom(built_rom, stock_rom, out_dir, title, artist)   -> its folder
    launch_test(game, custom_dir, difficulty, autoplay, recomp)  -> the run
    make_bps(stock_rom, built_rom, out_path)                     -> the patch
"""
import os
import shutil
import time
import struct
import subprocess
import zlib

import numpy as np

# each game: its game code, its story's stages by index (the session's), its play folder in the recomp
GAMES = {
    "EBA": dict(code=b"AOSE", folder="play", exe="EBA.exe",
                stages=["SS01", "SS02", "SS06", "SS04", "SS05", "SS09", "SS10", "SS03", "SS08", "SS11", "SS13",
                        "SS12", "SSB03", "SS07", "SS15A", "SS15B", "SSB02", "SSB01", "SS14"]),
    "OE2": dict(code=b"AO2J", folder="play_oe2", exe="OE2.exe",
                stages=["OS01", "OS02", "OS04", "OS06", "OS08", "OS10", "OSB02", "OS11", "OS03", "OS07", "OS14",
                        "OS12", "OS13", "OS05", "OS15A", "OS15B", "OSB03", "OSB01", "OS09"]),
    "OE1": dict(code=b"AOSJ", folder="play_oe1", exe="OE1.exe",
                stages=["s%02d" % n for n in range(1, 16)]),
}


def game_of(rom_path):
    with open(rom_path, "rb") as f:
        f.seek(0xC)
        code = f.read(4)
    for g, d in GAMES.items():
        if d["code"] == code:
            return g
    raise ValueError("%s is none of the three games (code %r)" % (os.path.basename(rom_path), code))


class RomFiles:
    """A cartridge's files by name, from its FNT and FAT."""

    def __init__(self, path):
        self.data = open(path, "rb").read()
        u32 = lambda o: struct.unpack_from("<I", self.data, o)[0]
        fnt, fat_off, fat_size = u32(0x40), u32(0x48), u32(0x4C)
        self.fat = [(u32(fat_off + 8 * i), u32(fat_off + 8 * i + 4)) for i in range(fat_size // 8)]
        self.names = [""] * len(self.fat)
        self._walk(fnt, 0xF000, "")

    def _walk(self, fnt, d, prefix):
        e = fnt + (d & 0xFFF) * 8
        sub, first = struct.unpack_from("<IH", self.data, e)
        p, fid = fnt + sub, first
        while self.data[p]:
            n = self.data[p] & 0x7F
            is_dir = self.data[p] & 0x80
            name = self.data[p + 1:p + 1 + n].decode("latin-1")
            p += 1 + n
            if is_dir:
                (sub_id,) = struct.unpack_from("<H", self.data, p)
                p += 2
                self._walk(fnt, sub_id, prefix + name + "/")
            else:
                if fid < len(self.names):
                    self.names[fid] = prefix + name
                fid += 1

    def file(self, i):
        a, b = self.fat[i]
        return self.data[a:b]


def make_custom(built_rom, stock_rom, out_dir, title="", artist="", tag=None, difficulties="enhx", art=None,
                hd=None):
    """The custom's folder (customs/<tag>) from a ROM the editor built. difficulties: those charted (of
    e n h x); the recomp offers only these (a custom without all four cannot join a full conversion).
    hd: the song's own audio file at full quality, played in Customs mode in place of the ROM's
    15401 Hz copy (write_hd); the result's hd_note says what came of it."""
    difficulties = "".join(d for d in "enhx" if d in (difficulties or ""))
    if not difficulties:
        raise ValueError("tick at least one difficulty as charted")
    game = game_of(built_rom)
    if game_of(stock_rom) != game:
        raise ValueError("the built ROM and the stock ROM are different games")
    stock, mine = RomFiles(stock_rom), RomFiles(built_rom)
    if len(stock.fat) != len(mine.fat):
        raise ValueError("the built ROM's file table is not the stock one's")
    stages = GAMES[game]["stages"]
    changed, stage = [], ""
    for i, name in enumerate(stock.names):
        if not name or stock.file(i) == mine.file(i):
            continue
        changed.append(i)
        if name.startswith("data/seq/") and name.endswith(".seq_"):
            stem = name[9:].split("_")[0]
            if stem in stages:
                stage = stem
    if not changed:
        raise ValueError("the built ROM changes nothing of the stock one")
    if not stage:
        raise ValueError("the built ROM changes no story stage's chart: the recomp's Customs play story stages")
    title = title or os.path.splitext(os.path.basename(built_rom))[0]
    tag = tag or (stage + "_" + "".join(c if c.isalnum() or c in "-_" else "_" for c in title))
    d = os.path.join(out_dir, tag)
    for _ in range(40):                                   # this tag's again: the new build replaces it
        if not os.path.exists(d):                         # (Windows finishes deleting a big folder a moment later)
            break
        shutil.rmtree(d, ignore_errors=True)
        time.sleep(0.25)
    else:
        raise RuntimeError("could not replace %s: is a game running from it? Close it and try again" % d)
    os.makedirs(os.path.join(d, "files"))
    lines = ["title=" + title, "artist=" + artist, "stage=" + stage, "stage_index=%d" % stages.index(stage),
             "replaces=" + stage, "difficulties=" + difficulties,
             "made_by=Customs Studio from " + os.path.basename(built_rom)]
    for i in changed:
        name = stock.names[i]
        with open(os.path.join(d, "files", name.replace("/", "__")), "wb") as f:
            f.write(mine.file(i))
        lines.append("file=" + name)
    if art:
        lines.append("art=" + write_art(art, d))
    hd_note = ""
    if hd:
        try:
            vals, hd_note = write_hd(hd, mine, stage, d)
            lines += ["%s=%s" % kv for kv in vals.items()]
        except Exception as e:                            # the custom all the same, with the game's own sound
            hd_note = "the game's own sound (full-quality audio: %s)" % e
    with open(os.path.join(d, "custom.ini"), "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines) + "\n")
    return dict(game=game, tag=tag, dir=d, stage=stage, files=len(changed), hd_note=hd_note)


# ---- full-quality song audio (2026-11-13) -------------------------------------------------------
# The ROM carries the song at 15401 Hz ADPCM, the DS's own format; a custom can carry the song's
# own file as well (hd.wav), which Customs mode plays in its place, held to the song clock
# (src/eba_customs.cpp, hd_frame): HD time = hd_offset + (clock - clock0) / clock_rate.
# The song clock is the stream's playback time (ms x 60 / 1000: docs/DECOMP_FINDINGS.md §19), so
# clock_rate is 60 and clock0 the clock's frame of lag (tools/bake_customs.py measured 0.2 to 3.5
# on 14 customs by running the game, their lines 1.5 frames from 60 x t on average over the 55 s
# they measured: 1.5 here). hd_offset is where the ROM's song starts in the file,
# found by correlating the two; it matched bake_customs' measures on all of them (0.98 to 0.99).

def _mono_8k(a, rate):
    m = a.mean(axis=1) if a.ndim == 2 else a
    t = np.arange(0, len(m) / rate, 1 / 8000)
    return np.interp(t, np.arange(len(m)) / rate, m).astype(np.float32)


def _lag(hay, needle, pad=8000 * 30):
    """Where (s) needle starts in hay (both mono 8 kHz), and how well (0-1); up to 30 s before
    hay's start (the ROM's song can open with silence the file doesn't have)."""
    hay = np.concatenate([np.zeros(pad, np.float32), hay])
    n = 1 << int(np.ceil(np.log2(len(hay) + len(needle))))
    c = np.fft.irfft(np.fft.rfft(hay, n) * np.conj(np.fft.rfft(needle, n)), n)[:len(hay)]
    i = int(np.argmax(c))
    seg = hay[i:i + len(needle)]
    q = float(np.dot(seg, needle[:len(seg)]) / (np.linalg.norm(seg) * np.linalg.norm(needle[:len(seg)]) + 1e-9))
    return (i - pad) / 8000, q


def _wav_info(path):
    """(rate, channels, sample width) of a WAV's header, or None."""
    import wave
    try:
        with wave.open(path, "rb") as w:
            return w.getframerate(), w.getnchannels(), w.getsampwidth()
    except Exception:
        return None


def _read_pcm16(path):
    """(samples x channels float32, rate) of a 16-bit PCM WAV."""
    import wave
    with wave.open(path, "rb") as w:
        n, ch, rate = w.getnframes(), w.getnchannels(), w.getframerate()
        a = np.frombuffer(w.readframes(n), np.int16).reshape(-1, ch).astype(np.float32)
    return a, rate


def rom_song(rom_files, stage):
    """The stage's song as the ROM plays it: mono float32 at its rate (15401), its streams at
    their places in the chart (seconds from the song clock's 0)."""
    import ndspy.soundArchive
    import stage_length as SL
    from seq_codec import SeqFile
    from strm_tool import decode_strm
    names = {n: i for i, n in enumerate(rom_files.names)}
    seq = SeqFile.fromRaw(SL.decompress(rom_files.file(names["data/seq/%s_n.seq_" % stage]))[0])
    streams = dict(ndspy.soundArchive.SDAT(rom_files.file(names["data/sound_data.sdat"])).streams)
    placed, rate = [], 15401
    for t, sid in seq.markers:
        s = streams.get("STRM_%d" % sid) if sid is not None else None
        if s is None:
            continue
        rate, ch = decode_strm(s)
        placed.append((t, np.mean([np.asarray(c, np.float32) for c in ch], axis=0)))
    if not placed:
        raise ValueError("the ROM's %s has no song stream" % stage)
    total = max(int(t / 60.0 * rate) + len(m) for t, m in placed)
    mix = np.zeros(total, np.float32)
    for t, m in placed:
        off = int(t / 60.0 * rate)
        mix[off:off + len(m)] = m
    return mix, rate


def write_hd(src, rom_files, stage, custom_dir):
    """hd.wav in the custom's folder from src (WAV, FLAC, MP3, OGG...; all but a 16-bit WAV need
    ffmpeg), lined up with the ROM's song. Returns ({custom.ini key: value}, a note for the author);
    raises ValueError when src is not the song the ROM was built with."""
    if not os.path.isfile(src):
        raise ValueError("not found: " + src)
    out = os.path.join(custom_dir, "hd.wav")
    info = _wav_info(src)
    if info and 15401 < info[0] <= 48000 and info[1] <= 2 and info[2] == 2:
        shutil.copyfile(src, out)                         # as it is
    else:
        try:
            r = subprocess.run(["ffmpeg", "-v", "error", "-y", "-i", src, "-ac", "2", "-sample_fmt", "s16",
                                "-ar", str(min(info[0], 48000)) if info else "44100", out],
                               capture_output=True, text=True, timeout=300,
                               creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
            ok = r.returncode == 0
        except FileNotFoundError:
            ok = False
        if not ok:                                        # no ffmpeg: the toolkit's built-in reader
            try:
                import audio_io
                a, r_in = audio_io.decode(src)
                rate_out = min(r_in, 48000)
                audio_io.write_wav16(out, audio_io.channels(audio_io.resample(a, r_in, rate_out), 2), rate_out)
            except Exception as e:
                raise ValueError("could not read %s (%s): install ffmpeg (in a terminal: winget install "
                                 "Gyan.FFmpeg), or choose a 16-bit WAV" % (os.path.basename(src), e))
    a, rate = _read_pcm16(out)
    if rate <= 15401:
        os.remove(out)
        raise ValueError("%s is %d Hz, no better than the ROM's own" % (os.path.basename(src), rate))
    b, brate = rom_song(rom_files, stage)
    bm, hm = _mono_8k(b, brate), _mono_8k(a, rate)
    off, q = _lag(hm, bm[:8000 * 40])
    strays = []                                           # the same edit all along: every 20 s
    for k in range(0, int(len(bm) / 8000) - 10, 20):
        seg = bm[8000 * k:8000 * (k + 8)]
        best = None                                       # the best of a few samples either side (bright songs
        for d in range(-3, 4):                            # correlate poorly one 8 kHz sample out)
            at = int(round((off + k) * 8000)) + d
            if at < 0:
                continue
            hs = hm[at:at + 8000 * 8]
            n = min(len(seg), len(hs))
            if n < 8000 or np.linalg.norm(seg[:n]) < 1 or np.linalg.norm(hs[:n]) < 1:   # past an end, or silence
                continue
            qk = float(np.dot(seg[:n], hs[:n]) / (np.linalg.norm(seg[:n]) * np.linalg.norm(hs[:n]) + 1e-9))
            best = qk if best is None else max(best, qk)
        if best is not None and best < 0.6:
            strays.append(k)
    if q < 0.6 or strays:
        os.remove(out)
        raise ValueError("%s is not the song this ROM was built with (%s)" % (
            os.path.basename(src), "start match %.2f" % q if q < 0.6 else "it differs at %s s" % strays[:4]))
    # its level against the ROM's (the author's volume settings): 30 s from 5 s into the song
    a0 = max(0, int(round(-off * 8000))) + 8000 * 5
    bs = bm[a0:a0 + 8000 * 30]
    hs = hm[int(round(off * 8000)) + a0:int(round(off * 8000)) + a0 + len(bs)]
    n = min(len(bs), len(hs))
    gain = float(np.sqrt(np.mean(bs[:n] ** 2)) / (np.sqrt(np.mean(hs[:n] ** 2)) + 1e-9)) if n > 8000 else 1.0
    gain = min(max(gain, 0.05), 4.0)
    vals = {"hd": "hd.wav", "hd_offset": "%.4f" % off, "clock0": "1.50", "clock_rate": "60.0000",
            "hd_gain": "%.3f" % gain}
    return vals, "full-quality song (%s, %d Hz)" % (os.path.basename(src), rate)


def hd_guess(chart_wav):
    """The song's own file behind the WAV a chart was built with: that WAV when it is better than
    15401 Hz, else its source beside it (the editor's converter writes NAME_15401.wav beside NAME.mp3;
    the MIDI import NAME_hq.wav). '' when there is none."""
    import re
    if not chart_wav:
        return ""
    base = os.path.splitext(chart_wav)[0]
    stem = re.sub(r"_15401(_lead\d+f)?$", "", base)   # (_leadNf: the copy padded for the stage's intro)
    for c in [chart_wav, base + "_hq.wav", stem + "_hq.wav"] + \
             [stem + e for e in (".flac", ".wav", ".mp3", ".ogg", ".m4a", ".opus", ".aac")]:
        if not os.path.isfile(c):
            continue
        if c.lower().endswith(".wav"):
            info = _wav_info(c)
            if info and info[0] <= 15401:
                continue
        return c
    return ""


def write_art(src, custom_dir, max_side=512):
    """A custom's cover art: the picture the author chose (PNG, JPG, BMP, WebP, GIF), as
    art.bmp in the custom's folder, which the Customs menu shows in place of the plain
    title card. A 24-bit BMP no larger than max_side: the game reads BMP (SDL_LoadBMP),
    v1.0.0 and v1.0.1 included. Returns the file's name for custom.ini's art= line."""
    from PIL import Image
    if not os.path.isfile(src):
        raise ValueError("cover art not found: " + src)
    try:
        im = Image.open(src)
        im.seek(0)                                        # an animation's first frame
        if im.mode in ("RGBA", "LA", "P"):                # see-through parts on black, like the menu
            im = im.convert("RGBA")
            bg = Image.new("RGBA", im.size, (0, 0, 0, 255))
            im = Image.alpha_composite(bg, im)
        im = im.convert("RGB")
    except Exception as e:
        raise ValueError("could not read the cover art (%s): %s" % (os.path.basename(src), e))
    im.thumbnail((max_side, max_side), Image.LANCZOS)
    im.save(os.path.join(custom_dir, "art.bmp"), "BMP")
    return "art.bmp"


def data_root(recomp_root):
    """Where the release keeps the player's files: %APPDATA%\\eba-recomp, or userdata beside it when portable."""
    if os.path.exists(os.path.join(recomp_root, "portable.txt")):
        return os.path.join(recomp_root, "userdata")
    return os.path.join(os.environ.get("APPDATA", os.path.expanduser("~")), "eba-recomp")


def launcher_rom(recomp_root, game):
    """The dump chosen for a game in the launcher (launcher.ini), or "" if none."""
    try:
        with open(os.path.join(data_root(recomp_root), "Launcher", "launcher.ini"), encoding="utf-8") as f:
            for line in f:
                if line.startswith("rom_%s=" % game):
                    return line.split("=", 1)[1].strip()
    except OSError:
        pass
    return ""


def recomp_folder(recomp_root, game):
    """A game's exe, its stock ROM, its save and its customs. The release: the exes together, the dump
    chosen in the launcher, the player's files in the data folder. A development tree: its play folders."""
    g = GAMES[game]
    if os.path.exists(os.path.join(recomp_root, g["exe"])):
        data = os.path.join(data_root(recomp_root), game)
        return dict(dir=recomp_root, exe=os.path.join(recomp_root, g["exe"]), rom=launcher_rom(recomp_root, game),
                    save=os.path.join(data, game + ".sav"), customs=os.path.join(data, "customs"))
    d = os.path.join(recomp_root, g["folder"])
    exe = os.path.join(d, g["exe"])
    if not os.path.exists(exe):
        raise FileNotFoundError("no %s in %s (tools\\make_play_folder.ps1 -Game %s makes it)" % (g["exe"], d, game))
    return dict(dir=d, exe=exe, rom=os.path.join(d, game + ".nds"), save=os.path.join(d, game + ".sav"),
                customs=os.path.join(d, "customs"))


def ensure_start_save(rf, customs_dir):
    """customs/pristine.sav: the save Customs starts from. The game makes it itself (its first boot,
    headless, --port customs-make-start-save) when the folder has none; a copy from the play
    folder's customs when that has one already. Returns its path (raises when it cannot be made)."""
    pristine = os.path.join(customs_dir, "pristine.sav")
    if os.path.exists(pristine) and os.path.getsize(pristine) > 0:
        return pristine
    have = os.path.join(rf["customs"], "pristine.sav")
    if os.path.exists(have) and os.path.getsize(have) > 0 and os.path.abspath(rf["customs"]) != os.path.abspath(customs_dir):
        shutil.copyfile(have, pristine)
        return pristine
    os.makedirs(customs_dir, exist_ok=True)
    work = os.path.join(customs_dir, "first-boot")
    os.makedirs(work, exist_ok=True)
    args = [rf["exe"], rf["rom"], "--save", os.path.join(work, "parent.sav"), "--out", work, "--vblanks", "30000",
            "--no-dump", "--port", "customs-dir=" + customs_dir, "--port", "customs-make-start-save"]
    with open(os.path.join(work, "parent.log"), "w") as log:
        subprocess.run(args, cwd=rf["dir"], stdout=log, stderr=subprocess.STDOUT,
                       creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    if not (os.path.exists(pristine) and os.path.getsize(pristine) > 0):
        raise RuntimeError("the game could not make its start save (see %s)" % os.path.join(work, "parent.log"))
    return pristine


def launch_test(game, custom, difficulty, autoplay, recomp_root, work_dir):
    """The recomp in Customs mode on this custom: its own run, the customs' known save, the stage at once."""
    rf = recomp_folder(recomp_root, game)
    customs = os.path.dirname(custom["dir"])
    pristine = ensure_start_save(rf, customs)            # the save Customs starts from (the game makes it)
    os.makedirs(work_dir, exist_ok=True)
    session = os.path.join(work_dir, "session.sav")
    shutil.copyfile(pristine, session)
    args = [rf["exe"], rf["rom"], "--window", "--save", session, "--out", work_dir,
            "--port", "customs-dir=" + customs, "--port", "custom=" + custom["tag"],
            "--port", "difficulty=" + difficulty, "--port", "campaign-save=" + rf["save"]]
    if autoplay:
        args += ["--port", "autoplay=1"]
    with open(os.path.join(custom["dir"], "custom.ini"), encoding="utf-8") as f:
        for line in f:
            if line.startswith("file="):
                name = line[5:].strip()
                args += ["--replace-file", "%s=%s" % (name, os.path.join(custom["dir"], "files", name.replace("/", "__")))]
    log = open(os.path.join(work_dir, "recomp.log"), "w")
    flags = getattr(subprocess, "CREATE_NO_WINDOW", 0)
    subprocess.Popen(args, cwd=rf["dir"], stdout=log, stderr=subprocess.STDOUT, creationflags=flags)
    return dict(exe=rf["exe"], log=os.path.join(work_dir, "recomp.log"))


# ---- BPS (beat's patch format, as Flips and most patchers apply it) ----------------------
def _num(n):
    out = bytearray()
    while True:
        x = n & 0x7F
        n >>= 7
        if n == 0:
            out.append(0x80 | x)
            return bytes(out)
        out.append(x)
        n -= 1


_P = np.array([0x9E3779B185EBCA87, 0xC2B2AE3D27D4EB4F, 0x165667B19E3779F9, 0x85EBCA77C2B2AE63,
               0x27D4EB2F165667C5, 0x94D049BB133111EB, 0xBF58476D1CE4E5B9, 0xFF51AFD7ED558CCD], dtype=np.uint64)


def _block_keys(words, starts):
    """A 64-bit key of each 64-byte block (8 words) starting at the word indices given."""
    k = np.zeros(len(starts), dtype=np.uint64)
    with np.errstate(over="ignore"):
        for j in range(8):
            k += words[starts + j] * _P[j]
    return k


def make_bps(stock_rom, built_rom, out_path, note=""):
    """A BPS patch from the stock ROM to the built one, small: the builder moves files and
    shifts what lies inside the sound archive, so every 64-byte block of the built ROM (at
    32-byte steps) is looked for anywhere in the stock ROM (an index of its blocks at 32-byte
    steps), each match grown both ways and copied from there (SourceRead in place, SourceCopy
    elsewhere); only what matches nothing is carried in the patch (TargetRead)."""
    src_raw, dst_raw = open(stock_rom, "rb").read(), open(built_rom, "rb").read()
    src = np.frombuffer(src_raw, dtype=np.uint8)
    dst = np.frombuffer(dst_raw, dtype=np.uint8)
    sw = np.frombuffer(src_raw[:len(src_raw) // 8 * 8], dtype=np.uint64)
    dw = np.frombuffer(dst_raw[:len(dst_raw) // 8 * 8], dtype=np.uint64)
    # the stock ROM's blocks: their keys sorted, each one's first place
    s_starts = np.arange(0, len(sw) - 8, 4, dtype=np.int64)
    s_keys = _block_keys(sw, s_starts)
    order = np.argsort(s_keys, kind="stable")
    s_keys, s_pos = s_keys[order], s_starts[order] * 8
    # the built ROM's blocks and where each is in the stock ROM (-1: nowhere)
    d_starts = np.arange(0, len(dw) - 8, 4, dtype=np.int64)
    d_keys = _block_keys(dw, d_starts)
    i = np.minimum(np.searchsorted(s_keys, d_keys), len(s_keys) - 1)
    found = np.where(s_keys[i] == d_keys, s_pos[i], -1)
    # prefer the same place where the block is there too (a SourceRead)
    t_pos = d_starts * 8
    here = np.zeros(len(t_pos), dtype=bool)
    ok = t_pos + 64 <= len(src)
    here[ok] = (sw[d_starts[ok]] == dw[d_starts[ok]])
    out = bytearray(b"BPS1" + _num(len(src)) + _num(len(dst)) + _num(len(note.encode())) + note.encode())

    def extend(t, s):                                     # how far dst[t:] and src[s:] agree
        n, step = 0, 1 << 16
        while t + n < len(dst) and s + n < len(src):
            m = min(step, len(dst) - t - n, len(src) - s - n)
            neq = np.flatnonzero(dst[t + n:t + n + m] != src[s + n:s + n + m])
            if len(neq):
                return n + int(neq[0])
            n += m
        return n

    cur, rel = 0, 0                                       # all before cur is written; SourceCopy's offset
    k = 0
    while k < len(t_pos):
        k = int(np.searchsorted(t_pos, cur))
        if k >= len(t_pos):
            break
        t = int(t_pos[k])
        s = t if here[k] else int(found[k])
        if s < 0 or dst_raw[t:t + 64] != src_raw[s:s + 64]:
            cur_k = k + 1                                 # no match: on to the next block, the bytes left pending
            while cur_k < len(t_pos):
                s2 = int(t_pos[cur_k]) if here[cur_k] else int(found[cur_k])
                if s2 >= 0:
                    break
                cur_k += 1
            if cur_k >= len(t_pos):
                break
            # the pending bytes stay pending: look on from that block without moving cur
            t = int(t_pos[cur_k])
            s = t if here[cur_k] else int(found[cur_k])
            if dst_raw[t:t + 64] != src_raw[s:s + 64]:
                # a false key: write the bytes up to it as literal, so the search moves on
                out.extend(_num(((t + 1 - cur) - 1) << 2 | 1) + dst[cur:t + 1].tobytes())
                cur = t + 1
                continue
        back = 0                                          # grown back over the bytes not yet written
        while t - back > cur and s - back > 0 and dst_raw[t - back - 1] == src_raw[s - back - 1]:
            back += 1
        t0, s0 = t - back, s - back
        if t0 > cur:
            out.extend(_num(((t0 - cur) - 1) << 2 | 1) + dst[cur:t0].tobytes())
        n = back + extend(t, s)
        if s0 == t0:
            out.extend(_num((n - 1) << 2 | 0))
        else:
            off = s0 - rel
            out.extend(_num((n - 1) << 2 | 2) + _num(abs(off) << 1 | (1 if off < 0 else 0)))
            rel = s0 + n
        cur = t0 + n
    if cur < len(dst):
        out.extend(_num(((len(dst) - cur) - 1) << 2 | 1) + dst[cur:].tobytes())
    out += struct.pack("<I", zlib.crc32(src_raw) & 0xFFFFFFFF)
    out += struct.pack("<I", zlib.crc32(dst_raw) & 0xFFFFFFFF)
    out += struct.pack("<I", zlib.crc32(bytes(out)) & 0xFFFFFFFF)
    with open(out_path, "wb") as f:
        f.write(out)
    return dict(path=out_path, size=len(out))


def apply_bps(patch_path, source_rom):
    """The patched ROM (a check of make_bps: SourceRead, TargetRead, SourceCopy)."""
    p = open(patch_path, "rb").read()
    src = open(source_rom, "rb").read()
    pos = [4]

    def num():
        data, shift = 0, 1
        while True:
            x = p[pos[0]]
            pos[0] += 1
            data += (x & 0x7F) * shift
            if x & 0x80:
                return data
            shift <<= 7
            data += shift

    rel = [0]
    assert p[:4] == b"BPS1"
    ssize, tsize, msize = num(), num(), num()
    pos[0] += msize
    out = bytearray()
    while pos[0] < len(p) - 12:
        d = num()
        kind, length = d & 3, (d >> 2) + 1
        if kind == 0:
            out += src[len(out):len(out) + length]
        elif kind == 1:
            out += p[pos[0]:pos[0] + length]
            pos[0] += length
        elif kind == 2:
            d2 = num()
            rel[0] += (-1 if d2 & 1 else 1) * (d2 >> 1)
            out += src[rel[0]:rel[0] + length]
            rel[0] += length
        else:
            raise ValueError("TargetCopy is not used by make_bps")
    assert len(out) == tsize and zlib.crc32(bytes(out)) & 0xFFFFFFFF == struct.unpack_from("<I", p, len(p) - 8)[0]
    return bytes(out)
