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

/** Finale clouds and falling-water atmosphere; shared weather changes are restored on exit. */
UCLASS()
class ADCMaelstromAtmosphere : public AActor
{
	GENERATED_BODY()
public:
	ADCMaelstromAtmosphere();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	void UpdateAtmosphere(ADCMaelstrom* Stage, float Elapsed, bool bActive);
	/** Authoring utility: finish shader compilation before a generated asset is saved. */
	UFUNCTION(BlueprintCallable, Category="Maelstrom|Authoring")
	static bool CompileSkyMaterial(UMaterialInterface* Material);

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
		float CloudRaySamples = 1.f;
		bool bShaftBloom = false;
		bool bShaftOcclusion = false;
		float BloomScale = 0.f;
		float BloomThreshold = 1.f;
		float BloomMaximum = 100.f;
		FColor BloomTint = FColor::White;
		float OcclusionDarkness = 1.f;
		float OcclusionDistance = 100000.f;
		bool bCloudShadows = false;
		bool bVolumetricShadow = false;
		TWeakObjectPtr<UMaterialInterface> LightFunction;
	};
	struct FVolumetricState
	{
		TWeakObjectPtr<class UExponentialHeightFogComponent> Component;
		bool bEnabled = false;
		float ExtinctionScale = 1.f;
		float ScatteringDistribution = 0.2f;
		float ViewDistance = 0.f;
	};
	struct FSkyFillState
	{
		TWeakObjectPtr<class USkyLightComponent> Component;
		float Scattering = 1.f;
		bool bCloudAO = false;
		float Strength = 1.f;
		float Extent = 150.f;
		float Resolution = 1.f;
		float Aperture = 1.f;
	};
	FSkyFillState SavedSkyFill;
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
	void ConfigureSkyCues();
	void CaptureSky();
	void RestoreSky();
	void RefreshSky();
	void UpdateSkyVolume();
	void LogLightingDiagnostics();
	int32 LightingDiagnosticSamples = 0;
	double NextLightingDiagnosticTime = 0.0;
	void SetSkyNumber(const TCHAR* Name, double Value);
	void SetSkyColor(const TCHAR* Name, FLinearColor Value);
	TWeakObjectPtr<AActor> StageSky;
	TMap<FName, FString> SavedSkyProperties;
	UPROPERTY() TObjectPtr<UMaterialInterface> SkyCloudTemplate;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SkyCloudMID;
	UPROPERTY() TObjectPtr<UMaterialInterface> PreviousCloudMaterial;
	TWeakObjectPtr<class UVolumetricCloudComponent> StageCloud;
	float PreviousCloudBottom = 0.f;
	float PreviousCloudHeight = 0.f;
	float PreviousCloudViewSamples = 1.f;
	float PreviousCloudShadowSamples = 1.f;
	float PreviousCloudBottomOcclusion = 0.5f;
	FColor PreviousCloudGroundAlbedo = FColor::White;
	bool bPreviousShafts = true;
	int32 PreviousCloudRenderMode = -1;
	float PreviousCloudSampleDistance = -1.f;
	float PreviousLightShaftDistance = -1.f;

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
