// Port of shared/net/updateMsg.ts — the per-tick object/state delta.
//
// This is the largest message and the one the whole client sim depends on, so
// the bit layout must match the TypeScript exactly. The object serializers live
// in ObjectSerializeFns.cs; this file owns the flags word, the active-player
// block, gas, player/group status, bullets, explosions, emotes, planes,
// airstrikes, map indicators and the kill leader.
using System.Collections.Generic;
using Survev.Core;

namespace Survev.Net
{
    /// <summary>Port of <c>UpdateExtFlags</c>.</summary>
    public static class UpdateExtFlags
    {
        public const int DeletedObjects = 1 << 0;
        public const int FullObjects = 1 << 1;
        public const int ActivePlayerId = 1 << 2;
        public const int Gas = 1 << 3;
        public const int GasCircle = 1 << 4;
        public const int PlayerInfos = 1 << 5;
        public const int DeletePlayerIds = 1 << 6;
        public const int PlayerStatus = 1 << 7;
        public const int GroupStatus = 1 << 8;
        public const int Bullets = 1 << 9;
        public const int Explosions = 1 << 10;
        public const int Emotes = 1 << 11;
        public const int Planes = 1 << 12;
        public const int AirstrikeZones = 1 << 13;
        public const int MapIndicators = 1 << 14;
        public const int KillLeader = 1 << 15;
    }

    public struct PlayerInfo
    {
        public ushort PlayerId;
        public byte TeamId;
        public byte GroupId;
        public string Name;
        public string LoadoutHeal;
        public string LoadoutBoost;
    }

    public struct GasData
    {
        public byte Mode;
        public float Duration;
        public Vec2 PosOld;
        public Vec2 PosNew;
        public float RadOld;
        public float RadNew;
    }

    public struct PlayerStatus
    {
        public bool HasData;
        public Vec2 Pos;
        public bool Visible;
        public bool Dead;
        public bool Downed;
        public string Role;
    }

    public struct GroupStatus
    {
        public float Health;
        public bool Disconnected;
    }

    public struct Bullet
    {
        public ushort PlayerId;
        public Vec2 StartPos;
        public Vec2 Pos;
        public Vec2 Dir;
        public string BulletType;
        public int Layer;
        public float VarianceT;
        public int DistAdjIdx;
        public bool ClipDistance;
        public float Distance;
        public bool ShotFx;
        public string ShotSourceType;
        public bool ShotOffhand;
        public bool LastShot;
        public int ReflectCount;
        public ushort ReflectObjId;
        public bool HasModifier;
        public float SpeedMult;
        public float DistanceMult;
        public bool HasSpecialFx;
        public bool ShotAlt;
        public bool Splinter;
        public bool TrailSaturated;
        public bool ApRounds;
        public bool HighVelocity;
        public bool CombatStims;
        public bool TrailSmall;
        public bool TrailThick;
    }

    public struct Explosion
    {
        public Vec2 Pos;
        public string Type;
        public int Layer;
    }

    public struct Emote
    {
        public ushort PlayerId;
        public string Type;
        public string ItemType;
        public bool IsPing;
        public Vec2 Pos;
    }

    public struct Airstrike
    {
        public Vec2 Pos;
        public float Duration;
        public float Rad;
    }

    public struct Plane
    {
        public Vec2 PlaneDir;
        public Vec2 Pos;
        public bool ActionComplete;
        public int Action;
        public byte Id;
    }

    public struct MapIndicator
    {
        public int Id;
        public bool Dead;
        public bool Equipped;
        public string Type;
        public Vec2 Pos;
    }

    /// <summary>Active-player dirty state (LocalDataWithDirty in updateMsg.ts).</summary>
    public sealed class LocalDataWithDirty
    {
        public float Health;
        public float Boost;
        public byte Zoom;
        public float ActionTime;
        public float ActionDuration;
        public ushort ActionTargetId;
        public string Scope = string.Empty;
        public Dictionary<string, int> Inventory = new Dictionary<string, int>();
        public int CurWeapIdx;
        public WeaponData[] Weapons = new WeaponData[GameConfig.WeaponSlotCount];
        public byte SpectatorCount;

