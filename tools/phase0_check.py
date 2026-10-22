"""Phase 0 check: run ndskit discovery on one program and report what the
first pass left open (refuse passes, unresolved calls vs jumps) and whether
the addresses found by hand in the EBA/OE RE work are function entries or
lie inside the expected function.

    python tools/phase0_check.py roms/EBA.nds --game eba [--cpu 9] [--overlay N]
"""
import argparse
import bisect
import collections
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from ndskit.rom import Rom
from ndskit.recomp.discover import Memory, Program, segments_from_rom, overlay_seeds, overlay_areas

# entries found by hand (name -> address)
ENTRIES = {
    "eba": {
        "fade ramp": 0x02028E20, "brightness primitive": 0x020320B8, "slam": 0x02054518,
        "idle fn": 0x02054318, "scheduler tick": 0x02059C20, "segment-done": 0x02058340,
        "segment-chain": 0x02057C70, "exit-event": 0x02082E70, "stage-complete": 0x02052374,
        "gameplay update": 0x02082FAC, "player tick": 0x0205D0E4, "segment-length load": 0x02059998,
        "fallback fn": 0x02054DB8, "brightness helper": 0x02004BC8, "sleep": 0x02008C30,
        # gameplay vtable at 0x020B65A0
        "vt0": 0x020823F0, "vt1": 0x020824A4, "vt2": 0x02082614, "vt3": 0x0208268C, "vt4": 0x02082FAC,
        "vt5": 0x020525E8, "vt6": 0x02083530, "vt7": 0x02052458, "vt8": 0x02083684, "vt9": 0x02082924,
    },
    "oe2": {"ramp": 0x0202C080, "brightness primitive": 0x0203610C, "slam": 0x02056C40},
    "oe1": {"ramp": 0x0205EB14, "brightness primitive": 0x0205F4E8, "slam": 0x02026BD4,
            "brightness helper": 0x020044E0},
}

# sites inside functions (name -> (site, expected containing entry))
SITES = {
    "eba": {"clear latch": (0x0205D228, 0x0205D0E4), "idle site": (0x020543D8, 0x02054318)},
    "oe2": {"clear latch": (0x02060798, 0x02060658)},
    "oe1": {"clear latch": (0x02029B60, 0x02029A40)},
}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("rom")
    ap.add_argument("--game", choices=sorted(ENTRIES), required=True)
    ap.add_argument("--cpu", type=int, choices=(7, 9), default=9)
    ap.add_argument("--overlay", type=int)
    a = ap.parse_args()

    rom = Rom(open(a.rom, "rb").read())
    segs, entry = segments_from_rom(rom, a.cpu, a.overlay)
    mem = Memory(segs, arch=5 if a.cpu == 9 else 4)
    seeds = [entry]
    if a.overlay is not None:
        seeds += overlay_seeds(rom, a.cpu, a.overlay, mem)
    prog = Program(mem, seeds)
    if a.overlay is not None:
        prog.areas = overlay_areas(rom, a.cpu, a.overlay)
    prog.run()
    text, conf, unres = prog.summary()
    print(text)
    print("refuse passes %d, refused entries %d" % (prog.refuse_passes, len(prog.refused)))

    kinds = collections.Counter(what.split()[0] for _, (_, what) in unres)
    ops = collections.Counter(" ".join(what.split()[:2]) for _, (_, what) in unres)
    print("unresolved by kind:", dict(kinds))
    print("unresolved by op:  ", dict(ops.most_common(12)))
    jumps = [(e, at, what) for e, (at, what) in unres if what.startswith("jump")]
    by_fn = collections.Counter(e for e, _, _ in jumps)
    print("jumps in %d functions; most: %s" % (
        len(by_fn), ", ".join("%08X x%d" % kv for kv in by_fn.most_common(8))))

    if a.cpu != 9 or a.overlay is not None:
        return
    starts = sorted(prog.funcs)

    def owner(addr):
        i = bisect.bisect_right(starts, addr) - 1
        while i >= 0:
            f = prog.funcs[starts[i]]
            if addr in f.code:
                return starts[i]
            i -= 1
            if starts[i + 1] - addr > 0x4000:
                break
        return None

    miss = 0
    for name, ad in ENTRIES[a.game].items():
        ok = ad in prog.funcs
        miss += not ok
        print("  %-22s %08X  %s" % (name, ad, "entry" if ok else "MISSING (in %s)" % (
            "%08X" % owner(ad) if owner(ad) else "nothing")))
    for name, (site, want) in SITES[a.game].items():
        got = owner(site)
        ok = got == want
        miss += not ok
        print("  %-22s %08X  in %s%s" % (name, site, "%08X" % got if got else "nothing",
                                        "" if ok else "  (expected %08X)" % want))
    print("hand-found addresses: %d checked, %d wrong" % (
        len(ENTRIES[a.game]) + len(SITES[a.game]), miss))


if __name__ == "__main__":
    main()
