# Converts the web client's Roboto Condensed woff2 fonts into TTF files under
# Content/fonts/ so axmol's FreeType renderer can load them. The web client ships
# only woff2 (`client/public/fonts/*.woff2`), which axmol cannot parse.
#
# Requires Python with `fontTools` + `brotli` (`pip install fonttools brotli`).
#
# Usage:  pwsh tools/fetch-fonts.ps1
$ErrorActionPreference = 'Stop'
$root   = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$srcDir = (Resolve-Path (Join-Path $root '..\client\public\fonts')).Path
$dstDir = Join-Path $root 'Content\fonts'
New-Item -ItemType Directory -Force -Path $dstDir | Out-Null

$map = @{
    'roboto-condensed-latin-400-normal.woff2' = 'RobotoCondensed-Normal.ttf'
    'roboto-condensed-latin-700-normal.woff2' = 'RobotoCondensed-Bold.ttf'
}

$script = @'
from fontTools.ttLib import TTFont
import sys
src, dst = sys.argv[1], sys.argv[2]
font = TTFont(src)
font.flavor = None
font.save(dst)
'@

foreach ($pair in $map.GetEnumerator()) {
    $src = Join-Path $srcDir $pair.Key
    if (-not (Test-Path $src)) { throw "Missing font: $src" }
    $dst = Join-Path $dstDir $pair.Value
    python -c $script $src $dst
    if ($LASTEXITCODE -ne 0) { throw "fontTools failed for $($pair.Key)" }
    Write-Host "wrote $($pair.Value)"
}
