#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCPortal.generated.h"

class UMaterialInstanceDynamic;
class USceneComponent;
class ADCRideDirector;

/**
 * The way out of Atlantis: a luminous tunnel of cloud and spray standing on the sea among the
 * ruins, straight ahead of the ship. Rings of billows turn slowly around a bright core that
 * flares with the lightning. The ship reaches it as the sea itself rolls up into the slide.
 */
UCLASS()
class ADCPortal : public AActor
{
	GENERATED_BODY()

public:
	ADCPortal();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Radius of the mouth, cm. */
	UPROPERTY(EditAnywhere) float Radius = 30000.f;
	/** Also build the old rings of billow meshes. Off: the volumetric cloud layer draws the portal. */
	UPROPERTY(EditAnywhere) bool bBillows = false;

protected:
	UPROPERTY() TArray<TObjectPtr<USceneComponent>> Rings;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> Shades;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CoreMID;
	UPROPERTY() TObjectPtr<ADCRideDirector> Director;
	float Flash = 0.f;
	float Clock = 0.f;
	bool bHooked = false;
};
