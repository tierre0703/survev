// Roundtrip tests for the ported net protocol.
//
// The real gate (plan.md M2) is byte-exact parity with captured web-client
// frames. Until fixtures are captured, these tests assert that each message
// serializes -> deserializes -> re-serializes to the SAME bytes, which catches
// field-order and width regressions across the whole message set.
using System.Collections.Generic;
using NUnit.Framework;
using Survev.Core;
using Survev.Net;

namespace Survev.Tests.EditMode
{
    public class ProtocolTests
    {
        private static byte[] Roundtrip(AbstractMsg msg, AbstractMsg reader)
        {
            var write = new MsgStream();
            write.SerializeMsg(TypeOf(msg), msg);
            byte[] bytes = write.GetBuffer();

            var stream = new MsgStream(bytes);
            MsgType type = stream.DeserializeMsgType();
            reader.Deserialize(stream.Stream);

            var reWrite = new MsgStream();
            reWrite.SerializeMsg(type, reader);
            return reWrite.GetBuffer();
        }

        private static MsgType TypeOf(AbstractMsg msg)
        {
            if (msg is JoinMsg)
            {
                return MsgType.Join;
            }
            if (msg is InputMsg)
            {
                return MsgType.Input;
            }
            if (msg is EditMsg)
            {
                return MsgType.Edit;
            }
            if (msg is EmoteMsg)
            {
                return MsgType.Emote;
            }
            if (msg is MapMsg)
            {
                return MsgType.Map;
            }
            if (msg is KillMsg)
            {
                return MsgType.Kill;
            }
            if (msg is PickupMsg)
            {
                return MsgType.Pickup;
            }
            throw new AssertionException("unmapped message type");
        }

        private static void AssertRoundtrip(AbstractMsg msg, AbstractMsg reader)
        {
            byte[] first = Roundtrip(msg, reader);
            var replay = new MsgStream(first);
            replay.DeserializeMsgType();
            reader.Deserialize(replay.Stream);
            var third = new MsgStream();
            third.SerializeMsg(TypeOf(reader), reader);
            Assert.AreEqual(first, third.GetBuffer());
        }

        [Test]
        public void JoinMsg_Roundtrips()
        {
            var msg = new JoinMsg
            {
                Protocol = GameConfig.ProtocolVersion,
                JoinToken = "abcd-1234",
                Name = "Player One",
                UseTouch = true,
                IsMobile = true,
            };
            msg.Loadout.Outfit = "outfit_01";
            msg.Loadout.Melee = "fists";
            msg.Loadout.Heal = "bandage";
            msg.Loadout.Boost = "soda";
            msg.Loadout.Emotes = new List<string> { "emote_01", "emote_02" };
            AssertRoundtrip(msg, new JoinMsg());
        }

        [Test]
        public void InputMsg_Roundtrips()
        {
            var msg = new InputMsg
            {
                Seq = 7,
                ShootHold = true,
                Portrait = true,
                TouchMoveActive = true,
                TouchMoveDir = new Vec2(0.5f, -0.5f),
                TouchMoveLen = 200,
                ToMouseDir = new Vec2(0.25f, 0.75f),
                ToMouseLen = 12.5f,
                UseItem = "bandage",
            };
            msg.AddInput(Input.Fire);
            msg.AddInput(Input.Reload);
            AssertRoundtrip(msg, new InputMsg());
        }

        [Test]
        public void EditMsg_Roundtrips()
        {
            var msg = new EditMsg
            {
                ZoomEnabled = true,
                Zoom = 4,
                SpeedEnabled = true,
                Speed = 3.5f,
                NoClip = true,
                GodMode = true,
                SpawnLootType = "loot_gun_mp5",
                PromoteToRole = true,
                PromoteToRoleType = "role_scout",
            };
            AssertRoundtrip(msg, new EditMsg());
        }

        [Test]
        public void MapMsg_Roundtrips()
        {
            var msg = new MapMsg
            {
                MapName = "main",
                Seed = 123456u,
                Width = 1024,
                Height = 1024,
                ShoreInset = 16,
                GrassInset = 32,
                Objects = new List<MapObj>
                {
                    new MapObj { Pos = new Vec2(100, 200), Scale = 1f, Type = "map_barrel_01", Ori = 1 },
                },
                GroundPatches = new List<GroundPatch>
                {
                    new GroundPatch
                    {
                        Bound = Collider.CreateAabb(new Vec2(0, 0), new Vec2(64, 64)),
                        Color = 0x80AF49,
                        Roughness = 0.5f,
                        OffsetDist = 2f,
                        Order = 3,
                        UseAsMapShape = true,
                    },
                },
            };
            msg.Rivers.Add(new MapRiverData { Width = 4, Looped = false, Points = new List<Vec2> { new Vec2(10, 10) } });
            msg.Places.Add(new MapPlace { Name = "Dock", Pos = new Vec2(0.5f, 0.5f) });
            AssertRoundtrip(msg, new MapMsg());
        }

