// Mobile WebSocket transport (Android).
//
// The web client uses the browser WebSocket with binaryType="arraybuffer". On
// Unity there are two options, and both are kept behind Connection.ITransport so
// either can be swapped without touching the game code:
//
//   WebSocketSharpTransport  (default)  — vendored sta/websocket-sharp
//   ClientWebSocketTransport (fallback) — built-in System.Net.WebSockets
//
// Both receive on a background thread and push raw frames into
// Connection.PushFrame(); the game decodes them from Update() via Pump().
//
// To use the built-in instead, construct ClientWebSocketTransport. The vendored
// library is preferred on Android because it avoids the older .NET profile's
// ClientWebSocket limitations (TLS + fragmented frames) on some devices.
using System;

namespace Survev.Net
{
    /// <summary>Factory so the app/tests pick the transport without naming it.</summary>
    public static class WebSocketTransport
    {
        /// <summary>Creates the default (websocket-sharp) transport for a connection.</summary>
        public static Connection.ITransport Create(Connection connection) =>
            new WebSocketSharpTransport(connection);

        /// <summary>Creates the built-in System.Net.WebSockets transport.</summary>
        public static Connection.ITransport CreateBuiltin(Connection connection) =>
            new ClientWebSocketTransport(connection);

        /// <summary>Scheme used by the game socket ("ws"/"wss").</summary>
        public static bool IsSecure(string url) =>
            url.StartsWith("wss", StringComparison.OrdinalIgnoreCase);
    }
}
