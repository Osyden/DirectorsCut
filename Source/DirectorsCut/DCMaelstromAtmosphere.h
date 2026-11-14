#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCMaelstromAtmosphere.generated.h"

class ADCMaelstrom;
class UDirectionalLightComponent;
class ULocalFogVolumeComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UProceduralMeshComponent;
class USpotLightComponent;

/** Local falling-water atmosphere. Owns no global fog or weather actor. */
UCLASS()
class ADCMaelstromAtmosphere : public AActor
{
	GENERATED_BODY()
public:
	ADCMaelstromAtmosphere();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	void UpdateAtmosphere(ADCMaelstrom* Stage, float Elapsed, bool bActive);

	/** Density of the soft 3D billows; sheets add only their torn spray detail. */
	UPROPERTY(EditAnywhere) float MistDensity = 0.60f;
	UPROPERTY(EditAnywhere) float SprayDensity = 0.42f;
	UPROPERTY(EditAnywhere) bool bEnableShafts = true;
	/** Actual spot lights inject into existing volumetric fog. No visible cone mesh. */
	UPROPERTY(EditAnywhere) float ShaftCandela = 900000000.f;
	/** Match the actual UDS sun's lux; its multiplier is not the director's 0..10 cue. */
	UPROPERTY(EditAnywhere) float ShaftSunFraction = 0.28f;
	UPROPERTY(EditAnywhere) float ShaftScattering = 2.0f;
	/** Keep the existing sun responsible for surface lighting. */
	UPROPERTY(EditAnywhere) float ShaftDiffuseScale = 0.005f;
	/** UDS can disable the medium with extinction zero even while fog is enabled. */
	UPROPERTY(EditAnywhere) float VolumetricExtinctionScale = 0.8f;
	UPROPERTY(EditAnywhere) bool bTuneSunCloudShadows = true;

private:
	struct FPlume
	{
		FVector Foot = FVector::ZeroVector;
		float Angle = 0.f;
		float Height = 0.f;
		float Width = 0.f;
		float Seed = 0.f;
		float Density = 1.f;
	};
	struct FFogLobe
	{
		int32 Plume = 0;
		FVector Offset = FVector::ZeroVector;
		float Radius = 0.f;
		float Density = 1.f;
		bool bDrift = false;
	};
	struct FSunState
	{
		TWeakObjectPtr<UDirectionalLightComponent> Component;
		float Scattering = 1.f;
		float CloudStrength = 1.f;
		float CloudAtmosphere = 1.f;
		float CloudSurface = 1.f;
		float CloudExtent = 1.f;
		float CloudResolution = 1.f;
		bool bCloudShadows = false;
		bool bVolumetricShadow = false;
	};
	struct FVolumetricState
	{
		TWeakObjectPtr<class UExponentialHeightFogComponent> Component;
		bool bEnabled = false;
		float ExtinctionScale = 1.f;
	};
	void BuildLayout(ADCMaelstrom* Stage);
	void BuildSpray();
	void AddFogLobe(const FFogLobe& Lobe);
	UDirectionalLightComponent* FindSun() const;
	void CaptureSun();
	void RestoreSun();
	void UpdateSun();
	void CaptureVolumetrics();
	void RestoreVolumetrics();
	void UpdateShafts(ADCMaelstrom* Stage, float Elapsed, float Reveal);
	void SetEffectsVisible(bool bVisible);

	UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> Spray;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SprayMID;
	UPROPERTY() TObjectPtr<UMaterialInterface> ShaftMask;
	UPROPERTY() TArray<TObjectPtr<ULocalFogVolumeComponent>> FogComponents;
	UPROPERTY() TArray<TObjectPtr<USpotLightComponent>> Shafts;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> ShaftMIDs;
	TArray<FPlume> Plumes;
	TArray<FFogLobe> FogLobes;
	TArray<FVolumetricState> SavedVolumetrics;
	FSunState SavedSun;
	bool bBuilt = false;
	bool bEffectsActive = false;
	bool bReportedRenderer = false;
};
