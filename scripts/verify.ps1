# Build Sabrina (CLAP + VST3 + tests) and run the golden render check.
# Usage: powershell -File scripts/verify.ps1   (expects an existing ./build from
#   cmake -B build -G "Visual Studio 17 2022" -A x64 -DCOPY_AFTER_BUILD=FALSE)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Push-Location $root
try {
    cmake -B build -G "Visual Studio 17 2022" -A x64 -DCOPY_AFTER_BUILD=FALSE | Out-Null
    if ($LASTEXITCODE) { throw "configure failed" }
    cmake --build build --config Release --parallel --target sabrina-reverb-clap_clap sabrina-reverb-clap_vst3 sabrina-reverb-clap-tests
    if ($LASTEXITCODE) { throw "build failed" }
    & .\build\tests\Release\sabrina-reverb-clap-tests.exe
    if ($LASTEXITCODE) { throw "tests failed" }
    Write-Host "VERIFY OK"
} finally { Pop-Location }