        public bool HealthDirty;
        public bool BoostDirty;
        public bool ZoomDirty;
        public bool ActionDirty;
        public bool InventoryDirty;
        public bool WeapsDirty;
        public bool SpectatorCountDirty;

        public struct WeaponData
        {
            public string Type;
            public byte Ammo;
        }
    }

    public sealed class UpdateMsg : AbstractMsg
    {
        // GameConfig.bagSizes key order — protocol-critical.
        public static readonly string[] BagSizes = GameConfig.BagSizes;

        public List<ushort> DelObjIds = new List<ushort>();
        public List<ObjectData> FullObjects = new List<ObjectData>();
        public List<ObjectData> PartObjects = new List<ObjectData>();

        public ushort ActivePlayerId;
        public bool ActivePlayerIdDirty;
        public LocalDataWithDirty ActivePlayerData = new LocalDataWithDirty();

        public GasData GasData;
        public bool GasDirty;
        public float GasT;
        public bool GasTDirty;

        public List<PlayerInfo> PlayerInfos = new List<PlayerInfo>();
        public List<ushort> DeletedPlayerIds = new List<ushort>();

        public List<PlayerStatus> PlayerStatusList = new List<PlayerStatus>();
        public bool PlayerStatusDirty;

        public List<GroupStatus> GroupStatusList = new List<GroupStatus>();
        public bool GroupStatusDirty;

        public List<Bullet> Bullets = new List<Bullet>();
        public List<Explosion> Explosions = new List<Explosion>();
        public List<Emote> Emotes = new List<Emote>();
        public List<Plane> Planes = new List<Plane>();
        public List<Airstrike> AirstrikeZones = new List<Airstrike>();
        public List<MapIndicator> MapIndicators = new List<MapIndicator>();

        public ushort KillLeaderId;
        public byte KillLeaderKills;
        public bool KillLeaderDirty;
        public byte Ack;

