"""EBA's live notes in a main RAM dump (ndsrun's main.bin): find the note
objects, walk their list, print their fields; and who points at a value.

    python tools/eba_notes.py build/eba_mem/main.bin                 # the notes
    python tools/eba_notes.py build/eba_mem/main.bin --refs 0216ACE4  # words equal to it
    python tools/eba_notes.py DUMP --raw 0216ACE4 0x60                # words from an address
"""
import argparse
import struct

BASE = 0x02000000
NOTE_VTABLE = 0x020B4648    # tap notes (found 2026-10-22); sliders and spinners may have their own


def u32(m, a):
    return struct.unpack_from("<I", m, a - BASE)[0]


def objects_by_vtable(m, vtables=None):
    """Every word-aligned address in main RAM holding one of the vtables (or any
    0x020Bxxxx value followed by a plausible note layout when none given)."""
    out = []
    for o in range(0, len(m) - 0x44, 4):
        v = struct.unpack_from("<I", m, o)[0]
        if vtables is not None and v not in vtables:
            continue
        if vtables is None and not (0x020B0000 <= v < 0x020C0000):
            continue
        a = BASE + o
        x, y = u32(m, a + 0x18), u32(m, a + 0x1C)
        t = u32(m, a + 0x14)
        if vtables is None and not (x < 256 and y < 192 and (t >> 16) >= (t & 0xFFFF)):
            continue
        out.append(a)
    return out


def show(m, a):
    w = [u32(m, a + 4 * i) for i in range(0x44 // 4)]
    t = w[5]
    print("%08X vt %08X  #%d b1 %02X  +8 %04X/%d  prev %08X next %08X  appear %d hit %d  x %d y %d  +20 %X  %s" % (
        a, w[0], w[1] >> 24, (w[1] >> 8) & 0xFF, w[2] & 0xFFFF, w[2] >> 16, w[3], w[4], t & 0xFFFF, t >> 16,
        w[6], w[7], w[8], " ".join("%08X" % x for x in w[9:])))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dump")
    ap.add_argument("--refs", help="hex value: every word in RAM equal to it")
    ap.add_argument("--raw", nargs=2, metavar=("ADDR", "LEN"))
    ap.add_argument("--any-vtable", action="store_true", help="note-shaped objects of any 0x020Bxxxx vtable")
    a = ap.parse_args()
    m = open(a.dump, "rb").read()
    if a.refs:
        v = int(a.refs, 16)
        for o in range(0, len(m) - 3, 4):
            if struct.unpack_from("<I", m, o)[0] == v:
                print("%08X" % (BASE + o))
        return
    if a.raw:
        s, n = int(a.raw[0], 16), int(a.raw[1], 0)
        for i in range(0, n, 16):
            print("%08X: %s" % (s + i, " ".join("%08X" % u32(m, s + i + 4 * j) for j in range(4))))
        return
    objs = objects_by_vtable(m, None if a.any_vtable else {NOTE_VTABLE})
    for o in objs:
        show(m, o)
    print("%d objects" % len(objs))


if __name__ == "__main__":
    main()
