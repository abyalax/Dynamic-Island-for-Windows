#!/usr/bin/env python3
"""mods.py: one command for the Windhawk mods of this repository.

A modular mod is a set of ordered module headers (src/<module_dir>/**/*.hpp)
plus an entry file (src/<entry>.wh.cpp). WHAM, the Windhawk source
amalgamation tool, bundles them into the ONE file Windhawk needs
(output/<entry>.wh.cpp). This script drives WHAM and Windhawk's own CLI.

    python tools/mods.py setup              # once: fetch and build WHAM
    python tools/mods.py all                # generate + check + lint + syntax
    python tools/mods.py generate           # create the one-file .wh.cpp
    python tools/mods.py check              # is output/ up to date?
    python tools/mods.py lint               # cross-file safety rules
    python tools/mods.py verify --baseline old.wh.cpp
    python tools/mods.py syntax             # clang -fsyntax-only (needs Windhawk)
    python tools/mods.py build              # compile + link a test DLL
    python tools/mods.py install --enable   # compile + install via windhawk-cli
    python tools/mods.py analyze            # codebase-analysis/<mod>/ + MODULES.md

Add `mac-island` (the default) or `dynamic-island` after the command to pick a
mod, and `--dry-run` before it to print the Windhawk commands without running
them. See docs/WHAM-WORKFLOW.md.
"""
from __future__ import annotations

import argparse
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools" / "modularize"))
from scan_units import classify, scan  # noqa: E402

WHAM_REPO = "https://github.com/Meteoni/WHAM-VSCode"
WHAM_REF = "20b52dc"  # the revision these tools were tested with
WHAM_DIR = ROOT / ".tools" / "wham"

# `modular`: split into modules and bundled by WHAM. A mod on hold is still a
# plain single file: it is neither generated nor linted, and `install`/`syntax`
# use the source file itself.
MODS = {
    "mac-island": dict(entry="local@mac-island-dock-fork.wh.cpp", module_dir="src/mac_island", modular=True),
    "dynamic-island": dict(entry="local@dynamic-island-for-windows.wh.cpp", module_dir="src/dynamic_island", modular=False),
}
DEFAULT_MODS = ["mac-island"]


# ---------------------------------------------------------------- helpers
def read_exact(path):
    """File text with its line endings untouched (Path.read_text would translate them)."""
    return Path(path).read_bytes().decode("utf-8")


def say(msg=""):
    print(msg, flush=True)


def sh(cmd, dry=False, **kw):
    shown = " ".join(shlex.quote(str(c)) for c in cmd)
    say("$ " + shown)
    if dry:
        return 0
    return subprocess.run([str(c) for c in cmd], **kw).returncode


def one_file(mod):
    m = MODS[mod]
    return ROOT / ("output" if m["modular"] else "src") / m["entry"]


def metadata(path, key):
    head = path.read_text(encoding="utf-8", errors="replace")[:60000]
    m = re.search(r"^//\s*@%s\s+(.+?)\s*$" % re.escape(key), head, re.M)
    return m.group(1) if m else None


def mod_id(mod):
    src = ROOT / "src" / MODS[mod]["entry"]
    value = metadata(src, "id")
    if not value:
        raise SystemExit("no @id in " + str(src))
    return value


def wham_cli():
    path = Path(os.environ.get("WHAM_CLI") or WHAM_DIR / "dist" / "cli.js")
    if not path.exists():
        raise SystemExit("WHAM is not set up (%s). Run: python tools/mods.py setup" % path)
    node = shutil.which("node")
    if not node:
        raise SystemExit("Node.js 20+ is required for WHAM (https://nodejs.org).")
    return [node, str(path)]


def windhawk_root(args):
    candidates = [args.windhawk_root, os.environ.get("WINDHAWK_HOME"),
                  r"C:\Program Files\Windhawk", r"C:\Program Files (x86)\Windhawk"]
    for c in candidates:
        if c and ((Path(c) / "windhawk-cli.exe").exists() or (Path(c) / "Compiler").is_dir()):
            return Path(c)
    return None


