// Port of the shared/net/*.ts message classes. Field order and widths are
// protocol-critical: every Serialize/Deserialize pair must match the TypeScript
// byte layout exactly (see the matching file named in each class comment).
//
// Only messages the client sends/receives are ported here. `UpdateMsg` is the
// large one and lives in UpdateMsg.cs / ObjectSerializeFns.cs.
using System.Collections.Generic;
using Survev.Core;

namespace Survev.Net
{
    /// <summary>Base for every networked message.</summary>
    public abstract class AbstractMsg : IMessage
    {
        public abstract void Serialize(BitBuffer s);
        public abstract void Deserialize(BitBuffer s);
    }

    /// <summary>Port of shared/net/joinMsg.ts.</summary>
    public sealed class JoinMsg : AbstractMsg
    {
        public uint Protocol = GameConfig.ProtocolVersion;
        public string JoinToken = string.Empty;
        public string Name = string.Empty;
        public bool UseTouch;
        public bool IsMobile;
        public bool Bot;
        public LoadoutData Loadout = new LoadoutData();

        public override void Serialize(BitBuffer s)
        {
            // Protocol version is always first with the same size (old clients).
            s.WriteUInt32(Protocol);
            s.WriteString(JoinToken);

            s.WriteStringFixed(Name, NetConstants.PlayerNameMaxLen);
            s.WriteBool(UseTouch);
            s.WriteBool(IsMobile);
            s.WriteBool(Bot);

            s.WriteGameType(Loadout.Outfit);
            s.WriteGameType(Loadout.Melee);
            s.WriteGameType(Loadout.Heal);
            s.WriteGameType(Loadout.Boost);

            s.WriteArray(Loadout.Emotes, 8, (emote, _) => s.WriteGameType(emote));
        }

        public override void Deserialize(BitBuffer s)
        {
            Protocol = s.ReadUInt32();
            JoinToken = s.ReadString();
            Name = s.ReadStringFixed(NetConstants.PlayerNameMaxLen);
            UseTouch = s.ReadBool();
            IsMobile = s.ReadBool();
            Bot = s.ReadBool();

            Loadout.Outfit = s.ReadGameType();
            Loadout.Melee = s.ReadGameType();
            Loadout.Heal = s.ReadGameType();
            Loadout.Boost = s.ReadGameType();

            Loadout.Emotes = s.ReadArray(8, _ => s.ReadGameType());
        }

        public sealed class LoadoutData
        {
            public string Outfit = string.Empty;
            public string Melee = string.Empty;
            public string Heal = string.Empty;
            public string Boost = string.Empty;
            public List<string> Emotes = new List<string>();
        }
    }

    /// <summary>Port of shared/net/inputMsg.ts.</summary>
    public sealed class InputMsg : AbstractMsg
    {
        public byte Seq;
        public bool MoveLeft;
        public bool MoveRight;
        public bool MoveUp;
        public bool MoveDown;
        public bool ShootStart;
        public bool ShootHold;
        public bool Portrait;
        public bool TouchMoveActive;
        public Vec2 TouchMoveDir = new Vec2(1, 0);
        public byte TouchMoveLen = 255;
        public Vec2 ToMouseDir = new Vec2(1, 0);
        public float ToMouseLen;
        public List<int> Inputs = new List<int>();
        public string UseItem = string.Empty;

        public void AddInput(int input)
        {
            if (Inputs.Count < 7 && !Inputs.Contains(input))
            {
                Inputs.Add(input);
            }
        }

        public override void Serialize(BitBuffer s)
        {
            s.WriteUInt8(Seq);
            s.WriteBool(MoveLeft);
            s.WriteBool(MoveRight);
            s.WriteBool(MoveUp);
            s.WriteBool(MoveDown);
            s.WriteBool(ShootStart);
            s.WriteBool(ShootHold);
            s.WriteBool(Portrait);
            s.WriteBool(TouchMoveActive);
            if (TouchMoveActive)
            {
                s.WriteUnitVec(TouchMoveDir, 8);
                s.WriteUInt8(TouchMoveLen);
            }
            s.WriteUnitVec(ToMouseDir, 10);
            s.WriteFloat(ToMouseLen, 0, NetConstants.MouseMaxDist, 8);

            s.WriteArray(Inputs, 4, (i, _) => s.WriteUInt8((byte)i));

            s.WriteGameType(UseItem);
        }

