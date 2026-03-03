#include "pch.h"
#include "Player.h"
#include "Options.h"
#include "Inventory.h"
#include "Lategame.h"
#include "Abilities.h"
#include "Helios.h"
#include "XP.h"

void Player::ServerAcknowledgePossession(AFortPlayerControllerAthena* PlayerController, APawn* Pawn)
{
	PlayerController->AcknowledgedPawn = Pawn;

	AFortPlayerStateAthena* PlayerState = static_cast<AFortPlayerStateAthena*>(PlayerController->PlayerState);
	if (PlayerState)
	{
		UFortKismetLibrary::UpdatePlayerCustomCharacterPartsVisualization(PlayerState);
		PlayerState->OnRep_CharacterData();

		TScriptInterface<IAbilitySystemInterface> AbilityInterface{};
		AbilityInterface.ObjectPointer = PlayerState;
		AbilityInterface.InterfacePointer = Runtime::GetInterface<IAbilitySystemInterface>(PlayerState);

		static void (*InitializePlayerGameplayAbilities)(void*) = decltype(InitializePlayerGameplayAbilities)(Helios::Offsets::InitializePlayerGameplayAbilities);
		InitializePlayerGameplayAbilities(AbilityInterface.InterfacePointer);
	}

	PlayerController->MyFortPawn->bIsInAnyStorm = false;
	PlayerController->MyFortPawn->OnRep_IsInAnyStorm();

	PlayerController->MyFortPawn->bIsInsideSafeZone = true;
	PlayerController->MyFortPawn->OnRep_IsInsideSafeZone();

	Abilities::UpdateActiveGameplayEffectSetByCallerMagnitude(PlayerController, Runtime::StaticFindObject<UClass>("/Game/Athena/SafeZone/GE_OutsideSafeZoneDamage.GE_OutsideSafeZoneDamage_C"), FGameplayTag(UKismetStringLibrary::Conv_StringToName(L"SetByCaller.StormCampingDamage")));
}

void Player::GetPlayerViewPoint(AFortPlayerControllerAthena* PlayerController, FVector& Location, FRotator& Rotation)
{
	if (PlayerController->StateName == UKismetStringLibrary::Conv_StringToName(L"Spectating"))
	{
		Location = PlayerController->LastSpectatorSyncLocation;
		Rotation = PlayerController->LastSpectatorSyncRotation;
	}
	else if (PlayerController->GetViewTarget())
	{
		Location = PlayerController->GetViewTarget()->K2_GetActorLocation();
		Rotation = PlayerController->GetControlRotation();
	}
	else
	{
		return Originals::GetPlayerViewPoint(PlayerController, Location, Rotation);
	}
}

void Player::ServerExecuteInventoryItem(AFortPlayerControllerAthena* PlayerController, FGuid& ItemGuid)
{
	UFortWorldItem* Item = static_cast<UFortWorldItem*>(PlayerController->BP_GetInventoryItemWithGuid(ItemGuid));
	if (!Item)
		return;

	if (PlayerController->MyFortPawn)
	{
		PlayerController->MyFortPawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)Item->ItemEntry.ItemDefinition, ItemGuid, Item->ItemEntry.TrackerGuid, false);
	}
}

void Player::ServerAttemptAircraftJump(UFortControllerComponent_Aircraft* Component_Aircraft, FRotator& Rotation)
{
	UFortGameStateComponent_BattleRoyaleGamePhaseLogic* GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(UWorld::GetWorld());
	AFortGameModeAthena* GameMode = static_cast<AFortGameModeAthena*>(UWorld::GetWorld()->AuthorityGameMode);

	AFortPlayerController* PlayerController = static_cast<AFortPlayerController*>(Component_Aircraft->GetOwner());
	if (!PlayerController)
		return;

	GameMode->RestartPlayer(PlayerController);

	if (UHeliosConfiguration::bIsLateGame && PlayerController->MyFortPawn && GamePhaseLogic->Aircrafts_GameState.IsValid()) {
		FVector AircraftLocation = GamePhaseLogic->Aircrafts_GameState[0]->K2_GetActorLocation();

		float Angle = (float)rand() / 5215.03002625f;
		float Radius = (float)(rand() % 1000);

		float OffsetX = cosf(Angle) * Radius;
		float OffsetY = sinf(Angle) * Radius;

		FVector Offset;
		Offset.X = OffsetX;
		Offset.Y = OffsetY;
		Offset.Z = 0.0f;

		FVector Location = AircraftLocation + Offset;
		PlayerController->MyFortPawn->K2_SetActorLocation(Location, false, nullptr, true);
	}

	if (PlayerController->MyFortPawn)
	{
		PlayerController->MyFortPawn->BeginSkydiving(true);
		PlayerController->MyFortPawn->SetHealth(100);

		if (UHeliosConfiguration::bIsLateGame)
		{
			PlayerController->MyFortPawn->SetShield(100);
			Lategame::GivePlayerRandomWeapon((AFortPlayerControllerAthena*)PlayerController);
		}
	}
}

void Player::ServerAttemptInventoryDrop(AFortPlayerControllerAthena* PlayerController, FGuid Guid, int32 Count, bool bTrash)
{
	if (!PlayerController->Pawn || !PlayerController || !PlayerController->NetConnection)
		return;

	AFortInventory* Inventory = static_cast<AFortInventory*>(PlayerController->WorldInventory.ObjectPointer);
	if (!Inventory)
		return;

	FFortItemEntry* ItemEntry = Inventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& Entry) { return Entry.ItemGuid == Guid; });
	if (!ItemEntry || (ItemEntry->Count - Count) < 0)
		return;

	UFortWorldItem* WorldItem = *Inventory->Inventory.ItemInstances.Search([&](UFortWorldItem* Entry) { return Entry->ItemEntry.ItemGuid == Guid; });
	if (!WorldItem)
		return;

	ItemEntry->Count -= Count;

	float RandomDegrees = UKismetMathLibrary::RandomFloatInRange(-18.f, 18.f) + (360.f / UKismetMathLibrary::RandomFloatInRange(0.0001f, 1.0f));
	float Float = UKismetMathLibrary::DegreesToRadians(RandomDegrees);

	FVector Location = PlayerController->Pawn->K2_GetActorLocation();
	Location = Location + PlayerController->Pawn->GetActorForwardVector() * 450.f;

	Location.Z += 50.f;
	Location.X += std::cos(Float) * 100.f;
	Location.Y += std::sin(Float) * 100.f;

	Inventory::SpawnPickup(PlayerController->Pawn->K2_GetActorLocation() + PlayerController->Pawn->GetActorForwardVector() * 70.f + FVector(0, 0, 50), ItemEntry, Count, (AFortPlayerPawnAthena*)PlayerController->Pawn, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, true, true, Location);

	if (ItemEntry->Count <= 0 || Count < 0)
		Inventory->RemoveItem(Guid);
	else
	{
		WorldItem->ItemEntry.Count = ItemEntry->Count;
		Inventory->UpdateEntry(ItemEntry);

		WorldItem->ItemEntry.bIsDirty = true;
	}
}

