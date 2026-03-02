#include "pch.h"
#include "GamePhaseLogic.h"
#include "Options.h"
#include "XP.h"
#include "Helios.h"
#include "Inventory.h"

void GamePhaseLogic::GenerateStormCircles(AFortAthenaMapInfo* MapInfo)
{
    if (!MapInfo)
        return;

    StormCircles.clear();

    const auto RandFloat = []() -> float
    {
        return (float)rand() / (float)RAND_MAX;
    };

    const float PI = 3.14159265358979323846f;
    const float CircleRadius[12] = { 120000, 95000, 70000, 55000, 32500, 20000, 10000, 5000, 2500, 1650, 1090, 0 };

    FVector MapCenter = MapInfo->GetMapCenter();
    StormCircles.push_back({ MapCenter, CircleRadius[0]});

    for (int i = 1; i < 7; ++i)
    {
        const FVector& Center = StormCircles[i - 1].Center;
        const float Radius = StormCircles[i - 1].Radius;

        const float angle = RandFloat() * 2.f * PI;
        const float sin = std::sin(angle);
        const float cos = std::cos(angle);

        const float Dist = RandFloat() * Radius * 0.4f;

        FVector Location(cos, sin, 0.f);
        Location = GetSafeNormal(Location);

        FVector NewCenter = Center + Location * Dist;
        NewCenter = ClampToPlayableBounds(NewCenter, CircleRadius[i], MapInfo->CachedPlayableBoundsForClients);

        StormCircles.push_back({ NewCenter, CircleRadius[i] });
    }

    for (int i = 7; i < 12; ++i)
    {
        const FVector& PreviousCenter = StormCircles[i - 1].Center;
        const float PreviousRadius = StormCircles[i - 1].Radius;
        const float NewRadius = CircleRadius[i];

        const float angle = RandFloat() * 2.f * PI;
        const float sin = std::sin(angle);
        const float cos = std::cos(angle);

        const float Dist = std::sqrt((NewRadius * NewRadius) - (NewRadius * NewRadius));

        FVector Location(cos, sin, 0.f);
        Location = GetSafeNormal(Location);

        FVector NewCenter = PreviousCenter + Location * Dist;
        NewCenter = ClampToPlayableBounds(NewCenter, NewRadius, MapInfo->CachedPlayableBoundsForClients);

        StormCircles.push_back({ NewCenter, NewRadius });
    }
}

