// websocket-sharp-backed transport (the default).
//
// Port of the WebSocket plumbing the browser provides for
// shared/net/connection.ts. Frames arrive on a background thread and are pushed
// into Connection.PushFrame(); Connection.Pump() decodes them on the main thread.
//
// Uses the vendored library in ThirdParty/WebSocketSharp (see ThirdParty/README.md).
using System;
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
            _socket.OnOpen += (_, __) =>
            {
                _connection.SetState(ConnectionState.Connected);
                _connection.RaiseOpen();
            };
            _socket.OnMessage += (_, e) =>
            {
                // Binary frames only; the protocol is never text.
                if (e.IsBinary && e.RawData != null)
                {
                    _connection.PushFrame(e.RawData);
                }
            };
            _socket.OnError += (_, e) => _connection.RaiseError(e.Message);
            _socket.OnClose += (_, e) =>
            {
                _connection.SetState(ConnectionState.Closed);
                _connection.RaiseClose(e.Code, e.Reason);
            };
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
            _connection.SetState(ConnectionState.Closing);
            _socket.Close();
            _socket = null;
        }
    }
}