        [Test]
        public void KillMsg_Roundtrips()
        {
            var msg = new KillMsg
            {
                DamageType = (byte)DamageType.Player,
                ItemSourceType = "gun_mp5",
                TargetId = 12,
                KillerId = 7,
                KillCreditId = 7,
                KillerKills = 3,
                Downed = true,
                Killed = false,
            };
            AssertRoundtrip(msg, new KillMsg());
        }

        [Test]
        public void PickupMsg_Roundtrips()
        {
            var msg = new PickupMsg { Type = (byte)PickupMsgType.Success, Item = "loot_bandage", Count = 5 };
            AssertRoundtrip(msg, new PickupMsg());
        }

        [Test]
        public void UpdateMsg_Roundtrips_WithFullAndPartialObjects()
        {
            var msg = new UpdateMsg();
            msg.ActivePlayerIdDirty = true;
            msg.ActivePlayerId = 99;
            msg.ActivePlayerData.HealthDirty = true;
            msg.ActivePlayerData.Health = 87.5f;
            msg.ActivePlayerData.InventoryDirty = true;
            msg.ActivePlayerData.Inventory["bandage"] = 5;
            msg.ActivePlayerData.WeapsDirty = true;
            msg.ActivePlayerData.Weapons = new LocalDataWithDirty.WeaponData[GameConfig.WeaponSlotCount];
            msg.ActivePlayerData.Weapons[0] = new LocalDataWithDirty.WeaponData { Type = "gun_mp5", Ammo = 30 };
            msg.Ack = 42;

            msg.FullObjects.Add(new ObjectData
            {
                Type = ObjectType.Player,
                Id = 99,
                Pos = new Vec2(100, 120),
                Dir = new Vec2(1, 0),
                Outfit = "outfit_01",
                ActiveWeapon = "gun_mp5",
                Scale = 1f,
                HasteType = (int)HasteType.None,
                HasteSeq = -1,
            });

            msg.PartObjects.Add(new ObjectData
            {
                Type = ObjectType.Obstacle,
                Id = 12,
                Pos = new Vec2(50, 60),
                Ori = 1,
                Scale = 1f,
            });

            msg.Bullets.Add(new Bullet
            {
                PlayerId = 99,
                StartPos = new Vec2(100, 120),
                Dir = new Vec2(1, 0),
                BulletType = "bullet_mp5",
                Layer = 0,
                VarianceT = 0.5f,
                DistAdjIdx = 2,
                HasModifier = false,
                HasSpecialFx = false,
                SpeedMult = 1,
                DistanceMult = 1,
            });

            byte[] first = Roundtrip(msg, new UpdateMsg());
            var reader = new UpdateMsg();
            var stream = new MsgStream(first);
            stream.DeserializeMsgType();
            reader.Deserialize(stream.Stream);
            var reWrite = new MsgStream();
            reWrite.SerializeMsg(MsgType.Update, reader);
            Assert.AreEqual(first, reWrite.GetBuffer());
        }

        [Test]
        public void GameOverMsg_Roundtrips()
        {
            var msg = new GameOverMsg
            {
                TeamId = 1,
                TeamRank = 2,
                GameOver = true,
                WinningTeamId = 3,
            };
            msg.PlayerStats.Add(new PlayerStats
            {
                PlayerId = 5,
                TimeAlive = 120,
                Kills = 4,
                Dead = true,
                DamageDealt = 250,
                DamageTaken = 180,
            });
            AssertRoundtrip(msg, new GameOverMsg());
        }

        [Test]
        public void TypeRegistry_RoundTripsIds()
        {
            Assert.AreEqual("gun_mp5", TypeRegistry.GameIdToType(TypeRegistry.GameTypeToId("gun_mp5")));
            Assert.AreEqual("", TypeRegistry.GameIdToType(TypeRegistry.GameTypeToId("not_a_real_type")));
        }
    }
}
