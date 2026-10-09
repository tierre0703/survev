// Transport factory so the app/tests pick a WebSocket backend without naming it.
//
//   WebSocketTransport.Create(connection)  -> websocket-sharp (default, Android)
//   WebSocketTransport.CreateBuiltin(...)   -> System.Net.WebSockets fallback
namespace Survev.Net
{
    /// <summary>Factory for the selectable WebSocket transport.</summary>
    public static class WebSocketTransport
    {
        /// <summary>Default transport (websocket-sharp).</summary>
        public static Connection.ITransport Create(Connection connection) =>
            new WebSocketSharpTransport(connection);

        /// <summary>Built-in System.Net.WebSockets transport.</summary>
        public static Connection.ITransport CreateBuiltin(Connection connection) =>
            new ClientWebSocketTransport(connection);

        /// <summary>True when the URL scheme is secure ("wss").</summary>
        public static bool IsSecure(string url) =>
            url != null && url.StartsWith("wss", System.StringComparison.OrdinalIgnoreCase);
    }
}