void Player::ClientOnPawnDied(AFortPlayerControllerAthena* PlayerController, FFortPlayerDeathReport& DeathReport)
{
	AFortGameStateAthena* GameState = static_cast<AFortGameStateAthena*>(UWorld::GetWorld()->GameState);
	AFortGameModeAthena* GameMode = static_cast<AFortGameModeAthena*>(UWorld::GetWorld()->AuthorityGameMode);

	AFortPlayerStateAthena* PlayerState = static_cast<AFortPlayerStateAthena*>(PlayerController->PlayerState);
	AFortPlayerStateAthena* KillerState = static_cast<AFortPlayerStateAthena*>(DeathReport.KillerPlayerState);
	AFortPlayerPawnAthena* KillerPawn = static_cast<AFortPlayerPawnAthena*>(DeathReport.KillerPawn.Get());

	if (!GameState->IsRespawningAllowed(PlayerState))
	{
		if (PlayerController->WorldInventory.ObjectPointer)
		{
			for (auto& Entry : (static_cast<AFortInventory*>(PlayerController->WorldInventory.ObjectPointer))->Inventory.ReplicatedEntries)
			{
				if (Entry.ItemDefinition && !Entry.ItemDefinition->IsA(UFortWeaponMeleeItemDefinition::StaticClass()) && !Entry.ItemDefinition->IsA(UFortBuildingItemDefinition::StaticClass()))
				{
					Inventory::SpawnPickup(PlayerController->Pawn->K2_GetActorLocation() + PlayerController->Pawn->GetActorForwardVector() * 70.f + FVector(0, 0, 50), &Entry, Entry.Count, (AFortPlayerPawnAthena*)PlayerController->Pawn, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, true, true);
				}
			}
		}
	}

	PlayerState->PawnDeathLocation = PlayerController->Pawn->K2_GetActorLocation();
	PlayerState->DeathInfo.bDBNO = PlayerController->MyFortPawn ? PlayerController->MyFortPawn->IsDBNO() : false;
	PlayerState->DeathInfo.bInitialized = true;
	PlayerState->DeathInfo.DeathLocation = PlayerController->Pawn->K2_GetActorLocation();
	PlayerState->DeathInfo.DeathTags = DeathReport.Tags;
	PlayerState->DeathInfo.FinisherOrDowner = KillerState ? KillerState : PlayerState;
	PlayerState->DeathInfo.VictimTags = ((AFortPlayerPawnAthena*)PlayerController->Pawn)->GameplayTags;
	PlayerState->DeathInfo.DeathCause = PlayerState->ToDeathCause(DeathReport.Tags, PlayerController->MyFortPawn ? PlayerController->MyFortPawn->IsDBNO() : false);
	PlayerState->DeathInfo.DeathClassSlot = -1;
	PlayerState->OnRep_DeathInfo();

	if (KillerState && KillerPawn && KillerPawn->Controller && KillerPawn->Controller != PlayerController)
	{
		KillerState->KillScore++;
		KillerState->TeamKillScore++;

		KillerState->OnRep_TeamKillScore();
		KillerState->OnRep_Kills();

		KillerState->ClientReportKill(PlayerState);
		KillerState->ClientReportTeamKill(KillerState->TeamKillScore);

		XP::SendStatEvent((AFortPlayerControllerAthena*)KillerState->GetOwner(), EFortQuestObjectiveStatEvent::Kill);
	}

	if (!GameState->IsRespawningAllowed(PlayerState))
	{
		FAthenaRewardResult Result{};
		PlayerController->ClientSendEndBattleRoyaleMatchForPlayer(true, Result);

		FAthenaMatchStats Stats{};
		FAthenaMatchTeamStats TeamStats{};

		if (PlayerState)
		{
			PlayerState->Place = GameState->PlayersLeft;
			PlayerState->OnRep_Place();
		}

		TeamStats.Place = PlayerState->Place;
		TeamStats.TotalPlayers = GameState->TotalPlayers;

		PlayerController->ClientSendMatchStatsForPlayer(Stats);
		PlayerController->ClientSendTeamStatsForPlayer(TeamStats);

		Helios::EndBattleRoyaleGameV2(PlayerController, PlayerController->XPComponent->TotalXpEarned, PlayerState->TeamKillScore, PlayerState->Place);
		((AFortPlayerPawnAthena*)PlayerController->Pawn)->CharacterMovement->DisableMovement();

		static void(*RemoveFromAlivePlayers)(AFortGameModeAthena*, AFortPlayerControllerAthena*, APlayerState*, AFortPlayerPawn*, UFortWeaponItemDefinition*, EDeathCause, char) = decltype(RemoveFromAlivePlayers)(Helios::Offsets::RemoveFromAlivePlayers);
		RemoveFromAlivePlayers(GameMode, PlayerController, (KillerState == PlayerState ? nullptr : KillerState), (AFortPlayerPawn*)DeathReport.KillerPawn.Get(), nullptr, PlayerState ? PlayerState->DeathInfo.DeathCause : EDeathCause::Rifle, 0);

		if (UHeliosConfiguration::PlaylistID.contains("Showdown"))
		{
			for (auto& Player : GameMode->AlivePlayers)
			{
				if (!Player)
					continue;

				AFortPlayerControllerAthena* PlayerController = (AFortPlayerControllerAthena*)Player;
				PlayerController->ClientReportTournamentPlacementPointsScored(GameState->PlayersLeft, GameState->PlayersLeft == 1 ? 25 : 2);
			}
		}

		if (KillerState)
		{
			if (KillerState->Place == 1)
			{
				AFortPlayerControllerAthena* KillerController = (AFortPlayerControllerAthena*)KillerState->GetOwner();
				if (!KillerController)
					return Originals::ClientOnPawnDied(PlayerController, DeathReport);

				FAthenaRewardResult Result{};
				KillerController->ClientSendEndBattleRoyaleMatchForPlayer(true, Result);

				KillerController->PlayWinEffects(DeathReport.KillerPawn.Get(), nullptr, PlayerState->DeathInfo.DeathCause, false, ERoundVictoryAnimation::Default);
				KillerController->ClientNotifyWon(DeathReport.KillerPawn.Get(), nullptr, PlayerState->DeathInfo.DeathCause);
				KillerController->ClientNotifyTeamWon(DeathReport.KillerPawn.Get(), nullptr, PlayerState->DeathInfo.DeathCause);

				FAthenaMatchStats Stats{};
				FAthenaMatchTeamStats TeamStats{};

				if (KillerState)
				{
					KillerState->Place = GameState->PlayersLeft;
					KillerState->OnRep_Place();
				}

				TeamStats.Place = 1;
				TeamStats.TotalPlayers = GameState->TotalPlayers;

				KillerController->ClientSendMatchStatsForPlayer(Stats);
				KillerController->ClientSendTeamStatsForPlayer(TeamStats);

				Helios::EndBattleRoyaleGameV2(((AFortPlayerControllerAthena*)KillerState->GetOwner()), ((AFortPlayerControllerAthena*)KillerState->GetOwner())->XPComponent->TotalXpEarned, KillerState->TeamKillScore, KillerState->Place);

				GameState->WinningPlayerState = KillerState;
				GameState->WinningTeam = KillerState->TeamIndex;

				GameState->OnRep_WinningPlayerState();
				GameState->OnRep_WinningTeam();
			}
		}
	}

	return Originals::ClientOnPawnDied(PlayerController, DeathReport);
}

