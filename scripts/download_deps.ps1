$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectDir = Split-Path -Parent $ScriptDir
Set-Location $ProjectDir

$ProgressPreference = 'SilentlyContinue'

Write-Host "Downloading third-party dependencies..." -ForegroundColor Cyan
Write-Host ""

New-Item -ItemType Directory -Force -Path third_party | Out-Null

Write-Host "[1/2] CLI11.hpp (v2.4.2)..." -ForegroundColor Yellow
Invoke-WebRequest -Uri "https://github.com/CLIUtils/CLI11/releases/download/v2.4.2/CLI11.hpp" `
    -OutFile "third_party/CLI11.hpp" -TimeoutSec 60
Write-Host "  OK" -ForegroundColor Green

Write-Host "[2/2] json.hpp (v3.11.3)..." -ForegroundColor Yellow
Invoke-WebRequest -Uri "https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp" `
    -OutFile "third_party/json.hpp" -TimeoutSec 60
Write-Host "  OK" -ForegroundColor Green

Write-Host ""
Write-Host "All dependencies downloaded. Ready to build!" -ForegroundColor Green
Write-Host "  Run: .\scripts\build.ps1" -ForegroundColor White
