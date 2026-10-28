# Adds the createJS muffled/club 10-band biquad EQ to the vendored axmol audio
# engine (PCM filtering path + AudioPlayerSettings::eq).
#
# The low-pass patch (patch-axmol-audio-filter.ps1) stays as the fallback for
# streams/unsupported formats. This patch replaces the affected core/audio
# files with the pinned axmol version used by this repo; run it from
# tools/vendor-axmol.ps1 after the low-pass patch.
param(
    [string]$AxmolRoot = (Join-Path $PSScriptRoot '..\axmol')
)
$ErrorActionPreference = 'Stop'

$src = Join-Path $PSScriptRoot '..\patches\axmol-audio-eq'
$dst = Join-Path $AxmolRoot 'core\audio'
if (-not (Test-Path (Join-Path $dst 'AudioEngine.h'))) {
    throw "Not an axmol tree: $AxmolRoot"
}

$files = @(
    'AudioEq.h',
    'AudioEngine.h',
    'AudioEngine.cpp',
    'AudioEngineImpl.h',
    'AudioEngineImpl.cpp',
    'AudioPlayer.h',
    'AudioPlayer.cpp',
    'AudioCache.h',
    'AudioCache.cpp'
)
foreach ($file in $files) {
    Copy-Item (Join-Path $src $file) (Join-Path $dst $file) -Force
}
Write-Host 'patched axmol: PCM 10-band EQ (muffled/club)'
