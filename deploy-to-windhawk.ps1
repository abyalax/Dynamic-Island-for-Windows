
[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

# Resolve paths
$repoRoot = $PSScriptRoot
$sourceDir = Join-Path $repoRoot "src"
$destinationDir = Join-Path $env:ProgramData "Windhawk\ModsSource"

# Check for Administrator privileges
$isAdmin = ([Security.Principal.WindowsPrincipal]::new(
        [Security.Principal.WindowsIdentity]::GetCurrent()
    )).IsInRole(
    [Security.Principal.WindowsBuiltInRole]::Administrator
)

if (-not $isAdmin) {
    Write-Host "Administrator privileges are required." -ForegroundColor Yellow
    Write-Host "Requesting elevation..." -ForegroundColor Yellow

    try {
        $arguments = @(
            "-NoProfile"
            "-ExecutionPolicy", "Bypass"
            "-File", "`"$PSCommandPath`""
        )

        Start-Process -FilePath "powershell.exe" `
            -Verb RunAs `
            -ArgumentList $arguments
    }
    catch {
        Write-Host "ERROR: Failed to request Administrator privileges." `
            -ForegroundColor Red
        Read-Host "Press Enter to exit"
    }

    exit
}

try {
    # Validate source directory
    if (-not (Test-Path -LiteralPath $sourceDir -PathType Container)) {
        throw "Source directory not found: $sourceDir"
    }

    # Locate the mod source
    $sources = @(
        Get-ChildItem -LiteralPath $sourceDir `
            -Filter "*.wh.cpp" `
            -File `
            -Recurse
    )

    if ($sources.Count -eq 0) {
        throw "No .wh.cpp source files were found in the src directory."
    }

    if ($sources.Count -gt 1) {
        Write-Host "Multiple mod source files were found:" `
            -ForegroundColor Yellow

        foreach ($file in $sources) {
            Write-Host "  - $($file.FullName)"
        }

        throw "Expected exactly one .wh.cpp file. Please select one source file."
    }

    $source = $sources[0]
    $content = Get-Content -LiteralPath $source.FullName -Raw

    # Extract the Windhawk metadata block
    $metadataMatch = [regex]::Match(
        $content,
        '(?s)//\s*==WindhawkMod==(.*?)//\s*==/WindhawkMod=='
    )

    if (-not $metadataMatch.Success) {
        throw "The Windhawk metadata block was not found in the source file."
    }

    $metadata = $metadataMatch.Groups[1].Value

    # Extract the mod ID
    $idMatch = [regex]::Match(
        $metadata,
        '(?m)^\s*//\s*@id\s+(.+?)\s*$'
    )

    if (-not $idMatch.Success) {
        throw "The @id field is missing from the Windhawk metadata."
    }

    # Extract the mod name
    $nameMatch = [regex]::Match(
        $metadata,
        '(?m)^\s*//\s*@name\s+(.+?)\s*$'
    )

    if (-not $nameMatch.Success) {
        throw "The @name field is missing from the Windhawk metadata."
    }

    $modId = $idMatch.Groups[1].Value.Trim()
    $modName = $nameMatch.Groups[1].Value.Trim()

    # Create the destination directory
    New-Item -ItemType Directory `
        -Path $destinationDir `
        -Force |
    Out-Null

    # Copy the source file
    $destination = Join-Path $destinationDir $source.Name

    Copy-Item -LiteralPath $source.FullName `
        -Destination $destination `
        -Force

    # Report deployment details
    Write-Host ""
    Write-Host " Windhawk Source Deployment Successful" -ForegroundColor Green
    Write-Host "Mod Name : $modName"
    Write-Host "Mod ID   : $modId"
    Write-Host "Source   : $($source.FullName)"
    Write-Host "Target   : $destination"
    Write-Host ""

    Write-Host "Next Steps:" -ForegroundColor Cyan
    Write-Host "1. Open Windhawk."
    Write-Host "2. Locate the mod: $modName"
    Write-Host "3. If it does not appear, refresh or restart Windhawk."
    Write-Host "4. Ensure Windhawk has loaded the updated source."
    Write-Host "5. Compile and apply the source through Windhawk if prompted."
    Write-Host "6. Enable the mod and test its functionality."
    Write-Host ""

    Write-Host "Note: Copying the source does not automatically enable the mod." `
        -ForegroundColor Yellow
}
catch {
    Write-Host ""
    Write-Host "ERROR: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host ""
}
finally {
    Read-Host "Press Enter to close this window"
}
