// Port of the connection/message-dispatch half of client/src/game.ts.
//
// The web Game owns the connection, sends JoinMsg, and dispatches each MsgType
// to the update loop. This port keeps that split: Game receives frames, decodes
// them on the main thread (Pump), and raises typed events the scene consumes.
//
// The full simulation (object pools, barns, gas, map) lands in later milestones
// (see plan.md M3/M4); this file is the verified networking core.
using System;
using System.Collections.Generic;
using Survev.Core;

namespace Survev.Game
{
    /// <summary>Join identity supplied by the menu (name + loadout).</summary>
    public sealed class JoinInfo
    {
        public string Name = string.Empty;
        public string Outfit = string.Empty;
        public string Melee = string.Empty;
        public string Heal = string.Empty;
        public string Boost = string.Empty;
        public List<string> Emotes = new List<string>();
        public bool IsMobile = true;
        public bool UseTouch = true;
    }

    /// <summary>Networking core: connect, join, dispatch messages.</summary>
    public sealed class Game
    {
        private Net.Connection _connection;
        private byte[] _joinToken;
        private string _joinUrl;
        private System.Action _onConnectFailure;

        public JoinInfo Join = new JoinInfo();
        public ushort ActivePlayerId;
        public bool Started;

        public bool IsConnecting => _connection != null && _connection.State == Net.ConnectionState.Connecting;
        public bool HasJoined { get; private set; }

        public event System.Action OnJoined;
        public event System.Action<Net.UpdateMsg> OnUpdate;
        public event System.Action<Net.MapMsg> OnMap;
        public event System.Action<Net.KillMsg> OnKill;
        public event System.Action<Net.GameOverMsg> OnGameOver;
        public event System.Action<Net.PickupMsg> OnPickup;
        public event System.Action<Net.JoinedMsg> OnJoinedMsg;

        /// <summary>Send sequence, mirroring game.ts seq/seqInFlight.</summary>
        public byte Seq { get; private set; }

        public bool IsConnected => _connection != null;

        /// <summary>Connect to a game server URL with a join token (game.ts tryJoinGame).</summary>
        public void TryJoinGame(string url, string joinToken, System.Action onConnectFailure = null)
        {
            _joinUrl = url;
            _joinToken = System.Text.Encoding.UTF8.GetBytes(joinToken ?? string.Empty);
            _onConnectFailure = onConnectFailure;

            _connection = new Net.Connection(Net.WebSocketTransport.Create(_connection));
            _connection.OnOpen = OnConnectionOpen;
            _connection.OnError = OnConnectionError;
            _connection.OnClose = OnConnectionClose;
            _connection.OnMessage = OnConnectionMessage;
            _connection.Open(url);
        }

        private void OnConnectionOpen()
        {
            SendJoinMessage();
        }

        private void OnConnectionError()
        {
            _onConnectFailure?.Invoke();
        }

        private void OnConnectionClose(int code, string reason)
        {
            if (!HasJoined)
            {
                _onConnectFailure?.Invoke();
            }
        }

        private void SendJoinMessage()
        {
            var msg = new Net.JoinMsg
            {
                Protocol = GameConfig.ProtocolVersion,
                JoinToken = System.Text.Encoding.UTF8.GetString(_joinToken ?? Array.Empty<byte>()),
                Name = Join.Name,
                UseTouch = Join.UseTouch,
                IsMobile = Join.IsMobile,
            };
            msg.Loadout.Outfit = Join.Outfit;
            msg.Loadout.Melee = Join.Melee;
            msg.Loadout.Heal = Join.Heal;
            msg.Loadout.Boost = Join.Boost;
            msg.Loadout.Emotes = new List<string>(Join.Emotes);

            var stream = new Net.MsgStream();
            stream.SerializeMsg(Net.MsgType.Join, msg);
            _connection.Send(stream.GetBuffer());
        }

        /// <summary>Pump queued frames and dispatch them (call from Update).</summary>
        public void Update()
        {
            _connection?.Pump();
        }

        private void OnConnectionMessage(byte[] frame)
        {
            var stream = new Net.MsgStream(frame);
            var type = stream.DeserializeMsgType();

            switch (type)
            {
                case Net.MsgType.Joined:
                {
                    var msg = new Net.JoinedMsg();
                    msg.Deserialize(stream.Stream);
                    ActivePlayerId = msg.PlayerId;
                    Started = msg.Started;
                    HasJoined = true;
                    OnJoinedMsg?.Invoke(msg);
                    OnJoined?.Invoke();
                    break;
                }
                case Net.MsgType.Update:
                {
                    var msg = new Net.UpdateMsg();
                    msg.Deserialize(stream.Stream);
                    if (msg.ActivePlayerIdDirty)
                    {
                        ActivePlayerId = msg.ActivePlayerId;
                    }
                    ActivePlayerData = msg.ActivePlayerData;
                    OnUpdate?.Invoke(msg);
                    break;
                }
                case Net.MsgType.Map:
                {
                    var msg = new Net.MapMsg();
                    msg.Deserialize(stream.Stream);
                    OnMap?.Invoke(msg);
                    break;
                }
                case Net.MsgType.Kill:
                {
                    var msg = new Net.KillMsg();
                    msg.Deserialize(stream.Stream);
                    OnKill?.Invoke(msg);
                    break;
                }
                case Net.MsgType.GameOver:
                {
                    var msg = new Net.GameOverMsg();
                    msg.Deserialize(stream.Stream);
                    OnGameOver?.Invoke(msg);
                    break;
                }
                case Net.MsgType.Pickup:
                {
                    var msg = new Net.PickupMsg();
                    msg.Deserialize(stream.Stream);
                    OnPickup?.Invoke(msg);
                    break;
                }
                default:
                    // Unhandled message types are ignored (as the web client does
                    // for message kinds it doesn't route yet).
                    break;
            }
        }

        public Net.LocalDataWithDirty ActivePlayerData { get; private set; }

        /// <summary>Send an InputMsg (seq assigned here, like game.ts).</summary>
        public void SendInput(Net.InputMsg input)
        {
            if (_connection == null)
            {
                return;
            }
            input.Seq = Seq++;
            var stream = new Net.MsgStream();
            stream.SerializeMsg(Net.MsgType.Input, input);
            _connection.Send(stream.GetBuffer());
        }

        /// <summary>Close the socket gracefully (background/quit).</summary>
        public void Close()
        {
            HasJoined = false;
            _connection?.ResetAndClose();
            _connection = null;
        }
    }
}
