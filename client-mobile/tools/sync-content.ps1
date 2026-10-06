# Stages the web client's static assets into ./Content (the APK asset payload).
# Copies ../client/public/* so the game images/audio/fonts/l10n ship in the APK.
$ErrorActionPreference = 'Stop'
$root   = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$source = (Resolve-Path (Join-Path $root '..\client\public')).Path
$dst    = Join-Path $root 'Content'

if (-not (Test-Path $source)) { throw "Missing web assets: $source" }
Write-Host "Staging assets: $source -> $dst"
robocopy $source $dst /E /NFL /NDL /NJH /NJS /NP /R:1 /W:1 | Out-Null
if ($LASTEXITCODE -ge 8) { throw "robocopy failed ($LASTEXITCODE)" }
$sz = (Get-ChildItem $dst -Recurse -File | Measure-Object -Property Length -Sum).Sum
Write-Host ("done: {0:N1} MB" -f ($sz / 1MB))
