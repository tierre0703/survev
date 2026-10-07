#pragma once
// Port of shared/gameConfig.ts enums + core config constants.
#include <cstdint>

namespace surv {

enum Action : uint8_t {
    Action_None,
    Action_Reload,
    Action_ReloadAlt,
    Action_UseItem,
    Action_Revive,
    Action_Count,
};

enum Anim : uint8_t {
    Anim_None,
    Anim_Melee,
    Anim_Cook,
    Anim_Throw,
    Anim_CrawlForward,
    Anim_CrawlBackward,
    Anim_Revive,
    Anim_DeployMelee,
    Anim_IdleMelee,
    Anim_Count,
};

enum DamageType : uint8_t {
    DamageType_Player,
    DamageType_Bleeding,
    DamageType_Gas,
    DamageType_Airdrop,
    DamageType_Airstrike,
};

enum GasMode : uint8_t {
    GasMode_Inactive,
    GasMode_Waiting,
    GasMode_Moving,
};

enum HasteType : uint8_t {
    HasteType_None,
    HasteType_Windwalk,
    HasteType_Takedown,
    HasteType_Inspire,
    HasteType_Count,
};

enum Input : uint8_t {
    Input_MoveLeft,
    Input_MoveRight,
    Input_MoveUp,
    Input_MoveDown,
    Input_Fire,
    Input_Reload,
    Input_Cancel,
    Input_Interact,
    Input_Revive,
    Input_Use,
    Input_Loot,
    Input_EquipPrimary,
    Input_EquipSecondary,
    Input_EquipMelee,
    Input_EquipThrowable,
    Input_EquipFragGrenade,
    Input_EquipSmokeGrenade,
    Input_EquipNextWeap,
    Input_EquipPrevWeap,
    Input_EquipLastWeap,
    Input_EquipOtherGun,
    Input_EquipPrevScope,
    Input_EquipNextScope,
    Input_UseBandage,
    Input_UseHealthKit,
    Input_UseSoda,
    Input_UsePainkiller,
    Input_StowWeapons,
    Input_SwapWeapSlots,
    Input_ToggleMap,
    Input_CycleUIMode,
    Input_EmoteMenu,
    Input_TeamPingMenu,
    Input_Fullscreen,
    Input_HideUI,
    Input_TeamPingSingle,
    Input_Count,
};

enum MapId : uint8_t {
    MapId_Main = 0,
    MapId_Desert = 1,
    MapId_Woods = 2,
    MapId_Faction = 3,
    MapId_Potato = 4,
    MapId_Savannah = 5,
    MapId_Halloween = 6,
    MapId_Cobalt = 7,
    MapId_Birthday = 8,
    MapId_Beach = 9,
    MapId_FactionPotato = 10,
};

enum Plane : uint8_t {
    Plane_Airdrop,
    Plane_Airstrike,
};

enum WeaponSlot : uint8_t {
    WeaponSlot_Primary,
    WeaponSlot_Secondary,
    WeaponSlot_Melee,
    WeaponSlot_Throwable,
    WeaponSlot_Count,
};

enum Rarity : uint8_t {
    Rarity_Stock,
    Rarity_Common,
    Rarity_Uncommon,
    Rarity_Rare,
    Rarity_Epic,
    Rarity_Mythic,
};

enum TeamMode : uint8_t {
    TeamMode_Solo = 1,
    TeamMode_Duo = 2,
    TeamMode_Squad = 4,
};

enum FactionTeam : uint8_t {
    FactionTeam_Red = 1,
    FactionTeam_Blue = 2,
};

// Port of the GameConfig.player values used by touch input.
struct PlayerConfig {
    static constexpr float throwableMaxMouseDist = 18.0f;
    // player.ts m_rad = netData.scale * GameConfig.player.radius.
    static constexpr float radius = 1.0f;
    // player.ts melee collision (GameConfig.player.meleeHeight).
    static constexpr float meleeHeight = 0.25f;
    // player.ts aura (medic heal/revive circle) radii.
    static constexpr float medicHealRange = 8.0f;
    static constexpr float medicReviveRange = 6.0f;
    // player.ts maxVisualRadius (stairs sort test).
    static constexpr float maxVisualRadius = 1.0f;
};

// Port of GameConfig.map (terrain generation + grid).
struct MapConfig {
    static constexpr float gridSize = 16.0f;
    static constexpr float shoreVariation = 3.0f;
    static constexpr float grassVariation = 2.0f;
};

// Port of GameConfig.projectile used by the projectile object barn.
struct ProjectileConfig {
    static constexpr float maxHeight = 256.0f;
};

// Server net-sync/input rate (server/src/.../config.ts netSyncTps). The client
// sends InputMsg at this cadence instead of once per rendered frame.
inline constexpr float kNetSyncTps = 33.0f;

} // namespace surv