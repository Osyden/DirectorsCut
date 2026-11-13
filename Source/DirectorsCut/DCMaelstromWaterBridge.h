#pragma once

#include "CoreMinimal.h"

class AActor;
class UMaterialInstanceDynamic;
class UWorld;

/** Read-only binding of an installed DRW ocean's live FFT outputs to our bowl material. */
class FDCMaelstromWaterBridge
{
public:
	// Call after the ocean has ticked. Returns true only when all required live inputs exist.
	bool Update(UWorld* World, UMaterialInstanceDynamic* Material, bool bActive = true);
	// Clears the material's opt-in flag. No DRW actor or simulation setting is changed.
	void Reset(UMaterialInstanceDynamic* Material = nullptr);

private:
	TWeakObjectPtr<UWorld> BoundWorld;
	TWeakObjectPtr<AActor> Grid;
	TWeakObjectPtr<UMaterialInstanceDynamic> LastMaterial;
	double NextScanTime = 0.0;
	bool bWasBound = false;
};
