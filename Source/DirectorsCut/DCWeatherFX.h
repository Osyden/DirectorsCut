#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCWeatherFX.generated.h"

class UProceduralMeshComponent;
class UMaterialInstanceDynamic;
class ADCRideDirector;

/**
 * Rain and embers without a particle system: each is one mesh of thousands of quads that the
 * M_Rain material animates and wraps around the camera. Amounts follow the ride director.
 */
UCLASS()
class ADCWeatherFX : public AActor
{
	GENERATED_BODY()

public:
	ADCWeatherFX();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	UProceduralMeshComponent* BuildStreaks(const TCHAR* Name, int32 Count);

	UPROPERTY() TObjectPtr<UProceduralMeshComponent> Rain;
	UPROPERTY() TObjectPtr<UProceduralMeshComponent> Embers;
	UPROPERTY() TObjectPtr<UProceduralMeshComponent> Spray;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SprayMID;
	/** Rain striking the deck: small quads lying on every upward-facing surface of the ship (M_Splash). */
	UPROPERTY() TObjectPtr<UProceduralMeshComponent> Splashes;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SplashMID;
	bool bSplashesBuilt = false;
	void BuildSplashes();
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> RainMID;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> EmberMID;
	UPROPERTY() TObjectPtr<ADCRideDirector> Director;
	float FlashBoost = 0.f;
};
