#pragma once
#include "pch.h"
#include "Runtime.h"

namespace Inventory
{
	AFortPickupAthena* SpawnPickup(FVector, FFortItemEntry*, int, AFortPlayerPawnAthena*, EFortPickupSourceTypeFlag, EFortPickupSpawnSource, bool, bool, FVector = {});
	AFortPickupAthena* SpawnPickup(FVector, UFortItemDefinition*, int, AFortPlayerPawnAthena*, EFortPickupSourceTypeFlag, EFortPickupSpawnSource, bool, bool, FVector = {});

	bool RemoveInventoryItem(IInterface*, FGuid&, int, bool);
	void SetLoadedAmmo(UFortWorldItem*, int);

	AFortPickup* K2_SpawnPickupInWorld(UObject*, FFrame&, AFortPickup**);

	void Patch();
}