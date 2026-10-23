"""The Studio's .exe made to run as python.exe does: no Control Flow Guard (the
OE2 charts' decompressor runs the game's own code in unicorn, whose JIT-made
code CFG stops with a fail-fast, 0xC0000409) and python.exe's stack (3 MB).
PyInstaller's launcher has CFG on; python.exe has it off.

    python patch_exe.py EXE
"""
import struct
import sys

p = sys.argv[1]
d = bytearray(open(p, "rb").read())
opt = struct.unpack_from("<I", d, 0x3C)[0] + 24
assert struct.unpack_from("<H", d, opt)[0] == 0x20B, "not a PE32+ image"
dc = struct.unpack_from("<H", d, opt + 70)[0]
struct.pack_into("<H", d, opt + 70, dc & ~0x4000)          # IMAGE_DLLCHARACTERISTICS_GUARD_CF
struct.pack_into("<Q", d, opt + 72, max(3000000, struct.unpack_from("<Q", d, opt + 72)[0]))
open(p, "wb").write(d)
print("%s: DllCharacteristics %#x -> %#x, stack %d" % (p, dc, dc & ~0x4000, struct.unpack_from("<Q", d, opt + 72)[0]))
