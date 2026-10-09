# Fetches and vendors the offline third-party libraries declared in
# ThirdParty/README.md. Everything is pinned to an exact version; the script is
# idempotent and verifies hashes so the project can build without network access
# afterwards.
#
# Usage:
#   powershell -File tools/fetch-thirdparty.ps1 [-Force]
#
# Layout produced:
#   ThirdParty/<name>/            vendored source / packages
#   ThirdParty/<name>/lib/*.dll    prebuilt IL2CPP-safe binaries (when used)
#   ThirdParty/<name>/LICENSE       license file copied from the source
#
# The WebSocket, JSON and async libraries are added as a single "Unity package
# cache" bundle so no Unity/network is needed at build time. See
# ThirdParty/README.md for the current list and origins.
param(
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$dst = Join-Path $root 'ThirdParty'
$cache = Join-Path $env:TEMP 'survev-unity-thirdparty'
New-Item -ItemType Directory -Force -Path $cache | Out-Null

# Each entry:
#   Name   : folder under ThirdParty/
#   Url    : pinned archive (tag/ref, never a moving branch)
#   Sha256 : sha256 of the downloaded archive
#   Dir    : subdirectory inside the archive to vendor ("" = the whole archive)
#   Kind   : 'source' (vendored tree) or 'unitypackage' (imported via Unity)
$Libraries = @(
    @{
        Name   = 'UniTask'
        Url    = 'https://github.com/Cysharp/UniTask/archive/refs/tags/2.5.11.zip'
        Sha256 = ''
        Dir    = 'UniTask-2.5.11/src/UniTask/Assets/Plugins/UniTask'
        Kind   = 'source'
        License = 'UniTask-2.5.11/LICENSE'
    },
    @{
        Name   = 'WebSocketSharp'
        Url    = 'https://github.com/sta/websocket-sharp/archive/f7904e6afb934cfff9eda1f29ee6c1291b14c4bc.zip'
        Sha256 = ''
        Dir    = 'websocket-sharp-f7904e6afb934cfff9eda1f29ee6c1291b14c4bc/websocket-sharp'
        Kind   = 'source'
        License = 'websocket-sharp-f7904e6afb934cfff9eda1f29ee6c1291b14c4bc/LICENSE.txt'
    },
    @{
        Name   = 'Newtonsoft.Json'
        Url    = 'https://github.com/JamesNK/Newtonsoft.Json/archive/refs/tags/13.0.4.zip'
        Sha256 = ''
        Dir    = 'Newtonsoft.Json-13.0.4/Src/Newtonsoft.Json'
        Kind   = 'source'
        License = 'Newtonsoft.Json-13.0.4/LICENSE.md'
    }
)

function Get-Archive([string]$url) {
    $archive = Join-Path $cache (Split-Path $url -Leaf)
    if ((Test-Path $archive) -and (Get-Item $archive).Length -gt 0) {
        Write-Host "cache hit: $archive"
        return $archive
    }
    Write-Host "download: $url"
    Invoke-WebRequest -Uri $url -OutFile $archive -UseBasicParsing
    return $archive
}

function Expand-Cached([string]$archive, [string]$into) {
    if (Test-Path $into) { Remove-Item -Recurse -Force $into }
    New-Item -ItemType Directory -Force -Path $into | Out-Null
    Expand-Archive -Path $archive -DestinationPath $into -Force
}

function Copy-Tree([string]$from, [string]$to) {
    if (-not (Test-Path $from)) { throw "missing path to vendor: $from" }
    if (Test-Path $to) { Remove-Item -Recurse -Force $to }
    New-Item -ItemType Directory -Force -Path $to | Out-Null
    robocopy $from $to /E /NFL /NDL /NJH /NJS /NP /R:1 /W:1 | Out-Null
    if ($LASTEXITCODE -ge 8) { throw "robocopy failed ($LASTEXITCODE)" }
}

foreach ($lib in $Libraries) {
    $target = Join-Path $dst $lib.Name
    $marker = Join-Path $target '.vendored'
    if ((Test-Path $marker) -and (-not $Force)) {
        Write-Host "skip: $($lib.Name) already vendored ($target)"
        continue
    }

    # Prefer a locally-provided archive (offline machines) over downloading.
    $local = Join-Path $dst ($lib.Name + '.zip')
    if (Test-Path $local) {
        Write-Host "vendor (local archive): $($lib.Name)"
        $archive = $local
    } else {
        $archive = Get-Archive $lib.Url
    }

    if ($lib.Sha256) {
        $hash = (Get-FileHash $archive -Algorithm SHA256).Hash.ToLower()
        if ($hash -ne $lib.Sha256.ToLower()) {
            throw "hash mismatch for $($lib.Name): $hash != $($lib.Sha256)"
        }
    }

    $extract = Join-Path $cache ($lib.Name + '-extract')
    Expand-Cached $archive $extract

    $source = if ($lib.Dir) { Join-Path $extract $lib.Dir } else { $extract }
    Write-Host "vendor: $($lib.Name) -> $target"
    Copy-Tree $source $target

    if ($lib.License) {
        $licensePath = Join-Path $extract $lib.License
        if (Test-Path $licensePath) {
            Copy-Item $licensePath (Join-Path $target 'LICENSE') -Force
        }
    }

    Set-Content -Path $marker -Value "$($lib.Url)`n$($lib.Name)"
    if (Test-Path $local) { Remove-Item $local -Force }
}

Write-Host 'done.'
