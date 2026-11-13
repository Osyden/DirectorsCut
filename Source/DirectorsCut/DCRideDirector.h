#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCTypes.h"
#include "DCRideDirector.generated.h"

class UDirectionalLightComponent;
class USkyLightComponent;
class USkyAtmosphereComponent;
class UExponentialHeightFogComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

DECLARE_MULTICAST_DELEGATE_OneParam(FDCOnPhaseChanged, EDCPhase);

/**
 * Owns ride time. One float (RideTime) drives the whole show: phase, environment blend,
 * sea motion and lightning. The ship never moves; this actor changes the world around it.
 */
UCLASS()
class ADCRideDirector : public AActor
{
	GENERATED_BODY()

public:
	ADCRideDirector();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable) void Seek(float NewTime);
	UFUNCTION(BlueprintCallable) void DamageShip(float Amount, bool bQuiet = false);

	EDCPhase GetPhase() const { return CurrentPhase; }
	const FDCEnvState& GetEnv() const { return Env; }
	/** Rotation and heave applied to everything that is "the world" (sea, hazards). */
	FTransform GetWorldRock() const { return WorldRock; }
	float GetRideLength() const;
	/** The sea mesh component: attach to it to ride the rocking, tilted sea. */
	class USceneComponent* GetSea() const;
	void AdoptLampGlass(AActor* Ship);
	/** Bottom of the maelstrom funnel, in the sea mesh component space. */
	FVector GetWhirlFloorLocal() const;

	static ADCRideDirector* Get(const UObject* WorldContext);

	UPROPERTY(EditAnywhere, BlueprintReadOnly) float RideTime = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float TimeScale = 1.f;
	UPROPERTY(EditAnywhere) TArray<FDCPhaseDef> Phases;
	UPROPERTY(EditAnywhere) float SeaLevelZ = 0.f;
	UPROPERTY(BlueprintReadOnly) float ShipHealth = 100.f;
	UPROPERTY(BlueprintReadOnly) int32 Score = 0;

	FDCOnPhaseChanged OnPhaseChanged;
	/** Fired at the start of every lightning flash. */
	FSimpleMulticastDelegate OnLightning;
	/** Fired for every blow to the hull (not for slow burn damage). */
	TMulticastDelegate<void(float)> OnShipHit;

	/** A ship lantern at a world position; lit automatically when it gets dark. */
	void AddLamp(const FVector& Location);
	UPROPERTY(EditAnywhere) float LampIntensity = 3200.f;
	/** 0..1 camera shake: hits, thunder, kraken blows. Decays by itself. */
	float Trauma = 0.f;
	/** A slow lean of the captain's view with the ship's motion, used while the plugin sea (which cannot be rocked) is shown. Degrees. */
	FRotator DeckSway = FRotator::ZeroRotator;
	float WhiteOut = 0.f;
	int32 HintStage = 0;

protected:
	void BuildDefaultPhases();
	void ApplyEnv(float DeltaSeconds);
	void BuildSeaMesh();
	void BuildBolt();
	void TickLamps();
	UPROPERTY() TObjectPtr<class UProceduralMeshComponent> Bolt;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> BoltMID;
	UPROPERTY() TArray<TObjectPtr<class UPointLightComponent>> Lamps;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> LampGlows;
	/** The ship's own materials, made wettable: rain and spray darken the timber and give it a sheen. */
	struct FWetMaterial { TWeakObjectPtr<UMaterialInstanceDynamic> MID; float RoughMin; float RoughMax; float Bright; float Spec; };
	TArray<FWetMaterial> WetMaterials;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> WetMIDs;
	float Wetness = 0.f;
	float WetApplied = -1.f;
	void TickWetness(float DeltaSeconds);
	FVector2D SeaScroll = FVector2D::ZeroVector;