        public override void Deserialize(BitBuffer s)
        {
            Seq = s.ReadUInt8();
            MoveLeft = s.ReadBool();
            MoveRight = s.ReadBool();
            MoveUp = s.ReadBool();
            MoveDown = s.ReadBool();
            ShootStart = s.ReadBool();
            ShootHold = s.ReadBool();
            Portrait = s.ReadBool();
            TouchMoveActive = s.ReadBool();
            if (TouchMoveActive)
            {
                TouchMoveDir = s.ReadUnitVec(8);
                TouchMoveLen = s.ReadUInt8();
            }
            ToMouseDir = s.ReadUnitVec(10);
            ToMouseLen = s.ReadFloat(0, NetConstants.MouseMaxDist, 8);

            Inputs = s.ReadArray(4, _ => (int)s.ReadUInt8());

            UseItem = s.ReadGameType();
        }
    }

    /// <summary>Port of shared/net/joinedMsg.ts.</summary>
    public sealed class JoinedMsg : AbstractMsg
    {
        public byte TeamMode;
        public ushort PlayerId;
        public bool Started;
        public List<string> Emotes = new List<string>();

        public override void Serialize(BitBuffer s)
        {
            s.WriteUInt8(TeamMode);
            s.WriteUInt16(PlayerId);
            s.WriteBool(Started);
            s.WriteArray(Emotes, 8, (emote, _) => s.WriteGameType(emote));
        }

        public override void Deserialize(BitBuffer s)
        {
            TeamMode = s.ReadUInt8();
            PlayerId = s.ReadUInt16();
            Started = s.ReadBool();
            Emotes = s.ReadArray(8, _ => s.ReadGameType());
        }
    }

    /// <summary>Port of shared/net/killMsg.ts.</summary>
    public sealed class KillMsg : AbstractMsg
    {
        public string ItemSourceType = string.Empty;
        public string MapSourceType = string.Empty;
        public byte DamageType = (byte)Core.DamageType.Player;
        public ushort TargetId;
        public ushort KillerId;
        public ushort KillCreditId;
        public byte KillerKills;
        public bool Downed;
        public bool Killed;

        public override void Serialize(BitBuffer s)
        {
            s.WriteUInt8(DamageType);
            s.WriteGameType(ItemSourceType);
            s.WriteMapType(MapSourceType);
            s.WriteUInt16(TargetId);
            s.WriteUInt16(KillerId);
            s.WriteUInt16(KillCreditId);
            s.WriteUInt8(KillerKills);
            s.WriteBool(Downed);
            s.WriteBool(Killed);
        }

        public override void Deserialize(BitBuffer s)
        {
            DamageType = s.ReadUInt8();
            ItemSourceType = s.ReadGameType();
            MapSourceType = s.ReadMapType();
            TargetId = s.ReadUInt16();
            KillerId = s.ReadUInt16();
            KillCreditId = s.ReadUInt16();
            KillerKills = s.ReadUInt8();
            Downed = s.ReadBool();
            Killed = s.ReadBool();
        }
    }

    /// <summary>Port of shared/net/pickupMsg.ts.</summary>
    public sealed class PickupMsg : AbstractMsg
    {
        public byte Type;
        public string Item = string.Empty;
        public byte Count;

        public override void Serialize(BitBuffer s)
        {
            s.WriteUInt8(Type);
            s.WriteGameType(Item);
            s.WriteUInt8(Count);
        }

        public override void Deserialize(BitBuffer s)
        {
            Type = s.ReadUInt8();
            Item = s.ReadGameType();
            Count = s.ReadUInt8();
        }
    }

    /// <summary>Port of shared/net/aliveCountsMsg.ts.</summary>
    public sealed class AliveCountsMsg : AbstractMsg
    {
        public List<int> TeamAliveCounts = new List<int>();

        public override void Serialize(BitBuffer s)
        {
            s.WriteArray(TeamAliveCounts, 8, (count, _) => s.WriteUInt8((byte)count));
        }

        public override void Deserialize(BitBuffer s)
        {
            TeamAliveCounts = s.ReadArray(8, _ => (int)s.ReadUInt8());
        }
    }

    /// <summary>Port of shared/net/spectateMsg.ts.</summary>
    public enum SpectateAction
    {
        None = 0,
        Begin = 1,
        Next = 2,
        Prev = 3,
    }

    public sealed class SpectateMsg : AbstractMsg
    {
        public SpectateAction Action = SpectateAction.None;

        public override void Serialize(BitBuffer s) => s.WriteUInt8((byte)Action);
        public override void Deserialize(BitBuffer s) => Action = (SpectateAction)s.ReadUInt8();
    }

