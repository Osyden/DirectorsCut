#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCTypes.h"
#include "DCEarlyWarningPolicy.h"
#include "DCEarlyEncounters.generated.h"

/** Readable bank encounters. Owns its actors, warnings and impacts only during the city. */
UCLASS()
class ADCEarlyEncounters : public AActor
{
	GENERATED_BODY()
public:
	ADCEarlyEncounters();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	static ADCEarlyEncounters* Get(const UObject* Context);
	FString GetThreatText() const;
	bool GetReviewStatueTarget(EDCSide Side,FVector& Out) const;

private:
	struct FThreat
	{
		TWeakObjectPtr<AActor> Body, Target, Charge, Marker;
		TWeakObjectPtr<class UDCCombatCueComponent> AlertCue;
		FName CurrentClip;
		float GaitPhase = 0.f, LastAnimLog = -10.f;
		TArray<TWeakObjectPtr<class UStaticMeshComponent>> Eyes, Beams;
		FVector EyeLocal[2] = {};
		FVector DeckTarget = FVector::ZeroVector;
		float X = 0.f, SeaY = 0.f, Side = 1.f, Age = 0.f, Exposure = 0.f, Clock = 0.f;
		float Size = 1.f, InitialWait = 8.f, HoldX = 3500.f, FootHeight = 0.f;
		int32 CannonHits = 0;
		int32 Stage = 0; // approach, charge, attack, cooldown, defeated
		bool bAttackAnnounced = false, bDefeatAnnounced = false;
		bool bStatue = false, bImpact = false, bAnnounced = false, bGroupLeader = true, bCanShoot = false;
	};
	struct FProjectile
	{
		TWeakObjectPtr<AActor> Visual;
		FVector From, DeckTarget;
		float Age = 0.f, Duration = 2.4f;
		bool bAnnounced = false;
	};
	TArray<FThreat> Threats;
	UPROPERTY() TMap<FName, TObjectPtr<class UAnimSequence>> CreatureClips;
	TArray<FProjectile> Projectiles;
	UPROPERTY() TObjectPtr<class ADCRideDirector> Director;
	UPROPERTY() TObjectPtr<class ADCTreadmill> Treadmill;
	UPROPERTY() TObjectPtr<class ADCGunnery> Guns;
	UPROPERTY() TObjectPtr<class ADCTroubles> Troubles;
	int32 NextGroup = 0, NextStatue = 0, ImpactCount = 0;
	float LastRideTime = -1.f, AttackSpacing = 0.f;
	FDelegateHandle HUDHandle;
	void DrawStatueTarget(class AHUD* HUD, class UCanvas* Canvas);
	void SpawnGroup(int32 Group);
	void SpawnStatue(int32 Index);
	void TickThreat(FThreat& Threat, float DeltaSeconds);
    bool IsThreatReportRelevant(TWeakObjectPtr<AActor> Body, float Side, DCEarlyWarningPolicy::EKind Kind) const;
    bool IsProjectileReportRelevant(TWeakObjectPtr<AActor> Visual) const;
	UAnimSequence* CreatureClip(FName Name);
	void UpdateCreaturePose(FThreat& Threat, float Dt, float GroundSpeed, float Facing);
	void UpdateAlert(FThreat& Threat, bool bConcealed);
	void Ignite(const FVector& DeckPoint, const TCHAR* Cause);
	void Clear();
	void RemoveThreat(FThreat& Threat);
	AActor* MakeTarget(const FVector& Position, float Radius, float Health, bool bCreature = false);
	float AcceptCreatureHit(float Damage, TWeakObjectPtr<AActor> Target);
	AActor* MakeFireball(const FVector& Position);
	AActor* MakeMarker(const FVector& DeckPoint);
	FVector Hand(const FThreat& Threat) const;
#if !UE_BUILD_SHIPPING
	void TickVerification(float DeltaSeconds);
	void TickEncounterVerification();
	TWeakObjectPtr<AActor> VerifyEnemy;
	int32 VerifyCreatureHits = 0;
	int32 VerifyShots = 0, VerifyFireballImpacts = 0, VerifyBeamImpacts = 0;
	bool bVerifyInterrupted = false;
	int32 VerifyStage = 0, VerifyScore = 0;
	float VerifyClock = 0.f;
	bool bVerifyFailed = false;
	FVector VerifyFire = FVector::ZeroVector;
#endif
};
