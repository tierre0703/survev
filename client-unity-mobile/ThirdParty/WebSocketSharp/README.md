# websocket-sharp (vendored)

Mobile-safe WebSocket client; used by `Assets/Scripts/Net/WebSocketTransport.cs`
as the robust fallback / primary transport (the built-in `ClientWebSocket` is the
other path, both behind `Connection.ITransport`).

| | |
|---|---|
| Origin | https://github.com/sta/websocket-sharp |
| Branch | master |
| Commit | `f7904e6afb934cfff9eda1f29ee6c1291b14c4bc` |
| License | MIT (see `LICENSE`) |
| Form | source (C# library) |
| Vendored path | `ThirdParty/WebSocketSharp/` |

## Layout / import

`WebSocketSharp/` is the library source tree (`websocket-sharp.csproj`, `Net/`,
`Server/`, `Ext.cs`, ...). Unity auto-compiles the `.cs` files; edit
`AssemblyInfo.cs` if the assembly name must change.

> The upstream `.csproj` is not compiled by Unity; it is kept for reference.

## Refresh

```powershell
.\tools\fetch-thirdparty.ps1 -Force
```

The script pins the commit above; update it there (and this README) when
bumping. `LICENSE` is copied automatically.
