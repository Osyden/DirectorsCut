#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCTypes.h"
#include "DCGunnery.generated.h"

class UInstancedStaticMeshComponent;
class ADCTreadmill;

/**
 * The ship's guns. Player and crew both fire through here, so a broadside looks and behaves
 * the same whoever orders it: staggered shots, ballistic arcs, splash or hit at the far end.
 */
UCLASS()
class ADCGunnery : public AActor
{
	GENERATED_BODY()

public:
	ADCGunnery();
	virtual void Tick(float DeltaSeconds) override;

	static ADCGunnery* Get(const UObject* WorldContext);

	/** Reads gun positions from the ship's components. */
	void Init(AActor* Ship, float InSeaLevelZ);

	/** Fire up to MaxGuns at a world position. Returns false while that side is reloading. */
	bool FireBroadside(EDCSide Side, const FVector& Target, int32 MaxGuns);
	bool IsReady(EDCSide Side) const { return ReloadLeft[Side == EDCSide::Port ? 0 : 1] <= 0.f; }
	float GetReloadFraction(EDCSide Side) const { return FMath::Clamp(ReloadLeft[Side == EDCSide::Port ? 0 : 1] / ReloadTime, 0.f, 1.f); }
	int32 NumGuns(EDCSide Side) const { return Muzzles[Side == EDCSide::Port ? 0 : 1].Num(); }

	UPROPERTY(EditAnywhere) float ReloadTime = 3.f;
	UPROPERTY(EditAnywhere) float ShotSpeed = 7000.f;
	UPROPERTY(EditAnywhere) float Gravity = 1400.f;
	UPROPERTY(EditAnywhere) float HitRadius = 700.f;

protected:
	struct FPendingShot { FVector Muzzle; FVector Target; float Delay; };
	struct FShot { FVector Pos; FVector Vel; float Life; };

	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> Balls;
	UPROPERTY() TObjectPtr<UClass> MuzzleEffectClass;
	UPROPERTY() TObjectPtr<UClass> ImpactEffectClass;

	TArray<FVector> Muzzles[2];
	TArray<FPendingShot> Pending;
	TArray<FShot> Shots;
	float ReloadLeft[2] = { 0.f, 0.f };
	float SeaLevelZ = 0.f;
};
