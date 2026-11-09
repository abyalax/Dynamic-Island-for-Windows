# codebase-analysis

Analysis reports, **one folder per mod**, named after the mod's one-file source
(`<entry>.wh.cpp`). Several mods live in this repository, so nothing here is shared.

| Folder | Mod | Analyzed file |
|---|---|---|
| `local@mac-island-dock-fork.wh.cpp/` | mac-island (modular) | `output/local@mac-island-dock-fork.wh.cpp` (WHAM one-file output) |
| `local@dynamic-island-for-windows.wh.cpp/` | dynamic-island (on hold) | `src/local@dynamic-island-for-windows.wh.cpp` |

Regenerate a mod's folder (this regenerates the one-file output first if you ran `generate`):

```powershell
python tools/mods.py generate
python tools/mods.py analyze                 # mac-island
python tools/mods.py analyze dynamic-island
```

A modular mod's folder also gets `MODULES.md`: for each module of `src/<module_dir>/`, its line
range in the analyzed one-file output. A line number in `README-ANALYSIS.md` or the CSV files
therefore maps to a module file. Do not edit these files by hand; they are overwritten.

The analyzer itself (`analyzer.py`) is unchanged: `python analyzer.py --file <path> -o codebase-analysis`.
