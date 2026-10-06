# Gradle-free APK build for the Survev axmol app.
#
# Gradle cannot run in this environment because Astrill's Winsock LSP breaks
# Java NIO (the Gradle daemon uses TCP loopback selectors). This script drives
# the Android build tools directly:
#   CMake+Ninja native lib  ->  aapt2  ->  javac  ->  d8  ->  zip  ->  zipalign  ->  apksigner
#
# Prerequisites: run tools/android-env.ps1 values (AX_ROOT / ANDROID_HOME / JAVA_HOME)
# and build the native lib first:
#   cmake --build build-android --config Release
#
# Usage:  pwsh/powershell -File tools/build-apk.ps1 [-Abis arm64-v8a,x86_64]
param(
    [string[]]$Abis = @('armeabi-v7a', 'arm64-v8a')
)
# Native tools (aapt2/javac/d8/zipalign/apksigner) write notes/progress to
# stderr, which PowerShell turns into a terminating error under 'Stop'. Every
# native invocation is checked via $LASTEXITCODE below, so keep the preference
# at Continue and fail explicitly instead.
$ErrorActionPreference = 'Continue'
if (Get-Variable -Name PSNativeCommandUseErrorActionPreference -ErrorAction SilentlyContinue) {
    $PSNativeCommandUseErrorActionPreference = $false
}
# normalize -Abis (supports "arm64-v8a,x86_64" or separate args)
$Abis = @($Abis | ForEach-Object { $_ -split ',' } | Where-Object { $_ -ne '' })

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
Set-Location $root
[Environment]::CurrentDirectory = $root

. (Join-Path $PSScriptRoot 'android-env.ps1')

$bt         = Join-Path $env:ANDROID_HOME 'build-tools\35.0.0'
$androidJar = Join-Path $env:ANDROID_HOME 'platforms\android-36\android.jar'
$ndk        = Join-Path $env:ANDROID_HOME 'ndk\27.1.12297006'
$cxxShared  = Join-Path $ndk 'toolchains\llvm\prebuilt\windows-x86_64\sysroot\usr\lib\aarch64-linux-android\libc++_shared.so'
$java       = Join-Path $env:JAVA_HOME 'bin\java.exe'
$javac      = Join-Path $env:JAVA_HOME 'bin\javac.exe'
$keytool    = Join-Path $env:JAVA_HOME 'bin\keytool.exe'
$jar        = Join-Path $env:JAVA_HOME 'bin\jar.exe'

$work    = Join-Path $root 'build-apk'
$genDir  = Join-Path $work 'gen'
$clsDir  = Join-Path $work 'classes'
$dexDir  = Join-Path $work 'dex'
$stubDir = Join-Path $work 'javastub'
$axJava  = Join-Path $env:AX_ROOT 'core\platform\android\java'

function New-Dir($p) { if (-not (Test-Path $p)) { New-Item -ItemType Directory -Path $p -Force | Out-Null } }
New-Dir $work; New-Dir $genDir; New-Dir $clsDir; New-Dir $dexDir

Write-Host "== 1/7 resources (aapt2) =="
# packaging manifest: add package + extractNativeLibs (original kept clean for Gradle)
$mf = Get-Content (Join-Path $root 'proj.android\app\AndroidManifest.xml') -Raw
$mf = $mf -replace '<manifest xmlns:android="http://schemas.android.com/apk/res/android"',
                   '<manifest xmlns:android="http://schemas.android.com/apk/res/android" package="com.survev.mobile"'
$mf = $mf -replace 'android:allowBackup="true"',
                   'android:allowBackup="true" android:extractNativeLibs="true"'
Set-Content -Path (Join-Path $work 'AndroidManifest.xml') -Value $mf -NoNewline

$resZip = Join-Path $work 'res.zip'
$baseApk = Join-Path $work 'base.apk'
& (Join-Path $bt 'aapt2.exe') compile --dir (Join-Path $root 'proj.android\app\res') -o $resZip
if ($LASTEXITCODE -ne 0) { throw "aapt2 compile failed" }
& (Join-Path $bt 'aapt2.exe') link -o $baseApk -I $androidJar `
    --manifest (Join-Path $work 'AndroidManifest.xml') $resZip --java $genDir `
    --min-sdk-version 24 --target-sdk-version 36 --version-code 1 --version-name 1.0 --auto-add-overlay