void Player::ServerHandlePickupInfo(UObject* Context, FFrame& Stack)
{
	AFortPickup* Pickup;
	FFortPickupRequestInfo Params;

	Stack.StepCompiledIn(&Pickup);
	Stack.StepCompiledIn(&Params);
	Stack.IncrementCode();

	AFortPlayerPawn* Pawn = static_cast<AFortPlayerPawn*>(Context);
	if (!Pawn || !Pickup || Pickup->bPickedUp)
		return;

	auto PlayerController = static_cast<AFortPlayerControllerAthena*>(Pawn->Controller);
	if (!PlayerController)
		return;

	AFortInventory* Inventory = static_cast<AFortInventory*>(PlayerController->WorldInventory.ObjectPointer);

	if (Params.bUseRequestedSwap && Pawn->CurrentWeapon && Inventory->GetQuickbar(Pawn->CurrentWeapon->WeaponData) == EFortQuickBars::Primary && Inventory->GetQuickbar((UFortItemDefinition*)Pickup->PrimaryPickupItemEntry.ItemDefinition) == EFortQuickBars::Primary)
	{
		AFortPlayerControllerAthena* PlayerController = static_cast<AFortPlayerControllerAthena*>(Pawn->Controller);
		if (PlayerController)
			PlayerController->bTryPickupSwap = true;
	}

	Pickup->PickupLocationData.bPlayPickupSound = Params.bPlayPickupSound;
	Pickup->PickupLocationData.FlyTime = 0.4f;
	Pickup->PickupLocationData.ItemOwner = Pawn;
	Pickup->PickupLocationData.PickupGuid = Pickup->PrimaryPickupItemEntry.ItemGuid;
	Pickup->PickupLocationData.PickupTarget = Pawn;
	Pickup->OnRep_PickupLocationData();

	Pickup->bPickedUp = true;
	Pickup->OnRep_bPickedUp();

	Pawn->IncomingPickups.Add(Pickup);
}

