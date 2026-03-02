#pragma once
#include "pch.h"

namespace Abilities
{
    void InternalServerTryActivateAbility(UAbilitySystemComponent*, FGameplayAbilitySpecHandle, bool, FPredictionKey&, FGameplayEventData*);
    void UpdateActiveGameplayEffectSetByCallerMagnitude(AFortPlayerControllerAthena*, UClass*, FGameplayTag);

    void Patch();
}