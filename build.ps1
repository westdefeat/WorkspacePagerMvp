
param(
    [string]$Output = "WorkspacePagerMvp.exe"
)
taskkill /IM WorkspacePagerMvp.exe /F 

$ErrorActionPreference = "Stop"

$clang = "C:\Program Files\LLVM\bin\clang++.exe"
if (-not (Test-Path $clang)) {
    throw "clang++ not found at $clang"
}

& $clang `
    -std=c++17 `
    -municode `
    "WorkspacePagerMvp.cpp" `
    -lole32 `
    -loleaut32 `
    -luuid `
    -luser32 `
    -lshell32 `
    -ladvapi32 `
    -lgdi32 `
    -o $Output

if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

Write-Host "Built $Output"
