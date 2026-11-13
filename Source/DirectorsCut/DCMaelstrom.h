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
	float RimElevation(float Angle) const;
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
	float NextOceanScan = 0.f;
	bool bOceanHidden = false;
	TWeakObjectPtr<class UCameraComponent> CaptainCamera;
	FVector OriginalEye = FVector::ZeroVector;
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
};
