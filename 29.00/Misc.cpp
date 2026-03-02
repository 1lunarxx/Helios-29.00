#include "pch.h"
#include "Misc.h"
#include "GamePhaseLogic.h"
#include "Options.h"
#include "Offsets.h"
#include "Helios.h"

enum class EReplicationSystemSendPass : unsigned
{
	Invalid,
	PostTickDispatch,
	TickFlush,
};

struct FSendUpdateParams {
	EReplicationSystemSendPass SendPass = EReplicationSystemSendPass::TickFlush;
	float DeltaSeconds = 0.f;
};

void Misc::SendClientMoveAdjustments(UNetDriver* Driver)
{
	static auto SendClientAdjustment = (void(*)(APlayerController*))(Helios::Offsets::SendClientAdjustment);

	for (UNetConnection* Connection : Driver->ClientConnections)
	{
		if (Connection == nullptr || Connection->ViewTarget == nullptr)
			continue;

		if (APlayerController* PC = Connection->PlayerController)
			SendClientAdjustment(PC);

		for (UNetConnection* ChildConnection : Connection->Children)
		{
			if (ChildConnection == nullptr)
				continue;

			if (APlayerController* PC = ChildConnection->PlayerController)
				SendClientAdjustment(PC);
		}
	}
}

void Misc::TickFlush(UNetDriver* Driver, float DeltaSeconds)
{
	GamePhaseLogic::Tick();

	if (Driver->ClientConnections.Num() > 0)
	{
		UReplicationSystem* ReplicationSystem = *(UReplicationSystem**)(__int64(&Driver->ReplicationDriver) + 0x8);

		if (ReplicationSystem)
		{
			static void(*UpdateIrisReplicationViews)(UNetDriver*) = decltype(UpdateIrisReplicationViews)(Helios::Offsets::UpdateIrisReplicationViews);
			static void(*PreSendUpdate)(UReplicationSystem*, FSendUpdateParams&) = decltype(PreSendUpdate)(Helios::Offsets::PreSendUpdate);

			UpdateIrisReplicationViews(Driver);
			SendClientMoveAdjustments(Driver);

			FSendUpdateParams Params;
			Params.DeltaSeconds = DeltaSeconds;
			PreSendUpdate(ReplicationSystem, Params);
		}
	}

	if (UHeliosConfiguration::bIsProd)
	{
		static bool bJoined = false;

		if (Driver->ClientConnections.Num() > 0)
		{
			bJoined = true;
		}

		if (Driver->ClientConnections.Num() == 0)
		{
			if (bJoined)
			{
				TerminateProcess(GetCurrentProcess(), 1);
			}
		}
	}

	return Originals::TickFlush(Driver, DeltaSeconds);
}

void* Misc::SendRequestNow(void* a1, void* a2, int)
{
	return Originals::SendRequestNow(a1, a2, 3);
}

bool Misc::Listen()
{
	UWorld* World = UWorld::GetWorld();
	UNetDriver* (*CreateNamedNetDriver)(UEngine*, void*, FName, int) = decltype(CreateNamedNetDriver)(Helios::Offsets::CreateNetDriver);
	__int64* (*GetWorldContext)(UEngine*, UWorld*) = decltype(GetWorldContext)(Helios::Offsets::GetWorldContext);

	if (World->NetDriver)
	{
		return false;
	}

	FName NetDriverDefinition = UKismetStringLibrary::Conv_StringToName(L"GameNetDriver");
	if (UNetDriver* NetDriver = CreateNamedNetDriver(UEngine::GetEngine(), GetWorldContext(UEngine::GetEngine(), World), NetDriverDefinition, 0))
	{
		World->NetDriver = NetDriver;

		World->NetDriver->World = World;
		World->NetDriver->NetDriverName = NetDriverDefinition;
		World->NetDriver->NetServerMaxTickRate = Misc::GetMaxTickRate();

		for (FLevelCollection& LevelCollection : World->LevelCollections)
		{
			LevelCollection.NetDriver = World->NetDriver;
		}
	}

	if (World->NetDriver == nullptr)
	{
		return false;
	}

	void (*SetWorld)(UNetDriver*, UWorld*) = decltype(SetWorld)(Helios::Offsets::SetWorld);
	bool (*InitListen)(UNetDriver*, UWorld*, FURL, bool, FString) = decltype(InitListen)(Helios::Offsets::InitListen);

	FString Error;

	if (UHeliosConfiguration::bIsProd)
	{
		UHeliosConfiguration::Port = UKismetMathLibrary::RandomIntegerInRange(7777, 8888);
		Helios::Sessions::CreateSeleniumSession();
	}

	FURL URL;
	URL.Port = UHeliosConfiguration::Port;

	SetWorld(World->NetDriver, World);
	if (!InitListen(World->NetDriver, World, URL, false, Error))
	{
		World->NetDriver = nullptr;

		for (FLevelCollection& LevelCollection : World->LevelCollections)
		{
			LevelCollection.NetDriver = nullptr;
		}

		return false;
	}

	SetWorld(World->NetDriver, World);
	return true;
}

__int64 Misc::PhysicsAssetCheck(__int64 a1, __int64 a2)
{
	if (!a1)
		return 0;

	return Originals::PhysicsAssetCheck(a1, a2);
}

uint32 Misc::CheckCheckpointHeartBeat()
{
	return -1;
}

float Misc::GetMaxTickRate() {
	return  (float)UHeliosConfiguration::bIsLateGame ? 120 : 60;
}

void Misc::CrashReporter(unsigned int* a1, wchar_t* a2)
{
	CreateThread(nullptr, 0, [](LPVOID) -> DWORD { Sleep(3000); TerminateProcess(GetCurrentProcess(), 1); return 0; }, nullptr, 0, nullptr);
	Originals::CrashReporter(a1, a2);
}

void Misc::Patch()
{
	Runtime::Hook(EHook::Hook, Helios::Offsets::TickFlush, TickFlush, (void**)&Originals::TickFlush);
	Runtime::Hook(EHook::Hook, Helios::Offsets::SendRequestNow, SendRequestNow, (void**)&Originals::SendRequestNow);
	Runtime::Hook(EHook::Hook, Helios::Offsets::GetMaxTickRate, GetMaxTickRate);
	Runtime::Hook(EHook::Hook, Helios::Offsets::CheckCheckpointHeartBeat, CheckCheckpointHeartBeat);
	Runtime::Hook(EHook::Hook, Helios::Offsets::PhysicsAssetCheck, PhysicsAssetCheck, (void**)&Originals::PhysicsAssetCheck);

	for (auto& NullFunc : Helios::Offsets::NullFuncs)
	{
		Runtime::Hook(EHook::Null, NullFunc);
	}

	for (auto& RetTrueFunc : Helios::Offsets::RetTrueFuncs)
	{
		Runtime::Hook(EHook::RetTrue, RetTrueFunc);
	}

	Runtime::Hook(EHook::RetTrue, Helios::Offsets::AttemptDeriveFromURL);
	Runtime::Hook(EHook::RetTrue, Helios::Offsets::WorldNetMode);

	Runtime::PatchNetModes(Helios::Offsets::AttemptDeriveFromURL);

	Runtime::Hook<AFortTeamMemberPedestal>(EHook::Virtual, 0x72, AActor::GetDefaultObj()->VTable[0x72]);
	Runtime::Patch<uint8_t>(Helios::Offsets::SpawnServerActorPatch, 0x1);
}