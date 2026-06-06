param(
    [ValidateSet("Debug", "Release")]
    [string]$BuildType = "Release",

    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectDir = Split-Path -Parent $ScriptDir
Set-Location $ProjectDir

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  log-query Build Script (Windows)" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

function Test-Command($cmd) {
    try { Get-Command $cmd -ErrorAction Stop | Out-Null; return $true }
    catch { return $false }
}

Write-Host "[1/4] Checking prerequisites..." -ForegroundColor Yellow

if (-not (Test-Command cmake)) {
    Write-Host "ERROR: cmake not found. Please install CMake >= 3.16" -ForegroundColor Red
    Write-Host "  Download: https://cmake.org/download/" -ForegroundColor Red
    exit 1
}
$cmakeVer = (cmake --version | Select-Object -First 1) -replace "cmake version ", ""
Write-Host "  cmake : $cmakeVer" -ForegroundColor Green

if (-not (Test-Command g++)) {
    Write-Host "ERROR: g++ not found. Please install MinGW-w64" -ForegroundColor Red
    Write-Host "  Download: https://www.mingw-w64.org/" -ForegroundColor Red
    exit 1
}
$gccVer = (g++ --version | Select-Object -First 1)
Write-Host "  GCC   : $gccVer" -ForegroundColor Green

if (-not (Test-Path "third_party/CLI11.hpp") -or -not (Test-Path "third_party/json.hpp")) {
    Write-Host "ERROR: third-party headers not found." -ForegroundColor Red
    Write-Host "  Run: .\scripts\download_deps.ps1  to download them first." -ForegroundColor Red
    exit 1
}
Write-Host "  deps  : OK (CLI11.hpp + json.hpp)" -ForegroundColor Green
Write-Host ""

$Generator = "MinGW Makefiles"

if ($Clean) {
    Write-Host "[*] Cleaning build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
}

if (Test-Path "build/CMakeCache.txt") {
    $cacheContent = Get-Content "build/CMakeCache.txt" -Raw
    if ($cacheContent -notmatch "MinGW Makefiles") {
        Write-Host "[*] Generator changed, cleaning old build cache..." -ForegroundColor Yellow
        Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
    }
}

Write-Host "[2/4] Configuring CMake..." -ForegroundColor Yellow
$cmakeArgs = @("-B", "build", "-G", $Generator,
               "-DCMAKE_BUILD_TYPE=$BuildType")
& cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: CMake configuration failed!" -ForegroundColor Red
    exit 1
}
Write-Host ""

Write-Host "[3/4] Building..." -ForegroundColor Yellow
& cmake --build build --config $BuildType
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Build failed!" -ForegroundColor Red
    exit 1
}
Write-Host ""

Write-Host "[4/4] Build complete!" -ForegroundColor Green
$exePath = Join-Path $ProjectDir "build\log-query.exe"
Write-Host "  Binary: $exePath" -ForegroundColor Green
Write-Host ""

Write-Host "Quick test:" -ForegroundColor Cyan
Write-Host "  echo '[2024-01-15 14:32:01.123] [ERROR] [lidar] [rx] timeout' | $exePath -f module=lidar -f level=ERROR" -ForegroundColor White
