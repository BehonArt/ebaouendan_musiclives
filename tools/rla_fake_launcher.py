"""Stands in for RecompLauncher on Windows (Recomp Achievements' channel: a named pipe, a JSON
object per line), for testing the port's events headless. The SDK's own sdk/tests/fake_launcher.py
is the same for Unix sockets.

    python tools/rla_fake_launcher.py OUT.jsonl EXPECT -- ndsrun.exe ROM --save ... --port ...

Runs the program with RL_SESSION set to a fresh pipe, answers its hello with "quiet" as the
launcher does, and writes every message it sends to OUT.jsonl. EXPECT: the event names that must
arrive, comma separated, with !NAME for one that must not ("-" for none; "offline": the program
must not connect at all). The program's output goes to OUT.log. Exits 0 when they are as
expected. Windows only (ctypes, no other modules).
"""
import ctypes
import json
import os
import subprocess
import sys
import threading
from ctypes import wintypes

k32 = ctypes.WinDLL("kernel32", use_last_error=True)
k32.CreateNamedPipeW.restype = wintypes.HANDLE
k32.CreateNamedPipeW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD, wintypes.DWORD,
                                 wintypes.DWORD, wintypes.DWORD, wintypes.DWORD, wintypes.LPVOID]
k32.ConnectNamedPipe.argtypes = [wintypes.HANDLE, wintypes.LPVOID]
k32.ReadFile.argtypes = [wintypes.HANDLE, wintypes.LPVOID, wintypes.DWORD, ctypes.POINTER(wintypes.DWORD), wintypes.LPVOID]
k32.WriteFile.argtypes = [wintypes.HANDLE, wintypes.LPCVOID, wintypes.DWORD, ctypes.POINTER(wintypes.DWORD), wintypes.LPVOID]
k32.CloseHandle.argtypes = [wintypes.HANDLE]
INVALID = wintypes.HANDLE(-1).value


def main():
    if "--" not in sys.argv or sys.argv.index("--") != 3:
        print(__doc__)
        return 2
    out_path, expect, cmd = sys.argv[1], sys.argv[2], sys.argv[4:]
    name = r"\\.\pipe\RecompLauncher-test-%d" % os.getpid()
    # duplex, byte mode, one instance, 64 KiB buffers
    pipe = k32.CreateNamedPipeW(name, 0x00000003, 0x00000000, 1, 65536, 65536, 0, None)
    if pipe == INVALID:
        print("CreateNamedPipe failed", ctypes.get_last_error())
        return 2
    got = []

    def serve():
        k32.ConnectNamedPipe(pipe, None)
        buf = ctypes.create_string_buffer(65536)
        pending = b""
        while True:
            n = wintypes.DWORD()
            if not k32.ReadFile(pipe, buf, len(buf), ctypes.byref(n), None) or n.value == 0:
                break
            pending += buf.raw[:n.value]
            while b"\n" in pending:
                line, pending = pending.split(b"\n", 1)
                m = json.loads(line)
                got.append(m)
                if m.get("t") == "hello":
                    w = wintypes.DWORD()
                    k32.WriteFile(pipe, b'{"t":"quiet"}\n', 14, ctypes.byref(w), None)

    t = threading.Thread(target=serve, daemon=True)
    t.start()
    proc = subprocess.run(cmd, env=dict(os.environ, RL_SESSION=name), stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    t.join(3)
    if t.is_alive():            # never connected: connect once ourselves so the wait for a client ends
        k32.CreateFileW.restype = wintypes.HANDLE
        k32.CreateFileW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD, wintypes.LPVOID,
                                    wintypes.DWORD, wintypes.DWORD, wintypes.HANDLE]
        h = k32.CreateFileW(name, 0xC0000000, 0, None, 3, 0, None)
        if h != INVALID:
            k32.CloseHandle(h)
        t.join(3)
    k32.CloseHandle(pipe)
    with open(out_path, "w", encoding="utf-8") as f:
        for m in got:
            f.write(json.dumps(m) + "\n")
    with open(os.path.splitext(out_path)[0] + ".log", "wb") as f:     # the program's own log
        f.write(proc.stdout)
    events =[m["name"] for m in got if m.get("t") == "event"]
    hello = next((m for m in got if m.get("t") == "hello"), None)
    print("program exit", proc.returncode, "| hello:", json.dumps(hello))
    print("events:", " ".join(events) or "(none)")
    if expect == "offline":
        ok = not got
        print("PASS" if ok else "FAIL: it connected")
        return 0 if ok else 1
    ok = hello is not None
    for e in [x for x in expect.split(",") if x and x != "-"]:
        if e.startswith("!"):
            if e[1:] in events:
                print("UNEXPECTED", e[1:]); ok = False
        elif e not in events:
            print("MISSING", e); ok = False
    if len(events) != len(set(events)):
        print("an event was sent twice"); ok = False
    print("PASS" if ok else "FAIL")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
