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

	/** Spray over the bow: when she drives her head down into a sea, a cloud of it is thrown up at the
	 *  stem, carried aft and to leeward across the forecastle, and falls away. Each is a small volume of
	 *  real mist (M_FogVolume), so it is lit like the fog and the lanterns shine in it. */
	struct FBowSpray { TObjectPtr<class UStaticMeshComponent> Mesh; TObjectPtr<UMaterialInstanceDynamic> MID; float Age = -1.f; float Strength = 0.f; float Side = 0.f; };
	FBowSpray BowSprays[3];
	UPROPERTY() TArray<TObjectPtr<UObject>> BowSprayRefs;
	void TickBowSpray(float DeltaSeconds);
	float LastBowHeight = 0.f;
	float BowSprayCooldown = 0.f;
};