def need_windhawk(args, what):
    root = windhawk_root(args)
    if root is None:
        raise SystemExit("%s needs Windhawk 2.0 (with the development tools). Pass --windhawk-root or set WINDHAWK_HOME." % what)
    return root


def selected(args):
    names = args.mods or DEFAULT_MODS
    for n in names:
        if n not in MODS:
            raise SystemExit("unknown mod %r (choose from: %s)" % (n, ", ".join(MODS)))
    return names


def flatten(text):
    """The generated one-file text without WHAM's bookkeeping."""
    cut = text.find("// #region WHAM recovery")
    if cut != -1:
        text = text[:cut]
    keep = []
    for line in text.splitlines(keepends=True):
        if re.match(r"// WHAM@\d+ ", line):
            continue
        if re.match(r"#(ifndef|define) WHAM_ONCE_[0-9A-Fa-f]+\s*$", line):
            continue
        if re.match(r"#endif // WHAM_ONCE_[0-9A-Fa-f]+\s*$", line) or line.strip() == "#pragma once":
            continue
        keep.append(line)
    return "".join(keep)


# ---------------------------------------------------------------- setup
def cmd_setup(args):
    git, npm, node = shutil.which("git"), shutil.which("npm"), shutil.which("node")
    if not (git and npm and node):
        raise SystemExit("setup needs git, Node.js 20+ and npm on PATH.")
    if not WHAM_DIR.exists():
        WHAM_DIR.parent.mkdir(parents=True, exist_ok=True)
        if sh([git, "clone", WHAM_REPO, WHAM_DIR]):
            return 1
    if sh([git, "-C", WHAM_DIR, "checkout", "-q", args.wham_ref or WHAM_REF]):
        return 1
    if sh([npm, "ci", "--ignore-scripts", "--no-audit", "--no-fund"], cwd=WHAM_DIR):
        return 1
    npx = shutil.which("npx") or "npx"
    if sh([npx, "tsc", "-p", "."], cwd=WHAM_DIR):
        return 1
    say("WHAM ready: " + str(WHAM_DIR / "dist" / "cli.js"))
    return 0


# ---------------------------------------------------------------- generate / check
def cmd_generate(args):
    modular = [m for m in selected(args) if MODS[m]["modular"]]
    if not modular:
        say("nothing to generate (mods on hold are single files).")
        return 0
    extra = ["--force"] if getattr(args, "force", False) else []
    rc = sh(wham_cli() + ["generate", ROOT, *extra])
    for m in modular:
        if one_file(m).exists():
            say("one-file mod: %s (%d bytes)" % (one_file(m).relative_to(ROOT), one_file(m).stat().st_size))
    return rc


def cmd_check(args):
    if not any(MODS[m]["modular"] for m in selected(args)):
        return 0
    rc = sh(wham_cli() + ["check", ROOT])
    say("output is up to date." if rc == 0 else "output is stale or missing: run `python tools/mods.py generate`.")
    return rc


def cmd_verify(args):
    """The generated mod must equal a baseline single file (after WHAM's bookkeeping)."""
    status = 0
    baseline = read_exact(args.baseline)
    for m in selected(args):
        if not MODS[m]["modular"]:
            continue
        got = flatten(read_exact(one_file(m)))
        if got == baseline:
            say("%s: EQUIVALENT to the baseline (%d bytes)" % (m, len(got)))
            continue
        status = 1
        a, b = got.splitlines(), baseline.splitlines()
        for i, (x, y) in enumerate(zip(a, b), 1):
            if x != y:
                say("%s: first difference at line %d\n  generated: %r\n  baseline : %r" % (m, i, x[:100], y[:100]))
                break
        else:
            say("%s: lengths differ (%d vs %d lines)" % (m, len(a), len(b)))
    return status


# ---------------------------------------------------------------- lint (cross-file safety)
STRIP = re.compile(
    r"""//[^\n]*|/\*.*?\*/|R"([^\s()\\]{0,16})\(.*?\)\1"|"(?:\\.|[^"\\\n])*"|'(?:\\.|[^'\\\n]){1,4}'""",
    re.S,
)
INCLUDE = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.M)


