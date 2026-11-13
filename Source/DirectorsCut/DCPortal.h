#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCPortal.generated.h"

class UMaterialInstanceDynamic;
class UProceduralMeshComponent;
class ADCRideDirector;

/** A hollow, curling water entrance out of Atlantis, independent of the sky's cloud layer. */
UCLASS()
class ADCPortal : public AActor
{
	GENERATED_BODY()

public:
	ADCPortal();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Radius of the entrance in cm. The low part of the opening intersects the sea. */
	UPROPERTY(EditAnywhere) float Radius = 30000.f;
	/** Kept for existing saved actors. The portal now uses continuous curled sheets. */
	UPROPERTY(EditAnywhere) bool bBillows = false;

protected:
	void BuildMouth();
	void BuildCrests();
	void BuildSpray();

	UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> Mouth;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> Crests;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> Spray;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> WaterMID;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SprayMID;
	UPROPERTY() TObjectPtr<ADCRideDirector> Director;
};