void Player::ServerNotifyPawnHit(UObject* Context, FFrame& Stack)
{
	FHitResult Hit;
	FVector ProjectileOriginPosition;
	float ProjectileStartTimestamp;
	TArray<uint8> ArrayContext;
	FVector LocalSpaceImpactPoint;
	FVector LocalSpaceImpactNormal;
	bool bWasTargetingWhenProjectileFired;

	Stack.StepCompiledIn(&Hit);
	Stack.StepCompiledIn(&ProjectileOriginPosition);
	Stack.StepCompiledIn(&ProjectileStartTimestamp);
	Stack.StepCompiledIn(&ArrayContext);
	Stack.StepCompiledIn(&LocalSpaceImpactPoint);
	Stack.StepCompiledIn(&LocalSpaceImpactNormal);
	Stack.StepCompiledIn(&bWasTargetingWhenProjectileFired);
	Stack.IncrementCode();

	AFortWeaponRanged* Weapon = (AFortWeaponRanged*)Context;
	if (!Weapon)
		return defOG(Weapon, "/Script/FortniteGame.FortWeaponRanged", ServerNotifyPawnHit, Hit, ProjectileOriginPosition, ProjectileStartTimestamp, ArrayContext, LocalSpaceImpactPoint, LocalSpaceImpactNormal, bWasTargetingWhenProjectileFired);

	AFortPlayerControllerAthena* PlayerController = (AFortPlayerControllerAthena*)Weapon->GetPlayerController();
	if (!PlayerController)
		return defOG(Weapon, "/Script/FortniteGame.FortWeaponRanged", ServerNotifyPawnHit, Hit, ProjectileOriginPosition, ProjectileStartTimestamp, ArrayContext, LocalSpaceImpactPoint, LocalSpaceImpactNormal, bWasTargetingWhenProjectileFired);

	auto Component = Hit.Component.Get();
	if (!Component)
		return defOG(Weapon, "/Script/FortniteGame.FortWeaponRanged", ServerNotifyPawnHit, Hit, ProjectileOriginPosition, ProjectileStartTimestamp, ArrayContext, LocalSpaceImpactPoint, LocalSpaceImpactNormal, bWasTargetingWhenProjectileFired);

	auto Actor = Component->GetOwner();
	if (!Actor)
		return defOG(Weapon, "/Script/FortniteGame.FortWeaponRanged", ServerNotifyPawnHit, Hit, ProjectileOriginPosition, ProjectileStartTimestamp, ArrayContext, LocalSpaceImpactPoint, LocalSpaceImpactNormal, bWasTargetingWhenProjectileFired);

	auto GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get((AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode);
	if (!GamePhaseLogic)
		return defOG(Weapon, "/Script/FortniteGame.FortWeaponRanged", ServerNotifyPawnHit, Hit, ProjectileOriginPosition, ProjectileStartTimestamp, ArrayContext, LocalSpaceImpactPoint, LocalSpaceImpactNormal, bWasTargetingWhenProjectileFired);

	if (Weapon->GetWeaponDataWeaponStatHandle().DataTable)
	{
		auto StatHandle = Weapon->GetWeaponDataWeaponStatHandle();
		if (!StatHandle.DataTable)
			return defOG(Weapon, "/Script/FortniteGame.FortWeaponRanged", ServerNotifyPawnHit, Hit, ProjectileOriginPosition, ProjectileStartTimestamp, ArrayContext, LocalSpaceImpactPoint, LocalSpaceImpactNormal, bWasTargetingWhenProjectileFired);

		auto Equals = [](const FName& LeftKey, const FName& RightKey) -> bool
		{
			return LeftKey == RightKey;
		};

		auto Index = StatHandle.DataTable->RowMap.Find(StatHandle.RowName, Equals).GetIndex();
		auto WeaponStats = (FFortRangedWeaponStats*)StatHandle.DataTable->RowMap[Index].Second;

		if (WeaponStats)
		{
			if (Actor->bCanBeDamaged == 1 && PlayerController->MyFortPawn)
			{
				if (Actor->IsA(AFortPlayerPawn::StaticClass()))
				{
					float Multiplier = 1;
					if (Hit.BoneName.ToString() == "Head")
						Multiplier = WeaponStats->DamageZone_Critical;

					float Damage = WeaponStats->DmgPB * Multiplier;

					FAthenaBatchedDamageGameplayCues_Shared SharedCue{};
					SharedCue.Location = (FVector_NetQuantize10)Hit.ImpactPoint;
					SharedCue.Normal = Hit.ImpactNormal;
					SharedCue.bIsCritical = Hit.BoneName.ToString() == "Head";
					SharedCue.Magnitude = Damage;
					SharedCue.bWeaponActivate = true;
					SharedCue.bIsFatal = false;
					SharedCue.bIsShield = false;
					SharedCue.bIsShieldDestroyed = false;
					SharedCue.bIsShieldApplied = false;
					SharedCue.bIsBallistic = false;
					SharedCue.bIsBeam = false;
					SharedCue.bIsValid = true;

					FAthenaBatchedDamageGameplayCues_NonShared NonSharedCue{};
					NonSharedCue.HitActor = Actor;
					NonSharedCue.NonPlayerHitActor = Actor;

					AFortPlayerPawn* Pawn = (AFortPlayerPawn*)Actor;
					if (Pawn->Controller && ((AFortGameStateAthena*)UWorld::GetWorld()->GameState)->CurrentPlaylistInfo.BasePlaylist->MaxSquadSize > 1)
					{
						AFortPlayerStateAthena* EnemyPlayerState = (AFortPlayerStateAthena*)Pawn->Controller->PlayerState;
						AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PlayerController->PlayerState;

						if (EnemyPlayerState && PlayerState)
						{
							if (EnemyPlayerState->TeamIndex == PlayerState->TeamIndex)
							{
								return defOG(Weapon, "/Script/FortniteGame.FortWeaponRanged", ServerNotifyPawnHit, Hit, ProjectileOriginPosition, ProjectileStartTimestamp, ArrayContext, LocalSpaceImpactPoint, LocalSpaceImpactNormal, bWasTargetingWhenProjectileFired);
							}
						}
					}

					if (GamePhaseLogic->GamePhase != EAthenaGamePhase::Warmup && Pawn) {
						auto PlayerShield = Pawn->GetShield();
						auto PlayerHealth = Pawn->GetHealth();
						float RemainingDamage = SharedCue.Magnitude;

						if (PlayerShield > 0.f)
						{
							SharedCue.bIsShield = true;

							if (PlayerShield <= RemainingDamage)
							{
								SharedCue.bIsShieldDestroyed = true;

								RemainingDamage -= PlayerShield;
								Pawn->SetShield(0.f);
							}
							else
							{
								Pawn->SetShield(PlayerShield - RemainingDamage);
								RemainingDamage = 0.f;
							}
						}

						if (RemainingDamage > 0.f)
						{
							if (PlayerHealth <= RemainingDamage)
							{
								Pawn->ForceKill(FGameplayTag(), PlayerController, Weapon);
								return;
							}
							else
							{
								Pawn->SetHealth(PlayerHealth - RemainingDamage);
							}
						}

						Pawn->ForceNetUpdate();
					}

					PlayerController->MyFortPawn->NetMulticast_Athena_BatchedDamageCues(SharedCue, NonSharedCue);
				}
				else
				{
					FAthenaBatchedDamageGameplayCues_Shared SharedCue{};

					SharedCue.Location = (FVector_NetQuantize10)Hit.ImpactPoint;
					SharedCue.Normal = Hit.ImpactNormal;
					SharedCue.Magnitude = WeaponStats->EnvDmgPB;
					SharedCue.bWeaponActivate = true;
					SharedCue.bIsFatal = false;
					SharedCue.bIsCritical = false;
					SharedCue.bIsBallistic = true;
					SharedCue.bIsBeam = false;
					SharedCue.bIsValid = true;

					FAthenaBatchedDamageGameplayCues_NonShared NonSharedCue{};
					NonSharedCue.HitActor = Actor;
					NonSharedCue.NonPlayerHitActor = Actor;

					PlayerController->MyFortPawn->NetMulticast_Athena_BatchedDamageCues(SharedCue, NonSharedCue);
					if (Actor->IsA(ABuildingSMActor::StaticClass()))
					{
						ABuildingSMActor* BuildingActor = (ABuildingSMActor*)Actor;
						if (BuildingActor)
						{
							float RemainingHealth = BuildingActor->GetHealth() - SharedCue.Magnitude;
							BuildingActor->SetHealth(RemainingHealth);
							BuildingActor->ForceNetUpdate();

							if (BuildingActor->GetHealth() <= 0)
								BuildingActor->K2_DestroyActor();
						}
					}
					else if (Actor->IsA(AAthenaSuperDingo::StaticClass()))
					{
						AAthenaSuperDingo* SuperDingo = (AAthenaSuperDingo*)Actor;
						if (SuperDingo)
						{
							float RemainingHealth = SuperDingo->GetHealth() - SharedCue.Magnitude;
							SuperDingo->SetHealth(RemainingHealth);
							SuperDingo->ForceNetUpdate();

							if (SuperDingo->GetHealth() <= 0)
								SuperDingo->K2_DestroyActor();
						}
					}
					else if (Actor->IsA(AFortAthenaVehicle::StaticClass()))
					{
						AFortAthenaVehicle* Vehicle = (AFortAthenaVehicle*)Actor;
						if (Vehicle)
						{
							Vehicle->HealthSet->Health.CurrentValue -= WeaponStats->EnvDmgPB;
							Vehicle->OnRep_HealthSet();

							if (Vehicle->HealthSet->Health.CurrentValue <= 0)
								Vehicle->DestroyVehicle();
						}
					}
					else 	if (Actor->IsA(ABuildingGameplayActorCrashpad::StaticClass()))
					{
						ABuildingGameplayActorCrashpad* CrashPad = (ABuildingGameplayActorCrashpad*)Actor;
						if (CrashPad)
						{
							float RemainingHealth = CrashPad->GetHealth() - SharedCue.Magnitude;
							CrashPad->SetHealth(RemainingHealth);
							CrashPad->ForceNetUpdate();

							if (CrashPad->GetHealth() <= 0)
								CrashPad->K2_DestroyActor();
						}
					}
					else if (Actor->IsA(ABuildingGameplayActorBalloon::StaticClass()))
					{
						ABuildingGameplayActorBalloon* Balloon = (ABuildingGameplayActorBalloon*)Actor;
						if (Balloon)
						{
							float RemainingHealth = Balloon->GetHealth() - SharedCue.Magnitude;
							Balloon->SetHealth(RemainingHealth);
							Balloon->ForceNetUpdate();

							if (Balloon->GetHealth() <= 0)
								Balloon->K2_DestroyActor();
						}
					}
				}
			}
		}
	}

	return defOG(Weapon, "/Script/FortniteGame.FortWeaponRanged", ServerNotifyPawnHit, Hit, ProjectileOriginPosition, ProjectileStartTimestamp, ArrayContext, LocalSpaceImpactPoint, LocalSpaceImpactNormal, bWasTargetingWhenProjectileFired);
}

void Player::ServerPlayEmoteItem(UObject* Context, FFrame& Stack)
{
	UFortMontageItemDefinitionBase* EmoteAsset;
	float EmoteRandomNumber;

	Stack.StepCompiledIn(&EmoteAsset);
	Stack.StepCompiledIn(&EmoteRandomNumber);
	Stack.IncrementCode();

	AFortPlayerController* PlayerController = static_cast<AFortPlayerController*>(Context);
	if (!PlayerController || !PlayerController->MyFortPawn || !EmoteAsset)
		return;

	AFortPlayerStateAthena* PlayerState = static_cast<AFortPlayerStateAthena*>(PlayerController->PlayerState);
	if (!PlayerState)
		return;

	UObject* AbilityToUse = nullptr;

	if (EmoteAsset->IsA(UAthenaSprayItemDefinition::StaticClass()))
	{
		static auto SprayAbilityClass = Runtime::StaticFindObject<UClass>("/Game/Abilities/Sprays/GAB_Spray_Generic.GAB_Spray_Generic_C");
		AbilityToUse = SprayAbilityClass->DefaultObject;
	}
	else if (EmoteAsset->IsA(UAthenaDanceItemDefinition::StaticClass()))
	{
		UAthenaDanceItemDefinition* Emote = (UAthenaDanceItemDefinition*)EmoteAsset;

		PlayerController->MyFortPawn->bMovingEmote = Emote->bMovingEmote;
		PlayerController->MyFortPawn->EmoteWalkSpeed = Emote->WalkForwardSpeed;

		PlayerController->MyFortPawn->bMovingEmoteForwardOnly = Emote->bMoveForwardOnly;
		PlayerController->MyFortPawn->bMovingEmoteFollowingOnly = Emote->bMoveFollowingOnly;

		UClass* CustomAbility = Emote->CustomDanceAbility ? Emote->CustomDanceAbility.Get() : nullptr;
		if (CustomAbility)
		{
			AbilityToUse = CustomAbility->DefaultObject;
		}
		else 
		{
			static UClass* EmoteAbilityClass = Runtime::StaticFindObject<UClass>("/Game/Abilities/Emotes/GAB_Emote_Generic.GAB_Emote_Generic_C");
			AbilityToUse = EmoteAbilityClass->DefaultObject;
		}
	}

	if (AbilityToUse)
	{
		FGameplayAbilitySpec Spec;
		((void (*)(FGameplayAbilitySpec*, const UObject*, int, int, UObject*))(Helios::Offsets::ConstructAbilitySpec))(&Spec, AbilityToUse, 1, -1, EmoteAsset);

		FGameplayAbilitySpecHandle Handle;
		((void (*)(UFortAbilitySystemComponent*, FGameplayAbilitySpecHandle*, FGameplayAbilitySpec*, void*))(Helios::Offsets::GiveAbilityAndActivateOnce))(PlayerState->AbilitySystemComponent, &Handle, &Spec, nullptr);

		UFortItemDefinition* Emote = PlayerController->MyFortPawn->LastReplicatedEmoteExecuted;
		PlayerController->MyFortPawn->LastReplicatedEmoteExecuted = EmoteAsset;
		PlayerController->MyFortPawn->OnRep_LastReplicatedEmoteExecuted(Emote);
	}
}

void Player::MovingEmoteStopped(UObject* Context, FFrame& Stack)
{
	Stack.IncrementCode();

	AFortPlayerPawnAthena* Pawn = (AFortPlayerPawnAthena*)Context;
	if (!Pawn)
		return;
	 
	Pawn->bMovingEmote = false;
	Pawn->bMovingEmoteForwardOnly = false;
	Pawn->bMovingEmoteFollowingOnly = false;
}

void Player::OpenActor(UObject* Context, FFrame& Stack)
{
	AActor* OpenableInterfaceActor;
	AFortPlayerControllerAthena* OptionalControllerInstigator;
	bool bRequestFastOpen = false;

	Stack.StepCompiledIn(&OpenableInterfaceActor);
	Stack.StepCompiledIn(&OptionalControllerInstigator);
	Stack.StepCompiledIn(&bRequestFastOpen);
	Stack.IncrementCode();

	UFortKismetLibrary* KismetLibrary = (UFortKismetLibrary*)Context;
	if (OpenableInterfaceActor->IsA(ABuildingWall::StaticClass()))
	{
		ABuildingWall* Wall = (ABuildingWall*)OpenableInterfaceActor;
		auto SetIsDoorOpen = (void(*)(ABuildingWall*, uint8_t, AFortPlayerPawnAthena*))(Helios::Offsets::SetIsDoorOpen);

		SetIsDoorOpen(Wall, bRequestFastOpen ? 1 : 0, (AFortPlayerPawnAthena*)(OptionalControllerInstigator ? OptionalControllerInstigator->Pawn : nullptr));
	} 
	else
	{
		return defOG(KismetLibrary, "/Script/FortniteGame.FortKismetLibrary", OpenActor, OpenableInterfaceActor, OptionalControllerInstigator, bRequestFastOpen);
	}
}

void Player::CloseActor(UObject* Context, FFrame& Stack)
{
	AActor* OpenableInterfaceActor;
	AFortPlayerControllerAthena* OptionalControllerInstigator;

	Stack.StepCompiledIn(&OpenableInterfaceActor);
	Stack.StepCompiledIn(&OptionalControllerInstigator);
	Stack.IncrementCode();

	UFortKismetLibrary* KismetLibrary = (UFortKismetLibrary*)Context;

	if (OpenableInterfaceActor->IsA(ABuildingWall::StaticClass()))
	{
		ABuildingWall* Wall = (ABuildingWall*)OpenableInterfaceActor;
		auto SetIsDoorOpen = (void(*)(ABuildingWall*, uint8_t, AFortPlayerPawnAthena*))(Helios::Offsets::SetIsDoorOpen);

		SetIsDoorOpen(Wall, 3, (AFortPlayerPawnAthena*)(OptionalControllerInstigator ? OptionalControllerInstigator->Pawn : nullptr));
	}
	else
	{
		return defOG(KismetLibrary, "/Script/FortniteGame.FortKismetLibrary", CloseActor, OpenableInterfaceActor, OptionalControllerInstigator);
	}
}

void Player::InternalPickup(AFortPlayerControllerAthena* PlayerController, FFortItemEntry PickupEntry)
{
	if (!PlayerController || !&PickupEntry)
		return;

	int32 MaxStack = (int32)PickupEntry.ItemDefinition->IsA(UFortResourceItemDefinition::StaticClass()) ? 500 : Runtime::EvaluateScalableFloat(((UFortItemDefinition*)PickupEntry.ItemDefinition)->MaxStackSize);
	int NumberOfSlotsToTake = 0;

	AFortInventory* Inventory = static_cast<AFortInventory*>(PlayerController->WorldInventory.ObjectPointer);
	if (!Inventory)
		return;

	for (auto& Item : Inventory->Inventory.ReplicatedEntries)
	{
		if (Inventory->GetQuickbar((UFortItemDefinition*)Item.ItemDefinition) == EFortQuickBars::Primary)
			NumberOfSlotsToTake += ((UFortWorldItemDefinition*)Item.ItemDefinition)->NumberOfSlotsToTake;
	}

	auto GiveOrSwap = [&]() {
		if (NumberOfSlotsToTake >= 5 && Inventory->GetQuickbar((UFortItemDefinition*)PickupEntry.ItemDefinition) == EFortQuickBars::Primary) {
			if (Inventory->GetQuickbar(PlayerController->MyFortPawn->CurrentWeapon->WeaponData) == EFortQuickBars::Primary) {
				auto ItemEntry = Inventory->Inventory.ReplicatedEntries.Search([PlayerController](FFortItemEntry& Entry) 
				{
					return Entry.ItemGuid == PlayerController->MyFortPawn->CurrentWeapon->ItemEntryGuid; 
				});

				if (ItemEntry)
				{

					Inventory::SpawnPickup(PlayerController->GetViewTarget()->K2_GetActorLocation(), ItemEntry, ItemEntry->Count, (AFortPlayerPawnAthena*)PlayerController->Pawn, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, true, true);
					Inventory->RemoveItem(PlayerController->MyFortPawn->CurrentWeapon->ItemEntryGuid);

					Inventory->AddItem((UFortItemDefinition*)PickupEntry.ItemDefinition, PickupEntry.Count, PickupEntry.LoadedAmmo);
				}
			}
			else {
				Inventory::SpawnPickup(PlayerController->GetViewTarget()->K2_GetActorLocation(), &PickupEntry, PickupEntry.Count, (AFortPlayerPawnAthena*)PlayerController->Pawn, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, true, true);
			}
		}
		else
		{
			Inventory->AddItem((UFortItemDefinition*)PickupEntry.ItemDefinition, PickupEntry.Count, PickupEntry.LoadedAmmo);
		}
	};

	auto GiveOrSwapStack = [&](int32 OriginalCount) {
		if (((UFortItemDefinition*)PickupEntry.ItemDefinition)->bAllowMultipleStacks && NumberOfSlotsToTake < 5)
		{
			Inventory->AddItem((UFortItemDefinition*)PickupEntry.ItemDefinition, OriginalCount - MaxStack, PickupEntry.LoadedAmmo);
		}
		else
		{
			Inventory::SpawnPickup(PlayerController->GetViewTarget()->K2_GetActorLocation(), &PickupEntry, OriginalCount - MaxStack, (AFortPlayerPawnAthena*)PlayerController->Pawn, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, true, true);
		}
	};

	if (((UFortItemDefinition*)PickupEntry.ItemDefinition)->IsStackable()) {
		auto ItemEntry = Inventory->Inventory.ReplicatedEntries.Search([PickupEntry, MaxStack](FFortItemEntry& Entry)
		{ 
			return Entry.ItemDefinition == PickupEntry.ItemDefinition && Entry.Count < MaxStack;
		});

		if (ItemEntry) {
			auto State = ItemEntry->StateValues.Search([](FFortItemEntryStateValue& Value)
			{
				return Value.StateType == EFortItemEntryState::ShouldShowItemToast; 
			});

			if (!State) {
				FFortItemEntryStateValue Value{};
				Value.StateType = EFortItemEntryState::ShouldShowItemToast;
				Value.IntValue = true;
				ItemEntry->StateValues.Add(Value);
			}
			else
			{
				State->IntValue = true;
			}

			if ((ItemEntry->Count += PickupEntry.Count) > MaxStack) {
				auto OriginalCount = ItemEntry->Count;
				ItemEntry->Count = MaxStack;

				GiveOrSwapStack(OriginalCount);
			}

			Inventory->UpdateEntry(ItemEntry);
		}
		else {
			if (PickupEntry.Count > MaxStack) {
				auto OriginalCount = PickupEntry.Count;
				PickupEntry.Count = MaxStack;

				GiveOrSwapStack(OriginalCount);
			}

			GiveOrSwap();
		}
	}
	else {
		GiveOrSwap();
	}
}

bool Player::FinishedTargetSpline(AFortPickup* Pickup) {
	if (!Pickup)
		return Originals::FinishedTargetSpline(Pickup);

	auto PickupTarget = Pickup->PickupLocationData.PickupTarget;
	if (!PickupTarget.Get())
		return Originals::FinishedTargetSpline(Pickup);

	AFortPlayerPawnAthena* Pawn = static_cast<AFortPlayerPawnAthena*>(PickupTarget.Get());
	if (!Pawn)
		return Originals::FinishedTargetSpline(Pickup);

	AFortPlayerControllerAthena* PlayerController =  static_cast<AFortPlayerControllerAthena*>(Pawn->Controller);
	if (!PlayerController)
		return Originals::FinishedTargetSpline(Pickup);

	AFortInventory* Inventory = static_cast<AFortInventory*>(PlayerController->WorldInventory.ObjectPointer);
	if (!Inventory)
		return Originals::FinishedTargetSpline(Pickup);

	if (PlayerController->bTryPickupSwap)
	{
		AFortInventory* Inventory = static_cast<AFortInventory*>(PlayerController->WorldInventory.ObjectPointer);
		if (Inventory)
		{
			float RandomAngle = UKismetMathLibrary::RandomFloatInRange(-18.f, 18.f);
			float FinalAngle = UKismetMathLibrary::DegreesToRadians(RandomAngle);

			FVector FinalLocation = Pawn->K2_GetActorLocation();
			FVector ForwardVector = Pawn->GetActorForwardVector();

			ForwardVector.Z = 0.0f;
			ForwardVector.Normalize();

			FinalLocation = FinalLocation + ForwardVector * 450.f;
			FinalLocation.Z += 50.f;
			FinalLocation.X += std::cos(FinalAngle) * 100.f;
			FinalLocation.Y += std::sin(FinalAngle) * 100.f;

			if (Inventory->GetQuickbar(Pawn->CurrentWeapon->WeaponData) == EFortQuickBars::Primary && Inventory->GetQuickbar((UFortItemDefinition*)Pickup->PrimaryPickupItemEntry.ItemDefinition) == EFortQuickBars::Primary && Inventory)
			{
				PlayerController->bTryPickupSwap = false;

				FFortItemEntry* ItemEntry = Inventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& Entry) { return Entry.ItemGuid == Pawn->CurrentWeapon->ItemEntryGuid; });
				if (ItemEntry)
				{
					Inventory::SpawnPickup(PlayerController->GetViewTarget()->K2_GetActorLocation(), ItemEntry, -1, Pawn, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, true, true, FinalLocation);
					Inventory->RemoveItem(ItemEntry->ItemGuid);

					UFortWorldItem* Item = Inventory->AddItem((UFortItemDefinition*)Pickup->PrimaryPickupItemEntry.ItemDefinition, Pickup->PrimaryPickupItemEntry.Count, Pickup->PrimaryPickupItemEntry.LoadedAmmo);
					if (Item)
					{
						PlayerController->ServerExecuteInventoryItem(Item->ItemEntry.ItemGuid);
					}
				}
			}
			else
			{
				Inventory::SpawnPickup(PlayerController->GetViewTarget()->K2_GetActorLocation(), &Pickup->PrimaryPickupItemEntry, -1, Pawn, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, true, true, FinalLocation);
			}
		}
	}
	else
	{
		InternalPickup(PlayerController, Pickup->PrimaryPickupItemEntry);
	}

	return Originals::FinishedTargetSpline(Pickup);
}

