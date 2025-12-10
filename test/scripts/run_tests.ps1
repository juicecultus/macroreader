Set-StrictMode -Version Latest
$testDir = Join-Path $PSScriptRoot '..'

# Candidate directories where CMake/IDEs place test executables. We'll pick the first that exists and contains executables.
$candidateBins = @(
    (Join-Path $testDir 'build\bin'),
    (Join-Path $testDir 'build\test\Debug'),
    (Join-Path $testDir 'build\test\Release'),
    (Join-Path $testDir 'build\x64\Debug\test'),
    (Join-Path $testDir 'build'),
    (Join-Path $testDir 'build\test')
)

$binDir = $null
foreach ($cand in $candidateBins) {
    if (Test-Path $cand) {
        # Does this folder contain .exe files?
        $exeCount = (Get-ChildItem -Path $cand -Filter *.exe -File -ErrorAction SilentlyContinue | Measure-Object).Count
        if ($exeCount -gt 0) { $binDir = $cand; break }
    }
}

if (-Not $binDir) {
    Write-Error "Tests not built or no test executables found. Run test/scripts/build_tests.ps1 first. Checked candidates: $($candidateBins -join ', ')"
    exit 1
}

Write-Host "Running tests in $binDir"
$outputDir = Join-Path $testDir 'output'
if (-Not (Test-Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir | Out-Null
}
$total = 0
$failures = @()
Get-ChildItem -Path $binDir -File | ForEach-Object {
    $exe = $_.FullName
    $name = $_.Name
    Write-Host "`n--- Running $name ---"
    & $exe
    $rc = $LASTEXITCODE
    $total++
    if ($rc -ne 0) {
        Write-Error "Test $name failed with exit code $rc"
        $failures += "$name (exit $rc)"
    }
}

Write-Host "`nRan $total test(s). Failures: $($failures.Count)"
if ($failures.Count -eq 0) {
    Write-Host "All tests passed"
    exit 0
} else {
    Write-Host "Failed tests:"
    $failures | ForEach-Object { Write-Host "  - $_" }
    exit 1
}
