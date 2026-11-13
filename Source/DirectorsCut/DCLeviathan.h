#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCLeviathan.generated.h"

class UAnimationAsset;
class UAnimSequence;
class UAudioComponent;
class UMaterialInstanceDynamic;
class UProceduralMeshComponent;
class USkeletalMeshComponent;
class UStaticMesh;

/** A body, its long reaching arms and the gun targets all share the crater's coordinate frame. */
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
	/** Allow the creature to finish sinking before handing the ride back to its ending. */
	bool ShouldExitEncounter(float Elapsed) const { return bWasActive && DefeatedAt >= 0.f && Elapsed - DefeatedAt >= 20.f; }

	UPROPERTY(EditAnywhere) float BodyScale = 18.f;
	/** Imported bind pose: Head Y=-959cm, tail Y=+1383cm. Unreal flips the FBX's Y axis. */
	UPROPERTY(EditAnywhere) float MeshForwardYaw = -90.f;
	UPROPERTY(EditAnywhere) float StrikeDamage = 2.5f;
	UPROPERTY(EditAnywhere) int32 BreaksToDefeat = 6;
	UPROPERTY(BlueprintReadOnly) int32 ArmsBroken = 0;

private:
	struct FArm
	{
		TWeakObjectPtr<AActor> Target;
		int32 Cycle = INDEX_NONE;
		bool bTargetRegistered = false;
		bool bBrokenThisCycle = false;
		bool bStruck = false;
		float BrokenAt = -1000.f;
	};

	void ResetEncounter();
	void RemoveTarget(FArm& Arm);
	void Animate(UAnimSequence* Animation, float Position, bool bLoop);
	void UpdateArm(int32 Index, float Elapsed, float Emergence, bool bCanDamage);
	FVector KeepTipReachable(const FVector& DesiredTip, float Side) const;
	void BuildArm(int32 Index, const FVector& Base, const FVector& Bend, const FVector& Joint, const FVector& Tip, float Elapsed);
	void SpawnTarget(FArm& Arm, const FVector& Position);

	UPROPERTY(VisibleAnywhere) TObjectPtr<USkeletalMeshComponent> Body;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> Arms;
	UPROPERTY() TObjectPtr<UAudioComponent> PainAudio;
	UPROPERTY() TObjectPtr<UAnimSequence> IntimidateAnimation;
	UPROPERTY() TObjectPtr<UAnimSequence> CrawlAnimation;
	UPROPERTY() TObjectPtr<UAnimSequence> AttackAnimation;
	UPROPERTY() TObjectPtr<UAnimSequence> HitAnimation;
	UPROPERTY() TObjectPtr<UAnimSequence> DefeatedAnimation;
	UPROPERTY() TObjectPtr<UAnimationAsset> CurrentAnimation;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> ArmMaterial;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> TargetMaterial;
	UPROPERTY() TObjectPtr<UStaticMesh> TargetMesh;

	TArray<int32> ArmTriangles;
	FArm ArmState[4];
	FVector ShipPosition = FVector::ZeroVector;
	float LastElapsed = -1.f;
	float LastHitAt = -1000.f;
	float DefeatedAt = -1.f;
	float FeetOffset = 0.f;
	FVector MeshBoundsCenter = FVector::ZeroVector;
	FVector MeshBoundsExtent = FVector::ZeroVector;
	bool bWasActive = false;
	bool bAssetsReady = false;
	bool bAnnounced = false;
};
