// Port of the BitStream extensions in shared/net/net.ts (Constants, BitSizes,
// MsgType, and the wire helpers: writeFloat/readFloat, writeVec, writeMapPos,
// writeUnitVec, writeGameType, writeMapType, writeArray, writeCollider, ...).
//
// This is the layer every message is built on, so it must stay bit-exact with
// the TypeScript implementation. The raw bit reader/writer lives in
// Core/BitBuffer.cs; this file adds the protocol-specific helpers.
//
// Coordinate note: the web client's writeFloat is the standard quantised float
// (min..max over `bits`), which is exactly what BitBuffer.WriteFloatScaled does.
using System;
using System.Collections.Generic;
using Survev.Core;

namespace Survev.Net
{
    /// <summary>Port of <c>Constants</c> in shared/net/net.ts.</summary>
    public static class NetConstants
    {
        public const int MaxPosition = 1024;
        public const int MapNameMaxLen = 24;
        public const int PlayerNameMaxLen = 16;
        public const int MouseMaxDist = 64;
        public const int SmokeMaxRad = 10;
        public const float ActionMaxDuration = 8.5f;
        public const int AirstrikeZoneMaxRad = 256;
        public const int AirstrikeZoneMaxDuration = 60;
        public const float PlayerMinScale = 0.75f;
        public const float PlayerMaxScale = 2f;
        public const float MapObjectMinScale = 0.125f;
        public const float MapObjectMaxScale = 2.5f;
        public const int MaxPerks = 8;
        public const int MaxMapIndicators = 16;
    }

    /// <summary>Port of <c>BitSizes</c> (getBits of the config counts).</summary>
    public static class BitSizes
    {
        // getBits(n) = ceil(log2(n))
        public const int Action = 3; // ceil(log2(5))
        public const int Anim = 4;   // ceil(log2(9))
        public const int Haste = 2;  // ceil(log2(4))
        public const int Perks = 3;  // ceil(log2(8))
        public const int MapIndicators = 4; // ceil(log2(16))
    }

    /// <summary>Port of the <c>MsgType</c> enum. Order is protocol-critical.</summary>
    public enum MsgType
    {
        None = 0,
        // JoinMsg must always be 1 (protocol version check with old clients).
        Join = 1,
        _Disconnect = 2,
        Input = 3,
        Edit = 4,
        Joined = 5,
        Update = 6,
        Kill = 7,
        GameOver = 8,
        Pickup = 9,
        Map = 10,
        Spectate = 11,
        DropItem = 12,
        Emote = 13,
        PlayerStats = 14,
        AdStatus = 15,
        Loadout = 16,
        RoleAnnouncement = 17,
        Stats = 18,
        UpdatePass = 19,
        AliveCounts = 20,
        PerkModeRoleSelect = 21,
    }

    /// <summary>Port of <c>PickupMsgType</c>.</summary>
    public enum PickupMsgType
    {
        Full = 0,
        AlreadyOwned = 1,
        AlreadyEquipped = 2,
        BetterItemEquipped = 3,
        Success = 4,
        GunCannotFire = 5,
        MaxPerks = 6,
    }

    /// <summary>Port of the <c>BitStream</c> subclass in shared/net/net.ts.</summary>
    public static class BitStreamExtensions
    {
        // getBits(n) = ceil(log2(n)), used by writeArray/readArray assertions.
        private static int GetBits(int n) => (int)Math.Ceiling(Math.Log(n, 2));

        public static void WriteFloat(this BitBuffer s, float f, float min, float max, int bits)
        {
            int range = (1 << bits) - 1;
            float x = MathUtil.Clamp(f, min, max);
            float t = (x - min) / (max - min);
            uint v = (uint)(t * range + 0.5f);
            s.WriteBits((int)v, bits);
        }

        public static float ReadFloat(this BitBuffer s, float min, float max, int bits)
        {
            int range = (1 << bits) - 1;
            int x = s.ReadBits(bits);
            float t = (float)x / range;
            return min + t * (max - min);
        }

        public static void WriteVec(this BitBuffer s, Vec2 vec, float minX, float minY, float maxX, float maxY, int bitCount)
        {
            s.WriteFloat(vec.X, minX, maxX, bitCount);
            s.WriteFloat(vec.Y, minY, maxY, bitCount);
        }

