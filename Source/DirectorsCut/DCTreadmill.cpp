#include "DCTreadmill.h"
#include "Components/LocalFogVolumeComponent.h"
#include "DCRideDirector.h"
#include "DCHazards.h"
#include "DCCrew.h"
#include "DCTroubles.h"
#include "DCCloudFace.h"
#include "DCPortal.h"
#include "DCRuins.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EngineUtils.h"

ADCTreadmill::ADCTreadmill()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

ADCTreadmill* ADCTreadmill::Get(const UObject* WorldContext)
{
	if (UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr)
	{
		for (TActorIterator<ADCTreadmill> It(World); It; ++It)
		{
			return *It;
		}
	}
	return nullptr;
}

void ADCTreadmill::BeginPlay()
{
	Super::BeginPlay();
	Director = ADCRideDirector::Get(this);
	CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	ConeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
	CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
}

void ADCTreadmill::SpawnObject(bool bHostile, const FVector& Base, const FVector& Scale, UStaticMesh* Mesh, const FLinearColor& Color, float Radius, float Health)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* Actor = GetWorld()->SpawnActor<AStaticMeshActor>(Base, FRotator::ZeroRotator, Params);
	if (!Actor)
	{
		return;
	}
	UStaticMeshComponent* Comp = Actor->GetStaticMeshComponent();
	Comp->SetMobility(EComponentMobility::Movable);
	Comp->SetStaticMesh(Mesh);
	Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Actor->SetActorScale3D(Scale);
	if (BaseMaterial)
	{
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMaterial, Actor);
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		Comp->SetMaterial(0, MID);
	}

	FDCWorldObject& Obj = Objects.AddDefaulted_GetRef();
	Obj.Actor = Actor;
	Obj.Base = Base + FVector(SpawnAhead, 0.f, 0.f);
	Obj.Radius = Radius;
	Obj.bHostile = bHostile;
	Obj.Health = Health;
}

void ADCTreadmill::Register(AActor* Actor, const FVector& Base, float Radius, bool bHostile, float Health, float Yaw, bool bSolid)
{
	if (!Actor) { return; }
	FDCWorldObject& Obj = Objects.AddDefaulted_GetRef();
	Obj.Actor = Actor;
	Obj.Base = Base;
	Obj.Radius = Radius;
	Obj.bHostile = bHostile;
	Obj.Health = Health;
	Obj.Yaw = Yaw;
	Obj.bSolid = bSolid;
	Actor->SetActorLocation(Obj.Base);
}

