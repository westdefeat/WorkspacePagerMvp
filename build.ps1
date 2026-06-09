
param(
    [string]$Output = "WorkspacePagerMvp.exe",
    [int]$Jobs = 0
)

$ErrorActionPreference = "Stop"

$clang = "C:\Program Files\LLVM\bin\clang++.exe"
if (-not (Test-Path $clang)) {
    throw "clang++ not found at $clang"
}

$ninja = Get-Command ninja -ErrorAction SilentlyContinue
if ($null -eq $ninja) {
    throw "ninja not found. Install it with: winget install --id Ninja-build.Ninja -e"
}

cmd /c "taskkill /IM WorkspacePagerMvp.exe /F >nul 2>nul"
New-Item -ItemType Directory -Force -Path ".build" | Out-Null

$ninjaArgs = @("-f", "build.ninja")
if ($Jobs -gt 0) {
    $ninjaArgs += @("-j", $Jobs)
}

& $ninja.Source @ninjaArgs

if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

if ($Output -ne "WorkspacePagerMvp.exe") {
    Copy-Item -Force "WorkspacePagerMvp.exe" $Output
}

Write-Host "Built $Output"
