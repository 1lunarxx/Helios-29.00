#include "pch.h"
#include "LateGame.h"
#include "Runtime.h"

struct LategameWeapons
{
    std::vector<UFortWeaponItemDefinition*> RifleOptions;
    std::vector<UFortWeaponItemDefinition*> PumpWeapons;
    std::vector<UFortWeaponRangedItemDefinition*> RangedOrSprayGuns;
    std::vector<std::pair<UFortItemDefinition*, int>> Meds;
};

LategameWeapons Loadout{};
void Lategame::InitializeLategameLoadouts()
{
    Loadout.RifleOptions.push_back(Runtime::StaticFindObject<UFortWeaponRangedItemDefinition>("/PaprikaCoreWeapons/Items/Weapons/PaprikaSMG_Burst/WID_SMG_Paprika_Burst_Athena_VR.WID_SMG_Paprika_Burst_Athena_VR"));
    Loadout.RifleOptions.push_back(Runtime::StaticFindObject<UFortWeaponRangedItemDefinition>("/SunRoseWeaponsGameplay/Items/Weapons/HarbingerSMG/WID_SMG_SunRose_DPS_Athena_VR.WID_SMG_SunRose_DPS_Athena_VR"));
    Loadout.RifleOptions.push_back(Runtime::StaticFindObject<UFortWeaponRangedItemDefinition>("/SunRoseWeaponsGameplay/Items/Weapons/WarforgedAR/WID_Assault_SunRose_Athena_VR.WID_Assault_SunRose_Athena_VR"));

    Loadout.PumpWeapons.push_back(Runtime::StaticFindObject<UFortWeaponRangedItemDefinition>("/SunRoseWeaponsGameplay/Items/Weapons/CerberusSG/WID_Shotgun_Break_Cerberus_Athena_SR.WID_Shotgun_Break_Cerberus_Athena_SR"));
    Loadout.PumpWeapons.push_back(Runtime::StaticFindObject<UFortWeaponRangedItemDefinition>("/PaprikaCoreWeapons/Items/Weapons/PaprikaShotgun_Pump/WID_Shotgun_Pump_Paprika_Athena_VR.WID_Shotgun_Pump_Paprika_Athena_VR"));
    Loadout.PumpWeapons.push_back(Runtime::StaticFindObject<UFortWeaponRangedItemDefinition>("/PaprikaCoreWeapons/Items/Weapons/PaprikaShotgun_Pump/WID_Shotgun_Pump_Paprika_Athena_VR.WID_Shotgun_Pump_Paprika_Athena_VR"));

    Loadout.RangedOrSprayGuns.push_back(Runtime::StaticFindObject<UFortWeaponRangedItemDefinition>("/PaprikaCoreWeapons/Items/Weapons/PaprikaSniper/WID_Sniper_Paprika_Athena_SR.WID_Sniper_Paprika_Athena_SR"));
    Loadout.RangedOrSprayGuns.push_back(Runtime::StaticFindObject<UFortWeaponRangedItemDefinition>("/PaprikaCoreWeapons/Items/Weapons/PaprikaSniper/WID_Sniper_Paprika_Athena_SR.WID_Sniper_Paprika_Athena_SR"));
    Loadout.RangedOrSprayGuns.push_back(Runtime::StaticFindObject<UFortWeaponRangedItemDefinition>("/SunRoseWeaponsGameplay/Items/Weapons/ModularDMR/WID_SunRose_ModularDMR_Athena_VR.WID_SunRose_ModularDMR_Athena_VR"));
    Loadout.RangedOrSprayGuns.push_back(Runtime::StaticFindObject<UFortWeaponRangedItemDefinition>("/SunRoseWeaponsGameplay/Items/Weapons/ModularDMR/WID_SunRose_ModularDMR_Athena_VR.WID_SunRose_ModularDMR_Athena_VR"));

    Loadout.Meds.push_back({ Runtime::StaticFindObject<UFortItemDefinition>("/Game/Athena/Items/Consumables/Shields/Athena_Shields.Athena_Shields"), 3 });
    Loadout.Meds.push_back({ Runtime::StaticFindObject<UFortItemDefinition>("/Game/Athena/Items/Consumables/Medkit/Athena_Medkit.Athena_Medkit"), 3 });
}