        public override void Serialize(BitBuffer s)
        {
            int flags = 0;
            int flagsByteIndex = s.ByteLength; // byte offset of the flags word
            s.WriteUInt16(0);

            if (DelObjIds.Count > 0)
            {
                s.WriteArray(DelObjIds, 16, (id, _) => s.WriteUInt16(id));
                flags |= UpdateExtFlags.DeletedObjects;
            }

            if (FullObjects.Count > 0)
            {
                s.WriteArray(FullObjects, 16, (obj, _) =>
                {
                    ObjectSerializeFns.For(obj.Type).SerializePart(s, obj);
                    s.WriteAlignToNextByte();
                    ObjectSerializeFns.For(obj.Type).SerializeFullFn(s, obj);
                    s.WriteAlignToNextByte();
                });
                flags |= UpdateExtFlags.FullObjects;
            }

            s.WriteArray(PartObjects, 16, (obj, _) =>
            {
                ObjectSerializeFns.For(obj.Type).SerializePart(s, obj);
                s.WriteAlignToNextByte();
            });

            if (ActivePlayerIdDirty)
            {
                s.WriteUInt16(ActivePlayerId);
                flags |= UpdateExtFlags.ActivePlayerId;
            }

            SerializeActivePlayer(s, ActivePlayerData);

            if (GasDirty)
            {
                SerializeGasData(s, GasData);
                flags |= UpdateExtFlags.Gas;
            }

            if (GasTDirty)
            {
                s.WriteFloat(GasT, 0, 1, 16);
                flags |= UpdateExtFlags.GasCircle;
            }

            if (PlayerInfos.Count > 0)
            {
                s.WriteArray(PlayerInfos, 8, (info, _) => SerializePlayerInfo(s, info));
                flags |= UpdateExtFlags.PlayerInfos;
            }

            if (DeletedPlayerIds.Count > 0)
            {
                s.WriteArray(DeletedPlayerIds, 8, (id, _) => s.WriteUInt16(id));
                flags |= UpdateExtFlags.DeletePlayerIds;
            }

            if (PlayerStatusDirty)
            {
                SerializePlayerStatus(s, PlayerStatusList);
                flags |= UpdateExtFlags.PlayerStatus;
            }

            if (GroupStatusDirty)
            {
                SerializeGroupStatus(s, GroupStatusList);
                flags |= UpdateExtFlags.GroupStatus;
            }

            if (Bullets.Count > 0)
            {
                s.WriteArray(Bullets, 8, (bullet, _) =>
                {
                    s.WriteUInt16(bullet.PlayerId);
                    s.WriteMapPos(bullet.StartPos);
                    s.WriteUnitVec(bullet.Dir, 8);
                    s.WriteGameType(bullet.BulletType);
                    s.WriteBits(bullet.Layer, 2);
                    s.WriteFloat(bullet.VarianceT, 0, 1, 4);
                    s.WriteBits(bullet.DistAdjIdx, 4);
                    s.WriteBool(bullet.ClipDistance);
                    if (bullet.ClipDistance)
                    {
                        s.WriteFloat(bullet.Distance, 0, NetConstants.MaxPosition, 16);
                    }
                    s.WriteBool(bullet.ShotFx);
                    if (bullet.ShotFx)
                    {
                        s.WriteGameType(bullet.ShotSourceType);
                        s.WriteBool(bullet.ShotOffhand);
                        s.WriteBool(bullet.LastShot);
                    }
                    s.WriteBool(bullet.ReflectCount > 0);
                    if (bullet.ReflectCount > 0)
                    {
                        s.WriteBits(bullet.ReflectCount, 2);
                        s.WriteUInt16(bullet.ReflectObjId);
                    }
                    s.WriteBool(bullet.HasModifier);
                    if (bullet.HasModifier)
                    {
                        s.WriteFloat(bullet.SpeedMult, 0.5f, 2, 8);
                        s.WriteFloat(bullet.DistanceMult, 0.5f, 2, 8);
                    }
                    s.WriteBool(bullet.HasSpecialFx);
                    if (bullet.HasSpecialFx)
                    {
                        s.WriteBool(bullet.ShotAlt);
                        s.WriteBool(bullet.Splinter);
                        s.WriteBool(bullet.TrailSaturated);
                        s.WriteBool(bullet.ApRounds);
                        s.WriteBool(bullet.HighVelocity);
                        s.WriteBool(bullet.CombatStims);
                        s.WriteBool(bullet.TrailSmall);
                        s.WriteBool(bullet.TrailThick);
                    }
                });

                s.WriteAlignToNextByte();
                flags |= UpdateExtFlags.Bullets;
            }

            if (Explosions.Count > 0)
            {
                s.WriteArray(Explosions, 8, (explosion, _) =>
                {
                    s.WriteMapPos(explosion.Pos);
                    s.WriteGameType(explosion.Type);
                    s.WriteBits(explosion.Layer, 2);
                    s.WriteAlignToNextByte();
                });
                flags |= UpdateExtFlags.Explosions;
            }

            if (Emotes.Count > 0)
            {
                s.WriteArray(Emotes, 8, (emote, _) =>
                {
                    s.WriteUInt16(emote.PlayerId);
                    s.WriteGameType(emote.Type);
                    s.WriteGameType(emote.ItemType);
                    s.WriteBool(emote.IsPing);
                    if (emote.IsPing)
                    {
                        s.WriteMapPos(emote.Pos);
                    }
                    s.WriteAlignToNextByte();
                });
                flags |= UpdateExtFlags.Emotes;
            }

            if (Planes.Count > 0)
            {
                s.WriteArray(Planes, 8, (plane, _) =>
                {
                    s.WriteUInt8(plane.Id);
                    s.WriteVec(plane.Pos, -256, -256,
                        NetConstants.MaxPosition + 256, NetConstants.MaxPosition + 256, 10);
                    s.WriteUnitVec(plane.PlaneDir, 8);
                    s.WriteBool(plane.ActionComplete);
                    s.WriteBits(plane.Action, 3);
                });
                flags |= UpdateExtFlags.Planes;
            }

            if (AirstrikeZones.Count > 0)
            {
                s.WriteArray(AirstrikeZones, 8, (zone, _) =>
                {
                    s.WriteMapPos(zone.Pos, 12);
                    s.WriteFloat(zone.Rad, 0, NetConstants.AirstrikeZoneMaxRad, 8);
                    s.WriteFloat(zone.Duration, 0, NetConstants.AirstrikeZoneMaxDuration, 8);
                });
                s.WriteAlignToNextByte();
                flags |= UpdateExtFlags.AirstrikeZones;
            }

            if (MapIndicators.Count > 0)
            {
                s.WriteArray(MapIndicators, BitSizes.MapIndicators, (indicator, _) =>
                {
                    s.WriteBits(indicator.Id, BitSizes.MapIndicators);
                    s.WriteBool(indicator.Dead);
                    s.WriteBool(indicator.Equipped);
                    s.WriteGameType(indicator.Type);
                    s.WriteMapPos(indicator.Pos);
                });
                s.WriteAlignToNextByte();
                flags |= UpdateExtFlags.MapIndicators;
            }

            if (KillLeaderDirty)
            {
                s.WriteUInt16(KillLeaderId);
                s.WriteUInt8(KillLeaderKills);
                flags |= UpdateExtFlags.KillLeader;
            }

            s.WriteUInt8(Ack);

            // Back-patch the flags word now that every branch has been taken.
            s.PatchUInt16(flagsByteIndex, (ushort)flags);
        }

