"""toolkit_deps.py for the Studio's build: an import of every module the toolkit's
own .py files import that is not one of them (the standard library's, numpy,
ndspy...), so PyInstaller puts them in the .exe. The toolkit itself stays .py
on disk, loaded from its folder at run time (its paths are its own).

    python make_deps.py TOOLKIT_TOOLS_DIR
"""
import ast
import os
import sys

tools = sys.argv[1]
roots = [tools, os.path.join(tools, "seq_editor"), os.path.join(os.path.dirname(tools), "ouendan2"),
         os.path.join(os.path.dirname(tools), "ouendan1")]
own, found = set(), set()
files = []
for r in roots:
    if not os.path.isdir(r):
        continue
    for f in os.listdir(r):
        if f.endswith(".py"):
            own.add(f[:-3])
            files.append(os.path.join(r, f))
for p in files:
    try:
        tree = ast.parse(open(p, encoding="utf-8", errors="replace").read())
    except SyntaxError:
        continue
    for n in ast.walk(tree):
        if isinstance(n, ast.Import):
            for a in n.names:
                found.add(a.name)
        elif isinstance(n, ast.ImportFrom) and n.module and n.level == 0:
            found.add(n.module)
mods = sorted(m for m in found if m.split(".")[0] not in own)
here = os.path.dirname(os.path.abspath(__file__))
with open(os.path.join(here, "toolkit_deps.py"), "w", encoding="utf-8", newline="\n") as f:
    f.write('"""Made by make_deps.py: what the toolkit imports, for PyInstaller (never run)."""\n')
    for m in mods:
        f.write("try:\n    import %s  # noqa\nexcept Exception:\n    pass\n" % m)
print("%d modules" % len(mods))