def code_only(text):
    return STRIP.sub(" ", text)


def cmd_lint(args):
    """Rules for modules that share one translation unit (all must hold)."""
    failures = 0
    for name in selected(args):
        m = MODS[name]
        if not m["modular"]:
            say("%s: on hold (single file), nothing to lint." % name)
            continue
        problems, notes = [], []
        entry = ROOT / "src" / m["entry"]
        mdir = ROOT / m["module_dir"]
        order = INCLUDE.findall(entry.read_text(encoding="utf-8"))
        rel = lambda p: p.relative_to(ROOT / "src").as_posix()
        on_disk = sorted(rel(p) for p in mdir.rglob("*.hpp"))

        # 1. every module is included exactly once, and none is missing
        for inc in set(order):
            if order.count(inc) > 1:
                problems.append("included %d times by the entry: %s" % (order.count(inc), inc))
            if not (ROOT / "src" / inc).exists():
                problems.append("included but missing on disk: " + inc)
        for f in on_disk:
            if f not in order:
                problems.append("orphan module (never included, so never built): " + f)

        # 2. include order follows the NN_ prefix (the order is the dependency order)
        numbered = [(int(mm.group(1)), inc) for inc in order for mm in [re.search(r"/(\d+)_[^/]+\.hpp$", inc)] if mm]
        for (a, ia), (b, ib) in zip(numbered, numbered[1:]):
            if b < a:
                problems.append("include order breaks the numeric prefix: %s then %s" % (ia, ib))

        # 3. per-module hygiene
        texts, units = {}, {}
        for inc in order:
            p = ROOT / "src" / inc
            if not p.exists():
                continue
            t = p.read_text(encoding="utf-8")
            texts[inc] = t
            is_meta = "/meta/" in inc
            if not is_meta and not t.startswith("#pragma once"):
                problems.append("no leading #pragma once (unsafe to include twice): " + inc)
            depth = 0
            for ln in t.splitlines():
                s = ln.strip()
                if re.match(r"#\s*(if|ifdef|ifndef)\b", s):
                    depth += 1
                elif re.match(r"#\s*endif\b", s):
                    depth -= 1
            packs = len(re.findall(r"#\s*pragma\s+pack\s*\(\s*push", t)) - len(re.findall(r"#\s*pragma\s+pack\s*\(\s*pop", t))
            if depth or packs:
                problems.append("unbalanced #if/#endif or #pragma pack across the module boundary: " + inc)
            for sub in INCLUDE.findall(t):
                if sub == m["entry"] or sub == inc:
                    problems.append("module includes itself or the entry: " + inc)

        # 4. a name must be declared before it is used (types, k* constants, g_* globals)
        code = {inc: code_only(t) for inc, t in texts.items() if "/meta/" not in inc}
        names = list(code)
        index = {inc: i for i, inc in enumerate(names)}
        # bare identifiers only: `A::B`, `x.B` and `p->B` are members or scoped names, not uses of a global B
        bare = re.compile(r"(?<![\w.])(?<!::)(?<!->)[A-Za-z_]\w*")
        idents = {inc: set(bare.findall(c)) for inc, c in code.items()}
        # enumerators are bare names too: `NetSpeed,` in an enum is not a use of a later struct NetSpeed
        enum_re = re.compile(r"\benum\b[^;{]*\{([^{}]*)\}")
        enumerators = {}
        for inc, c in code.items():
            found = set()
            for body in enum_re.findall(c):
                for item in body.split(","):
                    mm = re.match(r"\s*([A-Za-z_]\w*)", item)
                    if mm:
                        found.add(mm.group(1))
            enumerators[inc] = found
        forward, defined, dup = {}, {}, {}
        fwd_re = re.compile(r"^\s*(?:class|struct|union|enum(?:\s+class)?)\s+(\w+)\s*(?::\s*[\w:]+\s*)?;", re.M)
        for inc in names:
            for mm in fwd_re.finditer(code[inc]):
                forward.setdefault(mm.group(1), index[inc])
            lines = texts[inc].split("\n")
            for (s, e) in scan(texts[inc])[0]:
                kind, nm = classify(lines, s, e)
                body = "\n".join(lines[s - 1:e])
                relevant = kind in ("class", "struct", "enum", "using") or (kind == "var" and re.match(r"^(k[A-Z0-9]\w*|g_\w+)$", nm))
                if not relevant or (kind in ("class", "struct", "enum") and "{" not in body):
                    continue
                if nm in defined:
                    if kind in ("class", "struct", "enum"):
                        dup.setdefault(nm, [defined[nm][0]]).append(inc)
                    continue
                defined[nm] = (inc, index[inc])
        for nm, mods in dup.items():
            problems.append("type %r defined in more than one module: %s" % (nm, ", ".join(mods)))
        early = []
        for nm, (inc, k) in defined.items():
            enumerated = set()
            for j in range(k):
                enumerated |= enumerators[names[j]]
                if nm in idents[names[j]] and nm not in enumerated and not (nm in forward and forward[nm] <= j):
                    early.append((nm, inc, names[j]))
                    break
        for nm, inc, used in early[:25]:
            problems.append("%r is used in %s before %s declares it" % (nm, used, inc))
        if len(early) > 25:
            problems.append("... and %d more names used before their declaration" % (len(early) - 25))
        notes.append("%d modules, %d declarations tracked" % (len(order), len(defined)))

        if problems:
            failures += 1
            say("%s: LINT FAILED" % name)
            for p in problems:
                say("  - " + p)
        else:
            say("%s: lint OK (%s)" % (name, "; ".join(notes)))
    return 1 if failures else 0