void GamePhaseLogic::SpawnSafeZoneIndicator(UFortGameStateComponent_BattleRoyaleGamePhaseLogic* GamePhaseLogic)
{
    AFortGameModeAthena* GameMode = static_cast<AFortGameModeAthena*>(UWorld::GetWorld()->AuthorityGameMode);
    AFortGameStateAthena* GameState = static_cast<AFortGameStateAthena*>(GameMode->GameState);

    if (StormCircles.empty())
    {
        GenerateStormCircles(GameState->MapInfo);
    }

    if (!GamePhaseLogic->SafeZoneIndicator)
    {
        AFortSafeZoneIndicator* SafeZoneIndicator = Runtime::SpawnActor<AFortSafeZoneIndicator>({}, {}, GamePhaseLogic->SafeZoneIndicatorClass.Get());
        if (SafeZoneIndicator)
        {
            FFortSafeZoneDefinition& SafeZoneDefinition = GameState->MapInfo->SafeZoneDefinition;

            const float Time = UGameplayStatics::GetTimeSeconds(GameState);
            const float SafeZoneDefinitionCount = Runtime::EvaluateScalableFloat(SafeZoneDefinition.Count);

            TArray<FFortSafeZonePhaseInfo>& SafeZonePhases = SafeZoneIndicator->SafeZonePhases;
            if (SafeZonePhases.IsValid())
            {
                SafeZonePhases.Free();
            }

            for (float i = 0; i < SafeZoneDefinitionCount; i++)
            {
                FFortSafeZonePhaseInfo PhaseInfo{};
                
                UDataTableFunctionLibrary::EvaluateCurveTableRow(GameState->AthenaGameDataTable, UKismetStringLibrary::Conv_StringToName(L"Default.SafeZone.Damage"), i, nullptr, &PhaseInfo.DamageInfo.Damage, FString());
                if (i == 0.f)
                    PhaseInfo.DamageInfo.Damage = 0.01f;

                PhaseInfo.Center = StormCircles[(int)i].Center;
                PhaseInfo.DamageInfo.bPercentageBasedDamage = true;
                PhaseInfo.MegaStormGridCellThickness = (int)Runtime::EvaluateScalableFloat(SafeZoneDefinition.MegaStormGridCellThickness, i);
                PhaseInfo.PlayerCap = (int)Runtime::EvaluateScalableFloat(SafeZoneDefinition.PlayerCapSolo, i);
                PhaseInfo.Radius = Runtime::EvaluateScalableFloat(SafeZoneDefinition.Radius, i);
                PhaseInfo.ShrinkTime = Runtime::EvaluateScalableFloat(SafeZoneDefinition.ShrinkTime, i);
                PhaseInfo.StormCampingIncrementTimeAfterDelay = Runtime::EvaluateScalableFloat(GamePhaseLogic->StormCampingIncrementTimeAfterDelay, i);
                PhaseInfo.StormCampingInitialDelayTime = Runtime::EvaluateScalableFloat(GamePhaseLogic->StormCampingInitialDelayTime, i);
                PhaseInfo.StormCapDamagePerTick = Runtime::EvaluateScalableFloat(GamePhaseLogic->StormCapDamagePerTick, i);
                PhaseInfo.TimeBetweenStormCapDamage = Runtime::EvaluateScalableFloat(GamePhaseLogic->TimeBetweenStormCapDamage, i);

                PhaseInfo.UsePOIStormCenter = false;
                PhaseInfo.WaitTime = Runtime::EvaluateScalableFloat(SafeZoneDefinition.WaitTime, i);
                PhaseInfo.PlayerCap = (int)Runtime::EvaluateScalableFloat(SafeZoneDefinition.PlayerCapSolo, i);

                PhaseInfo.Center = StormCircles[(int)i].Center;

                SafeZonePhases.Add(PhaseInfo);
                SafeZoneIndicator->PhaseCount++;
                SafeZoneIndicator->OnRep_PhaseCount();
            }

            SafeZoneIndicator->SafeZoneStartShrinkTime = Time + SafeZonePhases[0].WaitTime;
            SafeZoneIndicator->SafeZoneFinishShrinkTime = SafeZoneIndicator->SafeZoneStartShrinkTime + SafeZonePhases[0].ShrinkTime;

            SafeZoneIndicator->CurrentPhase = 0;
        }

        SafeZoneIndicator->OnRep_CurrentPhase();

        GamePhaseLogic->SafeZoneIndicator = SafeZoneIndicator;
        GamePhaseLogic->OnRep_SafeZoneIndicator();
    }
}

