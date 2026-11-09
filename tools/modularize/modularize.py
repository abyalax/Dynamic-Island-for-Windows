#!/usr/bin/env python3
"""Split a single-file Windhawk mod into WHAM modules (scan -> plan -> split).

    python tools/modularize/modularize.py <single.wh.cpp> --src src \
        --entry local@my-mod.wh.cpp --module-dir my_mod

The original text is only sliced, never reordered, so the WHAM output of the
result equals the input (see `tools/mods.py verify`). Domain names come from
FAMILIES in plan_modules.py; edit them to fit another mod.
"""
import argparse
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))

ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
ap.add_argument("source", help="the single-file mod (.wh.cpp) to split")
ap.add_argument("--src", default="src", help="source directory of the WHAM project")
ap.add_argument("--entry", required=True, help="entry file name to write under --src")
ap.add_argument("--module-dir", required=True, help="folder under --src for the modules")
ap.add_argument("--no-pragma-once", action="store_true")
a = ap.parse_args()
with tempfile.TemporaryDirectory() as tmp:
    units, plan = os.path.join(tmp, "units.json"), os.path.join(tmp, "plan.json")
    run = lambda *c: subprocess.run([sys.executable, *c], check=True)
    run(os.path.join(HERE, "scan_units.py"), a.source, units)
    run(os.path.join(HERE, "plan_modules.py"), a.source, units, plan)
    extra = ["--no-pragma-once"] if a.no_pragma_once else []
    run(os.path.join(HERE, "split_modules.py"), a.source, units, plan, a.src,
        "--entry", a.entry, "--module-dir", a.module_dir, *extra)
