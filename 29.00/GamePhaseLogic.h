#pragma once
#include "pch.h"
#include "Runtime.h"

namespace GamePhaseLogic
{
	struct FStormCircle
	{
		FVector Center;
		float Radius;
	};

	static std::vector<FStormCircle> StormCircles;

	static FVector ClampToPlayableBounds(const FVector& Candidate, float Radius, const FBoxSphereBounds& Bounds)
	{
		FVector Clamped = Candidate;

		Clamped.X = std::clamp(Clamped.X, Bounds.Origin.X - Bounds.BoxExtent.X + Radius, Bounds.Origin.X + Bounds.BoxExtent.X - Radius);
		Clamped.Y = std::clamp(Clamped.Y, Bounds.Origin.Y - Bounds.BoxExtent.Y + Radius, Bounds.Origin.Y + Bounds.BoxExtent.Y - Radius);

		return Clamped;
	}

	static inline FVector GetSafeNormal(FVector Location)
	{
		const double sizeSquare = Location.X * Location.X + Location.Y * Location.Y + Location.Z * Location.Z;

		if (sizeSquare > 1.e-4f)
		{
			double inv = 1. / sqrt(sizeSquare);
			return FVector(Location.X * inv, Location.Y * inv, Location.Z * inv);
		}

		return FVector(0.f, 0.f, 0.f);
	}

	void SpawnSafeZoneIndicator(UFortGameStateComponent_BattleRoyaleGamePhaseLogic*);
	void StartNewSafeZonePhase(UFortGameStateComponent_BattleRoyaleGamePhaseLogic*, int);

	void GenerateStormCircles(AFortAthenaMapInfo*);
	void Tick();
}