void ADCTreadmill::SpawnForPhase(EDCPhase Phase, float DeltaSeconds)
{
	float ObstacleEvery = 0.f, HostileEvery = 0.f, SceneryEvery = 0.f;
	switch (Phase)
	{
	case EDCPhase::Fog: ObstacleEvery = 8.f; HostileEvery = bGhostShips ? 16.f : 0.f; SceneryEvery = 15.f; break;
	case EDCPhase::Hell: ObstacleEvery = 3.5f; HostileEvery = 9.f; SceneryEvery = 5.f; break;
	default: break; // the kraken owns the maelstrom
	}
	const bool bHell = Phase == EDCPhase::Hell;
	const float SeaZ = Director ? Director->SeaLevelZ : 0.f;
	const FLinearColor Stone = bHell ? FLinearColor(0.05f, 0.025f, 0.02f) : FLinearColor(0.07f, 0.085f, 0.08f);
	const FLinearColor Ember(9.f, 1.6f, 0.15f);
	UWorld* World = GetWorld();

	// Obstacles: in the lane, must be steered around.
	if (ObstacleEvery > 0.f && (ObstacleTimer -= DeltaSeconds) <= 0.f)
	{
		ObstacleTimer = ObstacleEvery * FMath::FRandRange(0.7f, 1.3f);
		// Sea space: add the lane offset so they land near the ship's present course.
		const FVector Base(SpawnDistance, LaneOffset + FMath::FRandRange(-2500.f, 2500.f), SeaZ);
		if (bHell || FMath::FRand() < 0.6f)
		{
			const float Radius = FMath::FRandRange(350.f, 750.f);
			AActor* Rock = (!bHell && DCRuins::Available()) ? DCRuins::SpawnCrag(World, Radius, FMath::FRandRange(1500.f, 3200.f)) : DCHazards::SpawnRock(World, Radius, FMath::FRandRange(1500.f, 4200.f), bHell, Stone);
			if (bHell) { DCHazards::SetGlow(Rock, Ember); }
			Register(Rock, Base, Radius, false, 1.f, FMath::FRandRange(0.f, 360.f), true);
		}
		else
		{
			const float Scale = FMath::FRandRange(0.7f, 1.15f);
			Register(DCRuins::Available() ? DCRuins::SpawnBrokenColumns(World, 1000.f * Scale) : DCHazards::SpawnRuin(World, Scale, Stone), Base, 1000.f * Scale, false, 1.f, FMath::FRandRange(-25.f, 25.f), true);
		}
	}

	// Scenery: off to the sides, never hit, there to loom.
	if (SceneryEvery > 0.f && (SceneryTimer -= DeltaSeconds) <= 0.f)
	{
		SceneryTimer = SceneryEvery * FMath::FRandRange(0.7f, 1.4f);
		const float Side = FMath::RandBool() ? 1.f : -1.f;
		const FVector Base(SpawnDistance, LaneOffset + Side * FMath::FRandRange(4500.f, 9000.f), SeaZ);
		if (!bHell && bSpecters && FMath::FRand() < 0.3f)
		{
			// Specters: pale giants walking on the water, turned toward the ship as it passes.
			Register(DCHazards::SpawnSpecter(World, FMath::FRandRange(2200.f, 5000.f), FLinearColor(0.7f, 1.5f, 1.7f), false), Base, 800.f, false, 1.f, Side > 0.f ? -90.f : 90.f, false);
		}
		else if (!bHell && FMath::FRand() < 0.55f)
		{
			// Statues face the passing ship.
			Register(DCRuins::Available() ? DCRuins::SpawnColossus(World, FMath::FRandRange(6000.f, 9000.f)) : DCHazards::SpawnSentinel(World, FMath::FRandRange(5000.f, 8000.f), FMath::RandBool(), Stone), Base - FVector(0.f, 0.f, 300.f), 1500.f, false, 1.f, Side > 0.f ? 205.f : 155.f, false);
		}
		else
		{
			const float Radius = FMath::FRandRange(900.f, 2200.f);
			AActor* Rock = DCHazards::SpawnRock(World, Radius, FMath::FRandRange(3000.f, 9000.f), bHell, Stone);
			if (bHell) { DCHazards::SetGlow(Rock, Ember); }
			Register(Rock, Base, Radius, false, 1.f, FMath::FRandRange(0.f, 360.f), false);
		}
	}

	// Hostiles: spectral ships that come alongside to be shot at.
	if (HostileEvery > 0.f && (HostileTimer -= DeltaSeconds) <= 0.f && NumHostiles() < 2)
	{
		HostileTimer = HostileEvery * FMath::FRandRange(0.7f, 1.3f);
		const float Side = FMath::RandBool() ? 1.f : -1.f;
		const FVector Base(SpawnDistance, Side * FMath::FRandRange(5000.f, 8500.f), SeaZ);
		if (bHell && FMath::RandBool())
		{
			// A demon wading in the burning sea, pacing the ship and hurling fire.
			AActor* Demon = DCHazards::SpawnSpecter(World, FMath::FRandRange(2800.f, 3800.f), FLinearColor(1.8f, 0.32f, 0.03f), true);
			if (UDCCrewSubsystem* Crew = World->GetSubsystem<UDCCrewSubsystem>())
			{
				Crew->Report(Side > 0.f ? TEXT("Demon in the fire to starboard!") : TEXT("Demon in the fire to port!"));
			}
			Register(Demon, FVector(SpawnDistance, Side * FMath::FRandRange(3800.f, 5500.f), SeaZ), 900.f, true, 6.f, Side > 0.f ? -90.f : 90.f, false);
		}
		else
		{
			AActor* Ghost = DCHazards::SpawnGhostShip(World, bHell ? FLinearColor(1.f, 0.25f, 0.03f) : FLinearColor(0.15f, 0.9f, 0.75f));
			if (UDCCrewSubsystem* Crew = World->GetSubsystem<UDCCrewSubsystem>())
			{
				Crew->Report(bHell ? (Side > 0.f ? TEXT("Burning ship to starboard!") : TEXT("Burning ship to port!")) : (Side > 0.f ? TEXT("Ghost ship to starboard!") : TEXT("Ghost ship to port!")));
			}
			Register(Ghost, Base, 1600.f, true, 6.f, FMath::FRandRange(-8.f, 8.f), false);
		}
	}
}
void ADCTreadmill::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Director)
	{
		Director = ADCRideDirector::Get(this);
		if (!Director) { return; }
	}

	const FDCEnvState& Env = Director->GetEnv();
	const float Speed = Env.ShipSpeed * Director->TimeScale;

	LaneOffset = FMath::Clamp(LaneOffset + Steer * LaneSpeed * SteerAuthority * DeltaSeconds, -LaneLimit, LaneLimit);

	if (!bOutliersBuilt)
	{
		bOutliersBuilt = true;
		SpawnOutliers();
	}
	// The fog begins at 58 s; until then the ship runs at the open-sea speed. (The test harness seeks
	// in the first frames, so wait a moment before trusting the ride time.)
	const float FogStart = 58.f;
	if (!bAtlantisBuilt && (Director->GetPhase() == EDCPhase::Fog || Director->RideTime > 2.5f))
	{
		bAtlantisBuilt = true;
		const bool bEarly = Director->GetPhase() == EDCPhase::OpenSea && Director->RideTime < FogStart;
		SpawnAhead = bEarly ? Env.ShipSpeed * (FogStart - Director->RideTime) : 0.f;
		const int32 Before = Objects.Num();
		SpawnAtlantis();
		SpawnAhead = 0.f;
		if (bEarly)
		{
			for (int32 i = Before; i < Objects.Num(); ++i)
			{
				if (AActor* A = Objects[i].Actor.Get()) { A->SetActorHiddenInGame(true); AtlantisHidden.Add(A); }
			}
		}
	}
	if (AtlantisHidden.Num() && Director->GetPhase() != EDCPhase::OpenSea)
	{
		// A couple a frame: showing them all at once still cost a quarter of a second. They are inside the front and out of sight meanwhile.
		for (int32 n = 0; n < 2 && AtlantisHidden.Num(); ++n)
		{
			if (AActor* A = AtlantisHidden.Pop().Get()) { A->SetActorHiddenInGame(false); }
		}
	}
	SpawnForPhase(Director->GetPhase(), DeltaSeconds * Director->TimeScale);
	TickIncoming(DeltaSeconds);
	for (int32 w = Wrecks.Num() - 1; w >= 0; --w)
	{
		FDCWreck& Wreck = Wrecks[w];
		Wreck.Age += DeltaSeconds;
		AActor* Hulk = Wreck.Actor.Get();
		if (!Hulk || Wreck.Age > 4.f)
		{
			if (Hulk) { Hulk->Destroy(); }
			Wrecks.RemoveAtSwap(w);
			continue;
		}
		// Settle, heel over and slide under, falling astern.
		Hulk->AddActorWorldOffset(FVector(-Speed * 0.5f * DeltaSeconds, 0.f, -260.f * Wreck.Age * DeltaSeconds));
		Hulk->AddActorLocalRotation(FRotator(-9.f * DeltaSeconds, 0.f, 14.f * DeltaSeconds));
	}

	const EDCPhase Now = Director->GetPhase();
	const bool bLeavingAtlantis = Now == EDCPhase::Slide || Now == EDCPhase::Breach || Now == EDCPhase::Maelstrom || Now == EDCPhase::Ending;
	const FTransform Rock = Director->GetWorldRock();
	for (int32 i = Objects.Num() - 1; i >= 0; --i)
	{
		FDCWorldObject& Obj = Objects[i];
		AActor* Actor = Obj.Actor.Get();
		if (!Actor)
		{
			Objects.RemoveAtSwap(i);
			continue;
		}

		if (Obj.bExternal)
		{
			// Owned and moved elsewhere; just keep our sea-space copy of its position current.
			Obj.Base = Actor->GetActorLocation() + FVector(0.f, LaneOffset, 0.f);
			continue;
		}


		// Once the sea rolls up, nothing from Atlantis follows the ship in. Ghost ships founder at
		// once (alongside in the tunnel they filled half the view from the helm); the rest goes under
		// as the walls close, so the breach opens on an empty bowl.
		if (bLeavingAtlantis)
		{
			if (Obj.bHostile)
			{
				Wrecks.Add({ Actor, 0.f });
				Objects.RemoveAtSwap(i);
				continue;
			}
			if (Env.Tube > 0.5f || Director->GetPhase() != EDCPhase::Slide)
			{
				Obj.Base.Z -= 30000.f * DeltaSeconds * Director->TimeScale;
				if (Obj.Base.Z < -160000.f)
				{
					Actor->Destroy();
					Objects.RemoveAtSwap(i);
					continue;
				}
			}
		}

		// Hostiles pace the ship once alongside; obstacles sweep past.
		if (Obj.bHostile)
		{
			const float TargetX = 0.f;
			Obj.Base.X = FMath::FInterpConstantTo(Obj.Base.X, TargetX, DeltaSeconds, Obj.Base.X > 4000.f ? Speed : Speed * 0.08f);
			if (Obj.Base.X <= 1.f)
			{
				Obj.Base.X -= Speed * 0.3f * DeltaSeconds;
			}
			// Alongside, an untouched hostile keeps shooting. Sinking it is the only way to stop that.
			if (!Obj.bExternal && FMath::Abs(Obj.Base.X) < 5000.f)
			{
				Obj.AttackTimer -= DeltaSeconds;
				if (Obj.AttackTimer <= 0.f)
				{
					Obj.AttackTimer = FMath::FRandRange(10.f, 14.f);
					LaunchAt(Actor->GetActorLocation() + FVector(0.f, 0.f, 700.f));
				}
			}
		}
		else
		{
			Obj.Base.X -= Speed * DeltaSeconds;
		}

		// Ships that pace us hold station on the ship itself; everything else is fixed in the sea and slides with the helm.
		const bool bPacing = Obj.bHostile && !Obj.bExternal;
		// Radius 0 marks a destination (the portal): it stays dead ahead whatever the helm does.
		FVector Local(Obj.Base.X, (bPacing || Obj.Radius < 1.f) ? Obj.Base.Y : Obj.Base.Y - LaneOffset, Obj.Base.Z);
		if (!Obj.bHostile && !Obj.bSolid && Obj.Radius > 1.f)
		{
			// Scenery cannot be hit, so never let the helm carry the ship through it: it keeps its distance.
			const float MinY = ShipHalfWidth + Obj.Radius + 1500.f;
			if (FMath::Abs(Local.Y) < MinY) { Local.Y = (Obj.Base.Y - LaneOffset >= 0.f ? 1.f : -1.f) * MinY; }
		}
		Actor->SetActorLocationAndRotation(Rock.TransformPosition(Local), Rock.GetRotation() * FRotator(0.f, Obj.Yaw, 0.f).Quaternion());

		// Lookout: call an obstacle once, when it is close enough to matter and near our course.
		if (!Obj.bHostile && Obj.bSolid && !Obj.bCalled && Local.X < 13000.f && FMath::Abs(Local.Y) < ShipHalfWidth + Obj.Radius + 150.f)
		{
			Obj.bCalled = true;
			if (UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>())
			{
				Crew->Report(FMath::Abs(Local.Y) < 250.f ? TEXT("Rocks dead ahead!") : (Local.Y > 0.f ? TEXT("Rocks ahead, off the starboard bow!") : TEXT("Rocks ahead, off the port bow!")));
			}
		}

		if (!Obj.bHostile && Obj.bSolid && FMath::Abs(Local.X) < ShipHalfLength && FMath::Abs(Local.Y) < ShipHalfWidth + Obj.Radius)
		{
			Director->DamageShip(3.f);
			if (UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>()) { Crew->Report(TEXT("We struck the rocks! Hull is breached!"), true); }
			Actor->Destroy();
			Objects.RemoveAtSwap(i);
			continue;
		}

		// Scenery is big and seen from far off, so it is kept until it is well astern.
		if (Local.X < -(DespawnDistance + ((Obj.bSolid || Obj.bHostile) ? 0.f : 90000.f)))
		{
			Actor->Destroy();
			Objects.RemoveAtSwap(i);
		}
	}
}

