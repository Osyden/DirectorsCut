#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCWaveCurl.generated.h"

class UProceduralMeshComponent;
class UMaterialInstanceDynamic;
class ADCRideDirector;

/**
 * The walls of water that stand beside the ship in the maelstrom: one great breaking wave on each
 * beam, its face hollow toward the ship and its lip curling over the rail. They rise out of the
 * sea as the tunnel falls away, are tallest abeam and astern, and fall to nothing ahead so the
 * crater and the creature stay in clear view between them.
 */
UCLASS()
class ADCWaveCurl : public AActor
{
	GENERATED_BODY()

public:
	ADCWaveCurl();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Distance of the back of each wall from the ship's centreline, cm. */
	UPROPERTY(EditAnywhere) float Distance = 9500.f;
	/** Height of the starboard and port walls, cm. */
	UPROPERTY(EditAnywhere) float HeightStarboard = 11000.f;
	UPROPERTY(EditAnywhere) float HeightPort = 7500.f;
	/** How far the hollow of the face reaches in toward the ship, cm. */
	UPROPERTY(EditAnywhere) float Hollow = 3600.f;
	/** The walls run from this far astern to this far ahead; they taper to nothing at both ends. */
	UPROPERTY(EditAnywhere) float Astern = 30000.f;
	UPROPERTY(EditAnywhere) float Ahead = 15000.f;

protected:
	void BuildSide(int32 Index, float Sign, float Height);
	void Spray(int32 Index, const FVector& Location, float Size);

	UPROPERTY() TObjectPtr<UProceduralMeshComponent> Walls[2];
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> MID;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SprayMID;
	UPROPERTY() TObjectPtr<ADCRideDirector> Director;
	float Rise = 0.f;
};
