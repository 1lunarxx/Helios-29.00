#pragma once
#include "pch.h"

namespace GameMode
{
	static inline uint8_t CurrentTeam = 3;

	namespace Originals
	{
		static void (*HandleMatchHasStarted)(AFortGameModeAthena*);
		static void (*HandleStartingNewPlayer)(AFortGameModeAthena*, AFortPlayerControllerAthena*);
	}

	bool ReadyToStartMatch(AFortGameModeAthena*);

	APawn* SpawnDefaultPawnFor(AFortGameModeAthena*, AFortPlayerControllerAthena*, AActor*);
	void HandleStartingNewPlayer(AFortGameModeAthena*, AFortPlayerControllerAthena*);
	void HandleMatchHasStarted(AFortGameModeAthena*);

	void Patch();
}