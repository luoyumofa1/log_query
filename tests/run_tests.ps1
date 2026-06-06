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
Write-Host "[4] Regex filtering" -ForegroundColor Yellow

Test-Case "regex on message field" {
    $out = Get-Content $sample | & $exe --match "message=timeout|overflow"
    if ($out.Count -ne 3) { throw "Expected 3 lines, got $($out.Count)" }
}

Test-Case "regex on module field" {
    $out = Get-Content $sample | & $exe --match "module=.*driver"
    if ($out.Count -ne 12) { throw "Expected 12 lines, got $($out.Count)" }
}

Test-Case "regex + field filter" {
    $out = Get-Content $sample | & $exe -f level=ERROR --match "message=overflow|failed"
    if ($out.Count -ne 2) { throw "Expected 2 lines, got $($out.Count)" }
}

Write-Host ""
Write-Host "[5] Output modes" -ForegroundColor Yellow

$jsonOut = Join-Path $ProjectDir "test-output.json"
$csvOut = Join-Path $ProjectDir "test-output.csv"

Test-Case "JSON output writes file with array wrapper" {
    if (Test-Path $jsonOut) { Remove-Item $jsonOut -Force }
    Get-Content $sample | & $exe -f level=ERROR --output json --output-file $jsonOut | Out-Null
    if (-not (Test-Path $jsonOut)) { throw "JSON output file not created" }
    $out = Get-Content $jsonOut
    if ($out[0] -ne "[") { throw "Expected '[' on first line, got $($out[0])" }
    if ($out[-1] -ne "]") { throw "Expected ']' on last line, got $($out[-1])" }
}

Test-Case "JSON output has correct number of entries" {
    if (Test-Path $jsonOut) { Remove-Item $jsonOut -Force }
    Get-Content $sample | & $exe -f level=ERROR --output json --output-file $jsonOut | Out-Null
    $out = Get-Content $jsonOut
    $dataLines = $out | Where-Object { $_ -match '"line":' }
    if ($dataLines.Count -ne 3) { throw "Expected 3 data lines, got $($dataLines.Count)" }
}

Test-Case "CSV output writes file with header" {
    if (Test-Path $csvOut) { Remove-Item $csvOut -Force }
    Get-Content $sample | & $exe -f level=ERROR --output csv --output-file $csvOut | Out-Null
    if (-not (Test-Path $csvOut)) { throw "CSV output file not created" }
    $out = Get-Content $csvOut
    if ($out[0] -notmatch "^line,") { throw "Expected CSV header starting with 'line,', got $($out[0])" }
}

Test-Case "CSV output has correct row count" {
    if (Test-Path $csvOut) { Remove-Item $csvOut -Force }
    Get-Content $sample | & $exe -f level=ERROR --output csv --output-file $csvOut | Out-Null
    $out = Get-Content $csvOut
    if ($out.Count -ne 4) { throw "Expected 4 lines (1 header + 3 data), got $($out.Count)" }
}

Test-Case "plain output has no ANSI escape codes" {
    $out = Get-Content $sample | & $exe -f level=ERROR --output plain
    foreach ($line in $out) {
        if ($line -match '\x1b') { throw "Plain output should not contain ANSI escape codes" }
    }
}

Test-Case "plain output has correct line count" {
    $out = Get-Content $sample | & $exe -f level=ERROR --output plain
    if ($out.Count -ne 3) { throw "Expected 3 lines, got $($out.Count)" }
}

Test-Case "plain output matches color output line count" {
    $plainOut = Get-Content $sample | & $exe -f level=WARN --output plain
    $colorOut = Get-Content $sample | & $exe -f level=WARN --output color
    if ($plainOut.Count -ne $colorOut.Count) { throw "Plain ($($plainOut.Count)) and color ($($colorOut.Count)) line counts differ" }
}

Test-Case "plain output with combined filters" {
    $out = Get-Content $sample | & $exe -f module=planner --output plain
    if ($out.Count -ne 6) { throw "Expected 6 lines, got $($out.Count)" }
}

if (Test-Path $jsonOut) { Remove-Item $jsonOut -Force }
if (Test-Path $csvOut) { Remove-Item $csvOut -Force }

Write-Host ""
Write-Host "[6] Summary output mode" -ForegroundColor Yellow

Test-Case "summary output has header row" {
    $out = Get-Content $sample | & $exe --output summary
    if ($out[0] -notmatch "^Module") { throw "Expected header starting with 'Module', got $($out[0])" }
}

Test-Case "summary output has Total row" {
    $out = Get-Content $sample | & $exe --output summary
    $totalRow = $out | Where-Object { $_ -match "^Total" }
    if ($totalRow.Count -eq 0) { throw "Expected 'Total' row in summary output" }
}

Test-Case "summary output total count matches input" {
    $out = Get-Content $sample | & $exe --output summary
    $totalRow = $out | Where-Object { $_ -match "^Total" }
    if ($totalRow -notmatch "\s+25\s*$") { throw "Expected total of 25 in summary, got: $totalRow" }
}

