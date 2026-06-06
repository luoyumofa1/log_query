$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectDir = Split-Path -Parent $ScriptDir
Set-Location $ProjectDir

$exe = Join-Path $ProjectDir "build\log-query.exe"
if (-not (Test-Path $exe)) {
    Write-Host "ERROR: log-query.exe not found. Run .\scripts\build.ps1 first." -ForegroundColor Red
    exit 1
}

$sample = Join-Path $ScriptDir "sample.log"
$passed = 0
$failed = 0

function Test-Case($name, [ScriptBlock]$test) {
    Write-Host "  $name ... " -NoNewline
    try {
        & $test
        Write-Host "PASS" -ForegroundColor Green
        $script:passed++
    } catch {
        Write-Host "FAIL" -ForegroundColor Red
        Write-Host "    $_" -ForegroundColor Red
        $script:failed++
    }
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  log-query Integration Tests" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

Write-Host "[1] Basic field filtering" -ForegroundColor Yellow

Test-Case "filter by module" {
    $out = Get-Content $sample | & $exe -f module=lidar_driver
    if ($out.Count -ne 6) { throw "Expected 6 lines, got $($out.Count)" }
}

Test-Case "filter by level ERROR" {
    $out = Get-Content $sample | & $exe -f level=ERROR
    if ($out.Count -ne 3) { throw "Expected 3 lines, got $($out.Count)" }
}

Test-Case "filter by module + level" {
    $out = Get-Content $sample | & $exe -f module=lidar_driver -f level=ERROR
    if ($out.Count -ne 1) { throw "Expected 1 line, got $($out.Count)" }
}

Test-Case "filter by module + level (no match)" {
    $out = Get-Content $sample | & $exe -f module=radar_driver -f level=ERROR
    if ($out.Count -ne 0) { throw "Expected 0 lines, got $($out.Count)" }
}

Write-Host ""
Write-Host "[2] Edge cases" -ForegroundColor Yellow

Test-Case "empty input" {
    $out = "" | & $exe -f module=lidar
    if ($out.Count -ne 0) { throw "Expected 0 lines, got $($out.Count)" }
}

Test-Case "non-matching format lines are skipped" {
    $out = @("this is not a log line", "neither is this") | & $exe -f module=lidar
    if ($out.Count -ne 0) { throw "Expected 0 lines, got $($out.Count)" }
}

Test-Case "case insensitive level matching" {
    $out = Get-Content $sample | & $exe -f level=error
    if ($out.Count -ne 3) { throw "Expected 3 lines, got $($out.Count)" }
}

Write-Host ""
Write-Host "[3] Time range filtering" -ForegroundColor Yellow

Test-Case "filter by --from and --to" {
    $out = Get-Content $sample | & $exe --from "2024-01-15 08:00:03" --to "2024-01-15 08:00:05"
    if ($out.Count -ne 6) { throw "Expected 6 lines, got $($out.Count)" }
}

Test-Case "filter by --from only" {
    $out = Get-Content $sample | & $exe --from "2024-01-15 08:00:07"
    if ($out.Count -ne 8) { throw "Expected 8 lines, got $($out.Count)" }
}

Test-Case "filter by --to only" {
    $out = Get-Content $sample | & $exe --to "2024-01-15 08:00:01"
    if ($out.Count -ne 6) { throw "Expected 6 lines, got $($out.Count)" }
}

Test-Case "time range + field filter" {
    $out = Get-Content $sample | & $exe --from "2024-01-15 08:00:03" --to "2024-01-15 08:00:05" -f level=ERROR
    if ($out.Count -ne 1) { throw "Expected 1 line, got $($out.Count)" }
}

Write-Host ""
Write-Host "[4] Pipe mode (stdin)" -ForegroundColor Yellow

Test-Case "pipe from Get-Content" {
    $out = Get-Content $sample | & $exe -f module=planner
    if ($out.Count -ne 6) { throw "Expected 6 lines, got $($out.Count)" }
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Results: $passed passed, $failed failed" -ForegroundColor $(if ($failed -eq 0) { "Green" } else { "Red" })
Write-Host "========================================" -ForegroundColor Cyan

exit $failed
