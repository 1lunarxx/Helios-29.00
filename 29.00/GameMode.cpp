#include "pch.h"
#include "GameMode.h"
#include "Options.h"
#include "Lategame.h"
#include "Misc.h"

bool GameMode::ReadyToStartMatch(AFortGameModeAthena* GameMode)
{
    AFortGameStateAthena* GameState = static_cast<AFortGameStateAthena*>(GameMode->GameState);
    if (!GameState->MapInfo)
    {
        return false;
    }

    if (GameMode->CurrentPlaylistId == -1)
    {
        UFortPlaylistAthena* Playlist = Runtime::StaticLoadObject<UFortPlaylistAthena>(UHeliosConfiguration::PlaylistID);
        if (!Playlist)
            return false;

        GameMode->CurrentPlaylistId = Playlist->PlaylistId;
        GameMode->CurrentPlaylistName = Playlist->PlaylistName;

        GameState->CurrentPlaylistId = Playlist->PlaylistId;
        GameState->CurrentPlaylistInfo.BasePlaylist = Playlist;
        GameState->CurrentPlaylistInfo.BasePlaylist->GarbageCollectionFrequency = 9999999999999999.f;
        GameState->CurrentPlaylistInfo.OverridePlaylist = Playlist;
        GameState->CurrentPlaylistInfo.PlaylistReplicationKey++;
        GameState->CurrentPlaylistInfo.MarkArrayDirty();

        GameState->OnRep_CurrentPlaylistId();
        GameState->OnRep_CurrentPlaylistInfo();

        GameMode->bDBNOEnabled = Playlist->MaxSquadSize > 1;

        GameState->bDBNODeathEnabled = Playlist->MaxSquadSize > 1;
        GameState->SetIsDBNODeathEnabled(GameState->bDBNODeathEnabled);

        GameMode->GameSession->MaxPlayers = Playlist->MaxPlayers;
        GameMode->WarmupRequiredPlayerCount = 1;

        GameMode->bDisableGCOnServerDuringMatch = true;
        GameMode->bPlaylistHotfixChangedGCDisabling = true;

        Lategame::InitializeLategameLoadouts();

        for (auto& Level : GameState->CurrentPlaylistInfo.BasePlaylist->AdditionalLevels)
        {
            bool Success = false;
            ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(UWorld::GetWorld(), Level, FVector(), FRotator(), &Success, FString(), nullptr, false);

            FAdditionalLevelStreamed LevelStreamed{};
            LevelStreamed.bIsServerOnly = false;
            LevelStreamed.LevelName = Level.ObjectID.AssetPath.AssetName;

            if (Success)
            {
                GameState->AdditionalPlaylistLevelsStreamed.Add(LevelStreamed);
            }
        }

        for (auto& Level : GameState->CurrentPlaylistInfo.BasePlaylist->AdditionalLevelsServerOnly)
        {
            bool Success = false;
            ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(UWorld::GetWorld(), Level, FVector(), FRotator(), &Success, FString(), nullptr, false);

            FAdditionalLevelStreamed LevelStreamed{};
            LevelStreamed.bIsServerOnly = true;
            LevelStreamed.LevelName = Level.ObjectID.AssetPath.AssetName;

            if (Success)
            {
                GameState->AdditionalPlaylistLevelsStreamed.Add(LevelStreamed);
            }
        }

        GameState->OnRep_AdditionalPlaylistLevelsStreamed();

        auto InitializeFlightPath = (void(*)(AFortAthenaMapInfo*, AFortGameStateAthena*, UFortGameStateComponent_BattleRoyaleGamePhaseLogic*, bool, double, float, float))Helios::Offsets::InitializeFlightPath;
        InitializeFlightPath(GameState->MapInfo, GameState, UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(GameState), false, 0.f, 0.f, 360.f);
    }

    if (!GameMode->bWorldIsReady)
    {
        TArray<AActor*> PlayerStarts = Runtime::GetAll<AFortPlayerStartWarmup>();
        int32 Num = PlayerStarts.Num();
        PlayerStarts.Free();

        if (Num == 0)
            return false;
      
        Misc::Listen();

        SetConsoleTitleA("Helios | Ready");
        GameMode->bWorldIsReady = true;
    }

    return GameMode->AlivePlayers.Num() > 0;
}

APawn* GameMode::SpawnDefaultPawnFor(AFortGameModeAthena* GameMode, AFortPlayerControllerAthena* PlayerController, AActor* StartSpot)
{
    AFortInventory* Inventory = static_cast<AFortInventory*>(PlayerController->WorldInventory.ObjectPointer);
    if (Inventory)
    {
        int32 Num = Inventory->Inventory.ReplicatedEntries.Num();
        if (Num != 0) {
            Inventory->Inventory.ReplicatedEntries.ResetNum();
            Inventory->Inventory.ItemInstances.ResetNum();
        }
    }

    Inventory->AddItem((UFortItemDefinition*)PlayerController->CosmeticLoadoutPC.Pickaxe->WeaponDefinition);

    for (auto& StartingItem : GameMode->StartingItems)
    {
        if (StartingItem.Count > 0 && !StartingItem.Item->IsA(UFortSmartBuildingItemDefinition::StaticClass()))
        {
           Inventory->AddItem((UFortItemDefinition*)StartingItem.Item, StartingItem.Count);
        }
    }

    UFortGameStateComponent_BattleRoyaleGamePhaseLogic* GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(UWorld::GetWorld());

    FTransform Transform{};
    Transform = StartSpot->GetTransform();
    Transform.Translation.Z += GamePhaseLogic->GamePhase == EAthenaGamePhase::Warmup ? 950.f : 0.f;

    AFortPlayerPawnAthena* Pawn = Runtime::SpawnActor<AFortPlayerPawnAthena>(Transform, GameMode->GetDefaultPawnClassForController(PlayerController), PlayerController);
    while (!Pawn)
    {
        AActor* PlayerStart = GameMode->ChoosePlayerStart(PlayerController);
        if (PlayerStart)
        {
            Transform = PlayerStart->GetTransform();
            Transform.Translation.Z += GamePhaseLogic->GamePhase == EAthenaGamePhase::Warmup ? 950.f : 0.f;

            Pawn = Runtime::SpawnActor<AFortPlayerPawnAthena>(Transform, GameMode->GetDefaultPawnClassForController(PlayerController), PlayerController);
        }
    }

    return Pawn;
}

