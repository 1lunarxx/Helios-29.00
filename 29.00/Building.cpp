#include "pch.h"
#include "Building.h"
#include "Inventory.h"
#include "memcury.h"
#include "Options.h"

bool Building::CanBePlacedByPlayer(UClass* BuildClass) {
	return ((AFortGameStateAthena*)UWorld::GetWorld()->GameState)->AllPlayerBuildableClasses.Search([BuildClass](UClass* Class) { return Class == BuildClass; });
}

void Building::SetEditingPlayer(ABuildingSMActor* Building, AFortPlayerStateAthena* EditingPlayer)
{
	if (Building->Role == ENetRole::ROLE_Authority && (!Building->EditingPlayer || !EditingPlayer))
	{
		Building->SetNetDormancy(ENetDormancy(2 - (EditingPlayer != 0)));
		Building->ForceNetUpdate();

		if (Building->EditingPlayer)
		{
			auto Owner = Building->EditingPlayer->Owner;
			if (Owner)
			{
				if (AFortPlayerControllerAthena* PlayerController = static_cast<AFortPlayerControllerAthena*>(Owner))
				{
					Building->EditingPlayer = EditingPlayer;
					Building->OnRep_EditingPlayer();
				}

				return;
			}
		}
		else
		{
			if (!EditingPlayer)
			{
				Building->EditingPlayer = EditingPlayer;
				Building->OnRep_EditingPlayer();

				return;
			}

			auto Owner = EditingPlayer->Owner;
			if (Owner)
			{
				if (AFortPlayerControllerAthena* PlayerController = static_cast<AFortPlayerControllerAthena*>(Owner))
				{
					Building->EditingPlayer = EditingPlayer;
					Building->OnRep_EditingPlayer();
				}

				return;
			}
		}
	}
}

void Building::OnDamageServer(ABuildingSMActor* BuildingSMActor, float Damage, FGameplayTagContainer DamageTags, FVector Momentum, FHitResult HitInfo, AFortPlayerControllerAthena* InstigatedBy, AActor* DamageCauser, FGameplayEffectContextHandle EffectContext)
{
	if (!InstigatedBy || BuildingSMActor->bPlayerPlaced || BuildingSMActor->GetHealth() == 1)
		return Originals::OnDamageServer(BuildingSMActor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);

	if (!DamageCauser || !DamageCauser->IsA(AFortWeapon::StaticClass()) || !((AFortWeapon*)DamageCauser)->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
		return Originals::OnDamageServer(BuildingSMActor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);

	UFortResourceItemDefinition* Resource = UFortKismetLibrary::K2_GetResourceItemDefinition(BuildingSMActor->ResourceType);
	if (!Resource)
		return Originals::OnDamageServer(BuildingSMActor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);

	AFortInventory* Inventory = static_cast<AFortInventory*>(InstigatedBy->WorldInventory.ObjectPointer);
	int32 ResourceCount = (int32)std::round(UKismetMathLibrary::RandomIntegerInRange(11, 33) / (BuildingSMActor->GetMaxHealth() / Damage));

	if (ResourceCount > 0)
	{
		FFortItemEntry* ItemEntry = Inventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& Entry) {
			return Entry.ItemDefinition == Resource;
		});

		if (ItemEntry)
		{
			ItemEntry->Count += ResourceCount;
			if (ItemEntry->Count > 500)
			{
				Inventory::SpawnPickup(InstigatedBy->Pawn->K2_GetActorLocation(), ItemEntry, ItemEntry->Count - 500, (AFortPlayerPawnAthena*)InstigatedBy->MyFortPawn, EFortPickupSourceTypeFlag::Tossed, EFortPickupSpawnSource::Unset, true, true);
				ItemEntry->Count = 500;
			}

			Inventory->UpdateEntry(ItemEntry);
		}
		else
		{
			if (ResourceCount > 500)
			{
				Inventory::SpawnPickup(InstigatedBy->Pawn->K2_GetActorLocation(), Resource, ResourceCount - 500, (AFortPlayerPawnAthena*)InstigatedBy->MyFortPawn, EFortPickupSourceTypeFlag::Tossed, EFortPickupSpawnSource::Unset, true, true);
				ResourceCount = 500;
			}

			Inventory->AddItem(Resource, ResourceCount);
		}
	}

	InstigatedBy->ClientReportDamagedResourceBuilding(BuildingSMActor, BuildingSMActor->ResourceType, ResourceCount, false, Damage == 100.f);
	return Originals::OnDamageServer(BuildingSMActor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
}

