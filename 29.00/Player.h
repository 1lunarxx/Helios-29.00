#pragma once
#include "pch.h"
#include "Runtime.h"

namespace Player
{
	namespace Originals
	{
		static void (*ClientOnPawnDied)(AFortPlayerControllerAthena*, FFortPlayerDeathReport&);
		static void (*GetPlayerViewPoint)(AFortPlayerControllerAthena*, FVector&, FRotator&);
		static bool (*FinishedTargetSpline)(AFortPickup*);
		static void (*ServerNotifyPawnHit)(UObject*, FFrame&);
		static void (*OpenActor)(UObject*, FFrame&);
		static void (*CloseActor)(UObject*, FFrame&);
		static void (*ServerLoadingScreenDropped)(UObject*, FFrame&);
		static void (*ServerClientIsReadyToRespawn)(AFortPlayerControllerAthena*);
	}

	void ServerAcknowledgePossession(AFortPlayerControllerAthena*, APawn*);
	void ServerExecuteInventoryItem(AFortPlayerControllerAthena*, FGuid&);
	void ServerAttemptAircraftJump(UFortControllerComponent_Aircraft*, FRotator&);

	void ServerAttemptInventoryDrop(AFortPlayerControllerAthena*, FGuid, int32, bool);
	void GetPlayerViewPoint(AFortPlayerControllerAthena*, FVector&, FRotator&);
	bool FinishedTargetSpline(AFortPickup*);
	void ServerSendZiplineState(AFortPlayerPawnAthena*, FZiplinePawnState&);

	void InternalPickup(AFortPlayerControllerAthena*, FFortItemEntry);
	void ClientOnPawnDied(AFortPlayerControllerAthena*, FFortPlayerDeathReport&);
	void ServerClientIsReadyToRespawn(AFortPlayerControllerAthena*);

	void ServerHandlePickupInfo(UObject*, FFrame&);
	void ServerNotifyPawnHit(UObject*, FFrame&);
	void ServerPlayEmoteItem(UObject*, FFrame&);
	void MovingEmoteStopped(UObject*, FFrame&);

	void OpenActor(UObject*, FFrame&);
	void CloseActor(UObject*, FFrame&);
	void TeleportPlayerPawn(UObject*, FFrame&, bool*);

	void Patch();
}
