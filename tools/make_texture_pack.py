"""Texture packs for EBA's 3D (ndskit/runtime/texpack.cpp): pictures named by the
hash of the DS texture they replace, at 8x its size.

    python tools/make_texture_pack.py hd-rings OUT [--dump DIR]
        sharp approach circles: the game's three rings redrawn, smooth, where they are
    python tools/make_texture_pack.py skin SKIN_DIR OUT [--dump DIR]
        an osu! skin's hit objects (visuals only): circles in the combo colours with
        their overlay and numbers, the approach circle, slider ball, reverse arrows,
        the glow ring, judgements, the spinner

The DS textures come from a dump (ndsrun --port tex-dump=DIR, The Anthem played
through: build/texdump2_n2), read for where each part sits in its texture, so a
replacement lands exactly where the original's texels were. Exact hashes
(<hash>.png) for the circle atlases (one per combo colour), shape hashes
(s<hash>.png, any palette) for the rest, t<hash>.png for the glow ring (tinted by
the colour the game cycles it through). Nothing of the game is in a pack.
"""
import argparse
import configparser
import math
import os

import numpy as np
from PIL import Image, ImageChops

S = 8                                                   # a pack's pictures: 8x the DS texture
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# EBA (USA): the textures of a song, from dumps of The Anthem on all four difficulties
ATLASES = ["9b8ed31f14bcc877", "ed94593c4721f57b", "c7bf988add93700e", "d54cfaaf85797701"]   # 4x8 cells of 32
APPROACH = ["bfc0096910488515", "40239154a3f1c691", "bb1d0471b237cda1"]                      # shapes: white rings
GLOW = "0adcc3f5bc9b8d6b"                               # shape: the orange ring, its palette cycling
SLIDERBALL = "cc509ce1216889ef"                         # 4 frames of 32
JUDGEMENTS = "c55ca51015d68ff1"                         # 300, 100, 50, miss in cells of 64
MISS = "4d50b15f43fd8e85"
SPINNER = "0c1fa0b3eb60bd5f"
SPIN = "20bf856d5b687f6f"
LIFEBAR = ["0ec7e029992bd7fb", "a0a30d9776046f10", "13b4c2330acd0315", "f59404e8a94ac33e"]   # (eba_play.cpp hides them)


def dump_index(dump):
    """hash -> (shape, its picture); shape -> a picture of it."""
    by_hash, by_shape = {}, {}
    for line in open(os.path.join(dump, "index.txt")):
        p = line.split()
        png = os.path.join(dump, p[0] + ".png")
        shape = p[-1][1:]
        by_hash[p[0]] = (shape, png)
        by_shape.setdefault(shape, png)
    return by_hash, by_shape


def alpha(img):
    return np.asarray(img.convert("RGBA"))[:, :, 3].astype(np.float32) / 255


def ring_fit(img):
    """A ring's centre, mid radius and thickness (texels), from its opaque texels."""
    a = alpha(img)
    ys, xs = np.nonzero(a > 0.5)
    cx, cy = xs.mean() + 0.5, ys.mean() + 0.5
    r = np.hypot(xs + 0.5 - cx, ys + 0.5 - cy)
    return cx, cy, float(np.median(r)), float(r.max() - r.min()) + 1


def opaque_box(img):
    a = alpha(img)
    ys, xs = np.nonzero(a > 0.5)
    return xs.min(), ys.min(), xs.max() + 1, ys.max() + 1


def image_ring(img):
    """A ring picture's mid radius, as a fraction of its half width (its opaque pixels' median distance)."""
    a = alpha(img)
    h, w = a.shape
    ys, xs = np.nonzero(a > 0.5)
    r = np.hypot(xs + 0.5 - w / 2, ys + 0.5 - h / 2)
    return float(np.median(r)) / (w / 2)


def canvas(w, h):
    return Image.new("RGBA", (w * S, h * S), (0, 0, 0, 0))


def paste_centered(dst, src, cx, cy, size):
    """src scaled to size x size (pixels of dst) and centred at (cx, cy)."""
    s = src.resize((max(1, round(size)), max(1, round(size))), Image.LANCZOS)
    dst.alpha_composite(s, (round(cx - s.width / 2), round(cy - s.height / 2)))


def paste_fit(dst, src, box):
    """src fitted into box (x0, y0, x1, y1 of dst), its shape kept, centred."""
    x0, y0, x1, y1 = box
    k = min((x1 - x0) / src.width, (y1 - y0) / src.height)
    s = src.resize((max(1, round(src.width * k)), max(1, round(src.height * k))), Image.LANCZOS)
    dst.alpha_composite(s, (round((x0 + x1 - s.width) / 2), round((y0 + y1 - s.height) / 2)))