void GameMode::HandleStartingNewPlayer(AFortGameModeAthena* GameMode, AFortPlayerControllerAthena* PlayerController)
{
    if (!PlayerController->WorldInventory.ObjectPointer)
    {
        PlayerController->WorldInventory.ObjectPointer = Runtime::SpawnActor<AFortInventory>(FVector{}, FRotator{}, PlayerController->WorldInventoryClass, PlayerController);
        PlayerController->WorldInventory.InterfacePointer = Runtime::GetInterface<IFortInventoryInterface>(PlayerController->WorldInventory.ObjectPointer);
    }

    AFortPlayerStateAthena* PlayerState = static_cast<AFortPlayerStateAthena*>(PlayerController->PlayerState);
    AFortGameStateAthena* GameState = static_cast<AFortGameStateAthena*>(GameMode->GameState);

    if (PlayerState)
    {
        uint8 TeamIndex = PlayerState->TeamIndex;

        PlayerState->TeamIndex = CurrentTeam++;
        PlayerState->OnRep_TeamIndex(TeamIndex);

        FGameMemberInfo Member{};
        Member.MostRecentArrayReplicationKey = -1;
        Member.ReplicationID = -1;
        Member.ReplicationKey = -1;
        Member.TeamIndex = PlayerState->TeamIndex;
        Member.SquadId = PlayerState->SquadId;
        Member.MemberUniqueId = PlayerState->UniqueID;

        GameState->GameMemberInfoArray.Members.Add(Member);
        GameState->GameMemberInfoArray.MarkItemDirty(Member);

        auto NotifyGameMemberAdded = (void(*)(AFortGameStateAthena*, uint8, uint8, FUniqueNetIdRepl*))Helios::Offsets::NotifyGameMemberAdded;
        NotifyGameMemberAdded(GameState, Member.SquadId, Member.TeamIndex, &Member.MemberUniqueId);

        PlayerState->SeasonLevelUIDisplay = PlayerController->XPComponent->CurrentLevel;
        PlayerState->OnRep_SeasonLevelUIDisplay();
    }

    PlayerController->XPComponent->bRegisteredWithQuestManager = true;
    PlayerController->XPComponent->OnRep_bRegisteredWithQuestManager();

    return Originals::HandleStartingNewPlayer(GameMode, PlayerController);
}

void GameMode::HandleMatchHasStarted(AFortGameModeAthena* GameMode)
{
    UFortGameStateComponent_BattleRoyaleGamePhaseLogic* GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(UWorld::GetWorld());
    AFortGameStateAthena* GameState = (AFortGameStateAthena*)GameMode->GameState;

    if (GamePhaseLogic)
    {
        const float EndTIme = 60.f;

        GamePhaseLogic->GamePhase = EAthenaGamePhase::Warmup;
        GamePhaseLogic->OnRep_GamePhase(EAthenaGamePhase::Setup);

        GamePhaseLogic->WarmupCountdownStartTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
        GamePhaseLogic->WarmupCountdownEndTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) + EndTIme;
        GamePhaseLogic->OnRep_WarmupCountdownEndTime();

        GamePhaseLogic->WarmupCountdownDuration = EndTIme;
        GamePhaseLogic->WarmupEarlyCountdownDuration = EndTIme;

        TArray<AFortAthenaAircraft*> Aircrafts;
        Aircrafts.Add(AFortAthenaAircraft::SpawnAircraft(UWorld::GetWorld(), GameState->MapInfo->AircraftClass, GameState->MapInfo->FlightInfos[0]));

        GamePhaseLogic->SetAircrafts(Aircrafts);
        GamePhaseLogic->OnRep_Aircrafts();
    }

    return Originals::HandleMatchHasStarted(GameMode);
}

void GameMode::Patch()
{
    Runtime::Hook(EHook::Hook, Helios::Offsets::SpawnDefaultPawnFor, SpawnDefaultPawnFor);
    Runtime::Hook(EHook::Hook, Helios::Offsets::ReadyToStartMatch, ReadyToStartMatch);
    Runtime::Hook(EHook::Hook, Helios::Offsets::HandleMatchHasStarted, HandleMatchHasStarted, (void**)&Originals::HandleMatchHasStarted);
    Runtime::Hook(EHook::Hook, Helios::Offsets::HandleStartingNewPlayer, HandleStartingNewPlayer, (void**)&Originals::HandleStartingNewPlayer);
}