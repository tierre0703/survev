# Builds a Play Store app bundle (AAB) from the Gradle-free release APK.
#
# The Gradle-free pipeline cannot create an AAB directly, and Gradle itself
# cannot run here (Astrill's Winsock LSP breaks Java NIO). This script assembles
# the `base` module in the format bundletool expects:
#
#   manifest/AndroidManifest.xml   (protobuf, from aapt2 link --proto-format)
#   resources.pb                   (protobuf resource table)
#   res/**                         (flat proto resources)
#   dex/classes.dex                (from the release APK)
#   lib/<abi>/**                   (stripped native libs from the release APK)
#   assets/**                      (Content + axslc from the release APK)
#
# then calls `bundletool build-bundle`. The release APK is the source of the dex,
# libs and assets, so build it first:
#
#   .\tools\build-apk.ps1 -Release -ApiUrl https://survev.io -KeystorePath upload.jks `
#       -KeystoreAlias upload -KeystorePassword ... -KeyPassword ...
#   .\tools\build-aab.ps1 -BundletoolJar C:\tools\bundletool-all.jar
#
# Version, API URL and ABI list are read from the release APK unless overridden.
param(
    [string[]]$Abis = @(),
    [string]$ApiUrl = "",
    [string]$VersionName = "",
    [int]$VersionCode = 0,
    # Input release APK (defaults to ../SurvevMobile-release.apk).
    [string]$Apk = "",
    # Output AAB (defaults to ../SurvevMobile-release.aab).
    [string]$Output = "",
    # bundletool-all.jar; falls back to $env:BUNDLETOOL_JAR and common paths.
    [string]$BundletoolJar = "",
    # Optional upload-key signing for the AAB (Play accepts unsigned bundles but
    # signing locally matches Android Studio's "Generate Signed Bundle").
    [string]$KeystorePath = "",
    [string]$KeystoreAlias = "",
    [string]$KeystorePassword = "",
    [string]$KeyPassword = ""
)
$ErrorActionPreference = 'Continue'
if (Get-Variable -Name PSNativeCommandUseErrorActionPreference -ErrorAction SilentlyContinue) {
    $PSNativeCommandUseErrorActionPreference = $false
}
Add-Type -AssemblyName System.IO.Compression.FileSystem
Add-Type -AssemblyName System.IO.Compression

$Abis = @($Abis | ForEach-Object { $_ -split ',' } | Where-Object { $_ -ne '' })
if ($Abis.Count -eq 0) { $Abis = @('arm64-v8a') }

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
Set-Location $root
[Environment]::CurrentDirectory = $root
. (Join-Path $PSScriptRoot 'android-env.ps1')

$bt         = Join-Path $env:ANDROID_HOME 'build-tools\35.0.0'
$androidJar = Join-Path $env:ANDROID_HOME 'platforms\android-36\android.jar'
$java       = Join-Path $env:JAVA_HOME 'bin\java.exe'
$jarsigner  = Join-Path $env:JAVA_HOME 'bin\jarsigner.exe'
$keytool    = Join-Path $env:JAVA_HOME 'bin\keytool.exe'
$aapt2      = Join-Path $bt 'aapt2.exe'

$work = Join-Path $root 'build-aab'
$stage = Join-Path $work 'base'
$resZip = Join-Path $work 'res.zip'
$protoApk = Join-Path $work 'base-proto.apk'
$baseZip = Join-Path $work 'base.zip'
if (Test-Path $work) { Remove-Item $work -Recurse -Force -ErrorAction SilentlyContinue }
New-Item -ItemType Directory -Path $work -Force | Out-Null

function New-Dir($p) { if (-not (Test-Path $p)) { New-Item -ItemType Directory -Path $p -Force | Out-Null } }

if (-not $Apk) { $Apk = Join-Path $root 'SurvevMobile-release.apk' }
if (-not (Test-Path $Apk)) {
    throw "Missing release APK: $Apk (run tools/build-apk.ps1 -Release ... first)"
}
$Apk = (Resolve-Path $Apk).Path

