#pragma once
#include "pch.h"
#include "Runtime.h"

namespace Misc
{
	namespace Originals
	{
		static void(*TickFlush)(UNetDriver*, float);
		static void* (*SendRequestNow)(void*, void*, int);
		static void (*CrashReporter)(unsigned int*, wchar_t*);

		static __int64(*PhysicsAssetCheck)(__int64, __int64);
	}

	void* SendRequestNow(void*, void*, int);
	void SendClientMoveAdjustments(UNetDriver*);
	void TickFlush(UNetDriver*, float);

	bool Listen();
	__int64 PhysicsAssetCheck(__int64, __int64);
	uint32 CheckCheckpointHeartBeat();

	float GetMaxTickRate();
	void CrashReporter(unsigned int*, wchar_t*);

	void Patch();
}