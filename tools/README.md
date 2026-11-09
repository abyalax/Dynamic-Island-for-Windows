# tools

| Tool | Purpose |
|---|---|
| `mods.py` | **The command** for building the mods: `setup`, `generate`, `check`, `lint`, `verify`, `syntax`, `build`, `install`, `analyze`, `all`. Documented in [`docs/WHAM-WORKFLOW.md`](../docs/WHAM-WORKFLOW.md). |
| `modularize/modularize.py` | Split a **single-file** `.wh.cpp` into WHAM modules (driver for the three scripts below). |
| `modularize/scan_units.py` | Find the top-level units (declarations and definitions) of a C++ file: understands strings, raw strings, comments, preprocessor lines and brace depth. |
| `modularize/plan_modules.py` | Label each unit with a domain from its name (`FAMILIES`) and merge neighbours into modules of reasonable size. |
| `modularize/split_modules.py` | Write the module files, the meta blocks and the entry file. |

`analyzer.py` (repository root) is the architecture analyzer; use it through `mods.py analyze`.

## How the single file was split

```powershell
python tools/modularize/modularize.py src/old.wh.cpp --src src --entry local@my-mod.wh.cpp --module-dir my_mod
```

* The text is only **sliced**, never reordered, so concatenating the modules in order gives back the
  original. WHAM's output of the result equals the input (`mods.py verify --baseline old.wh.cpp`).
* A cut never falls inside `#if`/`#endif` or `#pragma pack(push)`/`pop`, and a comment block directly
  above a declaration stays with it.
* Files are named `NN_domain.hpp`; `NN` is the include order (one translation unit: order is the dependency order).
* Every code module starts with `#pragma once` (`--no-pragma-once` to omit it).
* Names come from patterns in `plan_modules.py` (`FAMILIES`, `MEMBER_DOMAINS`). They fit this repository's
  mods; edit them for another mod, and rename or merge modules afterwards with `git mv` (keep the entry's include order and run `mods.py lint`).

The split is mechanical and domain names are heuristic: some modules still mix topics
(for example the first `taskbar/` modules also hold shared helpers). Improving that is ordinary
editing now, and `lint` plus `verify` tell you if a move broke something.

## Requirements

Python 3.9+. `mods.py setup` additionally needs Node.js 20+, npm and git.