        public override void Deserialize(BitBuffer s)
        {
            int flags = s.ReadUInt16();

            if ((flags & UpdateExtFlags.DeletedObjects) != 0)
            {
                DelObjIds = s.ReadArray(16, _ => s.ReadUInt16());
            }

            if ((flags & UpdateExtFlags.FullObjects) != 0)
            {
                FullObjects = s.ReadArray(16, _ =>
                {
                    var data = new ObjectData { Type = (ObjectType)s.ReadUInt8(), Id = s.ReadUInt16() };
                    ObjectSerializeFns.For(data.Type).DeserializePart(s, data);
                    s.ReadAlignToNextByte();
                    ObjectSerializeFns.For(data.Type).DeserializeFullFn(s, data);
                    s.ReadAlignToNextByte();
                    return data;
                });
            }

            PartObjects = s.ReadArray(16, _ =>
            {
                var data = new ObjectData { Type = (ObjectType)s.ReadUInt8(), Id = s.ReadUInt16() };
                ObjectSerializeFns.For(data.Type).DeserializePart(s, data);
                s.ReadAlignToNextByte();
                return data;
            });

            if ((flags & UpdateExtFlags.ActivePlayerId) != 0)
            {
                ActivePlayerId = s.ReadUInt16();
                ActivePlayerIdDirty = true;
            }

            ActivePlayerData = new LocalDataWithDirty();
            DeserializeActivePlayer(s, ActivePlayerData);

            if ((flags & UpdateExtFlags.Gas) != 0)
            {
                GasData = DeserializeGasData(s);
                GasDirty = true;
            }

            if ((flags & UpdateExtFlags.GasCircle) != 0)
            {
                GasT = s.ReadFloat(0, 1, 16);
                GasTDirty = true;
            }

            if ((flags & UpdateExtFlags.PlayerInfos) != 0)
            {
                PlayerInfos = s.ReadArray(8, _ => DeserializePlayerInfo(s));
            }

            if ((flags & UpdateExtFlags.DeletePlayerIds) != 0)
            {
                DeletedPlayerIds = s.ReadArray(8, _ => s.ReadUInt16());
            }

            if ((flags & UpdateExtFlags.PlayerStatus) != 0)
            {
                PlayerStatusList = DeserializePlayerStatus(s);
                PlayerStatusDirty = true;
            }

            if ((flags & UpdateExtFlags.GroupStatus) != 0)
            {
                GroupStatusList = DeserializeGroupStatus(s);
                GroupStatusDirty = true;
            }

            if ((flags & UpdateExtFlags.Bullets) != 0)
            {
                Bullets = s.ReadArray(8, _ => DeserializeBullet(s));
                s.ReadAlignToNextByte();
            }

            if ((flags & UpdateExtFlags.Explosions) != 0)
            {
                Explosions = s.ReadArray(8, _ =>
                {
                    var explosion = new Explosion
                    {
                        Pos = s.ReadMapPos(),
                        Type = s.ReadGameType(),
                        Layer = s.ReadBits(2),
                    };
                    s.ReadAlignToNextByte();
                    return explosion;
                });
            }

            if ((flags & UpdateExtFlags.Emotes) != 0)
            {
                Emotes = s.ReadArray(8, _ =>
                {
                    var emote = new Emote
                    {
                        PlayerId = s.ReadUInt16(),
                        Type = s.ReadGameType(),
                        ItemType = s.ReadGameType(),
                        IsPing = s.ReadBool(),
                    };
                    if (emote.IsPing)
                    {
                        emote.Pos = s.ReadMapPos();
                    }
                    s.ReadAlignToNextByte();
                    return emote;
                });
            }

            if ((flags & UpdateExtFlags.Planes) != 0)
            {
                Planes = s.ReadArray(8, _ => new Plane
                {
                    Id = s.ReadUInt8(),
                    Pos = s.ReadVec(-256, -256,
                        NetConstants.MaxPosition + 256, NetConstants.MaxPosition + 256, 10),
                    PlaneDir = s.ReadUnitVec(8),
                    ActionComplete = s.ReadBool(),
                    Action = s.ReadBits(3),
                });
            }

            if ((flags & UpdateExtFlags.AirstrikeZones) != 0)
            {
                AirstrikeZones = s.ReadArray(8, _ => new Airstrike
                {
                    Pos = s.ReadMapPos(12),
                    Rad = s.ReadFloat(0, NetConstants.AirstrikeZoneMaxRad, 8),
                    Duration = s.ReadFloat(0, NetConstants.AirstrikeZoneMaxDuration, 8),
                });
                s.ReadAlignToNextByte();
            }

            if ((flags & UpdateExtFlags.MapIndicators) != 0)
            {
                MapIndicators = s.ReadArray(BitSizes.MapIndicators, _ => new MapIndicator
                {
                    Id = s.ReadBits(BitSizes.MapIndicators),
                    Dead = s.ReadBool(),
                    Equipped = s.ReadBool(),
                    Type = s.ReadGameType(),
                    Pos = s.ReadMapPos(),
                });
                s.ReadAlignToNextByte();
            }

            if ((flags & UpdateExtFlags.KillLeader) != 0)
            {
                KillLeaderId = s.ReadUInt16();
                KillLeaderKills = s.ReadUInt8();
                KillLeaderDirty = true;
            }

            Ack = s.ReadUInt8();
        }

