#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCTypes.h"
#include "DCKraken.generated.h"

class UProceduralMeshComponent;
class UMaterialInstanceDynamic;
class ADCRideDirector;
class ADCTreadmill;

UENUM()
enum class EDCTentacleState : uint8 { Hidden, Rising, Sway, Windup, Strike, Hold, Recoil, Dying };

/** One arm of the kraken. Entirely procedural: a spine curve rebuilt every frame and skinned with a tube. */
struct FDCTentacle
{
	EDCTentacleState State = EDCTentacleState::Hidden;
	float StateTime = 0.f;
	float StateDuration = 1.f;

	FVector Base = FVector::ZeroVector;
	float Length = 3000.f;
	float BaseRadius = 240.f;
	/** 0 = fully under water, 1 = fully out. */
	float Rise = 0.f;
	float Seed = 0.f;

	/** The tip is a damped spring chasing TipTarget, which gives overshoot and follow-through for free. */
	FVector Tip = FVector::ZeroVector;
	FVector TipVel = FVector::ZeroVector;
	FVector TipTarget = FVector::ZeroVector;
	FVector StrikePoint = FVector::ZeroVector;
	bool bStruck = false;

	/** Invisible actor the guns aim at; destroyed by the treadmill when the arm has taken enough hits. */
	TWeakObjectPtr<AActor> Proxy;
	bool bHasProxy = false;
};

/**
 * The kraken of the maelstrom. Arms rise from the sea around the ship, sway, wind up and slam the deck.
 * No skeletal mesh and no authored animation: spine curves, a spring per tip and a state machine.
 */
UCLASS()
class ADCKraken : public AActor
{
	GENERATED_BODY()

public:
	ADCKraken();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere) int32 NumTentacles = 6;
	UPROPERTY(EditAnywhere) int32 Rings = 40;
	UPROPERTY(EditAnywhere) int32 Sides = 12;
	UPROPERTY(EditAnywhere) float StrikeDamage = 0.6f;
	UPROPERTY(BlueprintReadOnly) int32 ArmsDestroyed = 0;

protected:
	void SetState(FDCTentacle& T, EDCTentacleState NewState, float Duration);
	void PlaceTentacle(FDCTentacle& T, int32 Index);
	void TickTentacle(FDCTentacle& T, int32 Index, float DeltaSeconds, bool bActive);
	void BuildSpine(const FDCTentacle& T, TArray<FVector>& OutPoints) const;
	void UpdateMesh(const FDCTentacle& T, int32 Index, bool bCreate);

	UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> Mesh;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SkinMID;
	UPROPERTY() TObjectPtr<ADCRideDirector> Director;

	TArray<FDCTentacle> Tentacles;
	TArray<int32> SharedTris;
	float SeaZ = 0.f;
	float Clock = 0.f;
};