# ---------------------------------------------------------------- Windhawk compiler
def compile_command(args, name, link):
    root = need_windhawk(args, "syntax/build")
    out = one_file(name)
    if not out.exists():
        raise SystemExit("%s does not exist; run `generate` first." % out)
    opts = shlex.split(metadata(out, "compilerOptions") or "")
    std = next((o for o in opts if o.startswith("-std=")), "-std=c++23")
    flags = ["--target=x86_64-w64-mingw32", std, "-DUNICODE", "-D_UNICODE", "-DWINVER=0x0A00",
             "-D_WIN32_WINNT=0x0A00", "-D_WIN32_IE=0x0A00", "-DNTDDI_VERSION=0x0A000008",
             "-D__USE_MINGW_ANSI_STDIO=0", "-DWH_MOD", "-DWH_EDITING", "-I", str(root / "Compiler" / "include"),
             "-include", "winsock2.h", "-include", "windhawk_api.h"]
    rest = [o for o in opts if not o.startswith("-std=")]
    clang = str(root / "Compiler" / "bin" / "clang++.exe")
    if not link:
        # a syntax check needs the defines, not the libraries
        return [clang, "-fsyntax-only", *flags, *[o for o in rest if not o.startswith("-l")], str(out)]
    engine = args.engine_dir
    if not engine:
        found = sorted((root / "Engine").glob("*/64"), key=lambda p: [int(x) if x.isdigit() else 0 for x in re.split(r"\D+", p.parent.name)])
        engine = str(found[-1]) if found else None
    if not engine:
        raise SystemExit("could not find the engine library (Engine/<version>/64); pass --engine-dir.")
    libs = [o for o in rest if o.startswith("-l")]
    others = [o for o in rest if not o.startswith("-l")]
    return [clang, "-shared", *flags, *others, str(out), "-L", engine, "-lwindhawk", *libs,
            "-o", str(ROOT / "build" / (mod_id(name) + ".dll"))]


def cmd_syntax(args):
    status = 0
    for name in selected(args):
        status |= sh(compile_command(args, name, link=False), dry=args.dry_run)
    return status


def cmd_build(args):
    status = 0
    if not args.dry_run:
        (ROOT / "build").mkdir(exist_ok=True)
    for name in selected(args):
        status |= sh(compile_command(args, name, link=True), dry=args.dry_run)
    if not args.dry_run and not status:
        say("link OK: the mod compiles and links against the Windhawk engine (build/*.dll is a test artifact, not installable).")
    return status


