# Modular mods with WHAM (Windhawk Source Amalgamation)

The `mac-island` mod is no longer one 110,000-line file. It is a set of ordered
module headers that **WHAM** bundles back into the single `.wh.cpp` file Windhawk
needs. You edit modules; Windhawk always receives one generated file.

```text
src/
  local@mac-island-dock-fork.wh.cpp   entry: Windhawk metadata + the ordered #include list
  mac_island/                         the modules (one C++ translation unit, see below)
    meta/        readme.hpp, settings.hpp   the Windhawk README and settings blocks
    foundation/  00_prelude.hpp (system includes, macros), theme, utilities
    taskbar/ i18n/ render/ panels/ services/ settings/ island/
  dynamic_island/                     on hold (README.md only)
  local@dynamic-island-for-windows.wh.cpp   still a single file, excluded from WHAM
output/                               generated one-file mods (git-ignored)   <- give THIS to Windhawk
windhawk-amalgamation.json            WHAM project configuration
tools/mods.py                         the one command (this document)
tools/modularize/                     the tools that split the single file
codebase-analysis/<mod>/              analysis reports, one folder per mod
```

The output is **byte-equivalent to the previous single file** (apart from WHAM's
bookkeeping comments), so the compiled code did not change. Reproduce the proof
with `verify` below.

## 1. Prerequisites

| Need                                                             | Why                                                                                                        |
| ---------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------- |
| Windows 10/11 x64, **Windhawk 2.0** with the *development tools* | `windhawk-cli.exe` and the compiler (`Compiler\bin\clang++.exe`). Windhawk 2.0 alpha 1 introduced the CLI. |
| **Node.js 20+** and npm                                          | WHAM is a Node program.                                                                                    |
| **Git**                                                          | `setup` clones WHAM.                                                                                       |
| **Python 3.9+**                                                  | `tools/mods.py`, `analyzer.py`.                                                                            |

`generate`, `check`, `lint`, `verify` and `analyze` work without Windhawk (any OS);
`syntax`, `build` and `install` need it.

## 2. One-time setup

```powershell
git clone <this repository>
cd Dynamic-Island-for-Windows
python tools/mods.py setup
```

`setup` clones `Meteoni/WHAM-VSCode` into `.tools/wham` (git-ignored), checks out the tested
revision, runs `npm ci --ignore-scripts` and builds the CLI (`.tools/wham/dist/cli.js`). Use
`--wham-ref <ref>` to try another revision, or set `WHAM_CLI` to an existing `cli.js`.

Optional: install the WHAM VS Code extension (`meteoni.windhawk-amalgamation`) to regenerate on
save and to compile from the sidebar. It reads the same `windhawk-amalgamation.json`.

## 3. Everyday workflow

```powershell
# edit src/mac_island/**/*.hpp, then:
python tools/mods.py all                 # generate + check + lint (+ syntax if Windhawk is found)
python tools/mods.py install --enable    # compile and install into Windhawk
```

`all` ends with a summary and a non-zero exit code if any step failed.

## 4. Commands

Run `python tools/mods.py <command> [mac-island | dynamic-island]`. Global options go **before**
the command: `--dry-run` (print the Windhawk commands, run nothing), `--windhawk-root PATH`
(default: `WINDHAWK_HOME`, then `C:\Program Files\Windhawk`), `--engine-dir PATH`.

| Command                  | What it does                                                                                                                                                                                                                                           |
| ------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `setup`                  | Fetch and build WHAM (once).                                                                                                                                                                                                                           |
| `generate`               | **Create the one-file `output/local@mac-island-dock-fork.wh.cpp`** with WHAM. `--force` discards hand edits made to a generated file.                                                                                                                  |
| `check`                  | Fail if `output/` is stale or missing (CI-friendly).                                                                                                                                                                                                   |
| `lint`                   | Cross-file safety rules, section 5.                                                                                                                                                                                                                    |
| `verify --baseline FILE` | Prove the generated mod equals a single-file baseline once WHAM's comments and guards are removed.                                                                                                                                                     |
| `syntax`                 | `clang++ -fsyntax-only` with Windhawk's compiler and the mod's own `@compilerOptions` defines.                                                                                                                                                         |
| `build`                  | Compile **and link** a test DLL in `build/`. It takes every `-l...` library from the mod's `@compilerOptions`, the engine library, and `-lwindhawk`, so a missing library fails here and not in Windhawk. The DLL is a test artifact, not installable. |
| `install [--enable]`     | Compile and install through `windhawk-cli` (`mod install --file -`, the one-file mod on stdin). A first install is disabled; reinstalling keeps the enabled state. `--enable` enables it after.                                                        |
| `analyze`                | Run `analyzer.py` on the one-file output into `codebase-analysis/<mod>/` and write `MODULES.md` (module to output-line map).                                                                                                                           |
| `all`                    | `generate`, `check`, `lint`, `syntax` (skipped with a notice when Windhawk is not found); `--analyze` adds `analyze`, `--no-syntax` skips the compiler.                                                                                                |

The mod ID Windhawk uses is `local@` plus the `@id` of the metadata: `local@mac-island-dock-fork`.

### Equivalence proof

```powershell
git show 2347c16:src/local@mac-island-dock-fork.wh.cpp > baseline.cpp   # the last single-file version
python tools/mods.py generate
python tools/mods.py verify --baseline baseline.cpp
# mac-island: EQUIVALENT to the baseline (4870905 bytes)
```

