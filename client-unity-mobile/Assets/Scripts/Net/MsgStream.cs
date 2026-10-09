// Port of the <c>MsgStream</c> class in shared/net/net.ts.
//
// MsgStream frames a message: a single byte MsgType, the serialized body, then
// zero-padding to the next byte boundary. It also exposes the raw bytes for the
// WebSocket transport.
using Survev.Core;

namespace Survev.Net
{
    /// <summary>Port of <c>shared/net/net.ts MsgStream</c>.</summary>
    public sealed class MsgStream
    {
        public BitBuffer Stream { get; }

        public MsgStream()
        {
            Stream = new BitBuffer();
        }

        public MsgStream(byte[] bytes)
        {
            Stream = new BitBuffer(bytes.Length + 16);
            for (int i = 0; i < bytes.Length; i++)
            {
                Stream.WriteUInt8(bytes[i]);
            }
            Stream.Rewind();
        }

        public byte[] GetBuffer() => Stream.ToArray();

        public void SerializeMsg(MsgType type, IMessage msg)
        {
            Stream.WriteUInt8((byte)type);
            msg.Serialize(Stream);
            Stream.WriteAlignToNextByte();
        }

        /// <summary>Reads the leading MsgType byte, or None at end of stream.</summary>
        public MsgType DeserializeMsgType()
        {
            if (Stream.BitsRemaining >= 1)
            {
                return (MsgType)Stream.ReadUInt8();
            }
            return MsgType.None;
        }
    }
}
