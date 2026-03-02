#pragma once
#include "pch.h"

namespace Helios {
	namespace Offsets {
		static inline uint64_t GIsClient = ImageBase + 0x10AF70FB;
		static inline uint64_t GIsServer = ImageBase + 0x10AF708F;
		static inline uint64_t ReadyToStartMatch = ImageBase + 0x8510360;
		static inline uint64_t SpawnDefaultPawnFor = ImageBase + 0x851315C;
		static inline uint64_t HandleStartingNewPlayer = ImageBase + 0x8CA6DA4;
		static inline uint64_t HandleMatchHasStarted = ImageBase + 0x850AC34;
		static inline uint64_t InitNewPlayer = ImageBase + 0x1FBFC3C;
		static inline uint64_t TickFlush = ImageBase + 0x1737C20;
		static inline uint64_t SendRequestNow = ImageBase + 0x20D40B8;
		static inline uint64_t GetMaxTickRate = ImageBase + 0x128FDC0;
		static inline uint64_t CheckCheckpointHeartBeat = ImageBase + 0x4A5D760;
		static inline uint64_t WorldNetMode = ImageBase + 0x697B1B0;
		static inline uint64_t PhysicsAssetCheck = ImageBase + 0x56E2240;
		static inline uint64_t SpawnServerActorPatch = ImageBase + 0x2004F9F;
		static inline uint64_t AttemptDeriveFromURL = ImageBase + 0x16CF294;
		static inline uint64_t StaticFindObject = ImageBase + 0x13FBDEC;
		static inline uint64_t StaticLoadObject = ImageBase + 0x21F6A68;
		static inline uint64_t ServerAcknowledgePossession = ImageBase + 0x859733C;
		static inline uint64_t ClientOnPawnDied = ImageBase + 0x9322144;
		static inline uint64_t GetPlayerViewPoint = ImageBase + 0x857B54C;
		static inline uint64_t FinishedTargetSpline = ImageBase + 0x31AFAF4;
		static inline uint64_t GetInterface = ImageBase + 0x1241BB0;
		static inline uint64_t StepCompiledInCode = ImageBase + 0x12443B8;
		static inline uint64_t StepCompiledIn = ImageBase + 0x1917B68;
		static inline uint64_t OnDamageServer = ImageBase + 0x88097C0;
		static inline uint64_t Reset = ImageBase + 0x6B70DC8;
		static inline uint64_t RemoveInventoryItem = ImageBase + 0x92EDED4;
		static inline uint64_t InitializePlayerGameplayAbilities = ImageBase + 0x8CCEBC0;
		static inline uint64_t RemoveFromAlivePlayers = ImageBase + 0x8510BEC;
		static inline uint64_t ConstructAbilitySpec = ImageBase + 0x74C87AC;
		static inline uint64_t GiveAbilityAndActivateOnce = ImageBase + 0x748BDD4;
		static inline uint64_t OnRep_ZiplineState = ImageBase + 0x92492EC;
		static inline uint64_t ReplaceBuildingActor = ImageBase + 0x8878968;
		static inline uint64_t CanAffordToPlaceBuildableClass = ImageBase + 0x8CEA384;
		static inline uint64_t LoadPlayset = ImageBase + 0x9360348;
		static inline uint64_t SendClientAdjustment = ImageBase + 0x6B71AE0;
		static inline uint64_t UpdateIrisReplicationViews = ImageBase + 0x6AB4A90;
		static inline uint64_t PreSendUpdate = ImageBase + 0x5E84304;
		static inline uint64_t InitializeFlightPath = ImageBase + 0x84A9434;
		static inline uint64_t GetWorldContext = ImageBase + 0x126B314;
		static inline uint64_t CreateNetDriver = ImageBase + 0x2080EA0;
		static inline uint64_t SetWorld = ImageBase + 0x22C24D8;
		static inline uint64_t InitListen = ImageBase + 0x6D7889C;
		static inline uint64_t SetIsDoorOpen = ImageBase + 0x8899540;
		static inline uint64_t NotifyGameMemberAdded = ImageBase + 0x21DAB8C;

		static inline std::vector<uint64_t> NullFuncs{ ImageBase + 0x2A15684, ImageBase + 0x2A15C40, ImageBase + 0xA95A8BC };
		static inline std::vector<uint64_t> RetTrueFuncs{ ImageBase + 0x85843E8, ImageBase + 0x1FC3A4C };

		namespace VFTs {
			static inline uint32_t ServerCreateBuildingActor = 0x24F;
			static inline uint32_t ServerRepairBuildingActor = 0x24B;
			static inline uint32_t ServerClientIsReadyToRespawn = 0x57B;
			static inline uint32_t ServerLoadingScreenDropped = 0x282;
			static inline uint32_t ServerAttemptInventoryDrop = 0x23A;
			static inline uint32_t ServerExecuteInventoryItem = 0x22F;
			static inline uint32_t ServerGiveCreativeItem = 0x50C;
			static inline uint32_t ServerAttemptAircraftJump = 0xA6;
			static inline uint32_t ServerSendZiplineState = 0x250;
			static inline uint32_t InternalServerTryActivateAbility = 0x11A;
			static inline uint32_t ServerTeleportToPlaygroundLobbyIsland = 0x56B;
			static inline uint32_t SetLoadedAmmo = 0xBC;
		}
	}
}