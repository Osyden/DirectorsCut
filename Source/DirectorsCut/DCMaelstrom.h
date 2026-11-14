#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCMaelstromWaterBridge.h"
#include "DCMaelstrom.generated.h"

class UProceduralMeshComponent;
class UMaterialInstanceDynamic;
class UPostProcessComponent;
class ADCRideDirector;
class ADCLeviathan;
class ADCMaelstromAtmosphere;

/** A self-contained encounter, expressed in a single basin coordinate system. */
UCLASS()
class ADCMaelstrom : public AActor
{
	GENERATED_BODY()
public:
	ADCMaelstrom();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	static ADCMaelstrom* Get(const UObject* Context);
	bool IsActive() const { return bActive; }
	float GetElapsed() const { return Elapsed; }
	float GetRouteOffset() const { return RouteOffset; }
	void EmitWaterContact(const FVector& Position, float Strength = 1.f);
	FTransform GetArenaTransform() const;
	float SampleSurfaceWorld(const FVector& Position) const;
	float GetRimRadius(float Angle) const;
	float GetRimDrop(float Angle) const;
	float GetRimJoin(float Angle) const;
	float GetOverflowAmount(float Angle) const;

	UPROPERTY(EditAnywhere) float Radius = 160000.f;
	UPROPERTY(EditAnywhere) float Depth = 90000.f;

private:
	float HeightAt(const FVector2D& Point, bool bWaves = true) const;
	float BaseHeightAt(const FVector2D& Point) const;
	float RimElevation(float Angle) const;
	void UpdateContacts(float DeltaSeconds);
	float RimGroundHeight(float R, float Angle) const;
	void BuildBasin();
	void BuildTunnel();
	void BuildLandmarks();
	void UpdateStage(float DeltaSeconds);
	void UpdateView(float DeltaSeconds);
	void RestoreView();
	void SetStageVisible(bool bVisible);
	void DisableLegacyEncounter();
	void SetOceanVisible(bool bVisible);
	void RestoreAtmosphere();

