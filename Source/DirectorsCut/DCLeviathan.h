#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCLeviathan.generated.h"

class UAudioComponent;
class UMaterialInstanceDynamic;
class UProceduralMeshComponent;
class USkeletalMeshComponent;
class UStaticMesh;

/** An unseen submerged creature threatens the ship through local, continuous tentacles. */
UCLASS()
class ADCLeviathan : public AActor
{
	GENERATED_BODY()

public:
	ADCLeviathan();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	/** Elapsed is measured from the breach. Arena origin is the crater centre at rim height. */
	void UpdateEncounter(float Elapsed, const FTransform& Arena, float Radius, float Depth, bool bActive);
	/** Allow the arms to withdraw before handing the ride back to its ending. */
	bool ShouldExitEncounter(float Elapsed) const { return bWasActive && DefeatedAt >= 0.f && Elapsed - DefeatedAt >= 20.f; }

	UPROPERTY(EditAnywhere) float BodyScale = 18.f;
	UPROPERTY(EditAnywhere) float StrikeDamage = 2.5f;
	/** Seconds after the breach at which the creature is beaten, whatever the guns have done: the music is cut to it.
	 *  Broken arms make it flinch and add to the score; they no longer end the fight early. */
	UPROPERTY(EditAnywhere) float FinaleAt = 160.f;
	UPROPERTY(BlueprintReadOnly) int32 ArmsBroken = 0;

private:
	struct FArm
	{
		TWeakObjectPtr<AActor> Target;
		int32 Cycle = INDEX_NONE;
		bool bTargetRegistered = false;
		bool bBrokenThisCycle = false;
		bool bStruck = false;
		bool bEmerged = false;
		bool bHasWaterContact = false;
		FVector WaterContact = FVector::ZeroVector;
		float NextSprayAt = 0.f;
		float BrokenAt = -1000.f;
	};

	void ResetEncounter();
	void RemoveTarget(FArm& Arm);
	void UpdateArm(int32 Index, float Elapsed, bool bCanDamage);
	FVector KeepTipReachable(const FVector& DesiredTip, float Side) const;
	void BuildArm(int32 Index, const FVector& Base, const FVector& Bend, const FVector& Joint, const FVector& Tip, float Elapsed);
	void SpawnTarget(FArm& Arm, const FVector& Position);

	UPROPERTY(VisibleAnywhere) TObjectPtr<USkeletalMeshComponent> Body;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> Arms;
	UPROPERTY() TObjectPtr<UAudioComponent> PainAudio;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> ArmMaterial;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> TargetMaterial;
	UPROPERTY() TObjectPtr<UStaticMesh> TargetMesh;

	TArray<int32> ArmTriangles;
	FArm ArmState[4];
	FVector ShipPosition = FVector::ZeroVector;
	float LastElapsed = -1.f;
	float DefeatedAt = -1.f;
	FVector MeshBoundsCenter = FVector::ZeroVector;
	FVector MeshBoundsExtent = FVector::ZeroVector;
	bool bWasActive = false;
	bool bAssetsReady = false;
	bool bAnnounced = false;
};
