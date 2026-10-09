# UniTask (vendored)

Unity async/await integration; used for allocation-free net + asset loading.

| | |
|---|---|
| Origin | https://github.com/Cysharp/UniTask |
| Version | **2.5.11** |
| Commit | `2e993ff18f28c931602a07292df0b0804eebef99` |
| License | MIT (see `LICENSE`) |
| Form | source (Unity package) |
| Vendored path | `ThirdParty/UniTask/` |

## Layout

`UniTask/` is the package root (`package.json`, `Runtime/`, `Editor/`) as shipped
by the release. Import it as a local package from `Packages/manifest.json`:

```json
"com.cysharp.unitask": "file:../ThirdParty/UniTask"
```

## Refresh

```powershell
.\tools\fetch-thirdparty.ps1 -Force
```

The script pins the tag/ref above; update it there (and this README) when
bumping. `LICENSE` is copied from the release automatically.
