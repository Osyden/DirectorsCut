#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCWake.generated.h"

class UProceduralMeshComponent;
class UMaterialInstanceDynamic;
class ADCRideDirector;

/**
 * White water along the hull and a wake astern, for the plugin sea (Dynamic Real Water).
 * The ship stands still and the sea streams past it, so the wake is a fixed ribbon of foam down
 * each side and out behind the stern whose pattern runs aft (M_Wake). It rides on the plugin's
 * waves: the surface height is asked for at a row of points down each side every frame and the
 * ribbon is laid on those heights.
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
