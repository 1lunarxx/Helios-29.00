#include "pch.h"
#include "XP.h"
#include "Runtime.h"

void XP::GiveAccolade(AFortPlayerControllerAthena* Controller, UFortAccoladeItemDefinition* Accolade)
{
    if (!Controller || !Accolade)
        return;

    float EventXpValueVal = 0;
    TMap<FName, uint8*>& CurveRowMap = *reinterpret_cast<TMap<FName, uint8*>*>(__int64(Accolade->XpRewardAmount.Curve.CurveTable) + 0x30);
    FName TargetRowName = Accolade->XpRewardAmount.Curve.RowName;

    for (auto& Pair : CurveRowMap)
    {
        if (Pair.Key() == TargetRowName)
        {
            FSimpleCurve* Curve = (FSimpleCurve*)Pair.Value();
            EventXpValueVal = Curve->Keys[0].Value;
            break;
        }
    }

    float EventXpValue = EventXpValueVal * 6.2; // proper
    FXPEventInfo Info{};

    Info.Accolade = UKismetSystemLibrary::GetPrimaryAssetIdFromObject(Accolade);
    Info.bWasNonQuestReactionGrant = true;
    Info.EventName = Accolade->Name;
    Info.EventXpValue = (uint32)EventXpValue;
    Info.PlayerController = Controller;
    Info.Priority = Accolade->Priority;
    Info.RestedValuePortion = Info.EventXpValue;
    Info.RestedXPRemaining = Info.EventXpValue;
    Info.SimulatedName = Accolade->ItemName;
    Info.SimulatedText = Accolade->ItemDescription;
    Info.SeasonBoostValuePortion = 0;
    Info.TotalXpEarnedInMatch = Info.EventXpValue + Controller->XPComponent->TotalXpEarned;

    Controller->XPComponent->MatchXp += Info.EventXpValue;
    Controller->XPComponent->TotalXpEarned += Info.EventXpValue;
    Controller->XPComponent->OnXPEvent(Info);
}

void XP::SendStatEvent(AFortPlayerControllerAthena* Controller, EFortQuestObjectiveStatEvent StatEvent)
{
    switch (StatEvent)
    {
        case EFortQuestObjectiveStatEvent::Kill:
        	XP::GiveAccolade(Controller, Runtime::StaticFindObject<UFortAccoladeItemDefinition>("/BattlePassPermanentQuests/Items/Accolades/AccoladeId_012_Elimination.AccoladeId_012_Elimination"));
        break;
        case EFortQuestObjectiveStatEvent::StormPhase:
            XP::GiveAccolade(Controller, Runtime::StaticFindObject<UFortAccoladeItemDefinition>("/BattlePassPermanentQuests/Items/Accolades/AccoladeID_SurviveStormCircle_33a.AccoladeID_SurviveStormCircle_33a"));
        break;
    }
}