void Player::ServerSendZiplineState(AFortPlayerPawnAthena* Pawn, FZiplinePawnState& InZiplineState)
{
	AFortAthenaZiplineBase* OriginalZipline = Pawn->GetActiveZipline();

	Pawn->ZiplineState = InZiplineState;
	((void (*)(AFortPlayerPawn*))(ImageBase+ 0x92492EC))(Pawn);

	if (InZiplineState.bJumped)
	{
		auto Velocity = Pawn->CharacterMovement->Velocity;
		auto VelocityX = Velocity.X * -0.5f;
		auto VelocityY = Velocity.Y * -0.5f;
		Pawn->LaunchCharacterJump({ VelocityX >= -750 ? fminf(VelocityX, 750) : -750, VelocityY >= -750 ? fminf(VelocityY, 750) : -750, 1200 }, false, false, true, true);
	}

	AFortAthenaZiplineBase* Zipline = Pawn->GetActiveZipline();
	UClass* B_Athena_Zipline_Ascender = Runtime::StaticFindObject<UClass>("/Ascender/Gameplay/Ascender/B_Athena_Zipline_Ascender.B_Athena_Zipline_Ascender_C");

	if (OriginalZipline && OriginalZipline->IsA(B_Athena_Zipline_Ascender))
	{
		if (auto Ascender = (AFortAscenderZipline*)OriginalZipline)
		{
			Ascender->PawnUsingHandle = nullptr;
			Ascender->PreviousPawnUsingHandle = Pawn;
			Ascender->OnRep_PawnUsingHandle();
		}
	}
	else if (Zipline && Zipline->IsA(B_Athena_Zipline_Ascender))
	{
		if (auto Ascender = (AFortAscenderZipline*)Zipline)
		{
			Ascender->PawnUsingHandle = Pawn;
			Ascender->PreviousPawnUsingHandle = nullptr;
			Ascender->OnRep_PawnUsingHandle();
		}
	}
}

