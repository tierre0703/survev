# Vendors the axmol engine into ./axmol so the project builds fully offline
# (no AX_ROOT env dependency, no network fetches at configure time).
#
# - copies the engine excluding the large desktop `build/` artifacts
# - lowers 1k/fetch.cmake's cmake_minimum_required so the Android SDK's
#   bundled CMake 3.22.1 can configure it (axmol 2.11.5 ships 3.23)
#
# Usage:
#   powershell -File tools/vendor-axmol.ps1 [-Source E:\path\to\axmol-2.11.5]
param(
    [string]$Source = 'E:\work\survev-tools\axmol-2.11.5'
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$dst  = Join-Path $root 'axmol'

if (-not (Test-Path (Join-Path $Source 'core\CMakeLists.txt'))) {
    throw "Not an axmol checkout: $Source"
}

Write-Host "Vendoring axmol: $Source -> $dst"
if (Test-Path $dst) { Remove-Item -Recurse -Force $dst }
# /XD with absolute paths excludes only the top-level heavy dirs.
robocopy $Source $dst /E /XD "$Source\build" "$Source\docs" "$Source\tests" "$Source\.git" /NFL /NDL /NJH /NJS /NP /R:1 /W:1 | Out-Null
if ($LASTEXITCODE -ge 8) { throw "robocopy failed ($LASTEXITCODE)" }

# Patch: allow the Android SDK CMake 3.22.1 to configure 3rdparty/fetch.
$fetch = Join-Path $dst '1k\fetch.cmake'
$text = Get-Content $fetch -Raw
$patched = $text -replace 'cmake_minimum_required\(VERSION 3\.23\.\.\.4\.0\)', 'cmake_minimum_required(VERSION 3.22...4.0)'
if ($patched -ne $text) {
    Set-Content -Path $fetch -Value $patched -NoNewline
    Write-Host "patched 1k/fetch.cmake -> cmake_minimum_required 3.22"
} else {
    Write-Host "note: 1k/fetch.cmake already patched (or pattern changed)"
}

$sz = (Get-ChildItem $dst -Recurse -File | Measure-Object -Property Length -Sum).Sum
Write-Host ("done: {0:N0} MB" -f ($sz / 1MB))