def tint(img, rgb):
    r, g, b = (c / 255 for c in rgb)
    a = np.asarray(img.convert("RGBA")).astype(np.float32)
    a[:, :, 0] *= r
    a[:, :, 1] *= g
    a[:, :, 2] *= b
    return Image.fromarray(a.clip(0, 255).astype(np.uint8), "RGBA")


def hd_ring(out_size, cx, cy, radius, thickness, colour=(255, 255, 255)):
    """A smooth ring (pixels of the output)."""
    w, h = out_size
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32) + 0.5
    d = np.abs(np.hypot(xx - cx, yy - cy) - radius) - thickness / 2
    a = np.clip(0.5 - d, 0, 1)                            # one pixel of edge
    px = np.zeros((h, w, 4), np.uint8)
    px[:, :, :3] = colour
    px[:, :, 3] = (a * 255).astype(np.uint8)
    return Image.fromarray(px, "RGBA")


def write(out, name, img):
    os.makedirs(out, exist_ok=True)
    img.save(os.path.join(out, name + ".png"))


def hd_rings(dump, out):
    _, by_shape = dump_index(dump)
    for shape in APPROACH:
        ds = Image.open(by_shape[shape])
        cx, cy, r, t = ring_fit(ds)
        write(out, "s" + shape, hd_ring((ds.width * S, ds.height * S), cx * S, cy * S, r * S, t * S))
    print("hd-rings: %d rings in %s" % (len(APPROACH), out))


class Skin:
    def __init__(self, d):
        self.d = d
        ini = configparser.ConfigParser(strict=False, delimiters=(":",))
        ini.optionxform = str
        try:
            ini.read_string(open(os.path.join(d, "skin.ini"), encoding="utf-8-sig", errors="replace").read())
        except Exception:
            pass
        cols = ini["Colours"] if ini.has_section("Colours") else {}
        self.combos = [tuple(int(v) for v in cols[k].split(",")[:3]) for k in sorted(cols) if k.startswith("Combo")]
        if not self.combos:
            self.combos = [(255, 192, 0), (0, 202, 0), (18, 124, 255), (242, 24, 57)]
        fonts = ini["Fonts"] if ini.has_section("Fonts") else {}
        self.prefix = fonts.get("HitCirclePrefix", "default").strip()
        self.overlap = int(fonts.get("HitCircleOverlap", "-2"))

    def get(self, name):
        """name's picture, @2x if the skin has it (its scale: 2 for @2x, 1 else), or None."""
        for n, k in ((name + "@2x.png", 2), (name + ".png", 1)):
            p = os.path.join(self.d, n)
            if os.path.exists(p):
                return Image.open(p).convert("RGBA"), k
        return None, 1