bool ADCTreadmill::FindObstacleAhead(float MaxDistance, float& OutX, float& OutY) const
{
	float BestX = MaxDistance;
	bool bFound = false;
	for (const FDCWorldObject& Obj : Objects)
	{
		if (Obj.bHostile || !Obj.bSolid) { continue; }
		const float X = Obj.Base.X;
		const float Y = Obj.Base.Y - LaneOffset;
		if (X > ShipHalfLength && X < BestX && FMath::Abs(Y) < ShipHalfWidth + Obj.Radius + 600.f)
		{
			BestX = X; OutX = X; OutY = Y; bFound = true;
		}
	}
	return bFound;
}

int32 ADCTreadmill::FindHostile(EDCSide Side, float MaxRange) const
{
	int32 Best = INDEX_NONE;
	float BestDist = MaxRange;
	for (int32 i = 0; i < Objects.Num(); ++i)
	{
		const FDCWorldObject& Obj = Objects[i];
		if (!Obj.bHostile) { continue; }
		const float Y = Obj.bExternal ? Obj.Base.Y - LaneOffset : Obj.Base.Y;
		if ((Side == EDCSide::Port && Y > 0.f) || (Side == EDCSide::Starboard && Y < 0.f)) { continue; }
		const float Dist = FVector2D(Obj.Base.X, Y).Size();
		if (Dist < BestDist)
		{
			BestDist = Dist; Best = i;
		}
	}
	return Best;
}