# ---------------------------------------------------------------- install
def windhawk_cli(root, cli_args, stdin=None, dry=False):
    """`stdin` is bytes: the one-file mod exactly as generated."""
    cmd = [str(root / "windhawk-cli.exe"), "--app-root", str(root), "--json", *cli_args]
    say("$ " + " ".join(shlex.quote(c) for c in cmd) + ("  < one-file mod" if stdin is not None else ""))
    if dry:
        return 0, None, ""
    p = subprocess.run(cmd, input=stdin, capture_output=True)
    out = p.stdout.decode("utf-8", "replace")
    try:
        data = json.loads(out)
    except ValueError:
        data = None
    return p.returncode, data, out + p.stderr.decode("utf-8", "replace")


def cmd_install(args):
    root = need_windhawk(args, "install")
    status = 0
    for name in selected(args):
        path = one_file(name)
        if not path.exists():
            raise SystemExit("%s does not exist; run `generate` first." % path)
        ident = "local@" + mod_id(name)
        rc, data, _ = windhawk_cli(root, ["mod", "config", "get", ident], dry=args.dry_run)
        installed = rc == 0 and isinstance(data, dict) and isinstance(data.get("config"), dict)
        disabled = True if args.dry_run else (not installed or bool(data["config"].get("disabled", True)))
        # A first build starts disabled; recompiling keeps the enabled state.
        cli_args = ["mod", "install", "--file", "-"] + (["--disabled"] if disabled else [])
        rc, data, raw = windhawk_cli(root, cli_args, stdin=path.read_bytes(), dry=args.dry_run)
        if rc:
            say("install failed (exit %d):\n%s" % (rc, raw.strip()))
            status = 1
            continue
        if isinstance(data, dict) and data.get("id") not in (None, ident):
            say("warning: Windhawk reports mod id %r, expected %r" % (data.get("id"), ident))
        if isinstance(data, dict) and data.get("warnings"):
            say("compiler warnings:\n%s" % data["warnings"])
        say("%s compiled and installed (%s)." % (ident, "disabled" if disabled else "enabled state kept"))
        if args.enable:
            rc, _, raw = windhawk_cli(root, ["mod", "enable", ident], dry=args.dry_run)
            if rc:
                say("enable failed (exit %d):\n%s" % (rc, raw.strip()))
                status = 1
    return status


# ---------------------------------------------------------------- analyze
def module_index(name):
    """MODULES.md: each module's lines in the one-file output, so the analysis' line numbers map to modules."""
    out = one_file(name)
    begin = re.compile(r"^// WHAM@\d+ begin (\d+): (.+)$")
    end = re.compile(r"^// WHAM@\d+ end (\d+)$")
    open_, spans = {}, []
    for i, line in enumerate(out.read_text(encoding="utf-8").split("\n"), 1):
        b, e = begin.match(line), end.match(line)
        if b:
            open_[b.group(1)] = (b.group(2), i + 1)
        elif e and e.group(1) in open_:
            path, start = open_.pop(e.group(1))
            spans.append((path, start, i - 1))
    spans.sort(key=lambda s: s[1])
    rows = []
    for path, start, last in spans:
        p = ROOT / path
        types = []
        if p.exists() and "/meta/" not in path:
            text = p.read_text(encoding="utf-8")
            lines = text.split("\n")
            for (s, e) in scan(text)[0]:
                kind, nm = classify(lines, s, e)
                if kind in ("class", "struct", "enum") and "{" in "\n".join(lines[s - 1:e]):
                    types.append("`%s`" % nm)
        shown = ", ".join(types[:6]) + (" ...+%d" % (len(types) - 6) if len(types) > 6 else "")
        rows.append("| `%s` | %d-%d | %d | %s |" % (path.replace("src/", "", 1), start, last, last - start + 1, shown))
    head = ("# %s: module index\n\nGenerated by `python tools/mods.py analyze`. Line numbers are lines of the ONE-file output "
            "`output/%s`, the file the rest of this folder analyzes, so a line in `README-ANALYSIS.md` maps to a module here.\n\n"
            "Modules are one C++ translation unit and are **included in this order**: a module may only use what the modules "
            "above it declare (`python tools/mods.py lint` checks this).\n\n"
            "| Module | Output lines | Lines | Types declared (first) |\n|---|---|---|---|\n") % (MODS[name]["entry"], MODS[name]["entry"])
    return head + "\n".join(rows) + "\n"


