#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCKrakenHead.generated.h"

class UProceduralMeshComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class ADCRideDirector;

/**
 * The body of the kraken, rising out of the centre of the maelstrom half a mile off: a mantle the
 * size of a cathedral, two lit eyes fixed on the ship, and a ring of arms hundreds of metres tall.
 * It rides on the sea mesh, so it tilts and rocks with the funnel. Scenery only; the arms that
 * reach the ship are in DCKraken.
 */
UCLASS()
class ADCKrakenHead : public AActor
{
	GENERATED_BODY()

public:
	ADCKrakenHead();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Seconds into the maelstrom before the head starts to rise, and how long the rise takes. */
	UPROPERTY(EditAnywhere) float RiseDelay = 25.f;
	UPROPERTY(EditAnywhere) float RiseTime = 35.f;
	UPROPERTY(EditAnywhere) int32 NumArms = 7;

protected:
	void BuildMantle();
	void UpdateArm(int32 Index, bool bCreate);

	UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> Mesh;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Eyes;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> EyeMID;
	UPROPERTY() TObjectPtr<ADCRideDirector> Director;

	TArray<int32> ArmTris;
	TArray<float> ArmSeeds;
	float Raised = 0.f;
	float Clock = 0.f;
	float MaelstromTime = 0.f;
	float EyeFlare = 0.f;
	bool bAttached = false;
};
