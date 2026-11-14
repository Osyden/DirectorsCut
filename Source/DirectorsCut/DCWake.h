#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCWake.generated.h"

class UProceduralMeshComponent;
class UMaterialInstanceDynamic;
class ADCRideDirector;

/**
 * Samples the Dynamic Real Water surface for ship motion and bow spray.
 * Spray breaks away at the sampled waterline, moving aft and out as the ship
 * advances. The old flat hull ribbon remains an opt-in diagnostic only.
 */
UCLASS()
class ADCWake : public AActor
{
	GENERATED_BODY()

public:
	ADCWake();
	virtual void Tick(float DeltaSeconds) override;
	/** The plane of the sea under the hull this frame, from the heights sampled along both sides:
	 *  how far the water stands above the flat sea level amidships (cm) and its slope fore-and-aft and athwartships (rise per cm). False until the wake is showing. */
	bool GetSeaUnderHull(float& OutRise, float& OutSlopeX, float& OutSlopeY) const;
	static ADCWake* Get(const UObject* WorldContext);

protected:
	void Build();
	/** Height of the plugin sea at a point (world Z), or the flat sea level when it has no answer. Channel is one per point. */
	float SurfaceAt(const FVector2D& Where, int32 Channel);
	void TickBowSpray(float DeltaSeconds);
	void TickHullFoam(float DeltaSeconds);
	struct FHullFoamMaterial { TWeakObjectPtr<UMaterialInstanceDynamic> Material; float Width = 650.f, Strength = 1.f; };
	TArray<FHullFoamMaterial> HullFoamMaterials;
	float FoamTimer = 0.f;
	UPROPERTY() TObjectPtr<class UParticleSystem> BowSplash;
	struct FBowPulse { TWeakObjectPtr<class UParticleSystemComponent> Effect; float Age = 0.f; FVector Origin; float Side = 1.f; };
	TArray<FBowPulse> BowPulses;
	struct FBowContact { float PreviousDepth = 0.f, Closing = 0.f, Timer = 0.f; bool bValid = false; };
	FBowContact BowContacts[2];
	float BowLogTimer = 0.f;
	TWeakObjectPtr<class UPrimitiveComponent> BowHull;
	float BowSprayX = 2350.f, BowSprayY = 240.f;

	UPROPERTY() TObjectPtr<UProceduralMeshComponent> Mesh;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> MID;
	UPROPERTY() TObjectPtr<ADCRideDirector> Director;
	UPROPERTY() TObjectPtr<UClass> WaterLibrary;
	UPROPERTY() TObjectPtr<UFunction> SurfaceFn;

	/** One cross-section of the ribbon on one side: where its inner and outer edges are, and how strong it is. */
	struct FStation { FVector2D Inner; FVector2D Outer; float Strength; };
	TArray<FStation> Stations;      // port side first, then starboard
	TArray<float> Heights;          // smoothed surface height per station
	TArray<FVector> Verts;
	bool bBuilt = false;
	float Shown = 0.f;
};