        private static void SerializeActivePlayer(BitBuffer s, LocalDataWithDirty data)
        {
            s.WriteBool(data.HealthDirty);
            if (data.HealthDirty)
            {
                s.WriteFloat(data.Health, 0, 100, 8);
            }
            s.WriteBool(data.BoostDirty);
            if (data.BoostDirty)
            {
                s.WriteFloat(data.Boost, 0, 100, 8);
            }
            s.WriteBool(data.ZoomDirty);
            if (data.ZoomDirty)
            {
                s.WriteUInt8(data.Zoom);
            }
            s.WriteBool(data.ActionDirty);
            if (data.ActionDirty)
            {
                s.WriteFloat(data.ActionTime, 0, NetConstants.ActionMaxDuration, 8);
                s.WriteFloat(data.ActionDuration, 0, NetConstants.ActionMaxDuration, 8);
                s.WriteUInt16(data.ActionTargetId);
            }
            s.WriteBool(data.InventoryDirty);
            if (data.InventoryDirty)
            {
                s.WriteGameType(data.Scope);
                foreach (string key in BagSizes)
                {
                    bool hasItem = data.Inventory.TryGetValue(key, out int count) && count > 0;
                    s.WriteBool(hasItem);
                    if (hasItem)
                    {
                        s.WriteBits(count, 9);
                    }
                }
            }
            s.WriteBool(data.WeapsDirty);
            if (data.WeapsDirty)
            {
                s.WriteBits(data.CurWeapIdx, 2);
                for (int i = 0; i < GameConfig.WeaponSlotCount; i++)
                {
                    s.WriteGameType(data.Weapons[i].Type);
                    s.WriteUInt8(data.Weapons[i].Ammo);
                }
            }
            s.WriteBool(data.SpectatorCountDirty);
            if (data.SpectatorCountDirty)
            {
                s.WriteUInt8(data.SpectatorCount);
            }
            s.WriteAlignToNextByte();
        }

