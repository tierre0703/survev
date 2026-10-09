// Port of shared/gameConfig.ts — protocol + gameplay enums/constants.
//
// GENERATED from the real TypeScript source by tools/codegen-config.mjs.
// Do not hand-edit: change ../ shared/gameConfig.ts and regenerate.
//
// Values must match the server exactly; a mismatch breaks the bit protocol.
namespace Survev.Core
{
    public enum Action
    {
        None = 0,
        Reload = 1,
        ReloadAlt = 2,
        UseItem = 3,
        Revive = 4,
        Count = 5,
    }

    public enum Anim
    {
        None = 0,
        Melee = 1,
        Cook = 2,
        Throw = 3,
        CrawlForward = 4,
        CrawlBackward = 5,
        Revive = 6,
        DeployMelee = 7,
        IdleMelee = 8,
        Count = 9,
    }

    public enum DamageType
    {
        Player = 0,
        Bleeding = 1,
        Gas = 2,
        Airdrop = 3,
        Airstrike = 4,
    }

    public enum EmoteSlot
    {
        Top = 0,
        Right = 1,
        Bottom = 2,
        Left = 3,
        Win = 4,
        Death = 5,
        Count = 6,
    }

    public enum GasMode
    {
        Inactive = 0,
        Waiting = 1,
        Moving = 2,
    }

    public enum HasteType
    {
        None = 0,
        Windwalk = 1,
        Takedown = 2,
        Inspire = 3,
        Count = 4,
    }

    public static class Input
    {
        public const int MoveLeft = 0;
        public const int MoveRight = 1;
        public const int MoveUp = 2;
        public const int MoveDown = 3;
        public const int Fire = 4;
        public const int Reload = 5;
        public const int Cancel = 6;
        public const int Interact = 7;
        public const int Revive = 8;
        public const int Use = 9;
        public const int Loot = 10;
        public const int EquipPrimary = 11;
        public const int EquipSecondary = 12;
        public const int EquipMelee = 13;
        public const int EquipThrowable = 14;
        public const int EquipFragGrenade = 15;
        public const int EquipSmokeGrenade = 16;
        public const int EquipNextWeap = 17;
        public const int EquipPrevWeap = 18;
        public const int EquipLastWeap = 19;
        public const int EquipOtherGun = 20;
        public const int EquipPrevScope = 21;
        public const int EquipNextScope = 22;
        public const int UseBandage = 23;
        public const int UseHealthKit = 24;
        public const int UseSoda = 25;
        public const int UsePainkiller = 26;
        public const int StowWeapons = 27;
        public const int SwapWeapSlots = 28;
        public const int ToggleMap = 29;
        public const int CycleUIMode = 30;
        public const int EmoteMenu = 31;
        public const int TeamPingMenu = 32;
        public const int Fullscreen = 33;
        public const int HideUI = 34;
        public const int TeamPingSingle = 35;
        public const int Count = 36;
    }

    public enum MapId
    {
        Main = 0,
        Desert = 1,
        Woods = 2,
        Faction = 3,
        Potato = 4,
        Savannah = 5,
        Halloween = 6,
        Cobalt = 7,
        Birthday = 8,
        Beach = 9,
        FactionPotato = 10,
    }

    public enum Plane
    {
        Airdrop = 0,
        Airstrike = 1,
    }

    public enum WeaponSlot
    {
        Primary = 0,
        Secondary = 1,
        Melee = 2,
        Throwable = 3,
        Count = 4,
    }

    public enum Rarity
    {
        Stock = 0,
        Common = 1,
        Uncommon = 2,
        Rare = 3,
        Epic = 4,
        Mythic = 5,
    }

    public enum TeamMode
    {
        Solo = 1,
        Duo = 2,
        Squad = 4,
    }

    public enum FactionTeam
    {
        Red = 1,
        Blue = 2,
    }

    public static class GameConfig
    {
        /// <summary>Bump whenever a serialization function changes.</summary>
        public const int ProtocolVersion = 1028;

        /// <summary>GameConfig.projectile.maxHeight (objectSerializeFns.ts).</summary>
        public const int ProjectileMaxHeight = 5;

        /// <summary>GameConfig.structureLayerCount (objectSerializeFns.ts).</summary>
        public const int StructureLayerCount = 2;

        /// <summary>GameConfig.WeaponSlot.Count.</summary>
        public const int WeaponSlotCount = 4;

        /// <summary>
        /// Keys of GameConfig.bagSizes, in declaration order. The inventory block
        /// of the active-player state is serialized in this exact order.
        /// </summary>
        public static readonly string[] BagSizes =
        {
            "9mm", "762mm", "556mm", "12gauge", "50AE", "308sub", "flare", "45acp",
            "frag", "smoke", "strobe", "mirv", "snowball", "potato", "tomato",
            "coconut", "bandage", "healthkit", "soda", "painkiller",
            "1xscope", "2xscope", "4xscope", "8xscope", "15xscope",
        };
    }
}