if ($LASTEXITCODE -ne 0) { throw "aapt2 link failed" }

Write-Host "== 2/7 java (javac) =="
# Stub the media engine (video only). The full media3/ExoPlayer implementation
# is excluded from this Gradle-free build.
$stub = Join-Path $stubDir 'dev\axmol\lib'
New-Dir $stub
@'
package dev.axmol.lib;
import android.content.Context;
public class AxmolMediaEngine {
    public static void setContext(Context context) {}
    public static Object createMediaEngine() { return null; }
}
'@ | Set-Content (Join-Path $stub 'AxmolMediaEngine.java')

$srcs = @()
$srcs += Get-ChildItem (Join-Path $axJava 'src') -Recurse -Filter *.java |
    Where-Object { $_.Name -notin @('AxmolMediaEngine.java', 'MediaCodecVideoRenderer.java') } |
    Select-Object -ExpandProperty FullName
$srcs += Get-ChildItem (Join-Path $root 'proj.android\app\src') -Recurse -Filter *.java | Select-Object -ExpandProperty FullName
$srcs += Join-Path $stub 'AxmolMediaEngine.java'
$srcs += Get-ChildItem $genDir -Recurse -Filter *.java | Select-Object -ExpandProperty FullName
Write-Host "   $($srcs.Count) source files"

$argsFile = Join-Path $work 'javac.args'
$srcs | ForEach-Object { '"' + ($_ -replace '\\', '/') + '"' } | Set-Content $argsFile
$expJar = Join-Path $axJava 'libs\com.android.vending.expansion.zipfile.jar'
& $javac -source 8 -target 8 -nowarn -classpath "$androidJar;$expJar" -d $clsDir "@$argsFile"
if ($LASTEXITCODE -ne 0) { throw "javac failed" }

Write-Host "== 3/7 dex (d8) =="
$clsJar = Join-Path $work 'classes.jar'
& $jar cf $clsJar -C $clsDir .
& $java -cp (Join-Path $bt 'lib\d8.jar') com.android.tools.r8.D8 --lib $androidJar --min-api 24 --output $dexDir $clsJar
if ($LASTEXITCODE -ne 0) { throw "d8 failed" }