FVector ADCTreadmill::GetObjectLocation(int32 Index) const
{
	if (Objects.IsValidIndex(Index) && Objects[Index].Actor.IsValid())
	{
		return Objects[Index].Actor->GetActorLocation();
	}
	return FVector::ZeroVector;
}

bool ADCTreadmill::DamageObject(int32 Index, float Damage)
{
	if (!Objects.IsValidIndex(Index)) { return false; }
	FDCWorldObject& Obj = Objects[Index];
	Obj.Health -= Damage;
	if (Obj.Health <= 0.f)
	{
		if (AActor* Actor = Obj.Actor.Get())
		{
			// Kraken arms are retired by their owner. A ship goes down by the stern instead of vanishing.
			if (Obj.bExternal) { Actor->Destroy(); }
			else
			{
				Wrecks.Add({ Actor, 0.f });
				if (UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>()) { Crew->Report(TEXT("She is going down! Good shooting!")); }
			}
		}
		Objects.RemoveAtSwap(Index);
		if (Director) { Director->Score += 100; }
		return true;
	}
	return false;
}

int32 ADCTreadmill::NumHostiles() const
{
	int32 N = 0;
	for (const FDCWorldObject& Obj : Objects) { if (Obj.bHostile) { ++N; } }
	return N;
}

