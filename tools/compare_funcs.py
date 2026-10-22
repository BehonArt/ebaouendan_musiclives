"""Compare two directories of discovery function lists (ENTRY<TAB>SIZE<TAB>A|T,
from `ndskit.recomp.discover --out`), as ndskit asks of every discovery
change: no function lost or split, what grew and what is new.

    python tools/compare_funcs.py build/discover/before build/discover/after
"""
import os
import sys


def load(path):
    out = {}
    for line in open(path):
        e, size, mode = line.split()
        out[int(e, 16)] = (int(size), mode)
    return out


def main():
    before_dir, after_dir = sys.argv[1], sys.argv[2]
    bad = 0
    for name in sorted(os.listdir(before_dir)):
        if not name.endswith(".tsv"):
            continue
        b = load(os.path.join(before_dir, name))
        a = load(os.path.join(after_dir, name))
        lost = sorted(set(b) - set(a))
        new = sorted(set(a) - set(b))
        # split: a new entry inside a function that existed before
        split = [n for n in new if any(e < n < e + s for e, (s, _) in b.items())]
        grew = sorted(e for e in set(a) & set(b) if a[e][0] > b[e][0])
        shrank = sorted(e for e in set(a) & set(b) if a[e][0] < b[e][0])
        flag = "" if not (lost or split or shrank) else "   <-- CHECK"
        bad += bool(flag)
        print("%-16s %5d -> %5d  lost %d  new %d (split %d)  grew %d  shrank %d%s" % (
            name, len(b), len(a), len(lost), len(new), len(split), len(grew), len(shrank), flag))
        for e in grew:
            print("    grew   %08X  %d -> %d bytes" % (e, b[e][0], a[e][0]))
        for e in new:
            print("    new    %08X  %d bytes%s" % (e, a[e][0], "  (split)" if e in split else ""))
        for e in lost:
            print("    lost   %08X" % e)
        for e in shrank:
            print("    shrank %08X  %d -> %d bytes" % (e, b[e][0], a[e][0]))
    print("programs needing a look: %d" % bad)


if __name__ == "__main__":
    main()