Write-Host "== 4/7 assemble apk =="
Add-Type -AssemblyName System.IO.Compression.FileSystem
Add-Type -AssemblyName System.IO.Compression
# Unique intermediates per run: a stale handle on a previous run's APK (a
# crashed build, an editor, or AV holding it) must never block a rebuild.
$runStamp = [Guid]::NewGuid().ToString('N').Substring(0, 8)
$unsigned = Join-Path $work "unsigned-$runStamp.apk"
# Start from aapt2's base.apk: it already stores resources.arsc UNCOMPRESSED,
# which Android R+ (targetSdk >= 30) requires. Do NOT rebuild the zip from
# scratch with .NET's ZipArchive: CompressionLevel::NoCompression still emits a
# deflate stream (method 8), so the platform rejects the install with
# "resources.arsc ... stored uncompressed and aligned on a 4-byte boundary".
# Updating base.apk in place preserves the stored resources.arsc; zipalign then
# aligns it. See https://developer.android.com/about/versions/11/behavior-changes-11#apk-signing
Copy-Item $baseApk $unsigned -Force -ErrorAction Stop
$zip = [System.IO.Compression.ZipFile]::Open($unsigned, [System.IO.Compression.ZipArchiveMode]::Update)
if (-not $zip) { throw "failed to open $unsigned for update" }
function Add-Entry($src, $name, $level) {
    $existing = $zip.GetEntry($name)
    if ($existing) { $existing.Delete() }
    $entry = $zip.CreateEntry($name, $level)
    $es = $entry.Open()
    $fs = [System.IO.File]::OpenRead($src)
    $fs.CopyTo($es)
    $es.Close(); $fs.Close()
}
Add-Entry (Join-Path $dexDir 'classes.dex') 'classes.dex' ([System.IO.Compression.CompressionLevel]::Optimal)
# native libs per ABI
$abiMap = @{
    'armeabi-v7a' = @{ build = 'build-android-armeabi-v7a'; sysroot = 'arm-linux-androideabi' }
    'arm64-v8a'   = @{ build = 'build-android-arm64-v8a';   sysroot = 'aarch64-linux-android' }
    'x86_64'      = @{ build = 'build-android-x86_64';       sysroot = 'x86_64-linux-android' }
}
foreach ($abi in $Abis) {
    if (-not $abiMap.ContainsKey($abi)) { throw "Unsupported ABI: $abi" }
    $b = $abiMap[$abi].build
    $so = Join-Path $root "$b\libSurvevMobile.so"
    if (-not (Test-Path $so)) { throw "Missing native lib for ${abi}: $so (build it first)" }
    $cxx = Join-Path $ndk "toolchains\llvm\prebuilt\windows-x86_64\sysroot\usr\lib\$($abiMap[$abi].sysroot)\libc++_shared.so"
    Add-Entry $so "lib/$abi/libSurvevMobile.so" ([System.IO.Compression.CompressionLevel]::NoCompression)
    Add-Entry (Join-Path $root "$b\lib\libopenal.so") "lib/$abi/libopenal.so" ([System.IO.Compression.CompressionLevel]::NoCompression)
    Add-Entry $cxx "lib/$abi/libc++_shared.so" ([System.IO.Compression.CompressionLevel]::NoCompression)
}
$contentRoot = Join-Path $root 'Content'
Get-ChildItem $contentRoot -Recurse -File | ForEach-Object {
    $rel = $_.FullName.Substring($contentRoot.Length + 1).Replace('\', '/')
    Add-Entry $_.FullName "assets/$rel" ([System.IO.Compression.CompressionLevel]::Optimal)
}
$zip.Dispose()
Write-Host ("   unsigned.apk: {0:N1} MB" -f ((Get-Item $unsigned).Length / 1MB))

Write-Host "== 5/7 zipalign =="
$aligned = Join-Path $work "aligned-$runStamp.apk"
& (Join-Path $bt 'zipalign.exe') -p -f 4 $unsigned $aligned
if ($LASTEXITCODE -ne 0) { throw "zipalign failed" }

Write-Host "== 6/7 sign =="
$ks = Join-Path $work 'debug.keystore'
if (-not (Test-Path $ks)) {
    & $keytool -genkeypair -keystore $ks -storepass android -keypass android -alias androiddebugkey `
        -dname "CN=Android Debug,O=Android,C=US" -keyalg RSA -keysize 2048 -validity 10000
    if ($LASTEXITCODE -ne 0) { throw "keytool failed" }
}
$signed = Join-Path $root 'SurvevMobile-debug.apk'
if (Test-Path $signed) { Remove-Item $signed -Force }
& $java -jar (Join-Path $bt 'lib\apksigner.jar') sign --ks $ks --ks-pass pass:android --key-pass pass:android --out $signed $aligned
if ($LASTEXITCODE -ne 0) { throw "apksigner failed" }

Write-Host "== 7/7 verify =="
& $java -jar (Join-Path $bt 'lib\apksigner.jar') verify --verbose $signed
if ($LASTEXITCODE -ne 0) { throw "apksigner verify failed" }

# Android R+ requires resources.arsc stored uncompressed and 4-byte aligned.
# Catch the regression at build time instead of at adb install time.
$alignOut = & (Join-Path $bt 'zipalign.exe') -c -v 4 $signed 2>&1
$arscLine = $alignOut | Select-String -Pattern 'resources\.arsc' | Select-Object -First 1
Write-Host "   $arscLine"
if (-not $arscLine -or ($arscLine -match 'compressed|BAD')) {
    throw "resources.arsc must be stored uncompressed and 4-byte aligned (got: $arscLine)"
}

# Intermediates are per-run; clean them up (best-effort, ignore stale locks).
Remove-Item $unsigned, $aligned -Force -ErrorAction SilentlyContinue

Write-Host ("`nAPK: {0}  ({1:N1} MB)" -f $signed, ((Get-Item $signed).Length / 1MB))
