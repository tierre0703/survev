# Builds the native libSurvevMobile.so for one or more Android ABIs.
# Uses the Android SDK's bundled CMake 3.22.1 + ninja (offline, no system CMake)
# and the axmol engine vendored at ../axmol.
#
# Usage:
#   powershell -File tools/build-native.ps1 -Abis "armeabi-v7a,arm64-v8a"
param(
    [string[]]$Abis = @('armeabi-v7a', 'arm64-v8a')
)
# CMake/Ninja write warnings to stderr, which PowerShell turns into a
# terminating error under 'Stop'. Every invocation is checked via
# $LASTEXITCODE below, so keep the preference at Continue and fail explicitly.
$ErrorActionPreference = 'Continue'
if (Get-Variable -Name PSNativeCommandUseErrorActionPreference -ErrorAction SilentlyContinue) {
    $PSNativeCommandUseErrorActionPreference = $false
}
$Abis = @($Abis | ForEach-Object { $_ -split ',' } | Where-Object { $_ -ne '' })

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
Set-Location $root
[Environment]::CurrentDirectory = $root
. (Join-Path $PSScriptRoot 'android-env.ps1')

$ndk  = $env:ANDROID_NDK_HOME
$proj = Join-Path $root 'proj.android'

$map = @{
    'armeabi-v7a' = 'build-android-armeabi-v7a'
    'arm64-v8a'   = 'build-android-arm64-v8a'
    'x86_64'      = 'build-android-x86_64'
}

foreach ($abi in $Abis) {
    if (-not $map.ContainsKey($abi)) { throw "Unsupported ABI: $abi" }
    $b = $map[$abi]
    Write-Host "== configure $abi -> $b =="
    & $env:SURV_CMAKE -S . -B $b -G Ninja `
        -DCMAKE_TOOLCHAIN_FILE="$ndk/build/cmake/android.toolchain.cmake" `
        "-DANDROID_ABI=$abi" -DANDROID_PLATFORM=android-24 -DANDROID_STL=c++_shared `
        -DANDROID_TOOLCHAIN=clang -DANDROID_ARM_NEON=TRUE `
        -DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON -DANDROID_USE_LEGACY_TOOLCHAIN_FILE=false `
        -DCMAKE_MAKE_PROGRAM="$env:SURV_NINJA" -DCMAKE_BUILD_TYPE=Release `
        -D_AX_ANDROID_PROJECT_DIR="$proj"
    if ($LASTEXITCODE -ne 0) { throw "configure failed for $abi" }

    Write-Host "== build $abi =="
    & $env:SURV_CMAKE --build $b --config Release
    if ($LASTEXITCODE -ne 0) { throw "build failed for $abi" }
}
Write-Host "native build complete: $($Abis -join ', ')"