	UPROPERTY() TObjectPtr<ADCRideDirector> Director;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Arena;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> Basin;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> ContactMesh;
	struct FContactParticle { FVector Position, Velocity; float Age, Life, Size; };
	TArray<FContactParticle> ContactParticles;
	float NextWakeAt = 0.f;
	UPROPERTY() TObjectPtr<class USoundBase> ContactSound;
	float NextContactSoundAt = -100.f;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> Falls;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> RimLand;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> Tunnel;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPostProcessComponent> Exposure;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> WaterMID;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> TunnelMID;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CascadeMID;
	FDCMaelstromWaterBridge WaterBridge;
	UPROPERTY() TObjectPtr<ADCLeviathan> Boss;
	UPROPERTY() TObjectPtr<ADCMaelstromAtmosphere> Atmosphere;
	UPROPERTY() TArray<TObjectPtr<AActor>> Landmarks;
	TWeakObjectPtr<AActor> Ocean;
	TWeakObjectPtr<AActor> LegacyWeather;
	struct FStageFog
	{
		TWeakObjectPtr<class UExponentialHeightFogComponent> Component;
		FVector Location;
		float Distance;
		float Falloff;
		float Density;
	};
	TArray<FStageFog> StageFogs;
	struct FStageFogVolume
	{
		TWeakObjectPtr<AActor> Owner;
		TWeakObjectPtr<class UPrimitiveComponent> Mesh;
		bool bEnabled;
		bool bVisible;
		bool bHidden;
	};
	TArray<FStageFogVolume> StageFogVolumes;
	bool bAtmosphereAdjusted = false;
	TArray<TWeakObjectPtr<AActor>> OceanGrids;
	TMap<TWeakObjectPtr<AActor>, bool> OceanCollisionBeforeHide;
	float NextOceanScan = 0.f;
	bool bOceanHidden = false;
	TWeakObjectPtr<class UCameraComponent> CaptainCamera;
	FVector OriginalEye = FVector::ZeroVector;
	/** Waterspouts the creature raises in the ship's path. Each stands up ahead for a few seconds (the tell),
	 *  then comes down the current at the ship; the helm carries her to one side of it or she is struck. */
	struct FTwister
	{
		TWeakObjectPtr<class AHeterogeneousVolume> Actor;
		TWeakObjectPtr<UMaterialInstanceDynamic> MID;
		float RaisedAt = 0.f;     // seconds after the breach
		float Ahead = 0.f;        // cm ahead of the ship
		float RouteOffsetAtSpawn = 0.f;
		float Abeam = 0.f;        // cm to starboard of her course
		bool bStruck = false;
		bool bCalled = false;
		TWeakObjectPtr<AActor> Plain;     // a plain column of pale cloud, where the bought volume is not installed
		TWeakObjectPtr<UMaterialInstanceDynamic> PlainMID;
		TWeakObjectPtr<AActor> Skirt;     // churned water and spray round its foot (an Easy Fog card, if installed)
		TWeakObjectPtr<UMaterialInstanceDynamic> SkirtMID;
	};
	/** Rolling cloud lining the last of the water tunnel and collaring its mouth (the bought volume, if installed). */
	TWeakObjectPtr<class AHeterogeneousVolume> Throat;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> ThroatMID;
	bool bThroatTried = false;
	void UpdateThroat(float ExitX);
	/** Burning stones the creature hurls from the deep. Each is seen leaving the water and coming over in a high
	 *  arc for five seconds; the helm carries the ship out from under it, or it strikes and sets the deck alight. */
	struct FBoulder
	{
		TWeakObjectPtr<AActor> Actor;
		float ThrownAt = 0.f;     // seconds after the breach
		FVector From = FVector::ZeroVector;
		float RouteOffsetAtSpawn = 0.f;
		float TargetAcross = 0.f;
		FVector Fall = FVector::ZeroVector;   // where it comes down, in the ship's space
		TArray<TWeakObjectPtr<class UStaticMeshComponent>> Tail;
	};
	TArray<FBoulder> Boulders;
	int32 BouldersThrown = 0;
	void UpdateBoulders(float DeltaSeconds);
	/** The creature's end: a column of water and spray thrown up from the heart of the crater when the clock beats it. */
	TWeakObjectPtr<class AHeterogeneousVolume> Plume;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> PlumeMID;
	bool bPlumeShaken = false;
	/** The creature itself, seen once: it heaves up in the plume at the heart of the crater as it is beaten, and goes down. */
	TWeakObjectPtr<class ASkeletalMeshActor> Carcass;
	void UpdatePlume();
	/** The creature's young: one swims in on the beam, to be sunk by that side's guns before it reaches the hull and tears into her. */
	struct FSpawnling
	{
		TWeakObjectPtr<AActor> Actor;
		TWeakObjectPtr<AActor> Mark;      // what the guns aim at and destroy; unseen
		float SunkAt = -1.f;              // seconds after the breach at which the guns sank it
		float Side = 1.f;         // -1 port, +1 starboard
		float Abeam = 0.f;        // cm off the ship's side
		bool bWarned = false;
		bool bAttacked = false;
		float SafeDistance = 4000.f;
		float NextSprayAt = 0.f;
	};
	TArray<FSpawnling> Spawnlings;
	int32 SpawnlingsSent = 0;
	void UpdateSpawnlings(float DeltaSeconds);
	TArray<FTwister> Twisters;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> TwisterMIDs;
	int32 TwistersRaised = 0;
	void UpdateTwisters(float DeltaSeconds);
	void ClearTwisters();
	bool bActive = false;
	bool bHadView = false;
	bool bAutomaticHelmView = true;
	bool bLegacyDisabled = false;
	float Elapsed = 0.f;
	float BreachStart = 270.f;
	float SlideStart = 240.f;
	float EndingStart = 450.f;
	float PreviousTime = -1.f;
	float ShipRadius = 0.f;
	float HelmTurn = 0.f;
	float RouteOffset = 0.f;
	float RouteVelocity = 0.f;
	float NextRouteLog = 0.f;
};
