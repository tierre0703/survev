# Environment for building the Survev Android app with axmol.
# Fully offline / self-contained: the axmol engine is vendored at ../axmol and
# the Android SDK's own CMake (3.22.1) + ninja are used (NOT the system CMake).
#
# Dot-source it before any build command:
#   . .\tools\android-env.ps1
$root = Split-Path $PSScriptRoot -Parent

$env:AX_ROOT          = Join-Path $root 'axmol'
$env:ANDROID_HOME     = "E:\work\survev-tools\Sdk"
$env:ANDROID_SDK_ROOT = "E:\work\survev-tools\Sdk"
$env:ANDROID_NDK_HOME = "E:\work\survev-tools\Sdk\ndk\27.1.12297006"
$env:ANDROID_NDK_ROOT = "E:\work\survev-tools\Sdk\ndk\27.1.12297006"
$env:JAVA_HOME        = "C:\Program Files\Eclipse Adoptium\jdk-21.0.11.10-hotspot"

# The Android SDK's bundled CMake + ninja live here. Putting this first makes
# `cmake` resolve to 3.22.1 (SDK) instead of the system 4.4.
$sdkCmakeBin = Join-Path $env:ANDROID_HOME 'cmake\3.22.1\bin'
$env:SURV_CMAKE  = Join-Path $sdkCmakeBin 'cmake.exe'
$env:SURV_NINJA  = Join-Path $sdkCmakeBin 'ninja.exe'

$prepend = @(
    (Join-Path $env:JAVA_HOME 'bin'),
    (Join-Path $env:ANDROID_HOME 'platform-tools'),
    $sdkCmakeBin
)
$env:PATH = ($prepend -join ';') + ';' + $env:PATH

Write-Host "AX_ROOT=$env:AX_ROOT"
Write-Host "ANDROID_HOME=$env:ANDROID_HOME"
Write-Host "JAVA_HOME=$env:JAVA_HOME"
Write-Host "SURV_CMAKE=$env:SURV_CMAKE"
Write-Host "SURV_NINJA=$env:SURV_NINJA"