def cmd_analyze(args):
    for name in selected(args):
        path = one_file(name)
        if not path.exists():
            raise SystemExit("%s does not exist; run `generate` first." % path)
        rc = sh([sys.executable, ROOT / "analyzer.py", "--file", path, "-o", ROOT / "codebase-analysis"], cwd=ROOT)
        if rc:
            return rc
        if MODS[name]["modular"]:
            target = ROOT / "codebase-analysis" / MODS[name]["entry"] / "MODULES.md"
            target.write_text(module_index(name), encoding="utf-8", newline="\n")
            say("module index: " + str(target.relative_to(ROOT)))
    return 0


# ---------------------------------------------------------------- all
def cmd_all(args):
    steps = [("generate", cmd_generate), ("check", cmd_check), ("lint", cmd_lint)]
    if windhawk_root(args) is not None and not args.no_syntax:
        steps.append(("syntax", cmd_syntax))
    if args.analyze:
        steps.append(("analyze", cmd_analyze))
    results = []
    for label, fn in steps:
        say("\n== %s ==" % label)
        results.append((label, fn(args)))
    if windhawk_root(args) is None or args.no_syntax:
        results.append(("syntax", None))
    say("\n== summary ==")
    for label, rc in results:
        say("  %-9s %s" % (label, "SKIPPED (no Windhawk found; pass --windhawk-root)" if rc is None else ("ok" if rc == 0 else "FAILED")))
    return 1 if any(rc for _, rc in results if rc) else 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--windhawk-root", help="Windhawk install folder (default: WINDHAWK_HOME or Program Files)")
    ap.add_argument("--engine-dir", help="Engine/<version>/64 folder for `build`")
    ap.add_argument("--dry-run", action="store_true", help="print the Windhawk commands without running them")
    sub = ap.add_subparsers(dest="command", required=True)
    for cmd, fn, helptext in [
        ("setup", cmd_setup, "fetch and build WHAM into .tools/wham"),
        ("generate", cmd_generate, "create the one-file output/*.wh.cpp"),
        ("check", cmd_check, "fail if output/ is stale or missing"),
        ("lint", cmd_lint, "cross-file safety rules for the modules"),
        ("verify", cmd_verify, "compare the generated mod with a baseline single file"),
        ("syntax", cmd_syntax, "clang -fsyntax-only with Windhawk's compiler"),
        ("build", cmd_build, "compile and link a test DLL (finds missing -l libraries)"),
        ("install", cmd_install, "compile and install through windhawk-cli"),
        ("analyze", cmd_analyze, "refresh codebase-analysis/<mod>/ and MODULES.md"),
        ("all", cmd_all, "generate + check + lint (+ syntax when Windhawk is found)"),
    ]:
        p = sub.add_parser(cmd, help=helptext)
        p.add_argument("mods", nargs="*", help="mac-island (default) or dynamic-island")
        p.set_defaults(fn=fn)
        if cmd == "setup":
            p.add_argument("--wham-ref", help="WHAM git ref to build (default: %s)" % WHAM_REF)
        if cmd == "generate":
            p.add_argument("--force", action="store_true", help="discard edits made to output/ files")
        if cmd == "verify":
            p.add_argument("--baseline", required=True, help="single-file .wh.cpp to compare with")
        if cmd == "install":
            p.add_argument("--enable", action="store_true", help="enable the mod after installing")
        if cmd == "all":
            p.add_argument("--no-syntax", action="store_true", help="skip the Windhawk compiler step")
            p.add_argument("--analyze", action="store_true", help="also refresh codebase-analysis/")
    args = ap.parse_args()
    sys.exit(args.fn(args))


if __name__ == "__main__":
    main()
