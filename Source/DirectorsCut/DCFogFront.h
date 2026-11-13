#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCFogFront.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class ADCRideDirector;

/**
 * The cloud front the ship sails into at the end of the open-sea phase: a wall of billows from the
 * water to far above the masts, kilometres wide, dark at the foot and sunlit on top, lit from
 * inside by lightning. It closes on the ship and swallows it as the fog phase begins.
 */
UCLASS()
class ADCFogFront : public AActor
{
	GENERATED_BODY()

public:
	ADCFogFront();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Ride time at which the front reaches the bow. */
	UPROPERTY(EditAnywhere) float ArrivalTime = 60.f;
	/** Also draw the old wall of billow meshes. Off: the volumetric cloud layer carries the front. */
	UPROPERTY(EditAnywhere) bool bBillows = false;
	/** How far off it stands at the start of the ride, cm. */
	UPROPERTY(EditAnywhere) float StartDistance = 400000.f;

protected:
	struct FBillow
	{
		TWeakObjectPtr<UStaticMeshComponent> Component;
		FVector Offset;
		float Radial = 0.f, High = 0.f, Shown = -1.f;
		float Size;
		float Seed;
	};
	TArray<FBillow> Billows;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CloudMID;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> CloudShades;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CoreMID;
	UPROPERTY() TObjectPtr<ADCRideDirector> Director;
	/** Under the sky pack: the foot of the front, a wall of fog banks on the water that travels with it and swallows the ship. */
	struct FBank
	{
		TWeakObjectPtr<class ULocalFogVolumeComponent> Component;
		FVector Offset;
		float Radial = 0.f, High = 0.f, Shown = -1.f;
	};
	TArray<FBank> Banks;
	/** The mouth of the front: a rolling tunnel of cloud (the bought looping volume, if it is installed) that the ship sails through into the fog. */
	UPROPERTY() TObjectPtr<class AHeterogeneousVolume> Tunnel;
	UPROPERTY() TObjectPtr<class UMaterialInstanceDynamic> TunnelMID;
	float TunnelScale = 300.f;
	float TunnelSink = 5000.f;
	float TunnelShown = -1.f;
	float TunnelDensity = 1.f;
	/** The wall of the front either side of and over the mouth: the same volume, much enlarged and laid across the course like a roll cloud. */
	struct FWallPiece
	{
		TWeakObjectPtr<class AHeterogeneousVolume> Actor;
		TWeakObjectPtr<class UMaterialInstanceDynamic> MID;
		FVector Offset;   // from the tunnel's mouth at sea level
		float Scale = 900.f;
		float Density = 1.f;
		float Yaw = 0.f;
	};
	TArray<FWallPiece> Wall;
	UPROPERTY() TArray<TObjectPtr<class UMaterialInstanceDynamic>> WallMIDs;
	void BuildTunnel();
	void TickTunnel(float T);
	void BuildBanks();
	float Flash = 0.f;
	float NextFlashIn = 3.f;
};
