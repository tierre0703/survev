// Port of shared/lib/bitBuffer.ts — the binary net protocol primitive.
//
// The web client builds every network packet on a bit-level buffer, so this
// class MUST be byte/bit compatible with the TypeScript implementation
// (little-endian bit ordering within a byte, LSB first). Any change here must
// keep the captured-frame tests green (see Assets/Tests/EditMode/BitBufferTests).
//
// The TS side stores data in a Uint8Array; we use the same and treat index i as
// the byte holding bits [i*8, i*8+8). Bits are written LSB-first.
using System;

namespace Survev.Core
{
    /// <summary>
    /// Minimal bit-level writer/reader. Mirrors the subset of
    /// <c>shared/lib/bitBuffer.ts</c> the protocol uses: fixed-width unsigned and
    /// signed integers, booleans, floats and UTF-8 strings.
    /// </summary>
    public sealed class BitBuffer
    {
        private byte[] _bytes;
        private int _bitLength;
        private int _readOffset;

        public BitBuffer(int byteCapacity = 256)
        {
            _bytes = new byte[byteCapacity];
            _bitLength = 0;
            _readOffset = 0;
        }

        public byte[] Bytes => _bytes;
        public int BitLength => _bitLength;
        public int ByteLength => (_bitLength + 7) >> 3;

        /// <summary>Number of bits still writable before growing.</summary>
        public int BitsRemaining => (_bytes.Length << 3) - _bitLength;

        private void EnsureBits(int bits)
        {
            int needed = (_bitLength + bits + 7) >> 3;
            if (needed <= _bytes.Length)
            {
                return;
            }
            int size = _bytes.Length;
            while (size < needed)
            {
                size <<= 1;
            }
            Array.Resize(ref _bytes, size);
        }

        /// <summary>Write <paramref name="count"/> bits of <paramref name="value"/> LSB-first.</summary>
        public void WriteBits(int value, int count)
        {
            EnsureBits(count);
            for (int i = 0; i < count; i++)
            {
                int bit = (value >> i) & 1;
                int idx = _bitLength >> 3;
                int shift = 7 - (_bitLength & 7); // TS bit view writes MSB of byte first
                if (bit != 0)
                {
                    _bytes[idx] |= (byte)(1 << shift);
                }
                else
                {
                    _bytes[idx] &= (byte)~(1 << shift);
                }
                _bitLength++;
            }
        }

        public int ReadBits(int count)
        {
            int value = 0;
            for (int i = 0; i < count; i++)
            {
                int idx = _readOffset >> 3;
                int shift = 7 - (_readOffset & 7);
                int bit = (_bytes[idx] >> shift) & 1;
                value |= bit << i;
                _readOffset++;
            }
            return value;
        }

        public void WriteUInt8(byte value) => WriteBits(value, 8);
        public byte ReadUInt8() => (byte)ReadBits(8);

        public void WriteInt8(sbyte value) => WriteBits((byte)value, 8);
        public sbyte ReadInt8() => (sbyte)ReadBits(8);

        public void WriteUInt16(ushort value) => WriteBits(value, 16);
        public ushort ReadUInt16() => (ushort)ReadBits(16);

        public void WriteInt16(short value) => WriteBits((ushort)value, 16);
        public short ReadInt16() => (short)ReadBits(16);

        public void WriteUInt32(uint value)
        {
            WriteBits((int)(value & 0xFFFF), 16);
            WriteBits((int)(value >> 16), 16);
        }

        public uint ReadUInt32()
        {
            uint lo = (uint)ReadBits(16);
            uint hi = (uint)ReadBits(16);
            return lo | (hi << 16);
        }

        /// <summary>IEEE-754 float32 raw little-endian value (writeFloat32 in net.ts).</summary>
        public void WriteFloat32(float value)
        {
            byte[] raw = BitConverter.GetBytes(value);
            if (!BitConverter.IsLittleEndian)
            {
                Array.Reverse(raw);
            }
            for (int i = 0; i < 4; i++)
            {
                WriteUInt8(raw[i]);
            }
        }

        public float ReadFloat32()
        {
            byte[] raw = new byte[4];
            for (int i = 0; i < 4; i++)
            {
                raw[i] = ReadUInt8();
            }
            if (!BitConverter.IsLittleEndian)
            {
                Array.Reverse(raw);
            }
            return BitConverter.ToSingle(raw, 0);
        }

        public void WriteBool(bool value) => WriteBits(value ? 1 : 0, 1);
        public bool ReadBool() => ReadBits(1) != 0;