void Player::TeleportPlayerPawn(UObject* Context, FFrame& Stack, bool* Ret)
{
	UObject* WorldContextObject;
	AFortPlayerPawnAthena* PlayerPawn;
	FVector DestLocation;
	FRotator DestRotation;
	bool bIgnoreCollision;
	bool bIgnoreSupplementalKillVolumeSweep;

	Stack.StepCompiledIn(&WorldContextObject);
	Stack.StepCompiledIn(&PlayerPawn);
	Stack.StepCompiledIn(&DestLocation);
	Stack.StepCompiledIn(&DestRotation);
	Stack.StepCompiledIn(&bIgnoreCollision);
	Stack.StepCompiledIn(&bIgnoreSupplementalKillVolumeSweep);
	Stack.IncrementCode();

	PlayerPawn->K2_TeleportTo(DestLocation, DestRotation);
	*Ret = true;
}

void Player::ServerClientIsReadyToRespawn(AFortPlayerControllerAthena* PlayerController)
{
	AFortPlayerStateAthena* PlayerState = static_cast<AFortPlayerStateAthena*>(PlayerController->PlayerState);
	AFortGameModeAthena* GameMode = static_cast<AFortGameModeAthena*>(UWorld::GetWorld()->AuthorityGameMode);

	if (PlayerState->RespawnData.bRespawnDataAvailable && PlayerState->RespawnData.bServerIsReady)
	{
		FTransform Transform = {};
		Transform.Scale3D = FVector{ 1,1,1 };
		Transform.Translation = PlayerState->RespawnData.RespawnLocation;
		Transform.Rotation = PlayerState->RespawnData.RespawnRotation.Quat();

		AFortPlayerPawnAthena* Pawn = (AFortPlayerPawnAthena*)GameMode->SpawnDefaultPawnAtTransform(PlayerController, Transform);
	
		PlayerController->Possess(Pawn);
		PlayerController->RespawnPlayerAfterDeath(true);

		PlayerController->MyFortPawn->SetHealth(100);
		PlayerController->ServerAcknowledgePossession(PlayerController->MyFortPawn);
	}

	PlayerState->RespawnData.bClientIsReady = true;
	return Originals::ServerClientIsReadyToRespawn(PlayerController);
}

