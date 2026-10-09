# Throwaway M7 on-device probe: sweeps screen taps and captures a synthetic
# touch + key sweep so the pause menu's wiring can be checked without guessing
# scene->screen coordinates. Not part of the build.
$ErrorActionPreference = 'Continue'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'android-env.ps1')
$adb = Join-Path $env:ANDROID_HOME 'platform-tools\adb.exe'

function Snap($name) {
    & $adb shell screencap -p "/sdcard/$name.png" | Out-Null
    & $adb pull "/sdcard/$name.png" (Join-Path $root "$name.png") | Out-Null
}

& $adb shell am force-stop com.survev.mobile
& $adb logcat -c
& $adb shell am start -n com.survev.mobile/dev.axmol.app.AppActivity | Out-Null
Start-Sleep -Seconds 11
Snap 'p0-menu'

# Open the loadout via the button that works on-device.
& $adb shell input tap 335 378
Start-Sleep -Seconds 3
Snap 'p1-loadout'
# Sweep taps inside the modal to find the close button.
foreach ($x in 700, 760, 800, 830, 839, 860) {
    & $adb shell input tap $x 50
    Start-Sleep -Milliseconds 700
    Snap "p2-close-$x"
}
# Click the last cell in the emote grid row 1 to prove item selection works.
& $adb shell input tap 335 378
Start-Sleep -Seconds 2
foreach ($x in 200, 320, 440, 560, 680, 800) {
    & $adb shell input tap $x 225
    Start-Sleep -Milliseconds 700
    Snap "p3-cell-$x"
}
Write-Host 'probe done'
