#pragma once
#include "pch.h"

enum class EAmmoType : uint8
{
    Assault = 0,
    Shotgun = 1,
    Submachine = 2,
    Rocket = 3,
    Sniper = 4
};

namespace Lategame
{
    void InitializeLategameLoadouts();
    void GivePlayerRandomWeapon(AFortPlayerControllerAthena*);

    UFortAmmoItemDefinition* GetAmmo(EAmmoType);
    UFortResourceItemDefinition* GetResource(EFortResourceType);
}