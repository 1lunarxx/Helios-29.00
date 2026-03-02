#include "pch.h"
#include "Inventory.h"
#include "Options.h"

UFortWorldItem* AFortInventory::AddItem(class UFortItemDefinition* ItemDefinition, int32 Count, int32 LoadedAmmo)
{
	if (!ItemDefinition || !this || !this->Owner)
		return nullptr;

	UFortWorldItem* Item = (UFortWorldItem*)ItemDefinition->CreateTemporaryItemInstanceBP(Count, 0);
	if (!Item)
		return nullptr;

	Item->SetOwningControllerForTemporaryItem((AFortPlayerController*)this->Owner);
	Item->ItemEntry.LoadedAmmo = LoadedAmmo;

	if (ItemDefinition->IsA(UFortWeaponRangedItemDefinition::StaticClass()))
	{
		if (((UFortWeaponRangedItemDefinition*)ItemDefinition)->WeaponModSlots)
		{
			Item->ItemEntry.WeaponModSlots = ((UFortWeaponRangedItemDefinition*)ItemDefinition)->WeaponModSlots;
		}
	}

	this->Inventory.ReplicatedEntries.Add(Item->ItemEntry);
	this->Inventory.ItemInstances.Add(Item);

	if (&Item->ItemEntry)
		this->Inventory.MarkItemDirty(Item->ItemEntry);
	else
		this->Inventory.MarkArrayDirty();

	this->bRequiresLocalUpdate = true;
	this->HandleInventoryLocalUpdate();

	return Item;
}

void AFortInventory::RemoveItem(FGuid& ItemGuid)
{
	if (!this)
		return;

	auto ItemEntry = this->Inventory.ReplicatedEntries.SearchIndex([&](FFortItemEntry& entry) { return entry.ItemGuid == ItemGuid; });
	auto WorldItem = this->Inventory.ItemInstances.SearchIndex([&](UFortWorldItem* entry) { return entry->ItemEntry.ItemGuid == ItemGuid; });

	if (ItemEntry != -1)
		this->Inventory.ReplicatedEntries.Remove(ItemEntry);

	if (WorldItem != -1)
		this->Inventory.ItemInstances.Remove(WorldItem);

	this->bRequiresLocalUpdate = true;
	this->bRequiresSaving = true;

	this->HandleInventoryLocalUpdate();
	this->Inventory.MarkArrayDirty();

	this->ForceNetUpdate();
}

void AFortInventory::UpdateEntry(FFortItemEntry* ItemEntry)
{
	if (!this)
		return;

	if (ItemEntry->bIsReplicatedCopy)
	{
		ItemEntry->bIsDirty = false;

		this->Inventory.MarkItemDirty(*ItemEntry);
		this->bRequiresLocalUpdate = true;

		return;
	}

	if (ItemEntry->ItemGuid.A == 0 && ItemEntry->ItemGuid.B == 0 && ItemEntry->ItemGuid.C == 0 && ItemEntry->ItemGuid.D == 0)
		return;

	for (auto& Entry : this->Inventory.ReplicatedEntries)
	{
		if (Entry.ItemGuid == ItemEntry->ItemGuid)
		{
			Entry = *ItemEntry;
			Entry.bIsDirty = false;

			this->Inventory.MarkItemDirty(*ItemEntry);
			this->bRequiresLocalUpdate = true;

			this->HandleInventoryLocalUpdate();

			if (&ItemEntry) {
				this->Inventory.MarkItemDirty(*ItemEntry);
			}
			else {
				this->Inventory.MarkArrayDirty();
			}

			break;
		}
	}
}

struct FFortItemEntry* AFortInventory::MakeItemEntry(class UFortItemDefinition* ItemDefinition, int32 Count, int32 Level)
{
	if (!ItemDefinition)
		return nullptr;

	FFortItemEntry ItemEntry{};
	ItemEntry.MostRecentArrayReplicationKey = -1;
	ItemEntry.ReplicationID = -1;
	ItemEntry.ReplicationKey = -1;
	ItemEntry.ItemDefinition = (UItemDefinitionBase*)ItemDefinition;
	ItemEntry.Count = Count;
	ItemEntry.Durability = 1.f;
	ItemEntry.GameplayAbilitySpecHandle = FGameplayAbilitySpecHandle(-1);
	ItemEntry.ParentInventory.ObjectIndex = -1;
	ItemEntry.Level = Level;

	if (auto Weapon = ItemDefinition->IsA(UFortGadgetItemDefinition::StaticClass()) ? (UFortWeaponItemDefinition*)((UFortGadgetItemDefinition*)ItemDefinition)->GetWeaponItemDefinition() : (UFortWeaponItemDefinition*)ItemDefinition)
	{
		auto Stats = this->GetStats(Weapon);

		if (Stats)
		{
			ItemEntry.LoadedAmmo = Stats->ClipSize;
			ItemEntry.PhantomReserveAmmo = (Stats->InitialClips - 1) * Stats->ClipSize;
		}
	}