# Defaults from the release APK so the AAB and APK are the same app.
$badging = (& $aapt2 dump badging $Apk 2>&1) -join "`n"
if (-not $VersionName -and ($badging -match "versionName='([^']+)'")) { $VersionName = $matches[1] }
if ($VersionCode -le 0 -and ($badging -match "versionCode='(\d+)'")) { $VersionCode = [int]$matches[1] }
if (-not $ApiUrl) {
    $xmlTree = @(& $aapt2 dump xmltree --file AndroidManifest.xml $Apk 2>&1)
    $idx = ($xmlTree | Select-String -Pattern 'surv_apiUrl' | Select-Object -First 1)
    if ($idx -and ($idx.LineNumber -lt $xmlTree.Count)) {
        $next = $xmlTree[$idx.LineNumber] # 1-based LineNumber -> next array entry
        if ($next -match 'Raw: "([^"]+)"') { $ApiUrl = $matches[1] }
    }
}
if (-not $VersionName -or $VersionCode -le 0) {
    throw "Could not read version from $Apk; pass -VersionName/-VersionCode"
}
Write-Host "== config =="
Write-Host "   apk=$Apk"
Write-Host "   abis=$($Abis -join ',') version=$VersionName ($VersionCode) api=$ApiUrl"

# The release APK must contain the dex, assets, libs and all requested ABIs.
$zip = [System.IO.Compression.ZipFile]::OpenRead($Apk)
try {
    foreach ($abi in $Abis) {
        $entry = $zip.GetEntry("lib/$abi/libSurvevMobile.so")
        if (-not $entry) {
            throw "Release APK has no lib/$abi/libSurvevMobile.so (build it with -Abis $($Abis -join ','))"
        }
    }
    if (-not $zip.GetEntry('classes.dex')) { throw "Release APK has no classes.dex" }
} finally {
    $zip.Dispose()
}

# --- proto manifest + resources -------------------------------------------
Write-Host "== 1/4 proto resources (aapt2) =="
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

& $aapt2 compile --dir (Join-Path $root 'proj.android\app\res') -o $resZip
if ($LASTEXITCODE -ne 0) { throw "aapt2 compile failed" }
& $aapt2 link --proto-format -o $protoApk -I $androidJar `
    --manifest (Join-Path $work 'AndroidManifest.xml') $resZip `
    --min-sdk-version 24 --target-sdk-version 36 `
    --version-code $VersionCode --version-name "$VersionName" --auto-add-overlay
if ($LASTEXITCODE -ne 0) { throw "aapt2 link --proto-format failed" }

# --- module staging --------------------------------------------------------
Write-Host "== 2/4 stage base module =="
New-Dir $stage
[System.IO.Compression.ZipFile]::ExtractToDirectory($protoApk, $stage)
New-Dir (Join-Path $stage 'manifest')
New-Dir (Join-Path $stage 'dex')
Move-Item (Join-Path $stage 'AndroidManifest.xml') (Join-Path $stage 'manifest\AndroidManifest.xml') -Force
# Remove anything the proto APK carries that is not part of a bundle module.
foreach ($junk in @('META-INF')) {
    $p = Join-Path $stage $junk
    if (Test-Path $p) { Remove-Item $p -Recurse -Force }
}

$zip = [System.IO.Compression.ZipFile]::OpenRead($Apk)
try {
    foreach ($e in $zip.Entries) {
        if ($e.FullName.EndsWith('/')) { continue }
        $dest = $null
        if ($e.FullName -match '^classes[0-9]*\.dex$') {
            $dest = Join-Path $stage ('dex\' + $e.FullName)
        } elseif ($e.FullName.StartsWith('assets/') -or $e.FullName.StartsWith('lib/')) {
            $dest = Join-Path $stage ($e.FullName -replace '/', '\')
        }
        if ($dest) {
            New-Dir (Split-Path $dest -Parent)
            [System.IO.Compression.ZipFileExtensions]::ExtractToFile($e, $dest, $true)
        }
    }
} finally {
    $zip.Dispose()
}
if (-not (Test-Path (Join-Path $stage 'dex\classes.dex'))) {
    throw "No classes.dex staged from $Apk"
}

# --- bundle module zip (forward slashes; bundletool rejects '\\' entries) ---
Write-Host "== 3/4 module zip =="
if (Test-Path $baseZip) { Remove-Item $baseZip -Force }
$z = [System.IO.Compression.ZipFile]::Open($baseZip, [System.IO.Compression.ZipArchiveMode]::Create)
try {
    foreach ($f in Get-ChildItem $stage -Recurse -File) {
        $rel = $f.FullName.Substring($stage.Length + 1).Replace('\', '/')
        $entry = $z.CreateEntry($rel, [System.IO.Compression.CompressionLevel]::Optimal)
        $out = $entry.Open()
        $in = [System.IO.File]::OpenRead($f.FullName)
        $in.CopyTo($out)
        $in.Close(); $out.Close()
    }
} finally {
    $z.Dispose()
}

# --- bundletool ------------------------------------------------------------
if (-not $BundletoolJar) { $BundletoolJar = $env:BUNDLETOOL_JAR }
if (-not $BundletoolJar) {
    $candidates = @(
        (Join-Path $env:USERPROFILE '.gradle\caches\modules-2\files-2.1\com.android.tools.build\bundletool'),
        (Join-Path $env:ANDROID_HOME 'bundletool'),
        (Join-Path $root 'tools')
    )
    foreach ($c in $candidates) {
        if (Test-Path $c) {
            $hit = Get-ChildItem $c -Recurse -Filter 'bundletool*.jar' -ErrorAction SilentlyContinue |
                Select-Object -First 1
            if ($hit) { $BundletoolJar = $hit.FullName; break }
        }
    }
}
if (-not $BundletoolJar -or -not (Test-Path $BundletoolJar)) {
    throw "bundletool-all.jar not found. Pass -BundletoolJar <path> or set `$env:BUNDLETOOL_JAR (https://github.com/google/bundletool/releases)."
}