UFortAmmoItemDefinition* Lategame::GetAmmo(EAmmoType AmmoType)
{
    static std::vector<UFortAmmoItemDefinition*> Ammos
    {
        Runtime::StaticFindObject<UFortAmmoItemDefinition>("/Game/Athena/Items/Ammo/AthenaAmmoDataBulletsLight.AthenaAmmoDataBulletsLight"),
        Runtime::StaticFindObject<UFortAmmoItemDefinition>("/Game/Athena/Items/Ammo/AthenaAmmoDataShells.AthenaAmmoDataShells"),
        Runtime::StaticFindObject<UFortAmmoItemDefinition>("/Game/Athena/Items/Ammo/AthenaAmmoDataBulletsMedium.AthenaAmmoDataBulletsMedium"),
        Runtime::StaticFindObject<UFortAmmoItemDefinition>("/Game/Athena/Items/Ammo/AmmoDataRockets.AmmoDataRockets"),
        Runtime::StaticFindObject<UFortAmmoItemDefinition>("/Game/Athena/Items/Ammo/AthenaAmmoDataBulletsHeavy.AthenaAmmoDataBulletsHeavy")
    };

    return Ammos[(uint8)AmmoType];
}

UFortResourceItemDefinition* Lategame::GetResource(EFortResourceType ResourceType)
{
    static std::vector<UFortResourceItemDefinition*> Resources
    {
        Runtime::StaticFindObject<UFortResourceItemDefinition>("/Game/Items/ResourcePickups/WoodItemData.WoodItemData"),
        Runtime::StaticFindObject<UFortResourceItemDefinition>("/Game/Items/ResourcePickups/StoneItemData.StoneItemData"),
        Runtime::StaticFindObject<UFortResourceItemDefinition>("/Game/Items/ResourcePickups/MetalItemData.MetalItemData")
    };

    return Resources[(uint8)ResourceType];
}

void Lategame::GivePlayerRandomWeapon(AFortPlayerControllerAthena* Controller)
{
    AFortInventory* Inventory = (AFortInventory*)Controller->WorldInventory.ObjectPointer;
    if (!Inventory)
        return;

    auto Index = UKismetMathLibrary::RandomInteger(Loadout.RifleOptions.size() - 1);
    Inventory->AddItem(Loadout.RifleOptions[Index], 1, Inventory->GetStats((UFortWeaponItemDefinition*)Loadout.RifleOptions[Index])->ClipSize);

    Index = UKismetMathLibrary::RandomInteger(Loadout.PumpWeapons.size() - 1);
    Inventory->AddItem(Loadout.PumpWeapons[Index], 1, Inventory->GetStats((UFortWeaponItemDefinition*)Loadout.PumpWeapons[Index])->ClipSize);

    if (UKismetMathLibrary::RandomBool())
    {
        Index = UKismetMathLibrary::RandomInteger(Loadout.RangedOrSprayGuns.size() - 1);
        Inventory->AddItem(Loadout.RangedOrSprayGuns[Index], 1, Inventory->GetStats((UFortWeaponItemDefinition*)Loadout.RangedOrSprayGuns[Index])->ClipSize);
    }
    else
    {
        Inventory->AddItem(Runtime::StaticFindObject<UFortWeaponRangedItemDefinition>("/PaprikaConsumables/Gameplay/TeamSpray/WID_Paprika_TeamSpray_LowGrav.WID_Paprika_TeamSpray_LowGrav"), 1, 100);
    }

    if (UKismetMathLibrary::RandomBool())
    {
        Index = UKismetMathLibrary::RandomInteger(2);
        Inventory->AddItem(Loadout.Meds[Index].first, Loadout.Meds[Index].second);
    }
    else
    {
        Inventory->AddItem(Runtime::StaticFindObject<UFortWeaponRangedItemDefinition>("/PaprikaConsumables/Gameplay/ShieldSmallRefresh/WID_Paprika_ShieldSmall.WID_Paprika_ShieldSmall"), 6);
    }

    if (UKismetMathLibrary::RandomBool())
    {
        Index = UKismetMathLibrary::RandomInteger(2);
        Inventory->AddItem(Loadout.Meds[Index].first, Loadout.Meds[Index].second);
    }
    else
    {
        Inventory->AddItem(Runtime::StaticFindObject<UFortWeaponRangedItemDefinition>("/PaprikaConsumables/Gameplay/ShieldSmallRefresh/WID_Paprika_ShieldSmall.WID_Paprika_ShieldSmall"), 6);
    }

    Inventory->AddItem(GetResource(EFortResourceType::Wood), 500);
    Inventory->AddItem(GetResource(EFortResourceType::Stone), 500);
    Inventory->AddItem(GetResource(EFortResourceType::Metal), 500);

    Inventory->AddItem(GetAmmo(EAmmoType::Assault), 250);
    Inventory->AddItem(GetAmmo(EAmmoType::Shotgun), 100);
    Inventory->AddItem(GetAmmo(EAmmoType::Submachine), 400);
    Inventory->AddItem(GetAmmo(EAmmoType::Sniper), 20);
}