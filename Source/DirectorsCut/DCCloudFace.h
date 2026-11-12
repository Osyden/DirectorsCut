#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCCloudFace.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class USceneComponent;
class ADCRideDirector;

/**
 * A face in the clouds: a head hundreds of metres tall modelled out of soft billows (brow, nose,
 * cheeks, hollow eyes, mouth, chin, hair or beard of cloud). Barely there in the dark; each
 * lightning flash lights it from inside for a moment, so the storm is what reveals it.
 */
UCLASS()
class ADCCloudFace : public AActor
{
	GENERATED_BODY()

public:
	ADCCloudFace();
	virtual void Tick(float DeltaSeconds) override;

	/** Build the face. Height is brow-to-chin scale in cm (10000 = a head about 100 m tall). */
	void Build(float Height, bool bBearded, float PitchDown);

protected:
	void Part(const TCHAR* Shape, const FVector& Location, const FRotator& Rotation, const FVector& Scale, UMaterialInterface* Material);

	UPROPERTY() TObjectPtr<USceneComponent> FaceRoot;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Light;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Dark;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Eye;
	UPROPERTY() TObjectPtr<ADCRideDirector> Director;
	/** Also build the old face of billow meshes. */
	UPROPERTY(EditAnywhere) bool bBillows = false;
	float HeadSize = 0.f;
	float BaseZ = 0.f;
	int32 Slot = -1;
	bool bPlaced = false;
	float K = 1.f;
	float Flash = 0.f;
	float Seed = 0.f;
	bool bHooked = false;
};
