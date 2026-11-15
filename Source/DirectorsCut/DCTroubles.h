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
	/** Three deliberate rag strikes put out a fresh fire. Partial work is retained. */
	float Suppression = 0.f;
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
	/** Applies one rag strike; returns true on contact, including a partial extinguish. */
	bool SwatNear(const FVector& Location, float Reach = 300.f);
	int32 SwatsRemaining(const FVector& Location, float Reach = 300.f) const;
	/** Pick an unoccupied, authored deck point for an incoming attack. */
	bool FindIgnitionPoint(int32 Preference, FVector& OutLocation) const;

	bool IsRiggingLoose() const { return bRiggingLoose; }
	/** Amount is a fraction of the whole job; returns true when the repair is finished. */
	bool RepairRigging(float Amount);
	void TearRigging();
	/** Clear transient trouble when replaying the boss clock or resolving its finale. */
	void ResetEncounterTroubles();
	float GetRiggingProgress() const { return RiggingProgress; }
	/** Resting-deck coordinates of the same station used by player and crew repair. */
	bool GetRiggingRepairLocation(FVector& Location) const;

	UPROPERTY(EditAnywhere) int32 MaxFires = 2;
	/** A fire left alone burns itself out after this long, having done its damage. */
	UPROPERTY(EditAnywhere) float FireBurnTime = 40.f;
	UPROPERTY(EditAnywhere) float FireDamagePerSecond = 0.08f;

protected:
	UPROPERTY() TObjectPtr<ADCRideDirector> Director;
	TArray<FDCFire> Fires;
	bool bRiggingLoose = false;
	float RiggingProgress = 0.f;
	float FireTimer = 8.f;
	float RiggingTimer = 12.f;
	float Clock = 0.f;
	void UpdateRiggingMarker();
	UPROPERTY() TObjectPtr<class UTextRenderComponent> RiggingLabel;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> RiggingArrow;
	bool bRiggingMarkerVisible = false;
};