void Building::ServerCreateBuildingActor(AFortPlayerControllerAthena* PlayerController, FCreateBuildingActorData BuildingActorData)
{
	if (!PlayerController)
		return Originals::ServerCreateBuildingActor(PlayerController, BuildingActorData);

	struct _Pad_0x18
	{
		uint8_t Padding[0x18];
	};

	AFortGameStateAthena* GameState = static_cast<AFortGameStateAthena*>(UWorld::GetWorld()->GameState);
	TSubclassOf<ABuildingActor> BuildingClass = *GameState->AllPlayerBuildableClassesIndexLookup.SearchForKey([&](UClass* Class, int32 Handle) {
		return Handle == BuildingActorData.BuildingClassHandle;
	});

	if (!BuildingClass)
		return Originals::ServerCreateBuildingActor(PlayerController, BuildingActorData);

	TArray<ABuildingSMActor*> RemoveBuildings;
	char var;

	auto CantBuild = (__int64 (*)(UWorld*, TSubclassOf<ABuildingActor>&, _Pad_0x18, _Pad_0x18, bool, TArray<ABuildingSMActor*> *, char*))(ImageBase + 0x8CEA384);
	if (!CantBuild(UWorld::GetWorld(), BuildingClass, *(_Pad_0x18*)&BuildingActorData.BuildLoc, *(_Pad_0x18*)&BuildingActorData.BuildRot, BuildingActorData.bMirrored, &RemoveBuildings, &var))
	{
		ABuildingSMActor* Building = Runtime::SpawnActor<ABuildingSMActor>(BuildingActorData.BuildLoc, BuildingActorData.BuildRot, BuildingClass, PlayerController);
		if (!Building)
			return Originals::ServerCreateBuildingActor(PlayerController, BuildingActorData);

		Building->CurrentBuildingLevel = BuildingActorData.BuildingClassData.UpgradeLevel;
		Building->OnRep_CurrentBuildingLevel();

		Building->SetMirrored(BuildingActorData.bMirrored);;
		Building->bPlayerPlaced = true;

		Building->InitializeKismetSpawnedBuildingActor(Building, PlayerController, true, nullptr, false);
		Building->TeamIndex = ((AFortPlayerStateAthena*)PlayerController->PlayerState)->TeamIndex;
		Building->SetTeam(Building->TeamIndex);

		for (auto& RemoveBuilding : RemoveBuildings)
		{
			RemoveBuilding->K2_DestroyActor();
		}

		RemoveBuildings.Free();

		AFortInventory* Inventory = static_cast<AFortInventory*>(PlayerController->WorldInventory.ObjectPointer);
		UFortItemDefinition* ResourceItemDefinition = UFortKismetLibrary::K2_GetResourceItemDefinition(Building->ResourceType);

		if (!ResourceItemDefinition)
			return Originals::ServerCreateBuildingActor(PlayerController, BuildingActorData);

		FFortItemEntry* ItemEntry = Inventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& Entry)
		{
			return Entry.ItemDefinition == ResourceItemDefinition;
		});

		if (!ItemEntry)
			return Originals::ServerCreateBuildingActor(PlayerController, BuildingActorData);

		if (UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(GameState))
		{
			if (UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(GameState)->GamePhase != EAthenaGamePhase::Warmup)
			{
				ItemEntry->Count -= 10;
				if (ItemEntry->Count <= 0)
					Inventory->RemoveItem(ItemEntry->ItemGuid);
				else
					Inventory->UpdateEntry(ItemEntry);
			}
		}
	}

	return Originals::ServerCreateBuildingActor(PlayerController, BuildingActorData);
}

void Building::ServerBeginEditingBuildingActor(UObject* Context, FFrame& Stack)
{
	ABuildingSMActor* Building;
	Stack.StepCompiledIn(&Building);
	Stack.IncrementCode();

	AFortPlayerControllerAthena* PlayerController = static_cast<AFortPlayerControllerAthena*>(Context);
	if (!PlayerController || !PlayerController->MyFortPawn || !Building)
		return;

	AFortPlayerStateAthena* PlayerState = static_cast<AFortPlayerStateAthena*>(PlayerController->PlayerState);
	if (!PlayerState)
		return;

	SetEditingPlayer(Building, PlayerState);

	if (!PlayerController->MyFortPawn->CurrentWeapon->IsA(AFortWeap_EditingTool::StaticClass()))
	{
		AFortInventory* Inventory = static_cast<AFortInventory*>(PlayerController->WorldInventory.ObjectPointer);
		if (Inventory)
		{
			FFortItemEntry* EditToolEntry = Inventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& Entry)
			{
				return Entry.ItemDefinition->Class == UFortEditToolItemDefinition::StaticClass();
			});

			PlayerController->MyFortPawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)EditToolEntry->ItemDefinition, EditToolEntry->ItemGuid, EditToolEntry->TrackerGuid, false);
		}
	}

	if (auto EditTool = (AFortWeap_EditingTool*)PlayerController->MyFortPawn->CurrentWeapon)
	{
		EditTool->EditActor = Building;

		EditTool->ForceNetUpdate();
		EditTool->OnRep_EditActor();
	}
}