Test-Case "summary with filter shows filtered stats" {
    $out = Get-Content $sample | & $exe --output summary -f level=ERROR
    $totalRow = $out | Where-Object { $_ -match "^Total" }
    if ($totalRow -notmatch "\s+3\s*$") { throw "Expected total of 3 in filtered summary, got: $totalRow" }
}

Test-Case "summary output has separator lines" {
    $out = Get-Content $sample | & $exe --output summary
    $separators = $out | Where-Object { $_ -match "^-{10,}" }
    if ($separators.Count -lt 2) { throw "Expected at least 2 separator lines, got $($separators.Count)" }
}

Test-Case "summary with time range" {
    $out = Get-Content $sample | & $exe --output summary --from "2024-01-15 08:00:03" --to "2024-01-15 08:00:07"
    $totalRow = $out | Where-Object { $_ -match "^Total" }
    if ($totalRow -notmatch "\s+17\s*$") { throw "Expected total of 17 in time-filtered summary, got: $totalRow" }
}

Write-Host ""
Write-Host "[7] Numeric comparison filtering" -ForegroundColor Yellow

$metricsSample = Join-Path $ScriptDir "sample_metrics.log"
$metricsConfig = Join-Path $ScriptDir "test_metrics_config.json"

Test-Case "int field greater than" {
    $out = Get-Content $metricsSample | & $exe --format-config $metricsConfig -f "status>400" --output plain
    if ($out.Count -ne 6) { throw "Expected 6 lines, got $($out.Count)" }
}

Test-Case "int field less than" {
    $out = Get-Content $metricsSample | & $exe --format-config $metricsConfig -f "status<300" --output plain
    if ($out.Count -ne 9) { throw "Expected 9 lines, got $($out.Count)" }
}

Test-Case "float field greater than" {
    $out = Get-Content $metricsSample | & $exe --format-config $metricsConfig -f "resp_time>100" --output plain
    if ($out.Count -ne 5) { throw "Expected 5 lines, got $($out.Count)" }
}

Test-Case "float field less than or equal" {
    $out = Get-Content $metricsSample | & $exe --format-config $metricsConfig -f "resp_time<=5.0" --output plain
    if ($out.Count -ne 7) { throw "Expected 7 lines, got $($out.Count)" }
}

Test-Case "int not equal with string filter" {
    $out = Get-Content $metricsSample | & $exe --format-config $metricsConfig -f "status!=200" -f "service=payment" --output plain
    if ($out.Count -ne 4) { throw "Expected 4 lines, got $($out.Count)" }
}

Test-Case "comparison + level filter combined" {
    $out = Get-Content $metricsSample | & $exe --format-config $metricsConfig -f "status>=500" -f "level=ERROR" --output plain
    if ($out.Count -ne 3) { throw "Expected 3 lines, got $($out.Count)" }
}

Test-Case "comparison with summary output" {
    $out = Get-Content $metricsSample | & $exe --format-config $metricsConfig -f "resp_time>100" --output summary
    $totalRow = $out | Where-Object { $_ -match "^Total" }
    if ($totalRow -notmatch "\s+5\s*$") { throw "Expected total of 5 in comparison+summary, got: $totalRow" }
}

Test-Case "comparison on string field filters out all lines" {
    $out = Get-Content $metricsSample | & $exe --format-config $metricsConfig -f "service>0" --output plain
    if ($out.Count -ne 0) { throw "Expected 0 lines when comparing string field, got $($out.Count)" }
}

Test-Case "comparison on non-existent field filters out all lines" {
    $out = Get-Content $metricsSample | & $exe --format-config $metricsConfig -f "nonexistent>0" --output plain
    if ($out.Count -ne 0) { throw "Expected 0 lines when comparing non-existent field, got $($out.Count)" }
}

Test-Case "int field equal match" {
    $out = Get-Content $metricsSample | & $exe --format-config $metricsConfig -f "status=200" --output plain
    if ($out.Count -ne 7) { throw "Expected 7 lines, got $($out.Count)" }
}

Test-Case "int field equal no match" {
    $out = Get-Content $metricsSample | & $exe --format-config $metricsConfig -f "status=999" --output plain
    if ($out.Count -ne 0) { throw "Expected 0 lines, got $($out.Count)" }
}

Test-Case "float field equal match" {
    $out = Get-Content $metricsSample | & $exe --format-config $metricsConfig -f "resp_time=0.5" --output plain
    if ($out.Count -ne 1) { throw "Expected 1 line, got $($out.Count)" }
}

Test-Case "int equal combined with level filter" {
    $out = Get-Content $metricsSample | & $exe --format-config $metricsConfig -f "status=200" -f "level=DEBUG" --output plain
    if ($out.Count -ne 2) { throw "Expected 2 lines, got $($out.Count)" }
}

Write-Host ""
Write-Host "[8] Pipe mode (stdin)" -ForegroundColor Yellow

Test-Case "pipe from Get-Content" {
    $out = Get-Content $sample | & $exe -f module=planner
    if ($out.Count -ne 6) { throw "Expected 6 lines, got $($out.Count)" }
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Results: $passed passed, $failed failed" -ForegroundColor $(if ($failed -eq 0) { "Green" } else { "Red" })
Write-Host "========================================" -ForegroundColor Cyan

exit $failed