    /// <summary>Port of shared/net/dropItemMsg.ts.</summary>
    public sealed class DropItemMsg : AbstractMsg
    {
        public string Item = string.Empty;
        public byte WeapIdx;

        public override void Serialize(BitBuffer s)
        {
            s.WriteGameType(Item);
            s.WriteUInt8(WeapIdx);
        }

        public override void Deserialize(BitBuffer s)
        {
            Item = s.ReadGameType();
            WeapIdx = s.ReadUInt8();
        }
    }

    /// <summary>Port of shared/net/emoteMsg.ts.</summary>
    public sealed class EmoteMsg : AbstractMsg
    {
        public Vec2 Pos;
        public string Type = string.Empty;
        public bool IsPing;

        public override void Serialize(BitBuffer s)
        {
            s.WriteVec(Pos, 0, 0, 1024, 1024, 16);
            s.WriteGameType(Type);
            s.WriteBool(IsPing);
        }

        public override void Deserialize(BitBuffer s)
        {
            Pos = s.ReadVec(0, 0, 1024, 1024, 16);
            Type = s.ReadGameType();
            IsPing = s.ReadBool();
        }
    }

    /// <summary>Port of shared/net/editMsg.ts.</summary>
    public sealed class EditMsg : AbstractMsg
    {
        public bool ZoomEnabled;
        public byte Zoom = 1;
        public bool SpeedEnabled;
        public float Speed;
        public bool GameSpeedEnabled;
        public float GameSpeed = 1;
        public bool LoadNewMap;
        public uint NewMapSeed;
        public string SpawnLootType = string.Empty;
        public bool PromoteToRole;
        public string PromoteToRoleType = string.Empty;
        public bool ToggleLayer;
        public bool NoClip;
        public bool TeleportToPings;
        public bool GodMode;
        public bool MoveObjs;
        public bool PreventGameStart;

        public override void Serialize(BitBuffer s)
        {
            s.WriteBool(ZoomEnabled);
            if (ZoomEnabled)
            {
                s.WriteUInt8(Zoom);
            }
            s.WriteBool(SpeedEnabled);
            if (SpeedEnabled)
            {
                s.WriteFloat32(Speed);
            }
            s.WriteBool(GameSpeedEnabled);
            if (GameSpeedEnabled)
            {
                s.WriteFloat32(GameSpeed);
            }
            s.WriteBool(LoadNewMap);
            if (LoadNewMap)
            {
                s.WriteUInt32(NewMapSeed);
            }
            s.WriteGameType(SpawnLootType);
            s.WriteBool(PromoteToRole);
            if (PromoteToRole)
            {
                s.WriteGameType(PromoteToRoleType);
            }
            s.WriteBool(ToggleLayer);
            s.WriteBool(NoClip);
            s.WriteBool(TeleportToPings);
            s.WriteBool(GodMode);
            s.WriteBool(MoveObjs);
            s.WriteBool(PreventGameStart);
        }

        public override void Deserialize(BitBuffer s)
        {
            ZoomEnabled = s.ReadBool();
            if (ZoomEnabled)
            {
                Zoom = s.ReadUInt8();
            }
            SpeedEnabled = s.ReadBool();
            if (SpeedEnabled)
            {
                Speed = s.ReadFloat32();
            }
            GameSpeedEnabled = s.ReadBool();
            if (GameSpeedEnabled)
            {
                GameSpeed = s.ReadFloat32();
            }
            LoadNewMap = s.ReadBool();
            if (LoadNewMap)
            {
                NewMapSeed = s.ReadUInt32();
            }
            SpawnLootType = s.ReadGameType();
            PromoteToRole = s.ReadBool();
            if (PromoteToRole)
            {
                PromoteToRoleType = s.ReadGameType();
            }
            ToggleLayer = s.ReadBool();
            NoClip = s.ReadBool();
            TeleportToPings = s.ReadBool();
            GodMode = s.ReadBool();
            MoveObjs = s.ReadBool();
            PreventGameStart = s.ReadBool();
        }
    }

    /// <summary>Port of shared/net/roleAnnouncementMsg.ts.</summary>
    public sealed class RoleAnnouncementMsg : AbstractMsg
    {
        public ushort PlayerId;
        public ushort KillerId;
        public string Role = string.Empty;
        public bool Assigned;
        public bool Killed;

