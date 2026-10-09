// Port of shared/net/connection.ts.
//
// The TS Connection abstracts a WebSocket and exposes onOpen/onMessage/onClose/
// onError. On Unity the socket callbacks fire on a background thread, so the
// transport pushes raw frames into a queue and the game calls Pump() from
// Update() to dispatch them on the main thread (keeps join/pump/dispatch
// deterministic, like the web client's single-threaded event loop).
//
// Transport implementations: WebSocketSharpTransport (default) and
// ClientWebSocketTransport. Use WebSocketTransport.Create(...) to pick.
using System;
using System.Collections.Generic;

namespace Survev.Net
{
    /// <summary>Matches WebSocket.readyState (shared/net/connection.ts).</summary>
    public enum ConnectionState
    {
        Connecting = 0,
        Open = 1,
        Closing = 2,
        Closed = 3,
    }

    /// <summary>A message that can be (de)serialized over the wire.</summary>
    public interface IMessage
    {
        void Serialize(Core.BitBuffer s);
        void Deserialize(Core.BitBuffer s);
    }

    /// <summary>Port of the abstract <c>Connection</c>.</summary>
    public sealed class Connection
    {
        public Action OnOpen = () => { };
        public Action<byte[]> OnMessage = _ => { };
        public Action OnError = () => { };
        public Action<int, string> OnClose = (_, _) => { };

        private readonly Queue<byte[]> _pending = new Queue<byte[]>();
        private readonly ITransport _transport;
        private ConnectionState _state = ConnectionState.Closed;

        /// <summary>Transport abstraction so tests can inject an in-memory socket.</summary>
        public interface ITransport
        {
            void Connect(string url);
            void Send(byte[] data);
            void Close();
        }

        public Connection(ITransport transport)
        {
            _transport = transport;
        }

        /// <summary>Default factory: websocket-sharp transport.</summary>
        public static Connection Create(string url)
        {
            Connection connection = null;
            connection = new Connection(WebSocketTransport.Create(connection));
            connection.Open(url);
            return connection;
        }

        public ConnectionState State => _state;

        public void Open(string url)
        {
            _state = ConnectionState.Connecting;
            _transport.Connect(url);
        }

        /// <summary>Called by the transport on its background thread: queue only.</summary>
        public void Enqueue(byte[] frame)
        {
            lock (_pending)
            {
                _pending.Enqueue(frame);
            }
        }

        /// <summary>Dispatch queued frames on the main thread (once per game tick).</summary>
        public void Pump()
        {
            while (true)
            {
                byte[] frame;
                lock (_pending)
                {
                    if (_pending.Count == 0)
                    {
                        break;
                    }
                    frame = _pending.Dequeue();
                }
                OnMessage(frame);
            }
        }

        // Transport -> connection state notifications (already marshalled by Pump
        // for data; open/close callbacks are safe to apply directly).
        public void NotifyOpen()
        {
            _state = ConnectionState.Open;
            OnOpen();
        }

        public void NotifyClose(int code, string reason)
        {
            _state = ConnectionState.Closed;
            OnClose(code, reason);
        }

        public void NotifyError()
        {
            OnError();
        }

        public void SetState(ConnectionState state) => _state = state;

        public void Send(byte[] data) => _transport.Send(data);

        public void Close()
        {
            _state = ConnectionState.Closing;
            _transport.Close();
        }

        /// <summary>Port of <c>resetAndClose</c>: detach callbacks, then close.</summary>
        public void ResetAndClose()
        {
            OnOpen = () => { };
            OnMessage = _ => { };
            OnError = () => { };
            OnClose = (_, _) => { };
            Close();
        }
    }
}