        /// <summary>IEEE-754 float32 (writeFloat in shared/net/Net.ts).</summary>
        public void WriteFloat(float value)
        {
            byte[] raw = BitConverter.GetBytes(value);
            if (!BitConverter.IsLittleEndian)
            {
                Array.Reverse(raw);
            }
            for (int i = 0; i < 4; i++)
            {
                WriteUInt8(raw[i]);
            }
        }

        public float ReadFloat()
        {
            byte[] raw = new byte[4];
            for (int i = 0; i < 4; i++)
            {
                raw[i] = ReadUInt8();
            }
            if (!BitConverter.IsLittleEndian)
            {
                Array.Reverse(raw);
            }
            return BitConverter.ToSingle(raw, 0);
        }

        /// <summary>
        /// ASCII string, NULL-terminated. With an explicit length it writes
        /// exactly `length` bytes; otherwise `value.Length + 1`.
        /// Port of <c>writeASCIIString</c>/<c>readASCIIString</c> in
        /// shared/lib/bitBuffer.ts (used by net.ts writeString/readString).
        /// </summary>
        public void WriteString(string value, int length = 0)
        {
            value ??= string.Empty;
            int len = length > 0 ? length : value.Length + 1;
            for (int i = 0; i < len; i++)
            {
                WriteUInt8((byte)(i < value.Length ? value[i] : 0x00));
            }
        }

        public string ReadString(int length = 0)
        {
            var chars = new System.Collections.Generic.List<char>();
            bool append = true;
            bool fixedLength = length > 0;
            int bytes = fixedLength ? length : (BitsRemaining / 8);

            for (int i = 0; i < bytes; i++)
            {
                byte c = ReadUInt8();
                if (c == 0x00)
                {
                    append = false;
                    if (!fixedLength)
                    {
                        break;
                    }
                }
                if (append)
                {
                    chars.Add((char)c);
                }
            }
            return new string(chars.ToArray());
        }

        /// <summary>Alias of <see cref="WriteString"/> with an explicit byte length (net.ts writeString).</summary>
        public void WriteStringFixed(string value, int length) => WriteString(value, length);

        /// <summary>Alias of <see cref="ReadString"/> with an explicit byte length (net.ts readString).</summary>
        public string ReadStringFixed(int length) => ReadString(length);

        /// <summary>UTF-8 string, NULL-terminated (writeUTF8String in bitBuffer.ts).</summary>
        public void WriteUtf8String(string value, int length = 0)
        {
            byte[] utf8 = System.Text.Encoding.UTF8.GetBytes(value ?? string.Empty);
            int len = length > 0 ? length : utf8.Length + 1;
            for (int i = 0; i < len; i++)
            {
                WriteUInt8(i < utf8.Length ? utf8[i] : (byte)0x00);
            }
        }

        public string ReadUtf8String(int length = 0)
        {
            var bytes = new System.Collections.Generic.List<byte>();
            bool append = true;
            bool fixedLength = length > 0;
            int total = fixedLength ? length : (BitsRemaining / 8);

            for (int i = 0; i < total; i++)
            {
                byte c = ReadUInt8();
                if (c == 0x00)
                {
                    append = false;
                    if (!fixedLength)
                    {
                        break;
                    }
                }
                if (append)
                {
                    bytes.Add(c);
                }
            }
            return System.Text.Encoding.UTF8.GetString(bytes.ToArray());
        }

        /// <summary>Reset the write cursor (reader reuse: create a fresh reader instead).</summary>
        public void Reset()
        {
            Array.Clear(_bytes, 0, _bytes.Length);
            _bitLength = 0;
            _readOffset = 0;
        }

        /// <summary>Rewind the read cursor to the start (used by captured-frame tests).</summary>
        public void Rewind()
        {
            _readOffset = 0;
        }

        /// <summary>
        /// Overwrite 16 bits at a byte offset (used by UpdateMsg to back-patch its
        /// flags word after the body is written).
        /// </summary>
        public void PatchUInt16(int byteIndex, ushort value)
        {
            int offset = byteIndex * 8;
            for (int i = 0; i < 16; i++)
            {
                int idx = (offset + i) >> 3;
                int shift = 7 - ((offset + i) & 7);
                int bit = (value >> i) & 1;
                if (bit != 0)
                {
                    _bytes[idx] |= (byte)(1 << shift);
                }
                else
                {
                    _bytes[idx] &= (byte)~(1 << shift);
                }
            }
        }

        /// <summary>Trim the backing array to the bytes actually written.</summary>
        public byte[] ToArray()
        {
            byte[] result = new byte[ByteLength];
            Array.Copy(_bytes, result, ByteLength);
            return result;
        }
    }
}