public:
	UPROPERTY(EditAnywhere) float TubeRadius = 6000.f;
	/** Maelstrom centre relative to the ship, cm. Negative Y is to port. */
	UPROPERTY(EditAnywhere) FVector2D WhirlCenter = FVector2D(186000.f, 40000.f);
	/** The centre of the maelstrom starts this far from the ship and ends this far, on this bearing (degrees to starboard of the bow). */
	UPROPERTY(EditAnywhere) float WhirlStartDistance = 120000.f;
	UPROPERTY(EditAnywhere) float WhirlEndDistance = 96000.f;
	UPROPERTY(EditAnywhere) float WhirlBearing = 15.f;
	/** Seconds of ride time since the maelstrom phase began (0 before it). */
	float MaelstromElapsed = 0.f;
	/** True when Ultra Dynamic Sky is installed and has taken over sky, light, fog and weather. */
	bool bUDS = false;
	/** The storm front in the cloud volume: world X of its face, strength 0..1, radius of its tunnel mouth. Set by ADCFogFront. */
	float StormFrontX = 400000.f;
	float StormFrontStrength = 0.f;
	float StormMouthRadius = 0.f;
	/** The portal out of Atlantis, drawn in the cloud volume. Set by ADCPortal. */
	FVector PortalLocation = FVector::ZeroVector;
	float PortalRadius = 21000.f;
	float PortalStrength = 0.f;
	/** Faces in the clouds, drawn in the cloud volume: xyz centre, w half-height of the head (0 = none). Set by ADCCloudFace. */
	FVector4 CloudFaces[3] = { FVector4(0, 0, 0, 0), FVector4(0, 0, 0, 0), FVector4(0, 0, 0, 0) };
	int32 NumCloudFaces = 0;
	/** Height of the swell that lifts the ship in the maelstrom, cm, and how much of it is up right now. */
	UPROPERTY(EditAnywhere) float SwellHeight = 11000.f;
	float SwellLift = 0.f;
	UPROPERTY(EditAnywhere) float WhirlDepth = 100000.f;
	UPROPERTY(EditAnywhere) float WhirlRadius = 60000.f;
	/** The crater: a ring of falling water around the maelstrom. Radius, height and half-width of the wall, cm. */
	UPROPERTY(EditAnywhere) float CraterRim = 330000.f;
	UPROPERTY(EditAnywhere) float CraterCliff = 30000.f;
	UPROPERTY(EditAnywhere) float CraterWidth = 16000.f;

protected:

	UPROPERTY(VisibleAnywhere) TObjectPtr<UDirectionalLightComponent> Sun;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UDirectionalLightComponent> Flash;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USkyLightComponent> SkyLight;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USkyAtmosphereComponent> Atmosphere;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UExponentialHeightFogComponent> Fog;
	UPROPERTY(VisibleAnywhere) TObjectPtr<class UProceduralMeshComponent> Sea;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SeaMID;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CloudMID;
	UPROPERTY() TObjectPtr<AActor> UDSSky;
	UPROPERTY() TObjectPtr<AActor> UDSWeather;
	/** The sky pack's travelling storm: the front on the horizon that the ship sails into. */
	UPROPERTY() TObjectPtr<AActor> UDSStorm;
	float StormRadius = 1400000.f;
	void SetupUDS();
	void SetupOcean();
	UPROPERTY() TObjectPtr<AActor> Ocean;
	/** The plugin sea's wave state, re-made whenever the ride's weather changes enough to matter. */
	UPROPERTY() TObjectPtr<UObject> OceanWaves;
	float OceanWind = -1.f;
	void TickOcean();
	void TickUDS();
	UPROPERTY() TArray<TObjectPtr<class UTexture>> CloudNoise;
	UPROPERTY(VisibleAnywhere) TObjectPtr<class UPostProcessComponent> PostProcess;
	UPROPERTY(VisibleAnywhere) TObjectPtr<class UVolumetricCloudComponent> Clouds;

	FDCEnvState Env;
	EDCPhase CurrentPhase = EDCPhase::OpenSea;
	FTransform WorldRock;
	float FlashTimeLeft = 0.f;
	float NextFlashIn = 5.f;
};