	if (ItemDefinition->IsA(UFortWeaponItemDefinition::StaticClass()))
	{
		UFortWeaponItemDefinition* WeaponDefinition = (UFortWeaponItemDefinition*)ItemDefinition;

		if (WeaponDefinition->WeaponModSlots)
		{
			ItemEntry.WeaponModSlots = WeaponDefinition->WeaponModSlots;
		}
	}

	ItemEntry.PickupVariantIndex = -1;
	ItemEntry.ItemVariantDataMappingIndex = -1;
	ItemEntry.OrderIndex = -1;

	return &ItemEntry;
}

struct FFortRangedWeaponStats* AFortInventory::GetStats(class UFortWeaponItemDefinition* ItemDefinition)
{
	if (!ItemDefinition || !ItemDefinition->WeaponStatHandle.DataTable)
		return nullptr;

	auto WeaponStatHandle = ItemDefinition->WeaponStatHandle.DataTable->RowMap.Search([ItemDefinition](FName& Key, uint8_t* Value) {
		return ItemDefinition->WeaponStatHandle.RowName == Key && Value;
	});

	return WeaponStatHandle ? *(FFortRangedWeaponStats**)WeaponStatHandle : nullptr;
}

enum EFortQuickBars AFortInventory::GetQuickbar(class UFortItemDefinition* ItemDefinition)
{
	if (!ItemDefinition)
		return EFortQuickBars::Max_None;

	return ItemDefinition->IsA(UFortWeaponMeleeItemDefinition::StaticClass()) || ItemDefinition->IsA(UFortResourceItemDefinition::StaticClass()) || ItemDefinition->IsA(UFortAmmoItemDefinition::StaticClass()) || ItemDefinition->IsA(UFortTrapItemDefinition::StaticClass()) || ItemDefinition->IsA(UFortBuildingItemDefinition::StaticClass()) || ItemDefinition->IsA(UFortEditToolItemDefinition::StaticClass()) || ((UFortWorldItemDefinition*)ItemDefinition)->bForceIntoOverflow ? EFortQuickBars::Secondary : EFortQuickBars::Primary;
}

AFortPickupAthena* Inventory::SpawnPickup(FVector Location, FFortItemEntry* ItemEntry, int Count, AFortPlayerPawnAthena* Pawn, EFortPickupSourceTypeFlag SourceTypeFlag, EFortPickupSpawnSource SpawnSource, bool bToss, bool bRandomRotation, FVector FinalLoc)
{
	AFortPickupAthena* NewPickup = Runtime::SpawnActor<AFortPickupAthena>(Location);
	if (!NewPickup)
		return nullptr;

	if (!ItemEntry)
		return nullptr;

	NewPickup->bRandomRotation = bRandomRotation;
	if (ItemEntry->Level != -1)
		NewPickup->PrimaryPickupItemEntry.Level = ItemEntry->Level;

	NewPickup->PrimaryPickupItemEntry.ItemDefinition = ItemEntry->ItemDefinition;
	NewPickup->PrimaryPickupItemEntry.LoadedAmmo = ItemEntry->LoadedAmmo;
	NewPickup->PrimaryPickupItemEntry.Count = Count != -1 ? Count : ItemEntry->Count;
	NewPickup->PrimaryPickupItemEntry.PhantomReserveAmmo = ItemEntry->PhantomReserveAmmo;
	NewPickup->OnRep_PrimaryPickupItemEntry();

	NewPickup->PawnWhoDroppedPickup = Pawn ? Pawn : nullptr;
	NewPickup->TossPickup((FinalLoc.X || FinalLoc.Y || FinalLoc.Z) ? FinalLoc : Location, Pawn ? Pawn : nullptr, -1, bToss, true, SourceTypeFlag, SpawnSource);

	if (SpawnSource != EFortPickupSpawnSource::Unset)
	{
		NewPickup->bTossedFromContainer = SpawnSource == EFortPickupSpawnSource::Chest || SpawnSource == EFortPickupSpawnSource::AmmoBox;
		NewPickup->OnRep_TossedFromContainer();
	}

	return NewPickup;
}

AFortPickupAthena* Inventory::SpawnPickup(FVector Location, UFortItemDefinition* ItemDefinition, int Count, AFortPlayerPawnAthena* Pawn, EFortPickupSourceTypeFlag SourceTypeFlag, EFortPickupSpawnSource SpawnSource, bool bToss, bool bRandomRotation, FVector FinalLocation)
{
	FFortItemEntry* ItemEntry = AFortInventory::GetDefaultObj()->MakeItemEntry(ItemDefinition, Count, -1);
	AFortPickupAthena* Pickup = SpawnPickup(Location, ItemEntry, Count, Pawn, SourceTypeFlag, SpawnSource, bToss, bRandomRotation);

	return Pickup;
}