int32 ADCTreadmill::DamageNear(const FVector& WorldPos, float Radius, float Damage, bool bDirect)
{
	int32 Hits = 0;
	for (int32 i = Objects.Num() - 1; i >= 0; --i)
	{
		const FDCWorldObject& Obj = Objects[i];
		const AActor* Actor = Obj.Actor.Get();
		if (!Obj.bHostile || !Actor) { continue; }
		const float Reach = Radius + Obj.Radius * (bDirect ? 0.6f : 1.f);
		if (FVector::DistSquared(Actor->GetActorLocation(), WorldPos) < Reach * Reach)
		{
			DamageObject(i, Damage);
			++Hits;
			if (bDirect) { break; }
		}
	}
	return Hits;
}

void ADCTreadmill::AddExternalHostile(AActor* Actor, float Radius, float Health)
{
	FDCWorldObject& Obj = Objects.AddDefaulted_GetRef();
	Obj.Actor = Actor;
	Obj.Base = Actor ? Actor->GetActorLocation() : FVector::ZeroVector;
	Obj.Radius = Radius;
	Obj.bHostile = true;
	Obj.bExternal = true;
	Obj.Health = Health;
}

float ADCTreadmill::FindClearLane(float Lookahead) const
{
	float Best = LaneOffset;
	float BestCost = TNumericLimits<float>::Max();
	for (float Candidate = -LaneLimit; Candidate <= LaneLimit; Candidate += 250.f)
	{
		// Prefer staying put, and drift back toward the middle when nothing is in the way.
		float Cost = FMath::Abs(Candidate - LaneOffset) * 0.0005f + FMath::Abs(Candidate) * 0.00005f;
		for (const FDCWorldObject& Obj : Objects)
		{
			if (Obj.bHostile || !Obj.bSolid) { continue; }
			if (Obj.Base.X < -ShipHalfLength || Obj.Base.X > Lookahead) { continue; }
			const float Gap = FMath::Abs(Obj.Base.Y - Candidate) - (ShipHalfWidth + Obj.Radius);
			if (Gap < 500.f)
			{
				// Nearer obstacles count for more; overlapping the hull counts for much more than a tight squeeze.
				Cost += (Gap < 0.f ? 10.f : 2.f) * (1.f + 8000.f / (Obj.Base.X + 2500.f));
			}
		}
		if (Cost < BestCost) { BestCost = Cost; Best = Candidate; }
	}
	return Best;
}

