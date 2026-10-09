// websocket-sharp-backed transport (the default).
//
// Port of the WebSocket plumbing the browser provides for
// shared/net/connection.ts. Frames arrive on a background thread and are queued
// into Connection.Enqueue(); Connection.Pump() dispatches them on the main
// thread. Uses the vendored library in ThirdParty/WebSocketSharp.
using WebSocketSharp;

namespace Survev.Net
{
    /// <summary>Connection.ITransport implemented with sta/websocket-sharp.</summary>
    public sealed class WebSocketSharpTransport : Connection.ITransport
    {
        private readonly Connection _connection;
        private WebSocket _socket;

        public WebSocketSharpTransport(Connection connection)
        {
            _connection = connection;
        }

        public void Connect(string url)
        {
            _socket = new WebSocket(url);
            _socket.OnOpen += (_, _) => _connection.NotifyOpen();
            _socket.OnMessage += (_, e) =>
            {
                // Binary frames only; the protocol is never text.
                if (e.IsBinary && e.RawData != null)
                {
                    _connection.Enqueue(e.RawData);
                }
            };
            _socket.OnError += (_, e) => _connection.NotifyError();
            _socket.OnClose += (_, e) => _connection.NotifyClose(e.Code, e.Reason);
            _socket.ConnectAsync();
        }

        public void Send(byte[] data)
        {
            if (_socket != null && _socket.IsAlive)
            {
                _socket.Send(data);
            }
        }

        public void Close()
        {
            if (_socket == null)
            {
                return;
            }
            _socket.Close();
            _socket = null;
        }
    }
}
