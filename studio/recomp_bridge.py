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


def make_custom(built_rom, stock_rom, out_dir, title="", artist="", tag=None):
    """The custom's folder (customs/<tag>) from a ROM the editor built."""
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
    if os.path.exists(d):
        shutil.rmtree(d)                                  # this tag's again: the new build replaces it
    os.makedirs(os.path.join(d, "files"))
    lines = ["title=" + title, "artist=" + artist, "stage=" + stage, "stage_index=%d" % stages.index(stage),
             "replaces=" + stage, "made_by=Customs Studio from " + os.path.basename(built_rom)]
    for i in changed:
        name = stock.names[i]
        with open(os.path.join(d, "files", name.replace("/", "__")), "wb") as f:
            f.write(mine.file(i))
        lines.append("file=" + name)
    with open(os.path.join(d, "custom.ini"), "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines) + "\n")
    return dict(game=game, tag=tag, dir=d, stage=stage, files=len(changed))


def recomp_folder(recomp_root, game):
    """The recomp's play folder of a game: its exe, its stock ROM, its save, its customs."""
    g = GAMES[game]
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
