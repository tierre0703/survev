# Stages the web client's static assets into Assets/StreamingAssets so the Unity
# Android build ships the exact same images/audio/localization as ./client.
#
# Mirrors client/public/{img,audio,l10n,fonts} and adds English (the web bundle
# imports client/src/en.json rather than storing it under public/l10n).
#
# Assets/StreamingAssets/ is gitignored; run this on a fresh checkout before
# opening/building the project.
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$source = (Resolve-Path (Join-Path $root '..\client\public')).Path
$dst = Join-Path $root 'Assets\StreamingAssets'

if (-not (Test-Path $source)) { throw "Missing web assets: $source" }
Write-Host "Staging assets: $source -> $dst"
robocopy $source $dst /E /NFL /NDL /NJH /NJS /NP /R:1 /W:1 | Out-Null
if ($LASTEXITCODE -ge 8) { throw "robocopy failed ($LASTEXITCODE)" }

# English is imported by the web bundle, not stored under public/l10n.
New-Item -ItemType Directory -Force -Path (Join-Path $dst 'l10n') | Out-Null
Copy-Item (Join-Path $root '..\client\src\en.json') (Join-Path $dst 'l10n\en.json') -Force

$sz = (Get-ChildItem $dst -Recurse -File | Measure-Object -Property Length -Sum).Sum
Write-Host ("done: {0:N1} MB" -f ($sz / 1MB))