        public static Vec2 ReadVec(this BitBuffer s, float minX, float minY, float maxX, float maxY, int bitCount)
        {
            return new Vec2(
                s.ReadFloat(minX, maxX, bitCount),
                s.ReadFloat(minY, maxY, bitCount));
        }

        public static void WriteMapPos(this BitBuffer s, Vec2 vec, int bitCount = 16)
        {
            s.WriteVec(vec, 0, 0, NetConstants.MaxPosition, NetConstants.MaxPosition, bitCount);
        }

        public static Vec2 ReadMapPos(this BitBuffer s, int bitCount = 16)
        {
            return s.ReadVec(0, 0, NetConstants.MaxPosition, NetConstants.MaxPosition, bitCount);
        }

        public static void WriteUnitVec(this BitBuffer s, Vec2 vec, int bitCount)
        {
            s.WriteVec(vec, -1.0001f, -1.0001f, 1.0001f, 1.0001f, bitCount);
        }

        public static Vec2 ReadUnitVec(this BitBuffer s, int bitCount)
        {
            return s.ReadVec(-1.0001f, -1.0001f, 1.0001f, 1.0001f, bitCount);
        }

        public static void WriteVec32(this BitBuffer s, Vec2 vec)
        {
            s.WriteFloat32(vec.X);
            s.WriteFloat32(vec.Y);
        }

        public static Vec2 ReadVec32(this BitBuffer s)
        {
            return new Vec2(s.ReadFloat32(), s.ReadFloat32());
        }

        /// <summary>Align the cursor to the next byte boundary (writes zero padding).</summary>
        public static void WriteAlignToNextByte(this BitBuffer s)
        {
            int offset = 8 - (s.BitLength % 8);
            if (offset < 8)
            {
                s.WriteBits(0, offset);
            }
        }

        public static void ReadAlignToNextByte(this BitBuffer s)
        {
            int offset = 8 - (s.BitLength % 8);
            if (offset < 8)
            {
                s.ReadBits(offset);
            }
        }

        public static void WriteGameType(this BitBuffer s, string type)
        {
            s.WriteBits(TypeRegistry.GameTypeToId(type), 10);
        }

        public static string ReadGameType(this BitBuffer s)
        {
            return TypeRegistry.GameIdToType(s.ReadBits(10));
        }

        public static void WriteMapType(this BitBuffer s, string type)
        {
            s.WriteBits(TypeRegistry.MapTypeToId(type), 12);
        }

        public static string ReadMapType(this BitBuffer s)
        {
            return TypeRegistry.MapIdToType(s.ReadBits(12));
        }

        public static void WriteArray<T>(this BitBuffer s, IList<T> array, int bits, Action<T, int> writeFn)
        {
            int length = array.Count;
            int maxSize = (1 << bits) - 1;
            if (length > maxSize)
            {
                length = maxSize;
            }

            s.WriteBits(length, bits);
            for (int i = 0; i < length; i++)
            {
                writeFn(array[i], i);
            }
        }

        public static List<T> ReadArray<T>(this BitBuffer s, int bits, Func<int, T> readFn)
        {
            int length = s.ReadBits(bits);
            var array = new List<T>(length);
            for (int i = 0; i < length; i++)
            {
                array.Add(readFn(i));
            }
            return array;
        }

        public static void WriteCollider(this BitBuffer s, Collider col)
        {
            s.WriteUInt8((byte)col.Type);
            if (col.Type == ColliderType.Circle)
            {
                s.WriteMapPos(col.Pos);
                s.WriteFloat(col.Rad, 0, NetConstants.MaxPosition, 16);
            }
            else
            {
                s.WriteMapPos(col.Min);
                s.WriteMapPos(col.Max);
            }
        }

        public static Collider ReadCollider(this BitBuffer s)
        {
            byte type = s.ReadUInt8();
            if (type == (byte)ColliderType.Circle)
            {
                return Collider.CreateCircle(s.ReadMapPos(), s.ReadFloat(0, NetConstants.MaxPosition, 16));
            }
            return Collider.CreateAabb(s.ReadMapPos(), s.ReadMapPos());
        }
    }
}
