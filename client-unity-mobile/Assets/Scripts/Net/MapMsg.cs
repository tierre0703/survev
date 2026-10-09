// Port of shared/net/mapMsg.ts — the map generation data sent once on join.
//
// Serialization must match the TypeScript exactly; keep the field order and the
// per-object align-to-byte calls.
using System.Collections.Generic;
using Survev.Core;

namespace Survev.Net
{
    public struct MapRiverData
    {
        public byte Width;
        public bool Looped;
        public List<Vec2> Points;
    }

    public struct MapPlace
    {
        public string Name;
        public Vec2 Pos;
    }

    public struct GroundPatch
    {
        public Collider Bound;
        public uint Color;
        public float Roughness;
        public float OffsetDist;
        public int Order;
        public bool UseAsMapShape;
    }

    public struct MapObj
    {
        public Vec2 Pos;
        public float Scale;
        public string Type;
        public int Ori;
    }

    public sealed class MapMsg : AbstractMsg
    {
        public string MapName = string.Empty;
        public uint Seed;
        public ushort Width;
        public ushort Height;
        public ushort ShoreInset;
        public ushort GrassInset;
        public List<MapRiverData> Rivers = new List<MapRiverData>();
        public List<MapPlace> Places = new List<MapPlace>();
        public List<MapObj> Objects = new List<MapObj>();
        public List<GroundPatch> GroundPatches = new List<GroundPatch>();

        public override void Serialize(BitBuffer s)
        {
            s.WriteStringFixed(MapName, NetConstants.MapNameMaxLen);
            s.WriteUInt32(Seed);
            s.WriteUInt16(Width);
            s.WriteUInt16(Height);
            s.WriteUInt16(ShoreInset);
            s.WriteUInt16(GrassInset);

            s.WriteArray(Rivers, 8, (river, _) =>
            {
                s.WriteUInt8(river.Width);
                s.WriteUInt8((byte)(river.Looped ? 1 : 0));
                s.WriteArray(river.Points, 8, (pos, _) => s.WriteMapPos(pos));
            });

            s.WriteArray(Places, 8, (place, _) =>
            {
                s.WriteString(place.Name);
                s.WriteVec(place.Pos, 0, 0, 1, 1, 16);
            });

            s.WriteArray(Objects, 16, (obj, _) =>
            {
                s.WriteMapPos(obj.Pos);
                s.WriteFloat(obj.Scale, NetConstants.MapObjectMinScale, NetConstants.MapObjectMaxScale, 8);
                s.WriteMapType(obj.Type);
                s.WriteBits(obj.Ori, 2);
                s.WriteAlignToNextByte();
            });

            s.WriteArray(GroundPatches, 8, (patch, _) => SerializeGroundPatch(s, patch));
        }

        public override void Deserialize(BitBuffer s)
        {
            MapName = s.ReadStringFixed(NetConstants.MapNameMaxLen);
            Seed = s.ReadUInt32();
            Width = s.ReadUInt16();
            Height = s.ReadUInt16();
            ShoreInset = s.ReadUInt16();
            GrassInset = s.ReadUInt16();

            Rivers = s.ReadArray(8, _ => new MapRiverData
            {
                Width = s.ReadUInt8(),
                Looped = s.ReadUInt8() != 0,
                Points = s.ReadArray(8, __ => s.ReadMapPos()),
            });

            Places = s.ReadArray(8, _ => new MapPlace
            {
                Name = s.ReadString(),
                Pos = s.ReadVec(0, 0, 1, 1, 16),
            });

            Objects = s.ReadArray(16, _ =>
            {
                var obj = new MapObj
                {
                    Pos = s.ReadMapPos(),
                    Scale = s.ReadFloat(NetConstants.MapObjectMinScale, NetConstants.MapObjectMaxScale, 8),
                    Type = s.ReadMapType(),
                    Ori = s.ReadBits(2),
                };
                s.ReadAlignToNextByte();
                return obj;
            });

            GroundPatches = s.ReadArray(8, _ => DeserializeGroundPatch(s));
        }

        private static void SerializeGroundPatch(BitBuffer s, GroundPatch patch)
        {
            s.WriteCollider(patch.Bound);
            s.WriteUInt32(patch.Color);
            s.WriteFloat32(patch.Roughness);
            s.WriteFloat32(patch.OffsetDist);
            s.WriteBits(patch.Order, 7);
            s.WriteBool(patch.UseAsMapShape);
        }

        private static GroundPatch DeserializeGroundPatch(BitBuffer s)
        {
            return new GroundPatch
            {
                Bound = s.ReadCollider(),
                Color = s.ReadUInt32(),
                Roughness = s.ReadFloat32(),
                OffsetDist = s.ReadFloat32(),
                Order = s.ReadBits(7),
                UseAsMapShape = s.ReadBool(),
            };
        }
    }
}