void Inventory::SetLoadedAmmo(UFortWorldItem* WorldItem, int LoadedAmmo)
{
	AFortPlayerControllerAthena* PlayerController = static_cast<AFortPlayerControllerAthena*>(WorldItem->GetOwningController());
	if (!PlayerController)
		return;

	AFortInventory* Inventory = static_cast<AFortInventory*>(PlayerController->WorldInventory.ObjectPointer);
	if (!Inventory)
		return;

	FFortItemEntry* Entry = nullptr;

	for (auto& ReplicatedEntry : Inventory->Inventory.ReplicatedEntries)
	{
		if (ReplicatedEntry.ItemGuid == WorldItem->ItemEntry.ItemGuid)
		{
			Entry = &ReplicatedEntry;
			break;
		}
	}

	if (!Entry)
		return;

	if (LoadedAmmo == 0 && Entry->ItemDefinition->GetFullName().contains("TeamSpray"))
		Inventory->RemoveItem(Entry->ItemGuid);

	Entry->LoadedAmmo = LoadedAmmo;
	WorldItem->ItemEntry.LoadedAmmo = LoadedAmmo;

	Inventory->UpdateEntry(Entry);
	WorldItem->ItemEntry.bIsDirty = true;
}

bool Inventory::RemoveInventoryItem(IInterface* Interface, FGuid& ItemGuid, int Count, bool bForceRemoval)
{
	AFortPlayerControllerAthena* PlayerController = (AFortPlayerControllerAthena*)(__int64(Interface) - 2240);
	if (!PlayerController)
		return false;

	AFortInventory* Inventory = static_cast<AFortInventory*>(PlayerController->WorldInventory.ObjectPointer);
	if (!Inventory)
		return false;

	FFortItemEntry* Entry = nullptr;
	for (auto& ReplicatedEntry : Inventory->Inventory.ReplicatedEntries)
	{
		if (ReplicatedEntry.ItemGuid == ItemGuid)
		{
			Entry = &ReplicatedEntry;
			break;
		}
	}

	if (!Entry)
		return false;

	UFortWorldItem* Item = nullptr;
	for (auto& ItemInstance : Inventory->Inventory.ItemInstances)
	{
		if (ItemInstance->ItemEntry.ItemGuid == ItemGuid)
		{
			Item = ItemInstance;
			break;
		}
	}

	if (Item)
	{
		Entry->Count -= max(Count, 0);

		if (Count < 0 || Entry->Count <= 0 || bForceRemoval)
		{
			Inventory->RemoveItem(Entry->ItemGuid);
		}
		else
		{
			Item->ItemEntry.Count = Entry->Count;
			Inventory->UpdateEntry(Entry);

			Item->ItemEntry.bIsDirty = true;
		}

		return true;
	}

	return false;
}

AFortPickup* Inventory::K2_SpawnPickupInWorld(UObject* Object, FFrame& Stack, AFortPickup** Ret)
{
	class UObject* WorldContextObject;
	class UFortWorldItemDefinition* ItemDefinition;
	int32 NumberToSpawn;
	FVector Position;
	FVector Direction;
	int32 OverrideMaxStackCount;
	bool bToss;
	bool bRandomRotation;
	bool bBlockedFromAutoPickup;
	int32 PickupInstigatorHandle;
	EFortPickupSourceTypeFlag SourceType;
	EFortPickupSpawnSource Source;
	class AFortPlayerController* OptionalOwnerPC;
	bool bPickupOnlyRelevantToOwner;
	Stack.StepCompiledIn(&WorldContextObject);
	Stack.StepCompiledIn(&ItemDefinition);
	Stack.StepCompiledIn(&NumberToSpawn);
	Stack.StepCompiledIn(&Position);
	Stack.StepCompiledIn(&Direction);
	Stack.StepCompiledIn(&OverrideMaxStackCount);
	Stack.StepCompiledIn(&bToss);
	Stack.StepCompiledIn(&bRandomRotation);
	Stack.StepCompiledIn(&bBlockedFromAutoPickup);
	Stack.StepCompiledIn(&PickupInstigatorHandle);
	Stack.StepCompiledIn(&SourceType);
	Stack.StepCompiledIn(&Source);
	Stack.StepCompiledIn(&OptionalOwnerPC);
	Stack.StepCompiledIn(&bPickupOnlyRelevantToOwner);
	Stack.IncrementCode();

	return *Ret = Inventory::SpawnPickup(Position, ItemDefinition, NumberToSpawn, nullptr, SourceType, Source, bToss, bRandomRotation);
}

void Inventory::Patch()
{
	Runtime::Hook<UFortWorldItem>(EHook::Virtual, Helios::Offsets::VFTs::SetLoadedAmmo, SetLoadedAmmo);
	Runtime::Hook(EHook::Hook, Helios::Offsets::RemoveInventoryItem, RemoveInventoryItem);

	Runtime::Exec("/Script/FortniteGame.FortKismetLibrary.K2_SpawnPickupInWorld", K2_SpawnPickupInWorld);
}