# Sets up the environment for building the Survev Android app with axmol.
# Dot-source it before invoking gradlew / cmake:
#   . .\tools\android-env.ps1
$env:AX_ROOT         = "E:\work\survev-tools\axmol-2.11.5"
$env:ANDROID_HOME    = "E:\work\survev-tools\Sdk"
$env:ANDROID_SDK_ROOT= "E:\work\survev-tools\Sdk"
$env:ANDROID_NDK_HOME= "E:\work\survev-tools\Sdk\ndk\27.1.12297006"
$env:ANDROID_NDK_ROOT= "E:\work\survev-tools\Sdk\ndk\27.1.12297006"
$env:JAVA_HOME       = "C:\Program Files\Eclipse Adoptium\jdk-21.0.11.10-hotspot"

$prepend = @(
    "$env:JAVA_HOME\bin",
    "$env:ANDROID_HOME\platform-tools",
    "$env:ANDROID_HOME\cmake\3.22.1\bin"
)
$env:PATH = ($prepend -join ';') + ';' + $env:PATH
Write-Host "AX_ROOT=$env:AX_ROOT"
Write-Host "ANDROID_HOME=$env:ANDROID_HOME"
Write-Host "JAVA_HOME=$env:JAVA_HOME"
