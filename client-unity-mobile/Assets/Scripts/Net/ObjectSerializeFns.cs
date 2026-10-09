// Port of shared/net/objectSerializeFns.ts — per-object wire serialization.
//
// `UpdateMsg` embeds a partial stream and (for new objects) a full stream for
// every object. The exact bit layout here must match the TypeScript and the
// server; see the matching functions in shared/net/objectSerializeFns.ts.
//
// The data records are a single `ObjectData` class (union of partial + full
// fields) rather than a generic dictionary, because C# generics over an enum
// index don't map cleanly; behaviour is identical.
using System;
using System.Collections.Generic;
using Survev.Core;
using Survev.Net;

namespace Survev.Net
{
    /// <summary>Port of <c>ObjectType</c> in objectSerializeFns.ts.</summary>
    public enum ObjectType
    {
        Invalid = 0,
        Player = 1,
        Obstacle = 2,
        Loot = 3,
        LootSpawner = 4, // unused
        DeadBody = 5,
        Building = 6,
        Structure = 7,
        Decal = 8,
        Projectile = 9,
        Smoke = 10,
        Airdrop = 11,
    }

    /// <summary>Partial + full fields for one object (mirrors Objects*Data).</summary>
    public sealed class ObjectData
    {
        public ObjectType Type;
        public ushort Id;

        // --- Player ---
        public Vec2 Pos;
        public Vec2 Dir;
        public string Outfit = string.Empty;
        public string Backpack = string.Empty;
        public string Helmet = string.Empty;
        public string Chest = string.Empty;
        public string ActiveWeapon = string.Empty;
        public int Layer;
        public bool Dead;
        public bool Downed;
        public int AnimType;
        public int AnimSeq;
        public int ActionType;
        public int ActionSeq;
        public bool WearingPan;
        public bool HealEffect;
        public bool LastStandEffect;
        public bool Frozen;
        public int FrozenOri;
        public string FrozenType = string.Empty;
        public int HasteType;
        public int HasteSeq = -1;
        public string ActionItem = string.Empty;
        public float Scale = 1f;
        public string Role = string.Empty;
        public List<Perk> Perks = new List<Perk>();

        // --- Obstacle ---
        public float HealthT;
        public string MapType = string.Empty;
        public int Ori;
        public bool IsDoor;
        public bool DoorOpen;
        public bool DoorCanUse;
        public bool DoorLocked;
        public int DoorSeq;
        public bool IsButton;
        public bool ButtonOnOff;
        public bool ButtonCanUse;
        public int ButtonSeq;
        public bool IsPuzzlePiece;
        public ushort ParentBuildingId;
        public bool IsSkin;
        public ushort SkinPlayerId;

        // --- Loot ---
        public int Count;
        public bool IsOld;
        public bool IsPreloadedGun;
        public bool HasOwner;
        public ushort OwnerId;

        // --- DeadBody ---
        public ushort PlayerId;

        // --- Building ---
        public bool CeilingDead;
        public bool Occupied;
        public bool CeilingDamaged;
        public bool HasPuzzle;
        public bool PuzzleSolved;
        public int PuzzleErrSeq;

        // --- Structure ---
        public bool InteriorSoundEnabled;
        public bool InteriorSoundAlt;
        public List<ushort> LayerObjIds = new List<ushort>();

        // --- Decal ---
        public int GoreKills;

        // --- Projectile ---
        public float PosZ;

        // --- Smoke ---
        public float Rad;
        public int Interior;

        // --- Airdrop ---
        public float FallT;
        public bool Landed;
    }

    public struct Perk
    {
        public string Type;
        public bool Droppable;
    }

    /// <summary>Port of <c>ObjectSerializeFns</c>: one serializer per object type.</summary>
    public static class ObjectSerializeFns
    {
        public delegate void SerializePartial(BitBuffer s, ObjectData data);
        public delegate void SerializeFull(BitBuffer s, ObjectData data);
        public delegate void DeserializePartial(BitBuffer s, ObjectData data);
        public delegate void DeserializeFull(BitBuffer s, ObjectData data);

        public sealed class Fns
        {
            public SerializePartial SerializePart;
            public SerializeFull SerializeFullFn;
            public DeserializePartial DeserializePart;
            public DeserializeFull DeserializeFullFn;
        }

