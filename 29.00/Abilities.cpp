#include "pch.h"
#include "Abilities.h"
#include "Runtime.h"

void Abilities::InternalServerTryActivateAbility(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpecHandle Handle, bool, FPredictionKey& PredictionKey, FGameplayEventData* TriggerEventData)
{
	FGameplayAbilitySpec* Spec = nullptr;

	for (auto& Item : AbilitySystemComponent->ActivatableAbilities.Items)
	{
		if (Item.Handle.Handle == Handle.Handle)
		{
			Spec = &Item;
			break;
		}
	}

	if (!Spec)
	{
		return AbilitySystemComponent->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
	}

	UGameplayAbility* AbilityToActivate = Spec->Ability;
	if (!AbilityToActivate)
	{
		return AbilitySystemComponent->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
	}

	UGameplayAbility* InstancedAbility = nullptr;
	Spec->InputPressed = true;

	auto InternalTryActivateAbility = (bool (*)(UAbilitySystemComponent*, FGameplayAbilitySpecHandle, FPredictionKey, UGameplayAbility**, void*, const FGameplayEventData*)) (ImageBase + 0x748CB28);
	if (!InternalTryActivateAbility(AbilitySystemComponent, Handle, PredictionKey, &InstancedAbility, nullptr, TriggerEventData))
	{
		AbilitySystemComponent->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
		Spec->InputPressed = false;
	}

	AbilitySystemComponent->ActivatableAbilities.MarkItemDirty(*Spec);
}

void Abilities::UpdateActiveGameplayEffectSetByCallerMagnitude(AFortPlayerControllerAthena* PlayerController, UClass* Ability, FGameplayTag Tag)
{
    if (!PlayerController)
        return;

    AFortPlayerStateAthena* PlayerState = static_cast<AFortPlayerStateAthena*>(PlayerController->PlayerState);
    UFortAbilitySystemComponent* AbilitySystemComponent = PlayerState->AbilitySystemComponent;

    if (PlayerState && AbilitySystemComponent)
    {
        for (auto& Effect : AbilitySystemComponent->ActiveGameplayEffects.GameplayEffects_Internal)
        {
            if (Effect.Spec.Def)
            {
                if (!Effect.Spec.Def->IsA(Ability))
                {
                    FGameplayEffectContextHandle EffectContextHandle{};
                    AbilitySystemComponent->UpdateActiveGameplayEffectSetByCallerMagnitude(AbilitySystemComponent->BP_ApplyGameplayEffectToSelf(Ability, 0.f, EffectContextHandle), Tag, 1.f);

                    break;
                }
            }
        }
    }
}

void Abilities::Patch()
{
    Runtime::Hook<UAbilitySystemComponent>(EHook::Virtual, Helios::Offsets::VFTs::InternalServerTryActivateAbility, InternalServerTryActivateAbility);
    Runtime::Hook<UFortAbilitySystemComponent>(EHook::Virtual, Helios::Offsets::VFTs::InternalServerTryActivateAbility, InternalServerTryActivateAbility);
    Runtime::Hook<UFortAbilitySystemComponentAthena>(EHook::Virtual, Helios::Offsets::VFTs::InternalServerTryActivateAbility, InternalServerTryActivateAbility);
}