        private static void DeserializeActivePlayer(BitBuffer s, LocalDataWithDirty data)
        {
            data.HealthDirty = s.ReadBool();
            if (data.HealthDirty)
            {
                data.Health = s.ReadFloat(0, 100, 8);
            }
            data.BoostDirty = s.ReadBool();
            if (data.BoostDirty)
            {
                data.Boost = s.ReadFloat(0, 100, 8);
            }
            data.ZoomDirty = s.ReadBool();
            if (data.ZoomDirty)
            {
                data.Zoom = s.ReadUInt8();
            }
            data.ActionDirty = s.ReadBool();
            if (data.ActionDirty)
            {
                data.ActionTime = s.ReadFloat(0, NetConstants.ActionMaxDuration, 8);
                data.ActionDuration = s.ReadFloat(0, NetConstants.ActionMaxDuration, 8);
                data.ActionTargetId = s.ReadUInt16();
            }
            data.InventoryDirty = s.ReadBool();
            if (data.InventoryDirty)
            {
                data.Scope = s.ReadGameType();
                data.Inventory = new Dictionary<string, int>();
                foreach (string key in BagSizes)
                {
                    int count = 0;
                    if (s.ReadBool())
                    {
                        count = s.ReadBits(9);
                    }
                    data.Inventory[key] = count;
                }
            }
            data.WeapsDirty = s.ReadBool();
            if (data.WeapsDirty)
            {
                data.CurWeapIdx = s.ReadBits(2);
                data.Weapons = new LocalDataWithDirty.WeaponData[GameConfig.WeaponSlotCount];
                for (int i = 0; i < GameConfig.WeaponSlotCount; i++)
                {
                    data.Weapons[i] = new LocalDataWithDirty.WeaponData
                    {
                        Type = s.ReadGameType(),
                        Ammo = s.ReadUInt8(),
                    };
                }
            }
            data.SpectatorCountDirty = s.ReadBool();
            if (data.SpectatorCountDirty)
            {
                data.SpectatorCount = s.ReadUInt8();
            }
            s.ReadAlignToNextByte();
        }

        private static void SerializePlayerInfo(BitBuffer s, PlayerInfo info)
        {
            s.WriteUInt16(info.PlayerId);
            s.WriteUInt8(info.TeamId);
            s.WriteUInt8(info.GroupId);
            s.WriteString(info.Name);
            s.WriteGameType(info.LoadoutHeal);
            s.WriteGameType(info.LoadoutBoost);
            s.WriteAlignToNextByte();
        }

        private static PlayerInfo DeserializePlayerInfo(BitBuffer s)
        {
            var info = new PlayerInfo
            {
                PlayerId = s.ReadUInt16(),
                TeamId = s.ReadUInt8(),
                GroupId = s.ReadUInt8(),
                Name = s.ReadString(),
                LoadoutHeal = s.ReadGameType(),
                LoadoutBoost = s.ReadGameType(),
            };
            s.ReadAlignToNextByte();
            return info;
        }

        private static void SerializeGasData(BitBuffer s, GasData data)
        {
            s.WriteUInt8(data.Mode);
            s.WriteFloat32(data.Duration);
            s.WriteMapPos(data.PosOld);
            s.WriteMapPos(data.PosNew);
            s.WriteFloat(data.RadOld, 0, 2048, 16);
            s.WriteFloat(data.RadNew, 0, 2048, 16);
        }

        private static GasData DeserializeGasData(BitBuffer s)
        {
            return new GasData
            {
                Mode = s.ReadUInt8(),
                Duration = s.ReadFloat32(),
                PosOld = s.ReadMapPos(),
                PosNew = s.ReadMapPos(),
                RadOld = s.ReadFloat(0, 2048, 16),
                RadNew = s.ReadFloat(0, 2048, 16),
            };
        }

