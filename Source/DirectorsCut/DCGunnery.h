#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCTypes.h"
#include "DCGunnery.generated.h"

class UInstancedStaticMeshComponent;
class ADCTreadmill;

/**
 * Individually assigned ship guns. Each trigger launches one ballistic cannonball
 * from the moving barrel, with its own reload and water/solid impact.
 */
UCLASS()
class ADCGunnery : public AActor
{
	GENERATED_BODY()

public:
	ADCGunnery();
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	static ADCGunnery* Get(const UObject* WorldContext);

	/** Reads gun positions from the ship's components. */
	void Init(AActor* Ship, float InSeaLevelZ);
	AActor* GetGunShip() const { return GunShip.Get(); }

	/** Legacy test entry point; fires a single gun. */
	bool FireBroadside(EDCSide Side, const FVector& Target, int32 MaxGuns, bool bPlayerVolley = false);
	bool ResolveAim(EDCSide Side, const FVector& Origin, const FVector& Direction, FVector& OutTarget) const;
	int32 GetPlayerGun(EDCSide Side) const;
	int32 GetGunCount() const { return Guns.Num(); }
	EDCSide GetGunSide(int32 Gun) const;
	FVector GetGunStation(int32 Gun) const;
	FVector GetMuzzle(int32 Gun) const;
	bool IsGunReady(int32 Gun) const;
	bool FireGun(int32 Gun, const FVector& Target, bool bPlayer = false);
	bool CanAimGun(int32 Gun, const FVector& Target) const;
	bool HasClearShot(int32 Gun, const AActor* Target) const;
	bool FindCrewTarget(int32 Gun, FVector& OutTarget, bool bLeadTarget = true) const;
	float GetReloadSeconds(EDCSide Side) const;
	FString GetPlayerImpactLabel(EDCSide Side) const;
	bool IsReady(EDCSide Side) const { return IsGunReady(GetPlayerGun(Side)); }
	float GetReloadFraction(EDCSide Side) const { return FMath::Clamp(GetReloadSeconds(Side) / FMath::Max(ReloadTime, UE_SMALL_NUMBER), 0.f, 1.f); }
	int32 NumGuns(EDCSide Side) const;

	UPROPERTY(EditAnywhere) float ReloadTime = 3.f;
	UPROPERTY(EditAnywhere) float ShotSpeed = 7000.f;
	UPROPERTY(EditAnywhere) float Gravity = 1400.f;

protected:
	struct FPendingShot { int32 Gun; FVector Target; EDCSide PlayerSide; };
	struct FWaterHistory { FVector Position; double Time; };
	struct FShot
	{
		FVector Pos; FVector Vel; float Life; EDCSide PlayerSide;
		int32 WaterChannel = INDEX_NONE;
		TArray<FWaterHistory> WaterHistory;
	};
	struct FWaterPlane { FVector Point; FVector Normal = FVector::UpVector; bool bSampled = false; };
	FWaterPlane QueryShotWater(FShot& Shot);
	void WaterContact(const FVector& Position, bool bSampled);
	struct FWaterBurst { TWeakObjectPtr<class UParticleSystemComponent> Effect; float Age = 0.f; };
	TArray<FWaterBurst> WaterBursts;
	UPROPERTY() TObjectPtr<UClass> WaterLibrary;
	UPROPERTY() TObjectPtr<UFunction> WaterSurfaceFunction;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> Balls;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> Trails;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> Spray;
	UPROPERTY(VisibleAnywhere) TObjectPtr<class UProceduralMeshComponent> MuzzleFlames;
	UPROPERTY(VisibleAnywhere) TObjectPtr<class UProceduralMeshComponent> MuzzleSmoke;
	UPROPERTY(VisibleAnywhere) TObjectPtr<class UPointLightComponent> MuzzleLight;
	struct FMuzzleFlash { FVector Position; FVector Direction; float Age; float Seed; };
	TArray<FMuzzleFlash> MuzzleFlashes;
	void TickMuzzleFlashes(float DeltaSeconds);
	UPROPERTY() TObjectPtr<UClass> MuzzleEffectClass;
	UPROPERTY() TObjectPtr<UClass> ImpactEffectClass;
	UPROPERTY() TObjectPtr<class UParticleSystem> WaterImpact;
	UPROPERTY() TObjectPtr<class UParticleSystem> StoneImpact;
	UPROPERTY() TObjectPtr<class UParticleSystem> CreatureImpact;

	TWeakObjectPtr<AActor> GunShip;
	struct FGun
	{
		TWeakObjectPtr<class USkeletalMeshComponent> Component;
		FVector LocalMuzzle = FVector::ZeroVector;
		FVector Station = FVector::ZeroVector;
		EDCSide Side = EDCSide::None;
		float Reload = 0.f;
		bool bPlayer = false;
	};
	TArray<FGun> Guns;
	float ImpactUntil[2] = {};
	bool bImpactHit[2] = {};
	TArray<FVector> Muzzles[2];
	TArray<FPendingShot> Pending;
	TArray<FShot> Shots;
	struct FSplash { FVector Position; float Age; };
	TArray<FSplash> Splashes;
	float SeaLevelZ = 0.f;
#if !UE_BUILD_SHIPPING
	void TickVerification();
	int32 WaterReviewStage = 0;
	int32 VerificationStage = 0;
	bool bVerificationFailed = false;
	TWeakObjectPtr<AActor> VerificationTarget;
#endif
};