void Player::Patch()
{
	Runtime::Exec("/Script/FortniteGame.FortWeaponRanged.ServerNotifyPawnHit", ServerNotifyPawnHit, Originals::ServerNotifyPawnHit);
	Runtime::Exec("/Script/FortniteGame.FortKismetLibrary.OpenActor", OpenActor, Originals::OpenActor);
	Runtime::Exec("/Script/FortniteGame.FortKismetLibrary.CloseActor", CloseActor, Originals::CloseActor);
	Runtime::Exec("/Script/FortniteGame.FortPlayerPawn.ServerHandlePickupInfo", ServerHandlePickupInfo);
	Runtime::Exec("/Script/FortniteGame.FortPlayerController.ServerPlayEmoteItem", ServerPlayEmoteItem);
	Runtime::Exec("/Script/FortniteGame.FortPawn.MovingEmoteStopped", MovingEmoteStopped);
	Runtime::Exec("/Script/FortniteGame.FortMissionLibrary.TeleportPlayerPawn", TeleportPlayerPawn);

	Runtime::Hook(EHook::Hook, Helios::Offsets::ClientOnPawnDied, ClientOnPawnDied, (void**)&Originals::ClientOnPawnDied);
	Runtime::Hook(EHook::Hook, Helios::Offsets::GetPlayerViewPoint, GetPlayerViewPoint, (void**)&Originals::GetPlayerViewPoint);
	Runtime::Hook(EHook::Hook, Helios::Offsets::FinishedTargetSpline, FinishedTargetSpline, (void**)&Originals::FinishedTargetSpline);
	Runtime::Hook(EHook::Hook, Helios::Offsets::ServerAcknowledgePossession, ServerAcknowledgePossession);

	Runtime::Hook<AFortPlayerControllerAthena>(EHook::Virtual, Helios::Offsets::VFTs::ServerAttemptInventoryDrop, ServerAttemptInventoryDrop);
	Runtime::Hook<AFortPlayerControllerAthena>(EHook::Virtual, Helios::Offsets::VFTs::ServerExecuteInventoryItem, ServerExecuteInventoryItem);
	Runtime::Hook<AFortPlayerControllerAthena>(EHook::Virtual, Helios::Offsets::VFTs::ServerClientIsReadyToRespawn, ServerClientIsReadyToRespawn, (void**)&Originals::ServerClientIsReadyToRespawn);

	Runtime::Hook<AFortPlayerPawnAthena>(EHook::Virtual, Helios::Offsets::VFTs::ServerSendZiplineState, ServerSendZiplineState);
	Runtime::Hook<UFortControllerComponent_Aircraft>(EHook::Virtual, Helios::Offsets::VFTs::ServerAttemptAircraftJump, ServerAttemptAircraftJump);
}
