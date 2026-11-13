#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCFogBank.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class ADCRideDirector;

/** What one bank of fog is like. Lengths in cm. */
struct FDCFogBankDesc
{
	/** Half its length along the ship's course, across it, and half its height. */
	FVector Radii = FVector(8000.f, 6000.f, 2500.f);
	/** Extinction at its heart; about 0.004 is a veil a ship is seen through, 0.03 hides a temple. */
	float Density = 0.012f;
	/** Size of the biggest billows. */
	float Size = 6000.f;
	/** 0 = a smooth blob .. 1 = torn to rags. */
	float Erode = 0.6f;
	/** How fast the billows travel through the bank, cm a second. */
	FVector Drift = FVector(-250.f, 120.f, 0.f);
	/** How far the billows are drawn out along each axis. */
	FVector Stretch = FVector(1.f, 1.f, 1.f);
	/** If above zero, the bank thins away to nothing when the ship comes within this distance: what it hid is revealed. */
	float RevealWithin = 0.f;
	FLinearColor Tint = FLinearColor(0.8f, 0.86f, 0.95f);
	/** Stays round the ship (wisps across the deck) instead of lying in the sea. */
	bool bOnDeck = false;
	/** Draw it with the heterogeneous-volume renderer (sharp, self-shadowed) instead of the fog grid. */
	bool bHero = false;
	/** Above 0, only the crests of the noise are fog: streaks with clear air between. */
	float Wisp = 0.f;
};

/**
 * A bank of fog that is really in the air: an ellipsoid filled by the engine's volumetric fog
 * (M_FogVolume), with billows that drift and a ragged edge. It is lit like the rest of the air, by
 * the sky, lightning and the ship's lanterns, and ruins cast their shadows through it. The treadmill
 * carries the banks past the ship like the ruins, so fog can be laid where it is wanted: walls to
 * either hand that leave a way through, curtains that hide a statue until the ship is close.
 */
UCLASS()
class ADCFogBank : public AActor
{
	GENERATED_BODY()

public:
	ADCFogBank();
	virtual void Tick(float DeltaSeconds) override;
	static ADCFogBank* Spawn(UWorld* World, const FDCFogBankDesc& Desc, int32 Seed);

protected:
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> MID;
	/** A bank drawn by the engine's heterogeneous-volume renderer instead of the fog grid: ray-marched at
	 *  its own resolution and shadowing itself against the sun and the lightning, so it has sharp
	 *  sculpted billows where the fog grid can only give a soft glow. Costlier; used for the banks near the course. */
	UPROPERTY() TObjectPtr<class UHeterogeneousVolumeComponent> Hero;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> HeroMID;
	float HeroUnit = 1.f;
	UPROPERTY() TObjectPtr<ADCRideDirector> Director;
	FDCFogBankDesc Desc;
	/** 0..1: how much of the bank is there (it thins away when it reveals, and with the weather). */
	float Amount = 0.f;
	float Shown = -1.f;
};