$signedOut = if ($Output) { Join-Path $root $Output } else { Join-Path $root 'SurvevMobile-release.aab' }
if (Test-Path $signedOut) { Remove-Item $signedOut -Force }
Write-Host "== 4/4 build-bundle =="
& $java -jar $BundletoolJar build-bundle --modules=$baseZip --output=$signedOut --overwrite
if ($LASTEXITCODE -ne 0) { throw "bundletool build-bundle failed" }

# Optional upload-key (JAR) signing, matching Android Studio's signed bundle.
if (-not $KeystorePath) { $KeystorePath = $env:SURVEV_KEYSTORE }
if (-not $KeystoreAlias) { $KeystoreAlias = $env:SURVEV_KS_ALIAS }
if (-not $KeystorePassword) { $KeystorePassword = $env:SURVEV_KS_PASS }
if (-not $KeyPassword) { $KeyPassword = $env:SURVEV_KEY_PASS }
if ($KeystorePath) {
    if (-not (Test-Path $KeystorePath)) { $KeystorePath = Join-Path $root $KeystorePath }
    if (-not (Test-Path $KeystorePath)) { throw "Keystore not found: $KeystorePath" }
    if (-not $KeystoreAlias -or -not $KeystorePassword) {
        throw "AAB signing needs -KeystoreAlias/-KeystorePassword (or env SURVEV_KS_ALIAS/SURVEV_KS_PASS)"
    }
    if (-not $KeyPassword) { $KeyPassword = $KeystorePassword }
    $env:SURVEV_AAB_KS_PASS = $KeystorePassword
    $env:SURVEV_AAB_KEY_PASS = $KeyPassword
    & $jarsigner -keystore $KeystorePath -storepass:env SURVEV_AAB_KS_PASS `
        -keypass:env SURVEV_AAB_KEY_PASS -digestalg SHA-256 -sigalg SHA256withRSA `
        $signedOut $KeystoreAlias
    $signExit = $LASTEXITCODE
    Remove-Item Env:SURVEV_AAB_KS_PASS, Env:SURVEV_AAB_KEY_PASS -ErrorAction SilentlyContinue
    if ($signExit -ne 0) { throw "jarsigner failed" }
    Write-Host "   AAB signed with $KeystoreAlias"
} else {
    Write-Host "   AAB is unsigned (Play Console signs with the app signing key; pass -KeystorePath to upload-sign)"
}

Write-Host ("`nAAB: {0}  ({1:N1} MB)" -f $signedOut, ((Get-Item $signedOut).Length / 1MB))
Write-Host "Verify/local install:"
Write-Host "  java -jar $BundletoolJar build-apks --bundle=$signedOut --output=$($signedOut -replace '\.aab$', '.apks') --mode=universal --ks=<keystore> --ks-key-alias=<alias> --ks-pass=env:SURVEV_KS_PASS"