def number(skin, n, k_out):
    """The skin's digits of n, side by side as osu! draws them (k_out: output pixels per @1x pixel)."""
    digits = []
    for ch in str(n):
        img, k = skin.get("%s-%s" % (skin.prefix, ch))
        if img is None:
            return None
        digits.append(img.resize((max(1, round(img.width * k_out / k)), max(1, round(img.height * k_out / k))), Image.LANCZOS))
    ov = round(skin.overlap * k_out)
    w = sum(d.width for d in digits) - ov * (len(digits) - 1)
    h = max(d.height for d in digits)
    out = Image.new("RGBA", (max(1, w), h), (0, 0, 0, 0))
    x = 0
    for d in digits:
        out.alpha_composite(d, (x, (h - d.height) // 2))
        x += d.width - ov
    return out


def skin_pack(skin_dir, dump, out):
    skin = Skin(skin_dir)
    by_hash, by_shape = dump_index(dump)
    circle, kc = skin.get("hitcircle")
    overlay, _ = skin.get("hitcircleoverlay")
    arrow, _ = skin.get("reversearrow")
    made = 0

    # the circle atlases: 4 x 8 cells of 32 texels, one per combo colour
    for i, h in enumerate(ATLASES):
        ds = Image.open(by_hash[h][1]).convert("RGBA")
        body = np.asarray(ds.crop((8, 136, 24, 152)))[:, :, :3].reshape(-1, 3).mean(0)   # a plain cell's middle
        colour = min(skin.combos, key=lambda c: sum((c[j] - body[j]) ** 2 for j in range(3))) if skin.combos else (255, 255, 255)
        hd = canvas(ds.width, ds.height)
        cell = 32 * S
        # the skin's circle as big as the DS's (its opaque radius, of the cell)
        r_ds = 0.949
        r_skin = max(0.5, float(np.max(np.hypot(*[g - circle.width / 2 for g in np.nonzero(alpha(circle) > 0.5)[::-1]]))) / (circle.width / 2))
        size = cell * r_ds / r_skin
        k_out = size / circle.width * kc                   # output pixels per skin @1x pixel
        for row in range(8):
            for col in range(4):
                x0, y0 = col * cell, row * cell
                c = x0 + cell / 2, y0 + cell / 2
                n = row * 4 + col
                if row == 7 and col < 3:                   # the reverse arrows, as big as the DS's
                    box = opaque_box(ds.crop((col * 32, row * 32, col * 32 + 32, row * 32 + 32)))
                    if arrow is not None:
                        paste_fit(hd, arrow, tuple(v * S + o for v, o in zip(box, (x0, y0, x0, y0))))
                    continue
                paste_centered(hd, tint(circle, colour), *c, size)
                if overlay is not None:
                    paste_centered(hd, overlay, *c, size)
                if row < 4 and n > 0:
                    num = number(skin, n, k_out)
                    if num is not None:
                        hd.alpha_composite(num, (round(c[0] - num.width / 2), round(c[1] - num.height / 2)))
        write(out, h, hd)
        made += 1

    # the approach circle's three rings: the skin's ring as wide as each
    ring, _ = skin.get("approachcircle")
    if ring is not None:
        r_skin = image_ring(ring)
        for shape in APPROACH:
            ds = Image.open(by_shape[shape])
            cx, cy, r, t = ring_fit(ds)
            hd = canvas(ds.width, ds.height)
            paste_centered(hd, ring, cx * S, cy * S, 2 * r * S / r_skin)
            write(out, "s" + shape, hd)
            made += 1

    # the glow ring: the skin's follow circle, made white to take the game's cycling colour
    glow, _ = skin.get("sliderfollowcircle")
    if glow is None:
        glow, _ = skin.get("sliderfollowcircle-0")
    if glow is not None and GLOW in by_shape:
        ds = Image.open(by_shape[GLOW])
        cx, cy, r, t = ring_fit(ds)
        a = np.asarray(glow).astype(np.float32)
        lum = a[:, :, :3].max(axis=2, keepdims=True)
        lum = lum / max(1.0, float(lum.max())) * 255
        white = np.concatenate([np.repeat(lum, 3, axis=2), a[:, :, 3:]], axis=2).astype(np.uint8)
        hd = canvas(ds.width, ds.height)
        paste_centered(hd, Image.fromarray(white, "RGBA"), cx * S, cy * S, 2 * r * S / image_ring(glow))
        write(out, "t" + GLOW, hd)
        made += 1

    # the slider ball: 4 frames of 32, the skin's turning ball
    if SLIDERBALL in by_shape:
        ds = Image.open(by_shape[SLIDERBALL]).convert("RGBA")
        frames = [skin.get("sliderb%d" % i)[0] for i in range(10)]
        frames = [f for f in frames if f is not None] or [skin.get("sliderb")[0]]
        if frames[0] is not None:
            hd = canvas(ds.width, ds.height)
            for f in range(4):
                box = opaque_box(ds.crop((f * 32, 0, f * 32 + 32, 32)))
                pic = frames[f * len(frames) // 4]
                paste_fit(hd, pic, tuple(v * S + o for v, o in zip(box, (f * 32 * S, 0, f * 32 * S, 0))))
            write(out, "s" + SLIDERBALL, hd)
            made += 1

    # the judgements (cells of 64) and the miss mark, each in the DS's own box
    for shape, names, cell_w in ((JUDGEMENTS, ["hit300", "hit100", "hit50", "hit0"], 64), (MISS, ["hit0"], 32),
                                 (SPINNER, ["spinner-circle"], 128), (SPIN, ["spinner-spin"], 64)):
        if shape not in by_shape:
            continue
        ds = Image.open(by_shape[shape]).convert("RGBA")
        hd = canvas(ds.width, ds.height)
        ok = False
        for j, nm in enumerate(names):
            pic, _ = skin.get(nm)
            if pic is None:
                continue
            x0 = j * cell_w
            box = opaque_box(ds.crop((x0, 0, x0 + cell_w, ds.height)))
            pic = pic.crop(pic.getbbox() or (0, 0, pic.width, pic.height))
            paste_fit(hd, pic, tuple(v * S + o for v, o in zip(box, (x0 * S, 0, x0 * S, 0))))
            ok = True
        if ok:
            write(out, "s" + shape, hd)
            made += 1
    print("skin %s: %d pictures in %s" % (os.path.basename(skin_dir.rstrip("\\/")), made, out))


def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    a = sub.add_parser("hd-rings")
    a.add_argument("out")
    a.add_argument("--dump", default=os.path.join(ROOT, "build", "texdump2_n2"))
    b = sub.add_parser("skin")
    b.add_argument("skin")
    b.add_argument("out")
    b.add_argument("--dump", default=os.path.join(ROOT, "build", "texdump2_n2"))
    args = ap.parse_args()
    if args.cmd == "hd-rings":
        hd_rings(args.dump, args.out)
    else:
        skin_pack(args.skin, args.dump, args.out)


if __name__ == "__main__":
    main()
