"""Customs Studio: the EBA / Ouendan chart editor as a companion app of eba-recomp.

The toolkit's editor (rom_hacking/tools/seq_editor: the Note Editor and the
Section Editor, with every rule of a chart that loads) runs as it is, in a
window of its own (Edge's app mode: no browser around it). The Studio adds a
bar to both editors for the recomp's side of a custom (recomp_bridge.py):

- Test in recomp: the custom just built, played in the recomp's Customs mode
  at once (its own run, the stage straight away, any difficulty, autoplay).
- Export custom folder: customs/<tag> for the recomp (its Customs list, or
  Import custom folder on another PC).
- Export patch: a BPS patch of the stock ROM, to share without the game.

    studio.exe                 (or: python studio.py)
    studio.exe --browser       (the default browser instead of a window)

Its settings (studio.ini beside it): the toolkit's tools folder and the
recomp's folder (play/, play_oe2/, play_oe1/ in it).
"""
import argparse
import configparser
import json
import os
import shutil
import socket
import subprocess
import sys
import tempfile
import threading
import time
import traceback
import urllib.parse
import webbrowser

FROZEN = getattr(sys, "frozen", False)


def python_mode():
    """The .exe as Python for the toolkit's own child processes: it runs its builders as
    [sys.executable, builder.py, ...] and its save dialog as [sys.executable, -c, CODE, ...]."""
    if not FROZEN or len(sys.argv) < 2:
        return
    import runpy
    if sys.argv[1] == "-c" and len(sys.argv) > 2:
        code = sys.argv[2]
        sys.argv = ["-c"] + sys.argv[3:]
        exec(compile(code, "<string>", "exec"), {"__name__": "__main__"})
        sys.exit(0)
    if sys.argv[1].lower().endswith(".py"):
        script = os.path.abspath(sys.argv[1])
        sys.argv = sys.argv[1:]
        sys.path.insert(0, os.path.dirname(script))
        runpy.run_path(script, run_name="__main__")
        sys.exit(0)


python_mode()
HERE = os.path.dirname(os.path.abspath(__file__))
BUNDLE = getattr(sys, "_MEIPASS", HERE)               # studio.js (bundled with the exe)
APP_DIR = os.path.dirname(sys.executable) if FROZEN else HERE
sys.path.insert(0, HERE)
import recomp_bridge as B  # noqa: E402

DEFAULTS = {
    "toolkit": r"F:\dxvk_remix\Claude\EBAHD Old\rom_hacking\tools",
    "recomp": os.path.dirname(APP_DIR) if FROZEN else os.path.dirname(HERE),
}


def load_config():
    path = os.path.join(APP_DIR, "studio.ini")
    c = configparser.ConfigParser()
    c["paths"] = DEFAULTS
    if os.path.exists(path):
        c.read(path, encoding="utf-8-sig")         # (a BOM, as PowerShell writes, is fine)
    else:
        with open(path, "w", encoding="utf-8") as f:
            f.write("# Customs Studio: where the toolkit's tools folder and the recomp are\n")
            c.write(f)
    return c["paths"], path


