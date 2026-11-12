#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCTroubles.generated.h"

class UPointLightComponent;
class UStaticMeshComponent;
class ADCRideDirector;

/** One fire on deck. */
struct FDCFire
{
	FVector Location = FVector::ZeroVector;
	float Age = 0.f;
	float Seed = 0.f;
	TWeakObjectPtr<AActor> Visual;
	TWeakObjectPtr<UPointLightComponent> Light;
};

/**
 * Things that go wrong aboard and need a pair of hands: fires on deck in the hell channel,
 * rigging torn loose in the storm. Each one costs the ship until someone deals with it,
 * which is what forces the captain to keep re-assigning a small crew.
 */
UCLASS()
class ADCTroubles : public AActor
{
	GENERATED_BODY()

public:
	ADCTroubles();
	virtual void Tick(float DeltaSeconds) override;

	static ADCTroubles* Get(const UObject* WorldContext);

	/** Places on deck where a fire may start. Set by the game mode. */
	TArray<FVector> DeckPoints;

	int32 NumFires() const { return Fires.Num(); }
	bool NearestFire(const FVector& From, FVector& OutLocation) const;
	void StartFire(const FVector& Location);
	/** Puts out the fire nearest to Location if it is within reach. */
	bool ExtinguishNear(const FVector& Location, float Reach = 250.f);

	bool IsRiggingLoose() const { return bRiggingLoose; }
	/** Amount is a fraction of the whole job; returns true when the repair is finished. */
	bool RepairRigging(float Amount);
	void TearRigging();
	float GetRiggingProgress() const { return RiggingProgress; }

	UPROPERTY(EditAnywhere) int32 MaxFires = 3;
	UPROPERTY(EditAnywhere) float FireDamagePerSecond = 0.08f;

protected:
	UPROPERTY() TObjectPtr<ADCRideDirector> Director;
	TArray<FDCFire> Fires;
	bool bRiggingLoose = false;
	float RiggingProgress = 0.f;
	float FireTimer = 8.f;
	float RiggingTimer = 12.f;
	float Clock = 0.f;
};