## 5. Cross-file safety (what the modules share and how it is checked)

All modules are **one C++ translation unit**. There is no `import`/`export`, no separate linkage and
no per-module compilation: a module sees exactly what the modules included **before it** declare.
That is why order matters, and why this project makes it explicit:

* The entry includes every module **exactly once, in dependency order**, and file names carry the
  order: `NN_name.hpp`.
* Every code module starts with `#pragma once`. WHAM turns it into an include guard and keeps
  one copy, so a module can safely be `#include`d again by another module to state a dependency
  (`#include "mac_island/foundation/01_island_theme.hpp"`); it will not be duplicated.
* Modules are `.hpp`. A `.cpp` under `src/` is a *separate mod* for WHAM.

`python tools/mods.py lint` enforces, per mod:

1. every module on disk is included, and every include exists (no **orphans**, which would be silently never built);
2. nothing is included twice by the entry;
3. include order never goes backwards in the `NN_` prefix (equal prefixes are allowed for sub-modules);
4. each code module has `#pragma once`;
5. `#if`/`#endif` and `#pragma pack(push/pop)` are balanced inside each module (a cut in the middle of a block is rejected);
6. no module includes itself or the entry;
7. **no type, `k*` constant or `g_*` global is used before the module that declares it** (forward declarations
   such as `struct X;` are honoured, `A::B`/`x.B` and enumerators are ignored);
8. no type is defined in two modules.

Rule 7 is a textual check, so it is fast and may miss things or, rarely, object to a valid
trick; **the compiler is the final judge**, which is why `syntax` and `build` are part of the loop.

### Adding or moving code

* New code goes into the module whose domain it belongs to; a new module is `NN_name.hpp` placed
  in `src/mac_island/<group>/`, added to the entry's include list **at the position of its
  dependencies** (after everything it uses, before everything that uses it).
* To move a declaration to an earlier module, move everything it depends on first, then run `all`.
* Never edit `output/`. WHAM protects generated files and refuses to overwrite hand edits
  (`generate --force` overrides; `restore` below recovers them).
* Where the dragon eyes live: state, themes and shapes in `render/15_eyes.hpp`; the Direct2D
  drawing (`DrawEyes`) in `render/24_icons.hpp`; the `Island` members (timer, step, blink) in
  `island/86_island.hpp`. They are spread out because each piece had to sit after what it uses;
  `grep -rn "EyesState" src/mac_island` finds them all.

## 6. Going back and forth

* One generated file to modules: `node .tools/wham/dist/cli.js restore output/local@mac-island-dock-fork.wh.cpp recovered --no-git`.
  This works **only** for WHAM output (its `// WHAM@5 ...` comments and the recovery footer must be intact).
* A plain single-file mod to modules: `tools/modularize/` (see `tools/README.md`).

## 7. Windhawk CLI by hand

`tools/mods.py install` is these calls (`--app-root` is the Windhawk folder):

```powershell
$wh = "C:\Program Files\Windhawk"
& "$wh\windhawk-cli.exe" --app-root $wh --json status
& "$wh\windhawk-cli.exe" --app-root $wh --json mod config get local@mac-island-dock-fork
& "$wh\windhawk-cli.exe" --app-root $wh --json mod enable local@mac-island-dock-fork
& "$wh\windhawk-cli.exe" --app-root $wh --json mod disable local@mac-island-dock-fork
```

The source goes in on stdin (`mod install --file -`), as UTF-8. Prefer `mods.py install`: piping a file
from Windows PowerShell 5.1 re-encodes it and corrupts the non-ASCII text in this mod.

## 8. Troubleshooting

| Symptom                                                                                                          | Cause and fix                                                                                                                                                             |
| ---------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `undefined symbol: DefSubclassProc`, `PathFindFileNameW`, `AlphaBlend`, `Wlan...`, `CreateEnvironmentBlock`, ... | A hand-written link command missing libraries. The mod lists them in `@compilerOptions`; use `python tools/mods.py build` (or `install`, which Windhawk compiles itself). |
| `WHAM is not set up`                                                                                             | Run `python tools/mods.py setup`; needs Node 20+, npm, git.                                                                                                               |
| `Stale or missing: output/...`                                                                                   | Run `python tools/mods.py generate`.                                                                                                                                      |
| `Output edits block overwrite`                                                                                   | Someone edited a generated file. Recover the edit (`restore`) or `generate --force`.                                                                                      |
| `__has_include is unsupported ...`                                                                               | WHAM cannot bundle that file; this is why `dynamic-island` is excluded (on hold).                                                                                         |
| `lint` reports a name used before its declaration                                                                | Reorder the include (and number) so the declaring module comes first, or add a forward declaration.                                                                       |
| `syntax needs Windhawk 2.0`                                                                                      | Install Windhawk with the development tools, or pass `--windhawk-root`.                                                                                                   |

## 9. Not covered

`deploy-to-windhawk.ps1` is left as it was and is not part of this workflow (it expects exactly one
`.wh.cpp` under `src/`). The first full compile against Windhawk's headers and runtime behaviour must be
done on a Windows machine with Windhawk; this repository's CI-less checks stop at `generate`, `check`,
`lint` and `verify`.