        private static void SerializePlayerStatus(BitBuffer s, List<PlayerStatus> players)
        {
            s.WriteArray(players, 8, (info, _) =>
            {
                s.WriteBool(info.HasData);
                if (info.HasData)
                {
                    s.WriteMapPos(info.Pos, 11);
                    s.WriteBool(info.Visible);
                    s.WriteBool(info.Dead);
                    s.WriteBool(info.Downed);
                    s.WriteBool(info.Role != string.Empty);
                    if (info.Role != string.Empty)
                    {
                        s.WriteGameType(info.Role);
                    }
                }
            });
            s.WriteAlignToNextByte();
        }

        private static List<PlayerStatus> DeserializePlayerStatus(BitBuffer s)
        {
            var players = s.ReadArray(8, _ =>
            {
                var p = new PlayerStatus { HasData = s.ReadBool() };
                if (p.HasData)
                {
                    p.Pos = s.ReadMapPos(11);
                    p.Visible = s.ReadBool();
                    p.Dead = s.ReadBool();
                    p.Downed = s.ReadBool();
                    p.Role = string.Empty;
                    if (s.ReadBool())
                    {
                        p.Role = s.ReadGameType();
                    }
                }
                return p;
            });
            s.ReadAlignToNextByte();
            return players;
        }

        private static void SerializeGroupStatus(BitBuffer s, List<GroupStatus> players)
        {
            s.WriteArray(players, 8, (status, _) =>
            {
                s.WriteFloat(status.Health, 0, 100, 7);
                s.WriteBool(status.Disconnected);
            });
        }

        private static List<GroupStatus> DeserializeGroupStatus(BitBuffer s)
        {
            return s.ReadArray(8, _ => new GroupStatus
            {
                Health = s.ReadFloat(0, 100, 7),
                Disconnected = s.ReadBool(),
            });
        }

        private static Bullet DeserializeBullet(BitBuffer s)
        {
            var bullet = new Bullet
            {
                PlayerId = s.ReadUInt16(),
                Pos = s.ReadMapPos(),
                Dir = s.ReadUnitVec(8),
                BulletType = s.ReadGameType(),
                Layer = s.ReadBits(2),
                VarianceT = s.ReadFloat(0, 1, 4),
                DistAdjIdx = s.ReadBits(4),
                ClipDistance = s.ReadBool(),
            };
            if (bullet.ClipDistance)
            {
                bullet.Distance = s.ReadFloat(0, NetConstants.MaxPosition, 16);
            }
            bullet.ShotFx = s.ReadBool();
            if (bullet.ShotFx)
            {
                bullet.ShotSourceType = s.ReadGameType();
                bullet.ShotOffhand = s.ReadBool();
                bullet.LastShot = s.ReadBool();
            }
            bullet.ReflectCount = 0;
            bullet.ReflectObjId = 0;
            if (s.ReadBool())
            {
                bullet.ReflectCount = s.ReadBits(2);
                bullet.ReflectObjId = s.ReadUInt16();
            }
            bullet.SpeedMult = 1;
            bullet.DistanceMult = 1;
            bullet.HasModifier = s.ReadBool();
            if (bullet.HasModifier)
            {
                bullet.SpeedMult = s.ReadFloat(0.5f, 2, 8);
                bullet.DistanceMult = s.ReadFloat(0.5f, 2, 8);
            }
            bullet.HasSpecialFx = s.ReadBool();
            if (bullet.HasSpecialFx)
            {
                bullet.ShotAlt = s.ReadBool();
                bullet.Splinter = s.ReadBool();
                bullet.TrailSaturated = s.ReadBool();
                bullet.ApRounds = s.ReadBool();
                bullet.HighVelocity = s.ReadBool();
                bullet.CombatStims = s.ReadBool();
                bullet.TrailSmall = s.ReadBool();
                bullet.TrailThick = s.ReadBool();
            }
            return bullet;
        }
    }
}