void Building::ServerEditBuildingActor(UObject* Context, FFrame& Stack)
{
	ABuildingSMActor* Building;
	TSubclassOf<ABuildingSMActor> NewClass;
	uint8 RotationIterations;
	bool bMirrored;
	Stack.StepCompiledIn(&Building);
	Stack.StepCompiledIn(&NewClass);
	Stack.StepCompiledIn(&RotationIterations);
	Stack.StepCompiledIn(&bMirrored);
	Stack.IncrementCode();

	AFortPlayerControllerAthena* PlayerController = static_cast<AFortPlayerControllerAthena*>(Context);
	if (!PlayerController || !Building || !NewClass || !Building->IsA(ABuildingSMActor::StaticClass()) || !CanBePlacedByPlayer(NewClass) || Building->EditingPlayer != PlayerController->PlayerState || Building->bDestroyed)
		return;

	SetEditingPlayer(Building, nullptr);

	ABuildingSMActor* (*ReplaceBuildingActor)(ABuildingSMActor * BuildingSMActor, unsigned int, TSubclassOf<ABuildingSMActor>&BuildingClass, unsigned int BuildingLevel, int RotationIterations, bool bMirrored, AFortPlayerControllerAthena * PlayerController) = decltype(ReplaceBuildingActor)(Helios::Offsets::ReplaceBuildingActor);
	ABuildingSMActor* NewBuild = ReplaceBuildingActor(Building, 1, NewClass, Building->CurrentBuildingLevel, RotationIterations, bMirrored, PlayerController);

	if (NewBuild)
		NewBuild->bPlayerPlaced = true;
}

void Building::ServerEndEditingBuildingActor(UObject* Context, FFrame& Stack)
{
	ABuildingSMActor* Building;
	Stack.StepCompiledIn(&Building);
	Stack.IncrementCode();

	AFortPlayerControllerAthena* PlayerController = static_cast<AFortPlayerControllerAthena*>(Context);
	if (!PlayerController || !PlayerController->MyFortPawn || !Building || Building->EditingPlayer != PlayerController->PlayerState)
		return;

	SetEditingPlayer(Building, nullptr);

	AFortInventory* Inventory = static_cast<AFortInventory*>(PlayerController->WorldInventory.ObjectPointer);
	if (!Inventory)
		return;

	FFortItemEntry* EditToolEntry = Inventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& Entry)
	{
		return Entry.ItemDefinition->Class == UFortEditToolItemDefinition::StaticClass();
	});

	if (!EditToolEntry)
		return;

	auto EditTool = PlayerController->MyFortPawn->CurrentWeaponList.Search([&](AActor* Weapon)
	{
		return ((AFortWeapon*)Weapon)->ItemEntryGuid == EditToolEntry->ItemGuid;
	});

	if (!EditTool)
		return;

	if (auto Weap_EditingTool = *(AFortWeap_EditingTool**)EditTool)
	{
		Weap_EditingTool->EditActor = nullptr;
		Weap_EditingTool->ForceNetUpdate();
		Weap_EditingTool->OnRep_EditActor();
	}
}

void Building::Patch()
{
	Runtime::Exec("/Script/FortniteGame.FortPlayerController.ServerBeginEditingBuildingActor", ServerBeginEditingBuildingActor);
	Runtime::Exec("/Script/FortniteGame.FortPlayerController.ServerEditBuildingActor", ServerEditBuildingActor);
	Runtime::Exec("/Script/FortniteGame.FortPlayerController.ServerEndEditingBuildingActor", ServerEndEditingBuildingActor);

	Runtime::Hook<AFortPlayerControllerAthena>(EHook::Virtual, Helios::Offsets::VFTs::ServerCreateBuildingActor, ServerCreateBuildingActor, (void**)&Originals::ServerCreateBuildingActor);
	Runtime::Hook(EHook::Hook, Helios::Offsets::OnDamageServer, OnDamageServer, (void**)&Originals::OnDamageServer);
}