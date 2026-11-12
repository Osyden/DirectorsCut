#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCKrakenHead.generated.h"

class UProceduralMeshComponent;
class UStaticMeshComponent;
class USceneComponent;
class UMaterialInstanceDynamic;
class ADCRideDirector;

/**
 * The creature of the maelstrom, rising out of the centre of the crater ahead of the ship: a
 * leviathan with a neck three hundred metres tall, a horned, long-jawed head that turns to the
 * ship and opens to roar, a ridge of spines, and a ring of arms around its base. All procedural.
 * It rides on the sea mesh, so it tilts and rocks with the crater. Scenery only; the arms that
 * reach the ship are in DCKraken.
 */
UCLASS()
class ADCKrakenHead : public AActor
{
	GENERATED_BODY()

public:
	ADCKrakenHead();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Seconds into the maelstrom before it starts to rise, and how long the rise takes. */
	UPROPERTY(EditAnywhere) float RiseDelay = 6.f;
	UPROPERTY(EditAnywhere) float RiseTime = 22.f;
	UPROPERTY(EditAnywhere) int32 NumArms = 7;
	/** Height of the head above the crater floor when fully risen, cm. */
	UPROPERTY(EditAnywhere) float NeckLength = 27000.f;
	/** 1 = a skull about 70 m long. */
	UPROPERTY(EditAnywhere) float HeadScale = 1.25f;
	/** The whole creature is built at one size and then scaled by this: 2.8 puts the head 750 m above the floor. */
	UPROPERTY(EditAnywhere) float BodyScale = 2.8f;

protected:
	void BuildMound();
	void UpdateArm(int32 Index, bool bCreate);
	void UpdateNeck(bool bCreate);
	UStaticMeshComponent* AddPart(USceneComponent* Parent, const TCHAR* Shape, const FVector& Location, const FRotator& Rotation, const FVector& Scale, UMaterialInterface* Material);

	UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> Mesh;
	UPROPERTY() TObjectPtr<USceneComponent> HeadRoot;
	UPROPERTY() TObjectPtr<USceneComponent> JawRoot;
	UPROPERTY() TObjectPtr<USceneComponent> MistRoot;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> MistMID;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Spines;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> EyeMID;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> HideMID;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> MawMID;
	UPROPERTY() TObjectPtr<ADCRideDirector> Director;

	TArray<int32> TubeTris;
	TArray<float> ArmSeeds;
	TArray<FVector> NeckSpine;
	float Raised = 0.f;
	float Clock = 0.f;
	float MaelstromTime = 0.f;
	float EyeFlare = 0.f;
	float JawOpen = 0.f;
	bool bAttached = false;
};
