"""Reuse the built application's CLI objects/link flags for a native GUI test.

Usage: python build_color_gui_harness.py BUILD_DIR
No source changes to the application, installs, or printer connections.
"""
import json
from pathlib import Path
import shlex
import subprocess
import sys

build = Path(sys.argv[1]).resolve()
source = Path(__file__).with_name("loaded_filament_color_gui.cpp").resolve()
entries = json.loads((build / "compile_commands.json").read_text())
entry = next(item for item in entries if item["file"].endswith("/src/PrusaSlicer.cpp"))
args = shlex.split(entry["command"])
obj = str(build / "color_gui_harness.o")
out = str(build / "src" / "loaded_filament_color_gui")
args[args.index("-o") + 1] = obj
args[args.index("-c") + 1] = str(source)
subprocess.run(args, cwd=entry["directory"], check=True)
if "--compile-only" in sys.argv[2:]:
    print(obj)
    sys.exit(0)
commands = subprocess.check_output(["/opt/homebrew/bin/ninja", "-t", "commands", "PrusaSlicer"], cwd=build, text=True)
line = next(line for line in commands.splitlines() if " -o src/PrusaSlicer " in line)
link = shlex.split(line)
start = link.index(args[0])
end = link.index("&&", start) if "&&" in link[start:] else len(link)
link = link[start:end]
link[link.index("-o") + 1] = out
original = next(arg for arg in link if arg.endswith("/PrusaSlicer.cpp.o"))
link[link.index(original)] = obj
subprocess.run(link, cwd=build, check=True)
print(out)