def work_dir():
    d = os.path.join(os.environ.get("LOCALAPPDATA", tempfile.gettempdir()), "eba-recomp", "studio")
    os.makedirs(d, exist_ok=True)
    return d


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", type=int, default=0)
    ap.add_argument("--browser", action="store_true", help="the default browser instead of an app window")
    ap.add_argument("--no-window", action="store_true", help="serve only (tests)")
    a = ap.parse_args()
    if FROZEN and sys.platform == "win32":
        # a console program (so the toolkit's builders, run by it, have their pipes): its own
        # console, made for it on a double click, is hidden; its log goes to a file
        import ctypes
        k32 = ctypes.windll.kernel32
        procs = (ctypes.c_uint * 4)()
        if k32.GetConsoleProcessList(procs, 4) <= 1 and not a.no_window:
            ctypes.windll.user32.ShowWindow(k32.GetConsoleWindow(), 0)
            sys.stdout = sys.stderr = open(os.path.join(work_dir(), "studio.log"), "w", encoding="utf-8", buffering=1)
    paths, ini = load_config()
    seq_editor = os.path.join(paths["toolkit"], "seq_editor")
    if not os.path.exists(os.path.join(seq_editor, "server.py")):
        sys.exit("Customs Studio: no seq_editor\\server.py in the toolkit folder %s (set it in %s)" % (paths["toolkit"], ini))
    sys.path.insert(0, seq_editor)
    os.chdir(paths["toolkit"])
    import server as S  # the toolkit's editor, as it is

    S._ensure_stock_caches()
    last = {}                                             # the last ROM built, by chart

    class Handler(S.Handler):
        def _json(self, obj, code=200):
            self._send(code, json.dumps(obj).encode())

        def do_GET(self):
            url = urllib.parse.urlparse(self.path)
            if url.path in ("/", "/section_editor"):     # the editors, with the Studio's bar
                page = "index.html" if url.path == "/" else "section_editor.html"
                html = open(os.path.join(seq_editor, page), encoding="utf-8").read()
                tag = '<script src="/studio/studio.js"></script>'
                html = html.replace("</body>", tag + "</body>") if "</body>" in html else html + tag
                self._send(200, html.encode("utf-8"), "text/html; charset=utf-8")
                return
            if url.path == "/studio/studio.js":
                self._send(200, open(os.path.join(BUNDLE, "studio.js"), "rb").read(), "application/javascript")
                return
            super().do_GET()

        def do_POST(self):
            if not self.path.startswith("/studio/api/"):
                super().do_POST()
                return
            try:
                n = int(self.headers.get("Content-Length", 0))
                data = json.loads(self.rfile.read(n) or b"{}")
                self._json(api(self.path[len("/studio/api/"):], data))
            except Exception as e:
                traceback.print_exc()
                self._json({"error": str(e)})

    def stock_rom(game):
        return {"EBA": S.CLEAN_ROM, "OE2": S.OE2_ROM, "OE1": S.OE1_ROM}[game]

    def api(name, d):
        if name == "built":                               # the page's build finished: its ROM
            last[d.get("chart", "")] = d["rom"]
            return {"ok": True}
        if name == "info":
            out = {"toolkit": paths["toolkit"], "recomp": paths["recomp"], "ini": ini, "games": {}}
            for g in B.GAMES:
                try:
                    out["games"][g] = B.recomp_folder(paths["recomp"], g)["exe"]
                except Exception as e:
                    out["games"][g] = None
            return out
        rom = d.get("rom") or last.get(d.get("chart", ""))
        if not rom or not os.path.exists(rom):
            raise ValueError("build the ROM first (Build ROM / Build all diffs), then this")
        game = B.game_of(rom)
        if name == "test":
            customs = os.path.join(work_dir(), "customs_" + game.lower())
            os.makedirs(customs, exist_ok=True)
            c = B.make_custom(rom, stock_rom(game), customs, d.get("title") or "Studio test", d.get("artist", ""),
                              tag="studio_test")
            r = B.launch_test(game, c, d.get("difficulty", "n"), bool(d.get("autoplay")), paths["recomp"],
                              os.path.join(work_dir(), "run_" + game.lower()))
            return dict(r, stage=c["stage"], game=game)
        if name == "validate":                            # the recomp plays it through headless: a report
            import validate as V
            customs = os.path.join(work_dir(), "customs_" + game.lower())
            os.makedirs(customs, exist_ok=True)
            c = B.make_custom(rom, stock_rom(game), customs, d.get("title") or "Studio test", d.get("artist", ""),
                              tag="studio_test")
            r = V.validate_custom(game, c, d.get("difficulty", "n"), paths["recomp"],
                                  os.path.join(work_dir(), "validate_" + game.lower()))
            return dict(r, text=V.report_text(r))
        if name == "export_custom":
            out = d.get("out_dir") or B.recomp_folder(paths["recomp"], game)["customs"]
            c = B.make_custom(rom, stock_rom(game), out, d.get("title", ""), d.get("artist", ""))
            if d.get("zip"):
                c["zip"] = shutil.make_archive(c["dir"], "zip", os.path.dirname(c["dir"]), c["tag"])
            return c
        if name == "export_patch":
            out = d.get("out_path") or os.path.splitext(rom)[0] + ".bps"
            return B.make_bps(stock_rom(game), rom, out, "%s (Customs Studio)" % os.path.basename(rom))
        raise ValueError("no Studio call " + name)

    port = a.port or free_port()
    httpd = S.ThreadingHTTPServer(("127.0.0.1", port), Handler)
    threading.Thread(target=httpd.serve_forever, daemon=True).start()
    url = "http://127.0.0.1:%d/" % port
    print("Customs Studio: %s (toolkit %s, recomp %s)" % (url, paths["toolkit"], paths["recomp"]), flush=True)
    if a.no_window:
        httpd.serve_forever()
        return
    if a.browser or not open_app_window(url):
        webbrowser.open(url)
        print("(close this window to stop the Studio)", flush=True)
        httpd.serve_forever()


def free_port():
    s = socket.socket()
    s.bind(("127.0.0.1", 0))
    p = s.getsockname()[1]
    s.close()
    return p


def open_app_window(url):
    """Edge in app mode, a profile of its own (so it is its own process): returns once it closes."""
    edge = None
    for p in (r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe",
              r"C:\Program Files\Microsoft\Edge\Application\msedge.exe"):
        if os.path.exists(p):
            edge = p
    if not edge:
        return False
    profile = os.path.join(work_dir(), "window")
    proc = subprocess.Popen([edge, "--app=" + url, "--user-data-dir=" + profile, "--no-first-run",
                             "--window-size=1500,950"])
    proc.wait()
    return True


def fail(msg):
    """A problem at the start, said plainly (a windowed .exe has no console)."""
    if FROZEN and sys.platform == "win32":
        import ctypes
        ctypes.windll.user32.MessageBoxW(None, msg, "Customs Studio", 0x10)
    print(msg, file=sys.stderr)
    sys.exit(1)


if __name__ == "__main__":
    try:
        main()
    except SystemExit as e:
        if isinstance(e.code, str):
            fail(e.code)
        raise
    except Exception as e:
        traceback.print_exc()
        fail("Customs Studio could not start:\n\n%s\n\n(Its settings: studio.ini beside it; its log: %s)"
             % (e, os.path.join(work_dir(), "studio.log")))