        public override void Serialize(BitBuffer s)
        {
            s.WriteUInt16(PlayerId);
            s.WriteUInt16(KillerId);
            s.WriteGameType(Role);
            s.WriteBool(Assigned);
            s.WriteBool(Killed);
        }

        public override void Deserialize(BitBuffer s)
        {
            PlayerId = s.ReadUInt16();
            KillerId = s.ReadUInt16();
            Role = s.ReadGameType();
            Assigned = s.ReadBool();
            Killed = s.ReadBool();
        }
    }

    /// <summary>Port of shared/net/perkModeRoleSelectMsg.ts.</summary>
    public sealed class PerkModeRoleSelectMsg : AbstractMsg
    {
        public string Role = string.Empty;

        public override void Serialize(BitBuffer s)
        {
            s.WriteGameType(Role);
            s.WriteBits(0, 6);
        }

        public override void Deserialize(BitBuffer s)
        {
            Role = s.ReadGameType();
            s.ReadBits(6);
        }
    }

    /// <summary>Port of shared/net/playerStatsMsg.ts. Fields are public for reuse.</summary>
    public struct PlayerStats
    {
        public ushort PlayerId;
        public ushort TimeAlive;
        public byte Kills;
        public bool Dead;
        public ushort DamageDealt;
        public ushort DamageTaken;
    }

    public sealed class PlayerStatsMsg : AbstractMsg
    {
        public PlayerStats Stats;

        public override void Serialize(BitBuffer s)
        {
            s.WriteUInt16(Stats.PlayerId);
            s.WriteUInt16(Stats.TimeAlive);
            s.WriteUInt8(Stats.Kills);
            s.WriteUInt8((byte)(Stats.Dead ? 1 : 0));
            s.WriteUInt16(DamageRound(Stats.DamageDealt));
            s.WriteUInt16(DamageRound(Stats.DamageTaken));
        }

        public override void Deserialize(BitBuffer s)
        {
            Stats.PlayerId = s.ReadUInt16();
            Stats.TimeAlive = s.ReadUInt16();
            Stats.Kills = s.ReadUInt8();
            Stats.Dead = s.ReadUInt8() != 0;
            Stats.DamageDealt = s.ReadUInt16();
            Stats.DamageTaken = s.ReadUInt16();
        }

        // JS Math.round: .5 rounds up (unlike .NET's banker's rounding).
        private static ushort DamageRound(float value)
        {
            return (ushort)(int)System.Math.Floor(value + 0.5f);
        }
    }

    /// <summary>Port of shared/net/gameOverMsg.ts.</summary>
    public sealed class GameOverMsg : AbstractMsg
    {
        public byte TeamId;
        public byte TeamRank;
        public bool GameOver;
        public byte WinningTeamId;
        public List<PlayerStats> PlayerStats = new List<PlayerStats>();

        public override void Serialize(BitBuffer s)
        {
            s.WriteUInt8(TeamId);
            s.WriteUInt8(TeamRank);
            s.WriteUInt8((byte)(GameOver ? 1 : 0));
            s.WriteUInt8(WinningTeamId);

            s.WriteArray(PlayerStats, 8, (stats, _) =>
            {
                s.WriteUInt16(stats.PlayerId);
                s.WriteUInt16(stats.TimeAlive);
                s.WriteUInt8(stats.Kills);
                s.WriteUInt8((byte)(stats.Dead ? 1 : 0));
                s.WriteUInt16((ushort)(int)System.Math.Floor(stats.DamageDealt + 0.5f));
                s.WriteUInt16((ushort)(int)System.Math.Floor(stats.DamageTaken + 0.5f));
            });
        }

        public override void Deserialize(BitBuffer s)
        {
            TeamId = s.ReadUInt8();
            TeamRank = s.ReadUInt8();
            GameOver = s.ReadUInt8() != 0;
            WinningTeamId = s.ReadUInt8();

            PlayerStats = s.ReadArray(8, _ => new PlayerStats
            {
                PlayerId = s.ReadUInt16(),
                TimeAlive = s.ReadUInt16(),
                Kills = s.ReadUInt8(),
                Dead = s.ReadUInt8() != 0,
                DamageDealt = s.ReadUInt16(),
                DamageTaken = s.ReadUInt16(),
            });
        }
    }

    /// <summary>Port of the empty UpdatePassMsg in net.ts.</summary>
    public sealed class UpdatePassMsg : AbstractMsg
    {
        public override void Serialize(BitBuffer s)
        {
        }

        public override void Deserialize(BitBuffer s)
        {
        }
    }
}
