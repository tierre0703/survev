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
# Usage:
#   powershell -File tools/build-apk.ps1 [-Abis arm64-v8a,x86_64]
#   powershell -File tools/build-apk.ps1 -Release -KeystorePath upload.jks `
#       -KeystoreAlias upload -KeystorePassword $env:SURVEV_KS_PASS `
#       -ApiUrl https://survev.io
param(
    # Empty = all store ABIs for debug, arm64-v8a for release.
    [string[]]$Abis = @(),
    # Signed release build (non-debug keystore, version from package.json).
    [switch]$Release,
    [string]$VersionName = "",
    [int]$VersionCode = 0,
    # Release API origin; substituted for the manifest `surv_apiUrl`.
    [string]$ApiUrl = "",
    [string]$KeystorePath = "",
    [string]$KeystoreAlias = "",
    [string]$KeystorePassword = "",
    [string]$KeyPassword = "",
    [string]$Output = ""
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

if ($Abis.Count -eq 0) {
    $Abis = if ($Release) { @('arm64-v8a') } else { @('armeabi-v7a', 'arm64-v8a') }
}

# Version defaults come from the repo package.json (0.4.3 -> 403); a release
# build must bump package.json or pass -VersionCode (Play versions are monotonic).
$pkgVersion = '1.0'
try {
    $pkg = Get-Content (Join-Path $root '..\package.json') -Raw | ConvertFrom-Json
    if ($pkg.version) { $pkgVersion = [string]$pkg.version }
} catch {}
if (-not $VersionName) { $VersionName = $pkgVersion }
if ($VersionCode -le 0) {
    $parts = @($pkgVersion.Split('.') | ForEach-Object { [int]($_ -replace '[^0-9]', '') })
    $major = if ($parts.Count -gt 0) { $parts[0] } else { 1 }
    $minor = if ($parts.Count -gt 1) { $parts[1] } else { 0 }
    $patch = if ($parts.Count -gt 2) { $parts[2] } else { 0 }
    $VersionCode = ($major * 10000) + ($minor * 100) + $patch
}
if ($VersionCode -le 0) { $VersionCode = 1 }

# Signing: explicit args win over environment (CI-friendly, no secrets in argv
# on disk; apksigner still receives them via env: below).
if (-not $KeystorePath) { $KeystorePath = $env:SURVEV_KEYSTORE }
if (-not $KeystoreAlias) { $KeystoreAlias = $env:SURVEV_KS_ALIAS }
if (-not $KeystorePassword) { $KeystorePassword = $env:SURVEV_KS_PASS }
if (-not $KeyPassword) { $KeyPassword = $env:SURVEV_KEY_PASS }
if ($Release) {
    if (-not $ApiUrl) { $ApiUrl = 'https://survev.io' }
    if (-not $KeystorePath) {
        throw "Release build needs -KeystorePath (or `$env:SURVEV_KEYSTORE)"
    }
    if (-not (Test-Path $KeystorePath)) { $KeystorePath = Join-Path $root $KeystorePath }
    if (-not (Test-Path $KeystorePath)) { throw "Keystore not found: $KeystorePath" }
    if (-not $KeystoreAlias -or -not $KeystorePassword) {
        throw "Release build needs -KeystoreAlias/-KeystorePassword (or `$env:SURVEV_KS_ALIAS/`$env:SURVEV_KS_PASS)"
    }
    if (-not $KeyPassword) { $KeyPassword = $KeystorePassword }
}
$ApiUrl = $ApiUrl.Trim()
Write-Host "== config =="
Write-Host "   abis=$($Abis -join ',') version=$VersionName ($VersionCode) release=$([bool]$Release)"
if ($ApiUrl) { Write-Host "   api=$ApiUrl" }

. (Join-Path $PSScriptRoot 'android-env.ps1')

$bt         = Join-Path $env:ANDROID_HOME 'build-tools\35.0.0'
$androidJar = Join-Path $env:ANDROID_HOME 'platforms\android-36\android.jar'
$ndk        = Join-Path $env:ANDROID_HOME 'ndk\27.1.12297006'
$cxxShared  = Join-Path $ndk 'toolchains\llvm\prebuilt\windows-x86_64\sysroot\usr\lib\aarch64-linux-android\libc++_shared.so'
$java       = Join-Path $env:JAVA_HOME 'bin\java.exe'
$javac      = Join-Path $env:JAVA_HOME 'bin\javac.exe'
$keytool    = Join-Path $env:JAVA_HOME 'bin\keytool.exe'
$jar        = Join-Path $env:JAVA_HOME 'bin\jar.exe'
$strip      = Join-Path $ndk 'toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-strip.exe'

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
if ($ApiUrl) {
    $mf = $mf -replace '(android:name="surv_apiUrl"\s+android:value=")[^"]*(")',
                       ('${1}' + $ApiUrl + '${2}')
}
Set-Content -Path (Join-Path $work 'AndroidManifest.xml') -Value $mf -NoNewline

$resZip = Join-Path $work 'res.zip'
$baseApk = Join-Path $work 'base.apk'
& (Join-Path $bt 'aapt2.exe') compile --dir (Join-Path $root 'proj.android\app\res') -o $resZip
if ($LASTEXITCODE -ne 0) { throw "aapt2 compile failed" }
& (Join-Path $bt 'aapt2.exe') link -o $baseApk -I $androidJar `
    --manifest (Join-Path $work 'AndroidManifest.xml') $resZip --java $genDir `
    --min-sdk-version 24 --target-sdk-version 36 --version-code $VersionCode --version-name "$VersionName" --auto-add-overlay
if ($LASTEXITCODE -ne 0) { throw "aapt2 link failed" }

Write-Host "== 2/7 java (javac) =="
# Vendored Java UI dependencies (OKHttp + Okio; see ARCHITECTURE.md §6.2 and
# proj.android/app/libs/README.md). The Gradle-free path links them by
# classpath instead of resolving Maven coordinates.
$libsJar = Join-Path $root 'proj.android\app\libs'
$libsClasspath = ''
if (Test-Path $libsJar) {
    $vendored = Get-ChildItem $libsJar -Filter *.jar | Select-Object -ExpandProperty FullName
    if ($vendored.Count -gt 0) {
        $libsClasspath = ($vendored -join ';')
        Write-Host "   vendored jars: $($vendored.Count)"
    }
}
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
# gen dir is empty after aapt2; skip if empty
$genFiles = Get-ChildItem $genDir -Recurse -Filter *.java -ErrorAction SilentlyContinue
if ($genFiles.Count -gt 0) { $srcs += $genFiles | Select-Object -ExpandProperty FullName }
Write-Host "   $($srcs.Count) source files"

$argsFile = Join-Path $work 'javac.args'
$srcs | ForEach-Object { '"' + ($_ -replace '\\', '/') + '"' } | Set-Content $argsFile
$expJar = Join-Path $axJava 'libs\com.android.vending.expansion.zipfile.jar'
& $javac -source 8 -target 8 -nowarn -classpath "$androidJar;$expJar;$libsClasspath" -d $clsDir "@$argsFile"
if ($LASTEXITCODE -ne 0) { throw "javac failed" }

Write-Host "== 3/7 dex (d8) =="
$clsJar = Join-Path $work 'classes.jar'
& $jar cf $clsJar -C $clsDir .
# Include the vendored Java UI jars in the dex so the OkHttp classes ship in
# the APK (the Gradle-free pipeline does not resolve them from Maven).
$d8Inputs = @($clsJar)
if (Test-Path $libsJar) {
    $d8Inputs += Get-ChildItem $libsJar -Filter *.jar | Select-Object -ExpandProperty FullName
}
& $java -cp (Join-Path $bt 'lib\d8.jar') com.android.tools.r8.D8 --lib $androidJar --min-api 24 --output $dexDir @d8Inputs
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
    $openal = Join-Path $root "$b\lib\libopenal.so"
    if (-not (Test-Path $openal)) { throw "Missing libopenal for ${abi}: $openal (build it first)" }
    if ($Release) {
        # Store builds strip debug symbols; keep the build outputs untouched by
        # stripping copies in build-apk/stripped/<abi>/.
        $stripDir = Join-Path $work "stripped\$abi"
        New-Dir $stripDir
        $soStripped = Join-Path $stripDir 'libSurvevMobile.so'
        $openalStripped = Join-Path $stripDir 'libopenal.so'
        Copy-Item $so $soStripped -Force
        Copy-Item $openal $openalStripped -Force
        & $strip --strip-unneeded $soStripped $openalStripped
        if ($LASTEXITCODE -ne 0) { throw "llvm-strip failed for $abi" }
        $so = $soStripped
        $openal = $openalStripped
    }
    $cxx = Join-Path $ndk "toolchains\llvm\prebuilt\windows-x86_64\sysroot\usr\lib\$($abiMap[$abi].sysroot)\libc++_shared.so"
    Add-Entry $so "lib/$abi/libSurvevMobile.so" ([System.IO.Compression.CompressionLevel]::NoCompression)
    Add-Entry $openal "lib/$abi/libopenal.so" ([System.IO.Compression.CompressionLevel]::NoCompression)
    Add-Entry $cxx "lib/$abi/libc++_shared.so" ([System.IO.Compression.CompressionLevel]::NoCompression)
}
$contentRoot = Join-Path $root 'Content'
Get-ChildItem $contentRoot -Recurse -File | Where-Object { $_.Extension -ne '.gz' } | ForEach-Object {
    $rel = $_.FullName.Substring($contentRoot.Length + 1).Replace('\', '/')
    Add-Entry $_.FullName "assets/$rel" ([System.IO.Compression.CompressionLevel]::Optimal)
}

# Precompiled shaders. The engine build (axslcc) writes them to
# proj.android/build/runtime/axslc; the Gradle build copies them to
# assets/axslc (see proj.android/app/build.gradle). Without them axmol compiles
# empty shaders, every vertex attribute is "not exist", and the screen is black.
$axslcDir = Join-Path $root 'proj.android\build\runtime\axslc'
if (-not (Test-Path $axslcDir)) {
    throw "Missing axslc shader cache: $axslcDir (build the native lib first)"
}
Get-ChildItem $axslcDir -Recurse -File | ForEach-Object {
    $rel = $_.FullName.Substring($axslcDir.Length + 1).Replace('\', '/')
    Add-Entry $_.FullName "assets/axslc/$rel" ([System.IO.Compression.CompressionLevel]::Optimal)
}
$zip.Dispose()
Write-Host ("   unsigned.apk: {0:N1} MB" -f ((Get-Item $unsigned).Length / 1MB))

Write-Host "== 5/7 zipalign =="
$aligned = Join-Path $work "aligned-$runStamp.apk"
& (Join-Path $bt 'zipalign.exe') -p -f 4 $unsigned $aligned
if ($LASTEXITCODE -ne 0) { throw "zipalign failed" }

Write-Host "== 6/7 sign =="
if ($Release) {
    $signed = if ($Output) { Join-Path $root $Output } else { Join-Path $root 'SurvevMobile-release.apk' }
    if (Test-Path $signed) { Remove-Item $signed -Force }
    # apksigner reads the secrets from the environment (env: prefix), keeping
    # them out of the command line/logs.
    $env:SURVEV_APK_KS_PASS = $KeystorePassword
    $env:SURVEV_APK_KEY_PASS = $KeyPassword
    & $java -jar (Join-Path $bt 'lib\apksigner.jar') sign --ks $KeystorePath `
        --ks-key-alias $KeystoreAlias --ks-pass env:SURVEV_APK_KS_PASS `
        --key-pass env:SURVEV_APK_KEY_PASS --out $signed $aligned
    $signExit = $LASTEXITCODE
    Remove-Item Env:SURVEV_APK_KS_PASS, Env:SURVEV_APK_KEY_PASS -ErrorAction SilentlyContinue
    if ($signExit -ne 0) { throw "apksigner failed" }
} else {
    $ks = Join-Path $work 'debug.keystore'
    if (-not (Test-Path $ks)) {
        & $keytool -genkeypair -keystore $ks -storepass android -keypass android -alias androiddebugkey `
            -dname "CN=Android Debug,O=Android,C=US" -keyalg RSA -keysize 2048 -validity 10000
        if ($LASTEXITCODE -ne 0) { throw "keytool failed" }
    }
    $signed = if ($Output) { Join-Path $root $Output } else { Join-Path $root 'SurvevMobile-debug.apk' }
    if (Test-Path $signed) { Remove-Item $signed -Force }
    & $java -jar (Join-Path $bt 'lib\apksigner.jar') sign --ks $ks --ks-pass pass:android --key-pass pass:android --out $signed $aligned
    if ($LASTEXITCODE -ne 0) { throw "apksigner failed" }
}

Write-Host "== 7/7 verify =="
if ($Release) {
    & $java -jar (Join-Path $bt 'lib\apksigner.jar') verify --verbose --print-certs $signed
} else {
    & $java -jar (Join-Path $bt 'lib\apksigner.jar') verify --verbose $signed
}
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
