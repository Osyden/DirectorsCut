#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCDeckFireEffect.generated.h"

class UProceduralMeshComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;

/** Visuals only. DCTroubles remains responsible for damage, lifetime and crew interaction. */
UCLASS()
class ADCDeckFireEffect : public AActor
{
	GENERATED_BODY()
public:
	ADCDeckFireEffect();
	void Initialize(float Seed);
	void SetStrength(float Value) { Strength = Value; }
	void ReactToSwat();
	void Extinguish();
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY() TObjectPtr<UProceduralMeshComponent> Flames;
	UPROPERTY() TObjectPtr<UProceduralMeshComponent> Smoke;
	UPROPERTY() TObjectPtr<UProceduralMeshComponent> Embers;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> FlameMaterial;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SmokeMaterial;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> EmberMaterial;
	UPROPERTY() TObjectPtr<UPointLightComponent> Light;
	float Clock = 0.f;
	float SeedValue = 0.f;
	float Strength = 0.25f;
	float StopTime = -1.f;
	float SwatTime = -10.f;
	bool bSwatPending = false;
};
