#pragma once

#include "CoreMinimal.h"

class ADCRideDirector;
class UPrimitiveComponent;

/** Small, ship-local graph sampled from the visible upper deck. Never navigates the hold. */
struct FDCCrewNavigation
{
	struct FNode { FVector Feet; TArray<int32> Links; };
	TArray<FNode> Nodes;

	bool Build(ADCRideDirector* Director);
	bool FindRoute(const FVector& FromFeet, const FVector& ToFeet, TArray<FVector>& Route) const;
	bool Project(const FVector& Feet, FVector& OnDeck, float MaxDistance = 300.f) const;
	bool Ground(ADCRideDirector* Director, FVector& Feet) const;
	/** Graph-only entry point, also exercised by automation without a world or asset load. */
	bool FindNodeRoute(int32 Start, int32 Goal, TArray<int32>& Route) const;

private:
	int32 Closest(const FVector& Feet, float MaxDistance) const;
	TWeakObjectPtr<UPrimitiveComponent> Hull;
};
