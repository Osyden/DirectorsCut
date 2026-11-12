#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCFogFront.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class ADCRideDirector;

/**
 * The cloud front the ship sails into at the end of the open-sea phase: a wall of billows from the
 * water to far above the masts, kilometres wide, dark at the foot and sunlit on top, lit from
 * inside by lightning. It closes on the ship and swallows it as the fog phase begins.
 */
UCLASS()
class ADCFogFront : public AActor
{
	GENERATED_BODY()

public:
	ADCFogFront();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Ride time at which the front reaches the bow. */
	UPROPERTY(EditAnywhere) float ArrivalTime = 60.f;
	/** How far off it stands at the start of the ride, cm. */
	UPROPERTY(EditAnywhere) float StartDistance = 400000.f;

protected:
	struct FBillow
	{
		TWeakObjectPtr<UStaticMeshComponent> Component;
		FVector Offset;
		float Size;
		float Seed;
	};
	TArray<FBillow> Billows;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CloudMID;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> CloudShades;
	UPROPERTY() TObjectPtr<ADCRideDirector> Director;
	float Flash = 0.f;
	float NextFlashIn = 3.f;
};
