#!/usr/bin/env python3
"""Split a C++ source into top-level units (declarations/definitions).

A unit ends when brace/paren/bracket depth is 0 and a `;` is seen, or a `}`
closes a block that is followed by nothing else on its line (a function body).
Comments, strings, chars, raw strings and preprocessor lines are understood.
Leading comment lines directly above a unit belong to it; blank lines and
preprocessor-only lines between units are carried to the next unit.
Output: JSON list of {start, end, kind, name} with 1-based inclusive lines.
"""
import json
import re
import sys


def scan(text):
    lines = text.split("\n")
    n = len(text)
    i = 0
    line = 1
    depth = 0
    paren = 0
    bracket = 0
    unit_start_line = None
    units = []
    last_end = 0  # last line covered
    pending_close = False  # saw } closing to depth 0, decide on line end
    in_pp_cont = False
    at_line_start = True

    def end_unit(end_line, kind_hint=None):
        nonlocal unit_start_line, last_end
        units.append((unit_start_line, end_line))
        unit_start_line = None
        last_end = end_line

    while i < n:
        c = text[i]
        # preprocessor line (only at line start, depth 0 and no open unit text)
        if at_line_start and c in " \t":
            j = i
            while j < n and text[j] in " \t":
                j += 1
            if j < n and text[j] == "#" and depth == 0 and paren == 0:
                # whole directive incl. continuations
                k = j
                while True:
                    e = text.find("\n", k)
                    if e == -1:
                        e = n
                    seg = text[k:e]
                    if seg.rstrip().endswith("\\") and e < n:
                        k = e + 1
                        line += 1
                        continue
                    break
                i = e
                at_line_start = False
                if unit_start_line is None:
                    pass
                continue
        if at_line_start and c == "#" and depth == 0 and paren == 0:
            k = i
            while True:
                e = text.find("\n", k)
                if e == -1:
                    e = n
                seg = text[k:e]
                if seg.rstrip().endswith("\\") and e < n:
                    k = e + 1
                    line += 1
                    continue
                break
            i = e
            at_line_start = False
            continue
        if c == "\n":
            if pending_close:
                end_unit(line)
                pending_close = False
            line += 1
            i += 1
            at_line_start = True
            continue
        if c in " \t\r":
            i += 1
            at_line_start = at_line_start and c != "x"
            continue
        at_line_start = False
        # comments
        if c == "/" and i + 1 < n and text[i + 1] == "/":
            e = text.find("\n", i)
            i = n if e == -1 else e
            continue
        if c == "/" and i + 1 < n and text[i + 1] == "*":
            e = text.find("*/", i + 2)
            e = n if e == -1 else e + 2
            line += text.count("\n", i, e)
            i = e
            continue
        # a unit begins at its first code token
        if unit_start_line is None and not pending_close:
            unit_start_line = line
        # string / char / raw string
        if c == '"' or c == "'" or (
            c in "LuU" and i + 1 < n and text[i + 1] in "\"'"
        ) or (c == "u" and text.startswith("u8", i) and i + 2 < n and text[i + 2] == '"') or (
            c == "R" and i + 1 < n and text[i + 1] == '"'
        ) or (
            c in "LuU" and i + 2 < n and text[i + 1] == "R" and text[i + 2] == '"'
        ):
            raw = False
            j = i
            while j < n and text[j] in "LuU8":
                j += 1
            if j < n and text[j] == "R" and j + 1 < n and text[j + 1] == '"':
                raw = True
                j += 1
            if raw:
                # R"delim( ... )delim"
                k = text.index("(", j + 1)
                delim = text[j + 1 : k]
                endtok = ")" + delim + '"'
                e = text.index(endtok, k) + len(endtok)
                line += text.count("\n", i, e)
                i = e
                continue
            if j < n and text[j] in "\"'":
                q = text[j]
                k = j + 1
                while k < n:
                    if text[k] == "\\":
                        k += 2
                        continue
                    if text[k] == q:
                        break
                    if text[k] == "\n":
                        break
                    k += 1
                # digit separators like 1'000 are not char literals
                if q == "'" and j > 0 and (text[j - 1].isalnum()) and j == i:
                    i += 1
                    continue
                i = k + 1
                continue
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0 and paren == 0 and bracket == 0:
                # does anything else (other than ; or a comment) follow on the line?
                e = text.find("\n", i)
                rest = text[i + 1 : (n if e == -1 else e)]
                rest = re.sub(r"//.*$", "", rest).strip()
                if rest == "":
                    pending_close = True
                elif rest.startswith(";"):
                    pass  # `};` handled by the ; below
                else:
                    pass  # `} name;` continues to its ;
        elif c == "(":
            paren += 1
        elif c == ")":
            paren -= 1
        elif c == "[":
            bracket += 1
        elif c == "]":
            bracket -= 1
        elif c == ";" and depth == 0 and paren == 0 and bracket == 0:
            end_unit(line)
            pending_close = False
        i += 1
    if unit_start_line is not None:
        end_unit(line)
    return units, lines


def classify(lines, s, e):
    """Return (kind, name) from the unit's first code lines."""
    head = []
    for k in range(s - 1, min(e, s + 12)):
        t = lines[k].strip()
        if not t or t.startswith("//") or t.startswith("#"):
            continue
        head.append(t)
        if "{" in t or t.endswith(";"):
            break
    h = " ".join(head)
    m = re.match(r"^(?:template\s*<[^>]*>\s*)?(class|struct|enum(?:\s+class)?|union)\s+([A-Za-z_]\w*)", h)
    if m:
        return m.group(1).split()[0], m.group(2)
    m = re.match(r"^using\s+([A-Za-z_]\w*)\s*=", h)
    if m:
        return "using", m.group(1)
    if h.startswith("using ") or h.startswith("typedef"):
        return "using", h[:40]
    m = re.search(r"([A-Za-z_]\w*(?:::~?[A-Za-z_]\w*)+|~?[A-Za-z_]\w*)\s*\(", h)
    if m and "{" in h and "=" not in h.split("(")[0]:
        return "func", m.group(1)
    m = re.search(r"\b([A-Za-z_]\w*)\s*(?:\[[^\]]*\])?\s*(?:=|\{|;)", h)
    if m:
        return "var", m.group(1)
    return "other", h[:40]


if __name__ == "__main__":
    path = sys.argv[1]
    text = open(path, encoding="utf-8").read()
    units, lines = scan(text)
    out = []
    for (s, e) in units:
        kind, name = classify(lines, s, e)
        out.append({"start": s, "end": e, "kind": kind, "name": name})
    json.dump(out, open(sys.argv[2], "w"))
    print(len(out), "units;", "covered lines:", sum(u["end"] - u["start"] + 1 for u in out))
