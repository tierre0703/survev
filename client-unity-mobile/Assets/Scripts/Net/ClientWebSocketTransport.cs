// Built-in System.Net.WebSockets transport (fallback).
//
// Same contract as WebSocketSharpTransport: receive on a background task, push
// raw frames into Connection.PushFrame() and decode on the main thread.
//
// NOTE: ClientWebSocket is part of the Unity 2020.3 .NET Standard 2.0 profile.
// If IL2CPP stripping removes it, add an entry to link.xml. Because of profile
// quirks on some Android devices, websocket-sharp is the default transport.
using System;
using System.IO;
using System.Net.WebSockets;
using System.Threading;
using System.Threading.Tasks;

namespace Survev.Net
{
    /// <summary>Connection.ITransport implemented with ClientWebSocket.</summary>
    public sealed class ClientWebSocketTransport : Connection.ITransport
    {
        private readonly ClientWebSocket _socket = new ClientWebSocket();
        private readonly Connection _connection;
        private readonly CancellationTokenSource _cts = new CancellationTokenSource();

        public ClientWebSocketTransport(Connection connection)
        {
            _connection = connection;
        }

        public void Connect(string url)
        {
            _ = ConnectAsync(url);
        }

        private async Task ConnectAsync(string url)
        {
            try
            {
                await _socket.ConnectAsync(new Uri(url), _cts.Token);
                _connection.SetState(ConnectionState.Connected);
                _connection.RaiseOpen();
                await ReceiveLoop();
            }
            catch (Exception ex)
            {
                _connection.RaiseError(ex.Message);
                _connection.RaiseClose(1006, ex.Message);
            }
        }

        private async Task ReceiveLoop()
        {
            byte[] buffer = new byte[64 * 1024];
            var accumulated = new MemoryStream();
            while (_socket.State == WebSocketState.Open)
            {
                accumulated.SetLength(0);
                WebSocketReceiveResult result;
                do
                {
                    result = await _socket.ReceiveAsync(new ArraySegment<byte>(buffer), _cts.Token);
                    if (result.MessageType == WebSocketMessageType.Close)
                    {
                        _connection.SetState(ConnectionState.Closed);
                        _connection.RaiseClose(
                            result.CloseStatus.HasValue ? (int)result.CloseStatus.Value : 1000,
                            result.CloseStatusDescription);
                        return;
                    }
                    accumulated.Write(buffer, 0, result.Count);
                }
                while (!result.EndOfMessage);

                _connection.PushFrame(accumulated.ToArray());
            }
        }

        public void Send(byte[] data)
        {
            if (_socket.State != WebSocketState.Open)
            {
                return;
            }
            _ = _socket.SendAsync(
                new ArraySegment<byte>(data), WebSocketMessageType.Binary, true, _cts.Token);
        }

        public void Close()
        {
            _cts.Cancel();
            if (_socket.State == WebSocketState.Open)
            {
                _ = _socket.CloseAsync(
                    WebSocketCloseStatus.NormalClosure, "", CancellationToken.None);
            }
        }
    }
}
