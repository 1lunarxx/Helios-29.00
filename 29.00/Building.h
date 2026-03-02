#pragma once
#include "pch.h"
#include "Runtime.h"

namespace Building
{
	namespace Originals
	{
		static void (*ServerCreateBuildingActor)(AFortPlayerControllerAthena*, FCreateBuildingActorData);
		static void (*OnDamageServer)(ABuildingSMActor*, float, FGameplayTagContainer, FVector, FHitResult, AFortPlayerControllerAthena*, AActor*, FGameplayEffectContextHandle);
	}

	bool CanBePlacedByPlayer(UClass*);
	void SetEditingPlayer(ABuildingSMActor*, AFortPlayerStateAthena*);

	void OnDamageServer(ABuildingSMActor*, float, FGameplayTagContainer, FVector, FHitResult, AFortPlayerControllerAthena*, AActor*, FGameplayEffectContextHandle);
	void ServerCreateBuildingActor(AFortPlayerControllerAthena*, FCreateBuildingActorData);

	void ServerBeginEditingBuildingActor(UObject*, FFrame&);
	void ServerEditBuildingActor(UObject*, FFrame&);
	void ServerEndEditingBuildingActor(UObject*, FFrame&);

	void Patch();
}