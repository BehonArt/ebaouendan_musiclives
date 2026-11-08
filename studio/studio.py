"""Customs Studio: the EBA / Ouendan chart editor as a companion app of eba-recomp.

The toolkit's editor (rom_hacking/tools/seq_editor: the Note Editor and the
Section Editor, with every rule of a chart that loads) runs as it is, in a
window of its own (pywebview on WebView2, part of Windows 11 and most of
Windows 10): closing it stops the Studio. Without WebView2 (or with --browser)
it opens in the default browser, with a console window that stops it when
closed. Started again while it runs, it opens another window on the same
Studio. The Studio adds a bar to both editors for the recomp's side of a
custom (recomp_bridge.py):

- Test in recomp: the custom just built, played in the recomp's Customs mode
  at once (its own run, the stage straight away, any difficulty, autoplay).
- Export custom folder: customs/<tag> for the recomp (its Customs list, or
  Import custom folder on another PC).
- Export patch: a BPS patch of the stock ROM, to share without the game.

    studio.exe                 (or: python studio.py)
    studio.exe --browser       (the default browser instead of its own window)
    studio.exe --no-window     (serve only, no window: tests)

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
import urllib.request
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
        sys.path.insert(0, os.getcwd())             # as python -c: the current folder's modules
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
        for k in ("toolkit", "recomp"):            # a relative path is from the Studio's own folder (the release)
            c["paths"][k] = os.path.normpath(os.path.join(APP_DIR, os.path.expandvars(c["paths"][k])))
    else:
        with open(path, "w", encoding="utf-8") as f:
            f.write("# Customs Studio: where the toolkit's tools folder and the recomp are\n")
            c.write(f)
    return c["paths"], path


def deploy_toolkit(paths):
    """The release: the toolkit runs from the player's own folder, not the install's, so it can write
    anywhere (Program Files, a launcher's library) and an update never touches their charts and builds.
    The shipped files are copied over when they differ; what the player made is never in that set."""
    shipped = os.path.dirname(os.path.dirname(paths["toolkit"]))   # .../toolkit/rom_hacking/tools -> .../toolkit
    dest = os.path.join(B.data_root(paths["recomp"]), "Studio", "toolkit")
    for root, _, files in os.walk(shipped):
        to = os.path.join(dest, os.path.relpath(root, shipped))
        os.makedirs(to, exist_ok=True)
        for f in files:
            a, b = os.path.join(root, f), os.path.join(to, f)
            sa = os.stat(a)
            if not os.path.exists(b) or os.path.getsize(b) != sa.st_size or int(os.path.getmtime(b)) != int(sa.st_mtime):
                shutil.copy2(a, b)
    return os.path.join(dest, "rom_hacking", "tools")


def work_dir():
    d = os.path.join(os.environ.get("LOCALAPPDATA", tempfile.gettempdir()), "eba-recomp", "studio")
    os.makedirs(d, exist_ok=True)
    return d


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", type=int, default=0)
    ap.add_argument("--browser", action="store_true", help="the default browser instead of its own window")
    ap.add_argument("--no-window", action="store_true", help="serve only (tests)")
    a = ap.parse_args()
    if FROZEN and sys.platform == "win32":
        # a console program (so the toolkit's builders, run by it, have their pipes). Its own window:
        # the run itself has no console (hiding one fails under Windows Terminal): started again
        # with none, which the builders and ffmpeg inherit; this first run ends at once
        if not a.browser and not a.no_window and not os.environ.get(NO_CONSOLE_ENV):
            subprocess.Popen(sys.argv, env=dict(os.environ, **{NO_CONSOLE_ENV: "1"}),
                             creationflags=subprocess.CREATE_NO_WINDOW, close_fds=True)
            return
        import ctypes
        if a.browser:
            ctypes.windll.kernel32.SetConsoleTitleW("Customs Studio (close this window to stop it)")
        sys.stdout = sys.stderr = Tee(sys.stdout, open(os.path.join(work_dir(), "studio.log"), "w", encoding="utf-8", buffering=1))
    # already running: its page again, not a second Studio
    port = a.port or DEFAULT_PORT
    if port_answers(port):
        if studio_answers(port):
            url = "http://127.0.0.1:%d/" % port
            print("Customs Studio is already running: %s" % url, flush=True)
            if not a.no_window and (a.browser or not open_app(url)):
                webbrowser.open(url)
            return
        port = free_port()                                # (something else has that port)
    paths, ini = load_config()
    if paths.get("deploy", "") == "1":
        paths["toolkit"] = deploy_toolkit(paths)
    seq_editor = os.path.join(paths["toolkit"], "seq_editor")
    if not os.path.exists(os.path.join(seq_editor, "server.py")):
        sys.exit("Customs Studio: no seq_editor\\server.py in the toolkit folder %s (set it in %s)" % (paths["toolkit"], ini))
    sys.path.insert(0, seq_editor)
    os.chdir(paths["toolkit"])
    # the ROMs: the ones in the recomp's play folders (play\EBA.nds, ...), unless the toolkit's own
    # way (EBA_ROM, original_roms\) already says; so the tester kit needs each ROM in one place only
    for g, var in (("EBA", "EBA_ROM"), ("OE1", "OE1_ROM"), ("OE2", "OE2_ROM")):
        try:
            rom = B.recomp_folder(paths["recomp"], g)["rom"]
            if rom and os.path.isfile(rom) and not os.environ.get(var):
                os.environ[var] = rom
        except FileNotFoundError:
            pass
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
            out = {"toolkit": paths["toolkit"], "recomp": paths["recomp"], "ini": ini, "games": {}, "dumps": [], "missing": {}}
            for g in B.GAMES:
                try:
                    f = B.recomp_folder(paths["recomp"], g)
                    out["games"][g] = f["exe"]
                    if f["rom"] and os.path.exists(f["rom"]):
                        out["dumps"].append(g)
                    else:
                        out["missing"][g] = f["rom"]
                except Exception as e:
                    out["games"][g] = None
            return out
        if name == "open_builds":                         # the folder the built ROMs and patches land in
            os.startfile(os.path.join(os.path.dirname(paths["toolkit"]), "patched_roms" if not d.get("game") or d["game"] == "EBA"
                                      else "ouendan%s\\patched_roms" % d["game"][-1]))
            return {"ok": True}
        rom = d.get("rom") or last.get(d.get("chart", ""))
        if not rom or not os.path.exists(rom):
            raise ValueError("build the ROM first (Build ROM / Build all diffs), then this")
        game = B.game_of(rom)
        if name == "test":
            customs = os.path.join(work_dir(), "customs_" + game.lower())
            os.makedirs(customs, exist_ok=True)
            c = B.make_custom(rom, stock_rom(game), customs, d.get("title") or "Studio test", d.get("artist", ""),
                              tag="studio_test", difficulties=d.get("difficulties", "enhx"))
            r = B.launch_test(game, c, d.get("difficulty", "n"), bool(d.get("autoplay")), paths["recomp"],
                              os.path.join(work_dir(), "run_" + game.lower()))
            return dict(r, stage=c["stage"], game=game)
        if name == "validate":                            # the recomp plays it through headless: a report
            import validate as V
            customs = os.path.join(work_dir(), "customs_" + game.lower())
            os.makedirs(customs, exist_ok=True)
            c = B.make_custom(rom, stock_rom(game), customs, d.get("title") or "Studio test", d.get("artist", ""),
                              tag="studio_test", difficulties=d.get("difficulties", "enhx"))
            r = V.validate_custom(game, c, d.get("difficulty", "n"), paths["recomp"],
                                  os.path.join(work_dir(), "validate_" + game.lower()))
            return dict(r, text=V.report_text(r))
        if name == "export_custom":
            out = d.get("out_dir") or B.recomp_folder(paths["recomp"], game)["customs"]
            c = B.make_custom(rom, stock_rom(game), out, d.get("title", ""), d.get("artist", ""),
                              difficulties=d.get("difficulties", "enhx"))
            if d.get("zip"):
                c["zip"] = shutil.make_archive(c["dir"], "zip", os.path.dirname(c["dir"]), c["tag"])
            return c
        if name == "export_patch":
            out = d.get("out_path") or os.path.splitext(rom)[0] + ".bps"
            return B.make_bps(stock_rom(game), rom, out, "%s (Customs Studio)" % os.path.basename(rom))
        raise ValueError("no Studio call " + name)

    httpd = S.ThreadingHTTPServer(("127.0.0.1", port), Handler)
    url = "http://127.0.0.1:%d/" % port
    print("Customs Studio: %s (toolkit %s, recomp %s)" % (url, paths["toolkit"], paths["recomp"]), flush=True)
    if a.no_window:
        httpd.serve_forever()
        return
    threading.Thread(target=httpd.serve_forever, daemon=True).start()
    if not a.browser:
        if open_app(url):                                 # its own window, until it closes
            os._exit(0)                                   # (a build still running stops with it)
        print("(No WebView2 here: the default browser instead.)", flush=True)
        if FROZEN and sys.platform == "win32":            # a run with a console, the way to stop it
            httpd.server_close()
            env = {k: v for k, v in os.environ.items() if k != NO_CONSOLE_ENV}
            subprocess.Popen([sys.executable, "--browser"], env=env, creationflags=subprocess.CREATE_NEW_CONSOLE)
            os._exit(0)
    print("\nThe editor is open in your browser: %s\nKeep this window open while you work; close it to stop the Studio.\n" % url, flush=True)
    webbrowser.open(url)
    threading.Event().wait()                              # until this console closes, whatever the browser does


NO_CONSOLE_ENV = "EBA_STUDIO_NO_CONSOLE"
DEFAULT_PORT = 8574                                       # the same address each run: a tab or bookmark keeps working


class AppApi:
    """What the page may ask of the app window (studio.js): another window on one of the Studio's pages."""

    def __init__(self, base):
        self._base = base

    def open_window(self, path):
        if isinstance(path, str) and path.startswith("/") and not path.startswith("//"):
            import webview
            app_window(webview, self._base + path, self)


def app_window(webview, url, api):
    return webview.create_window("Customs Studio", url, js_api=api, width=1500, height=950, min_size=(900, 600),
                                 text_select=True, zoomable=True)


def open_app(url):
    """The Studio in a window of its own, until the last one closes. False: no WebView2 (or it failed to start)."""
    try:
        import webview
        app_window(webview, url, AppApi(url.rstrip("/")))
        webview.start(gui="edgechromium", private_mode=False, storage_path=os.path.join(work_dir(), "webview"))
        return True
    except Exception:
        traceback.print_exc()
        return False


class Tee:
    """The console and the log file."""

    def __init__(self, *streams):
        self.streams = [x for x in streams if x]

    def write(self, text):
        for x in self.streams:
            try:
                x.write(text)
                x.flush()
            except Exception:
                pass
        return len(text)

    def flush(self):
        pass


def port_answers(port):
    with socket.socket() as s:
        s.settimeout(0.5)
        return s.connect_ex(("127.0.0.1", port)) == 0


def studio_answers(port):
    try:
        with urllib.request.urlopen("http://127.0.0.1:%d/studio/studio.js" % port, timeout=2) as r:
            return r.status == 200
    except Exception:
        return False


def free_port():
    s = socket.socket()
    s.bind(("127.0.0.1", 0))
    p = s.getsockname()[1]
    s.close()
    return p


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