void GamePhaseLogic::StartNewSafeZonePhase(UFortGameStateComponent_BattleRoyaleGamePhaseLogic* GamePhaseLogic, int Phase)
{
    const float Time = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
    const auto& SafeZonePhases = GamePhaseLogic->SafeZoneIndicator->SafeZonePhases;

    if (SafeZonePhases.IsValidIndex(Phase))
    {
        if (SafeZonePhases.IsValidIndex(Phase - 1))
        {
            FFortSafeZonePhaseInfo PreviousPhaseInfo = SafeZonePhases.Get(Phase - 1);

            GamePhaseLogic->SafeZoneIndicator->PreviousCenter = (FVector_NetQuantize100)PreviousPhaseInfo.Center;
            GamePhaseLogic->SafeZoneIndicator->PreviousRadius = PreviousPhaseInfo.Radius;
        }

        FFortSafeZonePhaseInfo PhaseInfo = SafeZonePhases.Get(Phase);

        GamePhaseLogic->SafeZoneIndicator->NextCenter = (FVector_NetQuantize100)PhaseInfo.Center;
        GamePhaseLogic->SafeZoneIndicator->NextRadius = PhaseInfo.Radius;
        GamePhaseLogic->SafeZoneIndicator->NextMegaStormGridCellThickness = PhaseInfo.MegaStormGridCellThickness;

        if (SafeZonePhases.IsValidIndex(Phase + 1))
        {
            FFortSafeZonePhaseInfo NextPhaseInfo = SafeZonePhases.Get(Phase + 1);

            if (GamePhaseLogic->SafeZoneIndicator->FutureReplicator)
            {
                GamePhaseLogic->SafeZoneIndicator->FutureReplicator->NextNextCenter = (FVector_NetQuantize100)NextPhaseInfo.Center;
                GamePhaseLogic->SafeZoneIndicator->FutureReplicator->NextNextRadius = NextPhaseInfo.Radius;
            }

            GamePhaseLogic->SafeZoneIndicator->NextNextCenter = (FVector_NetQuantize100)NextPhaseInfo.Center;
            GamePhaseLogic->SafeZoneIndicator->NextNextRadius = NextPhaseInfo.Radius;
            GamePhaseLogic->SafeZoneIndicator->NextNextMegaStormGridCellThickness = NextPhaseInfo.MegaStormGridCellThickness;
        }

        GamePhaseLogic->SafeZoneIndicator->SafeZoneStartShrinkTime = Time + PhaseInfo.WaitTime;
        GamePhaseLogic->SafeZoneIndicator->SafeZoneFinishShrinkTime = GamePhaseLogic->SafeZoneIndicator->SafeZoneStartShrinkTime + PhaseInfo.ShrinkTime;

        GamePhaseLogic->SafeZoneIndicator->CurrentDamageInfo = PhaseInfo.DamageInfo;
        GamePhaseLogic->SafeZoneIndicator->OnRep_CurrentDamageInfo();

        GamePhaseLogic->SafeZoneIndicator->CurrentPhase = Phase;
        GamePhaseLogic->SafeZoneIndicator->OnRep_CurrentPhase();

        GamePhaseLogic->SafeZoneIndicator->OnSafeZonePhaseChanged.ProcessDelegate();

        EFortSafeZoneState& SafeZoneState = *(EFortSafeZoneState*)(__int64(&GamePhaseLogic->SafeZoneIndicator->FutureReplicator) - 0x4);
        SafeZoneState = EFortSafeZoneState::Holding;

        int32 CurrentPhase = GamePhaseLogic->SafeZoneIndicator->CurrentPhase;

        GamePhaseLogic->SafeZoneIndicator->OnSafeZoneStateChange(EFortSafeZoneState::Holding, CurrentPhase <= 0);
        GamePhaseLogic->SafeZoneIndicator->SafezoneStateChangedDelegate.ProcessDelegate(GamePhaseLogic->SafeZoneIndicator, EFortSafeZoneState::Holding);

        GamePhaseLogic->GamePhaseStep = EAthenaGamePhaseStep::StormHolding;
        GamePhaseLogic->HandleGamePhaseStepChanged(EAthenaGamePhaseStep::StormHolding);

        for (auto& Player : UFortKismetLibrary::GetAllFortPlayerControllers(UWorld::GetWorld(), true, false))
        {
            if (!Player)
                continue;

            AFortPlayerControllerAthena* Controller = static_cast<AFortPlayerControllerAthena*>(Player);
            XP::SendStatEvent(Controller, EFortQuestObjectiveStatEvent::StormPhase);
        }
    }
}

