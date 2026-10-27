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
# English is imported by the web bundle, not stored under public/l10n.
New-Item -ItemType Directory -Force -Path (Join-Path $dst 'l10n') | Out-Null
Copy-Item (Join-Path $root '..\client\src\en.json') (Join-Path $dst 'l10n\en.json') -Force

# Rasterize the web client's SVG GUI art (public/img/gui/*.svg) so axmol can
# draw the same buttons/icons (Content/gui/<name>.png).
Write-Host 'Rasterizing GUI icons (Content/gui)...'
& node (Join-Path $PSScriptRoot 'build-gui-icons.mjs') --scale 4
if ($LASTEXITCODE -ne 0) { throw "build-gui-icons.mjs failed ($LASTEXITCODE)" }

# Crosshair styles are inline SVG in shared/defs (not public/img); rasterize
# each style's three stroke buckets for the screen-space crosshair sprite.
Write-Host 'Rasterizing crosshair icons (Content/gui/crosshair)...'
& node --experimental-transform-types (Join-Path $PSScriptRoot 'build-crosshair-icons.mjs')
if ($LASTEXITCODE -ne 0) { throw "build-crosshair-icons.mjs failed ($LASTEXITCODE)" }

# Loadout item art (public/img/loot|emotes|particles|crosshairs/*.svg) for the
# Java customize screen.
Write-Host 'Rasterizing loadout item icons (Content/img/loadout)...'
& node --experimental-transform-types (Join-Path $PSScriptRoot 'build-loadout-icons.mjs')
if ($LASTEXITCODE -ne 0) { throw "build-loadout-icons.mjs failed ($LASTEXITCODE)" }

# Quick-play mode icons (MapDef.desc.icon) for the Java menu.
Write-Host 'Rasterizing menu mode icons (Content/img/menu)...'
& node --experimental-transform-types (Join-Path $PSScriptRoot 'build-menu-icons.mjs')
if ($LASTEXITCODE -ne 0) { throw "build-menu-icons.mjs failed ($LASTEXITCODE)" }

# The web client ships Roboto Condensed as woff2, which axmol cannot load; keep
# a TTF copy under Content/fonts (requires Python + fontTools/brotli).
if (Test-Path (Join-Path $source 'fonts\roboto-condensed-latin-400-normal.woff2')) {
    Write-Host 'Staging Roboto Condensed TTF (Content/fonts)...'
    & pwsh -NoProfile -File (Join-Path $PSScriptRoot 'fetch-fonts.ps1')
    if ($LASTEXITCODE -ne 0) { Write-Warning 'fetch-fonts.ps1 failed; UI falls back to the system font' }
}

$sz = (Get-ChildItem $dst -Recurse -File | Measure-Object -Property Length -Sum).Sum
Write-Host ("done: {0:N1} MB" -f ($sz / 1MB))
