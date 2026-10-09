# Third-party libraries (offline)

Everything the Unity project needs beyond the pinned Unity packages lives here as
source or as an IL2CPP-safe `.dll`. The project must build with **no network
access**; `tools/fetch-thirdparty.ps1` vendors each pinned revision and verifies a
hash for a fresh machine. The vendored trees are **committed** so a checkout is
self-contained.

> The Unity packages themselves are pinned in `../Packages/manifest.json` and
> cached by the Editor — they are not duplicated here.

## Vendored now

| Name | Purpose | Form | Version / ref | Path |
|---|---|---|---|---|
| UniTask | Allocation-free async for net + asset loading | source (UPM package) | 2.5.11 (`2e993ff…`) | `UniTask/` |
| websocket-sharp | Mobile-safe WebSocket transport | source (library) | master `f7904e6…` | `WebSocketSharp/` |
| Newtonsoft.Json | Fast JSON for `l10n`/API/atlas frames | source (library) | 13.0.4 | `Newtonsoft.Json/` |

Each folder has its own `README.md` with origin, exact revision, license and
refresh instructions. All three are MIT.

- **UniTask** is referenced as a local package from `Packages/manifest.json`:
  `"com.cysharp.unitask": "file:../ThirdParty/UniTask"`.
- **websocket-sharp** and **Newtonsoft.Json** are source trees Unity compiles
  in-place (no `.dll`). If a prebuilt `.dll` is ever preferred, put it in
  `ThirdParty/<name>/lib/` and it must target the Unity 2020.3 / .NET Standard
  2.0 profile.

## Not vendored (by design)

- **Unity built-ins** — `System.Net.WebSockets.ClientWebSocket`,
  `UnityWebRequest`, `AudioSource` cover the transport/HTTP/audio needs. The
  vendored libs are selected where they are more reliable or more capable on
  Android, and are kept behind interfaces so the built-ins remain a fallback.
- **Ad/SpellSync SDKs** — browser glue, out of scope (plan.md D6).

## Layout

```
ThirdParty/
  <name>/
    README.md      origin URL, license, exact version/commit
    LICENSE        copied from the release by the fetch script
    .vendored      marker written by the fetch script (gitignored)
    <source files>
```

## Rules

- One folder per library; never mix two libraries in a folder.
- Record the **exact** revision (tag + commit hash) and license in each README.
- Prefer source over prebuilt `.dll` where practical (IL2CPP + arm64 safety).
- A `.dll` must be built against the Unity 2020.3 / .NET Standard 2.0 profile.
- Update this table and `plan.md` §7 when a library is added or bumped.
- `tools/fetch-thirdparty.ps1` is idempotent, verifies hashes, and prefers a
  locally-provided `ThirdParty/<name>.zip` over downloading (offline machines).
