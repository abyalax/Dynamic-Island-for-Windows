# dynamic_island (on hold)

Reserved for the modular version of `src/local@dynamic-island-for-windows.wh.cpp`.

**Status: HOLD.** The mod is still a single file and is deliberately excluded from
amalgamation in `windhawk-amalgamation.json` (`exclude`), because it uses
`__has_include`, which WHAM cannot bundle. `python tools/mods.py` treats it as a
plain single file (`install` and `syntax` use the source directly).

When this mod is picked up:

1. Remove `__has_include` (or add the file it tests as an external include rule).
2. Split it: `python tools/modularize/modularize.py src/local@dynamic-island-for-windows.wh.cpp --src src --entry local@dynamic-island-for-windows.wh.cpp --module-dir dynamic_island`
   (adjust `FAMILIES` in `tools/modularize/plan_modules.py` to this mod's domains first).
3. Remove it from `exclude` in `windhawk-amalgamation.json` and set `modular=True` for
   `dynamic-island` in `tools/mods.py`.

Do not put `.cpp` files here: every `.cpp` under `src/` is treated as a separate mod. Use `.hpp`.
