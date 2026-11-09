#!/usr/bin/env python3
"""Slice the single-file mac-island mod into WHAM modules.

Every module is a contiguous slice of the original text, so including them in
order rebuilds the original. Cuts never fall inside an #if/#endif block or a
#pragma pack(push)/pop pair, and a comment block directly above a declaration
stays with it.

Usage: split_modules.py <original.cpp> <units.json> <plan.json> <src_dir>
           [--entry NAME.wh.cpp] [--module-dir NAME] [--no-pragma-once]
"""
import json
import os
import re
import sys

import argparse

ap = argparse.ArgumentParser()
ap.add_argument("original")
ap.add_argument("units_json")
ap.add_argument("plan_json")
ap.add_argument("src_dir")
ap.add_argument("--entry", default="local@mac-island-dock-fork.wh.cpp")
ap.add_argument("--module-dir", default="mac_island")
ap.add_argument("--no-pragma-once", action="store_true")
args = ap.parse_args()
original, units_json, plan_json, src_dir = args.original, args.units_json, args.plan_json, args.src_dir
ENTRY_NAME = args.entry
MOD_DIR = args.module_dir
# WHAM turns a leading #pragma once into an include guard and keeps one copy of a
# module however many times it is included, so modules stay safe to #include.
PRAGMA = "" if args.no_pragma_once else "#pragma once\n"

raw = open(original, encoding="utf-8", newline="").read()
lines = raw.splitlines(keepends=True)  # 0-based; line N is lines[N-1]
total = len(lines)
units = json.load(open(units_json))
plan = json.load(open(plan_json))


def is_comment(i):  # 1-based
    return lines[i - 1].lstrip().startswith("//")


# Locate the Windhawk blocks and the prelude.
def find(prefix):
    for i, l in enumerate(lines, 1):
        if l.startswith(prefix):
            return i
    raise SystemExit("missing " + prefix)

readme_begin = find("// ==WindhawkModReadme==")
readme_end = find("// ==/WindhawkModReadme==")
settings_begin = find("// ==WindhawkModSettings==")
settings_end = find("// ==/WindhawkModSettings==")
first_include = find("#include <windhawk_utils.h>")
first_unit = units[0]["start"]

# Unsafe ranges: no cut may fall in (start, end].
unsafe = []
stack = []
pack = []
for i, l in enumerate(lines, 1):
    t = l.strip()
    if re.match(r"#\s*(if|ifdef|ifndef)\b", t):
        stack.append(i)
    elif re.match(r"#\s*endif\b", t) and stack:
        unsafe.append((stack.pop(), i))
    elif re.match(r"#\s*pragma\s+pack\s*\(\s*push", t):
        pack.append(i)
    elif re.match(r"#\s*pragma\s+pack\s*\(\s*pop", t) and pack:
        unsafe.append((pack.pop(), i))


def attach_comments(cut):
    """Move a cut up over the comment block directly above it."""
    while cut > 1 and is_comment(cut - 1) and cut - 1 > first_include:
        cut -= 1
    return cut


def safe_cut(cut):
    changed = True
    while changed:
        changed = False
        for s, e in unsafe:
            if s < cut <= e:
                cut = attach_comments(s)
                changed = True
    return cut


cuts = []
for m in plan:
    c = attach_comments(m["start"])
    cuts.append(safe_cut(c))
cuts[0] = first_include  # the first module also holds the prelude
# keep strictly increasing
for i in range(1, len(cuts)):
    if cuts[i] <= cuts[i - 1]:
        raise SystemExit("cuts collapsed at module %d" % i)

# Build the module list: prelude is its own module; code modules follow.
modules = []
first_code_cut = safe_cut(attach_comments(plan[0]["start"]))
modules.append({"dir": "foundation", "domain": "prelude", "start": first_include, "end": first_code_cut - 1})
for i, m in enumerate(plan):
    start = cuts[i] if i > 0 else first_code_cut
    end = (cuts[i + 1] - 1) if i + 1 < len(plan) else total
    modules.append({"dir": m["file"].split("/")[0], "domain": m["domain"], "start": start, "end": end})

# Name files with a global sequence number (order matters in one TU).
width = len(str(len(modules) - 1)) if len(modules) > 10 else 2
width = max(width, 2)
for n, m in enumerate(modules):
    m["file"] = "%s/%s/%0*d_%s.hpp" % (MOD_DIR, m["dir"], width, n, m["domain"])

def write(path, text):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="") as f:
        f.write(text)

root = os.path.join(src_dir)
# meta blocks
meta = [
    ("%s/meta/readme.hpp" % MOD_DIR, readme_begin, readme_end),
    ("%s/meta/settings.hpp" % MOD_DIR, settings_begin, settings_end),
]
for rel, s, e in meta:
    write(os.path.join(root, rel), "".join(lines[s - 1 : e]))

for m in modules:
    write(os.path.join(root, m["file"]), PRAGMA + "".join(lines[m["start"] - 1 : m["end"]]))

# Entry: everything before the readme, includes in the original order, and the
# lines in between the blocks (blank separators) kept verbatim.
entry = []
entry.append("".join(lines[0 : readme_begin - 1]))  # metadata + license note
entry.append('#include "%s/meta/readme.hpp"\n' % MOD_DIR)
entry.append("".join(lines[readme_end : settings_begin - 1]))  # separator lines
entry.append('#include "%s/meta/settings.hpp"\n' % MOD_DIR)
entry.append("".join(lines[settings_end : first_include - 1]))  # separator lines
for m in modules:
    entry.append('#include "%s"\n' % m["file"])
write(os.path.join(root, ENTRY_NAME), "".join(entry))

# Sanity: slices tile the file exactly.
covered = (readme_end - readme_begin + 1) + (settings_end - settings_begin + 1)
covered += (readme_begin - 1) + (settings_begin - readme_end - 1) + (first_include - settings_end - 1)
covered += sum(m["end"] - m["start"] + 1 for m in modules)
assert covered == total, (covered, total)
assert modules[0]["start"] == first_include
for a, b in zip(modules, modules[1:]):
    assert a["end"] + 1 == b["start"], (a["file"], b["file"])
assert modules[-1]["end"] == total

print(len(modules), "modules written;", "entry:", os.path.join(root, ENTRY_NAME))
