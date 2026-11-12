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

protected:
	void BuildDefaultPhases();
	void ApplyEnv(float DeltaSeconds);
	void BuildSeaMesh();
	FVector2D SeaScroll = FVector2D::ZeroVector;

public:
	UPROPERTY(EditAnywhere) float TubeRadius = 3200.f;
	/** Maelstrom centre relative to the ship, cm. Negative Y is to port. */
	UPROPERTY(EditAnywhere) FVector2D WhirlCenter = FVector2D(0.f, -80000.f);
	UPROPERTY(EditAnywhere) float WhirlDepth = 50000.f;
	UPROPERTY(EditAnywhere) float WhirlRadius = 70000.f;

protected:

	UPROPERTY(VisibleAnywhere) TObjectPtr<UDirectionalLightComponent> Sun;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UDirectionalLightComponent> Flash;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USkyLightComponent> SkyLight;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USkyAtmosphereComponent> Atmosphere;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UExponentialHeightFogComponent> Fog;
	UPROPERTY(VisibleAnywhere) TObjectPtr<class UProceduralMeshComponent> Sea;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SeaMID;
	UPROPERTY(VisibleAnywhere) TObjectPtr<class UPostProcessComponent> PostProcess;
	UPROPERTY(VisibleAnywhere) TObjectPtr<class UVolumetricCloudComponent> Clouds;

	FDCEnvState Env;
	EDCPhase CurrentPhase = EDCPhase::OpenSea;
	FTransform WorldRock;
	float FlashTimeLeft = 0.f;
	float NextFlashIn = 5.f;
};
