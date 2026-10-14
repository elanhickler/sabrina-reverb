# Build every product (CLAP + VST3 + tests) and run each product's tests,
# including the golden render check.
# Usage: powershell -ExecutionPolicy Bypass -File scripts/verify.ps1 [slug ...]
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Push-Location $root
try {
    $slugs = if ($args.Count) { $args } else {
        Get-ChildItem products -Directory | ? { Test-Path "$($_.FullName)\product.cmake" } | % Name }
    cmake -B build -G "Visual Studio 17 2022" -A x64 -DCOPY_AFTER_BUILD=FALSE | Out-Null
    if ($LASTEXITCODE) { throw "configure failed" }
    $targets = @(); foreach ($s in $slugs) { $targets += "${s}_clap", "${s}_vst3", "${s}-tests" }
    cmake --build build --config Release --parallel --target $targets
    if ($LASTEXITCODE) { throw "build failed" }
    foreach ($s in $slugs) {
        & ".\build\Release\$s-tests.exe"
        if ($LASTEXITCODE) { throw "$s tests failed" }
    }
    Write-Host "VERIFY OK: $($slugs -join ', ')"
} finally { Pop-Location }