void ADCTreadmill::LaunchAt(const FVector& From)
{
	// A spectral shot lobbed from a hostile ship onto our deck.
	const bool bHell = Director && Director->GetPhase() == EDCPhase::Hell;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* Shot = GetWorld()->SpawnActor<AStaticMeshActor>(From, FRotator::ZeroRotator, Params);
	if (!Shot) { return; }
	UStaticMeshComponent* Comp = Shot->GetStaticMeshComponent();
	Comp->SetMobility(EComponentMobility::Movable);
	Comp->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
	Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Comp->SetCastShadow(false);
	Shot->SetActorScale3D(FVector(1.3f));
	if (UMaterialInterface* Glow = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Bolt.M_Bolt")))
	{
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Glow, Shot);
		MID->SetVectorParameterValue(TEXT("Color"), bHell ? FLinearColor(40.f, 8.f, 0.6f) : FLinearColor(3.f, 22.f, 18.f));
		Comp->SetMaterial(0, MID);
	}

	FDCIncoming& In = Incoming.AddDefaulted_GetRef();
	In.Actor = Shot;
	In.From = From;
	In.To = FVector(FMath::FRandRange(-900.f, 900.f), FMath::FRandRange(-220.f, 220.f), (Director ? Director->SeaLevelZ : 0.f) + 420.f);
	In.Duration = FMath::Clamp(FVector::Dist(In.From, In.To) / 4500.f, 1.2f, 3.f);

	if (UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>())
	{
		Crew->Report(From.Y > 0.f ? TEXT("They are firing on us from starboard!") : TEXT("They are firing on us from port!"));
	}
}

void ADCTreadmill::TickIncoming(float DeltaSeconds)
{
	for (int32 i = Incoming.Num() - 1; i >= 0; --i)
	{
		FDCIncoming& In = Incoming[i];
		In.T += DeltaSeconds / In.Duration;
		AActor* Shot = In.Actor.Get();
		if (In.T >= 1.f || !Shot)
		{
			if (Shot) { Shot->Destroy(); }
			if (Director)
			{
				Director->DamageShip(1.f);
				// In the hell channel their shot is fire, and sometimes it catches.
				if (Director->GetPhase() == EDCPhase::Hell && FMath::FRand() < 0.3f)
				{
					ADCTroubles* Troubles = ADCTroubles::Get(this);
					if (Troubles && Troubles->DeckPoints.Num() && Troubles->NumFires() < Troubles->MaxFires)
					{
						Troubles->StartFire(Troubles->DeckPoints[FMath::RandRange(0, Troubles->DeckPoints.Num() - 1)]);
					}
				}
			}
			Incoming.RemoveAtSwap(i);
			continue;
		}
		FVector P = FMath::Lerp(In.From, In.To, In.T);
		P.Z += FMath::Sin(In.T * UE_PI) * 1400.f;
		Shot->SetActorLocation(P);
	}
}

void ADCTreadmill::SpawnOutliers()
{
	// The first sign of the drowned city, out on the open sea in full sun before the storm closes in:
	// a broken arch, a lone statue, a run of columns, passed close on either hand in the first minute.
	if (!DCRuins::Available() || FParse::Param(FCommandLine::Get(), TEXT("DCNoOutliers"))) { return; }
	UWorld* World = GetWorld();
	const float SeaZ = Director ? Director->SeaLevelZ : 0.f;
	Register(DCRuins::SpawnArch(World, 3, 2.6f), FVector(21000.f, -10500.f, SeaZ - 250.f), 2000.f, false, 1.f, 70.f, false);
	Register(DCRuins::SpawnColossus(World, 11000.f), FVector(33000.f, 9000.f, SeaZ - 300.f), 2000.f, false, 1.f, 215.f, false);
	Register(DCRuins::SpawnBrokenColumns(World, 1500.f), FVector(27000.f, 5200.f, SeaZ), 2000.f, false, 1.f, 30.f, false);
	Register(DCRuins::SpawnColonnade(World, 6, 7000.f), FVector(47000.f, -13000.f, SeaZ - 200.f), 2000.f, false, 1.f, 25.f, false);
	Register(DCRuins::SpawnBrokenColumns(World, 1800.f), FVector(52000.f, 8000.f, SeaZ), 2000.f, false, 1.f, -15.f, false);
	Register(DCRuins::SpawnCrag(World, 1600.f, 5200.f), FVector(40000.f, -22000.f, SeaZ), 2000.f, false, 1.f, 40.f, false);
	Register(DCRuins::SpawnArch(World, 1, 2.2f), FVector(63000.f, -9500.f, SeaZ - 250.f), 2000.f, false, 1.f, 100.f, false);
	Register(DCRuins::SpawnColonnade(World, 5, 8000.f), FVector(72000.f, 12500.f, SeaZ - 200.f), 2000.f, false, 1.f, -20.f, false);
}

