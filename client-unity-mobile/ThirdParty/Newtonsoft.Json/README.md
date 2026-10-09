# Newtonsoft.Json (vendored)

Fast JSON for `l10n/*.json`, API responses and atlas frame indexes.

| | |
|---|---|
| Origin | https://github.com/JamesNK/Newtonsoft.Json |
| Version | **13.0.4** |
| Tag | `13.0.4` |
| License | MIT (see `LICENSE`) |
| Form | source (library only, no tests) |
| Vendored path | `ThirdParty/Newtonsoft.Json/` |

## Layout / import

Only the library source is vendored (`Src/Newtonsoft.Json/**`). Gradle-style
solution/build files and the test suite are not. Unity auto-compiles the `.cs`
files.

The source uses `#if` guards for `NETFX`/`NETSTANDARD`/`NET6_0_OR_GREATER`;
those symbols are undefined in Unity, so it compiles for the .NET Standard 2.0
profile. No `unsafe` code is present (only doc comments mention "unsafe").

## Refresh

```powershell
.\tools\fetch-thirdparty.ps1 -Force
```

The script pins the tag above and vendors `Src/Newtonsoft.Json`; update it there
(and this README) when bumping. `LICENSE` is copied automatically.