void GamePhaseLogic::Tick()
{
    UFortGameStateComponent_BattleRoyaleGamePhaseLogic* GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(UWorld::GetWorld());

    AFortGameModeAthena* GameMode = static_cast<AFortGameModeAthena*>(UWorld::GetWorld()->AuthorityGameMode);
    AFortGameStateAthena* GameState = static_cast<AFortGameStateAthena*>(GameMode->GameState);

    if (GameMode->AlivePlayers.Num() <= 0)
        return;

    const float Time = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());

    static bool bAircraftStarted = false;
    if (!bAircraftStarted && GamePhaseLogic->WarmupCountdownEndTime <= Time && GamePhaseLogic->Aircrafts_GameState.IsValid() && GameState->MapInfo->FlightInfos.IsValid())
    {
        GenerateStormCircles(GameState->MapInfo);

        AFortAthenaAircraft* Aircraft = GamePhaseLogic->Aircrafts_GameState[0].Get();
        FAircraftFlightInfo& FlightInfo = GameState->MapInfo->FlightInfos[0];

        if (!Aircraft)
            return;

        if (UHeliosConfiguration::bIsProd)
        {
            Helios::Sessions::DeleteSeleniumSession();
        }

        if (UHeliosConfiguration::bIsLateGame)
        {
            float EndTime = 20.f;
            int Phase = GameMode->AlivePlayers.Num() >= 20 ? 5 : 6;

            FVector Location = StormCircles[Phase].Center;
            Location.Z = 17500.f;

            FlightInfo.FlightSpeed = 0.f;
            FlightInfo.FlightStartLocation = (FVector_NetQuantize100)Location;

            FlightInfo.TimeTillFlightEnd = EndTime;
            FlightInfo.TimeTillDropEnd = EndTime;
            FlightInfo.TimeTillDropStart = 0.f;

            GamePhaseLogic->SafeZonesStartTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) + EndTime;
            GameState->DefaultParachuteDeployTraceForGroundDistance = 3500.f;

            SpawnSafeZoneIndicator(GamePhaseLogic);
            StartNewSafeZonePhase(GamePhaseLogic, Phase);
        }

        Aircraft->FlightElapsedTime = 0;
        Aircraft->FlightStartTime = Time;

        Aircraft->ReplicatedFlightTimestamp = Time;
        Aircraft->DropStartTime = Time + FlightInfo.TimeTillDropStart;
        Aircraft->DropEndTime = Time + FlightInfo.TimeTillDropEnd;
        Aircraft->FlightEndTime = Time + FlightInfo.TimeTillFlightEnd;

        Aircraft->FlightInfo = FlightInfo;
        GamePhaseLogic->bAircraftIsLocked = true;

        for (auto Player : UFortKismetLibrary::GetAllFortPlayerControllers(UWorld::GetWorld(), true, false))
        {
            if (!Player)
                continue;

            AFortPlayerPawnAthena* Pawn = static_cast<AFortPlayerPawnAthena*>(Player->Pawn);
            if (Pawn)
            {
                if (Pawn->Role == ENetRole::ROLE_SimulatedProxy)
                {
                    if (Pawn->bIsInAnyStorm)
                    {
                        Pawn->bIsInAnyStorm = false;
                        Pawn->OnRep_IsInAnyStorm();
                    }
                }

                Pawn->bIsInsideSafeZone = true;
                Pawn->OnRep_IsInsideSafeZone();

                Pawn->OnEnteredAircraft.ProcessDelegate();
            }

            Player->ClientActivateSlot(EFortQuickBars::Primary, 0, 0.f, true, true);

            if (Player->Pawn)
                Player->Pawn->K2_DestroyActor();

            ((void(*)(AFortPlayerControllerAthena*))Helios::Offsets::Reset)((AFortPlayerControllerAthena*)Player);
        }

        GamePhaseLogic->GamePhase = EAthenaGamePhase::Aircraft;
        GamePhaseLogic->OnRep_GamePhase(EAthenaGamePhase::Warmup);

        GamePhaseLogic->GamePhaseStep = EAthenaGamePhaseStep::BusLocked;
        GamePhaseLogic->HandleGamePhaseStepChanged(EAthenaGamePhaseStep::BusLocked);

        bAircraftStarted = true;
    }

    static bool bAircraftUnlocked = false;
    if (!bAircraftUnlocked && GamePhaseLogic->GamePhase == EAthenaGamePhase::Aircraft && GamePhaseLogic->Aircrafts_GameState.IsValid() && GamePhaseLogic->Aircrafts_GameState[0].Get() && GamePhaseLogic->Aircrafts_GameState[0]->DropStartTime < Time)
    {
        bAircraftUnlocked = true;

        GamePhaseLogic->bAircraftIsLocked = false;

        GamePhaseLogic->GamePhaseStep = EAthenaGamePhaseStep::BusFlying;
        GamePhaseLogic->HandleGamePhaseStepChanged(EAthenaGamePhaseStep::BusFlying);
    }

    static bool bEndedAircraft = false;
    if (!bEndedAircraft && GameMode->AlivePlayers.Num() > 0 && GamePhaseLogic->Aircrafts_GameState[0].Get() && GamePhaseLogic->Aircrafts_GameState[0]->DropEndTime != -1 && GamePhaseLogic->Aircrafts_GameState[0]->DropEndTime < Time)
    {
        bEndedAircraft = true;

        for (auto& Player : UFortKismetLibrary::GetAllFortPlayerControllers(UWorld::GetWorld(), true, false))
        {
            if (!Player)
                continue;

            if (Player->IsInAircraft())
                Player->GetAircraftComponent()->ServerAttemptAircraftJump(FRotator{});
        }

        GamePhaseLogic->SafeZonesStartTime = UHeliosConfiguration::bIsLateGame ? (float)Time : (float)Time + 60.f;

        GamePhaseLogic->GamePhase = EAthenaGamePhase::SafeZones;
        GamePhaseLogic->OnRep_GamePhase(EAthenaGamePhase::Aircraft);

        GamePhaseLogic->GamePhaseStep = EAthenaGamePhaseStep::StormForming;
        GamePhaseLogic->HandleGamePhaseStepChanged(EAthenaGamePhaseStep::StormForming);
    }

    static bool bDestroyedAircraft = false;
    if (!bDestroyedAircraft && GamePhaseLogic->Aircrafts_GameState[0].Get() && GamePhaseLogic->Aircrafts_GameState[0]->FlightEndTime != -1 && GamePhaseLogic->Aircrafts_GameState[0]->FlightEndTime < Time)
    {
        bDestroyedAircraft = true;

        GamePhaseLogic->Aircrafts_GameState[0].Get()->K2_DestroyActor();

        GamePhaseLogic->Aircrafts_GameState.Clear();
        GamePhaseLogic->Aircrafts_GameMode.Clear();
    }

    if (bEndedAircraft)
    {
        static bool bStartedInitialZone = false;
        if (bDestroyedAircraft && !bStartedInitialZone)
        {
            if (GameMode->AlivePlayers.Num() > 0 && GamePhaseLogic->SafeZonesStartTime != -1 && GamePhaseLogic->SafeZonesStartTime < Time)
            {
                bStartedInitialZone = true;

                if (!UHeliosConfiguration::bIsLateGame)
                {
                    SpawnSafeZoneIndicator(GamePhaseLogic);
                    StartNewSafeZonePhase(GamePhaseLogic, 1);
                }
            }
        }

        static bool bUpdatedPhase = false;
        if (bStartedInitialZone && GamePhaseLogic->SafeZoneIndicator)
        {
            if (GamePhaseLogic->SafeZoneIndicator->SafeZonePhases.IsValidIndex(GamePhaseLogic->SafeZoneIndicator->CurrentPhase))
            {
                if (!bUpdatedPhase && GamePhaseLogic->SafeZoneIndicator->SafeZoneStartShrinkTime < Time)
                {
                    bUpdatedPhase = true;

                    EFortSafeZoneState& SafeZoneState = *(EFortSafeZoneState*)(__int64(&GamePhaseLogic->SafeZoneIndicator->FutureReplicator) - 0x4);
                    SafeZoneState = EFortSafeZoneState::Shrinking;

                    GamePhaseLogic->SafeZoneIndicator->OnSafeZoneStateChange(EFortSafeZoneState::Shrinking, false);
                    GamePhaseLogic->SafeZoneIndicator->SafezoneStateChangedDelegate.ProcessDelegate(GamePhaseLogic->SafeZoneIndicator, EFortSafeZoneState::Shrinking);

                    GamePhaseLogic->GamePhaseStep = EAthenaGamePhaseStep::StormShrinking;
                    GamePhaseLogic->HandleGamePhaseStepChanged(EAthenaGamePhaseStep::StormShrinking);
                }
                else if (GamePhaseLogic->SafeZoneIndicator->SafeZoneFinishShrinkTime < Time)
                {
                    if (GamePhaseLogic->SafeZoneIndicator->SafeZonePhases.IsValidIndex(GamePhaseLogic->SafeZoneIndicator->CurrentPhase + 1))
                    {
                        bUpdatedPhase = false;
                        StartNewSafeZonePhase(GamePhaseLogic, GamePhaseLogic->SafeZoneIndicator->CurrentPhase + 1);
                    }
                }
            }

            for (auto& Player : UFortKismetLibrary::GetAllFortPlayerControllers(UWorld::GetWorld(), true, false))
            {
                if (!Player || !Player->MyFortPawn)
                    continue;

                if (AFortPlayerPawn* Pawn = Player->MyFortPawn)
                {
                    bool bIsInCurrentSafeZone = GamePhaseLogic->IsInCurrentSafeZone(Player->MyFortPawn->K2_GetActorLocation(), false);

                    if (Pawn->bIsInsideSafeZone != bIsInCurrentSafeZone || Pawn->bIsInAnyStorm != !bIsInCurrentSafeZone)
                    {
                        Pawn->bIsInAnyStorm = !bIsInCurrentSafeZone;
                        Pawn->OnRep_IsInAnyStorm();

                        Pawn->bIsInsideSafeZone = bIsInCurrentSafeZone;
                        Pawn->OnRep_IsInsideSafeZone();
                    }
                }
            }
        }
    }
}