void ADCTreadmill::SpawnAtlantis()
{
	// The drowned city, laid out once along the two or three kilometres the ship covers in the fog.
	// Positions are in sea space (cm ahead of the ship now, cm to starboard), so each landmark comes
	// up out of the haze in its turn: sentinels flanking the course, aqueducts crossing behind them,
	// the high temple far ahead to starboard, and vast figures standing in the cloud beyond.
	UWorld* World = GetWorld();
	const float SeaZ = Director ? Director->SeaLevelZ : 0.f;
	const FLinearColor Stone(0.085f, 0.1f, 0.11f);
	const bool bRuins = DCRuins::Available();

	struct FSentinel { float X, Y, Height; bool bTrident; };
	const FSentinel Sentinels[] = {
		{ 70000.f, -12000.f, 11000.f, false }, { 76000.f, 13000.f, 11500.f, false },
		{ 150000.f, 17000.f, 15000.f, true }, { 160000.f, -16000.f, 13000.f, false },
		{ 240000.f, -14000.f, 14000.f, true }, { 252000.f, 15000.f, 16000.f, false } };
	for (const FSentinel& S : Sentinels)
	{
		// Turned to watch the ship come.
		Register(bRuins ? DCRuins::SpawnColossus(World, S.Height * 1.5f) : DCHazards::SpawnSentinel(World, S.Height, S.bTrident, Stone), FVector(S.X, S.Y, SeaZ - 300.f), 2000.f, false, 1.f, S.Y > 0.f ? 205.f : 155.f, false);
	}

	Register(bRuins ? DCRuins::SpawnColonnade(World, 9, 9000.f) : DCHazards::SpawnAqueduct(World, 8, 5500.f, Stone), FVector(95000.f, 21000.f, SeaZ - 200.f), 2000.f, false, 1.f, 18.f, false);
	Register(bRuins ? DCRuins::SpawnColonnade(World, 12, 11000.f) : DCHazards::SpawnAqueduct(World, 10, 7000.f, Stone), FVector(185000.f, -27000.f, SeaZ - 200.f), 2000.f, false, 1.f, -22.f, false);
	Register(bRuins ? DCRuins::SpawnColonnade(World, 8, 10000.f) : DCHazards::SpawnAqueduct(World, 7, 6000.f, Stone), FVector(275000.f, 24000.f, SeaZ - 200.f), 2000.f, false, 1.f, 62.f, false);

	Register(bRuins ? DCRuins::SpawnTemple(World, 9.f) : DCHazards::SpawnTemple(World, 1.f, Stone), FVector(345000.f, 36000.f, SeaZ), 2000.f, false, 1.f, bRuins ? 76.f : -14.f, false);
	if (bRuins)
	{
		// The city proper: gates and arches the course threads between, temples on both hands, all at
		// several depths so that there is always something large coming up out of the mist.
		struct FPiece { int32 Kind; float X, Y, Scale, Yaw; };   // Kind 1..4 arch variants, 5 gate, 6 temple
		const FPiece City[] = {
			{ 5, 60000.f, 0.f, 5.f, 90.f },
			{ 2, 88000.f, -24000.f, 6.f, 60.f }, { 6, 112000.f, 30000.f, 5.f, 100.f }, { 4, 132000.f, -30000.f, 6.f, 75.f },
			{ 1, 172000.f, 26000.f, 5.f, 110.f }, { 6, 205000.f, -40000.f, 7.f, 70.f }, { 3, 222000.f, 22000.f, 6.f, 95.f },
			{ 5, 262000.f, 0.f, 9.f, 90.f }, { 4, 300000.f, -32000.f, 8.f, 80.f }, { 1, 318000.f, 30000.f, 8.f, 100.f } };
		// Sanctuaries: whole complexes on platforms, set back on either hand and turned to face the course.
		struct FSite { float X, Y, Scale; int32 Seed; };
		const FSite Sites[] = { { 92000.f, -30000.f, 3.6f, 11 }, { 140000.f, 34000.f, 4.2f, 23 }, { 200000.f, -40000.f, 5.f, 37 }, { 262000.f, 46000.f, 6.f, 41 }, { 330000.f, -60000.f, 8.f, 53 } };
		for (const FSite& S : Sites)
		{
			Register(DCRuins::SpawnComplex(World, S.Scale, S.Seed), FVector(S.X, S.Y, SeaZ), 2000.f, false, 1.f, S.Y > 0.f ? -90.f : 90.f, false);
		}
		for (const FPiece& P : City)
		{
			AActor* Built = P.Kind == 5 ? DCRuins::SpawnGate(World, P.Scale) : P.Kind == 6 ? DCRuins::SpawnTemple(World, P.Scale) : DCRuins::SpawnArch(World, P.Kind, P.Scale);
			// A gate stands across the course (radius 0: it is sailed through, not pushed aside).
			Register(Built, FVector(P.X, P.Y, SeaZ), P.Kind == 5 ? 0.f : 2000.f, false, 1.f, P.Yaw, false);
		}
	}

	// Fog banks lying on the water among the ruins: local fog volumes, each a low wide drift a couple
	// of hundred metres across, so that the city stands in mist at several depths and the low sun
	// rakes across the tops of the banks. Radius 0: the ship sails through them.
	if (!FParse::Param(FCommandLine::Get(), TEXT("DCNoBanks")))
	{
		float BankDensity = 1.f;
		FParse::Value(FCommandLine::Get(), TEXT("DCBankDensity="), BankDensity);
		FRandomStream Rng(613);
		for (int32 i = 0; i < 16; ++i)
		{
			const float X = 45000.f + i * 19000.f + Rng.FRandRange(-5000.f, 5000.f);
			const float Side = (i % 3 == 0) ? 0.f : ((i % 2) ? 1.f : -1.f);
			const float Y = Side * Rng.FRandRange(9000.f, 42000.f) + Rng.FRandRange(-4000.f, 4000.f);
			const float Radius = Rng.FRandRange(9000.f, 24000.f);
			const float Thick = Rng.FRandRange(1200.f, 3500.f);
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AActor* Bank = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
			ULocalFogVolumeComponent* C = NewObject<ULocalFogVolumeComponent>(Bank);
			C->SetMobility(EComponentMobility::Movable);
			Bank->SetRootComponent(C);
			C->SetWorldScale3D(FVector(Radius / 500.f));
			C->RadialFogExtinction = 0.1f * BankDensity;
			C->HeightFogExtinction = 0.16f * BankDensity;
			C->HeightFogFalloff = 100.f * Radius / Thick;
			C->FogPhaseG = 0.6f;
			C->FogAlbedo = FLinearColor(0.78f, 0.84f, 0.92f);
			C->RegisterComponent();
			Register(Bank, FVector(X, Y, SeaZ), 0.f, false, 1.f, 0.f, false);
		}
	}

	// The portal out of the city: on the ship's course, at the distance covered by the time the slide begins.
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ADCPortal* Portal = World->SpawnActor<ADCPortal>(FVector::ZeroVector, FRotator::ZeroRotator, Params))
		{
			// Radius 0 marks it as something the ship is meant to sail into, so it is not pushed aside.
			Register(Portal, FVector(PortalDistance, 0.f, SeaZ), 0.f, false, 1.f, 0.f, false);
		}
	}

	// Faces in the clouds, high above the city, turned toward the ship and gazing down at it.
	struct FFace { float X, Y, Z, Height, Yaw; bool bBearded; };
	const FFace Faces[] = {
		{ 280000.f, 100000.f, 55000.f, 75000.f, 195.f, true }, { 350000.f, -120000.f, 60000.f, 85000.f, 165.f, false },
		{ 440000.f, 50000.f, 90000.f, 110000.f, 182.f, true } };
	for (const FFace& F : Faces)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ADCCloudFace* Face = World->SpawnActor<ADCCloudFace>(FVector::ZeroVector, FRotator::ZeroRotator, Params))
		{
			Face->Build(F.Height, F.bBearded, 14.f);
			Register(Face, FVector(F.X, F.Y, SeaZ + F.Z), 2000.f, false, 1.f, F.Yaw, false);
		}
	}

	// Presences in the cloud: figures a kilometre tall, seen only as pale shapes far off.
	if (!bSpecters) { return; }
	const FLinearColor Pale(0.22f, 0.3f, 0.38f);
	Register(DCHazards::SpawnSpecter(World, 95000.f, Pale, false), FVector(310000.f, -120000.f, SeaZ - 30000.f), 2000.f, false, 1.f, 150.f, false);
	Register(DCHazards::SpawnSpecter(World, 110000.f, Pale, false), FVector(430000.f, 95000.f, SeaZ - 35000.f), 2000.f, false, 1.f, 215.f, false);
	Register(DCHazards::SpawnSpecter(World, 80000.f, Pale, false), FVector(230000.f, 140000.f, SeaZ - 25000.f), 2000.f, false, 1.f, 240.f, false);
}
