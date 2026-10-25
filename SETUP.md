## Local Development, Build, and Testing

This section describes how to set up the local development environment, compile the Dynamic Island for Windows Windhawk mod, and test it on a local Windows machine without relying on the Windhawk VS Code extension.

### Prerequisites

- Windows 10 or Windows 11, 64-bit.
- [Windhawk](https://windhawk.net/) installed locally.
- The Windhawk compiler toolchain available at `C:\Program Files\Windhawk\Compiler`.
- PowerShell.
- The repository cloned to a local directory.

The commands below use the following example paths:

| Component                   | Path                                                 |
| --------------------------- | ---------------------------------------------------- |
| Repository                  | `C:\Users\<username>\c\Dynamic-Island-for-Windows`   |
| Compiler                    | `C:\Program Files\Windhawk\Compiler\bin\clang++.exe` |
| Compiler headers            | `C:\Program Files\Windhawk\Compiler\include`         |
| Windhawk x64 engine library | `C:\Program Files\Windhawk\Engine\1.7.3\64`          |
| Build output                | `output\dynamic-island-for-windows.dll`              |

Adjust these paths to match your local installation. The engine library path above is specific to Windhawk 1.7.3.

### 1. Configure the Environment

Open PowerShell and navigate to the repository root:

```powershell
Set-Location "C:\Users\<username>\c\Dynamic-Island-for-Windows"
```

Define the compiler and SDK paths:

```powershell
$compiler = "C:\Program Files\Windhawk\Compiler\bin\clang++.exe"
$include  = "C:\Program Files\Windhawk\Compiler\include"
$engine   = "C:\Program Files\Windhawk\Engine\1.7.3\64"
$out      = ".\output"
```

Verify that the compiler is available:

```powershell
& $compiler --version
```

The tested environment uses Clang 20.1.3 with the Windhawk-provided MinGW-w64 toolchain.

### 2. Check the Source Before Building

Run a syntax-only check before producing a DLL:

```powershell
& $compiler `
    --target=x86_64-w64-mingw32 `
    -std=c++23 `
    -DUNICODE `
    -D_UNICODE `
    -DWINVER=0x0A00 `
    -D_WIN32_WINNT=0x0A00 `
    -D_WIN32_IE=0x0A00 `
    -DNTDDI_VERSION=0x0A000008 `
    -D__USE_MINGW_ANSI_STDIO=0 `
    -DWH_MOD `
    -DWH_EDITING `
    -I $include `
    -include windhawk_api.h `
    -fsyntax-only `
    "src\dynamic-island-for-windows.wh.cpp"

if ($LASTEXITCODE -ne 0) {
    throw "Syntax check failed. Fix the reported errors before building."
}

Write-Host "Syntax check passed."
```

If the command finishes without compiler errors, the source has passed this syntax-check step. This does not yet verify linking or runtime behavior.

### 3. Compile and Link the DLL

Create the output directory if it does not exist:

```powershell
New-Item -ItemType Directory -Force -Path $out | Out-Null
```

Compile the source and link it against the Windhawk engine library and required Windows libraries:

```powershell
& $compiler `
    --target=x86_64-w64-mingw32 `
    -std=c++23 `
    -shared `
    -DUNICODE `
    -D_UNICODE `
    -DWINVER=0x0A00 `
    -D_WIN32_WINNT=0x0A00 `
    -D_WIN32_IE=0x0A00 `
    -DNTDDI_VERSION=0x0A000008 `
    -D__USE_MINGW_ANSI_STDIO=0 `
    -DWH_MOD `
    -DWH_EDITING `
    -I $include `
    -include windhawk_api.h `
    "src\dynamic-island-for-windows.wh.cpp" `
    -L $engine `
    -lwindhawk `
    -lole32 -loleaut32 -lshcore -ld2d1 -ldwrite -ldwmapi `
    -lgdi32 -luser32 -lshell32 -lruntimeobject -lwindowscodecs `
    -lsetupapi -lwinhttp -lpdh -lwinmm `
    -o "$out\dynamic-island-for-windows.dll"

if ($LASTEXITCODE -ne 0) {
    throw "Build failed. Review the compiler and linker output."
}

Write-Host "Build completed successfully."
```

Verify that the DLL was generated:

```powershell
Get-Item ".\output\dynamic-island-for-windows.dll"
```

**Important:** This is a manually verified local build command for the tested environment. If the source changes its dependencies, or if a different Windhawk version is used, the build flags and linked libraries may need to be updated.

### 4. Install and Test the Mod Locally

The build step generates a DLL; it does not automatically install or activate the mod.

For local testing:

1. Open Windhawk.
2. Locate the installed Dynamic Island for Windows mod, if one is already present.
3. Back up your current mod source or settings before replacing anything.
4. Open the mod's editor in Windhawk and use the local source file `src\dynamic-island-for-windows.wh.cpp` as the source for the test build.
5. Compile and apply the source through Windhawk's editor, then verify that the mod is enabled.
6. Restart or reload the mod using Windhawk's available controls if necessary.

The DLL in `output` is a build artifact, not a standalone Windows application. Do not launch it by double-clicking it or assume that copying it into an arbitrary Windhawk directory will install the mod correctly. Windhawk must load the mod through its own mod-loading mechanism.

If you want to test the generated DLL directly, first verify the loading and installation workflow supported by your specific Windhawk version. Do not overwrite engine files or replace an active mod's files while Windhawk is using them.

### 5. Runtime Verification

After enabling the mod, test the relevant features on your local machine:

- Confirm that the Dynamic Island overlay appears and behaves as expected.
- Test media detection and metadata updates with supported media sources.
- Verify album artwork loading and fallback behavior.
- Test other enabled features, including clipboard, battery, or download integrations, where applicable.
- Check Windhawk's mod logs for runtime errors.
- Disable the mod in Windhawk if it causes instability or unexpected behavior.

A successful compile confirms that the source can be built. It does not, by itself, guarantee that every feature works correctly at runtime.

### Troubleshooting

**`__has_include is unsupported because its file tests change after bundling`**

This error can occur in the VS Code extension's source-bundling stage. The manual syntax check and build commands above bypass that bundling stage. If the error appears only in the extension, investigate its bundler configuration separately.

**`windhawk_api.h` not found**

Verify that the compiler include directory exists and that `-I $include` and `-include windhawk_api.h` are present in the build command.

**`cannot find -lwindhawk`**

Verify that the engine library directory matches the installed Windhawk version and target architecture. The example uses the 64-bit library directory for Windhawk 1.7.3.

**Undefined references or linker errors**

Check the required Windows libraries and the Windhawk API declarations. Do not assume that a successful syntax check guarantees that linking will succeed.

**The mod builds but does not run**

Confirm that the source was installed and enabled through Windhawk, then inspect the mod logs. A generated DLL is not equivalent to an installed, active mod.

### Analyze Codebase

Run script python `analyzer.py` for high overview architecture
1. Repository Structure
2. Entry Points and Lifecycle Candidates
3. Configurable Settings and Variable Candidates
4. Function Inventory
5. Widndows API and Internal System Interaction
6. Include and Dependency Inventory
7. Heuristic Internall Call Graph
8. Recommended Reading Orders for AI Agent

All Output Analyzed place on `/codebase-analysis`
- `README-ANALYSIS.md` — 
- `repository-files.csv` — file inventory and line counts
- `functions.csv` / `functions.json` — function index and heuristic call relationships
- `windhawk-settings.csv` — detected Windhawk settings
- `variable-candidates.csv` — global/declaration candidates
- `windows-api-usage.csv` — matched Windows/system API occurrences
- `lifecycle-symbols.csv` — lifecycle/event-related symbol occurrences
- `includes.csv` — include/dependency inventory
- `analysis-summary.json` — machine-readable summary


### Development Notes

- Keep source changes in `src\dynamic-island-for-windows.wh.cpp`.
- Treat generated files in `output` as build artifacts rather than source files.
- Run the syntax check before each build when making significant changes.
- Back up working source and settings before testing experimental changes.
- Prefer Windhawk's own mod-loading and reload mechanisms over manually replacing files used by a running instance.