        private static readonly Dictionary<ObjectType, Fns> Table = Build();

        public static Fns For(ObjectType type) => Table[type];

        private static Dictionary<ObjectType, Fns> Build()
        {
            return new Dictionary<ObjectType, Fns>
            {
                [ObjectType.Player] = new Fns
                {
                    SerializePart = (s, d) =>
                    {
                        s.WriteMapPos(d.Pos);
                        s.WriteUnitVec(d.Dir, 8);
                    },
                    SerializeFullFn = (s, d) =>
                    {
                        s.WriteGameType(d.Outfit);
                        s.WriteGameType(d.Backpack);
                        s.WriteGameType(d.Helmet);
                        s.WriteGameType(d.Chest);
                        s.WriteGameType(d.ActiveWeapon);

                        s.WriteBits(d.Layer, 2);
                        s.WriteBool(d.Dead);
                        s.WriteBool(d.Downed);

                        s.WriteBits(d.AnimType, BitSizes.Anim);
                        s.WriteBits(d.AnimSeq, 3);

                        s.WriteBits(d.ActionType, BitSizes.Action);
                        s.WriteBits(d.ActionSeq, 3);

                        s.WriteBool(d.WearingPan);
                        s.WriteBool(d.HealEffect);
                        s.WriteBool(d.LastStandEffect);

                        s.WriteBool(d.Frozen);
                        if (d.Frozen)
                        {
                            s.WriteBits(d.FrozenOri, 2);
                            s.WriteGameType(d.FrozenType);
                        }

                        s.WriteBool(d.HasteType != (int)HasteType.None);
                        if (d.HasteType != (int)HasteType.None)
                        {
                            s.WriteBits(d.HasteType, BitSizes.Haste);
                            s.WriteBits(d.HasteSeq, 3);
                        }

                        s.WriteBool(d.ActionItem != string.Empty);
                        if (d.ActionItem != string.Empty)
                        {
                            s.WriteGameType(d.ActionItem);
                        }

                        bool hasScale = d.Scale != 1f;
                        s.WriteBool(hasScale);
                        if (hasScale)
                        {
                            s.WriteFloat(d.Scale, NetConstants.PlayerMinScale, NetConstants.PlayerMaxScale, 8);
                        }

                        bool hasRole = d.Role != string.Empty;
                        s.WriteBool(hasRole);
                        if (hasRole)
                        {
                            s.WriteGameType(d.Role);
                        }

                        bool hasPerks = d.Perks.Count > 0;
                        s.WriteBool(hasPerks);
                        if (hasPerks)
                        {
                            s.WriteArray(d.Perks, BitSizes.Perks, (perk, _) =>
                            {
                                s.WriteGameType(perk.Type);
                                s.WriteBool(perk.Droppable);
                            });
                        }
                    },
                    DeserializePart = (s, d) =>
                    {
                        d.Pos = s.ReadMapPos();
                        d.Dir = s.ReadUnitVec(8);
                    },
                    DeserializeFullFn = (s, d) =>
                    {
                        d.Outfit = s.ReadGameType();
                        d.Backpack = s.ReadGameType();
                        d.Helmet = s.ReadGameType();
                        d.Chest = s.ReadGameType();
                        d.ActiveWeapon = s.ReadGameType();

                        d.Layer = s.ReadBits(2);
                        d.Dead = s.ReadBool();
                        d.Downed = s.ReadBool();

                        d.AnimType = s.ReadBits(BitSizes.Anim);
                        d.AnimSeq = s.ReadBits(3);

                        d.ActionType = s.ReadBits(BitSizes.Action);
                        d.ActionSeq = s.ReadBits(3);

                        d.WearingPan = s.ReadBool();
                        d.HealEffect = s.ReadBool();
                        d.LastStandEffect = s.ReadBool();

                        d.Frozen = s.ReadBool();
                        d.FrozenOri = 0;
                        d.FrozenType = string.Empty;
                        if (d.Frozen)
                        {
                            d.FrozenOri = s.ReadBits(2);
                            d.FrozenType = s.ReadGameType();
                        }

                        d.HasteType = (int)Core.HasteType.None;
                        d.HasteSeq = -1;
                        if (s.ReadBool())
                        {
                            d.HasteType = s.ReadBits(BitSizes.Haste);
                            d.HasteSeq = s.ReadBits(3);
                        }

                        d.ActionItem = s.ReadBool() ? s.ReadGameType() : string.Empty;

                        d.Scale = s.ReadBool()
                            ? s.ReadFloat(NetConstants.PlayerMinScale, NetConstants.PlayerMaxScale, 8)
                            : 1f;

                        d.Role = s.ReadBool() ? s.ReadGameType() : string.Empty;

                        d.Perks = new List<Perk>();
                        if (s.ReadBool())
                        {
                            d.Perks = s.ReadArray(BitSizes.Perks, _ => new Perk
                            {
                                Type = s.ReadGameType(),
                                Droppable = s.ReadBool(),
                            });
                        }
                    },
                },
                [ObjectType.Obstacle] = new Fns
                {
                    SerializePart = (s, d) =>
                    {
                        s.WriteMapPos(d.Pos);
                        s.WriteBits(d.Ori, 2);
                        s.WriteFloat(d.Scale, NetConstants.MapObjectMinScale, NetConstants.MapObjectMaxScale, 8);
                    },
                    SerializeFullFn = (s, d) =>
                    {
                        s.WriteFloat(d.HealthT, 0, 1, 8);
                        s.WriteMapType(d.MapType);
                        s.WriteBits(d.Layer, 2);
                        s.WriteBool(d.Dead);
                        s.WriteBool(d.IsDoor);
                        if (d.IsDoor)
                        {
                            s.WriteBool(d.DoorOpen);
                            s.WriteBool(d.DoorCanUse);
                            s.WriteBool(d.DoorLocked);
                            s.WriteBits(d.DoorSeq, 5);
                        }
                        s.WriteBool(d.IsButton);
                        if (d.IsButton)
                        {
                            s.WriteBool(d.ButtonOnOff);
                            s.WriteBool(d.ButtonCanUse);
                            s.WriteBits(d.ButtonSeq, 6);
                        }
                        s.WriteBool(d.IsPuzzlePiece);
                        if (d.IsPuzzlePiece)
                        {
                            s.WriteUInt16(d.ParentBuildingId);
                        }
                        s.WriteBool(d.IsSkin);
                        if (d.IsSkin)
                        {
                            s.WriteUInt16(d.SkinPlayerId);
                        }
                    },
                    DeserializePart = (s, d) =>
                    {
                        d.Pos = s.ReadMapPos();
                        d.Ori = s.ReadBits(2);
                        d.Scale = s.ReadFloat(NetConstants.MapObjectMinScale, NetConstants.MapObjectMaxScale, 8);
                    },
                    DeserializeFullFn = (s, d) =>
                    {
                        d.HealthT = s.ReadFloat(0, 1, 8);
                        d.MapType = s.ReadMapType();
                        d.Layer = s.ReadBits(2);
                        d.Dead = s.ReadBool();
                        d.IsDoor = s.ReadBool();
                        if (d.IsDoor)
                        {
                            d.DoorOpen = s.ReadBool();
                            d.DoorCanUse = s.ReadBool();
                            d.DoorLocked = s.ReadBool();
                            d.DoorSeq = s.ReadBits(5);
                        }
                        d.IsButton = s.ReadBool();
                        if (d.IsButton)
                        {
                            d.ButtonOnOff = s.ReadBool();
                            d.ButtonCanUse = s.ReadBool();
                            d.ButtonSeq = s.ReadBits(6);
                        }
                        d.IsPuzzlePiece = s.ReadBool();
                        if (d.IsPuzzlePiece)
                        {
                            d.ParentBuildingId = s.ReadUInt16();
                        }
                        d.IsSkin = s.ReadBool();
                        if (d.IsSkin)
                        {
                            d.SkinPlayerId = s.ReadUInt16();
                        }
                    },
                },
                [ObjectType.Building] = new Fns
                {
                    SerializePart = (s, d) =>
                    {
                        s.WriteBool(d.CeilingDead);
                        s.WriteBool(d.Occupied);
                        s.WriteBool(d.CeilingDamaged);
                        s.WriteBool(d.HasPuzzle);
                        if (d.HasPuzzle)
                        {
                            s.WriteBool(d.PuzzleSolved);
                            s.WriteBits(d.PuzzleErrSeq, 7);
                        }
                    },
                    SerializeFullFn = (s, d) =>
                    {
                        s.WriteMapPos(d.Pos);
                        s.WriteMapType(d.MapType);
                        s.WriteBits(d.Ori, 2);
                        s.WriteBits(d.Layer, 2);
                    },
                    DeserializePart = (s, d) =>
                    {
                        d.CeilingDead = s.ReadBool();
                        d.Occupied = s.ReadBool();
                        d.CeilingDamaged = s.ReadBool();
                        d.HasPuzzle = s.ReadBool();
                        if (d.HasPuzzle)
                        {
                            d.PuzzleSolved = s.ReadBool();
                            d.PuzzleErrSeq = s.ReadBits(7);
                        }
                    },
                    DeserializeFullFn = (s, d) =>
                    {
                        d.Pos = s.ReadMapPos();
                        d.MapType = s.ReadMapType();
                        d.Ori = s.ReadBits(2);
                        d.Layer = s.ReadBits(2);
                    },
                },
                [ObjectType.Structure] = new Fns
                {
                    SerializePart = (s, d) => { },
                    SerializeFullFn = (s, d) =>
                    {
                        s.WriteMapPos(d.Pos);
                        s.WriteMapType(d.MapType);
                        s.WriteBits(d.Ori, 2);
                        s.WriteBool(d.InteriorSoundEnabled);
                        s.WriteBool(d.InteriorSoundAlt);
                        for (int r = 0; r < GameConfig.StructureLayerCount; r++)
                        {
                            s.WriteUInt16(d.LayerObjIds[r]);
                        }
                    },
                    DeserializePart = (s, d) => { },
                    DeserializeFullFn = (s, d) =>
                    {
                        d.Pos = s.ReadMapPos();
                        d.MapType = s.ReadMapType();
                        d.Ori = s.ReadBits(2);
                        d.InteriorSoundEnabled = s.ReadBool();
                        d.InteriorSoundAlt = s.ReadBool();
                        d.LayerObjIds = new List<ushort>();
                        for (int r = 0; r < GameConfig.StructureLayerCount; r++)
                        {
                            d.LayerObjIds.Add(s.ReadUInt16());
                        }
                    },
                },
                [ObjectType.LootSpawner] = new Fns
                {
                    SerializePart = (s, d) =>
                    {
                        s.WriteMapPos(d.Pos);
                        s.WriteMapType(d.MapType);
                        s.WriteBits(d.Layer, 2);
                    },
                    SerializeFullFn = (s, d) => { },
                    DeserializePart = (s, d) =>
                    {
                        d.Pos = s.ReadMapPos();
                        d.MapType = s.ReadMapType();
                        d.Layer = s.ReadBits(2);
                    },
                    DeserializeFullFn = (s, d) => { },
                },
                [ObjectType.Loot] = new Fns
                {
                    SerializePart = (s, d) => s.WriteMapPos(d.Pos),
                    SerializeFullFn = (s, d) =>
                    {
                        s.WriteGameType(d.MapType);
                        s.WriteUInt8((byte)d.Count);
                        s.WriteBits(d.Layer, 2);
                        s.WriteBool(d.IsOld);
                        s.WriteBool(d.IsPreloadedGun);
                        s.WriteBool(d.OwnerId != 0);
                        if (d.OwnerId != 0)
                        {
                            s.WriteUInt16(d.OwnerId);
                        }
                    },
                    DeserializePart = (s, d) => d.Pos = s.ReadMapPos(),
                    DeserializeFullFn = (s, d) =>
                    {
                        d.MapType = s.ReadGameType();
                        d.Count = s.ReadUInt8();
                        d.Layer = s.ReadBits(2);
                        d.IsOld = s.ReadBool();
                        d.IsPreloadedGun = s.ReadBool();
                        d.HasOwner = s.ReadBool();
                        if (d.HasOwner)
                        {
                            d.OwnerId = s.ReadUInt16();
                        }
                    },
                },
                [ObjectType.DeadBody] = new Fns
                {
                    SerializePart = (s, d) => s.WriteMapPos(d.Pos),
                    SerializeFullFn = (s, d) =>
                    {
                        s.WriteUInt8((byte)d.Layer);
                        s.WriteUInt16(d.PlayerId);
                    },
                    DeserializePart = (s, d) => d.Pos = s.ReadMapPos(),
                    DeserializeFullFn = (s, d) =>
                    {
                        d.Layer = s.ReadUInt8();
                        d.PlayerId = s.ReadUInt16();
                    },
                },
                [ObjectType.Decal] = new Fns
                {
                    SerializePart = (s, d) => { },
                    SerializeFullFn = (s, d) =>
                    {
                        s.WriteMapPos(d.Pos);
                        s.WriteFloat(d.Scale, NetConstants.MapObjectMinScale, NetConstants.MapObjectMaxScale, 8);
                        s.WriteMapType(d.MapType);
                        s.WriteBits(d.Ori, 2);
                        s.WriteBits(d.Layer, 2);
                        s.WriteUInt8((byte)d.GoreKills);
                    },
                    DeserializePart = (s, d) => { },
                    DeserializeFullFn = (s, d) =>
                    {
                        d.Pos = s.ReadMapPos();
                        d.Scale = s.ReadFloat(NetConstants.MapObjectMinScale, NetConstants.MapObjectMaxScale, 8);
                        d.MapType = s.ReadMapType();
                        d.Ori = s.ReadBits(2);
                        d.Layer = s.ReadBits(2);
                        d.GoreKills = s.ReadUInt8();
                    },
                },
                [ObjectType.Projectile] = new Fns
                {
                    SerializePart = (s, d) =>
                    {
                        s.WriteMapPos(d.Pos);
                        s.WriteFloat(d.PosZ, 0, GameConfig.ProjectileMaxHeight, 10);
                        s.WriteUnitVec(d.Dir, 7);
                    },
                    SerializeFullFn = (s, d) =>
                    {
                        s.WriteGameType(d.MapType);
                        s.WriteBits(d.Layer, 2);
                    },
                    DeserializePart = (s, d) =>
                    {
                        d.Pos = s.ReadMapPos();
                        d.PosZ = s.ReadFloat(0, GameConfig.ProjectileMaxHeight, 10);
                        d.Dir = s.ReadUnitVec(7);
                    },
                    DeserializeFullFn = (s, d) =>
                    {
                        d.MapType = s.ReadGameType();
                        d.Layer = s.ReadBits(2);
                    },
                },
                [ObjectType.Smoke] = new Fns
                {
                    SerializePart = (s, d) =>
                    {
                        s.WriteMapPos(d.Pos);
                        s.WriteFloat(d.Rad, 0, NetConstants.SmokeMaxRad, 8);
                    },
                    SerializeFullFn = (s, d) =>
                    {
                        s.WriteBits(d.Layer, 2);
                        s.WriteBits(d.Interior, 6);
                    },
                    DeserializePart = (s, d) =>
                    {
                        d.Pos = s.ReadMapPos();
                        d.Rad = s.ReadFloat(0, NetConstants.SmokeMaxRad, 8);
                    },
                    DeserializeFullFn = (s, d) =>
                    {
                        d.Layer = s.ReadBits(2);
                        d.Interior = s.ReadBits(6);
                    },
                },
                [ObjectType.Airdrop] = new Fns
                {
                    SerializePart = (s, d) =>
                    {
                        s.WriteFloat(d.FallT, 0, 1, 7);
                        s.WriteBool(d.Landed);
                    },
                    SerializeFullFn = (s, d) => s.WriteMapPos(d.Pos),
                    DeserializePart = (s, d) =>
                    {
                        d.FallT = s.ReadFloat(0, 1, 7);
                        d.Landed = s.ReadBool();
                    },
                    DeserializeFullFn = (s, d) => d.Pos = s.ReadMapPos(),
                },
                [ObjectType.Invalid] = new Fns
                {
                    SerializePart = (s, d) => { },
                    SerializeFullFn = (s, d) => { },
                    DeserializePart = (s, d) => { },
                    DeserializeFullFn = (s, d) => { },
                },
            };
        }
    }
}
