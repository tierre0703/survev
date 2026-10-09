# Survev Unity Mobile

Unity **2020.3 LTS** Android port of the Survev web client (`../client`), written
in C#. It is a **1:1 conversion**: the same UI/UX, styles, look and feel, assets
and numbers as the web client.

- **Architecture, module mapping, decisions and milestones:** see [`plan.md`](plan.md).
- **Setup, build and workflow:** see [`AGENTS.md`](AGENTS.md).

## Quick start

```powershell
cd client-unity-mobile

.\tools\sync-assets.ps1                 # stage ../client/public -> Assets/StreamingAssets
node tools\build-atlas.mjs --res low    # web atlas cache -> Unity sprite sheets
node --experimental-transform-types tools\codegen-defs.mjs   # shared/defs -> C#
.\tools\fetch-thirdparty.ps1            # vendor offline libraries (ThirdParty/)
```

Then open the folder with **Unity Hub (Editor 2020.3.x)** and let it import.

## Build (Android)

```powershell
& "C:\Program Files\Unity\Hub\Editor\2020.3.x\Editor\Unity.exe" `
    -batchmode -quit -projectPath . `
    -executeMethod Survev.EditorTools.BuildScript.BuildAndroid -logFile build.log
# -> Builds/SurvevMobile.apk
```

## Layout

```
Assets/Scripts/    C# game + UI code (Core, Net, Game, Render, UI, Config)
Assets/Scenes/     Bootstrap, Menu, Game
Assets/Editor/     build scripts + asset importers
Assets/Tests/      EditMode/PlayMode tests
Assets/StreamingAssets/   staged web assets (gitignored)
ThirdParty/        vendored offline libraries
tools/             staging / atlas / codegen / fetch scripts
```

## Status

Scaffold only: project skeleton, tooling, and the first ported primitives
(`BitBuffer`, `Vec2`, `MathUtil`, `ConfigManager`, `Localization`, connection /
render adapter stubs). Milestones M0–M9 are tracked in `plan.md`.
