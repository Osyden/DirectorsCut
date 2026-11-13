#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCTypes.h"
#include "DCTreadmill.generated.h"

class ADCRideDirector;
class UStaticMesh;
class UMaterialInterface;

/** One thing in the moving world: an obstacle in the lane, or a hostile off to one side. */
struct FDCWorldObject
{
	TWeakObjectPtr<AActor> Actor;
	/** Position in "sea space": X ahead of the ship, Y to starboard, before helm offset and rocking. */
	FVector Base = FVector::ZeroVector;
	float Radius = 300.f;
	bool bHostile = false;
	float Health = 1.f;
	/** Moved by its owner (e.g. a kraken arm), not by the treadmill. */
	bool bExternal = false;
	/** Heading in degrees, on top of the sea rocking. */
	float Yaw = 0.f;
	/** False for scenery that the ship cannot hit. */
	bool bSolid = true;
	/** The lookout has already called this one. */
	bool bCalled = false;
	/** Seconds until a hostile alongside fires on us again. */
	float AttackTimer = 8.f;
};

/**
 * Moves the world past the stationary ship. Obstacles and hostiles spawn ahead, travel aft at
 * ship speed, and are removed behind. Steering shifts the whole world sideways (LaneOffset).
 */
UCLASS()
class ADCTreadmill : public AActor
{
	GENERATED_BODY()

public:
	ADCTreadmill();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	static ADCTreadmill* Get(const UObject* WorldContext);

	/** -1 (port) .. +1 (starboard). Set every frame by whoever has the helm. */
	void SetSteer(float InSteer) { Steer = FMath::Clamp(InSteer, -1.f, 1.f); }
	float GetSteer() const { return Steer; }
	float GetLaneOffset() const { return LaneOffset; }
	/** How fast the ship is crossing the sea sideways as a fraction of full helm (-1 port .. 1 starboard): she gathers way into a turn and carries it out of one. */
	float GetTurn() const { return LaneSpeed > 0.f ? LaneVelocity / LaneSpeed : 0.f; }

	/** Nearest obstacle ahead of the bow. Returns false if the lane is clear. OutY is relative to the ship centreline. */
	bool FindObstacleAhead(float MaxDistance, float& OutX, float& OutY) const;
	/** The lane offset with the most room among the obstacles within Lookahead, close to the present course. */
	float FindClearLane(float Lookahead) const;
	/** Index of the best hostile on a side, or INDEX_NONE. */
	int32 FindHostile(EDCSide Side, float MaxRange) const;
	FVector GetObjectLocation(int32 Index) const;
	/** Returns true if the hostile was destroyed. */
	bool DamageObject(int32 Index, float Damage);
	/** Damage hostiles near a world position. bDirect uses a tighter radius and stops at the first hit. Returns hits. */
	int32 DamageNear(const FVector& WorldPos, float Radius, float Damage, bool bDirect);
	/** Register a hostile that something else moves. The treadmill destroys the actor when its health runs out. */
	void AddExternalHostile(AActor* Actor, float Radius, float Health);
	int32 NumObjects() const { return Objects.Num(); }
	int32 NumHostiles() const;

	UPROPERTY(EditAnywhere) float SpawnDistance = 30000.f;
	UPROPERTY(EditAnywhere) float DespawnDistance = 12000.f;
	UPROPERTY(EditAnywhere) float LaneLimit = 5000.f;
	UPROPERTY(EditAnywhere) float LaneSpeed = 900.f;
	/** 1 = full helm response; lowered while the rigging is loose. */
	float SteerAuthority = 1.f;
	UPROPERTY(EditAnywhere) float ShipHalfLength = 2200.f;
	UPROPERTY(EditAnywhere) float ShipHalfWidth = 500.f;

protected:
	void SpawnObject(bool bHostile, const FVector& Base, const FVector& Scale, UStaticMesh* Mesh, const FLinearColor& Color, float Radius, float Health);
	void SpawnForPhase(EDCPhase Phase, float DeltaSeconds);
	void Register(AActor* Actor, const FVector& Base, float Radius, bool bHostile, float Health, float Yaw, bool bSolid);
	float SceneryTimer = 3.f;
	void SpawnAtlantis();
	bool bAtlantisBuilt = false;
	/** The drowned city is built in the first seconds of the voyage, further off by the distance still to
	 *  run, and kept hidden until the fog: building it all in the frame the fog began froze the game for half a second. */
	TArray<TWeakObjectPtr<AActor>> AtlantisHidden;
	float SpawnAhead = 0.f;
	bool bOutliersBuilt = false;
	void SpawnOutliers();
	/** How far ahead the portal stands when the city is laid out: the distance sailed before the slide starts. */
	UPROPERTY(EditAnywhere) float PortalDistance = 287000.f;
	/** Ghost ships and the walking giants are placeholders the owner has switched off for now (12 Nov). */
	UPROPERTY(EditAnywhere) bool bGhostShips = false;
	UPROPERTY(EditAnywhere) bool bSpecters = false;

	UPROPERTY() TObjectPtr<ADCRideDirector> Director;
	UPROPERTY() TObjectPtr<UStaticMesh> CubeMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> ConeMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> CylinderMesh;
	UPROPERTY() TObjectPtr<UMaterialInterface> BaseMaterial;

	TArray<FDCWorldObject> Objects;

	/** A hostile shot in flight toward our deck. */
	struct FDCIncoming { TWeakObjectPtr<AActor> Actor; FVector From; FVector To; float T = 0.f; float Duration = 2.f; };
	TArray<FDCIncoming> Incoming;
	void LaunchAt(const FVector& From);
	void TickIncoming(float DeltaSeconds);

	/** A sunk hostile on its way under. */
	struct FDCWreck { TWeakObjectPtr<AActor> Actor; float Age = 0.f; };
	TArray<FDCWreck> Wrecks;
	float Steer = 0.f;
	float LaneOffset = 0.f;
	float LaneVelocity = 0.f;
	float ObstacleTimer = 4.f;
	float HostileTimer = 8.f;
};
