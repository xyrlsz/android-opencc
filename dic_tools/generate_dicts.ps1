param(
    [string]$BuildDir = "build",
    [switch]$Force,
    [string]$OpenCC = ""
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

# Delegate to Python script
$PyArgs = @(
    "--build-dir", $BuildDir
)
if ($Force) { $PyArgs += "--force" }
if ($OpenCC) { $PyArgs += "--opencc", $OpenCC }

Write-Host "=== Running generate_dicts.py ===" -ForegroundColor Cyan
& python3 "$ScriptDir\generate_dicts.py" $PyArgs

if ($LASTEXITCODE -ne 0) {
    throw "Dictionary generation failed (exit code: $LASTEXITCODE)"
}

Write-Host "`nDone." -ForegroundColor Green
