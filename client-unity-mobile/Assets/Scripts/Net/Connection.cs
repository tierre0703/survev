// Port of shared/net/connection.ts — the WebSocket connection state machine.
//
// The web client's Connection owns the socket, queues incoming frames and
// decodes them on the game tick (never in the socket callback). We reproduce
// that split: WebSocketTransport receives bytes on a background thread, pushes
// them into a queue, and Game calls Pump() from Update().
//
// See Assets/Scripts/Net/WebSocketTransport.cs for the mobile socket adapter and
// Assets/Scripts/Net/Messages.cs for the message classes.
using System;
using System.Collections.Generic;

namespace Survev.Net
{
    public enum ConnectionState
    {
        Disconnected,
        Connecting,
        Connected,
        Closing,
        Closed,
    }

    /// <summary>Writer/reader callback type matching the TS message contract.</summary>
    public interface IMessage
    {
        void Serialize(Core.BitBuffer buffer);
        void Deserialize(Core.BitBuffer buffer);
    }

    /// <summary>Port placeholder for <c>shared/net/connection.ts</c>.</summary>
    public sealed class Connection
    {
        private readonly Queue<byte[]> _incoming = new Queue<byte[]>();

        public ConnectionState State { get; private set; } = ConnectionState.Disconnected;

        public event Action OnOpen;
        public event Action<int, string> OnClose;
        public event Action<byte[]> OnMessage;
        public event Action<string> OnError;

        /// <summary>Transport abstraction so tests can inject an in-memory socket.</summary>
        public interface ITransport
        {
            void Connect(string url);
            void Send(byte[] data);
            void Close();
        }

        public void Connect(ITransport transport, string url)
        {
            State = ConnectionState.Connecting;
            transport.Connect(url);
        }

        /// <summary>Called by the transport on the socket thread; does not decode.</summary>
        public void PushFrame(byte[] frame)
        {
            lock (_incoming)
            {
                _incoming.Enqueue(frame);
            }
        }

        /// <summary>Drain queued frames on the main thread (per game tick).</summary>
        public void Pump()
        {
            while (true)
            {
                byte[] frame;
                lock (_incoming)
                {
                    if (_incoming.Count == 0)
                    {
                        break;
                    }
                    frame = _incoming.Dequeue();
                }
                OnMessage?.Invoke(frame);
            }
        }

        public void SetState(ConnectionState state) => State = state;

        public void RaiseOpen() => OnOpen?.Invoke();
        public void RaiseClose(int code, string reason) => OnClose?.Invoke(code, reason);
        public void RaiseError(string message) => OnError?.Invoke(message);
    }
}